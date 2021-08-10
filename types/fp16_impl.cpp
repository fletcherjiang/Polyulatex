/**
* @file fp16_impl.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "fp16_impl.h"

namespace acl {
/**
 * @ingroup fp16_t global filed
 * @brief   round mode of last valid digital
 */
fp16RoundMode_t g_roundMode = ROUND_TO_NEAREST;

union TypeUnion {
    float fVal;
    uint32_t uVal;
};

static void ExtractFP16(const uint16_t &val, uint16_t *s, int16_t *e, uint16_t *m)
{
    // 1.Extract
    *s = FP16_EXTRAC_SIGN(val);
    *e = FP16_EXTRAC_EXP(val);
    *m = FP16_EXTRAC_MAN(val);

    // Denormal
    if ((*e) == 0) {
        *e = 1;
    }
}

/**
 * @ingroup fp16_t static method
 * @param [in] man       truncated mantissa
 * @param [in] shiftOut left shift bits based on ten bits
 * @brief   judge whether to add one to the result while converting fp16_t to other datatype
 * @return  Return true if add one, otherwise false
 */
static bool IsRoundOne(uint64_t man, uint16_t truncLen)
{
    uint64_t mask0 = 0x4UL;
    uint64_t mask1 = 0x2UL;
    uint64_t mask2;
    uint16_t shiftOut = truncLen - 2U; // shift 2 byte
    mask0 = mask0 << shiftOut;
    mask1 = mask1 << shiftOut;
    mask2 = mask1 - 1U;

    bool lastBit = ((man & mask0) > 0UL);
    bool truncHigh = false;
    bool truncLeft = false;
    if (ROUND_TO_NEAREST == g_roundMode) {
        truncHigh = ((man & mask1) > 0UL);
        truncLeft = ((man & mask2) > 0UL);
    }
    return (truncHigh && (truncLeft || lastBit));
}

/**
 * @ingroup fp16_t public method
 * @param [in] exp       exponent of fp16_t value
 * @param [in] man       exponent of fp16_t value
 * @brief   normalize fp16_t value
 * @return
 */
static void Fp16Normalize(uint16_t &exp, uint16_t &man)
{
    if (exp >= FP16_MAX_EXP) {
        exp = FP16_MAX_EXP - 1U;
        man = FP16_MAX_MAN;
    } else if (exp == 0U && man == FP16_MAN_HIDE_BIT) {
        exp++;
        man = 0U;
    }
}

float Fp16ToFloat(uint16_t val)
{
    uint16_t hfSign;
    uint16_t hfMan;
    int16_t hfExp;
    ExtractFP16(val, &hfSign, &hfExp, &hfMan);

    while ((hfMan != 0U) && ((hfMan & FP16_MAN_HIDE_BIT) == 0U)) {
        hfMan <<= 1U;
        hfExp--;
    }

    uint32_t eRet;
    uint32_t mRet;
    if (hfMan == 0U) {
        eRet = 0U;
        mRet = 0U;
    } else {
        eRet = hfExp - FP16_EXP_BIAS + FP32_EXP_BIAS;
        mRet = hfMan & FP16_MAN_MASK;
        mRet = mRet << (FP32_MAN_LEN - FP16_MAN_LEN);
    }

    uint32_t sRet = hfSign;
    TypeUnion u;
    u.uVal = FP32_CONSTRUCTOR(sRet, eRet, mRet);
    auto ret = u.fVal;
    return ret;
}

uint16_t FloatToFp16(float val)
{
    TypeUnion u;
    u.fVal = val;
    uint32_t ui32V = u.uVal;  // 1:8:23bit sign:exp:man
    auto sRet = static_cast<uint16_t>((ui32V & FP32_SIGN_MASK) >> FP32_SIGN_INDEX);  // 4Byte->2Byte
    uint32_t eF = (ui32V & FP32_EXP_MASK) >> FP32_MAN_LEN; // 8 bit exponent
    uint32_t mF = (ui32V & FP32_MAN_MASK); // 23 bit mantissa dont't need to care about denormal
    uint32_t mLenDelta = FP32_MAN_LEN - FP16_MAN_LEN;

    uint16_t mRet;
    uint16_t eRet;
    // Exponent overflow/NaN converts to signed inf/NaN
    if (eF > 0x8FU) {  // 0x8Fu:142=127+15
        eRet = FP16_MAX_EXP - 1U;
        mRet = FP16_MAX_MAN;
    } else if (eF <= 0x70U) {  // 0x70u:112=127-15 Exponent underflow converts to denormalized half or signed zero
        eRet = 0U;
        if (eF >= 0x67U) {  // 0x67:103=127-24 Denormal
            mF = (mF | FP32_MAN_HIDE_BIT);
            uint16_t shiftOut = FP32_MAN_LEN;
            uint64_t mTmp = (static_cast<uint64_t>(mF)) << (eF - 0x67U);

            bool needRound = IsRoundOne(mTmp, shiftOut);
            mRet = static_cast<uint16_t>(mTmp >> shiftOut);
            if (needRound) {
                mRet++;
            }
        } else if ((eF == 0x66U) && (mF > 0U)) {  // 0x66:102 Denormal 0<f_v<min(Denormal)
            mRet = 1U;
        } else {
            mRet = 0U;
        }
    } else {  // Regular case with no overflow or underflow
        eRet = eF - 0x70U;
        bool needRound = IsRoundOne(mF, mLenDelta);
        mRet = static_cast<uint16_t>(mF >> mLenDelta);
        if (needRound) {
            mRet++;
        }
        if ((mRet & FP16_MAN_HIDE_BIT) != 0U) {
            eRet++;
        }
    }

    Fp16Normalize(eRet, mRet);
    uint16_t ret = FP16_CONSTRUCTOR(sRet, eRet, mRet);
    return ret;
}

bool Fp16Eq(uint16_t lhs, uint16_t rhs)
{
    return (lhs == rhs) || (((lhs | rhs) & INT16_T_MAX) == 0U);
}
} // namespace acl
