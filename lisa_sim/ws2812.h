#ifndef LISA_WS2812_H
#define LISA_WS2812_H

// The third chip's WS2812B driver (ttsky26d RESPIN 33, src/lisa_ws2812.v):
// registers 0x40-0x45 and the engine's timing - each FULL half is a pixel
// of 24 x (TBIT + 1) clocks, cleared as it ends; with no half full the
// line is low and after (TRST + 1) x 64 clocks DONE is set and the next
// frame starts at half A; a pixel after a gap but before the reset sets
// UNDERRUN.  The pixels themselves (the TTLC's output flops) and the pins
// are not modelled: lisa_sim has no TTLC.  With IE the free-half request
// replaces the TTLC's interrupt source (bit 5, vector 6).
// DMA (RESPIN 35, WS_CTRL [5]): START (WS_STAT bit 6) sends LEN pixels
// back to back (the flush and the fetch take no time here), then the reset
// time: FRAME (bit 7, sticky) set, ACTIVE (bit 6) dropped or, with REPEAT
// ([6]), the next frame; the request is IE and FRAME.
#include <cstdint>

class LisaWs2812 {
public:
    void    reset();
    uint8_t read(uint8_t addr) const;
    void    write(uint8_t addr, uint8_t val);
    void    tick();
    bool    ie() const { return (ctrl_ & 0x11) == 0x11; }
    bool    irq() const { return ie() && (dma() ? frame_f_ : full_ != 3); }
    void    frame(uint16_t base, uint16_t len) { base_ = base; len_ = len; }

private:
    uint8_t  ctrl_ = 0, t0h_ = 20, t1h_ = 40, tbit_ = 62, trst_ = 219;
    uint8_t  full_ = 0;
    bool     next_ = false, busy_ = false, done_ = true, underrun_ = false;
    uint32_t left_ = 0;     // clocks of the pixel being sent
    uint32_t low_ = 0;      // clocks low since the last pixel
    // DMA
    bool     dma() const { return ctrl_ & 0x20; }
    bool     active_ = false, frame_f_ = false;
    uint16_t base_ = 0, len_ = 0, count_ = 0;
};

#endif
