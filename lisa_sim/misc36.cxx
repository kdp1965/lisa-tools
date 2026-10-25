#include "misc36.h"

void LisaMisc36::reset() {
    *this = LisaMisc36();
}

void LisaMisc36::reset_peripherals() {
    cyc_ = cyc_hold_ = 0;
    pdm_ctrl_ = pdm_val0_ = pdm_val1_ = 0;
}

uint8_t LisaMisc36::read(uint8_t addr) {
    switch (addr) {
    case 0x48: cyc_hold_ = cyc_ >> 8; return (uint8_t)cyc_;
    case 0x49: return (uint8_t)cyc_hold_;
    case 0x4A: return (uint8_t)(cyc_hold_ >> 8);
    case 0x4B: return (uint8_t)(cyc_hold_ >> 16);
    case 0x4C: return (uint8_t)((wdt_scale_ << 1) | (wdt_en_ ? 1 : 0));
    case 0x4D: return wdt_ticks_;
    case 0x4E: return wdt_timeout_;
    case 0x4F: return wdt_rst_ ? 1 : 0;
    case 0x50: return pdm_ctrl_;
    case 0x51: return pdm_val0_;
    case 0x52: return pdm_val1_;
    default:   return 0;
    }
}

void LisaMisc36::write(uint8_t addr, uint8_t val) {
    switch (addr) {
    case 0x48: cyc_ = 0; break;
    case 0x4C: wdt_en_ = val & 1; wdt_scale_ = (val >> 1) & 7; wdt_pre_ = 0; wdt_ticks_ = 0; break;
    case 0x4D: if (val == 0xA5) { wdt_pre_ = 0; wdt_ticks_ = 0; } break;
    case 0x4E: wdt_timeout_ = val; break;
    case 0x4F: if (val & 1) wdt_rst_ = false; break;
    case 0x50: pdm_ctrl_ = val & 3; break;
    case 0x51: pdm_val0_ = val; break;
    case 0x52: pdm_val1_ = val; break;
    default: break;
    }
}

void LisaMisc36::tick() {
    cyc_++;
    if (!wdt_en_)
        return;
    if (++wdt_pre_ < (1u << (16 + wdt_scale_)))
        return;
    wdt_pre_ = 0;
    if (wdt_ticks_ == wdt_timeout_) {
        wdt_en_ = false;                // fired: the core and the peripherals reset
        wdt_rst_ = true;
        wdt_ticks_ = 0;
        wdt_fired_ = true;
    } else {
        wdt_ticks_++;
    }
}
