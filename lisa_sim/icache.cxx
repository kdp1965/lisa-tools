#include "icache.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

void InstCache::configure(int lines, int words, int penalty, bool direct, bool stream, bool window) {
    if (const char *w = getenv("LISA_ICACHE_WORD"))
        FILL_PER_WORD = atoi(w);
    window_  = window;
    depth_   = std::max(words, WINDOW_AHEAD + 1);
    if (window) {
        lines = 0;
        stream = false;
    }
    words_   = std::min(std::max(words, 1), MAX_WORDS);
    lines_.assign(lines > 0 ? lines : 0, Line());
    stream_  = stream;
    direct_  = direct;
    penalty_ = penalty > 0 ? penalty : (stream || window) ? FILL_OVERHEAD + FILL_PER_WORD : default_penalty(words_);
    on_      = stream || window || lines > 0;
    reset();
}

void InstCache::reset() {
    for (auto& l : lines_) l = Line();
    fifo_.clear();
    wready_.clear();
    wactive_ = false;
    wbase_ = wcur_ = 0;
    next_addr_ = 0;
    engine_free_at_ = 0;
    fetches_ = misses_ = misses_seq_ = streamed_ = stream_wait_ = insts_ = exec_clocks_ = stall_clocks_ = stamp_ = 0;
    pinned_ = 0;
    prof_.clear();
    last_pc_ = 0xFFFF;
    spin_pc_ = 0xFFFF;
}

InstCache::Line* InstCache::lookup(uint16_t pc) {
    for (auto& l : lines_) {
        if (l.mask && pc >= l.base && pc < (uint16_t)(l.base + words_) && (l.mask >> (pc - l.base)) & 1)
            return &l;
    }
    return nullptr;
}

// The line a fill or a streamed block at base goes into: the direct-mapped
// one, or an empty one, or the least recently used.
InstCache::Line* InstCache::allocate(uint16_t base) {
    Line* victim;
    if (direct_) {
        victim = &lines_[(base / words_) % lines_.size()];
    } else {
        victim = &lines_[0];
        for (auto& l : lines_) {
            if (!l.mask) { victim = &l; break; }
            if (l.used < victim->used) victim = &l;
        }
    }
    victim->base = base;
    victim->mask = 0;
    victim->used = stamp_;
    return victim;
}

void InstCache::store(uint16_t pc) {
    if (lines_.empty()) return;
    uint16_t base = (uint16_t)(pc - pc % words_);
    Line* l = nullptr;
    for (auto& c : lines_)
        if (c.mask && c.base == base) { l = &c; break; }
    if (!l) l = allocate(base);
    l->mask |= 1u << (pc - base);
    l->used = stamp_;
}

void InstCache::top_up() {
    uint64_t now = exec_clocks_ + stall_clocks_;
    while ((int)fifo_.size() < STREAM_FIFO) {
        uint64_t ready = std::max(engine_free_at_, now) + FILL_PER_WORD;
        fifo_.push_back({next_addr_, ready});
        engine_free_at_ = ready;
        next_addr_ = (uint16_t)((next_addr_ + 1) & 0x7FFF);
    }
}

void InstCache::window_push(uint64_t now) {
    if ((int)wready_.size() == depth_) {    // full: the oldest goes
        wready_.pop_front();
        wbase_++;
    }
    uint64_t ready = std::max(engine_free_at_, now) + FILL_PER_WORD;
    wready_.push_back(ready);
    engine_free_at_ = ready;
}

bool InstCache::write_profile(const std::string& path) const {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    for (auto& kv : prof_)
        fprintf(f, "%06X %llu %llu\n", kv.first, (unsigned long long)kv.second.first,
                (unsigned long long)kv.second.second);
    fclose(f);
    return true;
}

