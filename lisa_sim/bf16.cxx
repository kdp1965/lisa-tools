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

// fops.v itobf16: normalize the magnitude, build the single-precision
// pattern and round it to the upper 16 bits (half up; half down when
// negative)
uint16_t BF16Unit::itobf16(uint16_t in, bool is_signed) {
    if (in == 0) return 0;
    bool neg = is_signed && (in & 0x8000);
    uint16_t sig = neg ? (uint16_t)(-in) : in;
    unsigned lz = 0;
    for (unsigned i = 8; i > 0; i >>= 1) {
        if ((sig & (uint16_t)(0xFFFF << (16 - i))) == 0) {
            sig <<= i;
            lz |= i;
        }
    }
    uint32_t exp = (142 - lz) & 0xFF;
    uint32_t pre = ((uint32_t)neg << 31) | (exp << 23) | ((uint32_t)(sig & 0x7FFF) << 8);
    uint32_t out = neg ? pre - 0x8000 : pre + 0x8000;
    return (uint16_t)(out >> 16);
}

// fops.v bf16toi: truncate toward zero, saturate; a negative value is 0
// when unsigned
uint16_t BF16Unit::bf16toi(uint16_t in, bool is_signed) {
    bool sign = in & 0x8000;
    unsigned exp = (in >> 7) & 0xFF;
    uint16_t mant = 0x80 | (in & 0x7F);
    if (exp < 127) return 0;
    unsigned lim = 127 + (is_signed ? 15 : 16);
    if (exp < lim || (sign && exp == lim && mant == 0x80)) {
        uint16_t sh = exp < 134 ? (uint16_t)(mant >> (134 - exp)) : (uint16_t)(mant << (exp - 134));
        if (is_signed)
            return sign ? (uint16_t)(-(sh & 0x7FFF)) : (uint16_t)(sh & 0x7FFF);
        return sign ? 0 : sh;
    }
    if (is_signed) return sign ? 0x8000 : 0x7FFF;
    return 0xFFFF;
}

// lampFPU's checkOperand / FUNC_extendExp, which feed fadd and fmul
static void classify_rtl(uint16_t op, bool &isInf, bool &isDN, bool &isZ, bool &isSNAN, bool &isQNAN, unsigned &extE) {
    unsigned e = (op >> 7) & 0xFF, f = op & 0x7F;
    isInf  = (e == 0xFF) && f == 0;
    isDN   = (e == 0) && f != 0;
    isZ    = (op & 0x7FFF) == 0;
    isSNAN = (e == 0xFF) && !(f & 0x40) && (f & 0x3F) != 0;
    isQNAN = (e == 0xFF) && (f & 0x40) != 0;
    extE   = (e & 0xFE) | ((e & 1) | (isDN ? 1 : 0));
}

