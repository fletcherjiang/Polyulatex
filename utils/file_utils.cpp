/**
* @file file_utils.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "file_utils.h"
#include "mmpa/mmpa_api.h"
#include "common/log_inner.h"


namespace acl {
namespace file_utils {
class MmDirEntGuard {
public:
    MmDirEntGuard(mmDirent2 **dirEnts, int32_t count) : dirEntries_(dirEnts), count_(count)
    {
    }

    ~MmDirEntGuard()
    {
        mmScandirFree2(dirEntries_, count_);
    }

private:
    mmDirent2 **dirEntries_;
    int32_t count_;
};

static int32_t RegularFileFilterFn(const mmDirent2 *entry)
{
    return (static_cast<int32_t>(entry->d_type) == MM_DT_DIR) || (static_cast<int32_t>(entry->d_type) == MM_DT_REG);
}

aclError ListFiles(const std::string &dirName, FileNameFilterFn filter, std::vector<std::string> &names, int32_t maxDepth)
{
    if (maxDepth <= 0) {
        return ACL_SUCCESS;
    }

    mmDirent2 **dirEntries = nullptr;
    auto ret = mmScandir2(dirName.c_str(), &dirEntries, RegularFileFilterFn, nullptr);
    if (ret < 0) {
        ACL_LOG_WARN("scan dir failed. path = %s, ret = %d", dirName.c_str(), ret);
        return ACL_ERROR_READ_MODEL_FAILURE;
    }

    MmDirEntGuard guard(dirEntries, ret);
    for (int32_t i = 0; i < ret; ++i) {
        mmDirent2 *dirEnt = dirEntries[i];
        std::string name = std::string(dirEnt->d_name);
        if ((static_cast<int32_t>(dirEnt->d_type) == MM_DT_DIR) && (name != ".") && (name != "..")) {
            ACL_REQUIRES_OK(ListFiles(dirName + "/" += name, filter, names, maxDepth - 1));
        } else if (static_cast<int32_t>(dirEnt->d_type) == MM_DT_REG) {
            if ((filter == nullptr) || filter(name)) {
                names.emplace_back(dirName + "/" += name);
            }
        }
    }

    return ACL_SUCCESS;
}
} // namespace file_utils
} // namespace acl

