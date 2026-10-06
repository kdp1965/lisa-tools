#include "div.h"

LisaDiv::LisaDiv() {
    reset();
}

void LisaDiv::reset() {
    result_ = 0;
    ready_  = false;
}

void LisaDiv::start(uint16_t rs1, uint16_t rs2, int op) {
    bool is_rem    = (op & 2) != 0;
    bool is_signed = (op & 1) != 0;

    if (rs2 == 0) {
        // Division by zero: div → 0xFFFF, rem → dividend
        result_ = is_rem ? rs1 : 0xFFFF;
    } else if (is_signed) {
        auto dividend = static_cast<int16_t>(rs1);
        auto divisor  = static_cast<int16_t>(rs2);
        // Signed overflow: -32768 / -1 wraps to -32768
        if (dividend == -32768 && divisor == -1) {
            result_ = is_rem ? 0 : static_cast<uint16_t>(-32768);
        } else {
            int16_t q = dividend / divisor;
            int16_t r = dividend % divisor;
            result_ = static_cast<uint16_t>(is_rem ? r : q);
        }
    } else {
        uint16_t q = rs1 / rs2;
        uint16_t r = rs1 % rs2;
        result_ = is_rem ? r : q;
    }

    ready_ = true;
}

void LisaDiv::tick() {
    // Functional simulation: result already computed in start()
}
