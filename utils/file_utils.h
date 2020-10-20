/**
* @file file_utils.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_UTILS_FILE_UTILS_H
#define ACL_UTILS_FILE_UTILS_H

#include <string>
#include <vector>

#include "acl/acl_base.h"

namespace acl {
namespace file_utils {
using FileNameFilterFn = bool(const std::string &fileName);

aclError ListFiles(const std::string &dirName,
    FileNameFilterFn filter,
    std::vector<std::string> &names,
    int maxDepth);
} // namespace file_utils
} // namespace acl

#endif // ACL_UTILS_FILE_UTILS_H