void InstCache::window_fetch(uint32_t pc, bool sequential) {
    for (auto& r : pins_)
        if (pc >= r.first && pc <= r.second) {  // the locked store: no flash, the window untouched
            pinned_++;
            return;
        }
    uint64_t stall0 = stall_clocks_;
    bool restart = false;
    uint64_t now = exec_clocks_ + stall_clocks_;
    uint32_t end = wbase_ + (uint32_t)wready_.size();
    if (wactive_ && pc >= wbase_ && pc < end) {
        // in the window: a word kept behind (a loop) or one the burst ran ahead to
        if (pc > wcur_) streamed_++;
    } else if (wactive_ && pc >= end && pc < end + WINDOW_REACH) {
        while (wbase_ + wready_.size() <= pc)       // the burst delivers up to it
            window_push(now);
        streamed_++;
    } else {
        // a restart: end the read, select, command, address, dummies, the word
        misses_++;
        restart = true;
        if (sequential) misses_seq_++;
        stall_clocks_ += penalty_;
        wready_.assign(1, exec_clocks_ + stall_clocks_);
        wbase_ = pc;
        engine_free_at_ = exec_clocks_ + stall_clocks_;
        wactive_ = true;
    }
    uint64_t ready = wready_[pc - wbase_];
    now = exec_clocks_ + stall_clocks_;
    if (ready > now) {
        stream_wait_ += ready - now;
        stall_clocks_ += ready - now;
    }
    wcur_ = pc;
    if (restart || stall_clocks_ > stall0) {
        auto& e = prof_[pc];
        e.first += restart;
        e.second += stall_clocks_ - stall0;
    }
    // run ahead: at most WINDOW_AHEAD words past the one the core is on; a
    // full window drops its oldest word, unless that is the core's
    now = exec_clocks_ + stall_clocks_;
    while ((int)(wbase_ + wready_.size() - 1 - wcur_) < WINDOW_AHEAD) {
        if ((int)wready_.size() == depth_ && wcur_ == wbase_) break;
        window_push(now);
    }
}

void InstCache::fetch(uint32_t pc) {
    if (!on_ || pc == spin_pc_) return;
    fetches_++;
    stamp_++;
    if (window_) {
        bool sequential = last_pc_ != 0xFFFF && (pc & 0x7FFF) == (uint16_t)((last_pc_ + 1) & 0x7FFF);
        window_fetch(pc, sequential);
        return;
    }
    if (Line* l = lookup(pc)) {
        l->used = stamp_;
        return;
    }
    bool sequential = last_pc_ != 0xFFFF && pc == (uint16_t)((last_pc_ + 1) & 0x7FFF);

    if (stream_) {
        // The next word of the burst, or one the engine has run ahead to?
        auto it = std::find_if(fifo_.begin(), fifo_.end(), [pc](const Pending& p) { return p.addr == pc; });
        if (it != fifo_.end()) {
            uint64_t now = exec_clocks_ + stall_clocks_;
            if (it->ready > now) {
                stream_wait_ += it->ready - now;
                stall_clocks_ += it->ready - now;
            }
            fifo_.erase(fifo_.begin(), it + 1);
            streamed_++;
        } else {
            // A restart: end the read, select, command, address, dummies, the word
            misses_++;
            if (sequential) misses_seq_++;
            stall_clocks_ += penalty_;
            fifo_.clear();
            engine_free_at_ = exec_clocks_ + stall_clocks_;
            next_addr_ = (uint16_t)((pc + 1) & 0x7FFF);
        }
        store(pc);
        top_up();
        return;
    }

    // A fill: the line that holds pc (the chip's line starts at pc; a
    // direct-mapped one at the aligned word) replaces a line.
    misses_++;
    if (sequential) misses_seq_++;
    stall_clocks_ += penalty_;
    Line* l = allocate(direct_ ? (uint16_t)(pc - pc % words_) : pc);
    l->mask = words_ >= 32 ? 0xFFFFFFFFu : (1u << words_) - 1;
}

