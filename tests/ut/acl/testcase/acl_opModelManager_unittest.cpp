#include <vector>

#include <gtest/gtest.h>

#define protected public
#define private public
#include "single_op/op_model_manager.h"
#undef private
#undef protected

#define protected public
#define private public
#include "utils/acl_op_map.h"
#undef private
#undef protected

#define protected public
#define private public
#include "utils/acl_dynamic_shape_op_map.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "json_parser.h"
#include "utils/file_utils.h"
#include "utils/attr_utils.h"
#include "single_op/op_model_parser.h"
#include "single_op/compile/op_compile_service.h"
#include "common/common_inner.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::file_utils;

class UTEST_ACL_OpModelManager : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(UTEST_ACL_OpModelManager, RegisterModelTest)
{
    OpModelDef modelDef;
    modelDef.opType = "testOp";
    OpModelManager::ModelMap modelMap;
    OpModelManager::DynamicModelMap dynamicModelMap;
    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), modelMap, dynamicModelMap, false), ACL_SUCCESS);
}

aclError ListFilesMock(const std::string &dirName,
                        FileNameFilterFn filter,
                        std::vector<std::string> &names,
                        int maxDepth)
{
    names.emplace_back("test_model.om");
    return ACL_SUCCESS;
}

aclError ParseOpModelMock(OpModel &opModel, OpModelDef &modelDef)
{
    modelDef.opType = "testOp";
    return ACL_SUCCESS;
}

TEST_F(UTEST_ACL_OpModelManager, BackAclopMatchTest)
{
    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    int64_t shape[]{16, -1};
    std::vector<int64_t> storageDims;
    storageDims.push_back(16);
    storageDims.push_back(-1);
    aclopAttr *opAttr = aclopCreateAttr();
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
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
    aclTensorShapeStatus opShapeStatus;
    opShapeStatus.isUnkownRank = false;
    opShapeStatus.shapeStatus.push_back(true);
    opShapeStatus.shapeStatus.push_back(false);
    std::vector<aclTensorShapeStatus> tensorShapeStatus = {opShapeStatus, opShapeStatus, opShapeStatus};
    std::vector<std::vector<int64_t>> tensorDims = {{16,16}, {16,16}, {16,16}};
    std::vector<int64_t> storageTensorDims = {16,16,16,16,16,16};
    OpModelManager::BackAclopMatch(aclOp, tensorShapeStatus, tensorDims, storageTensorDims);
    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpModelManager, FixedAclopMatchTest)
{
    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 1;
    aclOp.numOutputs = 0;
    int64_t shape[]{16, 15};
    std::vector<int64_t> storageDims;
    storageDims.push_back(16);
    storageDims.push_back(15);
    const aclTensorDesc *inputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    const_cast<aclTensorDesc *>(inputDesc[0])->storageDims = storageDims;
    const_cast<aclTensorDesc *>(inputDesc[0])->storageFormat = ACL_FORMAT_NCHW;
    inputDesc[0]->IsDynamicTensor();
    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = nullptr;
    aclTensorShapeStatus opShapeStatus;
    opShapeStatus.isUnkownRank = true;
    std::vector<aclTensorShapeStatus> tensorShapeStatus = {opShapeStatus};
    std::vector<std::pair<int64_t, int64_t>> shapeRange;
    std::vector<std::vector<int64_t>> tensorDims;
    std::vector<int64_t> storageTensorDims;
    OpModelManager::FixedAclopMatch(aclOp, tensorShapeStatus, shapeRange, tensorDims, storageTensorDims);
    aclDestroyTensorDesc(inputDesc[0]);
}

