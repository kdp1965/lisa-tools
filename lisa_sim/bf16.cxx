#include "bf16.h"
#include <cmath>
#include <cstring>
#include <algorithm>

// ── Helpers ──────────────────────────────────────────────────────────

bool BF16Unit::is_nan(uint16_t v) {
    uint8_t exp  = (v >> BF16_FRAC_BITS) & 0xFF;
    uint8_t frac = v & 0x7F;
    return exp == 0xFF && frac != 0;
}

bool BF16Unit::is_inf(uint16_t v) {
    uint8_t exp  = (v >> BF16_FRAC_BITS) & 0xFF;
    uint8_t frac = v & 0x7F;
    return exp == 0xFF && frac == 0;
}

bool BF16Unit::is_zero(uint16_t v) {
    return (v & 0x7FFF) == 0;
}

bool BF16Unit::is_denorm(uint16_t v) {
    uint8_t exp  = (v >> BF16_FRAC_BITS) & 0xFF;
    uint8_t frac = v & 0x7F;
    return exp == 0 && frac != 0;
}

float BF16Unit::to_float(uint16_t bf16) {
    // BF16 is the upper 16 bits of a float32
    uint32_t f32 = static_cast<uint32_t>(bf16) << 16;
    float result;
    std::memcpy(&result, &f32, sizeof(result));
    return result;
}

uint16_t BF16Unit::from_float(float f) {
    uint32_t f32;
    std::memcpy(&f32, &f, sizeof(f32));
    // Round to nearest even: add rounding bias then truncate
    uint32_t rounding_bias = (f32 >> 16) & 1;  // bit 16 of f32 = LSB of bf16
    f32 += 0x7FFF + rounding_bias;
    // If NaN, keep it NaN (don't let rounding turn NaN into Inf)
    if ((f32 & 0x7F800000) == 0x7F800000 && (f32 & 0x007FFFFF) != 0) {
        f32 = (f32 & 0xFF800000) | 0x00400000;  // quiet NaN
    }
    return static_cast<uint16_t>(f32 >> 16);
}

// ── Constructor / Reset ──────────────────────────────────────────────

BF16Unit::BF16Unit() {
    reset();
}

void BF16Unit::reset() {
    facc = BF16_POS_ZERO;
    f[0] = f[1] = f[2] = f[3] = BF16_POS_ZERO;
}

// ── Arithmetic via float32 conversion ────────────────────────────────

uint16_t BF16Unit::add(uint16_t a, uint16_t b) {
    if (is_nan(a) || is_nan(b)) return BF16_NAN;
    float fa = to_float(a);
    float fb = to_float(b);
    return from_float(fa + fb);
}

uint16_t BF16Unit::mul(uint16_t a, uint16_t b) {
    if (is_nan(a) || is_nan(b)) return BF16_NAN;
    float fa = to_float(a);
    float fb = to_float(b);
    return from_float(fa * fb);
}

uint16_t BF16Unit::div(uint16_t a, uint16_t b) {
    if (is_nan(a) || is_nan(b)) return BF16_NAN;
    if (is_zero(b)) {
        if (is_zero(a)) return BF16_NAN;       // 0/0
        uint16_t sign = (a ^ b) & 0x8000;
        return sign | BF16_POS_INF;             // x/0 → ±Inf
    }
    float fa = to_float(a);
    float fb = to_float(b);
    return from_float(fa / fb);
}

uint16_t BF16Unit::neg(uint16_t a) {
    return a ^ 0x8000;  // flip sign bit
}

int BF16Unit::cmp(uint16_t a, uint16_t b) {
    if (is_nan(a) || is_nan(b)) return -2;  // unordered; caller can treat as needed
    // +0 == -0
    if (is_zero(a) && is_zero(b)) return 0;
    float fa = to_float(a);
    float fb = to_float(b);
    if (fa < fb) return -1;
    if (fa > fb) return  1;
    return 0;
}

uint16_t BF16Unit::itof(int8_t val) {
    return from_float(static_cast<float>(val));
}

int8_t BF16Unit::ftoi(uint16_t val) {
    if (is_nan(val)) return 0;
    float f = to_float(val);
    // Truncate toward zero and clamp to int8 range
    float truncated = std::trunc(f);
    if (truncated >= 127.0f)  return  127;
    if (truncated <= -128.0f) return -128;
    return static_cast<int8_t>(truncated);
}
