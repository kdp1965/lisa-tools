#ifndef LISA_BF16_H
#define LISA_BF16_H

#include <cstdint>

// BF16 format constants
static constexpr int BF16_SIGN_BIT = 15;
static constexpr int BF16_EXP_BITS = 8;
static constexpr int BF16_FRAC_BITS = 7;
static constexpr int BF16_EXP_BIAS = 127;
static constexpr uint16_t BF16_POS_ZERO = 0x0000;
static constexpr uint16_t BF16_NEG_ZERO = 0x8000;
static constexpr uint16_t BF16_POS_INF  = 0x7F80;
static constexpr uint16_t BF16_NEG_INF  = 0xFF80;
static constexpr uint16_t BF16_NAN      = 0x7FC0;

struct BF16Unit {
    uint16_t facc;
    uint16_t f[4];  // f0-f3

    BF16Unit();
    void reset();

    // Operations
    uint16_t add(uint16_t a, uint16_t b);     // fadd
    uint16_t mul(uint16_t a, uint16_t b);     // fmul
    uint16_t div(uint16_t a, uint16_t b);     // fdiv
    uint16_t neg(uint16_t a);                  // fneg
    int      cmp(uint16_t a, uint16_t b);     // fcmp: returns -1/0/1, sets flags
    uint16_t itof(int8_t val);                 // signed int8 to BF16
    int8_t   ftoi(uint16_t val);               // BF16 to signed int8

    // Helpers
    static bool is_nan(uint16_t v);
    static bool is_inf(uint16_t v);
    static bool is_zero(uint16_t v);
    static bool is_denorm(uint16_t v);
    static float to_float(uint16_t bf16);
    static uint16_t from_float(float f);
};

#endif
