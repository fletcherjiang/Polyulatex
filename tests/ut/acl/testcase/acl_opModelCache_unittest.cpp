#include <vector>

#include <gtest/gtest.h>

#define protected public
#define private public
#include "single_op/op_model_cache.h"
#undef private
#undef protected

#include "acl/acl.h"

using namespace acl;
using namespace std;

class UTEST_ACL_OpModelCache : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}

    OpModelCache cache_;
};

TEST_F(UTEST_ACL_OpModelCache, AddTest)
{
    OpModelDef def;
    OpModel model;
    EXPECT_EQ(cache_.Add(def, model), ACL_SUCCESS);
}


TEST_F(UTEST_ACL_OpModelCache, GetTest)
{
    OpModelDef def;
    def.opType = "testOp";
    def.modelPath = "modelPath";
    OpModel model;
    EXPECT_NE(cache_.GetOpModel(def, model), ACL_SUCCESS);

    cache_.Add(def, model);
    EXPECT_EQ(cache_.GetOpModel(def, model), ACL_SUCCESS);
}