// fops.v fadd.  Significands are 11 bits {0, hidden, fraction, 2 guard
// bits}.  Two things this RTL does that an IEEE adder would not: on a
// carry out of the sum, f_res is built at bits [7:0] while the result
// reads the fraction from [8:2], so the fraction comes out shifted right
// by one (1.25 + 1.25 = 2.25); and the rounding adds 1 at the guard
// position unconditionally (round up when any dropped bit is set, where
// the dropped bits are only the two guard bits).
//
// This is the committed RTL (every revision of fops.v gives the same
// results in icarus).  The TT07 silicon agrees with it on 1.25 + 1.25
// and on the rounding-overflow case, but not everywhere: in a random
// batch of 49 additions it differed in 8 (one ulp, usually nearer the
// true sum, and 1.25 + 1.25 style cases that came out right), at 50 MHz
// and at 12.5 MHz alike - so the taped-out adder (pnl/tt_um_lisa.pnl.v)
// is not quite this fadd.  Nothing in sdcc's library uses the hardware
// adder (bf16_add is software), so the model stays the RTL.
uint16_t BF16Unit::fadd_rtl(uint16_t a_in, uint16_t b_in, bool round_to_zero) {
    bool i1, d1, z1, s1, q1, i2, d2, z2, s2, q2;
    unsigned e1, e2;
    classify_rtl(a_in, i1, d1, z1, s1, q1, e1);
    classify_rtl(b_in, i2, d2, z2, s2, q2, e2);
    bool a_GT_b = (a_in & 0x7FFF) > (b_in & 0x7FFF);
    uint16_t operand_a = a_GT_b ? a_in : b_in, operand_b = a_GT_b ? b_in : a_in;
    bool a_isDN = a_GT_b ? d1 : d2, b_isDN = a_GT_b ? d2 : d1;
    bool a_isZ = a_GT_b ? z1 : z2, b_isZ = a_GT_b ? z2 : z1;
    unsigned a_e = a_GT_b ? e1 : e2, b_e = a_GT_b ? e2 : e1;
    bool isINF = i1 || i2;
    bool isNAN = s1 || s2 || q1 || q2;
    bool a_assumedOne = !a_isZ && !a_isDN, b_assumedOne = !b_isZ && !b_isDN;
    bool operation_sub = ((operand_a ^ operand_b) & 0x8000) != 0;
    unsigned sig_a = (a_assumedOne ? 0x200 : 0) | ((operand_a & 0x7F) << 2);
    unsigned sig_b = (b_assumedOne ? 0x200 : 0) | ((operand_b & 0x7F) << 2);
    unsigned exponent_diff = (a_e - b_e) & 0xFF;
    unsigned sig_b_add = exponent_diff >= 11 ? 0 : (sig_b >> exponent_diff);
    unsigned sig_b_twos = operation_sub ? ((~sig_b_add + 1) & 0x7FF) : sig_b_add;
    unsigned add_temp = (sig_a + sig_b_twos) & 0x7FF;
    unsigned sig_add = operation_sub ? (add_temp & 0x3FF) : add_temp;
    unsigned top9 = (sig_add >> 2) & 0x1FF;
    int leftShift = 8;
    for (int i = 0; i < 9; i++)
        if (top9 & (0x100 >> i)) { leftShift = i ? i - 1 : 0; break; }
    unsigned e_res, f_res;
    if (sig_add & 0x400) {
        if (a_e + 1 == 255) { e_res = 0xFF; f_res = 0; }
        else { e_res = (a_e + 1) & 0x1FF; f_res = (sig_add >> 2) & 0xFF; }
    } else if (sig_add & 0x200) {
        e_res = a_e;
        f_res = (sig_add & 0x1FC) | (((sig_add & 3) != 0) ? 2 : 0);
    } else if (sig_add == 0) {
        e_res = 0; f_res = 0;
    } else if (a_e > (unsigned)leftShift) {
        e_res = (a_e - leftShift) & 0x1FF;
        f_res = (sig_add << leftShift) & 0x7FF;
    } else {
        e_res = 0;
        unsigned sh = (a_e - 1) & 0x1FF;
        f_res = sh >= 11 ? 0 : (sig_add << sh) & 0x7FF;
    }
    bool output_sign = (e_res == 0 && f_res == 0) ? false : (operand_a & 0x8000) != 0;
    unsigned f_rounded = (round_to_zero && output_sign) ? f_res : (f_res + 2) & 0x7FF;
    if (isINF) return 0x7F80;
    if (isNAN) return 0x7FC0;
    return (output_sign ? 0x8000 : 0) | ((e_res & 0xFF) << 7) | ((f_rounded >> 2) & 0x7F);
}

