/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef INC_REGISTER_OP_TILING_REGISTRY_H_
#define INC_REGISTER_OP_TILING_REGISTRY_H_

#include "external/graph/ascend_string.h"
#include "external/graph/operator.h"
#include "external/graph/tensor.h"
#include "external/register/register_error_codes.h"
#include "external/register/register_types.h"
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

extern const char *ATTR_NAME_ATOMIC_CLEAN_WORKSPACE;

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

extern thread_local int64_t last_op_tiling_perf;

enum TensorArgType {
  TA_NONE,
  TA_SINGLE,
  TA_LIST,
};

using ByteBuffer = std::stringstream;

class TeOpVarAttrArgsImpl;
class TeOpVarAttrArgs {
  friend class VarAttrHelper;

 public:
  TeOpVarAttrArgs() = default;
  ~TeOpVarAttrArgs() = default;

  const uint8_t *GetData(const std::string &name, const std::string &dtype, size_t &size) const;

 private:
  std::shared_ptr<TeOpVarAttrArgsImpl> impl_;
};

struct TeOpTensor {
  std::vector<int64_t> shape;
  std::vector<int64_t> ori_shape;
  std::string format;
  std::string ori_format;
  std::string dtype;
  std::string name;
  std::map<std::string, std::string> attrs;
};

struct TeOpTensorArg {
  TensorArgType arg_type;
  std::vector<TeOpTensor> tensor;
};

struct OpRunInfo {
  uint32_t block_dim;
  std::vector<int64_t> workspaces;
  ByteBuffer tiling_data;
  bool clear_atomic;
  uint32_t tiling_key;
};

using TeOpAttrArgs = std::vector<std::string>;
using TeConstTensorData = std::tuple<const uint8_t *, size_t, ge::Tensor>;

struct TeOpParas {
  std::vector<TeOpTensorArg> inputs;
  std::vector<TeOpTensorArg> outputs;
  std::map<std::string, TeConstTensorData> const_inputs;
  TeOpAttrArgs attrs;
  std::string op_type;
  TeOpVarAttrArgs var_attrs;
};

struct OpCompileInfo {
  std::string str;
  std::string key;
};

using OpTilingFunc = std::function<bool(const TeOpParas &, const OpCompileInfo &, OpRunInfo &)>;

using OpTilingFuncPtr = bool (*)(const TeOpParas &, const OpCompileInfo &, OpRunInfo &);

class FMK_FUNC_HOST_VISIBILITY OpTilingRegistryInterf {
 public:
  OpTilingRegistryInterf(std::string op_type, OpTilingFunc func);
  ~OpTilingRegistryInterf() = default;
  static std::map<std::string, OpTilingFunc> &RegisteredOpInterf();
};

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

namespace utils {
class OpRunInfoImpl;
class OpRunInfo {
 public:
  OpRunInfo();
  ~OpRunInfo() = default;
  OpRunInfo(uint32_t block_dim, bool clear_atomic, uint32_t tiling_key);
  // Copy
  OpRunInfo(const OpRunInfo &runinfo);
  // Move
  OpRunInfo(OpRunInfo &&runinfo);
  // Copy
  OpRunInfo &operator=(const OpRunInfo &runinfo);
  // Move
  OpRunInfo &operator=(OpRunInfo &&runinfo);

  void SetBlockDim(uint32_t block_dim);
  uint32_t GetBlockDim();

  void AddWorkspace(int64_t workspace);
  size_t GetWorkspaceNum();
  ge::graphStatus GetWorkspace(size_t idx, int64_t &workspace);
  ge::graphStatus GetAllWorkspaces(std::vector<int64_t> &workspace);

  template<class T>
  void AddTilingData(const T &value) {
    AddTilingData(reinterpret_cast<const char *>(&value), sizeof(value));
  }
  void AddTilingData(const char *value, size_t size);
  ByteBuffer &GetAllTilingData();
  void InternelSetTiling(ByteBuffer &value);

  void SetClearAtomic(bool clear_atomic);
  bool GetClearAtomic() const;

  void SetTilingKey(uint32_t tiling_key);
  uint32_t GetTilingKey() const;

 private:
  std::shared_ptr<OpRunInfoImpl> impl_;
};

class OpCompileInfoImpl;
class OpCompileInfo {
 public:
  OpCompileInfo();
  ~OpCompileInfo() = default;
  OpCompileInfo(const ge::AscendString &key, const ge::AscendString &value);
  // Copy
  OpCompileInfo(const OpCompileInfo &compileinfo);
  // Move
  OpCompileInfo(OpCompileInfo &&compileinfo);
  // Copy
  OpCompileInfo &operator=(const OpCompileInfo &compileinfo);
  // Move
  OpCompileInfo &operator=(OpCompileInfo &&compileinfo);

  void SetKey(const ge::AscendString &key);
  const ge::AscendString &GetKey() const;

  void SetValue(const ge::AscendString &value);
  const ge::AscendString &GetValue() const;

 private:
  std::shared_ptr<OpCompileInfoImpl> impl_;
};
using OpTilingFuncV2 = std::function<bool(const ge::Operator &, const OpCompileInfo &, OpRunInfo &)>;
using OpTilingFuncV2Ptr = bool (*)(const ge::Operator &, const OpCompileInfo &, OpRunInfo &);
class FMK_FUNC_HOST_VISIBILITY OpTilingRegistryInterf_V2 {
 public:
  OpTilingRegistryInterf_V2(std::string op_type, OpTilingFuncV2 func);
  ~OpTilingRegistryInterf_V2() = default;
  static std::map<std::string, OpTilingFuncV2> &RegisteredOpInterf();
};
}  // namespace utils
}  // namespace optiling

#endif  // INC_REGISTER_OP_TILING_REGISTRY_H_
