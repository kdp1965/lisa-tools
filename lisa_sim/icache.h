#ifndef LISA_ICACHE_H
#define LISA_ICACHE_H

// A model of the LISA instruction cache, to count what a bigger one would
// buy before building it.
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
// This model is N lines of W words: fully associative with
// least-recently-used replacement (flops), or direct-mapped on aligned
// lines (a RAM macro indexed by the PC).  It sees every instruction word
// the core reads and estimates the clocks the core would spend: 4 per
// instruction (fetch, predecode, decode, execute), 3 for a taken jump,
// branch, call or return (the PC is loaded in decode), one more for the
// second stage of a two-stage instruction, plus the fill per miss.  A
// store's extra fetch clock (the delayed data write) is not counted, nor is
// the branch to itself a finished program spins in (the counts stop there).
// Misses are split into those on the word after the previous instruction
// (the code streaming through the line) and those after a jump.
#include <cstdint>
#include <string>
#include <vector>

class InstCache {
public:
    static constexpr int FILL_OVERHEAD = 57;    // core clocks per fill before the first word
    static constexpr int FILL_PER_WORD = 10;    // ... and per 16-bit word
    static int default_penalty(int words) { return FILL_OVERHEAD + FILL_PER_WORD * words; }

    // lines 0 turns the model off; penalty 0 takes the default for the line
    // length; direct implies aligned.
    void configure(int lines, int words = 4, int penalty = 0, bool direct = false);
    void reset();                                   // invalidate the lines, zero the counters
    bool enabled() const { return !lines_.empty(); }

    void fetch(uint16_t pc);                                    // the core reads the instruction word at pc
    void retire(uint16_t pc, uint16_t pc_next, int sim_cycles); // ... and has executed it; pc_next follows

    uint64_t fetches()      const { return fetches_; }
    uint64_t misses()       const { return misses_; }
    uint64_t instructions() const { return insts_; }
    uint64_t exec_clocks()  const { return exec_clocks_; }
    uint64_t stall_clocks() const { return misses_ * (uint64_t)penalty_; }

    std::string describe() const;   // "4 lines of 4 words, LRU, fill 97 clocks"
    std::string report() const;     // the counters, several lines

private:
    struct Line {
        uint16_t base  = 0;         // first word in the line
        bool     valid = false;
        uint64_t used  = 0;         // last hit (for LRU)
    };
    std::vector<Line> lines_;
    int      words_   = 4;
    int      penalty_ = 97;
    bool     direct_  = false;
    uint16_t last_pc_ = 0xFFFF;     // the previous instruction's PC (0xFFFF: none)
    uint64_t fetches_ = 0, misses_ = 0, misses_seq_ = 0, insts_ = 0, exec_clocks_ = 0, stamp_ = 0;
};

#endif
