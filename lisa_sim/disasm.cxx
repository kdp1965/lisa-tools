#include "disasm.h"
#include <cstdio>

static const char* cond_names[] = { "eq", "ne", "nc", "c", "gt", "lt", "ge", "le" };
static const char* if_types[]   = { "if", "iftt", "ifte", "if" };

// Format indexed address: [ix+N] or [sp+N]
static std::string idx_addr(uint16_t inst) {
    bool sp = (inst >> 9) & 1;
    uint16_t off = inst & 0x1FF;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d(%s)", off, sp ? "sp" : "ix");
    return buf;
}

// Format peripheral/direct address: [0:N] or [1:N]
static std::string paddr(uint16_t inst) {
    bool p = (inst >> 9) & 1;
    uint8_t imm = inst & 0xFF;
    char buf[32];
    snprintf(buf, sizeof(buf), "%s(0x%02X)", p ? "p" : "", imm);
    return buf;
}

static int16_t sext11(uint16_t v) {
    if (v & 0x400) return (int16_t)(v | 0xF800);
    return (int16_t)(v & 0x7FF);
}

std::string lisa_disasm(uint16_t inst, uint16_t pc, uint16_t next_word) {
    char buf[64];

    // === inst[15]==0: JAL ===
    if ((inst & 0x8000) == 0) {
        snprintf(buf, sizeof(buf), "jal    0x%04X", inst & 0x7FFF);
        return buf;
    }

    uint8_t top5 = (inst >> 11) & 0x1F;
    uint8_t top6 = (inst >> 10) & 0x3F;
    uint8_t top7 = (inst >>  9) & 0x7F;
    uint8_t top8 = (inst >>  8) & 0xFF;
    uint16_t top10 = (inst >> 6) & 0x3FF;
    uint16_t top11 = (inst >> 5) & 0x7FF;
    uint16_t top14 = (inst >> 2) & 0x3FFF;

    // === Branches ===
    if (top5 == 0x16) {
        int16_t off = sext11(inst & 0x7FF);
        snprintf(buf, sizeof(buf), "br     0x%04X (%s%d)", (pc + off) & 0x7FFF, off < 0 ? "" : "+", off);
        return buf;
    }
    if (top5 == 0x15) {
        int16_t off = sext11(inst & 0x7FF);
        snprintf(buf, sizeof(buf), "bnz    0x%04X (%s%d)", (pc + off) & 0x7FFF, off < 0 ? "" : "+", off);
        return buf;
    }
    if (top5 == 0x17) {
        int16_t off = sext11(inst & 0x7FF);
        snprintf(buf, sizeof(buf), "bz     0x%04X (%s%d)", (pc + off) & 0x7FFF, off < 0 ? "" : "+", off);
        return buf;
    }

    // === Returns ===
    if (top6 == 0x23) {
        snprintf(buf, sizeof(buf), "ret    #0x%02X", inst & 0xFF);
        return buf;
    }
    if ((inst >> 7) == 0x114) return "ret";
    if (top10 == 0x22C) return "rc";
    if (top10 == 0x22E) return "rz";
    if (top10 == 0x22D) return "rets";

    // === Call/Jmp IX ===
    if (top11 == 0x454) return "call   ix";
    if (top11 == 0x455) return "jmp    ix";

    // === Exchange ops ===
    if (top14 == 0x22B0) return "xchg   ra";
    if (top14 == 0x22B1) return "xchg   ia";
    if (top14 == 0x22B2) return "xchg   sp";
    if (top14 == 0x22B3) return "spix";
    if (top14 == 0x22B4) return "cpx    ra";
    if (top14 == 0x22B6) return "cpx    sp";

    // === Immediate ops ===
    if (top6 == 0x20) {
        snprintf(buf, sizeof(buf), "ldi    #0x%02X", inst & 0xFF);
        return buf;
    }
    if (top6 == 0x24) {
        snprintf(buf, sizeof(buf), "adc    #0x%02X", inst & 0xFF);
        return buf;
    }
    if (top6 == 0x25) {
        snprintf(buf, sizeof(buf), "ads    #%d", (int)(int8_t)(inst & 0xFF));
        return buf;
    }
    if (top6 == 0x26) {
        snprintf(buf, sizeof(buf), "adx    #%d", (int)(int8_t)(inst & 0xFF));
        return buf;
    }
    if (top6 == 0x27) {
        snprintf(buf, sizeof(buf), "dcx    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x29) {
        snprintf(buf, sizeof(buf), "cpi    #0x%02X", inst & 0xFF);
        return buf;
    }
    if (top6 == 0x21) {
        snprintf(buf, sizeof(buf), "mulu   %s", idx_addr(inst).c_str());
        return buf;
    }

    // === IF instruction ===
    if (top8 == 0xA2) {
        uint8_t type = (inst >> 3) & 0x03;
        uint8_t cc   = inst & 0x07;
        snprintf(buf, sizeof(buf), "%-6s %s", if_types[type], cond_names[cc]);
        return buf;
    }

    // === ALU + Memory ops ===
    if (top6 == 0x30) {
        snprintf(buf, sizeof(buf), "add    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x31) {
        snprintf(buf, sizeof(buf), "mul    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x32) {
        snprintf(buf, sizeof(buf), "sub    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x34) {
        snprintf(buf, sizeof(buf), "and    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x35) {
        snprintf(buf, sizeof(buf), "andi   #0x%02X", inst & 0xFF);
        return buf;
    }
    if (top6 == 0x36) {
        snprintf(buf, sizeof(buf), "or     %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x38) {
        snprintf(buf, sizeof(buf), "xor    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x39) {
        snprintf(buf, sizeof(buf), "inx    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x3A) {
        snprintf(buf, sizeof(buf), "cmp    %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x3B) {
        snprintf(buf, sizeof(buf), "swap   %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x37) {
        snprintf(buf, sizeof(buf), "swapi  %s", paddr(inst).c_str());
        return buf;
    }
    if (top6 == 0x3C) {
        snprintf(buf, sizeof(buf), "ldax   %s", idx_addr(inst).c_str());
        return buf;
    }
    if (top6 == 0x3D) {
        snprintf(buf, sizeof(buf), "lda    %s", paddr(inst).c_str());
        return buf;
    }
    if ((inst >> 11) == 0x1F) {
        if (inst & 0x0400) {
            snprintf(buf, sizeof(buf), "sta    %s", paddr(inst).c_str());
        } else {
            snprintf(buf, sizeof(buf), "stax   %s", idx_addr(inst).c_str());
        }
        return buf;
    }

    // === LDXX / STXX ===
    if (top7 == 0x66) {
        snprintf(buf, sizeof(buf), "ldxx   %d(sp)", inst & 0x1FF);
        return buf;
    }
    if (top7 == 0x67) {
        snprintf(buf, sizeof(buf), "stxx   %d(sp)", inst & 0x1FF);
        return buf;
    }

    // === Misc/utility group (1010_xxxx prefix) ===
    if (top8 == 0xA0 || top8 == 0xA1) {

        // Shifts (inst[15:2] match — bottom 2 bits are don't-care)
        if (top14 == 0x2800) return "shl    a";
        if (top14 == 0x2801) return "shr    a";

        if ((inst >> 4) == 0xA02) {
            snprintf(buf, sizeof(buf), "shl16  %d(%s)", inst & 0x03,
                     ((inst >> 9) & 1) ? "sp" : "ix");
            return buf;
        }
        if ((inst >> 4) == 0xA03) {
            snprintf(buf, sizeof(buf), "shr16  %d(%s)", inst & 0x03,
                     ((inst >> 9) & 1) ? "sp" : "ix");
            return buf;
        }

        // LDC
        if ((inst >> 3) == 0x1401) {
            snprintf(buf, sizeof(buf), "ldc    #%d", inst & 1);
            return buf;
        }

        // LDZ
        if ((inst >> 4) == 0xA06) {
            bool c = (inst >> 1) & 1;
            bool b = inst & 1;
            if (!c)
                snprintf(buf, sizeof(buf), "ldz    #%d", b);
            else
                snprintf(buf, sizeof(buf), "ldz    %s", b ? "C" : "~Z");
            return buf;
        }

        // Special single instructions
        if (top14 == 0x281D) return "notz";
        if (top14 == 0x281E) {
            snprintf(buf, sizeof(buf), "%s", (inst & 1) ? "ei" : "di");
            return buf;
        }
        if (top14 == 0x281F) return "brk";
        if (top14 == 0x281C) return "nop";

        // Stack ops
        if (top10 == 0x282) return "push   a";
        if (top10 == 0x283) return "pop    a";

        // BTST
        if (top11 == 0x502) {
            snprintf(buf, sizeof(buf), "btst   #%d", inst & 0x07);
            return buf;
        }

        // Transfers
        if (top14 == 0x2804) return "txa";
        if (top14 == 0x2806) return "txau";
        if ((inst >> 3) == 0x1420) return "tax";
        if ((inst >> 3) == 0x1421) return "taxu";

        // RA/IX stack ops
        if (top14 == 0x2858) return "sra";
        if (top14 == 0x2859) return "lra";
        if (top14 == 0x285A) return "push   ix";
        if (top14 == 0x285B) return "pop    ix";
        if (top14 == 0x285C) return "lddiv";
        if (top14 == 0x285E) return "savec";
        if (top14 == 0x285F) return "restc";

        // AMODE
        if ((inst >> 4) == 0xA14) {
            snprintf(buf, sizeof(buf), "amode  #%d", inst & 0x07);
            return buf;
        }

        // LDX
        if ((inst >> 4) == 0xA18) {
            snprintf(buf, sizeof(buf), "ldx    #0x%04X", next_word & 0x7FFF);
            return buf;
        }

        // LDAC
        if (top11 == 0x50E) {
            snprintf(buf, sizeof(buf), "ldac   %s", cond_names[inst & 0x07]);
            return buf;
        }

        // ADDAX variants
        if (top11 == 0x50D) {
            uint8_t low3 = inst & 0x07;
            if ((low3 & 0x03) == 0x00) {
                snprintf(buf, sizeof(buf), "addax%s", (low3 & 0x04) ? "c" : "");
                return buf;
            }
            if (low3 == 0x01) return "addaxu";
            if ((low3 & 0x03) == 0x02) {
                snprintf(buf, sizeof(buf), "subax%s", (low3 & 0x04) ? "c" : "");
                return buf;
            }
            if (low3 == 0x03) return "subaxu";
        }

        // LDIRQ / STIRQ
        if (top14 == 0x2848) return "ldirq";
        if (top14 == 0x2849) return "stirq";

        // BF16 ops
        if ((inst >> 1) == 0x50F0) {
            snprintf(buf, sizeof(buf), "tfa    %s", (inst & 1) ? "hi" : "lo");
            return buf;
        }
        if ((inst >> 1) == 0x50F1) {
            snprintf(buf, sizeof(buf), "taf    %s", (inst & 1) ? "hi" : "lo");
            return buf;
        }
        if (top14 == 0x2879) { snprintf(buf, sizeof(buf), "fmul   f%d", inst & 3); return buf; }
        if (top14 == 0x287A) { snprintf(buf, sizeof(buf), "fadd   f%d", inst & 3); return buf; }
        if (top14 == 0x287B) return "fneg";
        if (top14 == 0x287C) { snprintf(buf, sizeof(buf), "fswap  f%d", inst & 3); return buf; }
        if (top14 == 0x287D) { snprintf(buf, sizeof(buf), "fcmp   f%d", inst & 3); return buf; }
        if (top14 == 0x287E) { snprintf(buf, sizeof(buf), "fdiv   f%d", inst & 3); return buf; }
        if (inst == 0xA320) return "itof";
        if (inst == 0xA321) return "ftoi";
        if (inst == 0xA322) return "fclr";

        // DIV/REM
        if ((inst >> 4) == 0xA30) {
            snprintf(buf, sizeof(buf), "div    #%d", inst & 0x03);
            return buf;
        }
        if ((inst >> 4) == 0xA31) {
            snprintf(buf, sizeof(buf), "rem    #%d", inst & 0x03);
            return buf;
        }
    }

    // Unknown
    snprintf(buf, sizeof(buf), "???    (0x%04X)", inst);
    return buf;
}