TEST_F(UTEST_ACL_OpModelManager, SetTensorShapeStatusTest)
{
    AclOp aclOp;
    aclOp.opType = "Add";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    int64_t shape[]{16, -1};
    std::vector<int64_t> storageDims;
    storageDims.push_back(16);
    storageDims.push_back(-1);
    aclopAttr *opAttr = aclopCreateAttr();
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
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
    std::vector<aclTensorShapeStatus> shapeStatus;
    auto &instance = OpModelManager::GetInstance();
    instance.SetTensorShapeStatus(aclOp, shapeStatus);
    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpModelManager, CheckRangeTest)
{
    int tensorNum = 2;
    const aclTensorDesc *inputDesc[2];
    int64_t shapeFind[]{16, 16};
    int64_t range[2][2] = {{16, 16}, {1, 16}};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    aclSetTensorShapeRange((aclTensorDesc*)inputDesc[0], 2, range);
    aclSetTensorShapeRange((aclTensorDesc*)inputDesc[1], 2, range);

    aclTensorShapeStatus inputStatus1;
    inputStatus1.shapeStatus= {true, false};
    aclTensorShapeStatus inputStatus2;
    inputStatus2.shapeStatus= {true, true};
    std::vector<aclTensorShapeStatus> tensorShapeStatus;
    tensorShapeStatus.emplace_back(inputStatus1);
    tensorShapeStatus.emplace_back(inputStatus2);

    std::vector<std::pair<int64_t, int64_t>> shapeRange;
    shapeRange.emplace_back(make_pair(16, 16));
    shapeRange.emplace_back(make_pair(1, 16));
    shapeRange.emplace_back(make_pair(16, 16));
    shapeRange.emplace_back(make_pair(16, 16));

    AclOp aclOp;
    aclOp.inputDesc = inputDesc;
    aclOp.numInputs = 2;
    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.CheckShapeRange(aclOp, tensorShapeStatus, shapeRange), true);

    aclDestroyTensorDesc(inputDesc[0]);
    int64_t shapeFind2[]{16, -1};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind2, ACL_FORMAT_ND);
    aclSetTensorShapeRange((aclTensorDesc*)inputDesc[0], 2, range);
    std::vector<std::pair<int64_t, int64_t>> shapeRange2;
    shapeRange2.emplace_back(make_pair(16, 16));
    shapeRange2.emplace_back(make_pair(2, -1));
    shapeRange2.emplace_back(make_pair(16, 16));
    shapeRange2.emplace_back(make_pair(16, 16));
    EXPECT_NE(instance.CheckShapeRange(aclOp, tensorShapeStatus, shapeRange2), true);

    std::vector<std::pair<int64_t, int64_t>> shapeRange3;
    shapeRange3.emplace_back(make_pair(16, 16));
    shapeRange3.emplace_back(make_pair(2, 17));
    shapeRange3.emplace_back(make_pair(16, 16));
    shapeRange3.emplace_back(make_pair(16, 16));
    EXPECT_NE(instance.CheckShapeRange(aclOp, tensorShapeStatus, shapeRange3), true);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
}

