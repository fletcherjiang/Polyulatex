#include "acl/acl.h"
#include "acl/acl_op_compiler.h"
#include "runtime/dev.h"

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public

#include "single_op/compile/local_compiler.h"
#include "single_op/compile/op_compile_processor.h"
#include "single_op/compile/op_compile_service.h"
#include "single_op/op_model_manager.h"
#include "single_op/compile/op_compile_processor.h"
#include "utils/array_utils.h"
#include "acl_stub.h"

#undef private
#undef protected

#include "framework/generator/ge_generator.h"
#include "ge/ge_api.h"
#include "framework/executor/ge_executor.h"
#include "common/common_inner.h"

using namespace testing;
using namespace std;
using namespace acl;
using namespace ge;

using OpDescPtr = std::shared_ptr<OpDesc>;


class UTEST_ACL_OpCompiler : public testing::Test {
    protected:
        UTEST_ACL_OpCompiler() {}
    public:
        virtual void SetUp() {}
        virtual void TearDown() {
            Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
        }
};

TEST_F(UTEST_ACL_OpCompiler, InitLocalCompilerTest)
{
    std::map<string, string> options;
    LocalCompiler compiler;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GEInitialize(_))
        .WillOnce(Return((ge::PARAM_INVALID)))
        .WillRepeatedly(Return(ge::SUCCESS));
    EXPECT_EQ(compiler.Init(options), ACL_ERROR_GE_FAILURE);
    EXPECT_EQ(compiler.Init(options), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Finalize())
        .WillRepeatedly(Return((ge::PARAM_INVALID)));
    ASSERT_NE(aclFinalize(), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GEFinalize())
        .WillRepeatedly(Return((ge::PARAM_INVALID)));
    ASSERT_NE(aclFinalize(), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, OnlineCompileTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxGetCurrent(_))
        .WillRepeatedly(Return(1));
    LocalCompiler compiler;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), BuildSingleOpModel(_,_,_,_,_,_))
        .WillOnce(Return(ge::PARAM_INVALID))
        .WillRepeatedly(Return(ge::SUCCESS));
    CompileParam param;
    std::shared_ptr<void> modelData = nullptr;
    size_t modelSize = 0;
    EXPECT_NE(compiler.OnlineCompile(param, modelData, modelSize), ACL_SUCCESS);
    EXPECT_EQ(compiler.OnlineCompile(param, modelData, modelSize), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, TestMakeCompileParamV3)
{
    CompileParam param;
    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    int64_t shape[]{16, -1};
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclMemType memType = ACL_MEMTYPE_HOST;
    aclSetTensorPlaceMent(const_cast<aclTensorDesc *>(inputDesc[0]), memType);
    aclSetTensorPlaceMent(const_cast<aclTensorDesc *>(outputDesc[0]), memType);
    inputDesc[0]->IsHostMemTensor();
    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;
    aclDataBuffer *inputs[2];
    inputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);
    inputs[1] = aclCreateDataBuffer((void *) 0x1000, 1024);
    aclDataBuffer *outputs[1];
    outputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);
    aclOp.inputs = inputs;
    aclOp.outputs = nullptr;
    aclOpCompileFlag compileFlag = ACL_OP_COMPILE_DEFAULT;
    ASSERT_NE(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);
    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclDestroyDataBuffer(inputs[0]);
    aclDestroyDataBuffer(inputs[1]);
    aclDestroyDataBuffer(outputs[0]);
}

TEST_F(UTEST_ACL_OpCompiler, MakeCompileParamTest)
{
    CompileParam param;
    AclOp aclOp;
    aclOp.opType = "TransData";
    std::vector<int64_t> dims2 = {-2};
    aclTensorDesc desc2(ACL_FLOAT16, dims2.size(), dims2.data(), ACL_FORMAT_ND);
    std::vector<int64_t> dims3 = {-1, 16};
    aclTensorDesc desc3(ACL_FLOAT16, dims3.size(), dims3.data(), ACL_FORMAT_ND);

    aclTensorDesc desc(ACL_FLOAT16, 0, nullptr, ACL_FORMAT_ND);
    aclSetTensorDescName(&desc, "xxx");
    aclTensorDesc* descArr[] = {&desc};
    aclOp.inputDesc = descArr;
    aclOp.outputDesc = descArr;
    aclOp.numInputs = 1;
    aclOp.numOutputs = 1;
    aclOpCompileFlag compileFlag = ACL_OP_COMPILE_DEFAULT;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    aclTensorDesc* descArr1[] = {&desc2};
    aclOp.inputDesc = descArr1;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    aclTensorDesc* descArr2[] = {&desc3};
    aclOp.inputDesc = descArr2;
    EXPECT_NE(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    desc.storageFormat = ACL_FORMAT_NCHW;
    aclOp.inputDesc = descArr;
    aclOp.outputDesc = descArr;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    aclOp.compileType = OP_COMPILE_UNREGISTERED;
    aclopAttr attr;
    string str = "";
    attr.SetAttr("unregst _attrlist", str);
    aclOp.opAttr = &attr;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetShapeRange(_))
            .WillOnce(Return(GRAPH_SUCCESS))
            .WillOnce(Return(GRAPH_SUCCESS))
            .WillOnce(Return(GRAPH_PARAM_INVALID))
            .WillOnce(Return(GRAPH_SUCCESS))
            .WillOnce(Return(GRAPH_PARAM_INVALID))
            .WillRepeatedly(Return(GRAPH_SUCCESS));
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_ERROR_GE_FAILURE);
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_ERROR_GE_FAILURE);
}

