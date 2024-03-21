//===-- Basic operations on floating point numbers --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC___SUPPORT_FPUTIL_BASICOPERATIONS_H
#define LLVM_LIBC_SRC___SUPPORT_FPUTIL_BASICOPERATIONS_H

#include "FEnvImpl.h"
#include "FPBits.h"

#include "src/__support/CPP/type_traits.h"
#include "src/__support/UInt128.h"
#include "src/__support/common.h"
#include "src/__support/macros/optimization.h" // LIBC_UNLIKELY

namespace LIBC_NAMESPACE {
namespace fputil {

template <typename T, cpp::enable_if_t<cpp::is_floating_point_v<T>, int> = 0>
LIBC_INLINE T abs(T x) {
  return FPBits<T>(x).abs().get_val();
}

template <typename T, cpp::enable_if_t<cpp::is_floating_point_v<T>, int> = 0>
LIBC_INLINE T fmin(T x, T y) {
  const FPBits<T> bitx(x), bity(y);

  if (bitx.is_nan()) {
    return y;
  } else if (bity.is_nan()) {
    return x;
  } else if (bitx.sign() != bity.sign()) {
    // To make sure that fmin(+0, -0) == -0 == fmin(-0, +0), whenever x and
    // y has different signs and both are not NaNs, we return the number
    // with negative sign.
    return (bitx.is_neg()) ? x : y;
  } else {
    return (x < y ? x : y);
  }
}

template <typename T, cpp::enable_if_t<cpp::is_floating_point_v<T>, int> = 0>
LIBC_INLINE T fmax(T x, T y) {
  FPBits<T> bitx(x), bity(y);

  if (bitx.is_nan()) {
    return y;
  } else if (bity.is_nan()) {
    return x;
  } else if (bitx.sign() != bity.sign()) {
    // To make sure that fmax(+0, -0) == +0 == fmax(-0, +0), whenever x and
    // y has different signs and both are not NaNs, we return the number
    // with positive sign.
    return (bitx.is_neg() ? y : x);
  } else {
    return (x > y ? x : y);
  }
}

template <typename T, cpp::enable_if_t<cpp::is_floating_point_v<T>, int> = 0>
LIBC_INLINE T fdim(T x, T y) {
  FPBits<T> bitx(x), bity(y);

  if (bitx.is_nan()) {
    return x;
  }

  if (bity.is_nan()) {
    return y;
  }

  return (x > y ? x - y : 0);
}

template <typename T, cpp::enable_if_t<cpp::is_floating_point_v<T>, int> = 0>
LIBC_INLINE int canonicalize(T &cx, const T &x) {
  FPBits<T> sx(x);
  if (LIBC_UNLIKELY(sx.is_signaling_nan())) {
    cx = FPBits<T>::quiet_nan(sx.sign(), sx.get_explicit_mantissa()).get_val();
    raise_except_if_required(FE_INVALID);
    return 1;
  } else if constexpr (get_fp_type<T>() == FPType::X86_Binary80) {
    // All the pseudo and unnormal numbers are not canonical.
    // More precisely :
    // Exponent   |       Significand      | Meaning
    //            | Bits 63-62 | Bits 61-0 |
    // All Ones   |     00     |    Zero   | Pseudo Infinity, Value = Infinty
    // All Ones   |     00     |  Non-Zero | Pseudo NaN, Value = SNaN
    // All Ones   |     01     | Anything  | Pseudo NaN, Value = SNaN
    //            |   Bit 63   | Bits 62-0 |
    // All zeroes |   One      | Anything  | Pseudo Denormal, Value =
    //            |            |           | (−1)**s × m × 2**−16382
    // All Other  |   Zero     | Anything  | Unnormal, Value =
    //  Values    |            |           | (−1)**s × m × 2**−16382
    bool bit63 = sx.get_implicit_bit();
    UInt128 mantissa = sx.get_explicit_mantissa();
    bool bit62 = mantissa & (1ULL << 62);
    bool bit61 = mantissa & (1ULL << 61);
    int exponent = sx.get_biased_exponent();
    if (exponent == 0x7FFF) {
      if (!bit63 && !bit62) {
        if (!bit61) {
          cx = FPBits<T>::inf().get_val();
        } else {
          cx = FPBits<T>::signaling_nan().get_val();
        }
      } else if (!bit63 && bit62) {
        cx = FPBits<T>::signaling_nan().get_val();
      }
    } else if (exponent == 0 && bit63) {
      cx = FPBits<T>::get_canonical_val(sx.sign(), mantissa).get_val();
    } else if (!bit63) {
      cx = FPBits<T>::get_canonical_val(sx.sign(), mantissa).get_val();
    } else {
      cx = x;
    }
  } else {
    cx = x;
  }
  return 0;
}

} // namespace fputil
} // namespace LIBC_NAMESPACE

#endif // LLVM_LIBC_SRC___SUPPORT_FPUTIL_BASICOPERATIONS_H
