/**
* @file fp16_impl.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_TYPES_FP16_IMPL_H_
#define ACL_TYPES_FP16_IMPL_H_

#include <cstdint>
#include <cmath>
#include <cmath>
#include <algorithm>

/**
 * @ingroup fp16 basic operator
 * @brief   get sign of fp16
 */
#define FP16_EXTRAC_SIGN(x)            (((x) >> 15U) & 1U)
/**
 * @ingroup fp16 basic parameter
 * @brief   maximum exponent value of fp16 is 15(11111)
 */
static constexpr uint32_t FP16_MAX_EXP = 0x001FU;
/**
 * @ingroup fp16 basic operator
 * @brief   get exponent of fp16
 */
#define FP16_EXTRAC_EXP(x)             (((x) >> 10U) & FP16_MAX_EXP)
/**
 * @ingroup fp16 basic operator
 * @brief   get mantissa of fp16
 */
#define FP16_EXTRAC_MAN(x)             ((((x) >> 0U) & 0x3FFU)    | (((((x) >> 10U) & 0x1FU) > 0 ? 1 : 0) * 0x400))
/**
 * @ingroup fp16 basic parameter
 * @brief   the mantissa bit length of fp16 is 10
 */
static constexpr uint32_t FP16_MAN_LEN = 10U;
/**
 * @ingroup fp16 basic parameter
 * @brief   maximum mantissa value of fp16(11111 11111)
 */
static constexpr uint32_t FP16_MAX_MAN = 0x03FFU;
/**
 * @ingroup fp16 basic operator
 * @brief   constructor of fp16 from sign exponent and mantissa
 */
#define FP16_CONSTRUCTOR(s, e, m)        (((s) << f) | ((e) << FP16_MAN_LEN) | ((m) & FP16_MAX_MAN))
/**
 * @ingroup fp32 basic parameter
 * @brief   bit index of sign in float/fp32
 */
static constexpr uint32_t FP32_SIGN_INDEX = 31U;
/**
 * @ingroup fp32 basic parameter
 * @brief   the mantissa bit length of float/fp32 is 23
 */
static constexpr uint32_t FP32_MAN_LEN = 23U;
/**
 * @ingroup fp32 basic parameter
 * @brief   maximum mantissa value of fp32    (1111 1111 1111 1111 1111 111)
 */
static constexpr uint32_t FP32_MAX_MAN = 0x7FFFFFU;
/**
 * @ingroup fp32 basic operator
 * @brief   constructor of fp32 from sign exponent and mantissa
 */
#define FP32_CONSTRUCTOR(s, e, m)        (((s) << FP32_SIGN_INDEX) | ((e) << FP32_MAN_LEN) | ((m) & FP32_MAX_MAN))