TEST_F(UTEST_ACL_OpCompiler, MakeCompileParamOptionalInputTest)
{
    GeTensorDescPtr desc = nullptr;
    CompileParam param;
    AclOp aclOp;
    aclOp.opType = "testOp";
    aclTensorDesc desc1(ACL_FLOAT16, 0, nullptr, ACL_FORMAT_ND);
    aclTensorDesc desc2(ACL_DT_UNDEFINED, 0, nullptr, ACL_FORMAT_UNDEFINED);
    aclTensorDesc desc3(ACL_FLOAT16, 0, nullptr, ACL_FORMAT_ND);
    aclSetTensorDynamicInput(&desc1, "x");
    aclSetTensorDynamicInput(&desc3, "y");
    aclTensorDesc* descArr[] = {&desc1, &desc2, &desc3};
    aclOp.inputDesc = descArr;
    aclOp.outputDesc = descArr;
    aclOp.numInputs = 3;
    aclOp.numOutputs = 1;
    aclOpCompileFlag compileFlag = ACL_OP_COMPILE_DEFAULT;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    desc1.storageFormat = ACL_FORMAT_NCHW;
    aclOp.inputDesc = descArr;
    aclOp.outputDesc = descArr;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    aclOp.compileType = OP_COMPILE_UNREGISTERED;
    aclopAttr attr;
    string str = "";
    attr.SetAttr("unregst _attrlist", str);
    aclOp.opAttr = &attr;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, MakeCompileParamTransTest)
{
    CompileParam param;
    AclOp aclOp;
    aclOp.opType = "TransData";
    std::vector<int64_t> dims2 = {1,2,3,4};
    aclTensorDesc desc2(ACL_FLOAT16, dims2.size(), dims2.data(), ACL_FORMAT_NCHW);
    std::vector<int64_t> dims3 = {1,2,3,16};
    aclTensorDesc desc3(ACL_FLOAT16, dims3.size(), dims3.data(), ACL_FORMAT_NCHW);

    aclTensorDesc* descArrIn[] = {&desc2};
    aclTensorDesc* descArrOut[] = {&desc3};
    aclOp.inputDesc = descArrIn;
    aclOp.outputDesc = descArrOut;
    aclOp.numInputs = 1;
    aclOp.numOutputs = 1;
    aclOpCompileFlag compileFlag = ACL_OP_COMPILE_DEFAULT;
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);

    aclSetTensorOriginFormat(&desc2, ACL_FORMAT_NC1HWC0);
    EXPECT_EQ(OpCompiler::MakeCompileParam(aclOp, param, compileFlag), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, MakeCompileParamHostMemTest)
{
    CompileParam param;
    AclOp aclOp;
    aclOp.opType = "TransData";
    std::vector<int64_t> dims2 = {1,2,3,4};
    aclTensorDesc desc2(ACL_FLOAT16, dims2.size(), dims2.data(), ACL_FORMAT_NCHW);
    desc2.memtype = ACL_MEMTYPE_HOST;

    std::vector<int64_t> dims3 = {1,2,3,16};
    aclTensorDesc desc3(ACL_FLOAT16, dims3.size(), dims3.data(), ACL_FORMAT_NCHW);

    aclTensorDesc* descArrIn[] = {&desc2};
    aclTensorDesc* descArrOut[] = {&desc3};

    aclDataBuffer *inputs[1];
    inputs[0] = aclCreateDataBuffer((void *) 0x0001, 1);
    aclOp.inputs = inputs;
    aclOp.inputDesc = descArrIn;
    aclOp.outputDesc = descArrOut;
    aclOp.numInputs = 1;
    aclOp.numOutputs = 1;
    aclOpCompileFlag compileFlag = ACL_OP_COMPILE_DEFAULT;
    aclError ret1 = OpCompiler::MakeCompileParam(aclOp, param, compileFlag);
    compileFlag = ACL_OP_COMPILE_FUZZ;
    aclError ret2 = OpCompiler::MakeCompileParam(aclOp, param, compileFlag);
    aclDestroyDataBuffer(inputs[0]);
    EXPECT_EQ(ret1, ACL_SUCCESS);
    EXPECT_EQ(ret2, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, SetCompileStrategyTest)
{
    std::map<std::string, std::string> options;
    OpCompileService service;
    service.creators_.clear();
    EXPECT_EQ(service.SetCompileStrategy(NATIVE_COMPILER, options), ACL_ERROR_COMPILER_NOT_REGISTERED);
}

TEST_F(UTEST_ACL_OpCompiler, aclSetCompileoptTest)
{
    aclCompileOpt opt1 = ACL_OP_COMPILER_CACHE_DIR;
    char value1[10] = "111";
    EXPECT_EQ(aclSetCompileopt(opt1, value1), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, ReadOpModelFromFileTest)
{
    const std::string path;
    OpModel opModel;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ReadBytesFromBinaryFile(_, _, _))
        .WillOnce(Return((false)));
    EXPECT_NE(ReadOpModelFromFile(path, opModel), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ReadBytesFromBinaryFile(_, _, _))
        .WillOnce(Return((true)));
    EXPECT_EQ(ReadOpModelFromFile(path, opModel), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, aclopSetCompileFlagTest)
{
    EXPECT_EQ(aclopSetCompileFlag(ACL_OP_COMPILE_DEFAULT), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, TestOnlineCompile02)
{
    LocalCompiler compiler;
    CompileParam param;
    std::shared_ptr<void> modelData = nullptr;
    size_t modelSize = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCtxGetCurrent(_))
        .WillRepeatedly(Return(ACL_ERROR_INVALID_PARAM));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Initialize(_, _))
        .WillOnce(Return(ge::PARAM_INVALID));
    EXPECT_NE(compiler.OnlineCompile(param, modelData, modelSize), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Initialize(_, _))
        .WillOnce(Return(ge::SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), BuildSingleOpModel(_,_,_,_,_,_))
        .WillOnce(Return(ge::SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Ge_Generator_Finalize())
        .WillOnce(Return(ge::PARAM_INVALID))
        .WillOnce(Return(ge::SUCCESS));
    EXPECT_NE(compiler.OnlineCompile(param, modelData, modelSize), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, TestInitLocalCompiler02)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GEInitialize(_))
        .WillOnce(Return(ge::SUCCESS));

    std::map<string, string> options;
    LocalCompiler compiler;
    ASSERT_EQ(compiler.Init(options), ACL_SUCCESS);
    ASSERT_NE(compiler.Init(options), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpCompiler, AclIsDigitTest)
{
    std::string str = "";
    EXPECT_EQ(string_utils::IsDigit(str), false);
}

TEST_F(UTEST_ACL_OpCompiler, aclopCompileAndExecuteTest)
{
    char *opType;
    int numInputs = 2;

    int64_t shape[]{16, -1};
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);

    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);

    aclDataBuffer *inputs[2];
    inputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);
    inputs[1] = aclCreateDataBuffer((void *) 0x1000, 1024);

    aclDataBuffer *outputs[1];
    outputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);

    int numOutputs = 1;

    aclopAttr attr;
    string str = "";
    attr.SetAttr("unregst _attrlist", str);
    aclopEngineType engineType;
    aclopCompileType compileFlag = (aclCompileType)(3);
    const char *opPath;
    aclrtStream stream;

    aclError ret = aclopCompileAndExecute(opType, numInputs, inputDesc, inputs,
                numOutputs, outputDesc, outputs, &attr, engineType, compileFlag,
                opPath, stream);
    EXPECT_EQ(ret, ACL_ERROR_API_NOT_SUPPORT);

    compileFlag = ACL_COMPILE_UNREGISTERED;
    opType = "Add";
    ret = aclopCompileAndExecute(opType, numInputs, inputDesc, inputs,
                numOutputs, outputDesc, outputs, &attr, engineType, compileFlag,
                opPath, stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    compileFlag = ACL_COMPILE_SYS;
    ret = aclopCompileAndExecute(opType, numInputs, inputDesc, inputs,
                numOutputs, outputDesc, outputs, &attr, engineType, compileFlag,
                opPath, stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    opPath = "tests/ut/";
    ret = aclopCompileAndExecute(opType, numInputs, inputDesc, inputs,
                numOutputs, outputDesc, outputs, &attr, engineType, compileFlag,
                opPath, stream);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclDestroyDataBuffer(inputs[0]);
    aclDestroyDataBuffer(inputs[1]);
    aclDestroyDataBuffer(outputs[0]);
}

TEST_F(UTEST_ACL_OpCompiler, aclopCompile)
{
    char *opType = "Add";
    int numInputs = 2;

    int64_t shape[]{16, -1};
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);

    aclDataBuffer *inputs[2];
    inputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);
    inputs[1] = aclCreateDataBuffer((void *) 0x1000, 1024);

    aclDataBuffer *outputs[1];
    outputs[0] = aclCreateDataBuffer((void *) 0x1000, 1024);

    int numOutputs = 1;

    aclopAttr attr;
    string str = "";
    attr.SetAttr("unregst _attrlist", str);

    const char *opPath = "tests/";
    aclopEngineType engineType = ACL_ENGINE_AICORE;
    aclopCompileType compileFlag = ACL_COMPILE_UNREGISTERED;

    aclError ret = aclopCompile(opType, numInputs, inputDesc,
                numOutputs, outputDesc, &attr, engineType, compileFlag,
                opPath);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    compileFlag = ACL_COMPILE_UNREGISTERED;
    opPath = nullptr;
    ret = aclopCompile(opType, numInputs, inputDesc,
                numOutputs, outputDesc, &attr, engineType, compileFlag,
                opPath);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}