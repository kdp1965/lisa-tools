#ifndef LISA_TIMER3_H
#define LISA_TIMER3_H

// The third chip's Timer 3 (ttsky26d RESPIN 32, src/lisa_timer3.v): a 16-bit
// counter with a prescaler, two compares (the PWM outputs, not modelled as
// pins) and two input captures on port A bits 4 and 5 (ui_in[4] / ui_in[5]),
// registers 0x30-0x3F; its enabled events are interrupt source 8, vector 9.
#include <cstdint>

class LisaTimer3 {
public:
    void    reset();
    uint8_t read(uint8_t addr);
    void    write(uint8_t addr, uint8_t val);
    void    tick(uint8_t port_a);       // one clock; the capture inputs from port A
    bool    irq() const { return (status_ & ie_) != 0; }

private:
    uint8_t  pre_ = 0, pcnt_ = 0;
    uint16_t top_ = 0xFFFF, cnt_ = 0;
    uint8_t  cnt_hold_ = 0, cnt_wbuf_ = 0;
    uint16_t cmp_[2] = {0, 0}, cap_[2] = {0, 0};
    uint8_t  ctrl_ = 0, ie_ = 0, status_ = 0;
    uint8_t  last_ = 0;                 // the capture inputs' previous levels
};

#endif
