#include "core.h"
#include "memory.h"
#include "periph.h"
#include "bf16.h"
#include "div.h"

#include <cstring>

LisaCore::LisaCore() : mem_(nullptr), periph_(nullptr), bf16_(nullptr), div_(nullptr) {
    clear_breakpoints();
    reset();
}

void LisaCore::reset() {
    a_ = 0;
    ix_ = 0;  ix_cond_ = false;
    sp_ = D_MASK;
    pc_ = 0;
    ra_ = 0;  ra_cond_ = false;
    ia_ = 0;
    zflag_ = false;
    cflag_ = false;
    cflag_save_ = false;
    signed_inv_ = false;
    signed_inv_save_ = false;
    cond_ = 0x03;  // Both bits set = unconditional execution
    amode_ = 0x01;
    ie_ = false;
    halted_ = false;
    cflag_isr_ = false;
    zflag_isr_ = false;
    signed_inv_isr_ = false;
    int_pending_ = 0;
    int_en_ = 0;
    cycle_count_ = 0;
    inst_count_ = 0;
    last_inst_ = 0;
}

// --- Memory access helpers ---

uint8_t LisaCore::data_read(uint16_t addr, bool periph) {
    if (periph && periph_)
        return periph_->read(addr & 0x1FF);
    if (mem_)
        return mem_->data_read(addr & D_MASK);
    return 0;
}

void LisaCore::data_write(uint16_t addr, uint8_t val, bool periph) {
    if (periph && periph_)
        periph_->write(addr & 0x1FF, val);
    else if (mem_)
        mem_->data_write(addr & D_MASK, val);
}

void LisaCore::update_flags_zc(uint8_t result, bool carry) {
    zflag_ = (result == 0);
    cflag_ = carry;
}

void LisaCore::update_flag_z(uint8_t result) {
    zflag_ = (result == 0);
}

bool LisaCore::eval_condition(uint8_t cc, bool s) const {
    // Per RTL: cflag_signed = (inst[5] & signed_inversion) ? ~cflag : cflag
    bool cflag_s = (s && signed_inv_) ? !cflag_ : cflag_;
    switch (cc & 7) {
    case 0: return zflag_;                    // EQ
    case 1: return !zflag_;                   // NE
    case 2: return !cflag_;                   // NC
    case 3: return cflag_;                    // C
    case 4: return !cflag_s && !zflag_;       // GT
    case 5: return cflag_s && !zflag_;        // LT
    case 6: return !cflag_s || zflag_;        // GE
    case 7: return cflag_s || zflag_;         // LE
    }
    return false;
}

int16_t LisaCore::sign_extend_11(uint16_t val) const {
    if (val & 0x400)
        return (int16_t)(val | 0xF800);
    return (int16_t)(val & 0x7FF);
}

int16_t LisaCore::sign_extend_10(uint16_t val) const {
    if (val & 0x200)
        return (int16_t)(val | 0xFC00);
    return (int16_t)(val & 0x3FF);
}

int8_t LisaCore::sign_extend_8(uint8_t val) const {
    return (int8_t)val;
}

void LisaCore::push_byte(uint8_t val) {
    if (mem_) mem_->data_write(sp_ & D_MASK, val);
    sp_ = (sp_ - 1) & D_MASK;
}

uint8_t LisaCore::pop_byte() {
    sp_ = (sp_ + 1) & D_MASK;
    return mem_ ? mem_->data_read(sp_ & D_MASK) : 0;
}

void LisaCore::do_interrupt() {
    if (!ie_ || !(cond_ & 2)) return;

    uint8_t active = int_pending_ & int_en_;
    if (periph_) active |= periph_->get_interrupts();
    if (!active) return;

    // Save state
    ia_ = (pc_) & PC_MASK;
    ie_ = false;
    cflag_isr_ = cflag_;
    zflag_isr_ = zflag_;
    signed_inv_isr_ = signed_inv_;

    // Priority-encoded vector
    uint16_t vector = 9;
    for (int i = 0; i < 8; i++) {
        if (active & (1 << i)) {
            vector = i + 1;
            break;
        }
    }
    pc_ = vector & PC_MASK;
    cond_ = 0x03;
}

void LisaCore::set_interrupt(uint8_t mask) { int_pending_ |= mask; }
void LisaCore::clear_interrupt(uint8_t mask) { int_pending_ &= ~mask; }

bool LisaCore::add_breakpoint(uint16_t addr) {
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (!bp_active_[i]) {
            breakpoints_[i] = addr & PC_MASK;
            bp_active_[i] = true;
            return true;
        }
    }
    return false;
}

bool LisaCore::remove_breakpoint(uint16_t addr) {
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (bp_active_[i] && breakpoints_[i] == (addr & PC_MASK)) {
            bp_active_[i] = false;
            return true;
        }
    }
    return false;
}

void LisaCore::clear_breakpoints() {
    for (int i = 0; i < MAX_BREAKPOINTS; i++) bp_active_[i] = false;
}

bool LisaCore::is_breakpoint(uint16_t addr) const {
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (bp_active_[i] && breakpoints_[i] == (addr & PC_MASK))
            return true;
    }
    return false;
}

int LisaCore::get_breakpoints(uint16_t* addrs, int max) const {
    int n = 0;
    for (int i = 0; i < MAX_BREAKPOINTS && n < max; i++) {
        if (bp_active_[i])
            addrs[n++] = breakpoints_[i];
    }
    return n;
}

