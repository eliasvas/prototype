#ifndef BRAND_H__
#define BRAND_H__
#include "helper.h"

#define BRAND_SEED(s) (xstate.a = s)
#define BRAND_MAX (U32_MAX)

// VERY simple xorshift random number generation + useful functions
// https://en.wikipedia.org/wiki/Xorshift
// TODO: Can we get reed of the seed def while maintaining sinle header?

typedef struct {
    u64 a;
} Xor_Shift_State;

#ifdef BRAND_IMPLEMENTATION
  Xor_Shift_State xstate = {.a = 666};

  u64 xorshift64(Xor_Shift_State* xstate) {
    u64 x = xstate->a;
    x ^= x << 7;
    x ^= x >> 9;
    return xstate->a = x;
  }

  u64 brand() {
    return xorshift64(&xstate) % BRAND_MAX;
  }
  f64 brand_frange(f64 min, f64 max) {
    assert(max >= min);
    return (max-min) * (brand()/(f64)BRAND_MAX) + min;
  }
  f64 brand_f01() {
    return brand_frange(0,1);
  }
  u64 brand_range(u64 min, u64 max) {
    return (u64)round_f64(brand_frange(min, max));
  }
  s64 brand_srange(s64 min, s64 max) {
    return (s64)round_f64(brand_frange(min, max));
  }
#else 
  u64 brand();
  u64 brand_range(u64 min, u64 max);
  f64 brand_f01();
  f64 brand_frange(f64 min, f64 max);
  s64 brand_srange(s64 min, s64 max);
#endif

#endif
