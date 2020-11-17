#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>

#define protected public
#define private public
#include "acl/ops/acl_dvpp.h"
#include "single_op/op_executor.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "single_op/dvpp/common/dvpp_util.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/base/video_processor.h"
#include "single_op/dvpp/v200/video_processor_v200.h"
#include "single_op/dvpp/v200/image_processor_v200.h"
#include "single_op/dvpp/v100/video_processor_v100.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/common/callback_info_manager.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

void VdecCallbackStub(acldvppStreamDesc *input, acldvppPicDesc *output, void *userData)
{

}

class VdecTest : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};


TEST_F(VdecTest, aclvdecCreateChannelDesc_1)
{
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    uint32_t outMode = 2;
    aclError ret = aclvdecSetChannelDescOutMode(vdecChannelDesc, outMode);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

void* mmAlignMallocStub_dvpp(mmSize mallocSize, mmSize alignSize)
{
    aclvdecChannelDesc *aclChannelDesc = nullptr;
    mallocSize = CalAclDvppStructSize(aclChannelDesc);
    return malloc(mallocSize);
}

TEST_F(VdecTest, aclvdecCreateChannel_1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = nullptr;
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    uint32_t outMode = 1;
    ret = aclvdecSetChannelDescOutMode(vdecChannelDesc, outMode);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_3)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCapability(_, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_4)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.aicpuVersion_ = acl::dvpp::AICPU_VERSION_NO_NOTIFY;
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_5)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxGetCurrent(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_6)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_7)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_8)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_9)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_10)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_11)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetStreamId(_, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_12)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->isNeedNotify = true;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->isNeedNotify = true;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);


    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_13)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->isNeedNotify = true;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSubscribeReport(_,_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecCreateChannel_14)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->isNeedNotify = true;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyCreate(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetNotifyID(_, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtSubscribeReport(_,_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

rtError_t rtGetDeviceCapabilityInv(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value)
{
    *value = 1;
    return RT_ERROR_NONE;
}

TEST_F(VdecTest, aclvdecDestroyChannel_0)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = nullptr;
    aclError ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->vdecWaitTaskType = EVENT_TASK;
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecDestroyChannel_1)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = nullptr;
    aclError ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->eosBackFlag.store(true);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->eosBackFlag.store(true);
    vdecChannelDesc->isNeedNotify = true;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtUnSubscribeReport(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->eosBackFlag.store(true);
    vdecChannelDesc->isNeedNotify = false;
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecDestroyChannel_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->isNeedNotify = true;
    vdecChannelDesc->eosBackFlag.store(true);
    vdecChannelDesc->vdecWaitTaskType = EVENT_TASK;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->isNeedNotify = true;
    vdecChannelDesc->eosBackFlag.store(true);
    vdecChannelDesc->vdecWaitTaskType = EVENT_TASK;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->isNeedNotify = false;
    vdecChannelDesc->eosBackFlag.store(true);
    vdecChannelDesc->vdecWaitTaskType = EVENT_TASK;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    ret = aclvdecDestroyChannel(vdecChannelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_v100_1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->callback = VdecCallbackStub;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);
    ret = aclvdecSendSkippedFrame(vdecChannelDesc, inputStreamDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_v100_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_3)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_4)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->callback = VdecCallbackStub;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = false;
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_5)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_6)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_7)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_8)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_9)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_10)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_11)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_12)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCallbackLaunch(_, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V100_13)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    inputStreamDesc->dvppStreamDesc.eos = true;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    vdecChannelDesc->eosBackFlag.store(true);
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_v100_14)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 3;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCapability(_,_,_,_))
        .WillRepeatedly(Invoke((rtGetDeviceCapabilityInv)));
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->isNeedNotify = false;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_v200_1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);

    ret = aclvdecCreateChannel(vdecChannelDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);

    ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);

    aclvdecDestroyChannel(vdecChannelDesc);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_3)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_4)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    vdecChannelDesc->callback = VdecCallbackStub;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_5)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_6)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_7)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_8)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_9)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_10)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_11)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = malloc(4);
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    free(dataDev);
    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_12)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCallbackLaunch(_, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame_V200_13)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = malloc(4);
    vdecChannelDesc->getFrameStream = malloc(4);
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();

    inputStreamDesc->dvppStreamDesc.eos = true;
    vdecChannelDesc->eosBackFlag.store(true);

    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    free(vdecChannelDesc->sendFrameStream);
    free(vdecChannelDesc->getFrameStream);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

rtError_t rtCallbackLaunchStubCallBackError(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock)
{
    callBackFunc(nullptr);
    return RT_ERROR_NONE;
}

rtError_t rtCallbackLaunchStubCallBackOk(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock)
{
    callBackFunc(fnData);
    return RT_ERROR_NONE;
}

