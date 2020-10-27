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

using namespace std;
using namespace testing;
using namespace acl;

class OpModelParserTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

ge::GeAttrValue::NAMED_ATTRS value1;
vector<ge::GeAttrValue::NAMED_ATTRS> g_value{value1};

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

    ASSERT_EQ(OpModelParser::ToModelConfig(geModel, opModelDef), ACL_SUCCESS);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetBool(_, _, _))
        .WillOnce(Invoke(GetBool_invoke))
        .WillRepeatedly(Return(false));

    ASSERT_EQ(OpModelParser::ToModelConfig(geModel, opModelDef), ACL_ERROR_OP_UNSUPPORTED_DYNAMIC);
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
        .WillOnce(Invoke(GetInt_invoke));
    // 返回值未做判断
    OpModelParser::ToModelConfig(geModel, opModelDef);
}

bool GetListNamedAttrs_invoke(ge::AttrUtils::ConstAttrHolderAdapter obj, const string &name, vector<ge::GeAttrValue::NAMED_ATTRS> &value)
{   value = g_value;
    return true;
}

TEST_F(OpModelParserTest, TestToModelConfig2)
{
    OpModelDef opModelDef;
    aclTensorDesc desc;
    opModelDef.inputDescArr.push_back(desc);
    ge::Model geModel;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetInt(_, _, _))
        .WillOnce(Invoke(GetInt_invoke));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetListNamedAttrs(_, _, _))
        .WillOnce(Invoke(GetListNamedAttrs_invoke))
        .WillOnce(Return(false))
        .WillOnce(Return(false));
    // 返回值未做判断
    OpModelParser::ToModelConfig(geModel, opModelDef);
}

TEST_F(OpModelParserTest, TestToModelConfig3)
{
    OpModelDef opModelDef;
    ge::Model geModel;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetInt(_, _, _))
        .WillOnce(Invoke(GetInt_invoke));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetListNamedAttrs(_, _, _))
        .WillOnce(Invoke(GetListNamedAttrs_invoke))
        .WillOnce(Return(false))
        .WillOnce(Return(false));
    OpModelParser::ToModelConfig(geModel, opModelDef);
}