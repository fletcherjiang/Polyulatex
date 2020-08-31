/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "runtime/dev.h"
#include "runtime/stream.h"
#include "runtime/context.h"
#include "runtime/event.h"
#include "runtime/mem.h"
#include "runtime/config.h"
#include "runtime/kernel.h"
#include "runtime/base.h"

#include <stdlib.h>
#include <string.h>
#include "securec.h"

rtError_t rtSetDevice(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t rtDeviceReset(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t rtSetDeviceWithoutTsd(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t rtDeviceResetWithoutTsd(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t rtDeviceSynchronize(void)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetDevice(int32_t *device)
{
    *device = 0;
    return RT_ERROR_NONE;
}

rtError_t rtSetTSDevice(uint32_t tsId)
{
    return RT_ERROR_NONE;
}

rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority)
{
    return RT_ERROR_NONE;
}

rtError_t rtStreamDestroy(rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtStreamSynchronize(rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtStreamWaitEvent(rtStream_t stream, rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t rtCtxCreateEx(rtContext_t *ctx, uint32_t flags, int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t rtCtxDestroyEx(rtContext_t ctx)
{
    return RT_ERROR_NONE;
}

rtError_t rtCtxSetCurrent(rtContext_t ctx)
{
    return RT_ERROR_NONE;
}

rtError_t rtCtxSynchronize()
{
    return RT_ERROR_NONE;
}

rtError_t rtCtxGetCurrent(rtContext_t *ctx)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetPriCtxByDeviceId(int32_t device, rtContext_t *ctx)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventCreateWithFlag(rtEvent_t *event_, uint32_t flag)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventCreate(rtEvent_t *event)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetEventID(rtEvent_t event, uint32_t *eventId)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventDestroy(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventRecord(rtEvent_t event, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventReset(rtEvent_t event, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventSynchronize(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t rtEventQuery(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t rtNotifyCreate(int32_t device_id, rtNotify_t *notify_)
{
    return RT_ERROR_NONE;
}

rtError_t rtNotifyDestroy(rtNotify_t notify_)
{
    return RT_ERROR_NONE;
}

rtError_t rtNotifyRecord(rtNotify_t notify_, rtStream_t stream_)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetNotifyID(rtNotify_t notify_, uint32_t *notify_id)
{
    *notify_id = 0;
    return RT_ERROR_NONE;
}

rtError_t rtNotifyWait(rtNotify_t notify_, rtStream_t stream_)
{
    return RT_ERROR_NONE;
}

rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t rtMallocCached(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t rtFlushCache(void *devPtr, size_t size)
{
    return RT_ERROR_NONE;
}

rtError_t rtInvalidCache(void *devPtr, size_t size)
{
    return RT_ERROR_NONE;
}

rtError_t rtFree(void *devPtr)
{
    free(devPtr);
    return RT_ERROR_NONE;
}

rtError_t rtDvppMalloc(void **devPtr, uint64_t size)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t rtDvppFree(void *devPtr)
{
    free(devPtr);
    return RT_ERROR_NONE;
}

rtError_t rtMallocHost(void **hostPtr,  uint64_t size)
{
    *hostPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t rtFreeHost(void *hostPtr)
{
    free(hostPtr);
    return RT_ERROR_NONE;
}

rtError_t rtMemset(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count)
{
    memset(devPtr, value, count);
    return RT_ERROR_NONE;
}

rtError_t rtMemcpy(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind)
{
    memcpy(dst, src, count);
    return RT_ERROR_NONE;
}

rtError_t rtMemcpyAsync(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtCpuKernelLaunch(const void *soName,
                            const void *kernelName,
                            uint32_t blockDim,
                            const void *args,
                            uint32_t argsSize,
                            rtSmDesc_t *smDesc,
                            rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtMemGetInfoEx(rtMemInfoType_t memInfoType, size_t *free, size_t *total)
{
    return RT_ERROR_NONE;
}

rtError_t rtSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtCallbackLaunch (rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock)
{
    return RT_ERROR_NONE;
}

rtError_t rtProcessReport(int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t rtUnSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetRunMode(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_ONLINE;
    return RT_ERROR_NONE;
}

rtError_t rtGetDeviceCount(int32_t *count)
{
    *count = 1;
    return RT_ERROR_NONE;
}

rtError_t rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end)
{
    *time = 1.0f;
    return RT_ERROR_NONE;
}

rtError_t rtDevBinaryUnRegister(void *handle)
{
    return RT_ERROR_NONE;
}

rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle)
{
    return RT_ERROR_NONE;
}

rtError_t rtFunctionRegister(void *binHandle,
                            const void *stubFunc,
                            const char *stubName,
                            const void *devFunc,
                            uint32_t funcMode)
{
    return RT_ERROR_NONE;
}

rtError_t rtKernelLaunch(const void *stubFunc,
                        uint32_t blockDim,
                        void *args,
                        uint32_t argsSize,
                        rtSmDesc_t *smDesc,
                        rtStream_t stream)
{
    return RT_ERROR_NONE;
}


rtError_t rtSetTaskFailCallback(rtTaskFailCallback callback)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetSocVersion(char *version, const uint32_t maxLen)
{
    const char *socVersion = "Ascend910";
    memcpy_s(version, maxLen, socVersion, strlen(socVersion) + 1);
    return RT_ERROR_NONE;
}

rtError_t rtGetGroupCount(uint32_t *count)
{
    *count = 2;
    return RT_ERROR_NONE;
}

rtError_t rtGetGroupInfo(int32_t groupid, rtGroupInfo_t* groupInfo, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        groupInfo[i].groupId = (int32_t)i;
        groupInfo[i].flag = (int32_t)i;
        groupInfo[i].aicoreNum = i + 1;
        groupInfo[i].aicpuNum = i + 2;
        groupInfo[i].aivectorNum = i + 3;
        groupInfo[i].sdmaNum = i + 4;
        groupInfo[i].activeStreamNum = i + 5;
    }
    return RT_ERROR_NONE;
}

rtError_t rtSetGroup(int32_t groupid)
{
    return RT_ERROR_NONE;
}

rtError_t rtGetDevicePhyIdByIndex(uint32_t devIndex, uint32_t *phyId)
{
    *phyId = 10;
    return RT_ERROR_NONE;
}

rtError_t rtEnableP2P(uint32_t devIdDes, uint32_t phyIdSrc, uint32_t flag)
{
    return RT_ERROR_NONE;
}

rtError_t rtDisableP2P(uint32_t devIdDes, uint32_t phyIdSrc)
{
    return RT_ERROR_NONE;
}

rtError_t rtDeviceCanAccessPeer(int32_t* canAccessPeer, uint32_t device, uint32_t peerDevice)
{
    *canAccessPeer = 1;
    return RT_ERROR_NONE;
}

rtError_t rtGetStreamId(rtStream_t stream_, int32_t *streamId)
{
    *streamId = 1;
    return RT_ERROR_NONE;
}

rtError_t rtRegDeviceStateCallback(const char *regName, rtDeviceStateCallback callback)
{
    return RT_ERROR_NONE;
}

rtError_t rtDeviceGetStreamPriorityRange(int32_t *leastPriority, int32_t *greatestPriority)
{
    *leastPriority = 7;
    *greatestPriority = 0;
    return RT_ERROR_NONE;
}

rtError_t rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
    *value = 0;
    return RT_ERROR_NONE;
}

rtError_t rtSetOpWaitTimeOut(uint32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t rtSetOpExecuteTimeOut(uint32_t timeout)
{
    return RT_ERROR_NONE;
}