namespace acl {
/**
 * @ingroup fp16 basic parameter
 * @brief   fp16 exponent bias
 */
static constexpr uint32_t FP16_EXP_BIAS = 15U;
/**
 * @ingroup fp16 basic parameter
 * @brief   the exponent bit lengt h of fp16 is 5
 */
static constexpr int32_t FP16_EXP_LEN = 5;
/**
 * @ingroup fp16 basic parameter
 * @brief   bit index of sign in fp16
 */
static constexpr uint32_t FP16_SIGN_INDEX = 15U;
/**
 * @ingroup fp16 basic parameter
 * @brief   sign mask of fp16         (1 00000 00000 00000)
 */
static constexpr uint32_t FP16_SIGN_MASK = 0x8000U;
/**
 * @ingroup fp16 basic parameter
 * @brief   exponent mask of fp16     (  11111 00000 00000)
 */
static constexpr uint32_t FP16_EXP_MASK = 0x7C00U;
/**
 * @ingroup fp16 basic parameter
 * @brief   mantissa mask of fp16     (        11111 11111)
 */
static constexpr uint32_t FP16_MAN_MASK = 0x03FFU;
/**
 * @ingroup fp16 basic parameter
 * @brief   hide bit of mantissa of fp16(   1 00000 00000)
 */
static constexpr uint32_t FP16_MAN_HIDE_BIT = 0x0400U;
/**
 * @ingroup fp16 basic parameter
 * @brief   maximum value            (0111 1011 1111 1111)
 */
static constexpr int32_t FP16_MAX = 0x7BFF;
/**
 * @ingroup fp16 basic parameter
 * @brief   minimum value            (1111 1011 1111 1111)
 */
static constexpr int32_t FP16_MIN = 0xFBFF;
/**
 * @ingroup fp16 basic parameter
 * @brief   absolute maximum value   (0111 1111 1111 1111)
 */
static constexpr int32_t FP16_ABS_MAX = 0x7FFF;
/**
 * @ingroup fp16 basic parameter
 * @brief   maximum valid exponent value of fp16 is 14(11110)
 */
static constexpr int32_t FP16_MAX_VALID_EXP = 0x001E;
/**
 * @ingroup fp32 basic parameter
 * @brief   fp32 exponent bias
 */
static constexpr int32_t FP32_EXP_BIAS = 127;
/**
 * @ingroup fp32 basic parameter
 * @brief   the exponent bit length of float/fp32 is 8
 */
static constexpr int32_t FP32_EXP_LEN = 8;
/**
 * @ingroup fp32 basic parameter
 * @brief   sign mask of fp32         (1 0000 0000  0000 0000 0000 0000 000)
 */
static constexpr uint32_t FP32_SIGN_MASK = 0x80000000U;
/**
 * @ingroup fp32 basic parameter
 * @brief   exponent mask of fp32     (  1111 1111  0000 0000 0000 0000 000)
 */
static constexpr uint32_t FP32_EXP_MASK = 0x7F800000U;
/**
 * @ingroup fp32 basic parameter
 * @brief   mantissa mask of fp32     (             1111 1111 1111 1111 111)
 */
static constexpr uint32_t FP32_MAN_MASK = 0x007FFFFFU;
/**
 * @ingroup fp32 basic parameter
 * @brief   hide bit of mantissa of fp32      (  1  0000 0000 0000 0000 000)
 */
static constexpr uint32_t FP32_MAN_HIDE_BIT = 0x00800000U;
/**
 * @ingroup fp32 basic parameter
 * @brief   absolute maximum value    (0 1111 1111  1111 1111 1111 1111 111)
 */
static constexpr uint32_t FP32_ABS_MAX = 0x7FFFFFFFU;
/**
 * @ingroup fp32 basic parameter
 * @brief   maximum exponent value of fp32 is 255(1111 1111)
 */
static constexpr int32_t FP32_MAX_EXP = 0xFF;
/**
 * @ingroup fp64 basic parameter
 * @brief   fp64 exponent bias
 */
static constexpr int32_t FP64_EXP_BIAS = 1023;
/**
 * @ingroup fp64 basic parameter
 * @brief   the exponent bit length of double/fp64 is 11
 */
static constexpr int32_t FP64_EXP_LEN = 11;
/**
 * @ingroup fp64 basic parameter
 * @brief   the mantissa bit length of double/fp64 is 52
 */
static constexpr int32_t FP64_MAN_LEN = 52;
/**
 * @ingroup fp64 basic parameter
 * @brief   bit index of sign in double/fp64 is 63
 */
static constexpr int32_t FP64_SIGN_INDEX = 63;
/**
 * @ingroup fp64 basic parameter
 * @brief   sign mask of fp64                 (1 000                   (total 63bits 0))
 */
static constexpr uint64_t FP64_SIGN_MASK = 0x8000000000000000U;
/**
 * @ingroup fp64 basic parameter
 * @brief   exponent mask of fp64            (0 1 11111 11111  0000?-?-(total 52bits 0))
 */
static constexpr uint64_t FP64_EXP_MASK = 0x7FF0000000000000U;
/**
 * @ingroup fp64 basic parameter
 * @brief   mantissa mask of fp64            (                 1111?-?-(total 52bits 1))
 */
static constexpr uint64_t FP64_MAN_MASK = 0x000FFFFFFFFFFFFFU;
/**
 * @ingroup fp64 basic parameter
 * @brief   hide bit of mantissa of fp64     (               1 0000?-?-(total 52bits 0))
 */
static constexpr uint64_t FP64_MAN_HIDE_BIT = 0x0010000000000000U;
/**
 * @ingroup fp64 basic parameter
 * @brief   absolute maximum value           (0 111?-?-(total 63bits 1))
 */
static constexpr uint64_t FP64_ABS_MAX = 0x7FFFFFFFFFFFFFFFU;
/**
 * @ingroup fp64 basic parameter
 * @brief   maximum exponent value of fp64 is 2047(1 11111 11111)
 */
static constexpr int32_t FP64_MAX_EXP = 0x07FF;
/**
 * @ingroup fp64 basic parameter
 * @brief   maximum mantissa value of fp64  (111?-?-(total 52bits 1))
 */
static constexpr uint64_t FP64_MAX_MAN = 0xFFFFFFFFFFFU;
/**
 * @ingroup integer special value judgment
 * @brief   maximum positive value of int8_t            (0111 1111)
 */
static constexpr int32_t INT8_T_MAX = 0x7F;
/**
 * @ingroup integer special value judgment
 * @brief   maximum value of a data with 8 bits length  (1111 111)
 */
static constexpr int32_t BIT_LEN8_MAX = 0xFF;
/**
 * @ingroup integer special value judgment
 * @brief   maximum positive value of int16_t           (0111 1111 1111 1111)
 */
static constexpr uint32_t INT16_T_MAX = 0x7FFFU;
/**
 * @ingroup integer special value judgment
 * @brief   maximum value of a data with 16 bits length (1111 1111 1111 1111)
 */
static constexpr uint32_t BIT_LEN16_MAX = 0xFFFFU;
/**
 * @ingroup integer special value judgment
 * @brief   maximum positive value of int32_t           (0111 1111 1111 1111 1111 1111 1111 1111)
 */
static constexpr uint32_t INT32_T_MAX = 0x7FFFFFFFU;
/**
 * @ingroup integer special value judgment
 * @brief   maximum value of a data with 32 bits length (1111 1111 1111 1111 1111 1111 1111 1111)
 */
static constexpr uint32_t BIT_LEN32_MAX = 0xFFFFFFFFU;
/**
 * @ingroup fp16_t enum
 * @brief   round mode of last valid digital
 */
typedef enum TagFp16RoundMode {
    ROUND_TO_NEAREST = 0,    /**< round to nearest even */
    ROUND_BY_TRUNCATED,      /**< round by truncated    */
    ROUND_MODE_RESERVED,
} fp16RoundMode_t;

uint16_t FloatToFp16(float val);

float Fp16ToFloat(uint16_t val);

bool Fp16Eq(uint16_t lhs, uint16_t rhs);
} // namespace acl

#endif // ACL_TYPES_FP16_IMPL_H_
