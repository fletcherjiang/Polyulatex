/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef GE_ALIGNED_PTR_H_
#define GE_ALIGNED_PTR_H_

#include <memory>
#include <functional>

namespace ge {
namespace {
constexpr size_t kAlignmentBytes = 16;
using deleter = std::function<void(uint8_t *)>;
using allocator = std::function<void(std::unique_ptr<uint8_t[], deleter> &base_addr)>;
}
class AlignedPtr {
 public:
  explicit AlignedPtr(size_t buffer_size, size_t alignment = kAlignmentBytes);
  AlignedPtr() = default;
  ~AlignedPtr() = default;
  AlignedPtr(const AlignedPtr &) = delete;
  AlignedPtr(AlignedPtr &&) = delete;
  AlignedPtr &operator=(const AlignedPtr &) = delete;
  AlignedPtr &operator=(AlignedPtr &&) = delete;

  const uint8_t *Get() const { return aligned_addr_; }
  uint8_t *MutableGet() { return aligned_addr_; }

  static std::shared_ptr<AlignedPtr> BuildFromAllocFunc(const allocator &alloc_func, const deleter &delete_func);

 private:
  std::unique_ptr<uint8_t[], deleter> base_ = nullptr;
  uint8_t *aligned_addr_ = nullptr;
};
}  // namespace ge
#endif//GE_ALIGNED_PTR_H_
