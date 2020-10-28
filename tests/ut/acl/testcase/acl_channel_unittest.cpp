#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>

#define protected public
#define private public

#include "acl/ops/acl_dvpp.h"
#include "single_op/op_executor.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "single_op/dvpp/base/image_processor.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"

#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

class ChannelTest : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(ChannelTest, TestDvppChannelCreate_01)
{
    aclrtStream stream;
    EXPECT_NE(acldvppCreateChannel(nullptr), ACL_SUCCESS);

    acldvppChannelDesc desc;
    desc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    desc.dataBuffer.data = (void*)0x1;
    desc.dvppDesc.retCode = ACL_SUCCESS;
    EXPECT_EQ(acldvppCreateChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    desc.dvppWaitTaskType = EVENT_TASK;
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelCreate_02)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.notify = (void*)0x1;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelCreate_03)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    desc.dataBuffer.data = (void*)0x1;
    desc.dvppDesc.retCode = ACL_ERROR_INVALID_PARAM;
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

rtError_t rtGetRunMode_invoke_(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_OFFLINE;
    return RT_ERROR_NONE;
}

TEST_F(ChannelTest, TestDvppChannelCreate_04)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.dvppDesc.retCode = ACL_SUCCESS;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillRepeatedly(Invoke((rtGetRunMode_invoke_)));
    EXPECT_EQ(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

rtError_t rtGetDeviceCapabilityStub(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
    *value = 1;
    return RT_ERROR_NONE;
}

TEST_F(ChannelTest, TestDvppChannelCreate_05)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.dvppDesc.retCode = ACL_SUCCESS;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 3;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCapability(_,_,_,_))
        .WillRepeatedly(Invoke((rtGetDeviceCapabilityStub)));
    EXPECT_EQ(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelCreate_06)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.dvppDesc.retCode = ACL_SUCCESS;
    EXPECT_EQ(acldvppCreateChannel(&desc), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    dvppManager.dvppVersion_ = DVPP_KERNELS_UNKOWN;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
}

TEST_F(ChannelTest, TestDvppChannelDestroy_01)
{
    aclrtStream stream;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    EXPECT_NE(acldvppDestroyChannel(nullptr), ACL_SUCCESS);

    acldvppChannelDesc desc;
    desc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    desc.dataBuffer.data = (void*)0x1;
    EXPECT_EQ(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillRepeatedly(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    desc.dvppWaitTaskType = EVENT_TASK;
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS); // Error
}

TEST_F(ChannelTest, TestDvppChannelDestroy_02)
{
    aclrtStream stream;
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.notify = (void*)0x1;
    EXPECT_EQ(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillRepeatedly(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS); // ERROR
}

TEST_F(ChannelTest, TestDvppChannelDestroy_03)
{
    aclrtStream stream;
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.notify = nullptr;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppCreateChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelDestroy_04)
{
    aclrtStream stream;
    EXPECT_NE(acldvppDestroyChannel(nullptr), ACL_SUCCESS);

    acldvppChannelDesc desc;
    desc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    desc.dataBuffer.data = (void*)0x1;
    EXPECT_EQ(acldvppDestroyChannel(&desc), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((ACL_ERROR_RT_FEATURE_NOT_SUPPORT)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelDestroy_05)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppDestroyChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, TestDvppChannelDestroy_06)
{
    acldvppChannelDesc desc;
    desc.dataBuffer.data = (void*)0x1;
    desc.isNeedNotify = false;
    EXPECT_EQ(acldvppDestroyChannel(&desc), ACL_SUCCESS);
}

TEST_F(ChannelTest, LaunchDvppWaitTask01)
{
    aclrtStream stream;
    acldvppChannelDesc channelDesc;
    channelDesc.dvppWaitTaskType = EVENT_TASK;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    aclError ret = imageProcessor->LaunchDvppWaitTask(&channelDesc, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(ChannelTest, LaunchDvppWaitTask02)
{
    aclrtStream stream;
    acldvppChannelDesc channelDesc;
    channelDesc.dvppWaitTaskType = EVENT_TASK;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    aclError ret = imageProcessor->LaunchDvppWaitTask(&channelDesc, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
}