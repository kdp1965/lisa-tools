#include "timer3.h"

void LisaTimer3::reset() {
    *this = LisaTimer3();
}

uint8_t LisaTimer3::read(uint8_t addr) {
    switch (addr) {
    case 0x30: return pre_;
    case 0x31: return top_ & 0xFF;
    case 0x32: return top_ >> 8;
    case 0x33: cnt_hold_ = cnt_ >> 8; return cnt_ & 0xFF;     // latches the high byte
    case 0x34: return cnt_hold_;
    case 0x35: return cmp_[0] & 0xFF;
    case 0x36: return cmp_[0] >> 8;
    case 0x37: return cmp_[1] & 0xFF;
    case 0x38: return cmp_[1] >> 8;
    case 0x39: return cap_[0] & 0xFF;
    case 0x3A: return cap_[0] >> 8;
    case 0x3B: return cap_[1] & 0xFF;
    case 0x3C: return cap_[1] >> 8;
    case 0x3D: return ctrl_;
    case 0x3E: return ie_;
    case 0x3F: return status_;
    default:   return 0;
    }
}

void LisaTimer3::write(uint8_t addr, uint8_t val) {
    switch (addr) {
    case 0x30: pre_ = val; break;
    case 0x31: top_ = (top_ & 0xFF00) | val; break;
    case 0x32: top_ = (top_ & 0x00FF) | (val << 8); break;
    case 0x33: cnt_ = (uint16_t)((cnt_wbuf_ << 8) | val); pcnt_ = 0; break;
    case 0x34: cnt_wbuf_ = val; break;
    case 0x35: cmp_[0] = (cmp_[0] & 0xFF00) | val; break;
    case 0x36: cmp_[0] = (cmp_[0] & 0x00FF) | (val << 8); break;
    case 0x37: cmp_[1] = (cmp_[1] & 0xFF00) | val; break;
    case 0x38: cmp_[1] = (cmp_[1] & 0x00FF) | (val << 8); break;
    case 0x3D: ctrl_ = val; break;
    case 0x3E: ie_ = val & 0x7F; break;     // [6:5] PDM0 / PDM1 (RESPIN 36: the outputs not modelled)
    case 0x3F: status_ &= ~(val & 0x1F); break;     // a 1 clears
    default: break;
    }
}

void LisaTimer3::tick(uint8_t port_a) {
    uint8_t in = (port_a >> 4) & 3;
    uint8_t rise = in & ~last_, fall = ~in & last_;
    last_ = in;
    for (int k = 0; k < 2; k++) {
        uint8_t sel = (ctrl_ >> (4 + 2 * k)) & 3;
        if ((((rise >> k) & 1) && (sel & 1)) || (((fall >> k) & 1) && (sel & 2))) {
            cap_[k] = cnt_;
            status_ |= 0x08 << k;
        }
    }
    if (!(ctrl_ & 1))
        return;
    if (pcnt_ != pre_) {
        pcnt_++;
        return;
    }
    pcnt_ = 0;
    if (cnt_ == top_)
        status_ |= 0x01;
    uint16_t next = cnt_ == top_ ? 0 : (uint16_t)(cnt_ + 1);
    if (next == cmp_[0]) status_ |= 0x02;
    if (next == cmp_[1]) status_ |= 0x04;
    cnt_ = next;
}
