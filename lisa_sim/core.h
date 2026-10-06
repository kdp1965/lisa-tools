#ifndef LISA_CORE_H
#define LISA_CORE_H

#include <cstdint>
#include <string>
#include <functional>

class LisaMemory;
struct BF16Unit;
struct LisaDiv;

// Forward declaration for peripherals
class LisaPeripherals;

// CPU state visible to debugger
struct LisaCoreState {
    uint8_t  a;           // Accumulator (8-bit)
    uint16_t ix;          // Index register (15-bit, stored in 16)
    bool     ix_cond;     // IX condition bit
    uint16_t sp;          // Stack pointer (15-bit)
    uint16_t pc;          // Program counter (15-bit)
    uint16_t ra;          // Return address (15-bit)
    bool     ra_cond;     // RA condition bit
    uint16_t ia;          // Interrupt return address (15-bit)
    bool     zflag;       // Zero flag
    bool     cflag;       // Carry flag
    uint8_t  cond;        // Condition register (2-bit)
    uint8_t  amode;       // Arithmetic mode (3-bit)
    bool     ie;          // Interrupt enable
    bool     halted;      // CPU halted (by BRK or debug)
    uint64_t cycle_count; // Total cycles executed
    uint64_t inst_count;  // Total instructions retired
    uint16_t last_inst;   // Last executed instruction word
};

class LisaCore {
public:
    static constexpr int PC_BITS = 15;
    static constexpr int D_BITS  = 15;
    static constexpr int PC_MASK = (1 << PC_BITS) - 1;
    static constexpr int D_MASK  = (1 << D_BITS) - 1;

    LisaCore();

    void reset();

    // Connect to memory and peripherals
    void set_memory(LisaMemory* mem) { mem_ = mem; }
    void set_peripherals(LisaPeripherals* periph) { periph_ = periph; }
    void set_bf16(BF16Unit* bf16) { bf16_ = bf16; }
    void set_div(LisaDiv* div) { div_ = div; }

    // Execute one full instruction (returns number of cycles taken)
    int step();

    // Get/set state for debugger
    LisaCoreState get_state() const;
    void set_state(const LisaCoreState& state);

    // Debug control
    void halt()    { halted_ = true; }
    void resume()  { halted_ = false; }
    bool is_halted() const { return halted_; }

    // Breakpoints
    static constexpr int MAX_BREAKPOINTS = 6;
    bool add_breakpoint(uint16_t addr);
    bool remove_breakpoint(uint16_t addr);
    void clear_breakpoints();
    bool is_breakpoint(uint16_t addr) const;
    int  get_breakpoints(uint16_t* addrs, int max) const;

    // Interrupt request (bits correspond to int sources 0-7)
    void set_interrupt(uint8_t mask);
    void clear_interrupt(uint8_t mask);
    void set_int_enable(uint8_t mask) { int_en_ = mask; }

    // Trace callback
    using TraceCallback = std::function<void(uint16_t pc, uint16_t inst, const LisaCoreState& state)>;
    void set_trace_callback(TraceCallback cb) { trace_cb_ = cb; }

private:
    // Registers
    uint8_t  a_;
    uint16_t ix_;
    bool     ix_cond_;
    uint16_t sp_;
    uint16_t pc_;
    uint16_t ra_;
    bool     ra_cond_;
    uint16_t ia_;
    bool     zflag_;
    bool     cflag_;
    bool     cflag_save_;
    bool     signed_inv_;
    bool     signed_inv_save_;
    uint8_t  cond_;       // 2-bit
    uint8_t  amode_;      // 3-bit
    bool     ie_;
    bool     halted_;

    // ISR saved state
    bool     cflag_isr_;
    bool     zflag_isr_;
    bool     signed_inv_isr_;

    // BF16 registers are in BF16Unit

    // Statistics
    uint64_t cycle_count_;
    uint64_t inst_count_;
    uint16_t last_inst_;

    // Breakpoints
    uint16_t breakpoints_[MAX_BREAKPOINTS];
    bool     bp_active_[MAX_BREAKPOINTS];

    // Interrupts
    uint8_t  int_pending_;
    uint8_t  int_en_;

    // Connected modules
    LisaMemory*       mem_;
    LisaPeripherals*  periph_;
    BF16Unit*         bf16_;
    LisaDiv*          div_;
    TraceCallback     trace_cb_;

    // Internal helpers
    uint8_t  data_read(uint16_t addr, bool periph);
    void     data_write(uint16_t addr, uint8_t val, bool periph);
    void     update_flags_zc(uint8_t result, bool carry);
    void     update_flag_z(uint8_t result);
    bool     eval_condition(uint8_t cc, bool s) const;
    int16_t  sign_extend_11(uint16_t val) const;
    int16_t  sign_extend_10(uint16_t val) const;
    int8_t   sign_extend_8(uint8_t val) const;
    void     do_interrupt();
    void     push_byte(uint8_t val);
    uint8_t  pop_byte();
};

#endif