// fops.v fmul: the 8x8 product of the significands, truncated to the
// 7 fraction bits below the leading one (no rounding); an underflowing
// exponent is treated like an overflow (infinity)
uint16_t BF16Unit::fmul_rtl(uint16_t a_in, uint16_t b_in) {
    bool i1, d1, z1, s1, q1, i2, d2, z2, s2, q2;
    unsigned a_e, b_e;
    classify_rtl(a_in, i1, d1, z1, s1, q1, a_e);
    classify_rtl(b_in, i2, d2, z2, s2, q2, b_e);
    bool a_assumedOne = !z1 && !d1, b_assumedOne = !z2 && !d2;
    unsigned mul = ((a_assumedOne ? 0x80 : 0) | (a_in & 0x7F)) * ((b_assumedOne ? 0x80 : 0) | (b_in & 0x7F));
    bool zero_check = (a_in & 0x7FFF) == 0 || (b_in & 0x7FFF) == 0;
    unsigned top6 = (mul >> 10) & 0x3F;
    unsigned M_result;
    int e_add;
    if (top6 & 0x20)      { M_result = (mul >> 8) & 0x7F; e_add = 1; }
    else if (top6 & 0x10) { M_result = (mul >> 7) & 0x7F; e_add = 0; }
    else if (top6 & 0x08) { M_result = (mul >> 6) & 0x7F; e_add = -1; }
    else if (top6 & 0x04) { M_result = (mul >> 5) & 0x7F; e_add = -2; }
    else if (top6 & 0x02) { M_result = (mul >> 4) & 0x7F; e_add = -3; }
    else if (top6 & 0x01) { M_result = (mul >> 3) & 0x7F; e_add = -4; }
    else                  { M_result = (mul >> 8) & 0x7F; e_add = 1; }
    unsigned esum = (a_e + b_e + ((mul >> 15) & 1)) & 0x1FF;
    bool overflow = zero_check || esum < 127 || esum > 381;
    unsigned e_result0 = zero_check ? 0 : overflow ? 0x1FF : ((int)a_e + (int)b_e + e_add - 127) & 0x1FF;
    bool sign = ((a_in ^ b_in) & 0x8000) != 0;
    if (s1 || s2 || q1 || q2) return 0x7FC0;
    if (i1 || i2) return 0x7F80;
    return (sign ? 0x8000 : 0) | ((e_result0 & 0xFF) << 7) | (overflow ? 0 : M_result);
}

// lisa_core.v fcmp: Z when the patterns are equal, C when facc > fx by
// sign and magnitude (so -0 and +0 differ, and NaNs compare as numbers)
void BF16Unit::fcmp_rtl(uint16_t facc, uint16_t fx, bool &z, bool &c) {
    bool sa = facc & 0x8000, sb = fx & 0x8000;
    unsigned ma = facc & 0x7FFF, mb = fx & 0x7FFF;
    z = (facc == fx);
    c = (!sa && sb) || (!sa && !sb && ma > mb) || (sa && sb && ma < mb);
}

static unsigned nlz8(unsigned f) {
    for (unsigned i = 0; i < 8; i++)
        if (f & (0x80 >> i)) return i;
    return 0;   // FUNC_numLeadingZeros: zero gives 0
}

// lampFPU.h FUNC_approxRecip: indexed by the four bits below the hidden one
static unsigned approxRecip(unsigned f8) {
    static const unsigned tab[16] = {15, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 0};
    return tab[(f8 >> 3) & 0xF];
}

// lampFPU_utils.v rndToNearestEven on the 12-bit {ovf, hidden, f[6:0], G, R, S}
static unsigned rndToNearestEven(unsigned op) {
    unsigned t = op & 0xFFF;
    unsigned bits = (op >> 1) & 7;
    bool addOne = bits >= 6;
    if (bits >= 3) t |= 8; else t &= ~8u;
    unsigned tempF = (((t >> 3) & 0xF) == 0xF) ? t : (t + (addOne ? 8 : 0));
    return (tempF >> 3) & 0x7F;
}

