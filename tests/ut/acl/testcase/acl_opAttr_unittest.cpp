#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#include "graph/utils/attr_utils.h"
#undef private

#include "acl/acl.h"
#include "types/op_attr.h"
#include "graph/ge_attr_value.h"
#include "utils/attr_utils.h"
#include "acl_stub.h"

using namespace testing;
using namespace acl;
using namespace ge;

namespace acl {
    namespace attr_utils {
        extern bool IsListFloatEquals(const vector<float> &lhsValue, const vector<float> &rhsValue);
        extern void GeAttrValueToStringForDigest(std::string &buffer, const ge::GeAttrValue &val);
    }
}

class UTEST_ACL_OpAttr : public testing::Test {
protected:
    virtual void SetUp() {}

    virtual void TearDown() {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(UTEST_ACL_OpAttr, OpAttrEqualsTest)
{
    aclopAttr *opAttr1 = aclopCreateAttr();
    ASSERT_FALSE(attr_utils::OpAttrEquals(nullptr, opAttr1));
    EXPECT_EQ(aclopSetAttrString(opAttr1, "string", "string"), ACL_SUCCESS);
    ASSERT_TRUE(attr_utils::OpAttrEquals(opAttr1, opAttr1));
    aclopAttr *opAttr2 = aclopCreateAttr();
    ASSERT_FALSE(attr_utils::OpAttrEquals(opAttr1, opAttr2));
    EXPECT_EQ(aclopSetAttrString(opAttr2, "string1", "string1"), ACL_SUCCESS);
    ASSERT_FALSE(attr_utils::OpAttrEquals(opAttr1, opAttr2));
    aclopDestroyAttr(opAttr1);
    aclopDestroyAttr(opAttr2);
}

TEST_F(UTEST_ACL_OpAttr, SetScalarAttrTest)
{
    aclopAttr *opAttr = aclopCreateAttr();
    EXPECT_EQ(aclopSetAttrString(opAttr, "string", "string"), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrInt(opAttr, "666", 666), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrInt(opAttr, "666666", 666666), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrBool(opAttr, "false", false), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrBool(opAttr, "true", true), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrFloat(opAttr, "float", 1.0), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());

    for (auto &it : opAttr->Attrs()) {
        ASSERT_TRUE(attr_utils::AttrValueEquals(it.second, it.second));
    }
    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpAttr, SetListAttrTest)
{
    aclopAttr *opAttr = aclopCreateAttr();
    const char *string1 = "string1";
    const char *string2 = "string2";
    const char *argv[2] = {string1, string2};

    int64_t intList[3]{1, 2, 3};
    uint8_t boolList[2]{false, true};
    float floatList[2]{1.0, 0.0};
    EXPECT_EQ(aclopSetAttrListString(opAttr, "stringList", 2, argv), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrListBool(opAttr, "boolList", 2, boolList), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrListInt(opAttr, "intList", 3, intList), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());
    EXPECT_EQ(aclopSetAttrListFloat(opAttr, "floatList", 2, floatList), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());

    for (auto &it : opAttr->Attrs()) {
        ASSERT_TRUE(attr_utils::AttrValueEquals(it.second, it.second));
    }
    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpAttr, SetListListAttrTest)
{
    aclopAttr *opAttr = aclopCreateAttr();

    int64_t value1[2] = {1, 2};
    int64_t value2[3] = {4, 5, 6};
    const int64_t *values[2] = {value1, value2};
    int numValues[2] = {2, 3};

    EXPECT_EQ(aclopSetAttrListListInt(opAttr, "ListListInt", 2, numValues, values), ACL_SUCCESS);
    ASSERT_FALSE(opAttr->DebugString().empty());

    for (auto &it : opAttr->Attrs()) {
        ASSERT_TRUE(attr_utils::AttrValueEquals(it.second, it.second));
    }

    aclopDestroyAttr(opAttr);
}

TEST_F(UTEST_ACL_OpAttr, SetAttrByTypeTest)
{
    aclopAttr opAttr;
    int64_t int16Value = 16;
    int8_t int8Value = 8;
    int32_t int32Value = 32;
    float floatValue = 1.0;

    EXPECT_EQ(opAttr.SetAttrByType("value", ACL_FLOAT, &floatValue), ACL_SUCCESS);
    EXPECT_EQ(opAttr.SetAttrByType("value", ACL_FLOAT16, &int16Value), ACL_SUCCESS);
    EXPECT_EQ(opAttr.SetAttrByType("value", ACL_INT32, &int32Value), ACL_SUCCESS);
    EXPECT_EQ(opAttr.SetAttrByType("value", ACL_INT16, &int16Value), ACL_SUCCESS);
    EXPECT_EQ(opAttr.SetAttrByType("value", ACL_INT8, &int8Value), ACL_SUCCESS);
    EXPECT_NE(opAttr.SetAttrByType("value", static_cast<aclDataType>(1000), &int8Value), ACL_SUCCESS);
};

TEST_F(UTEST_ACL_OpAttr, GeAttrValueToStringForDigestTest)
{
    string buffer;
    ge::GeAttrValue value;
    value.SetValue<string>("hello");
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "hello");

    value.SetValue<bool>(false);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "helloF");
    value.SetValue<bool>(true);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "helloFT");

    value.SetValue<int64_t>(666);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "helloFT666");

    value.SetValue<float>(1.11f);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);

    vector<string> val1{"hello", "world"};
    vector<bool> val2{false, true};
    vector<int64_t> val3{666, 444};
    vector<vector<int64_t>> val4;
    val4.emplace_back(vector<int64_t>{1,2});
    val4.emplace_back(vector<int64_t>{3,4,5});
    vector<float> val5{1.0f, 2.0f};

    buffer = "";
    value.SetValue(val1);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "hello,world,");
    value.SetValue(val2);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "hello,world,FT");
    value.SetValue(val3);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "hello,world,FT666,444,");
    value.SetValue(val4);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);
    EXPECT_EQ(buffer, "hello,world,FT666,444,1,2,|3,4,5,|");
    value.SetValue(val5);
    attr_utils::GeAttrValueToStringForDigest(buffer, value);

    map<string, ge::GeAttrValue> attr;
    EXPECT_EQ(attr_utils::AttrMapToDigest(attr), 0);
    attr.emplace("alpha", value);
    attr_utils::AttrMapToDigest(attr);
}

TEST_F(UTEST_ACL_OpAttr, ListFloatEqualsTest)
{
    vector<float> lhsValue;
    vector<float> rhsValue;
    ASSERT_TRUE(attr_utils::IsListFloatEquals(lhsValue, rhsValue));

    lhsValue.push_back(1.0000001f);
    ASSERT_FALSE(attr_utils::IsListFloatEquals(lhsValue, rhsValue));

    rhsValue.push_back(1.0000002f);
    ASSERT_TRUE(attr_utils::IsListFloatEquals(lhsValue, rhsValue));

    rhsValue[0] = 1.0002f;
    ASSERT_FALSE(attr_utils::IsListFloatEquals(lhsValue, rhsValue));
}