#ifndef LISA_DIV_H
#define LISA_DIV_H

#include <cstdint>

// The chip's divider, bit for bit (ttsky26d src/lisa_div.v since RESPIN 27:
// a restoring divider; TT07's repeated subtraction gave the same results).
// start(): dividend / divisor, op bit 0 signed (amode[1]), bit 1 remainder.
// result() is what RA and A take (the quotient, or the remainder for rem);
// other() the other one (IX on the second chip; TT07 and rc10 cleared IX).
// chain(): divc (rev 2) - {rem_in, digit} / the divisor of the last start(),
// a byte of quotient in result(), the remainder in other(); unsigned, and
// rem_in must be below the divisor (the RTL's 8 steps are replayed exactly
// either way).
struct LisaDiv {
    void start(uint16_t dividend, uint16_t divisor, int op);
    void chain(uint16_t rem_in, uint8_t digit);

    uint16_t result() const { return result_; }
    uint16_t other() const { return other_; }
    bool ready() const { return ready_; }

    void tick();
    void reset();

    LisaDiv();

private:
    void run(uint16_t b, uint16_t res, int steps, uint16_t &q, uint16_t &r) const;

    uint16_t result_;
    uint16_t other_;
    uint16_t divisor_;   // the magnitude, kept for chain()
    bool     ready_;
};

#endif
