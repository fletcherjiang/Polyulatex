/**
* @file fp16.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_base.h"

#include "fp16_impl.h"

float aclFloat16ToFloat(aclFloat16 val)
{
    return acl::Fp16ToFloat(val);
}

aclFloat16 aclFloatToFloat16(float val)
{
    return acl::FloatToFp16(val);
}
