/**
* @file retr_search.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "retr_search.h"

#include <securec.h>
#include "aicpu/common/aicpu_task_struct.h"

#include "acl/acl_rt.h"
#include "common/log_inner.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
namespace {
constexpr const char *RETR_KERNELNAME_SEARCH = "RetrSearchKernel";
constexpr uint32_t RETR_BATCH_SEARCH_MAX = 6;
}  // namespace
AclFvSearch::AclFvSearch() : stream_(nullptr), notify_(nullptr), notifyId_(0), type_(SEARCH_1_N), runMode_(ACL_HOST)
{}

AclFvSearch::~AclFvSearch()
{}

/**
 * Check parameters
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::SearchCheck(
    aclfvSearchType type, aclfvSearchInput *searchInput, aclfvSearchResult *searchRst, aclrtStream stream)
{
    ACL_REQUIRES_NOT_NULL(stream);
    ACL_REQUIRES_NOT_NULL(searchInput);
    ACL_REQUIRES_NOT_NULL(searchInput->dataBuffer.data);
    ACL_REQUIRES_NOT_NULL(searchRst);
    ACL_REQUIRES_NOT_NULL(searchRst->dataBuffer.data);
    ACL_REQUIRES_NOT_NULL(searchRst->searchResult.resultNum);
    ACL_REQUIRES_NOT_NULL(searchRst->searchResult.id0);
    ACL_REQUIRES_NOT_NULL(searchRst->searchResult.id1);
    ACL_REQUIRES_NOT_NULL(searchRst->searchResult.resultOffset);
    ACL_REQUIRES_NOT_NULL(searchRst->searchResult.resultDistance);

    ACL_REQUIRES_TRUE(searchInput->searchInput.queryTable.queryCnt == searchRst->searchResult.queryCnt,
        ACL_ERROR_INVALID_PARAM,
        "input qureyCnt must equal to result queryCnt.");

    size_t hostSize = sizeof(searchInput->searchInput);
    size_t devSize = searchInput->dataBuffer.length;
    if (hostSize != devSize) {
        ACL_LOG_INNER_ERROR("[Check][HostSize]input size between host [%u] and device [%u] not equal",
            hostSize, devSize);
        return ACL_ERROR_FAILURE;
    }

    hostSize = sizeof(searchRst->searchResult);
    devSize = searchRst->dataBuffer.length;
    if (hostSize != devSize) {
        ACL_LOG_INNER_ERROR("[Check][HostSize]result size between host [%u] and device [%u] not equal",
            hostSize, devSize);
        return ACL_ERROR_FAILURE;
    }

    if (searchRst->searchResult.dataLen <
        searchRst->searchResult.queryCnt * searchInput->searchInput.topK * sizeof(uint32_t)) {
        ACL_LOG_INNER_ERROR("[Check][Params]dataLen:%u of search result should not be less than queryCnt:%u * "
            "topK:%u * sizeof(uint32_t).",
            searchRst->searchResult.dataLen,
            searchRst->searchResult.queryCnt,
            searchInput->searchInput.topK);
        return ACL_ERROR_INVALID_PARAM;
    }

    type_ = type;
    stream_ = stream;
    return ACL_SUCCESS;
}

/**
 * copy search input and result to device buff
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::CopySearchToDevice(aclfvSearchInput *searchInput, aclfvSearchResult *searchRst)
{
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    aclrtMemcpyKind copyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        copyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }

    ret = aclrtMemcpyAsync(searchInput->dataBuffer.data,
        searchInput->dataBuffer.length,
        &(searchInput->searchInput),
        sizeof(searchInput->searchInput),
        copyKind,
        stream_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy from host to device failed, result = %d", ret);
        return ret;
    }

    ret = aclrtMemcpyAsync(searchRst->dataBuffer.data,
        searchRst->dataBuffer.length,
        &(searchRst->searchResult),
        sizeof(searchRst->searchResult),
        copyKind,
        stream_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy from host to device failed, result = %d", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

/**
 * construct the input parameters of the rtCpuKernelLaunch
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::InitSearchTask(aclfvSearchInput *searchInput, aclfvSearchResult *searchRst,
                                     int32_t ioAddrNum, char *args, uint32_t argsSize, aclrtStream &stream)
{
    aclError ret = CopySearchToDevice(searchInput, searchRst);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
    paramHead->length = argsSize;
    paramHead->ioAddrNum = ioAddrNum;
    auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
    ioAddr[0] = reinterpret_cast<uintptr_t>(searchInput->dataBuffer.data);
    ioAddr[1] = reinterpret_cast<uintptr_t>(searchRst->dataBuffer.data);
    uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);

    if (argsSize <= configOffset) {
        ACL_LOG_INNER_ERROR("[Check][ArgsSize]param argsSize invalid, args size[%u] is smaller than or "
            "equal to configOffset[%u].", argsSize, configOffset);
        return ACL_ERROR_RT_FAILURE;
    }
    auto memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(type_), sizeof(aclfvSearchType));
    if (memcpyRet != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]copy aclfvSearchType to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    configOffset += sizeof(aclfvSearchType);

    if (argsSize <= configOffset) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy_s failed, args size[%u] is smaller than or equal to "
            "configOffset[%u].", argsSize, configOffset);
        return ACL_ERROR_RT_FAILURE;
    }

    ret = acl::retr::aclCreateNotify(notify_, notifyId_, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(notifyId_), sizeof(uint32_t));
    if (memcpyRet != EOK) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        ACL_LOG_INNER_ERROR("[Copy][NotifyId]copy notifyId to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    return ACL_SUCCESS;
}

/**
 * execute retr Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::SearchTask(aclfvArgs &retrArgs, uint32_t queryNum, uint32_t queryIndex)
{
    ACL_LOG_INFO("SearchTask start, queryNum:%u, queryIndex:%u.", queryNum, queryIndex);
    auto memcpyRet = memcpy_s(
        retrArgs.args + retrArgs.configOffset, retrArgs.argsSize - retrArgs.configOffset, &queryNum, sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][QueryNum]copy queryNum to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    memcpyRet = memcpy_s(retrArgs.args + retrArgs.configOffset + sizeof(uint32_t),
        retrArgs.argsSize - retrArgs.configOffset - sizeof(uint32_t),
        &queryIndex,
        sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][QueryIndex]copy queryIndex to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    rtError_t rtRet = rtCpuKernelLaunch(RETR_KERNELS_SONAME,
        RETR_KERNELNAME_SEARCH,
        1,  // blockDim default 1
        retrArgs.args,
        retrArgs.argsSize,
        nullptr,  // no need smDesc
        stream_);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Copy][Retr]retr repo add call rtCpuKernelLaunch failed, runtime result = %d.",
            rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    auto ret = aclFvNotifyWait(notify_, stream_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Wait][Notify]wait for a notify to stream failed, runtime result = %d.", ret);
        return ret;
    }
    ACL_LOG_INFO("SearchTask end.");
    return ACL_SUCCESS;
}

/**
 * execute retr 1vN Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::Search1vN(aclfvArgs &retrArgs, aclfvSearchInput *searchInput)
{
    ACL_LOG_INFO("Search1vN start.");
    uint32_t queryNum = searchInput->searchInput.queryTable.queryCnt;
    uint32_t queryIndex = 0;
    aclError ret = SearchTask(retrArgs, queryNum, queryIndex);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Search][Task]execute aclfvRepoAddTask failed, result = %d.", ret);
        return ret;
    }
    ACL_LOG_INFO("Search1vN end.");
    return ACL_SUCCESS;
}

/**
 * execute retr NvM Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::SearchNvM(aclfvArgs &retrArgs, aclfvSearchInput *searchInput)
{
    ACL_LOG_INFO("SearchNvM start.");
    // N:M search op queryCnt max 1024
    ACL_REQUIRES_TRUE(searchInput->searchInput.queryTable.queryCnt <= 1024,
        ACL_ERROR_INVALID_PARAM,
        "N:M search op queryCnt max 1024.");

    ACL_REQUIRES_TRUE(
        (searchInput->searchInput.queryTable.queryCnt * searchInput->searchInput.queryTable.tableLen) ==
            searchInput->searchInput.queryTable.tableDataLen,
        ACL_ERROR_INVALID_PARAM,
        "queryCnt * tableLen != tableDataLen, please check.");

    uint32_t queryNum = 0;
    uint32_t queryIndex = 0;
    while (queryIndex < searchInput->searchInput.queryTable.queryCnt) {
        uint32_t remainNum = searchInput->searchInput.queryTable.queryCnt - queryIndex;
        queryNum = (remainNum > acl::retr::RETR_BATCH_SEARCH_MAX) ? acl::retr::RETR_BATCH_SEARCH_MAX : remainNum;
        aclError ret = SearchTask(retrArgs, queryNum, queryIndex);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Search][Task]execute aclfvRepoAddTask failed, result = %d.", ret);
            return ret;
        }
        queryIndex += queryNum;
    }
    ACL_LOG_INFO("SearchNvM end.");
    return ACL_SUCCESS;
}

/**
 * launch search task to aicpu
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::LaunchSearchTask(aclfvArgs &retrArgs, aclfvSearchInput *searchInput)
{
    aclError ret = ACL_SUCCESS;

    if (type_ == SEARCH_1_N) {
        ret = Search1vN(retrArgs, searchInput);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][Search1vN]execute Search1vN failed, result = %d.", ret);
            return ret;
        }
    } else if (type_ == SEARCH_N_M) {
        ret = SearchNvM(retrArgs, searchInput);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][SearchNvM]execute SearchNvM failed, result = %d.", ret);
            return ret;
        }
    } else {
        ACL_LOG_INNER_ERROR("[Exec][Type]type must be SEARCH_1_N or SEARCH_N_M, type = %d",
            static_cast<int32_t>(type_));
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

/**
 * get and check result of stream task, 0 represent success
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::GetSearchResult(aclfvSearchResult *searchRst)
{
    ACL_LOG_INFO("execute GetSearchResult begin.");

    void *hostData = &(searchRst->searchResult);
    size_t hostSize = sizeof(searchRst->searchResult);
    void *devData = searchRst->dataBuffer.data;
    size_t devSize = searchRst->dataBuffer.length;

    aclrtMemcpyKind copyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        copyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }

    aclError ret = aclrtMemcpy(hostData, hostSize, devData, devSize, copyKind);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy from device to host failed, result = %d", ret);
        return ret;
    }

    if (searchRst->searchResult.retCode != 0) {
        ACL_LOG_INNER_ERROR("[Exec][Search]execute aclfvSearch failed, result = %d.",
            searchRst->searchResult.retCode);
        return ACL_ERROR_FAILURE;
    }
    ACL_LOG_INFO("execute GetSearchResult success.");

    return ACL_SUCCESS;
}

/**
 * execute retr Search task, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvSearch::Search(
    aclfvSearchType type, aclfvSearchInput *searchInput, aclfvSearchResult *searchRst, aclrtStream stream)
{
    ACL_LOG_INFO("Search start.");
    aclError ret = SearchCheck(type, searchInput, searchRst, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // prepara launch parameters
    constexpr const int32_t ioAddrNum = 2;
    constexpr const uint32_t uintParasNum = 3;
    constexpr const uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
        sizeof(aclfvSearchType) + uintParasNum * sizeof(uint32_t);
    char args[argsSize] = {0};
    ret = InitSearchTask(searchInput, searchRst, ioAddrNum, args, argsSize, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // launch task
    acl::retr::aclfvArgs retrArgs;
    retrArgs.args = args;
    retrArgs.argsSize = argsSize;
    constexpr const uint32_t leftParasNum = 2;
    retrArgs.configOffset = argsSize - leftParasNum * sizeof(uint32_t);
    ret = LaunchSearchTask(retrArgs, searchInput);
    if (ret != ACL_SUCCESS) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ret;
    }
    ACL_LOG_INFO("aclLaunchSearchTask success");

    // wait stream end
    rtError_t rtRet = rtStreamSynchronize(stream);
    if (rtRet != RT_ERROR_NONE) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("rtStreamSynchronize success.");

    // check result
    ret = GetSearchResult(searchRst);
    if (ret != ACL_SUCCESS) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ret;
    }
    ACL_LOG_INFO("aclGetSearchResult success.");

    // destroy notif
    (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
    ACL_LOG_INFO("Search success.");
    return ACL_SUCCESS;
}
}  // namespace retr
}  // namespace acl