TEST_F(UTEST_ACL_OpModelManager, MatchModelDynamicTest)
{
    SetCastHasTruncateAttr(false);
    OpModelDef modelDef;
    modelDef.opType = "Cast";
    int64_t shape[]{16, -1};
    int64_t shapeStatic[]{16, 16};
    int64_t range[2][2] = {{16, 16}, {1, 16}};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef.inputDescArr[0], 2, range);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");
    modelDef.opAttr.SetAttr<string>("truncate", "1");

    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, true), ACL_SUCCESS);

    OpModelDef modelDef_2;
    modelDef.opType = "test_acl";
    int64_t shape_2[]{16, -1};
    int64_t shapeStatic_2[]{16, 16};
    int64_t range_2[2][2] = {{16, 16}, {1, 16}};
    modelDef_2.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef_2.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic_2, ACL_FORMAT_ND);
    modelDef_2.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic_2, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef_2.inputDescArr[0], 2, range_2);
    modelDef_2.opAttr.SetAttr<string>("testAttr", "attrValue");
    modelDef_2.opAttr.SetAttr<string>("truncate", "1");


    AclOp aclOp;
    aclopAttr *opAttr = aclopCreateAttr();
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    int64_t shapeFind[]{16, 16};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);

    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;

    OpModel opModel;
    bool isDynamic;
    aclOp.opType = "Cast";
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numInputs = 1;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numInputs = 2;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numOutputs = 1;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.opAttr = opAttr;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclopSetAttrString(opAttr, "testAttr", "invalid");
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclopSetAttrString(opAttr, "testAttr", "attrValue");
    EXPECT_EQ(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_ERROR_OP_NOT_FOUND);
    aclOp.isCompile = true;
    EXPECT_EQ(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_ERROR_OP_NOT_FOUND);
    aclOp.isCompile = false;
    aclDestroyTensorDesc(inputDesc[0]);
    int64_t shapeFindDynamic[]{16, -1};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFindDynamic, ACL_FORMAT_ND);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.isCompile = true;
    aclDestroyTensorDesc(inputDesc[0]);
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    int64_t rangeStatic[2][2] = {{16, 16}, {16, 16}};
    aclSetTensorShapeRange((aclTensorDesc*)aclOp.inputDesc[0], 2, range);
    aclSetTensorShapeRange((aclTensorDesc*)aclOp.inputDesc[1], 2, rangeStatic);
    aclSetTensorShapeRange((aclTensorDesc*)aclOp.outputDesc[0], 2, rangeStatic);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    int64_t range2[2][2] = {{16, 16}, {1, 32}};
    aclSetTensorShapeRange((aclTensorDesc*)aclOp.inputDesc[0], 2, range2);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    int64_t range3[2][2] = {{16, 16}, {1, -1}};
    aclSetTensorShapeRange((aclTensorDesc*)aclOp.inputDesc[0], 2, range3);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    int64_t shapeFind2[]{16, 0};
    aclDestroyTensorDesc(inputDesc[0]);
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind2, ACL_FORMAT_ND);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    int64_t shapeFind3[]{16, 17};
    aclDestroyTensorDesc(inputDesc[0]);
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind3, ACL_FORMAT_ND);
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);
}


TEST_F(UTEST_ACL_OpModelManager, MatchModelDynamicHashTest)
{
    SetCastHasTruncateAttr(false);
    OpModelDef modelDef;
    modelDef.opType = "Cast";
    int64_t shape[]{16, -1};
    int64_t shapeStatic[]{16, 16};
    int64_t range[2][2] = {{16, 16}, {1, 16}};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef.inputDescArr[0], 2, range);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, true), ACL_SUCCESS);

    AclOp aclOp;
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    int64_t shapeFind[]{16, 16};
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shapeFind, ACL_FORMAT_ND);
    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;

    OpModel opModel;
    bool isDynamic;
    aclOp.opType = "Cast";
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    aclopAttr *opAttr = aclopCreateAttr();
    aclopSetAttrString(opAttr, "testAttr", "attrValue");
    aclOp.opAttr = opAttr;

    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);

    OpModelDef modelDef_2;
    modelDef.opType = "test_acl";
    int64_t shape_2[]{16, -1};
    int64_t shapeStatic_2[]{16, 16};
    int64_t range_2[2][2] = {{16, 16}, {1, 16}};
    modelDef_2.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef_2.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic_2, ACL_FORMAT_ND);
    modelDef_2.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic_2, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef_2.inputDescArr[0], 2, range_2);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto modelDefPtr = shared_ptr<OpModelDef>(new (std::nothrow)OpModelDef(std::move(modelDef_2)));
    instance.dynamicOpModels_.hashMap_[17836075261947842321].push_back(std::move(modelDefPtr));

    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);

    instance.dynamicOpModels_.hashMap_[17836075261947842321].clear();
}

