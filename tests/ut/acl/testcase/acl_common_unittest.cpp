#include "acl/acl.h"

#include <string>
#include <cassert>
#include <iostream>
#include <types/tensor_desc_internal.h>

#include "toolchain/slog.h"
#include "toolchain/plog.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "runtime/dev.h"
#include "runtime/stream.h"
#include "runtime/context.h"
#include "runtime/event.h"
#include "runtime/mem.h"
#include "runtime/kernel.h"
#include "runtime/base.h"
#include "runtime/config.h"
#include "error_codes_api.h"
#include "utils/file_utils.h"
#include "acl_stub.h"

#include "framework/executor/ge_executor.h"
#include "common/util/error_manager/error_manager.h"
#include "json_parser.h"

#define protected public
#define private public
#include "log_inner.h"
#include "toolchain/dump.h"
#include "toolchain/profiling.h"
#include "toolchain/profiling_manager.h"
#include "common/common_inner.h"
#include "single_op/op_model_manager.h"
#include "runtime/set_device_vxx.h"
#undef private
#undef protected

using namespace testing;
using namespace std;
using namespace acl;

extern "C" int AdxDataDumpServerInit();
extern "C" int AdxDataDumpServerUnInit();
extern aclError MemcpyKindTranslate(aclrtMemcpyKind kind, rtMemcpyKind_t &rtKind);

class UTEST_ACL_Common : public testing::Test
{
    public:
        UTEST_ACL_Common(){}
    protected:
        virtual void SetUp()
        {

        }
        virtual void TearDown()
        {
            Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
        }
    protected:

};

rtError_t rtGetRunMode_invoke(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_OFFLINE;
    return RT_ERROR_NONE;
}

static void ExceptionInfoCallback(aclrtExceptionInfo *exceptionInfo)
{
    uint32_t task_id = aclrtGetTaskIdFromExceptionInfo(nullptr);
    uint32_t stream_id = aclrtGetStreamIdFromExceptionInfo(nullptr);
    uint32_t thread_id = aclrtGetThreadIdFromExceptionInfo(nullptr);
}

static int32_t ctrl_callback(uint32_t type, void *data, uint32_t len)
{
    return 0;
}

static int32_t reporter_callback(uint32_t moduleId, uint32_t type, void *data, uint32_t len)
{
    return 0;
}

static int32_t reporter_callback1(uint32_t moduleId, uint32_t type, void *data, uint32_t len)
{
    return 1;
}

TEST_F(UTEST_ACL_Common, aclrtGetSocName)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetSocVersion(_, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));
    const char *socName = aclrtGetSocName();
    EXPECT_EQ(socName, nullptr);
}

TEST_F(UTEST_ACL_Common, ErrorManagerTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Init())
        .WillOnce(Return(1));
    auto ret = aclInit(nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, finalize1)
{
    SetGeFinalizeCallback(nullptr);
    // ge executor finalize failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Finalize())
        .WillRepeatedly(Return(PARAM_INVALID));
    aclError ret = aclFinalize();
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, finalize2)
{
    acl::AclDump::GetInstance().aclDumpFlag_ = true;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Finalize())
        .WillRepeatedly(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), AdxDataDumpServerInit())
        .WillRepeatedly(Return(0));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), AdxDataDumpServerUnInit())
        .WillOnce(Return(ACL_ERROR_INTERNAL_ERROR));
    aclError ret = aclFinalize();
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, finalize3)
{
    aclError ret = aclFinalize();
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclFinalize();
    EXPECT_EQ(ret, ACL_ERROR_REPEAT_FINALIZE);
}

TEST_F(UTEST_ACL_Common, getVersion)
{
    int majorVersion = 0;
    int minorVersion = 0;
    int patchVersion = 0;
    aclError ret = aclrtGetVersion(&majorVersion, &minorVersion, &patchVersion);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, aclInitFlag)
{
    bool ret = GetAclInitFlag();
    EXPECT_EQ(ret, true);
}

