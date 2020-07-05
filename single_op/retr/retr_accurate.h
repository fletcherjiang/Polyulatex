/* *
 * @file retr_accurate.h
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef ACL_SINGLE_OP_RETR_ACCURATE_H
#define ACL_SINGLE_OP_RETR_ACCURATE_H

#include "runtime/rt.h"
#include "acl/acl_base.h"
#include "acl/ops/acl_fv.h"

namespace acl {
namespace retr {
// accurate operation type
enum aclAccurateType {
    ACCURATE_DELETE, // accurate delete
    ACCURATE_MODFILY // accurate modfily
};

class AclFvAccurate {
public:
    AclFvAccurate();
    ~AclFvAccurate();

    /* *
     * execute retr accurate Del, including check parameters launch task and check result
     * @param featureInfo: accurate delete feature information
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Delete(aclfvFeatureInfo *featureInfo, aclrtStream stream);

    /* *
     * execute retr accurate Modify, including check parameters launch task and check result
     * @param featureInfo: accurate modify feature information
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Modify(aclfvFeatureInfo *featureInfo, aclrtStream stream);

private:
    /* *
     * Check parameters and construct the input parameters of the rtCpuKernelLaunch
     * @param typeAccurate: accurate feature information
     * @param featureInfo: accurate feature information
     * @param stream: stream of task execute
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError PrepareInput(aclAccurateType typeAccurate, aclfvFeatureInfo *featureInfo, aclrtStream stream, char *args,
        uint32_t argsSize);

    /* *
     * execute retr accurate del or modify, including check parameters launch task and check result
     * @param typeAccurate: accurate feature information
     * @param featureInfo: accurate feature information
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError AccurateDelOrModify(aclAccurateType typeAccurate, aclfvFeatureInfo *featureInfo, aclrtStream stream);

    /* *
     * execute retr accurate del or modify, including check parameters launch task and check result
     * @param featureInfo: accurate feature information
     * @param stream: stream of task execute
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError ExecAccurateDelOrModify(aclfvFeatureInfo *featureInfo, aclrtStream stream, const char *args,
        uint32_t argsSize);

private:
    rtNotify_t notify_; // use to wait for task end
    uint32_t notifyId_;
    aclrtRunMode runMode_; // support running on host or device
};
} // namespace retr
} // namespace acl

#endif // ACL_SINGLE_OP_RETR_ACCURATE_H