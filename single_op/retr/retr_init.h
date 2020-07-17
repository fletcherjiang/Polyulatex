/**
* @file retr_init.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_SINGLE_OP_RETR_INIT_H
#define ACL_SINGLE_OP_RETR_INIT_H

#include "runtime/rt.h"
#include "acl/acl_base.h"
#include "acl/ops/acl_fv.h"

namespace acl {
namespace retr {
class AclFvInit {
public:
    AclFvInit();
    ~AclFvInit();

    /**
     * execute retr init task, including check parameters launch task and check result
     * @param initPara: fv init param
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Init(aclfvInitPara *initPara);

private:
    /**
     * Check parameters and construct the input parameters of the rtCpuKernelLaunch
     * @param initPara: fv init param
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError PrepareInput(aclfvInitPara *initPara, char *args, uint32_t argsSize);

private:
    rtStream_t stream_;  // separate stream for init task
    aclrtRunMode runMode_; // support running on host or device
};
}  // namespace retr
}  // namespace acl

#endif  // ACL_SINGLE_OP_RETR_INIT_H