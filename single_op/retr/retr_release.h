/**
* @file retr_release.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/


#ifndef ACL_SINGLE_OP_RETR_RELEASE_H
#define ACL_SINGLE_OP_RETR_RELEASE_H

#include "runtime/rt.h"
#include "acl/acl_base.h"

namespace acl {
namespace retr {
class AclFvRelease {
public:
    AclFvRelease();
    ~AclFvRelease();

    /**
     * execute retr release task, including check parameters launch task and check result
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Release();

private:
    /**
     * Check parameters and construct the input parameters of the rtCpuKernelLaunch
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError PrepareInput(char *args, uint32_t argsSize);

private:
    rtStream_t stream_;  // separate stream for init task
    void *retCode_;      // result of init task
};
}  // namespace retr
}  // namespace acl

#endif  // ACL_SINGLE_OP_RETR_RELEASE_H