#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>

#define protected public
#define private public
#include "single_op/op_model_parser.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "utils/file_utils.h"
#include "single_op/op_model_parser.h"
#include "framework/common/helper/om_file_helper.h"
#include "framework/common/types.h"
#include "acl_stub.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_tensor.h"


using namespace std;
using namespace testing;
using namespace acl;

class OpModelParserTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};
//
ge::GeAttrValue::NAMED_ATTRS value1;
vector<ge::GeAttrValue::NAMED_ATTRS> g_value{value1};

TEST_F(OpModelParserTest, TestDeserializeModel)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Init(_, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelPartition(_, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Load(_, _, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));

    OpModel opModel;
    ge::Model geModel;
    opModel.size = sizeof(struct ge::ModelFileHeader) + 128;
    ge::ModelFileHeader header;
    header.length = 128;
    opModel.data = std::shared_ptr<void>(&header, [](void *) {});

    ASSERT_NE(OpModelParser::DeserializeModel(opModel, geModel), ACL_SUCCESS);
    ASSERT_NE(OpModelParser::DeserializeModel(opModel, geModel), ACL_SUCCESS);
    ASSERT_NE(OpModelParser::DeserializeModel(opModel, geModel), ACL_SUCCESS);
    ASSERT_EQ(OpModelParser::DeserializeModel(opModel, geModel), ACL_SUCCESS);
}

TEST_F(OpModelParserTest, TestParseModelContent)
{
    OpModel opModel;
    ge::Model geModel;

    ge::ModelFileHeader header;
    header.length = 128;
    opModel.data = std::shared_ptr<void>(&header, [](void *) {});

    uint32_t modelSize;
    uint8_t *modelData = nullptr;
    opModel.size = 0;
    EXPECT_NE(OpModelParser::ParseModelContent(opModel, modelSize, modelData), ACL_SUCCESS);

    opModel.size = sizeof(struct ge::ModelFileHeader) + 129;
    EXPECT_NE(OpModelParser::ParseModelContent(opModel, modelSize, modelData), ACL_SUCCESS);

    opModel.size = sizeof(struct ge::ModelFileHeader) + 128;
    EXPECT_EQ(OpModelParser::ParseModelContent(opModel, modelSize, modelData), ACL_SUCCESS);
}

bool GetBool_invoke(ge::AttrUtils::ConstAttrHolderAdapter obj, const string &name, bool &value)
{
    value = false;
    return true;
}

TEST_F(OpModelParserTest, TestToModelConfig)
{
    OpModelDef opModelDef;
    ge::Model geModel;
    ASSERT_EQ(OpModelParser::ToModelConfig(geModel, opModelDef), ACL_ERROR_MODEL_MISSING_ATTR);
}

TEST_F(OpModelParserTest, ParseGeTensorDescTest)
{
    vector<ge::GeTensorDesc> geTensorDescList;
    vector<aclTensorDesc> output;
    string opName = "TransData";
    OpModelParser::ParseGeTensorDesc(geTensorDescList, output, opName);
    EXPECT_EQ(output.size(), 0);

    ge::GeTensorDesc desc;
    geTensorDescList.emplace_back(desc);
    OpModelParser::ParseGeTensorDesc(geTensorDescList, output, opName);
    EXPECT_EQ(output.size(), 1);
    opName = "Add";
    EXPECT_EQ(OpModelParser::ParseGeTensorDesc(geTensorDescList, output, opName), ACL_SUCCESS);
}

TEST_F(OpModelParserTest, TestParseOpAttrs)
{
    ge::Model model;
    aclopAttr attr;
    OpModelParser::ParseOpAttrs(model, attr);
}

TEST_F(OpModelParserTest, TestParseOpModel)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), Init(_, _))
        .WillOnce(Return(ACL_ERROR_DESERIALIZE_MODEL))
        .WillRepeatedly(Return(ACL_SUCCESS));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetListTensor(_, _, _))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));

    OpModelDef opModelDef;
    OpModel opModel;
    opModel.size = sizeof(struct ge::ModelFileHeader) + 128;
    ge::ModelFileHeader header;
    header.length = 128;
    opModel.data = std::shared_ptr<void>(&header, [](void *) {});
    ASSERT_NE(OpModelParser::ParseOpModel(opModel, opModelDef), ACL_SUCCESS);
    ASSERT_NE(OpModelParser::ParseOpModel(opModel, opModelDef), ACL_SUCCESS);
    ASSERT_EQ(OpModelParser::ParseOpModel(opModel, opModelDef), ACL_SUCCESS);
}

bool GetInt_invoke(ge::AttrUtils::ConstAttrHolderAdapter obj, const string &name, int32_t& value)
{
    value = 1;
    return true;
}

TEST_F(OpModelParserTest, TestToModelConfig1)
{
    OpModelDef opModelDef;
    ge::Model geModel;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetInt(_, _, _))
        .WillRepeatedly(Invoke(GetInt_invoke));
    OpModelParser::ToModelConfig(geModel, opModelDef);
}

bool GetListNamedAttrs_invoke(ge::AttrUtils::ConstAttrHolderAdapter obj, const string &name, vector<ge::GeAttrValue::NAMED_ATTRS> &value)
{   value = g_value;
    return true;
}

TEST_F(OpModelParserTest, TestToModelConfig2)
{
    OpModelDef opModelDef;
    ge::Model geModel;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetListTensor(_, _, _))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetInt(_, _, _))
        .WillRepeatedly(Invoke(GetInt_invoke));
    OpModelParser::ToModelConfig(geModel, opModelDef);
}
