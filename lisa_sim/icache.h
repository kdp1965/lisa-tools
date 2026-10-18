#ifndef LISA_ICACHE_H
#define LISA_ICACHE_H

// A model of the LISA instruction fetch path, to count what a bigger cache
// or a different fetch would buy before building it.
//
// The chip's inst_cache.v is a single line of 4 instructions: a miss
// fetches the missed word and the 3 after it from the flash, and the line
// starts at the missed word (it is not aligned).  The fill is a QSPI
// command (8 clocks of SCLK, single bit), a 24-bit address (6 clocks in
// quad), the dummy clocks (10 at the debug registers' defaults) and 16 bits
// per word (4 clocks), with SCLK at half the core clock, plus a clock of
// handshake per state and per word (lisa_qqspi.v, inst_cache.v): about 57
// core clocks of overhead and 10 per word, 97 for the 4-word line.  In
// single-bit SPI it would be about twice that.
//
// Line fills (the chip's way): N lines of W words, fully associative with
// least-recently-used replacement (flops), or direct-mapped on aligned
// lines (a RAM macro indexed by the PC); a miss stalls for the whole fill.
//
// Streaming fetch (`stream`): the flash read is never ended while the core
// takes the next sequential word - the engine keeps clocking words into a
// small FIFO, 10 clocks apart, and the core waits only for the word it
// needs; a word that is not the next one (a jump, or a return to code the
// stream has passed) restarts the burst at the fill overhead plus one word.
// The N lines of W words, if any, are a cache of the words that came
// through (aligned blocks, filled a word at a time), hit before the FIFO is
// looked at, so loops run from it while the burst serves straight-line
// code.
//
// The model sees every instruction word the core reads and estimates the
// clocks the core would spend: 4 per instruction (fetch, predecode, decode,
// execute), 3 for a taken jump, branch, call or return (the PC is loaded in
// decode), one more for the second stage of a two-stage instruction, plus
// the stalls.  A store's extra fetch clock (the delayed data write) is not
// counted, nor is the branch to itself a finished program spins in (the
// counts stop there), nor the data cache's traffic on the same QSPI bus.
// Misses are split into those on the word after the previous instruction
// (the code streaming through the line) and those after a jump.
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

class InstCache {
public:
    static constexpr int FILL_OVERHEAD = 57;    // core clocks per fill before the first word
    static constexpr int FILL_PER_WORD = 10;    // ... and per 16-bit word
    static constexpr int STREAM_FIFO   = 4;     // words the streaming engine runs ahead
    static constexpr int MAX_WORDS     = 32;    // per line
    static int default_penalty(int words) { return FILL_OVERHEAD + FILL_PER_WORD * words; }

    // lines 0: no cache (only meaningful with stream); penalty 0 takes the
    // default (the fill of a line, or a restart: the overhead plus a word);
    // direct implies aligned lines.
    void configure(int lines, int words = 4, int penalty = 0, bool direct = false, bool stream = false);
    void reset();                                   // invalidate, zero the counters
    bool enabled() const { return on_; }

    void fetch(uint32_t pc);                                    // the core reads the instruction word at pc
    void retire(uint16_t pc, uint16_t pc_next, int sim_cycles); // ... and has executed it; pc_next follows

    uint64_t fetches()      const { return fetches_; }
    uint64_t misses()       const { return misses_; }
    uint64_t instructions() const { return insts_; }
    uint64_t exec_clocks()  const { return exec_clocks_; }
    uint64_t stall_clocks() const { return stall_clocks_; }

    std::string describe() const;   // "4 lines of 4 words, LRU, fill 97 clocks"
    std::string report() const;     // the counters, several lines

private:
    struct Line {
        uint16_t base  = 0;         // first word in the line
        uint32_t mask  = 0;         // which of its words are valid (all, for a fill)
        uint64_t used  = 0;         // last hit (for LRU)
    };
    struct Pending {                // a word the streaming engine has been told to read
        uint16_t addr;
        uint64_t ready;             // the clock it is available at
    };
    bool     on_      = false;
    std::vector<Line> lines_;
    int      words_   = 4;
    int      penalty_ = 97;
    bool     direct_  = false;
    bool     stream_  = false;
    std::deque<Pending> fifo_;
    uint16_t next_addr_      = 0;   // the word the engine reads next
    uint64_t engine_free_at_ = 0;   // when it has delivered the last one it was told to
    uint16_t last_pc_ = 0xFFFF;     // the previous instruction's PC (0xFFFF: none)
    uint16_t spin_pc_ = 0xFFFF;     // a branch to itself: the program is done, its fetches are not counted
    uint64_t fetches_ = 0, misses_ = 0, misses_seq_ = 0, streamed_ = 0, stream_wait_ = 0,
             insts_ = 0, exec_clocks_ = 0, stall_clocks_ = 0, stamp_ = 0;

    Line* lookup(uint16_t pc);
    Line* allocate(uint16_t base);
    void  store(uint16_t pc);       // a streamed word goes into the cache
    void  top_up();                 // keep the engine STREAM_FIFO words ahead
};

#endif
