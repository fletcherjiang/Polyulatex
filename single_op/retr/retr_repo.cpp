/* *
 * @file retr_repo.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "retr_repo.h"

#include "aicpu/common/aicpu_task_struct.h"
#include <securec.h>

#include "acl/acl_rt.h"
#include "common/log_inner.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
namespace {
constexpr const char *RETR_KERNELNAME_REPO_ADD = "RetrRepoAddKernel";
constexpr const char *RETR_KERNELNAME_REPO_DEL = "RetrRepoDelKernel";
constexpr uint32_t FPGA_DRV_CRASH_FEAT_ADD_MAX = 1000000;
} // namespace
AclFvRepo::AclFvRepo() : notify_(nullptr), notifyId_(0), runMode_(ACL_HOST) {}

AclFvRepo::~AclFvRepo() {}

/* *
 * construct the input parameters of the rtCpuKernelLaunch
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::PrepareInput(aclfvSearchType type, void *devData, uint32_t ioAddrNum,
                                 char *args, uint32_t argsSize, aclrtStream stream)
{
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ret = acl::retr::aclCreateNotify(notify_, notifyId_, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
    paramHead->length = argsSize;
    paramHead->ioAddrNum = ioAddrNum;
    auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
    ioAddr[0] = reinterpret_cast<uintptr_t>(devData);
    uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
    auto memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(type), sizeof(aclfvSearchType));
    if (memcpyRet != EOK) {
        ACL_LOG_ERROR("copy aclfvSearchType to args failed, result = %d.", memcpyRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    configOffset += sizeof(aclfvSearchType);
    memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(notifyId_), sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_ERROR("copy notifyId to args failed, result = %d.", memcpyRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }

    return ACL_SUCCESS;
}

/* *
 * execute retr Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::Add1vN(aclfvArgs &retrArgs, aclfvFeatureInfo *featureInfo, aclrtStream stream, rtNotify_t notify)
{
    ACL_LOG_INFO("aclfvRepoAdd1vN start.");
    uint32_t addFeatureNum = featureInfo->retrFeatureInfo.featureCount;
    uint32_t addFeatureCount = 0;
    aclError ret = AddTask(retrArgs, addFeatureNum, addFeatureCount, stream, notify);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("execute aclfvRepoAddTask failed, result = %d.", ret);
        return ret;
    }
    ACL_LOG_INFO("aclfvRepoAdd1vN success.");
    return ACL_SUCCESS;
}

/* *
 * execute retr Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::AddNvM(aclfvArgs &retrArgs, aclfvFeatureInfo *featureInfo, aclrtStream stream, rtNotify_t notify)
{
    ACL_LOG_INFO("aclfvRepoAddNvM start.");
    // N:M repoAdd op aclfvFeatureInfo offset must be 0
    ACL_REQUIRES_TRUE(featureInfo->retrFeatureInfo.offset == 0, ACL_ERROR_INVALID_PARAM,
        "N:M repoAdd op aclfvFeatureInfo offset must be 0.");

    ACL_REQUIRES_TRUE((featureInfo->retrFeatureInfo.featureLen * featureInfo->retrFeatureInfo.featureCount) ==
        featureInfo->retrFeatureInfo.featureDataLen,
        ACL_ERROR_INVALID_PARAM, "featureLen * featureCount != featureDataLen, please check.");

    uint32_t addFeatureNum = 0;
    uint32_t addFeatureCount = 0;
    while (addFeatureCount < featureInfo->retrFeatureInfo.featureCount) {
        uint32_t currentNum = featureInfo->retrFeatureInfo.featureCount - addFeatureCount;
        addFeatureNum =
            (currentNum > acl::retr::FPGA_DRV_CRASH_FEAT_ADD_MAX) ? acl::retr::FPGA_DRV_CRASH_FEAT_ADD_MAX : currentNum;
        aclError ret = AddTask(retrArgs, addFeatureNum, addFeatureCount, stream, notify);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("execute aclfvRepoAddTask failed, result = %d.", ret);
            return ret;
        }
        addFeatureCount += addFeatureNum;
    }
    ACL_LOG_INFO("aclfvRepoAddNvM success.");
    return ACL_SUCCESS;
}

/* *
 * execute retr Search task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::AddTask(aclfvArgs &retrArgs, uint32_t &addFeatureNum, uint32_t &addFeatureCount, aclrtStream stream,
    rtNotify_t notify)
{
    ACL_LOG_INFO("aclfvRepoAddTask start.");
    auto memcpyRet = memcpy_s(retrArgs.args + retrArgs.configOffset, retrArgs.argsSize - retrArgs.configOffset,
        &addFeatureNum, sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_ERROR("copy addFeatureNum to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    memcpyRet = memcpy_s(retrArgs.args + retrArgs.configOffset + sizeof(uint32_t),
        retrArgs.argsSize - retrArgs.configOffset - sizeof(uint32_t), &addFeatureCount, sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_ERROR("copy addFeatureCount to args failed, result = %d.", memcpyRet);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("start to execute aclfvRepoAdd::rtCpuKernelLaunch.");
    rtError_t rtRet = rtCpuKernelLaunch(acl::retr::RETR_KERNELS_SONAME, RETR_KERNELNAME_REPO_ADD,
        1, // blockDim default 1
        retrArgs.args, retrArgs.argsSize,
        nullptr, // no need smDesc
        stream);
    ACL_LOG_INFO("end to execute aclfvRepoAdd::rtCpuKernelLaunch.");
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("retr repo add call rtCpuKernelLaunch failed, runtime result = %d.", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    auto ret = aclFvNotifyWait(notify, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("wait for a notify to stream failed, runtime result = %d.", ret);
        return ret;
    }
    ACL_LOG_INFO("aclfvRepoAddTask success.");
    return ACL_SUCCESS;
}

/* *
 * Check add task parameters
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::AddCheck(aclfvFeatureInfo *featureInfo, aclrtStream stream)
{
    ACL_REQUIRES_NOT_NULL(stream);
    ACL_REQUIRES_NOT_NULL(featureInfo);
    ACL_REQUIRES_NOT_NULL(featureInfo->dataBuffer.data);

    size_t hostSize = sizeof(featureInfo->retrFeatureInfo);
    size_t devSize = featureInfo->dataBuffer.length;
    if (hostSize != devSize) {
        ACL_LOG_ERROR("memory size between host [%u] and device [%u] not equal", hostSize, devSize);
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr repo add task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::AddExecute(aclfvSearchType type, aclfvFeatureInfo *featureInfo, aclrtStream stream, char *args,
    uint32_t argsSize)
{
    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    aclError ret = aclrtMemcpyAsync(featureInfo->dataBuffer.data, featureInfo->dataBuffer.length,
        &(featureInfo->retrFeatureInfo), sizeof(featureInfo->retrFeatureInfo), memcpyKind, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("memcpy between host and device failed, kind = %d, result = %d",
            static_cast<int32_t>(memcpyKind), ret);
        return ret;
    }

    acl::retr::aclfvArgs retrArgs;
    constexpr const uint32_t leftParasNum = 2;
    retrArgs.args = args;
    retrArgs.argsSize = argsSize;
    retrArgs.configOffset = argsSize - leftParasNum * sizeof(uint32_t);

    if (type == SEARCH_1_N) {
        ret = Add1vN(retrArgs, featureInfo, stream, notify_);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("execute aclfvRepoAdd1vN failed, result = %d.", ret);
            return ret;
        }
    } else if (type == SEARCH_N_M) {
        ret = AddNvM(retrArgs, featureInfo, stream, notify_);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("execute aclfvRepoAddNvM failed, result = %d.", ret);
            return ret;
        }
    } else {
        ACL_LOG_ERROR("type must be SEARCH_1_N or SEARCH_N_M, the current type = %d", static_cast<int32_t>(type));
        return ACL_ERROR_INVALID_PARAM;
    }

    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    ret = aclrtMemcpyAsync(&(featureInfo->retrFeatureInfo), sizeof(featureInfo->retrFeatureInfo),
        featureInfo->dataBuffer.data, featureInfo->dataBuffer.length, memcpyKind, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("memcpy between host and device failed, kind = %d, result = %d",
            static_cast<int32_t>(memcpyKind), ret);
        return ret;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr repo add task, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::Add(aclfvSearchType type, aclfvFeatureInfo *featureInfo, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclfvRepoAdd.");
    aclError ret = AddCheck(featureInfo, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // aclfvRepoAdd has 1 input/output aclfvFeatureInfo
    constexpr const uint32_t ioAddrNum = 1;
    // 3 is 1 notifyId, 1 add feature num, 1 already add feature num
    constexpr const uint32_t uintParasNum = 3;
    constexpr const uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
        sizeof(aclfvSearchType) + uintParasNum * sizeof(uint32_t);
    char args[argsSize] = {0};
    ret = PrepareInput(type, featureInfo->dataBuffer.data, ioAddrNum, args, argsSize, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ret = AddExecute(type, featureInfo, stream, args, argsSize);
    if (ret != ACL_SUCCESS) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ret;
    }

    rtError_t rtRetCode = rtStreamSynchronize(stream);
    if (rtRetCode != RT_ERROR_NONE) {
        ACL_LOG_ERROR("synchronize stream failed, runtime result = %d.", rtRetCode);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    if (featureInfo->retrFeatureInfo.retCode != 0) {
        ACL_LOG_ERROR("execute aclfvRepoAdd failed, result = %d.", featureInfo->retrFeatureInfo.retCode);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_FAILURE;
    }

    (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
    ACL_LOG_INFO("execute aclfvRepoAdd success.");
    return ACL_SUCCESS;
}

/* *
 * Check del task parameters
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::DelCheck(aclfvRepoRange *repoRange, aclrtStream stream)
{
    ACL_REQUIRES_NOT_NULL(stream);
    ACL_REQUIRES_NOT_NULL(repoRange);
    ACL_REQUIRES_NOT_NULL(repoRange->dataBuffer.data);

    size_t hostSize = sizeof(repoRange->retrRepoRange);
    size_t devSize = repoRange->dataBuffer.length;
    if (hostSize != devSize) {
        ACL_LOG_ERROR("memory size between host [%u] and device [%u] not equal", hostSize, devSize);
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr repo del task
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::DelExecute(aclfvRepoRange *repoRange, aclrtStream stream, const char *args, uint32_t argsSize)
{
    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    aclError ret = aclrtMemcpyAsync(repoRange->dataBuffer.data,
                                    repoRange->dataBuffer.length,
                                    &(repoRange->retrRepoRange),
                                    sizeof(repoRange->retrRepoRange),
                                    memcpyKind,
                                    stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("memcpy between host and device failed, kind = %d, result = %d",
            static_cast<int32_t>(memcpyKind), ret);
        return ret;
    }

    ACL_LOG_INFO("rtCpuKernelLaunch %s begin.", RETR_KERNELNAME_REPO_DEL);
    rtError_t rtRet = rtCpuKernelLaunch(acl::retr::RETR_KERNELS_SONAME, RETR_KERNELNAME_REPO_DEL,
        1, // blockDim default 1
        args, argsSize,
        nullptr, // no need smDesc
        stream);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("retr aclfvRepoDel call rtCpuKernelLaunch failed, runtime result = %d.", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("rtCpuKernelLaunch %s success.", RETR_KERNELNAME_REPO_DEL);

    ret = aclFvNotifyWait(notify_, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("wait for a notify to stream failed, runtime result = %d.", ret);
        return ret;
    }

    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    ret = aclrtMemcpyAsync(&(repoRange->retrRepoRange), sizeof(repoRange->retrRepoRange), repoRange->dataBuffer.data,
        repoRange->dataBuffer.length, memcpyKind, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("copy retrRepoRange from device to host failed, result = %d.", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr repo del task, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRepo::Del(aclfvSearchType type, aclfvRepoRange *repoRange, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclfvRepoDel.");
    aclError ret = DelCheck(repoRange, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // aclfvRepoDel has 1 input/output aclfvRepoRange
    constexpr const uint32_t ioAddrNum = 1;
    constexpr const uint32_t argsSize =
        sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + sizeof(aclfvSearchType) + sizeof(uint32_t);
    char args[argsSize] = {0};
    ret = PrepareInput(type, repoRange->dataBuffer.data, ioAddrNum, args, argsSize, stream);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ret = DelExecute(repoRange, stream, args, argsSize);
    if (ret != ACL_SUCCESS) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ret;
    }

    rtError_t rtStreamRet = rtStreamSynchronize(stream);
    if (rtStreamRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("synchronize stream failed, runtime result = %d.", rtStreamRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    if (repoRange->retrRepoRange.retCode != 0) {
        ACL_LOG_ERROR("execute aclfvRepoDel failed, result = %d.", repoRange->retrRepoRange.retCode);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_FAILURE;
    }

    (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
    ACL_LOG_INFO("execute aclfvRepoDel success.");
    return ACL_SUCCESS;
}
} // namespace retr
} // namespace acl
