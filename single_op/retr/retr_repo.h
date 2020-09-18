/* *
 * @file retr_repo.h
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ACL_SINGLE_OP_RETR_REPO_H
#define ACL_SINGLE_OP_RETR_REPO_H

#include "runtime/rt.h"
#include "acl/acl_base.h"
#include "acl/ops/acl_fv.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
class AclFvRepo {
public:
    AclFvRepo();
    ~AclFvRepo();

    /* *
     * execute retr repo add task, including check parameters launch task and check result
     * @param type: add type
     * @param featureInfo: add feature information
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Add(aclfvSearchType type, aclfvFeatureInfo *featureInfo);

    /* *
     * execute retr  repo del task, including check parameters launch task and check result
     * @param type: delete type
     * @param repoRange: repo range information
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Del(aclfvSearchType type, aclfvRepoRange *repoRange);

private:
    /* *
     * construct the input parameters of the rtCpuKernelLaunch
     * @param type: add or delete type
     * @param devData: device data pointer
     * @param ioAddrNum: io address number
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @param stream: stream
     * @return ACL_SUCCESS:success other:failed
     */
    aclError PrepareInput(aclfvSearchType type, void *devData, uint32_t ioAddrNum,
                          char *args, uint32_t argsSize, aclrtStream stream);

    /* *
     * Check add task parameters
     * @param featureInfo: add feature information
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError AddCheck(aclfvFeatureInfo *featureInfo, aclrtStream stream);

    /* *
     * Check del task parameters
     * @param repoRange: repo range information
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError DelCheck(aclfvRepoRange *repoRange, aclrtStream stream);

    /* *
     * execute retr Search task
     * @param retrArgs: kernel args
     * @param addFeatureNum: repo add feature number
     * @param addFeatureCount: repo add feature index
     * @param stream: stream of task execute
     * @param notify: use to wait for current task end
     * @return ACL_SUCCESS:success other:failed
     */
    aclError AddTask(aclfvArgs &retrArgs, uint32_t &addFeatureNum, uint32_t &addFeatureCount, aclrtStream stream,
        rtNotify_t notify);

    /* *
     * execute retr Search task
     * @param retrArgs: kernel args
     * @param featureInfo: add feature information
     * @param stream: stream of task execute
     * @param notify: use to wait for current task end
     * @return ACL_SUCCESS:success other:failed
     */
    aclError AddNvM(aclfvArgs &retrArgs, aclfvFeatureInfo *featureInfo, aclrtStream stream, rtNotify_t notify);

    /* *
     * execute retr Search task
     * @param retrArgs: kernel args
     * @param featureInfo: add feature information
     * @param stream: stream of task execute
     * @param notify: use to wait for current task end
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Add1vN(aclfvArgs &retrArgs, aclfvFeatureInfo *featureInfo, aclrtStream stream, rtNotify_t notify);

    /* *
     * execute retr repo add task
     * @param type: add type
     * @param featureInfo: add feature information
     * @param stream: stream of task execute
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError AddExecute(aclfvSearchType type, aclfvFeatureInfo *featureInfo, aclrtStream stream, char *args,
        uint32_t argsSize);

    /* *
     * execute retr repo del task
     * @param repoRange: repo range information
     * @param stream: stream of task execute
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @return ACL_SUCCESS:success other:failed
     */
    aclError DelExecute(aclfvRepoRange *repoRange, aclrtStream stream, const char *args, uint32_t argsSize);
private:
    rtNotify_t notify_; // use to wait for task end
    uint32_t notifyId_;
    aclrtRunMode runMode_; // support running on host or device
};
} // namespace retr
} // namespace acl

#endif // ACL_SINGLE_OP_RETR_REPO_H