#include "div.h"

LisaDiv::LisaDiv() {
    reset();
}

void LisaDiv::reset() {
    result_  = 0;
    other_   = 0;
    divisor_ = 0;
    ready_   = false;
}

// The RTL's loop: the dividend's bits leave res at the top, one a step,
// into the remainder b; when the divisor fits it comes out and a one
// enters res at the bottom.  17-bit trial subtraction as in lisa_div.v.
void LisaDiv::run(uint16_t b, uint16_t res, int steps, uint16_t &q, uint16_t &r) const {
    for (int i = 0; i < steps; i++) {
        uint32_t shifted = ((uint32_t)b << 1) | (res >> 15);
        uint32_t trial   = shifted - divisor_;
        bool fits = !(trial & 0x10000);
        b   = fits ? (uint16_t)trial : (uint16_t)shifted;
        res = (uint16_t)((res << 1) | (fits ? 1 : 0));
    }
    q = res;
    r = b;
}

void LisaDiv::start(uint16_t dividend, uint16_t divisor, int op) {
    bool is_rem    = (op & 2) != 0;
    bool is_signed = (op & 1) != 0;
    bool sdd = is_signed && (dividend & 0x8000);
    bool sdv = is_signed && (divisor & 0x8000);
    uint16_t add = sdd ? (uint16_t)(0 - dividend) : dividend;
    divisor_     = sdv ? (uint16_t)(0 - divisor) : divisor;

    uint16_t q, r;
    run(0, add, 16, q, r);
    // signed as C: the quotient negative when the signs differ (not by
    // zero: all ones), the remainder with the dividend's sign
    if ((sdd ^ sdv) && divisor != 0)
        q = (uint16_t)(0 - q);
    if (sdd)
        r = (uint16_t)(0 - r);
    result_ = is_rem ? r : q;
    other_  = is_rem ? q : r;
    ready_  = true;
}

void LisaDiv::chain(uint16_t rem_in, uint8_t digit) {
    uint16_t q, r;
    run(rem_in, (uint16_t)(digit << 8), 8, q, r);
    result_ = q;
    other_  = r;
    ready_  = true;
}

void LisaDiv::tick() {
}
