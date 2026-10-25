#include "ws2812.h"

void LisaWs2812::reset() {
    *this = LisaWs2812();
}

uint8_t LisaWs2812::read(uint8_t addr) const {
    switch (addr) {
    case 0x40: return ctrl_;
    case 0x41: return (uint8_t)(full_ | (busy_ ? 0x04 : 0) | (done_ ? 0x08 : 0) |
                                (underrun_ ? 0x10 : 0) | (next_ ? 0x20 : 0) |
                                (active_ ? 0x40 : 0) | (frame_f_ ? 0x80 : 0));
    case 0x42: return t0h_;
    case 0x43: return t1h_;
    case 0x44: return tbit_;
    case 0x45: return trst_;
    default:   return 0;
    }
}

void LisaWs2812::write(uint8_t addr, uint8_t val) {
    switch (addr) {
    case 0x40:
        ctrl_ = val;
        if (!(val & 1)) {                   // off: nothing sent, A next
            busy_ = false;
            full_ = 0;
            next_ = false;
            done_ = true;
            low_ = 0;
            active_ = false;
        }
        if (!(val & 0x20)) active_ = false;
        break;
    case 0x41:
        if (val & 0x10) underrun_ = false;
        if (val & 0x80) frame_f_ = false;
        if ((ctrl_ & 1) && !dma()) full_ |= val & 3;
        if ((ctrl_ & 1) && dma() && (val & 0x40)) {
            active_ = true;                 // a frame from the buffer at base_ x 256
            count_ = 0;
        }
        break;
    case 0x42: t0h_ = val; break;
    case 0x43: t1h_ = val; break;
    case 0x44: tbit_ = val; break;
    case 0x45: trst_ = val; break;
    default: break;
    }
}

void LisaWs2812::tick() {
    if (!(ctrl_ & 1))
        return;
    if (dma()) {
        if (!active_)
            return;
        if (busy_) {
            if (--left_ == 0) {
                busy_ = false;
                low_ = 0;
                count_++;
            }
        } else if (count_ != len_) {
            busy_ = true;
            done_ = false;
            left_ = 24u * (tbit_ + 1u);
        } else if (!done_) {
            if (low_ == ((uint32_t)trst_ << 6 | 63))
                done_ = true;
            else
                low_++;
        } else {
            frame_f_ = true;                // the frame and its reset time are out
            if (ctrl_ & 0x40)
                count_ = 0;                 // REPEAT
            else
                active_ = false;
        }
        return;
    }
    if (busy_) {
        if (--left_ == 0) {                 // the pixel is out: its half is free
            full_ &= ~(1u << next_);
            next_ = !next_;
            low_ = 0;
            busy_ = (full_ >> next_) & 1;
            if (busy_) left_ = 24u * (tbit_ + 1u);
        }
        return;
    }
    if ((full_ >> next_) & 1) {
        if (!done_ && low_ != 0)
            underrun_ = true;
        busy_ = true;
        done_ = false;
        left_ = 24u * (tbit_ + 1u);
    } else if (!done_) {
        if (low_ == ((uint32_t)trst_ << 6 | 63)) {
            done_ = true;
            next_ = false;                  // every frame starts at half A
        } else {
            low_++;
        }
    }
}
