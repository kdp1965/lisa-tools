#include "icache.h"
#include <cstdio>

void InstCache::configure(int lines, int words, int penalty, bool direct) {
    lines_.assign(lines > 0 ? lines : 0, Line());
    words_   = words > 0 ? words : 4;
    penalty_ = penalty > 0 ? penalty : default_penalty(words_);
    direct_  = direct;
    reset();
}

void InstCache::reset() {
    for (auto& l : lines_) l = Line();
    fetches_ = misses_ = misses_seq_ = insts_ = exec_clocks_ = stamp_ = 0;
    last_pc_ = 0xFFFF;
}

void InstCache::fetch(uint16_t pc) {
    if (lines_.empty()) return;
    fetches_++;
    stamp_++;
    for (auto& l : lines_) {
        if (l.valid && pc >= l.base && pc < (uint16_t)(l.base + words_)) {
            l.used = stamp_;
            return;
        }
    }

    // A miss: the fill brings the line that holds pc (the chip's line starts
    // at pc; a direct-mapped one at the aligned word) into the line it
    // replaces.
    misses_++;
    if (last_pc_ != 0xFFFF && pc == (uint16_t)((last_pc_ + 1) & 0x7FFF))
        misses_seq_++;
    Line* victim;
    if (direct_) {
        victim = &lines_[(pc / words_) % lines_.size()];
    } else {
        victim = &lines_[0];
        for (auto& l : lines_) {
            if (!l.valid) { victim = &l; break; }
            if (l.used < victim->used) victim = &l;
        }
    }
    victim->base  = direct_ ? (uint16_t)(pc - pc % words_) : pc;
    victim->valid = true;
    victim->used  = stamp_;
}

void InstCache::retire(uint16_t pc, uint16_t pc_next, int sim_cycles) {
    if (lines_.empty()) return;
    if (pc_next == pc) {            // a branch to itself: the program is done, spinning
        fetches_--;                 // (the word is in the cache by then)
        return;
    }
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
    if (lines_.empty()) return "off";
    char buf[128];
    snprintf(buf, sizeof buf, "%zu line%s of %d words, %s, fill %d clocks",
             lines_.size(), lines_.size() == 1 ? "" : "s", words_,
             direct_ ? "direct-mapped" : lines_.size() == 1 ? "as the chip's" : "LRU",
             penalty_);
    return buf;
}

std::string InstCache::report() const {
    if (lines_.empty()) return "icache: off\n";
    uint64_t hits  = fetches_ - misses_;
    uint64_t stall = stall_clocks();
    uint64_t total = exec_clocks_ + stall;
    char buf[512];
    int n = snprintf(buf, sizeof buf, "icache: %s\n", describe().c_str());
    n += snprintf(buf + n, sizeof buf - n,
                  "  instructions %llu  fetches %llu  hits %.1f%%  misses %llu (%.1f per 1000 instructions: "
                  "%llu streaming, %llu after jumps)\n",
                  (unsigned long long)insts_, (unsigned long long)fetches_,
                  fetches_ ? 100.0 * hits / fetches_ : 0.0,
                  (unsigned long long)misses_,
                  insts_ ? 1000.0 * misses_ / insts_ : 0.0,
                  (unsigned long long)misses_seq_, (unsigned long long)(misses_ - misses_seq_));
    snprintf(buf + n, sizeof buf - n,
             "  clocks %llu: %llu executing + %llu in fills (%.0f%% stalled), %.1f per instruction\n",
             (unsigned long long)total, (unsigned long long)exec_clocks_, (unsigned long long)stall,
             total ? 100.0 * stall / total : 0.0,
             insts_ ? (double)total / insts_ : 0.0);
    return buf;
}
