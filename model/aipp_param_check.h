/**
* @file aipp_param_check.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef AIPP_PARAM_CHECK_H_
#define AIPP_PARAM_CHECK_H_
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <sstream>
#include "log_inner.h"
#include "securec.h"
#include "acl/acl_mdl.h"
#include "common/dynamic_aipp.h"
#include "executor/ge_executor.h"
#include "common/ge_inner_error_codes.h"
#include "common/ge_types.h"
#include "model_desc_internal.h"

uint64_t GetSrcImageSize(const aclmdlAIPP *aippParmsSet);
aclError AippParamsCheck(const aclmdlAIPP *aippParmsSet, std::string socVersion,
                         int32_t aippOutputW, int32_t aippOutputH, bool isNewModel);

#endif // AIPP_PARAM_CHECK_H_
