#include <gtest/gtest.h>

#ifndef private
#define private public
#include "acl/acl.h"
#include "log_inner.h"
#undef private
#endif

#define OFFSET_OF_MEMBER(type, member) (size_t)(&(((type *)0)->member))

class UTEST_ACL_compatibility_struct_check : public testing::Test
{
    public:
        UTEST_ACL_compatibility_struct_check() {}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_ACL_compatibility_struct_check, aclmdlIODims)
{
    // check every member
    size_t offset;
    offset  = OFFSET_OF_MEMBER(aclmdlIODims, name);
    EXPECT_EQ(offset, 0);

    offset  = OFFSET_OF_MEMBER(aclmdlIODims, dimCount);
    EXPECT_EQ(offset, 128);

    offset  = OFFSET_OF_MEMBER(aclmdlIODims, dims);
    EXPECT_EQ(offset, 128 + sizeof(size_t));

    // check total size
    EXPECT_EQ(sizeof(aclmdlIODims), 128 + sizeof(size_t) + 128*sizeof(int64_t));
}

TEST_F(UTEST_ACL_compatibility_struct_check, aclAippDims)
{
    size_t offset;
    offset = OFFSET_OF_MEMBER(aclAippDims, srcDims);
    EXPECT_EQ(offset, 0);

    offset = OFFSET_OF_MEMBER(aclAippDims, srcSize);
    EXPECT_EQ(offset, sizeof(aclmdlIODims));

    offset = OFFSET_OF_MEMBER(aclAippDims, aippOutdims);
    EXPECT_EQ(offset, sizeof(aclmdlIODims) + sizeof(size_t));

    offset = OFFSET_OF_MEMBER(aclAippDims, aippOutSize);
    EXPECT_EQ(offset, 2*sizeof(aclmdlIODims) + sizeof(size_t));
    
    EXPECT_EQ(sizeof(aclAippDims), 2*(sizeof(size_t) + sizeof(aclmdlIODims)));
}

TEST_F(UTEST_ACL_compatibility_struct_check, aclmdlBatch)
{
    size_t offset;
    offset = OFFSET_OF_MEMBER(aclmdlBatch, batchCount);
    EXPECT_EQ(offset, 0);

    offset = OFFSET_OF_MEMBER(aclmdlBatch, batch);
    EXPECT_EQ(offset, sizeof(size_t));

    EXPECT_EQ(sizeof(aclmdlBatch), sizeof(size_t) + 128*sizeof(uint64_t));
}

TEST_F(UTEST_ACL_compatibility_struct_check, aclmdlHW)
{
    size_t offset;
    offset = OFFSET_OF_MEMBER(aclmdlHW, hwCount);
    EXPECT_EQ(offset, 0);

    offset = OFFSET_OF_MEMBER(aclmdlHW, hw);
    EXPECT_EQ(offset, sizeof(size_t));

    EXPECT_EQ(sizeof(aclmdlHW), sizeof(size_t) + 2*128*sizeof(uint64_t));
}

TEST_F(UTEST_ACL_compatibility_struct_check, aclAippInfo)
{
    size_t offset;
    size_t offsetExpected;

    offset = OFFSET_OF_MEMBER(aclAippInfo, inputFormat);
    offsetExpected = 0;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, srcImageSizeW);
    offsetExpected += sizeof(aclAippInputFormat);
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, srcImageSizeH);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, cropSwitch);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, loadStartPosW);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, loadStartPosH);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, cropSizeW);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, cropSizeH);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, resizeSwitch);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, resizeOutputW);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, resizeOutputH);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, paddingSwitch);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, leftPaddingSize);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, rightPaddingSize);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, topPaddingSize);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, bottomPaddingSize);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, cscSwitch);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, rbuvSwapSwitch);
    offsetExpected += 1;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, axSwapSwitch);
    offsetExpected += 1;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, singleLineMode);
    offsetExpected += 1;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR0C0);
    offsetExpected += 1;
    EXPECT_EQ(offset, offsetExpected); 

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR0C1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR0C2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR1C0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR1C1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR1C2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR2C0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR2C1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, matrixR2C2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, outputBias0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, outputBias1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, outputBias2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, inputBias0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, inputBias1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, inputBias2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, meanChn0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, meanChn1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, meanChn2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, meanChn3);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, minChn0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, minChn1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, minChn2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, minChn3);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, varReciChn0);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, varReciChn1);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, varReciChn2);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, varReciChn3);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, srcFormat);
    offsetExpected += 4;
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, srcDatatype);
    offsetExpected += sizeof(aclFormat);
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, srcDimNum);
    offsetExpected += sizeof(aclDataType);
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, shapeCount);
    offsetExpected += sizeof(size_t);
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, outDims);
    offsetExpected += sizeof(size_t);
    EXPECT_EQ(offset, offsetExpected);

    offset = OFFSET_OF_MEMBER(aclAippInfo, aippExtend);
    offsetExpected += 128*sizeof(aclAippDims);
    EXPECT_EQ(offset, offsetExpected);

    size_t total_size;
    total_size = sizeof(aclAippInputFormat) + sizeof(aclFormat) + sizeof(aclDataType) + 2*sizeof(size_t) +
        128*sizeof(aclAippDims) + sizeof (aclAippExtendInfo *) + 39*4 + 4 + 3*4;
    EXPECT_EQ(sizeof(aclAippInfo), total_size);
}