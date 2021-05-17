#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public
#include "single_op/op_executor.h"
#include "executor/stream_executor.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "utils/file_utils.h"
#include "single_op/op_model_parser.h"
#include "single_op/executor/resource_manager.h"
#include "utils/math_utils.h"
#include "common/ge_types.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace ge;

static aclError SelectAclopBatchNorm(int numInputs,
                            const aclTensorDesc *const inputDesc[],
                            int numOutputs,
                            const aclTensorDesc *const outputDesc[],
                            const aclopAttr *opAttr,
                            aclopKernelDesc *aclopKernelDes)
{
    aclopKernelDes->kernelId = "kernel1";
    return ACL_SUCCESS;
}

class OpExecutorTest : public testing::Test {
protected:
    void SetUp()
    {
        int64_t shape[] = {16, 16};
        inputs_[0] = aclCreateDataBuffer((void *) 0x12345, 1024);
        inputs_[1] = aclCreateDataBuffer((void *) 0x12345, 1024);
        outputs_[0] = aclCreateDataBuffer((void *) 0x12345, 1024);
        input_desc_[0] = aclCreateTensorDesc(ACL_FLOAT, 2, shape, ACL_FORMAT_ND);
        input_desc_[1] = aclCreateTensorDesc(ACL_FLOAT, 2, shape, ACL_FORMAT_ND);
        input_desc_[2] = nullptr;
        output_desc_[0] = aclCreateTensorDesc(ACL_FLOAT, 2, shape, ACL_FORMAT_ND);
        output_desc_[1] = nullptr;
    }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
        aclDestroyDataBuffer(inputs_[0]);
        aclDestroyDataBuffer(inputs_[1]);
        aclDestroyDataBuffer(outputs_[0]);
        aclDestroyTensorDesc(input_desc_[0]);
        aclDestroyTensorDesc(input_desc_[1]);
        aclDestroyTensorDesc(output_desc_[0]);
    }

    const aclDataBuffer *inputs_[2];
    aclDataBuffer *outputs_[1];
    const aclTensorDesc *input_desc_[3];
    const aclTensorDesc *output_desc_[2];
};

Status LoadSingleOpMock(const std::string &modelName,
                        const ge::ModelData &modelData,
                        void *stream,
                        SingleOp **single_op,
                        const uint64_t model_id)
{
    *single_op = (SingleOp *)0x12345678;
    return SUCCESS;
}

Status LoadDynamicSingleOpMock(const std::string &modelName,
                                const ge::ModelData &modelData,
                                void *stream,
                                DynamicSingleOp **single_op,
                                const uint64_t model_id)
{
    *single_op = (DynamicSingleOp *)0x12345678;
    return SUCCESS;
}

TEST_F(OpExecutorTest, TestLoadSingleOp)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadSingleOpV2(_, _,_,_,_))
        .WillOnce(Return(RT_FAILED))
        .WillRepeatedly(Invoke(LoadSingleOpMock));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadDynamicSingleOpV2(_, _,_,_,_))
        .WillOnce(Return(RT_FAILED))
        .WillRepeatedly(Invoke(LoadDynamicSingleOpMock));

    OpModel opModel;
    auto aclStream = (aclrtStream) 0x1234;
    SingleOp *nullSingleOp = nullptr;
    DynamicSingleOp *nullDynamicSingleOp = nullptr;
    auto *expectedSingleOp = (SingleOp *) 0x12345678;
    auto *expectedDynamicSingleOp = (DynamicSingleOp *) 0x12345678;
    ASSERT_EQ(OpExecutor::LoadSingleOp(opModel, aclStream), nullSingleOp);
    ASSERT_EQ(OpExecutor::LoadSingleOp(opModel, aclStream), expectedSingleOp);
    ASSERT_EQ(OpExecutor::LoadDynamicSingleOp(opModel, aclStream), nullDynamicSingleOp);
    ASSERT_EQ(OpExecutor::LoadDynamicSingleOp(opModel, aclStream), expectedDynamicSingleOp);
}

