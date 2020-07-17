/**
* @file retr_search.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_SINGLE_OP_RETR_SEARCH_H
#define ACL_SINGLE_OP_RETR_SEARCH_H

#include "runtime/rt.h"
#include "acl/acl_base.h"
#include "acl/ops/acl_fv.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
class AclFvSearch {
public:
    AclFvSearch();
    ~AclFvSearch();

    /**
     * execute retr Search task, including check parameters launch task and check result
     * @param type: search type
     * @param searchInput: search input
     * @param searchRst: search result
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Search(
        aclfvSearchType type, aclfvSearchInput *searchInput, aclfvSearchResult *searchRst, aclrtStream stream);

private:
    /**
     * get and check result of stream task, 0 represent success
     * @param searchRst: search result
     * @return ACL_SUCCESS:success other:failed
     */
    aclError GetSearchResult(aclfvSearchResult *searchRst);

    /**
     * launch search task to aicpu
     * @param retrArgs: kernel args
     * @param searchInput: search input
     * @return ACL_SUCCESS:success other:failed
     */
    aclError LaunchSearchTask(aclfvArgs &retrArgs, aclfvSearchInput *searchInput);

    /**
     * execute retr NvM Search task
     * @param retrArgs: kernel args
     * @param searchInput: search input
     * @return ACL_SUCCESS:success other:failed
     */
    aclError SearchNvM(aclfvArgs &retrArgs, aclfvSearchInput *searchInput);

    /**
     * execute retr 1vN Search task
     * @param retrArgs: kernel args
     * @param searchInput: search input
     * @return ACL_SUCCESS:success other:failed
     */
    aclError Search1vN(aclfvArgs &retrArgs, aclfvSearchInput *searchInput);

    /**
     * execute retr Search task
     * @param retrArgs: kernel args
     * @param queryNum: query number
     * @param queryIndex: query index
     * @return ACL_SUCCESS:success other:failed
     */
    aclError SearchTask(aclfvArgs &retrArgs, uint32_t queryNum, uint32_t queryIndex);

    /**
     * copy search input and result to device buff
     * @param searchInput: search input
     * @param searchRst: search result
     * @return ACL_SUCCESS:success other:failed
     */
    aclError CopySearchToDevice(aclfvSearchInput *searchInput, aclfvSearchResult *searchRst);

    /**
     * construct the input parameters of the rtCpuKernelLaunch
     * @param searchInput: search input
     * @param searchRst: search result
     * @param ioAddrNum: io address number
     * @param args: kernel args
     * @param argsSize: kernel args size
     * @param stream: stream
     * @return ACL_SUCCESS:success other:failed
     */
    aclError InitSearchTask(aclfvSearchInput *searchInput, aclfvSearchResult *searchRst,
                            int32_t ioAddrNum, char *args, uint32_t argsSize, aclrtStream &stream);

    /**
     * Check parameters
     * @param type: search type
     * @param searchInput: search input
     * @param searchRst: search result
     * @param stream: stream of task execute
     * @return ACL_SUCCESS:success other:failed
     */
    aclError SearchCheck(
        aclfvSearchType type, aclfvSearchInput *searchInput, aclfvSearchResult *searchRst, aclrtStream stream);

private:
    aclrtStream stream_;
    rtNotify_t notify_;  // use to wait for task end
    uint32_t notifyId_;
    aclfvSearchType type_;  // support 1VN and NVM
    aclrtRunMode runMode_;  // support running on host or device
};
}  // namespace retr
}  // namespace acl

#endif  // ACL_SINGLE_OP_RETR_SEARCH_H