LisaCoreState LisaCore::get_state() const {
    return {
        a_, ix_, ix_cond_, sp_, pc_, ra_, ra_cond_, ia_,
        zflag_, cflag_, cond_, amode_, ie_, halted_,
        cycle_count_, inst_count_, last_inst_
    };
}

void LisaCore::set_state(const LisaCoreState& s) {
    a_ = s.a;  ix_ = s.ix;  ix_cond_ = s.ix_cond;
    sp_ = s.sp;  pc_ = s.pc;  ra_ = s.ra;  ra_cond_ = s.ra_cond;
    ia_ = s.ia;  zflag_ = s.zflag;  cflag_ = s.cflag;
    cond_ = s.cond;  amode_ = s.amode;  ie_ = s.ie;  halted_ = s.halted;
}

// =========================================================================
// Execute one instruction. Returns cycle count.
// =========================================================================
int LisaCore::step() {
    if (halted_) return 1;

    // Check breakpoint
    if (is_breakpoint(pc_)) {
        halted_ = true;
        return 1;
    }

    // Fetch instruction
    uint16_t inst = mem_ ? mem_->inst_read(pc_ & PC_MASK) : 0;
    last_inst_ = inst;
    int cycles = 1;

    // Trace callback
    if (trace_cb_) {
        trace_cb_(pc_, inst, get_state());
    }

    // Execute only if cond_[0] is true
    bool should_exec = cond_ & 1;
    if (!should_exec)
    {
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;

        // Advance PC ... operation skipped
        pc_ = (pc_ + 1) & PC_MASK;
        if ((inst >> 4) == 0xA18) {
            // Skip argument to LDX
            pc_ = (pc_ + 1) & PC_MASK;
        }
        return cycles;
    }

    // Decode helper extractions
    uint8_t  imm8    = inst & 0xFF;
    uint16_t uimm9   = inst & 0x1FF;
    uint8_t  top7    = (inst >> 9) & 0x7F;
    bool     base_sp = (inst >> 9) & 1;    // inst[9]: 0=IX, 1=SP
    bool     p_bit   = (inst >> 9) & 1;    // peripheral select for direct addressing

    // Compute data address for indexed ops
    uint16_t ix_addr = (ix_ + uimm9) & D_MASK;
    uint16_t sp_addr = (sp_ + uimm9) & D_MASK;
    uint16_t d_addr  = base_sp ? sp_addr : ix_addr;

    // ---- Top-level decode (MSB first) ----

    // === inst[15]==0: JAL (jump and link) ===
    if ((inst & 0x8000) == 0) {
        uint16_t target = inst & PC_MASK;
        ra_ = (pc_ + 1) & PC_MASK;
        ra_cond_ = (cond_ >> 1) & 1;
        pc_ = target;
        cond_ = 0x03;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === Branches: inst[15:11] ===
    uint8_t top5 = (inst >> 11) & 0x1F;

    if (top5 == 0x16) {  // 10110 = BR (unconditional)
        int16_t offset = sign_extend_11(inst & 0x7FF);
        pc_ = (pc_ + offset) & PC_MASK;
        cond_ = 0x03;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top5 == 0x15) {  // 10101 = BNZ
        // Per RTL: cond resets to 3 only when the branch is actually taken;
        // not-taken falls to the normal shift.
        int16_t offset = sign_extend_11(inst & 0x7FF);
        if (!zflag_) {
            pc_ = (pc_ + offset) & PC_MASK;
            cond_ = 0x03;
        } else {
            pc_ = (pc_ + 1) & PC_MASK;
            cond_ = 0x02 | (cond_ >> 1);
        }
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top5 == 0x17) {  // 10111 = BZ
        int16_t offset = sign_extend_11(inst & 0x7FF);
        if (zflag_) {
            pc_ = (pc_ + offset) & PC_MASK;
            cond_ = 0x03;
        } else {
            pc_ = (pc_ + 1) & PC_MASK;
            cond_ = 0x02 | (cond_ >> 1);
        }
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === Returns: check before general decode ===
    // ret #imm8: inst[15:10]==100011
    uint8_t top6 = (inst >> 10) & 0x3F;
    if (top6 == 0x23) {  // 100011 = RET #imm8
        a_ = imm8;
        pc_ = ra_ & PC_MASK;
        cond_ = 0x02 | (ra_cond_ ? 1 : 0);  // cond[1]=1, cond[0]=ra_cond
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // ret: inst[15:7]==100010100
    if ((inst >> 7) == 0x114) {  // 100010100 = RET
        pc_ = ra_ & PC_MASK;
        cond_ = 0x02 | (ra_cond_ ? 1 : 0);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // rc: inst[15:6]==1000101100 = return if C
    uint16_t top10 = (inst >> 6) & 0x3FF;
    if (top10 == 0x22C) {  // 1000101100
        if (cflag_) {
            pc_ = ra_ & PC_MASK;
            cond_ = 0x02 | (ra_cond_ ? 1 : 0);
        } else {
            pc_ = (pc_ + 1) & PC_MASK;
            cond_ = 0x02 | (cond_ >> 1);
        }
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // rz: inst[15:6]==1000101110 = return if Z
    if (top10 == 0x22E) {  // 1000101110
        if (zflag_) {
            pc_ = ra_ & PC_MASK;
            cond_ = 0x02 | (ra_cond_ ? 1 : 0);
        } else {
            pc_ = (pc_ + 1) & PC_MASK;
            cond_ = 0x02 | (cond_ >> 1);
        }
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // rets: inst[15:6]==1000101101 = return from ISR
    if (top10 == 0x22D) {  // 1000101101
        pc_ = ia_;
        ie_ = true;
        cflag_ = cflag_isr_;
        zflag_ = zflag_isr_;
        signed_inv_ = signed_inv_isr_;
        cond_ = 0x03;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === call ix / jmp ix: inst[15:5] ===
    uint16_t top11 = (inst >> 5) & 0x7FF;
    if (top11 == 0x454) {  // 10001010100 = CALL IX
        // Per RTL: PC<=IX, RA<=PC+1, ra_cond<=cond[1], then IX<=IX+1 with ix_cond<=cond[1].
        ra_ = (pc_ + 1) & PC_MASK;
        ra_cond_ = (cond_ >> 1) & 1;
        bool new_ix_cond = (cond_ >> 1) & 1;
        pc_ = ix_ & PC_MASK;
        ix_ = (ix_ + 1) & PC_MASK;
        ix_cond_ = new_ix_cond;
        cond_ = 0x03;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top11 == 0x455) {  // 10001010101 = JMP IX
        pc_ = ix_ & PC_MASK;
        cond_ = 0x03;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === Exchange operations: inst[15:2] ===
    uint16_t top14 = (inst >> 2) & 0x3FFF;
    if (top14 == 0x22B0) {  // 10001010110000 = XCHG RA
        if (cond_ & 1) {
            uint16_t t = ix_;  ix_ = ra_;  ra_ = t;
            bool tc = ix_cond_;  ix_cond_ = ra_cond_;  ra_cond_ = tc;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top14 == 0x22B1) {  // 10001010110001 = XCHG IA
        // Per RTL: ix_cond becomes 1 (IA has no companion cond bit).
        if (cond_ & 1) {
            uint16_t t = ix_;  ix_ = ia_;  ia_ = t;
            ix_cond_ = true;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top14 == 0x22B2) {  // 10001010110010 = XCHG SP
        // Per RTL: ix_cond becomes 1 after the swap.
        if (cond_ & 1) {
            uint16_t t = ix_;  ix_ = sp_;  sp_ = t;
            ix_cond_ = true;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top14 == 0x22B3) {  // 10001010110011 = SPIX (IX <= SP)
        // Per RTL: ix_cond becomes 1.
        if (cond_ & 1) {
            ix_ = sp_;
            ix_cond_ = true;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top14 == 0x22B4) {  // 10001010110100 = CPX RA
        if (cond_ & 1) {
            zflag_ = (ix_ == ra_);
            cflag_ = (ix_ < ra_);
            signed_inv_ = false;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top14 == 0x22B6) {  // 10001010110110 = CPX SP
        if (cond_ & 1) {
            zflag_ = (ix_ == sp_);
            cflag_ = (ix_ < sp_);
            signed_inv_ = false;
        }
        pc_ = (pc_ + 1) & PC_MASK;
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === Immediate / accumulator ops by top6 ===
    // Advance PC by default (overridden if needed)
    pc_ = (pc_ + 1) & PC_MASK;

    if (top6 == 0x20) {  // 100000 = LDI a, #imm8
        if (cond_ & 1) {
            a_ = imm8;
            cflag_ = false;
            signed_inv_ = false;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x24) {  // 100100 = ADC a, #imm8
        // Per RTL (TT07 silicon): the operand imm8 + C is formed in 8 bits
        // before the 9-bit add, so adc #0xff with C=1 adds 0 and loses its
        // carry.  signed_inversion = a[7] XOR imm[7].
        if (cond_ & 1) {
            uint8_t adder = (uint8_t)(imm8 + (cflag_ ? 1 : 0));
            uint16_t sum = (uint16_t)a_ + adder;
            signed_inv_ = ((a_ ^ imm8) >> 7) & 1;
            a_ = sum & 0xFF;
            cflag_ = (sum >> 8) & 1;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x25) {  // 100101 = ADS sp, #simm10
        // Per RTL: SP += sext(inst[9:0]) — 10-bit signed offset
        if (cond_ & 1) {
            sp_ = (sp_ + sign_extend_10(inst & 0x3FF)) & D_MASK;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x26) {  // 100110 = ADX ix, #simm10
        // Per RTL: IX += sext(inst[9:0]) — 10-bit signed offset; ix_cond <= 1
        if (cond_ & 1) {
            ix_ = (ix_ + sign_extend_10(inst & 0x3FF)) & PC_MASK;
            ix_cond_ = true;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x27) {  // 100111 = DCX [base+uimm9]
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint8_t result = val - 1;
            data_write(d_addr, result, false);
            cflag_ = (val == 0);     // Borrow: was 0, wraps to 0xFF
            signed_inv_ = false;
            zflag_ = (result == 0);  // Z set when new value is 0 (RTL checks old d_i==0x01)
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x29) {  // 101001 = CPI a, #imm8
        // Per RTL: cflag is always the unsigned borrow (a < imm).
        // signed_inversion = a[7] XOR imm[7] (sign-bit disagreement), captured
        // unconditionally; it's only applied at the `if` consumer when s=inst[5]=1.
        // signed_valid is NOT set for cpi in the RTL: signed_inversion is
        // cleared, so `if slt` after cpi is an unsigned compare.  With
        // amode[1] set the carry comes from the 8-bit-truncated adder instead
        // of the comparator (wrong for imm8 == 0).
        if (cond_ & 1) {
            zflag_ = (a_ == imm8);
            if (amode_ & 2) {
                uint8_t adder = (uint8_t)(~imm8 + 1);
                cflag_ = !(((uint16_t)a_ + adder) >> 8);
            } else {
                cflag_ = (a_ < imm8);
            }
            signed_inv_ = false;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x21) {  // 100001 = MULU [base+uimm9]
        // Per RTL: MULU writes the HIGH byte of A*M to A; MUL (110001) writes LOW.
        // Neither updates Z or C.
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint16_t result;
            if (amode_ & 0x02) {
                result = (uint16_t)((int16_t)(int8_t)a_ * (int16_t)(int8_t)val);
            } else {
                result = (uint16_t)a_ * (uint16_t)val;
            }
            a_ = (result >> 8) & 0xFF;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === IF instruction: inst[15:8]==10100010 ===
    if ((inst >> 8) == 0xA2) {
        uint8_t type = (inst >> 3) & 0x03;
        uint8_t cc   = inst & 0x07;
        bool    s    = (inst >> 5) & 1;     // signed-select bit
        bool truth = eval_condition(cc, s);
        switch (type) {
        case 0: case 3:  // IF
            cond_ = (truth ? 1 : 0) | 0x02;  // cond[0]=truth, cond[1]=1
            break;
        case 1:  // IFTT
            cond_ = (truth ? 3 : 0);  // cond[0]=truth, cond[1]=truth
            break;
        case 2:  // IFTE
            cond_ = (truth ? 1 : 2);  // cond[0]=truth, cond[1]=!truth
            break;
        }
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === ALU + Memory ops (top6 in 11xxxx range) ===
    if (top6 == 0x30) {  // 110000 = ADD a, [base+uimm9]
        // Per RTL (TT07 silicon): add does NOT include the carry (acc_adder = d_i);
        // only adc #imm and sub do.  signed_inversion = a[7] XOR mem[7].
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint16_t sum = (uint16_t)a_ + val;
            signed_inv_ = ((a_ ^ val) >> 7) & 1;
            a_ = sum & 0xFF;
            cflag_ = (sum >> 8) & 1;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x31) {  // 110001 = MUL [base+uimm9]
        // Per RTL: MUL writes LOW byte of A*M to A; neither Z nor C is updated.
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint16_t result;
            if (amode_ & 0x02) {
                result = (uint16_t)((int16_t)(int8_t)a_ * (int16_t)(int8_t)val);
            } else {
                result = (uint16_t)a_ * (uint16_t)val;
            }
            a_ = result & 0xFF;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x32) {  // 110010 = SUB a, [base+uimm9]
        // Per RTL (TT07 silicon): acc_adder = ~(mem + C) + 1 in 8 bits, so a
        // zero operand with C=0 adds 0 instead of 256: the result is right
        // but a borrow is reported.  signed_inversion = a[7] XOR mem[7].
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint8_t adder = (uint8_t)(~(uint8_t)(val + (cflag_ ? 1 : 0)) + 1);
            uint16_t sum = (uint16_t)a_ + adder;
            signed_inv_ = ((a_ ^ val) >> 7) & 1;
            a_ = sum & 0xFF;
            cflag_ = !((sum >> 8) & 1);  // Borrow = no carry out
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x34) {  // 110100 = AND a, [base+uimm9]
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            a_ = a_ & val;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x35) {  // 110101 = ANDI a, #imm8
        if (cond_ & 1) {
            a_ = a_ & imm8;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x36) {  // 110110 = OR a, [base+uimm9]
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            a_ = a_ | val;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x38) {  // 111000 = XOR a, [base+uimm9]
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            a_ = a_ ^ val;
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x39) {  // 111001 = INX [base+uimm9]
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint8_t result = val + 1;
            data_write(d_addr, result, false);
            cflag_ = (val == 0xFF);     // Carry: was 0xFF, wraps to 0
            signed_inv_ = false;
            zflag_ = (result == 0);     // Z set when new value is 0 (RTL checks old d_i==0xFF)
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x3A) {  // 111010 = CMP a, [base+uimm9]
        // Per RTL: cflag = unsigned borrow (a < mem) regardless of amode[1].
        // signed_inversion = a[7] XOR mem[7].
        // Per RTL (TT07 silicon): acc_adder = ~mem + 1 in 8 bits, so comparing
        // against a zero byte reports a borrow (a < 0) for every a.
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            uint8_t adder = (uint8_t)(~val + 1);
            zflag_ = (a_ == val);
            cflag_ = !(((uint16_t)a_ + adder) >> 8);
            signed_inv_ = ((a_ ^ val) >> 7) & 1;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x3B) {  // 111011 = SWAP a, [base+uimm9]
        // Per RTL: neither Z nor C is updated by swap.
        if (cond_ & 1) {
            uint8_t val = data_read(d_addr, false);
            data_write(d_addr, a_, false);
            a_ = val;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x37) {  // 110111 = SWAPI a, [p:abs9]
        // Per RTL: 9-bit absolute address (inst[8:0]); neither Z nor C is updated.
        if (cond_ & 1) {
            bool periph_access = p_bit;
            uint8_t val = data_read(uimm9, periph_access);
            data_write(uimm9, a_, periph_access);
            a_ = val;
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x3C) {  // 111100 = LDAX a, [base+uimm9]
        if (cond_ & 1) {
            a_ = data_read(d_addr, false);
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if (top6 == 0x3D) {  // 111101 = LDA a, [p:abs9]
        // Per RTL: address is 9 bits (inst[8:0]); p=inst[9] selects periph.
        if (cond_ & 1) {
            bool periph_access = p_bit;
            a_ = data_read(uimm9, periph_access);
            zflag_ = (a_ == 0);
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    if ((inst >> 11) == 0x1F) {  // 11111 = STAX/STA
        if (cond_ & 1) {
            // inst[10] selects: 0=STAX [base+uimm9], 1=STA [p:abs9]
            if (inst & 0x0400) {
                // STA [p:abs9] — inst[15:10]==111111, p=inst[9], addr=inst[8:0]
                bool periph_access = p_bit;
                data_write(uimm9, a_, periph_access);
            } else {
                // STAX [base+uimm9] — inst[15:10]==111110
                data_write(d_addr, a_, false);
            }
        }
        cond_ = 0x02 | (cond_ >> 1);
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === LDXX / STXX ===
    if (top7 == 0x66) {  // 1100110 = LDXX ix, [sp+uimm9]
        // Per RTL: 2-cycle load. Memory layout from a prior `stxx`: high+cond at
        // [SP+off+1], low at [SP+off]. ldxx leaves cond on the normal shift path
        // (ix_cond goes into IX only, not into cond).
        if (cond_ & 1) {
            uint16_t addr = (sp_ + uimm9) & D_MASK;
            uint8_t lo = data_read(addr, false);
            uint8_t hi = data_read((addr + 1) & D_MASK, false);
            ix_ = ((uint16_t)(hi & 0x7F) << 8) | lo;
            ix_cond_ = (hi >> 7) & 1;
        }
        cond_ = 0x02 | (cond_ >> 1);
        cycles = 2;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }
    if (top7 == 0x67) {  // 1100111 = STXX [sp+uimm9], ix
        if (cond_ & 1) {
            uint16_t addr = (sp_ + uimm9) & D_MASK;
            data_write(addr, ix_ & 0xFF, false);
            data_write((addr + 1) & D_MASK,
                       ((ix_ >> 8) & 0x7F) | (ix_cond_ ? 0x80 : 0), false);
        }
        cond_ = 0x02 | (cond_ >> 1);
        cycles = 2;
        inst_count_++;
        cycle_count_ += cycles;
        return cycles;
    }

    // === Misc utility group: inst[15:8]==10100000 / 10100001 / 10100011 ===
    // 0xA3 (div/rem, itof/ftoi/fclr) shares this sub-decode block.
    uint8_t top8 = (inst >> 8) & 0xFF;

    if (top8 == 0xA0 || top8 == 0xA1 || top8 == 0xA3) {
        // Detailed sub-decode within the 1010_0000 / 1010_0001 prefix

        // SHL a: inst[15:2]==10100000000000 (op_shl in RTL)
        if (top14 == 0x2800) {
            if (cond_ & 1) {
                bool old_msb = (a_ >> 7) & 1;
                a_ = (a_ << 1) | ((amode_ & 1) ? (cflag_ ? 1 : 0) : 0);
                cflag_ = old_msb;
                signed_inv_ = false;
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SHR a: inst[15:2]==10100000000001 (op_shr in RTL)
        if (top14 == 0x2801) {
            if (cond_ & 1) {
                bool old_lsb = a_ & 1;
                uint8_t fill;
                if (amode_ & 0x02)
                    fill = (a_ >> 7) & 1;  // Sign extend
                else if (amode_ & 0x01)
                    fill = cflag_ ? 1 : 0;  // Rotate through carry
                else
                    fill = 0;  // Logical
                a_ = (fill << 7) | (a_ >> 1);
                cflag_ = old_lsb;
                signed_inv_ = false;
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SHL16 [sp+u2]: inst[15:4]==101000000010
        // Per RTL: base is always SP; offset is inst[1:0]. Neither C nor Z is updated.
        if ((inst >> 4) == 0xA02) {
            if (cond_ & 1) {
                uint16_t sh_addr = (sp_ + (inst & 0x03)) & D_MASK;
                uint8_t mem_val = data_read(sh_addr, false);
                uint8_t new_mem = (mem_val << 1) | ((a_ >> 7) & 1);
                data_write(sh_addr, new_mem, false);
                a_ = (a_ << 1) | ((amode_ & 1) ? (cflag_ ? 1 : 0) : 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SHR16 [sp+u2]: inst[15:4]==101000000011
        // Per RTL: base is always SP; offset is inst[1:0]. Neither C nor Z is updated.
        if ((inst >> 4) == 0xA03) {
            if (cond_ & 1) {
                uint16_t sh_addr = (sp_ + (inst & 0x03)) & D_MASK;
                uint8_t mem_val = data_read(sh_addr, false);
                uint8_t fill;
                if (amode_ & 0x02)
                    fill = (mem_val >> 7) & 1;
                else if (amode_ & 0x01)
                    fill = cflag_ ? 1 : 0;
                else
                    fill = 0;
                uint8_t new_mem = (fill << 7) | (mem_val >> 1);
                data_write(sh_addr, new_mem, false);
                a_ = ((mem_val & 1) << 7) | (a_ >> 1);
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDC C, #imm1: inst[15:3]==1010000000001
        if ((inst >> 3) == 0x1401) {
            if (cond_ & 1) {
                cflag_ = inst & 1;
                signed_inv_ = false;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDZ: inst[15:4]==101000000110
        if ((inst >> 4) == 0xA06) {
            if (cond_ & 1) {
                bool c_bit = (inst >> 1) & 1;
                bool b_bit = inst & 1;
                if (!c_bit)
                    zflag_ = b_bit;
                else
                    zflag_ = b_bit ? cflag_ : !zflag_;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // NOTZ: inst[15:0] pattern = 10100000 01110100 ... check inst[15:2]
        // Actually: inst == 10100000_01110100 not quite... let's check:
        // notz: 10100000011101 in bits [15:2]
        if (top14 == 0x281D) {  // 10100000011101
            if (cond_ & 1) {
                zflag_ = (a_ != 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // EIDI: inst[15:1]==10100000011110
        if ((inst >> 1) == 0x503C) {
            if (cond_ & 1) {
                ie_ = inst & 1;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // BRK: inst[15:2]==10100000011111
        if (top14 == 0x281F) {
            halted_ = true;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // NOP: inst[15:2]==10100000011100
        if (top14 == 0x281C) {
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // PUSH A: inst[15:6]==1010000010
        if (top10 == 0x282) {
            if (cond_ & 1) {
                push_byte(a_);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // POP A: inst[15:6]==1010000011
        if (top10 == 0x283) {
            if (cond_ & 1) {
                a_ = pop_byte();
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // BTST a, #bit3: inst[15:5]==10100000010
        if (top11 == 0x502) {
            if (cond_ & 1) {
                uint8_t bit = inst & 0x07;
                zflag_ = ((a_ >> bit) & 1);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // TXA: inst[15:2]==10100000000100
        if (top14 == 0x2804) {
            if (cond_ & 1) {
                a_ = ix_ & 0xFF;
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // TXAU: inst[15:2]==10100000000110
        if (top14 == 0x2806) {
            if (cond_ & 1) {
                a_ = ((ix_ >> 8) & 0x7F) | (ix_cond_ ? 0x80 : 0);
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // TAX: inst[15:3]==1010000100000
        if ((inst >> 3) == 0x1420) {
            if (cond_ & 1) {
                ix_ = (ix_ & 0x7F00) | a_;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // TAXU: inst[15:3]==1010000100001
        if ((inst >> 3) == 0x1421) {
            if (cond_ & 1) {
                ix_ = (ix_ & 0x00FF) | ((uint16_t)(a_ & 0x7F) << 8);
                ix_cond_ = (a_ >> 7) & 1;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SRA (save RA): inst[15:2]==10100001011000
        if (top14 == 0x2858) {
            if (cond_ & 1) {
                push_byte(ra_ & 0xFF);
                push_byte(((ra_ >> 8) & 0x7F) | (ra_cond_ ? 0x80 : 0));
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LRA (load RA): inst[15:2]==10100001011001
        if (top14 == 0x2859) {
            if (cond_ & 1) {
                uint8_t hi = pop_byte();
                uint8_t lo = pop_byte();
                ra_ = ((uint16_t)(hi & 0x7F) << 8) | lo;
                ra_cond_ = (hi >> 7) & 1;
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // PUSH IX: inst[15:2]==10100001011010
        if (top14 == 0x285A) {
            if (cond_ & 1) {
                push_byte(ix_ & 0xFF);
                push_byte(((ix_ >> 8) & 0x7F) | (ix_cond_ ? 0x80 : 0));
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // POP IX: inst[15:2]==10100001011011
        if (top14 == 0x285B) {
            if (cond_ & 1) {
                uint8_t hi = pop_byte();
                uint8_t lo = pop_byte();
                ix_ = ((uint16_t)(hi & 0x7F) << 8) | lo;
                ix_cond_ = (hi >> 7) & 1;
            }
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDDIV (two-word): the following word is the SP-relative offset.
        // IX[7:0]<=A; RA[7:0]<=mem[SP+offset].
        if (top14 == 0x285C) {
            uint16_t off = (mem_ ? mem_->inst_read(pc_ & PC_MASK) : 0) & 0x1FF;
            if (cond_ & 1) {
                ix_ = (ix_ & 0x7F00) | a_;
                uint16_t addr = (sp_ + off) & D_MASK;
                ra_ = (ra_ & 0x7F00) | data_read(addr, false);
            }
            pc_ = (pc_ + 1) & PC_MASK;  // consume the offset word
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SAVEC: inst[15:2]==10100001011110
        if (top14 == 0x285E) {
            if (cond_ & 1) {
                cflag_save_ = cflag_;
                signed_inv_save_ = signed_inv_;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // RESTC: inst[15:2]==10100001011111
        if (top14 == 0x285F) {
            if (cond_ & 1) {
                cflag_ = cflag_save_;
                signed_inv_ = signed_inv_save_;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // AMODE: inst[15:4]==101000010100
        if ((inst >> 4) == 0xA14) {
            amode_ = inst & 0x07;
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDX (load IX from next word): inst[15:4]==101000011000
        // Per RTL: ix_cond becomes 1 after the load.
        if ((inst >> 4) == 0xA18) {
            if (cond_ & 1) {
                uint16_t next_inst = mem_ ? mem_->inst_read(pc_ & PC_MASK) : 0;
                ix_ = next_inst & PC_MASK;
                ix_cond_ = true;
            }
            pc_ = (pc_ + 1) & PC_MASK;  // Skip the data word
            cond_ = 0x02 | (cond_ >> 1);
            cycles = 2;
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDAC: inst[15:5]==10100001110
        // Per RTL: the ldac encoding pins inst[5]=0, so signed_inversion is
        // never applied to its condcode evaluation (always unsigned).
        if (top11 == 0x50E) {
            if (cond_ & 1) {
                a_ = eval_condition(inst & 0x07, false) ? 1 : 0;
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // ADDAX: inst[15:5]==10100001101, inst[1:0]==00
        if (top11 == 0x50D && (inst & 0x03) == 0x00) {
            if (cond_ & 1) {
                bool use_carry = (inst >> 2) & 1;
                uint16_t addend = (uint16_t)a_ + (use_carry && cflag_ ? 0x100 : 0);
                ix_ = (ix_ + addend) & PC_MASK;
                ix_cond_ = true;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // ADDAXU: inst[15:5]==10100001101, inst[2:0]==001
        if (top11 == 0x50D && (inst & 0x07) == 0x01) {
            if (cond_ & 1) {
                uint16_t upper = (ix_ >> 8) & 0x7F;
                upper = (upper + a_) & 0x7F;
                ix_ = (ix_ & 0x00FF) | (upper << 8);
                ix_cond_ = true;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SUBAX: inst[15:5]==10100001101, inst[1:0]==10
        // Per RTL: ix_cond becomes 1 after the subtract.
        if (top11 == 0x50D && (inst & 0x03) == 0x02) {
            if (cond_ & 1) {
                bool use_carry = (inst >> 2) & 1;
                uint16_t subtrahend = (uint16_t)a_ + (use_carry && cflag_ ? 0x100 : 0);
                ix_ = (ix_ - subtrahend) & PC_MASK;
                ix_cond_ = true;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // SUBAXU: inst[15:5]==10100001101, inst[2:0]==011
        // Per RTL: ix_cond becomes 1 after the subtract.
        if (top11 == 0x50D && (inst & 0x07) == 0x03) {
            if (cond_ & 1) {
                uint16_t upper = (ix_ >> 8) & 0x7F;
                upper = (upper - a_) & 0x7F;
                ix_ = (ix_ & 0x00FF) | (upper << 8);
                ix_cond_ = true;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // LDIRQ: inst[15:2]==10100001001000
        if (top14 == 0x2848) {
            if (cond_ & 1) {
                a_ = (signed_inv_isr_ ? 0x04 : 0) | (zflag_isr_ ? 0x02 : 0) | (cflag_isr_ ? 0x01 : 0);
                zflag_ = (a_ == 0);
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // STIRQ: inst[15:2]==10100001001001
        if (top14 == 0x2849) {
            if (cond_ & 1) {
                cflag_isr_ = a_ & 1;
                zflag_isr_ = (a_ >> 1) & 1;
                signed_inv_isr_ = (a_ >> 2) & 1;
            }
            cond_ = 0x02 | (cond_ >> 1);
            inst_count_++;
            cycle_count_ += cycles;
            return cycles;
        }

        // === BF16 floating-point operations ===
        if (bf16_) {
            // TFA: inst[15:1]==101000011110000
            if ((inst >> 1) == 0x50F0) {
                if (cond_ & 1) {
                    if (inst & 1)
                        a_ = (bf16_->facc >> 8) & 0xFF;  // Upper byte
                    else
                        a_ = bf16_->facc & 0xFF;          // Lower byte
                    zflag_ = (a_ == 0);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // TAF: inst[15:1]==101000011110001
            if ((inst >> 1) == 0x50F1) {
                if (cond_ & 1) {
                    if (inst & 1)
                        bf16_->facc = (bf16_->facc & 0x00FF) | ((uint16_t)a_ << 8);
                    else
                        bf16_->facc = (bf16_->facc & 0xFF00) | a_;
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FMUL: inst[15:2]==10100001111001
            if (top14 == 0x2879) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    bf16_->facc = bf16_->mul(bf16_->facc, bf16_->f[idx]);
                    zflag_ = (bf16_->facc == 0 || bf16_->facc == 0x8000);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FADD: inst[15:2]==10100001111010
            if (top14 == 0x287A) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    bf16_->facc = bf16_->add(bf16_->facc, bf16_->f[idx]);
                    zflag_ = (bf16_->facc == 0 || bf16_->facc == 0x8000);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FNEG: inst[15:2]==10100001111011
            // Per RTL/ISA: fneg negates fx[ff], not facc.
            if (top14 == 0x287B) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    bf16_->f[idx] = bf16_->neg(bf16_->f[idx]);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FSWAP: inst[15:2]==10100001111100
            if (top14 == 0x287C) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    uint16_t t = bf16_->facc;
                    bf16_->facc = bf16_->f[idx];
                    bf16_->f[idx] = t;
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FCMP: inst[15:2]==10100001111101
            // Per RTL: C=1 iff facc > fx[ff] (sign-aware). bf16_->cmp returns -1/0/+1
            // with the usual convention (-1 if a<b, +1 if a>b), so C=(r>0).
            if (top14 == 0x287D) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    int r = bf16_->cmp(bf16_->facc, bf16_->f[idx]);
                    zflag_ = (r == 0);
                    cflag_ = (r > 0);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FDIV: inst[15:2]==10100001111110
            if (top14 == 0x287E) {
                if (cond_ & 1) {
                    uint8_t idx = inst & 0x03;
                    bf16_->facc = bf16_->div(bf16_->facc, bf16_->f[idx]);
                    zflag_ = (bf16_->facc == 0 || bf16_->facc == 0x8000);
                }
                cond_ = 0x02 | (cond_ >> 1);
                cycles = 2;
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // ITOF: inst==1010001100100000
            // Per RTL: source is the 16-bit facc (signed when amode[1]==1), and the
            // bf16 result is written back to facc.
            if (inst == 0xA320) {
                if (cond_ & 1) {
                    float f = (amode_ & 0x02)
                                  ? (float)(int16_t)bf16_->facc
                                  : (float)bf16_->facc;
                    bf16_->facc = BF16Unit::from_float(f);
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FTOI: inst==1010001100100001
            // Per RTL: facc <= int16(bf16 facc). Signed truncation when amode[1]==1.
            if (inst == 0xA321) {
                if (cond_ & 1) {
                    float f = BF16Unit::to_float(bf16_->facc);
                    if (amode_ & 0x02)
                        bf16_->facc = (uint16_t)(int16_t)f;
                    else
                        bf16_->facc = (uint16_t)f;
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // FCLR: inst==1010001100100010
            if (inst == 0xA322) {
                if (cond_ & 1) {
                    bf16_->facc = 0;
                }
                cond_ = 0x02 | (cond_ >> 1);
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }
        }

        // === Division operations ===
        if (div_) {
            // DIV: inst[15:4]==101000110000.  When the divisor is 16-bit
            // (dv bit0==0), a following word supplies the SP-relative offset of
            // the divisor high byte (and the result high byte is written back
            // there, per RTL d_o = div_result[15:8]).
            if ((inst >> 4) == 0xA30) {
                uint8_t dv = inst & 0x03;
                uint16_t off = 0;
                if (!(dv & 1)) {
                    off = (mem_ ? mem_->inst_read(pc_ & PC_MASK) : 0) & 0x1FF;
                    pc_ = (pc_ + 1) & PC_MASK;  // consume the offset word
                }
                if (cond_ & 1) {
                    uint16_t divisor, dividend;
                    if (dv & 1) {
                        divisor = ((amode_ & 0x02) ? (uint16_t)(int16_t)(int8_t)a_ : (uint16_t)a_);
                    } else {
                        uint8_t mem_val = data_read((sp_ + off) & D_MASK, false);
                        divisor = ((uint16_t)mem_val << 8) | a_;
                    }
                    if (dv & 2) {
                        dividend = ((amode_ & 0x02) ? (uint16_t)(int16_t)(int8_t)(ix_ & 0xFF) : (uint16_t)(ix_ & 0xFF));
                    } else {
                        dividend = ((uint16_t)(ra_ & 0xFF) << 8) | (ix_ & 0xFF);
                    }
                    int op = (amode_ & 0x02) ? 1 : 0;  // signed vs unsigned div
                    div_->start(dividend, divisor, op);
                    a_ = div_->result() & 0xFF;
                    zflag_ = (a_ == 0);
                    if (!(dv & 1))
                        data_write((sp_ + off) & D_MASK,
                                   (div_->result() >> 8) & 0xFF, false);
                }
                cond_ = 0x02 | (cond_ >> 1);
                cycles = 2;
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }

            // REM: inst[15:4]==101000110001 (same two-word form as DIV).
            if ((inst >> 4) == 0xA31) {
                uint8_t dv = inst & 0x03;
                uint16_t off = 0;
                if (!(dv & 1)) {
                    off = (mem_ ? mem_->inst_read(pc_ & PC_MASK) : 0) & 0x1FF;
                    pc_ = (pc_ + 1) & PC_MASK;  // consume the offset word
                }
                if (cond_ & 1) {
                    uint16_t divisor, dividend;
                    if (dv & 1) {
                        divisor = ((amode_ & 0x02) ? (uint16_t)(int16_t)(int8_t)a_ : (uint16_t)a_);
                    } else {
                        uint8_t mem_val = data_read((sp_ + off) & D_MASK, false);
                        divisor = ((uint16_t)mem_val << 8) | a_;
                    }
                    if (dv & 2) {
                        dividend = ((amode_ & 0x02) ? (uint16_t)(int16_t)(int8_t)(ix_ & 0xFF) : (uint16_t)(ix_ & 0xFF));
                    } else {
                        dividend = ((uint16_t)(ra_ & 0xFF) << 8) | (ix_ & 0xFF);
                    }
                    int op = (amode_ & 0x02) ? 3 : 2;  // signed vs unsigned rem
                    div_->start(dividend, divisor, op);
                    a_ = div_->result() & 0xFF;
                    zflag_ = (a_ == 0);
                    if (!(dv & 1))
                        data_write((sp_ + off) & D_MASK,
                                   (div_->result() >> 8) & 0xFF, false);
                }
                cond_ = 0x02 | (cond_ >> 1);
                cycles = 2;
                inst_count_++;
                cycle_count_ += cycles;
                return cycles;
            }
        }
    }

    // Unknown instruction — treat as NOP
    cond_ = 0x02 | (cond_ >> 1);
    inst_count_++;
    cycle_count_ += cycles;

    // Check for interrupts after instruction execution
    do_interrupt();

    return cycles;
}
