#ifndef LISA_MISC36_H
#define LISA_MISC36_H

// The third chip's RESPIN 36 peripherals (ttsky26d src/lisa_cyccnt.v,
// lisa_wdt.v): the 32-bit cycle counter (0x48-0x4B: a CYC0 read latches
// 31:8, a write clears it) and the watchdog (0x4C CTRL [0] EN [3:1] SCALE,
// 0x4D KICK 0xA5 / read the ticks, 0x4E TIMEOUT, 0x4F STAT [0] WDRST:
// (TIMEOUT + 1) x 2^(16 + SCALE) clocks, then the core and the peripherals
// reset, EN cleared, WDRST set).  The PDM outputs are Timer 3's channels in
// PDM mode (timer3.h: IE [6:5]).
#include <cstdint>

class LisaMisc36 {
public:
    void    reset();                    // power-on: the watchdog too
    void    reset_peripherals();        // the watchdog's reset: all but the watchdog
    bool    handles(uint8_t addr) const { return addr >= 0x48 && addr <= 0x4F; }
    uint8_t read(uint8_t addr);
    void    write(uint8_t addr, uint8_t val);
    void    tick();
    bool    take_wdt_reset() { bool r = wdt_fired_; wdt_fired_ = false; return r; }

private:
    uint32_t cyc_ = 0, cyc_hold_ = 0;
    bool     wdt_en_ = false, wdt_rst_ = false, wdt_fired_ = false;
    uint8_t  wdt_scale_ = 0, wdt_timeout_ = 0xff, wdt_ticks_ = 0;
    uint32_t wdt_pre_ = 0;
};

#endif