void InstCache::retire(uint16_t pc, uint16_t pc_next, int sim_cycles) {
    if (!on_ || pc == spin_pc_) return;
    if (pc_next == pc)              // a branch to itself: the program is done, spinning
        spin_pc_ = pc;              // (counted once; its fetches from now on are not)
    insts_++;
    last_pc_ = pc;
    int clocks = 4;                                     // fetch, predecode, decode, execute
    if (pc_next != (uint16_t)((pc + 1) & 0x7FFF))
        clocks = 3;                                     // the PC is loaded in decode: no execute state
    if (sim_cycles > 1)
        clocks += 1;                                    // the second stage of a two-stage instruction
    exec_clocks_ += clocks;
}

std::string InstCache::describe() const {
    if (!on_) return "off";
    char buf[160];
    int n = 0;
    if (window_)
        return std::string("streaming window of ") + std::to_string(depth_) + " words, " + std::to_string(WINDOW_AHEAD) +
               " ahead (loops of " + std::to_string(depth_ - WINDOW_AHEAD) + " or fewer), restart " +
               std::to_string(penalty_) + " clocks, " + std::to_string(FILL_PER_WORD) + " per word";
    if (lines_.empty())
        n = snprintf(buf, sizeof buf, "no cache");
    else
        n = snprintf(buf, sizeof buf, "%zu line%s of %d words, %s",
                     lines_.size(), lines_.size() == 1 ? "" : "s", words_,
                     direct_ ? "direct-mapped" : stream_ || lines_.size() > 1 ? "LRU" : "as the chip's");
    if (stream_)
        snprintf(buf + n, sizeof buf - n, ", streaming fetch: restart %d clocks, %d per word, %d ahead",
                 penalty_, FILL_PER_WORD, STREAM_FIFO);
    else
        snprintf(buf + n, sizeof buf - n, ", fill %d clocks", penalty_);
    return buf;
}

std::string InstCache::report() const {
    if (!on_) return "icache: off\n";
    uint64_t hits  = fetches_ - misses_ - streamed_;
    uint64_t total = exec_clocks_ + stall_clocks_;
    char buf[640];
    int n = snprintf(buf, sizeof buf, "icache: %s\n", describe().c_str());
    n += snprintf(buf + n, sizeof buf - n,
                  "  instructions %llu  fetches %llu  hits %.1f%%  misses %llu (%.1f per 1000 instructions: "
                  "%llu streaming, %llu after jumps)\n",
                  (unsigned long long)insts_, (unsigned long long)fetches_,
                  fetches_ ? 100.0 * hits / fetches_ : 0.0,
                  (unsigned long long)misses_,
                  insts_ ? 1000.0 * misses_ / insts_ : 0.0,
                  (unsigned long long)misses_seq_, (unsigned long long)(misses_ - misses_seq_));
    if (window_ && !pins_.empty())
        n += snprintf(buf + n, sizeof buf - n, "  pinned %llu fetches (%.1f%%) from %zu locked ranges\n",
                      (unsigned long long)pinned_, fetches_ ? 100.0 * pinned_ / fetches_ : 0.0, pins_.size());
    if (stream_ || window_)
        n += snprintf(buf + n, sizeof buf - n,
                      "  streamed %llu words (%.1f%% of the fetches), waited %llu clocks for them\n",
                      (unsigned long long)streamed_, fetches_ ? 100.0 * streamed_ / fetches_ : 0.0,
                      (unsigned long long)stream_wait_);
    snprintf(buf + n, sizeof buf - n,
             "  clocks %llu: %llu executing + %llu stalled (%.0f%%), %.1f per instruction\n",
             (unsigned long long)total, (unsigned long long)exec_clocks_, (unsigned long long)stall_clocks_,
             total ? 100.0 * stall_clocks_ / total : 0.0,
             insts_ ? (double)total / insts_ : 0.0);
    return buf;
}
