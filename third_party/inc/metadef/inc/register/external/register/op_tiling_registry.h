/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INC_EXTERNAL_REGISTER_OP_TILING_REGISTRY_H_
#define INC_EXTERNAL_REGISTER_OP_TILING_REGISTRY_H_

#include <functional>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>
#include "external/graph/operator.h"
#include "external/register/register_error_codes.h"
#include "external/register/register_types.h"
#include "external/register/op_tiling_info.h"

#define REGISTER_OP_TILING(optype, opfunc) REGISTER_OP_TILING_UNIQ_HELPER(optype, opfunc, __COUNTER__)

#define REGISTER_OP_TILING_UNIQ_HELPER(optype, opfunc, counter) REGISTER_OP_TILING_UNIQ(optype, opfunc, counter)

#define REGISTER_OP_TILING_UNIQ(optype, opfunc, counter)                                                               \
  static OpTilingRegistryInterf g_##optype##TilingRegistryInterf##counter(#optype, opfunc)

#define REGISTER_OP_TILING_V2(optype, opfunc) REGISTER_OP_TILING_UNIQ_HELPER_V2(optype, opfunc, __COUNTER__)

#define REGISTER_OP_TILING_UNIQ_HELPER_V2(optype, opfunc, counter) REGISTER_OP_TILING_UNIQ_V2(optype, opfunc, counter)

#define REGISTER_OP_TILING_UNIQ_V2(optype, opfunc, counter)                                                            \
  static optiling::utils::OpTilingRegistryInterf_V2 g_##optype##TilingRegistryInterf##counter(#optype, opfunc)

using Status = domi::Status;
namespace optiling {
template<class T>
ByteBuffer &ByteBufferPut(ByteBuffer &buf, const T &value) {
  buf.write(reinterpret_cast<const char *>(&value), sizeof(value));
  buf.flush();
  return buf;
}

template<class T>
ByteBuffer &ByteBufferGet(ByteBuffer &buf, T &value) {
  buf.read(reinterpret_cast<char *>(&value), sizeof(value));
  return buf;
}

size_t ByteBufferGetAll(ByteBuffer &buf, char *dest, size_t dest_len);
ByteBuffer &ByteBufferPut(ByteBuffer &buf, const uint8_t *data, size_t dest_len);

using OpTilingFunc = std::function<bool(const TeOpParas &, const OpCompileInfo &, OpRunInfo &)>;
using OpTilingFuncPtr = std::shared_ptr<OpTilingFunc>;
class FMK_FUNC_HOST_VISIBILITY OpTilingRegistryInterf {
 public:
  OpTilingRegistryInterf(std::string op_type, OpTilingFunc func);
  ~OpTilingRegistryInterf() = default;
  static std::unordered_map<std::string, OpTilingFunc> &RegisteredOpInterf();
};

namespace utils {
using OpTilingFuncV2 = std::function<bool(const ge::Operator &, const OpCompileInfo &, OpRunInfo &)>;
using OpTilingFuncV2Ptr = std::shared_ptr<OpTilingFuncV2>;

class FMK_FUNC_HOST_VISIBILITY OpTilingRegistryInterf_V2 {
 public:
  OpTilingRegistryInterf_V2(const std::string &op_type, OpTilingFuncV2 func);
  ~OpTilingRegistryInterf_V2() = default;
  static std::unordered_map<std::string, OpTilingFuncV2> &RegisteredOpInterf();
};
}  // namespace utils
}  // namespace optiling
#endif  // INC_EXTERNAL_REGISTER_OP_TILING_REGISTRY_H_
