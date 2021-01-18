/**
* @file attr_utils.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_UTILS_ATTR_UTILS_H_
#define ACL_UTILS_ATTR_UTILS_H_

#include <string>
#include <sstream>
#include <map>

#include "acl/acl_base.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "utils/string_utils.h"
#include "types/op_attr.h"
#include "types/acl_op.h"
#include "types/tensor_desc_internal.h"
#include "types/op_model.h"

namespace acl {
namespace attr_utils {
ACL_FUNC_VISIBILITY std::string GeAttrValueToString(const ge::GeAttrValue &value);

ACL_FUNC_VISIBILITY std::string AttrMapToString(const std::map<std::string, ge::GeAttrValue> &attrMap);

ACL_FUNC_VISIBILITY size_t AttrMapToDigest(const std::map<std::string, ge::GeAttrValue> &attrMap);

ACL_FUNC_VISIBILITY bool AttrValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs);

ACL_FUNC_VISIBILITY bool OpAttrEquals(const aclopAttr *lhs, const aclopAttr *rhs);

ACL_FUNC_VISIBILITY uint64_t GetCurrentTimestamp();

bool ValueRangeCheck(const std::map<AttrRangeType, ge::GeAttrValue> &valueRange,
                     const aclDataBuffer *value, aclDataType dataType);

bool SaveConstToAttr(OpModelDef& modelDef);

bool SaveConstToAttr(const AclOp &aclOp, aclopAttr *opAttr);
} // namespace attr_utils
} // acl

#endif // ACL_UTILS_ATTR_UTILS_H_
