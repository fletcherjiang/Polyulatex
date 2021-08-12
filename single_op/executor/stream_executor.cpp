/**
* @file stream_executor.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "stream_executor.h"
#include "utils/math_utils.h"
#include "single_op/compile/op_kernel_selector.h"
#include "op_task.h"
#include "securec.h"
#include "utils/math_utils.h"

namespace acl {
namespace {
const size_t MAX_WORKSPACES = 16U;
}

std::mutex Executors::mu;
std::map<uintptr_t, std::unique_ptr<StreamExecutor>> Executors::executors;

StreamExecutor::StreamExecutor(ResourceManager *resMgr, aclrtStream stream): resMgr_(resMgr), stream_(stream)
{
}

aclError StreamExecutor::ExecuteAsync(const AclOp &aclOp,
                                      const aclDataBuffer *const *inputs,
                                      aclDataBuffer *const *outputs)
{
    std::shared_ptr<OpKernelDesc> desc;
    auto ret = OpKernelSelector::GetInstance().GetOpKernelDesc(aclOp, desc);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][OpKernelDesc]Op with the given shape was not compiled. aclOp = %s",
            aclOp.DebugString().c_str());
        return ret;
    }

    ACL_REQUIRES_NOT_NULL(desc);
    ACL_REQUIRES_OK(ExecuteAsync(*desc, aclOp.numInputs, inputs, aclOp.numOutputs, outputs));
    return ACL_SUCCESS;
}

aclError StreamExecutor::ExecuteAsync(const OpKernelDesc &kernelDesc,
                                      int32_t numInputs,
                                      const aclDataBuffer *const *inputs,
                                      int32_t numOutputs,
                                      aclDataBuffer *const *outputs)
{
    ACL_LOG_DEBUG("Start to execute op by dynamic kernel");
    TbeOpTask task(kernelDesc.stubFunc, kernelDesc.blockDim);
    ACL_REQUIRES_OK(InitTbeTask(kernelDesc, numInputs, numOutputs, task));
    ACL_REQUIRES_OK(task.ExecuteAsync(numInputs, inputs, numOutputs, outputs, stream_));
    return ACL_SUCCESS;
}

aclError StreamExecutor::InitTbeTask(const OpKernelDesc &desc, int32_t numInputs, int32_t numOutputs, TbeOpTask &task)
{
    // create new op task
    size_t numWorkSpaces = desc.workspaceSizes.size();
    if (numWorkSpaces > MAX_WORKSPACES) {
        ACL_LOG_INNER_ERROR("[Check][numWorkSpaces]numWorkSpaces invalid, "
            "numWorkSpace[%zu] is larger than MAX_WORKSPACES[%zu]",
            numWorkSpaces, MAX_WORKSPACES);
        return ACL_ERROR_INVALID_PARAM;
    }

    std::vector<uintptr_t> workspaces;
    ACL_REQUIRES_OK(AllocateWorkspaces(desc.workspaceSizes, workspaces));

    // assemble args
    const std::string &tilingDesc = desc.extendArgs;
    int32_t sum = 0;
    ACL_CHECK_ASSIGN_INT32_ADD(numInputs, numOutputs, sum);
    size_t numArgs = 0U;
    ACL_CHECK_ASSIGN_SIZET_ADD(static_cast<size_t>(sum), numWorkSpaces, numArgs);
    size_t numSize = 0U;
    ACL_CHECK_ASSIGN_SIZET_MULTI(numArgs, sizeof(void *), numSize);
    size_t argSize = 0U;
    size_t descSize = tilingDesc.size();
    ACL_CHECK_ASSIGN_SIZET_ADD(numSize, descSize, argSize);

    auto args = std::unique_ptr<uint8_t[]>(new(std::nothrow)uint8_t[argSize]);
    ACL_CHECK_MALLOC_RESULT(args);
    auto *argBase = args.get();

    // set workspace addresses
    auto *workspaceBase = reinterpret_cast<uintptr_t *>(argBase) + numInputs + numOutputs;
    for (auto wsAddr : workspaces) {
        *(workspaceBase++) = wsAddr;
    }

    // set tiling
    if (!tilingDesc.empty()) {
        void *tilingStart = argBase + (numArgs * sizeof(void *));
        ACL_LOG_DEBUG("tiling desc size = %zu", tilingDesc.size());
        if (memcpy_s(tilingStart, tilingDesc.size(), tilingDesc.data(), tilingDesc.size()) != EOK) {
            ACL_LOG_INNER_ERROR("[Check][Memcpy]Invoking memcpy_s failed");
            return ACL_ERROR_FAILURE;
        }
    }

    task.SetArgs(std::move(args), argSize);
    return ACL_SUCCESS;
}

aclError StreamExecutor::AllocateWorkspaces(const std::vector<size_t> &workspaceSizes, vector<uintptr_t> &workspaces)
{
    auto numWs = workspaceSizes.size();
    if (numWs > MAX_WORKSPACES) {
        ACL_LOG_INNER_ERROR("[Check][numWs]numWs invalid, numWs[%zu] is larger than MAX_WORKSPACES[%zu]",
            numWs, MAX_WORKSPACES);
        return ACL_ERROR_INVALID_PARAM;
    }

    size_t totalSize = 0U;
    vector<uintptr_t> offsets;
    uintptr_t offset = 0U;
    for (auto wsSize : workspaceSizes) {
        offsets.emplace_back(offset);
        size_t alignedSize = 0U;
        ACL_REQUIRES_OK(GetAlignedSize(wsSize, alignedSize));
        totalSize += alignedSize;
        ACL_CHECK_ASSIGN_SIZET_ADD(totalSize, alignedSize, totalSize);
        offset += alignedSize;
        ACL_CHECK_ASSIGN_SIZET_ADD(offset, alignedSize, offset);
    }

    std::lock_guard<std::mutex> lk(mu_);
    void *wsMemory = nullptr;
    ACL_REQUIRES_OK(resMgr_->GetMemory(&wsMemory, totalSize));

    auto wsBase = reinterpret_cast<uintptr_t>(wsMemory);
    for (auto wsOffset : offsets) {
        workspaces.emplace_back(wsBase + wsOffset);
    }

    return ACL_SUCCESS;
}

StreamExecutor::~StreamExecutor()
{
    ACL_LOG_INFO("StreamExecutor::~StreamExecutor IN");
}


StreamExecutor *Executors::GetOrCreate(aclrtContext context, aclrtStream stream)
{
    std::lock_guard<std::mutex> lk(mu);
    uintptr_t key = (stream != nullptr) ? reinterpret_cast<uintptr_t>(stream) : reinterpret_cast<uintptr_t>(context);
    auto it = executors.find(key);
    if (it != executors.end()) {
        return it->second.get();
    }

    auto *resMgr = new(std::nothrow) ResourceManager(context);
    if (resMgr == nullptr) {
        return nullptr;
    }

    auto *executor = new(std::nothrow) StreamExecutor(resMgr, stream);
    if (executor == nullptr) {
        ACL_DELETE_AND_SET_NULL(resMgr);
        return nullptr;
    }

    executors.emplace(key, std::unique_ptr<StreamExecutor>(executor));
    return executor;
}

void Executors::Remove(aclrtContext context, aclrtStream stream)
{
    std::lock_guard<std::mutex> lk(mu);
    auto key = reinterpret_cast<uintptr_t>(stream);
    if (key != 0U) {
        ACL_LOG_INFO("To remove executor by stream = %lu", key);
    } else {
        key = reinterpret_cast<uintptr_t>(context);
        ACL_LOG_INFO("To remove executor by context = %lu", key);
    }
    executors.erase(key);
}
} // namespace acl
