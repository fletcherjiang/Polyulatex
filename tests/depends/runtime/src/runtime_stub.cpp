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
#include "runtime/rt_mem_queue.h"
#include "rt_error_codes.h"

#include <stdlib.h>
#include <string.h>
#include "securec.h"
#include "acl_stub.h"

rtError_t aclStub::rtSetDevice(int32_t device)
{
    printf("call stub for rtSetDevice\n");
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetDevice(int32_t *device)
{
    *device = 0;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDeviceReset(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtSetDeviceWithoutTsd(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDeviceResetWithoutTsd(int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDeviceSynchronize(void)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtSetTSDevice(uint32_t tsId)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtStreamCreate(rtStream_t *stream, int32_t priority)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtStreamDestroy(rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtStreamSynchronize(rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtStreamWaitEvent(rtStream_t stream, rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCtxCreateEx(rtContext_t *ctx, uint32_t flags, int32_t device)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCtxDestroyEx(rtContext_t ctx)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCtxSetCurrent(rtContext_t ctx)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCtxSynchronize()
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCtxGetCurrent(rtContext_t *ctx)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetPriCtxByDeviceId(int32_t device, rtContext_t *ctx)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventCreateWithFlag(rtEvent_t *event_, uint32_t flag)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventCreate(rtEvent_t *event)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetEventID(rtEvent_t event, uint32_t *eventId)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventDestroy(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventRecord(rtEvent_t event, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventReset(rtEvent_t event, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventSynchronize(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventQuery(rtEvent_t event)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventQueryWaitStatus(rtEvent_t event, rtEventWaitStatus *status)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtNotifyCreate(int32_t device_id, rtNotify_t *notify_)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtNotifyDestroy(rtNotify_t notify_)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtNotifyRecord(rtNotify_t notify_, rtStream_t stream_)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetNotifyID(rtNotify_t notify_, uint32_t *notify_id)
{
    *notify_id = 0;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtNotifyWait(rtNotify_t notify_, rtStream_t stream_)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMalloc(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMallocCached(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtFlushCache(void *devPtr, size_t size)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtInvalidCache(void *devPtr, size_t size)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtFree(void *devPtr)
{
    free(devPtr);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDvppMalloc(void **devPtr, uint64_t size)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDvppFree(void *devPtr)
{
    free(devPtr);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMallocHost(void **hostPtr,  uint64_t size)
{
    *hostPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtFreeHost(void *hostPtr)
{
    free(hostPtr);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemset(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count)
{
    memset(devPtr, value, count);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemcpy(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind)
{
    memcpy(dst, src, count);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemcpyAsync(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCpuKernelLaunch(const void *soName,
                            const void *kernelName,
                            uint32_t blockDim,
                            const void *args,
                            uint32_t argsSize,
                            rtSmDesc_t *smDesc,
                            rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemGetInfoEx(rtMemInfoType_t memInfoType, size_t *free, size_t *total)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtCallbackLaunch(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtProcessReport(int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtUnSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetRunMode(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_ONLINE;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetDeviceCount(int32_t *count)
{
    *count = 1;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end)
{
    *time = 1.0f;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDevBinaryUnRegister(void *handle)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtFunctionRegister(void *binHandle,
                            const void *stubFunc,
                            const char *stubName,
                            const void *devFunc,
                            uint32_t funcMode)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtKernelLaunch(const void *stubFunc,
                        uint32_t blockDim,
                        void *args,
                        uint32_t argsSize,
                        rtSmDesc_t *smDesc,
                        rtStream_t stream)
{
    return RT_ERROR_NONE;
}


rtError_t aclStub::rtSetTaskFailCallback(rtTaskFailCallback callback)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetSocVersion(char *version, const uint32_t maxLen)
{
    const char *socVersion = "Ascend910";
    memcpy_s(version, maxLen, socVersion, strlen(socVersion) + 1);
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetGroupCount(uint32_t *count)
{
    *count = 2;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetGroupInfo(int32_t groupid, rtGroupInfo_t* groupInfo, uint32_t count)
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

rtError_t aclStub::rtSetGroup(int32_t groupid)
{
    return RT_ERROR_NONE;
}

rtError_t rtProfRegisterCtrlCallback(uint32_t logId, rtProfCtrlHandle callback)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetDevicePhyIdByIndex(uint32_t devIndex, uint32_t *phyId)
{
    *phyId = 10;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEnableP2P(uint32_t devIdDes, uint32_t phyIdSrc, uint32_t flag)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDisableP2P(uint32_t devIdDes, uint32_t phyIdSrc)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDeviceCanAccessPeer(int32_t* canAccessPeer, uint32_t device, uint32_t peerDevice)
{
    *canAccessPeer = 1;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetStreamId(rtStream_t stream_, int32_t *streamId)
{
    *streamId = 1;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtRegDeviceStateCallback(const char *regName, rtDeviceStateCallback callback)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtDeviceGetStreamPriorityRange(int32_t *leastPriority, int32_t *greatestPriority)
{
    *leastPriority = 7;
    *greatestPriority = 0;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
    *value = 0;
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtSetOpWaitTimeOut(uint32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtSetOpExecuteTimeOut(uint32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueDestroy(int32_t devId, uint32_t qid)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueInit(int32_t devId)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *mbuf)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **mbuf)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueEnQueueBuf(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout)

{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueDeQueueBuf(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueQuery(int32_t devId, rtMemQueueQueryCmd_t cmd, void *inBuff, uint32_t inLen,
                                  void *outBuff, uint32_t *outLen)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtQueryDevPid(rtBindHostpidInfo_t *info, pid_t *devPid)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufInit(rtMemBuffCfg_t *cfg)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufFree(rtMbufPtr_t mbuf)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemGrpCreate(const char *name, const rtMemGrpConfig_t *cfg)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemGrpAddProc(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemGrpAttach(const char *name, int32_t timeout)
{
    return RT_ERROR_NONE;
}

rtError_t aclStub::rtMemGrpQuery(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output)
{
    return RT_ERROR_NONE;
}

MockFunctionTest& MockFunctionTest::aclStubInstance()
{
    static MockFunctionTest stub;
    return stub;
};

rtError_t rtSetDevice(int32_t device)
{
    return MockFunctionTest::aclStubInstance().rtSetDevice(device);
}

rtError_t rtDeviceReset(int32_t device)
{
    return MockFunctionTest::aclStubInstance().rtDeviceReset(device);
}

rtError_t rtSetDeviceWithoutTsd(int32_t device)
{
    return MockFunctionTest::aclStubInstance().rtSetDeviceWithoutTsd(device);
}

rtError_t rtDeviceResetWithoutTsd(int32_t device)
{
    return MockFunctionTest::aclStubInstance().rtDeviceResetWithoutTsd(device);
}

rtError_t rtDeviceSynchronize(void)
{
    return MockFunctionTest::aclStubInstance().rtDeviceSynchronize();
}

rtError_t rtGetDevice(int32_t *device)
{
    *device = 0;
    return MockFunctionTest::aclStubInstance().rtGetDevice(device);
}

rtError_t rtSetTSDevice(uint32_t tsId)
{
    return MockFunctionTest::aclStubInstance().rtSetTSDevice(tsId);
}

rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority)
{
    return MockFunctionTest::aclStubInstance().rtStreamCreate(stream, priority);
}

rtError_t rtStreamDestroy(rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtStreamDestroy(stream);
}

rtError_t rtStreamSynchronize(rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtStreamSynchronize(stream);
}

rtError_t rtStreamWaitEvent(rtStream_t stream, rtEvent_t event)
{
    return MockFunctionTest::aclStubInstance().rtStreamWaitEvent(stream, event);
}

rtError_t rtCtxCreateEx(rtContext_t *ctx, uint32_t flags, int32_t device)
{
    return MockFunctionTest::aclStubInstance().rtCtxCreateEx(ctx, flags, device);
}

rtError_t rtCtxDestroyEx(rtContext_t ctx)
{
    return MockFunctionTest::aclStubInstance().rtCtxDestroyEx(ctx);
}

rtError_t rtCtxSetCurrent(rtContext_t ctx)
{
    return MockFunctionTest::aclStubInstance().rtCtxSetCurrent(ctx);
}

rtError_t rtCtxSynchronize()
{
    return MockFunctionTest::aclStubInstance().rtCtxSynchronize();
}

rtError_t rtCtxGetCurrent(rtContext_t *ctx)
{
    return MockFunctionTest::aclStubInstance().rtCtxGetCurrent(ctx);
}

rtError_t rtGetPriCtxByDeviceId(int32_t device, rtContext_t *ctx)
{
    return MockFunctionTest::aclStubInstance().rtGetPriCtxByDeviceId(device, ctx);
}

rtError_t rtEventCreateWithFlag(rtEvent_t *event_, uint32_t flag)
{
    return MockFunctionTest::aclStubInstance().rtEventCreateWithFlag(event_, flag);
}

rtError_t rtEventCreate(rtEvent_t *event)
{
    return MockFunctionTest::aclStubInstance().rtEventCreate(event);
}

rtError_t rtGetEventID(rtEvent_t event, uint32_t *eventId)
{
    return MockFunctionTest::aclStubInstance().rtGetEventID(event, eventId);
}

rtError_t rtEventDestroy(rtEvent_t event)
{
    return MockFunctionTest::aclStubInstance().rtEventDestroy(event);
}

rtError_t rtEventRecord(rtEvent_t event, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtEventRecord(event, stream);
}

rtError_t rtEventReset(rtEvent_t event, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtEventReset(event, stream);
}

rtError_t rtEventSynchronize(rtEvent_t event)
{
    return MockFunctionTest::aclStubInstance().rtEventSynchronize(event);
}

rtError_t rtEventQuery(rtEvent_t event)
{
    return MockFunctionTest::aclStubInstance().rtEventQuery(event);
}

rtError_t rtEventQueryWaitStatus(rtEvent_t event, rtEventWaitStatus *status)
{
    return MockFunctionTest::aclStubInstance().rtEventQueryWaitStatus(event, status);
}

rtError_t rtNotifyCreate(int32_t device_id, rtNotify_t *notify_)
{
    return MockFunctionTest::aclStubInstance().rtNotifyCreate(device_id, notify_);
}

rtError_t rtNotifyDestroy(rtNotify_t notify_)
{
    return MockFunctionTest::aclStubInstance().rtNotifyDestroy(notify_);
}

rtError_t rtNotifyRecord(rtNotify_t notify_, rtStream_t stream_)
{
    return MockFunctionTest::aclStubInstance().rtNotifyRecord(notify_, stream_);
}

rtError_t rtGetNotifyID(rtNotify_t notify_, uint32_t *notify_id)
{
    return MockFunctionTest::aclStubInstance().rtGetNotifyID(notify_, notify_id);
}

rtError_t rtNotifyWait(rtNotify_t notify_, rtStream_t stream_)
{
    return MockFunctionTest::aclStubInstance().rtNotifyWait(notify_, stream_);
}

rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return MockFunctionTest::aclStubInstance().rtMalloc(devPtr, size, type);
}

rtError_t rtMallocCached(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return MockFunctionTest::aclStubInstance().rtMallocCached(devPtr, size, type);
}

rtError_t rtFlushCache(void *devPtr, size_t size)
{
    return MockFunctionTest::aclStubInstance().rtFlushCache(devPtr, size);
}

rtError_t rtInvalidCache(void *devPtr, size_t size)
{
    return MockFunctionTest::aclStubInstance().rtInvalidCache(devPtr, size);
}

rtError_t rtFree(void *devPtr)
{
    return MockFunctionTest::aclStubInstance().rtFree(devPtr);
}

rtError_t rtDvppMalloc(void **devPtr, uint64_t size)
{
    *devPtr = malloc(size);
    return MockFunctionTest::aclStubInstance().rtDvppMalloc(devPtr, size);
}

rtError_t rtDvppFree(void *devPtr)
{
    return MockFunctionTest::aclStubInstance().rtDvppFree(devPtr);
}

rtError_t rtMallocHost(void **hostPtr,  uint64_t size)
{
    *hostPtr = malloc(size);
    return MockFunctionTest::aclStubInstance().rtMallocHost(hostPtr, size);
}

rtError_t rtFreeHost(void *hostPtr)
{
    free(hostPtr);
    return MockFunctionTest::aclStubInstance().rtFreeHost(hostPtr);
}

rtError_t rtMemset(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count)
{
    return MockFunctionTest::aclStubInstance().rtMemset(devPtr, destMax, value, count);
}

rtError_t rtMemcpy(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind)
{
    memcpy(dst, src, count);
    return MockFunctionTest::aclStubInstance().rtMemcpy(dst, destMax, src, count, kind);
}

rtError_t rtMemcpyAsync(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtMemcpyAsync(dst, destMax, src, count, kind, stream);
}

rtError_t rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtMemsetAsync(ptr, destMax, value, count, stream);
}

rtError_t rtCpuKernelLaunch(const void *soName,
                            const void *kernelName,
                            uint32_t blockDim,
                            const void *args,
                            uint32_t argsSize,
                            rtSmDesc_t *smDesc,
                            rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtCpuKernelLaunch(soName, kernelName, blockDim, args, argsSize,
        smDesc, stream);
}

rtError_t rtMemGetInfoEx(rtMemInfoType_t memInfoType, size_t *free, size_t *total)
{
    return MockFunctionTest::aclStubInstance().rtMemGetInfoEx(memInfoType, free, total);
}

rtError_t rtSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtSubscribeReport(threadId, stream);
}

rtError_t rtCallbackLaunch(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock)
{
    return MockFunctionTest::aclStubInstance().rtCallbackLaunch(callBackFunc, fnData, stream, isBlock);
}

rtError_t rtProcessReport(int32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtProcessReport(timeout);
}

rtError_t rtUnSubscribeReport(uint64_t threadId, rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtUnSubscribeReport(threadId, stream);
}

rtError_t rtGetRunMode(rtRunMode *mode)
{
    return MockFunctionTest::aclStubInstance().rtGetRunMode(mode);
}

rtError_t rtGetDeviceCount(int32_t *count)
{
    *count = 1;
    return MockFunctionTest::aclStubInstance().rtGetDeviceCount(count);
}

rtError_t rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end)
{
    *time = 1.0f;
    return MockFunctionTest::aclStubInstance().rtEventElapsedTime(time, start, end);
}

rtError_t rtDevBinaryUnRegister(void *handle)
{
    return MockFunctionTest::aclStubInstance().rtDevBinaryUnRegister(handle);
}

rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle)
{
    return MockFunctionTest::aclStubInstance().rtDevBinaryRegister(bin, handle);
}

rtError_t rtFunctionRegister(void *binHandle,
                            const void *stubFunc,
                            const char *stubName,
                            const void *devFunc,
                            uint32_t funcMode)
{
    return MockFunctionTest::aclStubInstance().rtFunctionRegister(binHandle, stubFunc, stubName, devFunc, funcMode);
}

rtError_t rtKernelLaunch(const void *stubFunc,
                        uint32_t blockDim,
                        void *args,
                        uint32_t argsSize,
                        rtSmDesc_t *smDesc,
                        rtStream_t stream)
{
    return MockFunctionTest::aclStubInstance().rtKernelLaunch(stubFunc, blockDim, args, argsSize, smDesc, stream);
}


rtError_t rtSetTaskFailCallback(rtTaskFailCallback callback)
{
    return MockFunctionTest::aclStubInstance().rtSetTaskFailCallback(callback);
}

rtError_t rtGetSocVersion(char *version, const uint32_t maxLen)
{
    const char *socVersion = "Ascend910";
    memcpy_s(version, maxLen, socVersion, strlen(socVersion) + 1);
    return MockFunctionTest::aclStubInstance().rtGetSocVersion(version, maxLen);
}

rtError_t rtGetGroupCount(uint32_t *count)
{
    *count = 2;
    return MockFunctionTest::aclStubInstance().rtGetGroupCount(count);
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
    return MockFunctionTest::aclStubInstance().rtGetGroupInfo(groupid, groupInfo, count);
}

rtError_t rtSetGroup(int32_t groupid)
{
    return MockFunctionTest::aclStubInstance().rtSetGroup(groupid);
}

rtError_t rtGetDevicePhyIdByIndex(uint32_t devIndex, uint32_t *phyId)
{
    *phyId = 10;
    return MockFunctionTest::aclStubInstance().rtGetDevicePhyIdByIndex(devIndex, phyId);
}

rtError_t rtEnableP2P(uint32_t devIdDes, uint32_t phyIdSrc, uint32_t flag)
{
    return MockFunctionTest::aclStubInstance().rtEnableP2P(devIdDes, phyIdSrc, flag);
}

rtError_t rtDisableP2P(uint32_t devIdDes, uint32_t phyIdSrc)
{
    return MockFunctionTest::aclStubInstance().rtDisableP2P(devIdDes, phyIdSrc);
}

rtError_t rtDeviceCanAccessPeer(int32_t* canAccessPeer, uint32_t device, uint32_t peerDevice)
{
    *canAccessPeer = 1;
    return MockFunctionTest::aclStubInstance().rtDeviceCanAccessPeer(canAccessPeer, device, peerDevice);
}

rtError_t rtGetStreamId(rtStream_t stream_, int32_t *streamId)
{
    *streamId = 1;
    return MockFunctionTest::aclStubInstance().rtGetStreamId(stream_, streamId);
}

rtError_t rtRegDeviceStateCallback(const char *regName, rtDeviceStateCallback callback)
{
    return MockFunctionTest::aclStubInstance().rtRegDeviceStateCallback(regName, callback);
}

rtError_t rtDeviceGetStreamPriorityRange(int32_t *leastPriority, int32_t *greatestPriority)
{
    *leastPriority = 7;
    *greatestPriority = 0;
    return MockFunctionTest::aclStubInstance().rtDeviceGetStreamPriorityRange(leastPriority, greatestPriority);
}

rtError_t rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
    *value = 0;
    return MockFunctionTest::aclStubInstance().rtGetDeviceCapability(device, moduleType, featureType, value);
}

rtError_t rtSetOpWaitTimeOut(uint32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtSetOpWaitTimeOut(timeout);
}

rtError_t rtSetOpExecuteTimeOut(uint32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtSetOpExecuteTimeOut(timeout);
}

rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueCreate(devId, queAttr, qid);
}

rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueDestroy(devId, qid);
}

rtError_t rtMemQueueInit(int32_t devId)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueInit(devId);
}

rtError_t rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *mbuf)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueEnQueue(devId, qid, mbuf);
}

rtError_t rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **mbuf)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueDeQueue(devId, qid, mbuf);
}

rtError_t rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtMemQueuePeek(devId, qid, bufLen, timeout);
}

rtError_t rtMemQueueEnQueueBuf(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout)

{
    return MockFunctionTest::aclStubInstance().rtMemQueueEnQueueBuf(devId, qid, inBuf, timeout);
}

rtError_t rtMemQueueDeQueueBuf(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueDeQueueBuf(devId, qid,outBuf, timeout);
}

rtError_t rtMemQueueQuery(int32_t devId, rtMemQueueQueryCmd_t cmd, void *inBuff, uint32_t inLen,
                                  void *outBuff, uint32_t *outLen)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueQuery( devId, cmd, inBuff, inLen, outBuff, outLen);
}

rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueGrant(devId, qid, pid, attr);
}

rtError_t rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtMemQueueAttach(devId, qid, timeout);
}

rtError_t rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack)
{
    return MockFunctionTest::aclStubInstance().rtEschedSubmitEventSync(devId, event, ack);
}

rtError_t rtQueryDevPid(rtBindHostpidInfo_t *info, pid_t *devPid)
{
    return MockFunctionTest::aclStubInstance().rtQueryDevPid(info, devPid);
}

rtError_t rtMbufInit(rtMemBuffCfg_t *cfg)
{
    return MockFunctionTest::aclStubInstance().rtMbufInit(cfg);
}

rtError_t rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size)
{
    return MockFunctionTest::aclStubInstance().rtMbufAlloc(mbuf, size);
}

rtError_t rtMbufFree(rtMbufPtr_t mbuf)
{
    return MockFunctionTest::aclStubInstance().rtMbufFree(mbuf);
}

rtError_t rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf)
{
    return MockFunctionTest::aclStubInstance().rtMbufGetBuffAddr(mbuf, databuf);
}

rtError_t rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size)
{
    return MockFunctionTest::aclStubInstance().rtMbufGetBuffSize(mbuf, size);
}

rtError_t rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size)
{
    return MockFunctionTest::aclStubInstance().rtMbufGetPrivInfo(mbuf, priv, size);
}

rtError_t rtMemGrpCreate(const char *name, const rtMemGrpConfig_t *cfg)
{
    return MockFunctionTest::aclStubInstance().rtMemGrpCreate(name, cfg);
}

rtError_t rtMemGrpAddProc(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr)
{
    return MockFunctionTest::aclStubInstance().rtMemGrpAddProc(name, pid, attr);
}

rtError_t rtMemGrpAttach(const char *name, int32_t timeout)
{
    return MockFunctionTest::aclStubInstance().rtMemGrpAttach(name, timeout);
}

rtError_t rtMemGrpQuery(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output)
{
    return MockFunctionTest::aclStubInstance().rtMemGrpQuery(cmd, input, output);
}