TEST_F(VdecTest, aclvdecSendFrame_v100_callback_error)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = (rtStream_t)4;
    vdecChannelDesc->getFrameStream = (rtStream_t)4;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = (void*)4;
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);

    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCallbackLaunch(_, _, _, _))
        .WillRepeatedly(Invoke((rtCallbackLaunchStubCallBackError)));
    aclError ret = aclvdecSendFrame(vdecChannelDesc, inputStreamDesc, outputPicDesc, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, GetFrameCallback_v100)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    uint64_t frameId = 1;
    memcpy(vdecChannelDesc->shareBuffer.data, &frameId, 8);
    vdecChannelDesc->callback = VdecCallbackStub;
    vdecChannelDesc->sendFrameStream = (rtStream_t)4;
    vdecChannelDesc->getFrameStream = (rtStream_t)4;
    acldvppStreamDesc *inputStreamDesc = acldvppCreateStreamDesc();
    void *dataDev = (void*)4;
    acldvppPicDesc *outputPicDesc = acldvppCreatePicDesc();
    acldvppSetPicDescData(outputPicDesc, dataDev);
    acldvppSetPicDescSize(outputPicDesc, 4);
    inputStreamDesc->dvppStreamDesc.eos = false;
    vdecChannelDesc->eosBackFlag.store(true);

    aicpu::dvpp::VdecCallbackInfoPtr callbackInfoPtr = nullptr;
    callbackInfoPtr = std::make_shared<aicpu::dvpp::VdecGetFrameCallbackInfo>(
        inputStreamDesc, outputPicDesc, nullptr, false);
    vdecChannelDesc->taskQueue.push(callbackInfoPtr);
    vdecChannelDesc->callbackMap[1] = callbackInfoPtr;

    VideoProcessor::GetVdecFrameCallback(static_cast<void *>(vdecChannelDesc));
    EXPECT_EQ(vdecChannelDesc->callbackMap.size(), 0);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    acldvppDestroyStreamDesc(inputStreamDesc);
    inputStreamDesc = nullptr;

    acldvppDestroyPicDesc(outputPicDesc);
    outputPicDesc = nullptr;
}

TEST_F(VdecTest, aclvdecSendFrame)
{
    aclvdecChannelDesc *channelDesc = aclvdecCreateChannelDesc();
    acldvppStreamDesc *input;
    acldvppPicDesc *output;
    aclvdecFrameConfig *config;
    void *userData;

    aclError ret = aclvdecSendFrame(channelDesc, input, output, config, userData);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(VdecTest, aclvdecSendSkippedFrame)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    acldvppStreamDesc *input;
    aclvdecFrameConfig *config;
    void *userData;

    aclError ret = aclvdecSendSkippedFrame(channelDesc, input, config, userData);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(VdecTest, acldvppVpcEqualizeHistAsyncTest)
{
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;
    channelDesc.isNeedNotify = false;

    acldvppPicDesc inputDesc;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    acldvppPicDesc outputDesc;
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    acldvppLutMap lutMap;
    uint8_t *tempPtr = new uint8_t[3];
    lutMap.dvppLutMap.map = tempPtr;
    lutMap.dvppLutMap.lens[0] = 1;
    lutMap.dvppLutMap.lens[1] = 1;
    lutMap.dvppLutMap.lens[2] = 1;

    aclrtStream stream;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    imageProcessor->acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc, &outputDesc, &lutMap, stream);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    delete[] tempPtr;
} 

TEST_F(VdecTest, acldvppVpcBatchCropResizeMakeBorderAsyncTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    acldvppChannelDesc channelDesc;
    acldvppResizeConfig resizeConfig;

    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    acldvppRoiConfig  *pasteRoiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    inputBatch->dvppBatchPicDescs.batchSize = 1;
    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;
    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    acldvppBorderConfig *borderConfig = dvppManager.GetImageProcessor()->acldvppCreateBorderConfig();
    EXPECT_NE(acldvppVpcBatchCropResizeMakeBorderAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                    &roiConfig, &borderConfig, &resizeConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    inputBatch->dvppBatchPicDescs.batchSize = 2;
    EXPECT_NE(acldvppVpcBatchCropResizeMakeBorderAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                    &roiConfig, &borderConfig, &resizeConfig, stream), ACL_SUCCESS);
    acldvppDestroyBorderConfig(borderConfig);
    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyRoiConfig(pasteRoiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);
    delete[] roiNums;
}