TEST_F(OpExecutorTest, DoExecuteAsyncTest)
{
    auto *singleOp = (SingleOp *) 0x12345678;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ExecuteAsync(_,_,_))
        .WillOnce(Return(PARAM_INVALID))
        .WillOnce(Return(SUCCESS))
        .WillOnce(Return(SUCCESS))
        .WillOnce(Return(SUCCESS));

    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    aclTensorDesc *inputDesc[2];
    aclTensorDesc *outputDesc[1];
    int64_t shape[]{16, 16};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;
    bool executeWithExactModel = true;
    ASSERT_NE(OpExecutor::DoExecuteAsync(singleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(singleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclSetTensorPlaceMent(inputDesc[0], aclMemType::ACL_MEMTYPE_HOST);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(singleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclSetTensorPlaceMent(outputDesc[0], aclMemType::ACL_MEMTYPE_HOST);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(singleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
}

TEST_F(OpExecutorTest, CheckConstTensor)
{
    int64_t shape[]{16, -1};
    aclTensorDesc *desc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    desc->isConst = true;
    ASSERT_EQ(desc->CheckConstTensor(false), true);
    desc->isConst = false;
    ASSERT_EQ(desc->CheckConstTensor(true), false);
    desc->memtype = ACL_MEMTYPE_HOST;
    ASSERT_EQ(desc->CheckConstTensor(true), true);
    aclDestroyTensorDesc(desc);
}

TEST_F(OpExecutorTest, DoExecuteAsyncDynamicSuccessTest)
{
    auto *dynamicSingleOp = (DynamicSingleOp *) 0x12345678;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ExecuteAsync(_,_,_,_,_))
        .WillOnce(Return(PARAM_INVALID))
        .WillRepeatedly(Return(SUCCESS));
    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    int64_t shape[]{16, -1};
    std::vector<int64_t> storageDims;
    storageDims.push_back(16);
    storageDims.push_back(-1);
    aclopAttr *opAttr = aclopCreateAttr();
    aclTensorDesc *inputDesc[2];
    aclTensorDesc *outputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    const_cast<aclTensorDesc *>(inputDesc[0])->storageDims = storageDims;
    inputDesc[0]->IsDynamicTensor();
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    const_cast<aclTensorDesc *>(inputDesc[1])->storageDims = storageDims;
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    const_cast<aclTensorDesc *>(outputDesc[0])->storageDims = storageDims;
    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;
    aclOp.opAttr = opAttr;
    ASSERT_NE(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_), ACL_SUCCESS);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_), ACL_SUCCESS);
    
    bool executeWithExactModel = true;   
    aclSetTensorStorageFormat(const_cast<aclTensorDesc *>(aclOp.inputDesc[0]), ACL_FORMAT_ND);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclSetTensorStorageFormat(const_cast<aclTensorDesc *>(aclOp.outputDesc[0]), ACL_FORMAT_ND);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclOp.exeucteType = ACL_OP_EXECUTE_V2;
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclSetTensorPlaceMent(inputDesc[0], aclMemType::ACL_MEMTYPE_HOST);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclSetTensorPlaceMent(outputDesc[0], aclMemType::ACL_MEMTYPE_HOST);
    ASSERT_EQ(OpExecutor::DoExecuteAsync(dynamicSingleOp, aclOp, inputs_, outputs_, executeWithExactModel), ACL_SUCCESS);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);
}

TEST_F(OpExecutorTest, TestCaseExecuteAsync)
{
    aclopUnregisterCompileFunc("BatchNorm");
    aclopRegisterCompileFunc("BatchNorm", SelectAclopBatchNorm);
    ASSERT_EQ(aclopCreateKernel("BatchNorm", "kernel1", "kernel1", (void *)0x1000, 1024, ACL_ENGINE_AICORE, nullptr),
              ACL_SUCCESS);
    ASSERT_EQ(aclopUpdateParams("BatchNorm", 1, input_desc_, 1, output_desc_, nullptr),
	      ACL_SUCCESS);
    int res1 = 0;
    size_t res2 = 0;
    ASSERT_NE(CheckIntAddOverflow(2147483647, 1, res1), ACL_SUCCESS);
    ASSERT_NE(CheckSizeTAddOverflow(0xffffffffffffffff, 1, res2), ACL_SUCCESS);
    ASSERT_NE(CheckSizeTMultiOverflow(0xffffffffffffffff, 2, res2), ACL_SUCCESS);
    aclopUnregisterCompileFunc("BatchNorm");
}

TEST_F(OpExecutorTest, TestResourceManager)
{
    ResourceManager resource_manager((void*)0x1234);
    const char* str = "str";
    ASSERT_EQ(resource_manager.GetMemory((void**)&str, 10),
               ACL_SUCCESS);
}

TEST_F(OpExecutorTest, TestInitTbeTask)
{
    OpKernelDesc desc;
    TbeOpTask task(nullptr, 0);
    desc.workspaceSizes.resize(18);
    StreamExecutor se(nullptr, nullptr);
    se.InitTbeTask(desc, 0, 0, task);
}

TEST_F(OpExecutorTest, TestAllocateWorkspaces)
{
    std::vector<size_t> workspaceSizes;
    vector<uintptr_t> workspaces;
    workspaceSizes.resize(18);
    StreamExecutor se(nullptr, nullptr);
    se.AllocateWorkspaces(workspaceSizes, workspaces);
}