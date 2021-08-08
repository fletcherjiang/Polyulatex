#include <string>
#include <cassert>
#include <iostream>

#include "acl/acl.h"
#include "log_inner.h"
#include "acl/acl_rt.h"

#include "runtime/set_device_vxx.h"
#include "runtime/dev.h"
#include "runtime/stream.h"
#include "runtime/context.h"
#include "runtime/event.h"
#include "runtime/mem.h"
#include "runtime/kernel.h"
#include "runtime/base.h"
#include "runtime/config.h"
#include "acl_stub.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace std;
using namespace acl;

class UTEST_ACL_Runtime : public testing::Test
{
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

extern aclError MemcpyKindTranslate(aclrtMemcpyKind kind, rtMemcpyKind_t &rtKind);

rtError_t rtEventQueryWaitStatus_Invoke(rtEvent_t event, rtEventWaitStatus *status)
{
    *status = EVENT_STATUS_COMPLETE;
    return RT_ERROR_NONE;
}

rtError_t rtEventQueryWaitStatus_Invoke2(rtEvent_t event, rtEventWaitStatus *status)
{
    *status = EVENT_STATUS_NOT_READY;
    return RT_ERROR_NONE;
}

TEST_F(UTEST_ACL_Runtime, aclrtSetDeviceFailedTest)
{
    int32_t deviceId = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetDevice(_))
        .WillOnce(Return(ACL_ERROR_RT_INVALID_DEVICEID));
    aclError ret = aclrtSetDevice(deviceId);
    EXPECT_EQ(ret, ACL_ERROR_RT_INVALID_DEVICEID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetDeviceSuccessTest)
{
    int32_t deviceId = 0;
    aclError ret = aclrtSetDevice(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtResetDeviceTest)
{
    int32_t deviceId = 0;
    aclError ret = aclrtResetDevice(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceReset(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtResetDevice(deviceId);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetDeviceWithoutTsdVXXTest)
{
    int32_t deviceId = 0;
    aclError initRet = aclInit(nullptr);
    aclError ret = aclrtSetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetDeviceWithoutTsd(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtSetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtResetDeviceWithoutTsdVXXTest)
{
    int32_t deviceId = 0;
    aclError ret = aclrtResetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceResetWithoutTsd(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtResetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetDeviceTest)
{
    int32_t deviceId = 0;
    aclError ret = aclrtGetDevice(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetDevice(&deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetDevice(&deviceId);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSynchronizeDeviceTest)
{
    aclError ret =  aclrtSynchronizeDevice();
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceSynchronize())
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSynchronizeDevice();
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetRunModeTest)
{
    aclrtRunMode runMode;
    aclError ret = aclrtGetRunMode(&runMode);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetRunMode(&runMode);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetTsDeviceTest)
{
    aclrtTsId tsId = ACL_TS_ID_RESERVED;
    aclError ret = aclrtSetTsDevice(tsId);
    EXPECT_NE(ret, ACL_SUCCESS);

    tsId = ACL_TS_ID_AICORE;
    ret = aclrtSetTsDevice(tsId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetTSDevice(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSetTsDevice(tsId);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetDeviceCountTest)
{
    uint32_t count = 0;
    aclError ret = aclrtGetDeviceCount(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetDeviceCount(&count);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCount(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetDeviceCount(&count);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtCreateContextTest)
{
    aclrtContext context = (aclrtContext)0x01;
    int32_t deviceId = 0;
    // aclrtCreateContext
    aclError ret = aclrtCreateContext(&context, deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtCreateContext(nullptr, deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxCreateEx(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtCreateContext(&context, deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtDestroyContextTest)
{
    // aclrtDestroyContext
    aclrtContext context = (aclrtContext)0x01;
    aclError ret = aclrtDestroyContext(context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxDestroyEx(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyContext(context);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetCurrentContextTest)
{
    // aclrtSetCurrentContext
    aclrtContext context = (aclrtContext)0x01;
    aclError ret = aclrtSetCurrentContext(context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSetCurrentContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxSetCurrent(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSetCurrentContext(context);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetCurrentContextTest)
{
    // aclrtGetCurrentContext
    aclrtContext context = (aclrtContext)0x01;
    aclError ret = aclrtGetCurrentContext(&context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtGetCurrentContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxGetCurrent(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetCurrentContext(&context);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtCreateStreamTest)
{
    // test aclrtCreateStream
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtCreateStream(&stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtCreateStream(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtCreateStream(&stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtDestroyStreamTest)
{
    // test aclrtDestroyStream
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtDestroyStream(stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyStream(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyStream(stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSynchronizeStreamTest)
{
    // test aclrtSynchronizeStream
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtSynchronizeStream(stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSynchronizeStream(nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSynchronizeStream(stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtStreamWaitEventTest)
{
    // test aclrtStreamWaitEvent
    aclrtStream stream = (aclrtStream)0x01;
    aclrtEvent event = (aclrtEvent)0x01;
    aclError ret = aclrtStreamWaitEvent(stream, event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtStreamWaitEvent(nullptr, event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtStreamWaitEvent(stream, event);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtCreateEventTest)
{
    aclrtEvent event = (aclrtEvent)0x01;
    // aclrtCreateEvent
    aclError ret = aclrtCreateEvent(&event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtCreateEvent(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreate(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtCreateEvent(&event);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtDestroyEventTest)
{
    // aclrtDestroyEvent
    aclrtEvent event = (aclrtEvent)0x01;
    aclError ret = aclrtDestroyEvent(event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyEvent(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyEvent(event);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtRecordEventTest)
{
    // aclrtRecordEvent(aclrtEvent event, aclrtStream stream)
    aclrtEvent event = (aclrtEvent)0x01;
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtRecordEvent(event, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtRecordEvent(nullptr, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventRecord(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtRecordEvent(event, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtResetEventTest)
{
    // aclrtResetEvent
    aclrtEvent event = (aclrtEvent)0x01;
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtResetEvent(event, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtResetEvent(nullptr, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtResetEvent(event, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtQueryEventWaitStatusTest)
{
    //aclrtQueryEventWaitStatus
    aclrtEvent event = (aclrtEvent)0x01;
    aclrtEventWaitStatus status;
    aclError ret = aclrtQueryEventWaitStatus(nullptr, &status);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclrtQueryEventWaitStatus(event, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQueryWaitStatus(_,_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret =  aclrtQueryEventWaitStatus(event, &status);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQueryWaitStatus(_,_))
        .WillOnce(Invoke(rtEventQueryWaitStatus_Invoke));
    ret =  aclrtQueryEventWaitStatus(event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_WAIT_STATUS_COMPLETE);

   EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQueryWaitStatus(_,_))
        .WillOnce(Invoke(rtEventQueryWaitStatus_Invoke2));
    ret =  aclrtQueryEventWaitStatus(event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_WAIT_STATUS_NOT_READY);
}

TEST_F(UTEST_ACL_Runtime, aclrtQueryEventTest)
{
    // aclrtQueryEvent
    aclrtEvent event;
    aclrtEventStatus status;
    aclError ret = aclrtResetEvent(nullptr, &status);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtResetEvent(&event, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);

    ret = aclrtQueryEvent(&event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_STATUS_COMPLETE);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQuery(_))
        .WillOnce(Return((ACL_ERROR_RT_EVENT_NOT_COMPLETE)));
    ret =  aclrtQueryEvent(event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_STATUS_NOT_READY);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQuery(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtQueryEvent(event, &status);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSynchronizeEventTest)
{
    // aclrtSynchronizeEvent
    aclrtEvent event = (aclrtEvent)0x01;
    aclError ret = aclrtSynchronizeEvent(event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSynchronizeEvent(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSynchronizeEvent(event);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtCreateEventWithFlagTest)
{
    uint32_t flag = 0x00000008u;
    aclError ret;
    aclrtEvent event = (aclrtEvent)0x01;

    ret = aclrtCreateEventWithFlag(nullptr, flag);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtCreateEventWithFlag(&event, flag);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtCreateEventWithFlag(&event, flag);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtEventElapsedTimeTest)
{
    // aclrtEventElapsedTime
    float time = 0.0f;
    aclError ret;
    aclrtEvent start = (aclrtEvent)0x01;
    aclrtEvent end = (aclrtEvent)0x01;

    ret = aclrtEventElapsedTime(nullptr, start, end);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclrtEventElapsedTime(&time, nullptr, end);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclrtEventElapsedTime(&time, start, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtEventElapsedTime(&time, start, end);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventElapsedTime(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtEventElapsedTime(&time, start, end);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetOpWaitTimeout)
{
    uint32_t timeout = 3;
    aclError ret = aclrtSetOpWaitTimeout(timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetOpWaitTimeOut(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSetOpWaitTimeout(timeout);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

void CallBackFuncStub(void *arg)
{
    int a = 1;
    a++;
}

TEST_F(UTEST_ACL_Runtime, aclrtSubscribeReportTest)
{
    aclError ret;
    aclrtStream stream = (aclrtStream)0x01;
    ret = aclrtSubscribeReport(1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtUnSubscribeReportTest)
{
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtUnSubscribeReport(1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtUnSubscribeReport(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtUnSubscribeReport(1, stream), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtLaunchCallbackTest)
{
    aclrtStream stream = (aclrtStream)0x01;
    aclError ret = aclrtLaunchCallback(CallBackFuncStub, nullptr, ACL_CALLBACK_BLOCK, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtLaunchCallback(CallBackFuncStub, nullptr, static_cast<aclrtCallbackBlockType>(2), stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCallbackLaunch(_, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtLaunchCallback(CallBackFuncStub, nullptr, ACL_CALLBACK_BLOCK, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtProcessReportTest)
{
    // timeout value is invalid
    aclError ret = aclrtProcessReport(0);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtProcessReport success
    ret = aclrtProcessReport(1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    // rtProcessReport failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_THREAD_SUBSCRIBE)));
    EXPECT_EQ(aclrtProcessReport(1), ACL_ERROR_RT_THREAD_SUBSCRIBE);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_REPORT_TIMEOUT)));
    EXPECT_EQ(aclrtProcessReport(1), ACL_ERROR_RT_REPORT_TIMEOUT);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtProcessReport(1), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMalloc_DeviceTest)
{
    void *devPtr = nullptr;
    size_t size = 1;

    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(devPtr, nullptr);

    ret = aclrtFree(devPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(nullptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST_P2P);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY_P2P);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY_P2P);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size = static_cast<size_t>(-1);
    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);
    
    size = static_cast<size_t>(0);
    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMallocCached_DeviceTest)
{
    void *devPtr = nullptr;
    size_t size = 1;

    aclError ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(devPtr, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMallocCached(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemFlushTest)
{
    // aclrtMemFlush
    void *devPtr = nullptr;
    size_t size = 1;
    aclError ret = aclrtMemFlush(devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    size = 0;
    devPtr = (void *)0x11;
    ret = aclrtMemFlush(devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    size = 1;
    devPtr = (void *)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFlushCache(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemFlush(devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemInvalidateTest)
{
    // aclrtMemInvalidate
    void *devPtr = nullptr;
    size_t size = static_cast<size_t>(1);
    aclError ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    size = static_cast<size_t>(0);
    devPtr = (void *)0x11;
    ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(-1);
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(0);
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(1);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtInvalidCache(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_free_device)
{
    aclError ret = aclrtFree(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    void *devPtr = nullptr;
    size_t size = 1;
    aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY_P2P);
    ret = aclrtFree(devPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_ONLY_P2P);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFree(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtFree(devPtr);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_malloc_host)
{
    void *hostPtr = nullptr;
    size_t size = 0;

    aclError ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = 1;
    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(hostPtr, nullptr);

    ret = aclrtFreeHost(hostPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMallocHost(nullptr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMallocHost(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memoryFreeHostTest)
{
    void *hostPtr = nullptr;

    aclError ret = aclrtFreeHost(hostPtr);
    EXPECT_NE(ret, ACL_SUCCESS);
    size_t size = 1;
    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(hostPtr, nullptr);
    ret = aclrtFreeHost(hostPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtFreeHost(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFreeHost(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtFreeHost(hostPtr);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_memset)
{
    aclError ret = aclrtMemset(nullptr, 1, 1, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    void *src = (void *)malloc(5);
    ret = aclrtMemset(src, 1, 1, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemset(_, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemset(src, 1, 1, 1);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_memcpy)
{
    void *dst = (void *)malloc(5);
    void *src = (void *)malloc(5);
    aclError ret = aclrtMemcpy(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_HOST);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(nullptr, 1, src, 1, ACL_MEMCPY_HOST_TO_HOST);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(dst, 1, nullptr, 1, ACL_MEMCPY_HOST_TO_HOST);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_DEVICE);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_HOST);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_DEVICE);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpy(dst, 1, src, 1, (aclrtMemcpyKind)0x7FFFFFFF);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemcpy(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_DEVICE);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_memcpyAsync)
{
    void *dst = (void *)0x01;
    void *src = (void *)0x02;
    aclrtStream stream = (aclrtStream)0x10;
    aclError ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_HOST, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(nullptr, 1, src, 1, ACL_MEMCPY_HOST_TO_HOST, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, nullptr, 1, ACL_MEMCPY_HOST_TO_HOST, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_DEVICE, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_DEVICE, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_HOST, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_DEVICE, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtMemcpyAsync(dst, 1, src, 1, (aclrtMemcpyKind)0x7FFFFFFF, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtMemcpyAsync failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_DEVICE_TO_DEVICE, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, memory_getMemInfo)
{
    size_t free = 0x01;
    size_t total = 0x02;
    aclError ret = aclrtGetMemInfo(ACL_DDR_MEM_HUGE, &free, &total);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtGetMemInfo(ACL_DDR_MEM_HUGE, &free, &total);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtGetMemInfo(ACL_HBM_MEM, &free, &total);
    EXPECT_EQ(ret, ACL_SUCCESS);

    // aclrtGetMemInfo failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemGetInfoEx(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetMemInfo(ACL_DDR_MEM_HUGE, &free, &total);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemsetAsyncFailedTest)
{
    void *devPtr = (void *)0x01;
    aclrtStream stream = (aclrtStream)0x10;
    aclError ret = aclrtMemsetAsync(nullptr, 1, 1, 1, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtMemsetAsync failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemsetAsync(_, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtMemsetAsync(devPtr, 1, 1, 1, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemsetAsyncSuccessTest)
{
    void *devPtr = (void *)0x01;
    aclrtStream stream = (aclrtStream)0x10;
    aclError ret = aclrtMemsetAsync(devPtr, 1, 1, 1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetGroupCountTest)
{
    // aclrtGetGroupCount success
    uint32_t count = 0;
    EXPECT_EQ(aclrtGetGroupCount(&count), ACL_SUCCESS);

    // aclrtGetGroupCount failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetGroupCount(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(aclrtGetGroupCount(&count), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtCreateGroupInfoTest)
{
    // aclrtCreateGroupInfo success
    aclrtGroupInfo *groupInfo = aclrtCreateGroupInfo();
    EXPECT_NE(groupInfo, nullptr);

    // aclrtCreateGroupInfo failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetGroupCount(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(aclrtCreateGroupInfo(), nullptr);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetGroupInfoDetailTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetGroupCount(_))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    aclrtGroupInfo *groupInfo = aclrtCreateGroupInfo();
    uint32_t aicoreNum = 0;
    size_t param_ret_size = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AICORE_INT,
        (void *)(&aicoreNum), 4, &param_ret_size), ACL_ERROR_RT_PARAM_INVALID);
    EXPECT_EQ(aicoreNum, 0);
    EXPECT_EQ(param_ret_size, 0);

    uint32_t aicpuNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AIC_INT,
        (void *)(&aicpuNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_NE(aicpuNum, 0);

    uint32_t aivectorNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AIV_INT,
        (void *)(&aivectorNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_NE(aivectorNum, 0);

    uint32_t sdmaNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_SDMANUM_INT,
        (void *)(&sdmaNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_NE(sdmaNum, 0);

    uint32_t activeStreamNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_ASQNUM_INT,
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_NE(activeStreamNum, 0);

    uint32_t groupId = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_GROUPID_INT,
        (void *)(&groupId), 4, &param_ret_size), ACL_SUCCESS);

    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 2, ACL_GROUP_ASQNUM_INT,
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_ERROR_INVALID_PARAM);

    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, static_cast<aclrtGroupAttr>(6),
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_ERROR_INVALID_PARAM);

    EXPECT_EQ(aclrtDestroyGroupInfo(groupInfo), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtGetAllGroupInfoTest)
{
    aclrtGroupInfo *groupInfo = aclrtCreateGroupInfo();

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetGroupCount(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtGetAllGroupInfo(groupInfo), ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetGroupInfo(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtGetAllGroupInfo(groupInfo), ACL_ERROR_RT_PARAM_INVALID);

    // successfull execute aclrtGetAllGroupInfo
    EXPECT_EQ(aclrtGetAllGroupInfo(groupInfo), ACL_SUCCESS);

}

TEST_F(UTEST_ACL_Runtime, aclrtSetGroupFailedTest)
{
    int32_t groupId = 1;
    EXPECT_EQ(aclrtSetGroup(groupId), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetGroup(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtSetGroup(groupId), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclDeviceCanAccessPeerTest)
{
    int32_t canAccessPeer = 0;
    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 0), ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceCanAccessPeer(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclEnablePeerAccessTest)
{
    int32_t peerDeviceId = 1;
    uint32_t tmpFlags = 1;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(peerDeviceId, 0), ACL_ERROR_RT_PARAM_INVALID);
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(0, 1), ACL_ERROR_FEATURE_UNSUPPORTED);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(peerDeviceId, 0), ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEnableP2P(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(peerDeviceId, 0), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclDisablePeerAccessTest)
{
    int32_t peerDeviceId = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_ERROR_RT_PARAM_INVALID);
    EXPECT_EQ(aclrtDeviceDisablePeerAccess(0), ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDisableP2P(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_ERROR_RT_PARAM_INVALID);
}

typedef void *aclrtNotify;
extern aclError aclrtCreateNotify(int32_t deviceId, aclrtNotify *notify);
extern aclError aclrtDestroyNotify(aclrtNotify notify);
extern aclError aclrtRecordNotify(aclrtNotify notify, aclrtStream stream);
extern aclError aclrtWaitNotify(aclrtNotify notify, aclrtStream stream);
TEST_F(UTEST_ACL_Runtime, aclrtCreateNotifyTest)
{
    int32_t deviceId = 0;
    void *notify = (void *)0x11;
    EXPECT_EQ(aclrtCreateNotify(deviceId, &notify), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtCreateNotify(deviceId, &notify), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtDestroyNotifyTest)
{
    void *notify = (void *)0x11;
    EXPECT_EQ(aclrtDestroyNotify(notify), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtDestroyNotify(notify), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtRecordNotifyTest)
{
    aclrtStream stream = (aclrtStream)0x10;
    void *notify = (void *)0x11;
    EXPECT_EQ(aclrtRecordNotify(notify, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyRecord(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtRecordNotify(notify, stream), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtWaitNotifyTest)
{
    aclrtStream stream = (aclrtStream)0x10;
    void *notify = (void *)0x11;
    EXPECT_EQ(aclrtWaitNotify(notify, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclrtWaitNotify(notify, stream), ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_Runtime, aclrtSubscribeReportFailedTest)
{
    uint64_t threadId = 1;
    aclrtStream stream;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSubscribeReport(_,_))
        .WillOnce(Return((1)));
    aclError ret = aclrtSubscribeReport(threadId, stream);
    EXPECT_EQ(ret, 1);
}

TEST_F(UTEST_ACL_Runtime, aclrtSetExceptionInfoCallbackFailedTest)
{
    aclrtExceptionInfoCallback callback;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetTaskFailCallback(_))
        .WillOnce(Return((1)));
    aclError ret = aclrtSetExceptionInfoCallback(callback);
    EXPECT_EQ(ret, 1);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemcpy2dTest)
{
    int32_t temp = 1;
    void *dst = reinterpret_cast<void *>(&temp);
    void *src = reinterpret_cast<void *>(&temp);
    size_t dpitch = 2;
    size_t spitch = 2;
    size_t width = 3;
    size_t height = 1;
    aclrtMemcpyKind kind = ACL_MEMCPY_HOST_TO_DEVICE;
    aclError ret = aclrtMemcpy2d(dst, dpitch, src, spitch, width, height, kind);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 2;
    height = 0;
    ret = aclrtMemcpy2d(dst, dpitch, src, spitch, width, height, kind);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 1;
    height = 2;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy2d(_, _, _, _, _, _, _))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    ret = aclrtMemcpy2d(dst, dpitch, src, spitch, width, height, kind);
    EXPECT_EQ(ret, 1);

    width = 1;
    height = 2;
    kind = ACL_MEMCPY_HOST_TO_HOST;
    ret = aclrtMemcpy2d(dst, dpitch, src, spitch, width, height, kind);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 1;
    height = 2;
    kind = ACL_MEMCPY_HOST_TO_DEVICE;
    ret = aclrtMemcpy2d(dst, dpitch, src, spitch, width, height, kind);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Runtime, aclrtMemcpy2dAsyncTest)
{
    int32_t temp = 1;
    void *dst = reinterpret_cast<void *>(&temp);
    void *src = reinterpret_cast<void *>(&temp);
    size_t dpitch = 2;
    size_t spitch = 2;
    size_t width = 3;
    size_t height = 1;
    aclrtStream stream;
    aclrtMemcpyKind kind = ACL_MEMCPY_HOST_TO_DEVICE;
    aclError ret = aclrtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, &stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 2;
    height = 0;
    ret = aclrtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, &stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 1;
    height = 2;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy2dAsync(_, _, _, _, _, _, _, _))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    ret = aclrtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, stream);
    EXPECT_EQ(ret, 1);

    width = 1;
    height = 2;
    kind = ACL_MEMCPY_HOST_TO_HOST;
    ret = aclrtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    width = 1;
    height = 2;
    kind = ACL_MEMCPY_HOST_TO_DEVICE;
    ret = aclrtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
}