TEST_F(VdecTest, CreateEventForVdecChannelTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc vencChannelDesc;
    vencChannelDesc.dataBuffer.data = reinterpret_cast<void *>(0x1);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((1)));
    imageProcessor->CreateEventForVdecChannel(&vencChannelDesc);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((0)))
        .WillOnce(Return((1)));
    imageProcessor->CreateEventForVdecChannel(&vencChannelDesc);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((0)))
        .WillOnce(Return((0)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetEventID(_, _))
        .WillOnce(Return((1)));
    imageProcessor->CreateEventForVdecChannel(&vencChannelDesc);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return((0)))
        .WillOnce(Return((0)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetEventID(_, _))
        .WillOnce(Return((0)))
        .WillOnce(Return(1));
    imageProcessor->CreateEventForVdecChannel(&vencChannelDesc);
}

TEST_F(VdecTest, DestroyAllNotifyAndStreamForVdecChannelTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vencChannelDesc = nullptr;
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    bool isNeedNotify;

    imageProcessor->DestroyAllNotifyAndStreamForVdecChannel(vencChannelDesc, isNeedNotify);

    vencChannelDesc = aclvdecCreateChannelDesc();
    vencChannelDesc->sendFrameStream = (rtStream_t)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllNotifyAndStreamForVdecChannel(vencChannelDesc, isNeedNotify);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vencChannelDesc->getFrameStream = (rtStream_t)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllNotifyAndStreamForVdecChannel(vencChannelDesc, isNeedNotify);
    aclvdecDestroyChannelDesc(vencChannelDesc);
}

TEST_F(VdecTest, DestroyAllEventForVdecChannelTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vencChannelDesc = aclvdecCreateChannelDesc();
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    vencChannelDesc->sendFrameNotify = (void *)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllEventForVdecChannel(vencChannelDesc);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    vencChannelDesc->getFrameNotify = (void *)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllEventForVdecChannel(vencChannelDesc);
    aclvdecDestroyChannelDesc(vencChannelDesc);
}

TEST_F(VdecTest, DestroyAllNotifyForVdecChannelTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub_dvpp));
    aclvdecChannelDesc *vencChannelDesc = aclvdecCreateChannelDesc();
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    vencChannelDesc->sendFrameNotify = (void *)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllNotifyForVdecChannel(vencChannelDesc);

    vencChannelDesc->getFrameNotify = (void *)0x11;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyAllNotifyForVdecChannel(vencChannelDesc);
    aclvdecDestroyChannelDesc(vencChannelDesc);
}

TEST_F(VdecTest, LaunchTaskForGetStreamTest)
{
    aclvdecChannelDesc vdecChannelDesc;
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    vdecChannelDesc.isNeedNotify = false;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    imageProcessor->LaunchTaskForGetStream(&vdecChannelDesc);
}

TEST_F(VdecTest, CreateVdecChannelDescOnDeviceTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((1)));
    imageProcessor->CreateVdecChannelDescOnDevice();
}

TEST_F(VdecTest, CreateVdecChannelDescOnHostTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillOnce(Return((VOID *)nullptr));
    imageProcessor->CreateVdecChannelDescOnHost();
}

TEST_F(VdecTest, CreateDvppStreamDescOnDeviceTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((1)));
    imageProcessor->CreateDvppStreamDescOnDevice();
}

TEST_F(VdecTest, CreateDvppStreamDescOnHostTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillOnce(Return((VOID *)nullptr));
    imageProcessor->CreateDvppStreamDescOnHost();
}

TEST_F(VdecTest, v100_aclvdecSetChannelDescChannelIdTest)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    uint32_t channelId;
    acl::dvpp::VideoProcessorV100 v100(ACL_HOST);

    v100.aclvdecSetChannelDescChannelId(channelDesc, channelId);

    channelDesc = aclvdecCreateChannelDesc();
    channelId = 32;
    v100.aclvdecSetChannelDescChannelId(channelDesc, channelId);
    aclvdecDestroyChannelDesc(channelDesc);
}

TEST_F(VdecTest, v100_aclvdecSetChannelDescOutPicFormatTest)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    acldvppPixelFormat outPicFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    acl::dvpp::VideoProcessorV100 v100(ACL_HOST);

    v100.aclvdecSetChannelDescOutPicFormat(channelDesc, outPicFormat);

    channelDesc = aclvdecCreateChannelDesc();
    outPicFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    v100.aclvdecSetChannelDescOutPicFormat(channelDesc, outPicFormat);
    aclvdecDestroyChannelDesc(channelDesc);
}

TEST_F(VdecTest, v100_aclvencGetChannelDescIPPropTest)
{
    aclvencChannelDesc *channelDesc = nullptr;
    bool isSupport;
    acl::dvpp::VideoProcessorV100 v100(ACL_HOST);

    v100.aclvencGetChannelDescIPProp(channelDesc, isSupport);

    channelDesc = aclvencCreateChannelDesc();
    v100.aclvencGetChannelDescIPProp(channelDesc, isSupport);
    aclvencDestroyChannelDesc(channelDesc);
}