TEST_F(UTEST_ACL_Common, device)
{
    int32_t deviceId = 0;
    aclError ret = aclrtSetDevice(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetDevice(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    ret = aclrtSetDevice(deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtResetDevice(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceReset(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtResetDevice(deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclInit(nullptr);
    ret = aclrtSetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetDeviceWithoutTsd(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtSetDeviceWithoutTsdVXX(deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtResetDeviceWithoutTsdVXX(deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceResetWithoutTsd(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtResetDeviceWithoutTsdVXX(deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetDevice(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetDevice(&deviceId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetDevice(&deviceId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret =  aclrtSynchronizeDevice();
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceSynchronize())
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret =  aclrtSynchronizeDevice();
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclrtRunMode runMode;
    ret = aclrtGetRunMode(&runMode);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclrtTsId tsId = ACL_TS_ID_RESERVED;
    ret = aclrtSetTsDevice(tsId);
    EXPECT_NE(ret, ACL_SUCCESS);

    tsId = ACL_TS_ID_AICORE;
    ret = aclrtSetTsDevice(tsId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetTSDevice(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSetTsDevice(tsId);
    EXPECT_NE(ret, ACL_SUCCESS);

    uint32_t count = 0;
    ret = aclrtGetDeviceCount(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetDeviceCount(&count);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCount(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetDeviceCount(&count);
    EXPECT_NE(ret, ACL_SUCCESS);

}

TEST_F(UTEST_ACL_Common, context)
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

    // aclrtDestroyContext
    context = (aclrtContext)0x01;
    ret = aclrtDestroyContext(context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxDestroyEx(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyContext(context);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtSetCurrentContext
    context = (aclrtContext)0x01;
    ret = aclrtSetCurrentContext(context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSetCurrentContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxSetCurrent(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSetCurrentContext(context);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtGetCurrentContext
    context = (aclrtContext)0x01;
    ret = aclrtGetCurrentContext(&context);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtGetCurrentContext(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxGetCurrent(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtGetCurrentContext(&context);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, stream)
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
    EXPECT_NE(ret, ACL_SUCCESS);

    // test aclrtDestroyStream
    stream = (aclrtStream)0x01;
    ret = aclrtDestroyStream(stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyStream(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyStream(stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    // test aclrtSynchronizeStream
    stream = (aclrtStream)0x01;
    ret = aclrtSynchronizeStream(stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSynchronizeStream(nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSynchronizeStream(stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    // test aclrtStreamWaitEvent
    stream = (aclrtStream)0x01;
    aclrtEvent event = (aclrtEvent)0x01;
    ret = aclrtStreamWaitEvent(stream, event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtStreamWaitEvent(nullptr, event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtStreamWaitEvent(stream, event);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, event)
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
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    // aclrtDestroyEvent
    event = (aclrtEvent)0x01;
    ret = aclrtDestroyEvent(event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtDestroyEvent(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtDestroyEvent(event);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtRecordEvent(aclrtEvent event, aclrtStream stream)
    event = (aclrtEvent)0x01;
    aclrtStream stream = (aclrtStream)0x01;
    ret = aclrtRecordEvent(event, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtRecordEvent(nullptr, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventRecord(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtRecordEvent(event, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    // aclrtResetEvent
    event = (aclrtEvent)0x01;
    stream = (aclrtStream)0x01;
    ret = aclrtResetEvent(event, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtResetEvent(nullptr, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtResetEvent(event, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtQueryEvent
    event = (aclrtEvent)0x01;
    aclrtEventStatus status;
    ret = aclrtResetEvent(nullptr, &status);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclrtResetEvent(event, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventQuery(_))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return(ACL_ERROR_RT_EVENT_NOT_COMPLETE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    ret = aclrtQueryEvent(event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_STATUS_COMPLETE);

    ret = aclrtQueryEvent(event, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(status, ACL_EVENT_STATUS_NOT_READY);

    ret = aclrtQueryEvent(event, &status);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtSynchronizeEvent
    event = (aclrtEvent)0x01;
    ret = aclrtSynchronizeEvent(event);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtSynchronizeEvent(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtSynchronizeEvent(event);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, eventWithFlag)
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
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, elapsedTime)
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
    EXPECT_NE(ret, ACL_SUCCESS);

}

TEST_F(UTEST_ACL_Common, setOpWaitTimeOut)
{
    uint32_t timeout = 3;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetOpWaitTimeOut(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclrtSetOpWaitTimeout(timeout);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    timeout = 3;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetOpWaitTimeOut(_))
        .WillOnce(Return((RT_ERROR_NONE)));
    ret = aclrtSetOpWaitTimeout(timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
}

void CallBackFunc(void *arg)
{
    int a = 1;
    a++;
}

TEST_F(UTEST_ACL_Common, callback)
{
    aclError ret;
    aclrtStream stream = (aclrtStream)0x01;
    ret = aclrtSubscribeReport(1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSubscribeReport(_,_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtSubscribeReport(1, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtUnSubscribeReport(1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtUnSubscribeReport(_,_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclrtUnSubscribeReport(1, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtLaunchCallback(CallBackFunc, nullptr, ACL_CALLBACK_BLOCK, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclrtLaunchCallback(CallBackFunc, nullptr, static_cast<aclrtCallbackBlockType>(2), stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCallbackLaunch(_, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtLaunchCallback(CallBackFunc, nullptr, ACL_CALLBACK_BLOCK, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtProcessReport(0);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtProcessReport(1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtProcessReport(-1);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_THREAD_SUBSCRIBE)));
    ret = aclrtProcessReport(-1);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtProcessReport(_))
        .WillOnce(Return((ACL_ERROR_RT_REPORT_TIMEOUT)));
    ret = aclrtProcessReport(-1);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, memory_malloc_device)
{
    void *devPtr = nullptr;
    size_t size = 1;

    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(devPtr, nullptr);

    ret = aclrtFree(devPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return((RT_ERROR_NONE)));

    ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_NE(ret, ACL_SUCCESS);

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
}

TEST_F(UTEST_ACL_Common, memory_malloc_cache_device)
{
    void *devPtr = nullptr;
    size_t size = 1;

    aclError ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(devPtr, nullptr);

    // aclrtMemFlush
    ret = aclrtMemFlush(devPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFlushCache(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemFlush(devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(0);
    ret = aclrtMemFlush(devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclrtMemInvalidate
    size = static_cast<size_t>(1);
    ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtInvalidCache(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(0);
    ret = aclrtMemInvalidate(devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtFree(devPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMallocCached(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    size = static_cast<size_t>(1);
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(-1);
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);

    size = static_cast<size_t>(0);
    ret = aclrtMallocCached(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, memory_malloc_host)
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

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMallocHost(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    ret = aclrtMallocHost(nullptr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMallocHost(&hostPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, memory_memcpyAsync)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return((RT_ERROR_NONE)));

    void *dst = (void *)0x01;
    void *src = (void *)0x02;
    aclrtStream stream = (aclrtStream)0x10;
    aclError ret = aclrtMemcpyAsync(dst, 1, src, 1, ACL_MEMCPY_HOST_TO_HOST, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

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
}

TEST_F(UTEST_ACL_Common, memory_getMemInfo)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemGetInfoEx(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    size_t free = 0x01;
    size_t total = 0x02;
    aclError ret = aclrtGetMemInfo(ACL_DDR_MEM_HUGE, &free, &total);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtGetMemInfo(ACL_DDR_MEM_HUGE, &free, &total);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtGetMemInfo(ACL_HBM_MEM, &free, &total);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, memory_memsetAsync)
{

    void *devPtr = (void *)0x01;
    aclrtStream stream = (aclrtStream)0x10;
    aclError ret = aclrtMemsetAsync(nullptr, 1, 1, 1, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclrtMemsetAsync(devPtr, 1, 1, 1, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemsetAsync(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclrtMemsetAsync(devPtr, 1, 1, 1, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_,_))
        .WillOnce(Return((DLOG_ERROR)));
    uint32_t log_level = AclLog::GetCurLogLevel();
    EXPECT_EQ(log_level, ACL_ERROR);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_,_))
        .WillOnce(Return((DLOG_WARN)));
    uint32_t log_level = AclLog::GetCurLogLevel();
    EXPECT_EQ(log_level, ACL_WARNING);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel3)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_,_))
        .WillOnce(Return((DLOG_INFO)));
    uint32_t log_level = AclLog::GetCurLogLevel();
    EXPECT_EQ(log_level, ACL_INFO);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel4)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_,_))
        .WillOnce(Return((DLOG_DEBUG)));
    uint32_t log_level = AclLog::GetCurLogLevel();
    EXPECT_EQ(log_level, ACL_DEBUG);
}

TEST_F(UTEST_ACL_Common, GetCurLogLevel5)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), dlog_getlevel(_,_))
        .WillOnce(Return((DLOG_NULL)));
    uint32_t log_level = AclLog::GetCurLogLevel();
    EXPECT_EQ(log_level, ACL_INFO);
}

TEST_F(UTEST_ACL_Common, ACLSaveLog)
{
    char *strLog = (char *)0x01;
    AclLog::ACLSaveLog(ACL_DEBUG, strLog);
    AclLog::ACLSaveLog(ACL_INFO, strLog);
    AclLog::ACLSaveLog(ACL_WARNING, strLog);
    AclLog::ACLSaveLog(ACL_ERROR, strLog);
}

TEST_F(UTEST_ACL_Common, ACLProfiling)
{
    MsprofReporterCallback callback1 = reporter_callback;
    MsprofReporterCallback callback2 = reporter_callback1;
    AclProfilingManager::GetInstance().SetProfReporterCallback(callback2);
    EXPECT_NE(acl::AclProfilingManager::GetInstance().Init(), ACL_SUCCESS);
    EXPECT_NE(acl::AclProfilingManager::GetInstance().UnInit(), ACL_SUCCESS);
    acl::AclProfilingManager::GetInstance().SetProfReporterCallback(callback1);
}

TEST_F(UTEST_ACL_Common, ACLExceptionCallback)
{
    EXPECT_EQ(aclrtSetExceptionInfoCallback(ExceptionInfoCallback), ACL_SUCCESS);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSetTaskFailCallback(_))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtSetExceptionInfoCallback(ExceptionInfoCallback), ACL_SUCCESS);
    uint32_t task_id = aclrtGetTaskIdFromExceptionInfo(nullptr);
    uint32_t stream_id = aclrtGetStreamIdFromExceptionInfo(nullptr);
    uint32_t thread_id = aclrtGetThreadIdFromExceptionInfo(nullptr);
    uint32_t device_id = aclrtGetDeviceIdFromExceptionInfo(nullptr);
    aclrtExceptionInfo info;
    device_id = aclrtGetDeviceIdFromExceptionInfo(&info);
}

TEST_F(UTEST_ACL_Common, ACLErrorCode)
{
    ACL_REG_ERRCODE(ACL_ERROR_RT_FAILURE, MODULE_RTS, ACL_ERROR_RT_MODEL_EXECUTE,
    "stream exec failed");
    ACL_GET_ERRDESC(MODULE_RTS, ACL_ERROR_RT_MODEL_EXECUTE);
}

TEST_F(UTEST_ACL_Common, ACLSetGroup)
{
    uint32_t count = 0;
    EXPECT_EQ(aclrtGetGroupCount(&count), ACL_SUCCESS);
    aclrtGroupInfo * groupInfo = aclrtCreateGroupInfo();
    EXPECT_EQ(aclrtGetAllGroupInfo(groupInfo), ACL_SUCCESS);

    uint32_t aicoreNum = 0;
    size_t param_ret_size = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AICORE_INT,
        (void *)(&aicoreNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_EQ(aicoreNum, 2);
    EXPECT_EQ(param_ret_size, 4);

    uint32_t aicpuNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AIC_INT,
        (void *)(&aicpuNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_EQ(aicpuNum, 3);

    uint32_t aivectorNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_AIV_INT,
        (void *)(&aivectorNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_EQ(aivectorNum, 4);

    uint32_t sdmaNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_SDMANUM_INT,
        (void *)(&sdmaNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_EQ(sdmaNum, 5);

    uint32_t activeStreamNum = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_ASQNUM_INT,
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_SUCCESS);
    EXPECT_EQ(activeStreamNum, 6);

    uint32_t groupId = 0;
    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, ACL_GROUP_GROUPID_INT,
        (void *)(&groupId), 4, &param_ret_size), ACL_SUCCESS);

    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 2, ACL_GROUP_ASQNUM_INT,
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_ERROR_INVALID_PARAM);

    EXPECT_EQ(aclrtGetGroupInfoDetail(groupInfo, 1, static_cast<aclrtGroupAttr>(6),
        (void *)(&activeStreamNum), 4, &param_ret_size), ACL_ERROR_INVALID_PARAM);

    EXPECT_EQ(aclrtSetGroup(1), ACL_SUCCESS);
    EXPECT_EQ(aclrtDestroyGroupInfo(groupInfo), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, ACLDeviceCanAccessPeer)
{
    int32_t canAccessPeer = 0;
    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 0), ACL_ERROR_INVALID_PARAM);

    EXPECT_EQ(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDeviceCanAccessPeer(_, _, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceCanAccessPeer(&canAccessPeer, 0, 1), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, ACLEnablePeerAccess)
{
    int32_t peerDeviceId = 1;
    uint32_t tmpFlags = 1;
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(peerDeviceId, tmpFlags), ACL_ERROR_FEATURE_UNSUPPORTED);

    uint32_t flags = 0;
    EXPECT_EQ(aclrtDeviceEnablePeerAccess(peerDeviceId, flags), ACL_SUCCESS);

    EXPECT_EQ(aclrtDeviceEnablePeerAccess(0, flags), ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEnableP2P(_, _, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceEnablePeerAccess(peerDeviceId, flags), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceEnablePeerAccess(peerDeviceId, flags), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((1)));
    EXPECT_NE(aclrtDeviceEnablePeerAccess(peerDeviceId, flags), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, ACLDisablePeerAccess)
{
    int32_t peerDeviceId = 1;
    EXPECT_EQ(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_SUCCESS);

    EXPECT_EQ(aclrtDeviceDisablePeerAccess(0), ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDisableP2P(_, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevicePhyIdByIndex(_, _))
        .WillOnce(Return((1)));
    EXPECT_NE(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((1)));
    EXPECT_NE(aclrtDeviceDisablePeerAccess(peerDeviceId), ACL_SUCCESS);
}

bool FileNameFilterFnStub(const std::string &fileName)
{
    return true;
}

TEST_F(UTEST_ACL_Common, AclListFilesTest)
{
    std::string dirName = "llt/acl/ut/json/";
    file_utils::FileNameFilterFn *filter = FileNameFilterFnStub;
    std::vector<std::string> names;
    int maxDepth = 0;
    aclError ret = file_utils::ListFiles(dirName, *filter, names, maxDepth);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    maxDepth = 10;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmScandir2(_,_,_,_))
        .WillOnce(Return((-1)));
    ret = file_utils::ListFiles(dirName, *filter, names, maxDepth);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    ret = file_utils::ListFiles(dirName, *filter, names, maxDepth);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Common, AclGetRecentErrMsgTest)
{
    std::string errMsg = "123";
    EXPECT_EQ(aclGetRecentErrMsg(), nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetErrorMessage())
        .WillOnce(Return((errMsg)));
    EXPECT_NE(aclGetRecentErrMsg(), nullptr);
}

TEST_F(UTEST_ACL_Common, AclrtGetSocNameTest)
{
    std::string aclSocVersion = "lhisi";
    const char *ret = aclrtGetSocName();
    EXPECT_NE(ret, nullptr);
}