TEST_F(UTEST_ACL_OpModelManager, MatchModelHashTest)
{
    OpModelDef modelDef;
    modelDef.opType = "testOp";
    int64_t shape[]{16, 16};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, false), ACL_SUCCESS);

    AclOp aclOp;
    aclopAttr *opAttr = aclopCreateAttr();
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    int64_t shape1[]{32, 32};

    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);

    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;

    OpModel opModel;
    bool isDynamic;
    aclOp.opType = "testOp";
    aclopSetAttrString(opAttr, "testAttr", "attrValue");
    aclOp.opAttr = opAttr;
    aclOp.numInputs = 2;
    aclOp.numOutputs = 1;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);

    OpModelDef modelDef_2;
    modelDef.opType = "acltest";
    int64_t shape_2[]{16, 16};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto modelDefPtr = shared_ptr<OpModelDef>(new (std::nothrow)OpModelDef(std::move(modelDef_2)));
    instance.opModels_.hashMap_[6687538955415257199].push_back(std::move(modelDefPtr));

    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);

    instance.opModels_.hashMap_[6687538955415257199].clear();
}

TEST_F(UTEST_ACL_OpModelManager, MatchModelTest)
{
    OpModelDef modelDef;
    modelDef.opType = "testOp";
    int64_t shape[]{16, 16};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, false), ACL_SUCCESS);

    OpModelDef modelDef_2;
    modelDef.opType = "acltest";
    int64_t shape_2[]{16, 16};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape_2, ACL_FORMAT_ND);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    AclOp aclOp;
    aclopAttr *opAttr = aclopCreateAttr();
    const aclTensorDesc *inputDesc[2];
    const aclTensorDesc *outputDesc[1];
    inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    inputDesc[1] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    outputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);

    aclOp.inputDesc = inputDesc;
    aclOp.outputDesc = outputDesc;

    OpModel opModel;
    bool isDynamic;
    aclOp.opType = "testOp";
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numInputs = 1;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numInputs = 2;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.numOutputs = 1;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclOp.opAttr = opAttr;
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclopSetAttrString(opAttr, "testAttr", "invalid");
    EXPECT_NE(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_SUCCESS);
    aclopSetAttrString(opAttr, "testAttr", "attrValue");
    EXPECT_EQ(instance.MatchOpModel(aclOp, opModel, isDynamic), ACL_ERROR_FAILURE);

    aclDestroyTensorDesc(inputDesc[0]);
    aclDestroyTensorDesc(inputDesc[1]);
    aclDestroyTensorDesc(outputDesc[0]);
    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpModelManager, TestStaticMapAging)
{
    OpModelDef modelDef;
    modelDef.opType = "testOpStatic";
    int64_t shape[]{16, 16};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto &instance = OpModelManager::GetInstance();
    EXPECT_EQ(instance.HandleMaxOpQueueConfig("../tests/ut/acl/json/aging.json"), ACL_SUCCESS);
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, false, false), ACL_SUCCESS);

    OpModelDef modelDef1;
    modelDef1.opType = "testOpStatic1";
    modelDef1.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef1.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef1.outputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef1.opAttr.SetAttr<string>("testAttr", "attrValue");
    EXPECT_EQ(instance.RegisterModel(std::move(modelDef1), instance.opModels_, instance.dynamicOpModels_, false, false), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpModelManager, TestDynamicMapAging)
{
    OpModelDef modelDef;
    modelDef.opType = "testOpDynamic";
    int64_t shape[]{16, -1};
    int64_t shapeStatic[]{16, 16};
    int64_t range[2][2] = {{16, 16}, {1, 16}};
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    modelDef.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef.inputDescArr[0], 2, range);
    modelDef.opAttr.SetAttr<string>("testAttr", "attrValue");

    auto &instance = OpModelManager::GetInstance();
    ASSERT_EQ(instance.HandleMaxOpQueueConfig("../tests/ut/acl/json/aging.json"), ACL_SUCCESS);
    ASSERT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, true, false), ACL_SUCCESS);

    OpModelDef modelDef1;
    modelDef1.opType = "testOpDynamic1";
    modelDef1.inputDescArr.emplace_back(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    modelDef1.inputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    modelDef1.outputDescArr.emplace_back(ACL_FLOAT16, 2, shapeStatic, ACL_FORMAT_ND);
    aclSetTensorShapeRange(&modelDef1.inputDescArr[0], 2, range);
    modelDef1.opAttr.SetAttr<string>("testAttr", "attrValue");
    ASSERT_EQ(instance.RegisterModel(std::move(modelDef), instance.opModels_, instance.dynamicOpModels_, true, false), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpModelManager, TestGetOpModel)
{
    AclOp aclOp;
    aclOp.opType = "newOp";
    auto &instance = OpModelManager::GetInstance();
    EXPECT_NE(instance.GetOpModel(aclOp), ACL_SUCCESS);

}

TEST_F(UTEST_ACL_OpModelManager, TestOmFileFilterFn)
{
    ASSERT_TRUE(OpModelManager::OmFileFilterFn("a.om"));
    ASSERT_TRUE(OpModelManager::OmFileFilterFn("aaa.om"));
    ASSERT_TRUE(OpModelManager::OmFileFilterFn("a_123.om"));
    ASSERT_TRUE(OpModelManager::OmFileFilterFn(".om"));
    ASSERT_FALSE(OpModelManager::OmFileFilterFn("a_123.o"));
    ASSERT_FALSE(OpModelManager::OmFileFilterFn("a_123.m"));
    ASSERT_FALSE(OpModelManager::OmFileFilterFn("a_123o.m"));
    ASSERT_FALSE(OpModelManager::OmFileFilterFn("a_123om"));
    ASSERT_FALSE(OpModelManager::OmFileFilterFn("om"));
}

TEST_F(UTEST_ACL_OpModelManager, DebugString)
{
    aclTensorDesc desc;
    desc.isConst = true;
    auto *data = new (std::nothrow) int[4];
    std::shared_ptr<void> modelData;
    modelData.reset(data, [](const int *p) { delete[]p; });
    desc.constDataBuf = modelData;
    desc.constDataLen = 4;
    desc.DebugString();
    vector<int64_t> shape{1};
    desc.UpdateTensorShape(shape);
    std::pair<int64_t, int64_t> range;
    std::vector<std::pair<int64_t, int64_t>> ranges{range};
    desc.UpdateTensorShapeRange(ranges);
}

TEST_F(UTEST_ACL_OpModelManager, HandleMaxOpQueueConfigTest)
{
    OpModelManager modelManager;
    aclError ret = modelManager.HandleMaxOpQueueConfig("llt/acl/ut/json/handleOpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = modelManager.HandleMaxOpQueueConfig("../tests/ut/acl/json/handleOpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpModelManager, LoadModelFromMemTest)
{
    size_t modelSize = 20;
    auto *aclModelData = new (std::nothrow) char[modelSize];
    bool isStatic = false;
    auto &instance = OpModelManager::GetInstance();
    EXPECT_NE(instance.LoadModelFromMem(aclModelData, modelSize, isStatic), ACL_SUCCESS);
    delete []aclModelData;
}

TEST_F(UTEST_ACL_OpModelManager, BuildOpModelTest)
{
    AclOp aclop;
    auto &instance = OpModelManager::GetInstance();
    EXPECT_NE(instance.BuildOpModel(aclop), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpModelManager, LoadAllModelsTest)
{
    EXPECT_EQ(OpModelManager::GetInstance().LoadAllModels("op_models"), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_OpModelManager, LoadModelFromMemFailedTest)
{
    int modelSize = 10;
    char* model = new(std::nothrow) char[modelSize]();
    auto &instance = OpModelManager::GetInstance();
    EXPECT_NE(instance.LoadModelFromMem(model, modelSize), ACL_SUCCESS);

    modelSize = 257;
    auto *aclModelData = new (std::nothrow) char[modelSize];
    bool isStatic = false;
    EXPECT_NE(instance.LoadModelFromMem(aclModelData, modelSize, isStatic), ACL_SUCCESS);
    delete []aclModelData;
    delete []model;
}