uint16_t BF16Unit::fdiv_rtl(uint16_t op1, uint16_t op2, bool round_to_zero) {
    bool i1, d1, z1, s1, q1, i2, d2, z2, s2, q2;
    unsigned e1, e2;
    classify_rtl(op1, i1, d1, z1, s1, q1, e1);
    classify_rtl(op2, i2, d2, z2, s2, q2, e2);
    unsigned extF1 = ((!d1 && !z1) ? 0x80 : 0) | (op1 & 0x7F);
    unsigned extF2 = ((!d2 && !z2) ? 0x80 : 0) | (op2 & 0x7F);
    unsigned nlz1 = nlz8(extF1), nlz2 = nlz8(extF2);
    unsigned shF1 = (extF1 << nlz1) & 0xFF, shF2 = (extF2 << nlz2) & 0xFF;

    // lampFPU_fractDiv: n / d in Q1.15, r = approximate 1/d, two refinements
    uint32_t n = shF1 << 8, d = shF2 << 8;
    uint32_t r = ((0x10 | approxRecip(shF2)) << 10) & 0xFFFF;
    uint32_t n_tmp = n * r, d_tmp = d * r;
    n = (n_tmp >> 15) & 0xFFFF;
    d = (d_tmp >> 15) & 0xFFFF;
    r = (0u - d) & 0xFFFF;                     // (16'b1 << 17) - d_r, in 16 bits: 2 - d
    n_tmp = n * r;
    unsigned fd = (n_tmp >> 16) & 0xFFFF;      // Q2.14

    // lampFPU_div: the exponent, the normalization and the sticky bit
    unsigned e = (127 + e1 + nlz2 - nlz1 - e2) & 0x3FF;
    unsigned extra_neg = (nlz1 + e2 - 127 - e1 - nlz2) & 0x3FF;
    unsigned leftShift = (fd & 0x4000) ? 0 : 1;
    if (!(extra_neg & 0x200)) {
        if (extra_neg > 11) { fd = 0; e = 0; }
        else { fd >>= (extra_neg + 1); e = (e + extra_neg) & 0x3FF; }
    } else if (e >= 255 && (e - 255) >= leftShift) {
        fd = 0; e = 0x3FF;
    } else {
        fd = (fd << leftShift) & 0xFFFF;
        e = (e - leftShift) & 0x3FF;
    }
    unsigned f_init = (fd >> 4) & 0xFFF;
    unsigned sticky = (fd & 0xF) ? 1 : 0;
    f_init |= sticky;
    unsigned f_post, e_post;
    if (e != 0) {
        f_post = (f_init & 0xFFC) | ((((f_init >> 1) & 1) | (f_init & 1)) << 1);
        e_post = e & 0x1FF;
    } else if (f_init & 0x400) {
        e_post = 0x1FF; f_post = f_init;
    } else {
        e_post = 0; f_post = f_init;
    }
    f_post = (f_post & ~3u) | ((((f_post >> 1) & 1) | sticky) << 1) | sticky;

    // calcInfNanZeroResDiv
    bool nan1 = s1 || q1, nan2 = s2 || q2;
    bool isValidRes = z1 || z2 || i1 || i2 || nan1 || nan2;
    bool isZeroRes = false, isInfRes = false, isNanRes = false, signRes = false;
    bool sgn1 = op1 & 0x8000, sgn2 = op2 & 0x8000;
    if (nan1) { isNanRes = true; signRes = sgn1; }
    else if (nan2) { isNanRes = true; signRes = sgn2; }
    else {
        unsigned k = (z1 << 3) | (z2 << 2) | (i1 << 1) | (unsigned)i2;
        switch (k) {
        case 0x1: isZeroRes = true; signRes = sgn1 ^ sgn2; break;   // x / inf
        case 0x2: isInfRes = true;  signRes = sgn1 ^ sgn2; break;   // inf / x
        case 0x3: isNanRes = true;  signRes = sgn1 ^ sgn2; break;   // inf / inf
        case 0x4: isInfRes = true;  signRes = sgn1 ^ sgn2; break;   // x / 0
        case 0x6: isInfRes = true;  signRes = sgn1 ^ sgn2; break;   // inf / 0
        case 0x8: isZeroRes = true; signRes = sgn1 ^ sgn2; break;   // 0 / x
        case 0x9: isZeroRes = true; signRes = sgn1 ^ sgn2; break;   // 0 / inf
        case 0xC: isNanRes = true;  signRes = sgn1 ^ sgn2; break;   // 0 / 0
        default: break;
        }
    }
    unsigned s_res, e_res, f_res;
    if (isZeroRes)      { s_res = signRes; e_res = 0;    f_res = 0; }
    else if (isInfRes)  { s_res = signRes; e_res = 0xFF; f_res = 0; }
    else if (isNanRes)  { s_res = signRes; e_res = 0xFF; f_res = 0x800; }
    else                { s_res = sgn1 ^ sgn2; e_res = e_post & 0xFF; f_res = f_post; }
    bool isToRound = !isValidRes;

    // lampFPU_div_top: the rounding
    unsigned f7 = round_to_zero ? ((f_res >> 3) & 0x7F) : rndToNearestEven(f_res);
    if (!isToRound)
        f7 = (f_res >> 5) & 0x7F;
    return (s_res ? 0x8000 : 0) | (e_res << 7) | f7;
}
