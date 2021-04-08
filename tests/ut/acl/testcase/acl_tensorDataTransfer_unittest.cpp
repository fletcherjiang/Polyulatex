#include "acl/acl_base.h"
#include "acl/acl.h"
#include "acl/acl_tdt.h"
#include "acl/acl_tdt_queue.h"
#include "log_inner.h"


#include "tensor_data_transfer/tensor_data_transfer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "tdt_host_interface.h"
#include "data_common.h"
#include "acl_stub.h"

#define protected public
#define private public
#undef private
#undef protected

using namespace testing;
using namespace std;
using namespace acl;
using namespace tdt;

namespace acl {
    extern aclError TensorDatasetDeserializes(const std::vector<tdt::DataItem> &itemVec, acltdtDataset *dataset);
    extern aclError TensorDatasetSerializes(const acltdtDataset *dataset, std::vector<tdt::DataItem> &itemVec);
    extern aclError GetAclTypeByTdtDataType(tdt::TdtDataType tdtDataType, acltdtTensorType &aclType);
    extern void GetTensorDimsString(const int64_t *dims, size_t dimNum, std::string &dimsStr);
    extern aclError GetTdtDataTypeByAclDataType(acltdtTensorType aclType, tdt::TdtDataType &tdtDataType);
    extern bool GetTensorShape(const std::string &dimsStr, std::vector<int64_t> &dims);

    extern aclError GetTdtDataTypeByAclDataTypeV2(acltdtTensorType aclType, int32_t &tdtDataType);
    extern aclError GetAclTypeByTdtDataTypeV2(int32_t tdtDataType, acltdtTensorType &aclType);
    extern aclError TensorDatasetSerializesV2(const acltdtDataset *dataset, std::vector<aclTdtDataItemInfo> &itemVec);
    extern aclError TensorDatasetDeserializesV2(const std::vector<aclTdtDataItemInfo> &itemVec, acltdtDataset *dataset);
    extern aclError UnpackageRecvDataInfo(uint8_t *outputHostAddr, size_t size, std::vector<aclTdtDataItemInfo> &itemVec);
    extern aclError TensorDataitemSerialize(std::vector<aclTdtDataItemInfo> &itemVec,
                                            std::vector<rtMemQueueBuffInfo> &qBufVec);
}

class UTEST_tensor_data_transfer : public testing::Test
{
    public:
        UTEST_tensor_data_transfer(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_tensor_data_transfer, TestGetTensorTypeFromItem)
{
    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *dataItem = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_INT64, data, 10);
    acltdtTensorType tensorType = acltdtGetTensorTypeFromItem(dataItem);
    EXPECT_EQ(tensorType, ACL_SUCCESS);
    EXPECT_NE(acltdtGetTensorTypeFromItem(nullptr), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(dataItem), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestGetTensorInfo)
{
    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *dataItem = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_INT64, data, 1);
    aclDataType dataType = acltdtGetDataTypeFromItem(dataItem);
    EXPECT_NE(dataType, ACL_SUCCESS);
    EXPECT_NE(acltdtGetDataTypeFromItem(nullptr), ACL_SUCCESS);

    void *dataAddr = acltdtGetDataAddrFromItem(dataItem);
    EXPECT_NE(dataAddr, nullptr);

    EXPECT_NE(acltdtGetDataSizeFromItem(dataItem), 0);
    EXPECT_EQ(acltdtGetDataSizeFromItem(nullptr), 0);

    EXPECT_NE(acltdtGetDimNumFromItem(dataItem), 0);
    EXPECT_EQ(acltdtGetDimNumFromItem(nullptr), 0);

    int64_t *dim = new int64_t[4];
    EXPECT_NE(acltdtGetDimsFromItem(dataItem, dim, 1), ACL_SUCCESS);
    EXPECT_EQ(acltdtGetDimsFromItem(dataItem, dim, 5), ACL_SUCCESS);

    EXPECT_NE(dataItem, nullptr);
    acltdtDataItem *dataItemNull = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 0, ACL_INT64, data, 10);
    EXPECT_EQ(dataItemNull, nullptr);
    EXPECT_NE(acltdtDestroyDataItem(dataItemNull), ACL_SUCCESS);

    acltdtDataset *createDataSet = acltdtCreateDataset();
    EXPECT_EQ(acltdtDestroyDataset(createDataSet), ACL_SUCCESS);
    createDataSet = acltdtCreateDataset();
    EXPECT_NE(acltdtAddDataItem(createDataSet, dataItemNull), ACL_ERROR_FEATURE_UNSUPPORTED);
    EXPECT_EQ(acltdtAddDataItem(createDataSet, dataItem), ACL_SUCCESS);
    EXPECT_EQ(acltdtGetDataItem(nullptr, 0), nullptr);
    EXPECT_EQ(acltdtDestroyDataset(createDataSet), ACL_SUCCESS);

    createDataSet = acltdtCreateDataset();
    EXPECT_EQ(acltdtGetDatasetSize(createDataSet), 0);
    EXPECT_EQ(acltdtGetDatasetSize(nullptr), 0);
    EXPECT_EQ(acltdtDestroyDataset(createDataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(dataItem), ACL_SUCCESS);
    delete [] dim;
}

TEST_F(UTEST_tensor_data_transfer, TestTdtAddDataItem)
{
    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtDataItem *dataItem = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_INT64, data, 1);
    EXPECT_EQ(acltdtAddDataItem(dataSet, dataItem), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(dataItem), ACL_SUCCESS);

    dataSet = acltdtCreateDataset();
    dataItem = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_INT64, data, 1);
    dataSet->freeSelf = true;
    EXPECT_EQ(acltdtAddDataItem(dataSet, dataItem), ACL_ERROR_FEATURE_UNSUPPORTED);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(dataItem), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtChannel01)
{
    uint32_t deviceId = 1;
    const char *name = "Pooling";
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostInit(_))
        .WillRepeatedly(Return((0)));
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    EXPECT_NE(handle, nullptr);

    handle->recvName = "Poolings";
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostStop(_))
        .WillOnce(Return((1)));
    EXPECT_NE(acltdtStopChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtChannel02)
{
    uint32_t deviceId = 1;
    const char *name = "Pooling";
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    EXPECT_EQ(acltdtStopChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtChannel03)
{
    uint32_t deviceId = 1;
    const char *name = "";
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    std::map<std::string, acltdtChannelHandle *> aclChannleMap;
    aclChannleMap.erase(handle->name);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostDestroy())
        .WillOnce(Return((1)))
        .WillRepeatedly(Return(0));
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtSendTensor01)
{
    uint32_t deviceId = 0;
    const char *name = "tensor";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    aclError ret = acltdtSendTensor(handle, dataSet, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostPushData(_, _, _))
        .WillOnce(Return((1)));

    ret = acltdtSendTensor(handle, dataSet, -1);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);

    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtSendTensor02)
{
    uint32_t deviceId = 0;
    const char *name = "tensor";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostPushData(_, _, _))
        .WillOnce(Return((1)));
    aclError ret = acltdtSendTensor(handle, dataSet, -1);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtSendTensor03)
{
    uint32_t deviceId = 0;
    const char *name = "tensor";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostPushData(_, _, _))
        .WillOnce(Return(0));
    aclError ret = acltdtSendTensor(handle, dataSet, -1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAclTdtReceiveTensor01)
{
    uint32_t deviceId = 0;
    const char *name = "Pooling";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    handle->recvName = "Poolings";
    EXPECT_NE(acltdtReceiveTensor(handle, dataSet, -1), ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(acltdtReceiveTensor(handle, dataSet, 0), ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);

    deviceId = 1;
    name = "Pooling";
    dataSet = acltdtCreateDataset();
    handle = acltdtCreateChannel(deviceId, name);
    handle->recvName = "";
    EXPECT_NE(acltdtReceiveTensor(handle, dataSet, -1), ACL_ERROR_FAILURE);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestTensorDatasetSerializes01)
{
    uint32_t deviceId = 0;
    const char *name = "Pooling";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);

    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *item = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_FLOAT16, data, 1);

    tdt::DataItem dataItem1 = {tdt::TDT_END_OF_SEQUENCE, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    tdt::DataItem dataItem2 = {tdt::TDT_TENSOR, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    tdt::DataItem dataItem3 = {tdt::TDT_ABNORMAL, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    tdt::DataItem dataItem4 = {tdt::TDT_DATATYPE_MAX, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec1;
    itemVec1.push_back(dataItem1);
    std::vector<tdt::DataItem> itemVec2;
    itemVec2.push_back(dataItem2);
    std::vector<tdt::DataItem> itemVec3;
    itemVec3.push_back(dataItem3);
    std::vector<tdt::DataItem> itemVec4;
    itemVec3.push_back(dataItem4);

    aclError ret = TensorDatasetSerializes(dataSet, itemVec1);
    EXPECT_EQ(TensorDatasetSerializes(dataSet, itemVec2), ACL_SUCCESS);
    EXPECT_EQ(TensorDatasetSerializes(dataSet, itemVec3), ACL_SUCCESS);

    EXPECT_EQ(TensorDatasetDeserializes(itemVec1, dataSet), ACL_SUCCESS);
    EXPECT_NE(TensorDatasetDeserializes(itemVec2, dataSet), ACL_SUCCESS);
    EXPECT_NE(TensorDatasetDeserializes(itemVec3, dataSet), ACL_SUCCESS);
    EXPECT_NE(TensorDatasetDeserializes(itemVec4, dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(item), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestTensorDatasetSerializes02)
{
    acltdtDataset *dataSet = acltdtCreateDataset();
    tdt::DataItem dataItem = {tdt::TDT_END_OF_SEQUENCE, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec;
    itemVec.push_back(dataItem);

    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *item = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_FLOAT16, data, 1);
    aclError ret = acltdtAddDataItem(dataSet, item);
    EXPECT_EQ(TensorDatasetSerializes(dataSet, itemVec), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(item), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestGetTdtDataTypeByAclDataType)
{
    tdt::TdtDataType tdtDataType1 = tdt::TDT_TFRECORD;
    aclError ret = GetTdtDataTypeByAclDataType(ACL_TENSOR_DATA_TENSOR, tdtDataType1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    tdt::TdtDataType tdtDataType2 = tdt::TDT_END_OF_SEQUENCE;
    EXPECT_EQ(GetTdtDataTypeByAclDataType(ACL_TENSOR_DATA_END_OF_SEQUENCE, tdtDataType2), ACL_SUCCESS);
    tdt::TdtDataType tdtDataType3 = tdt::TDT_ABNORMAL;
    EXPECT_EQ(GetTdtDataTypeByAclDataType(ACL_TENSOR_DATA_ABNORMAL, tdtDataType3), ACL_SUCCESS);
    tdt::TdtDataType tdtDataType4 = tdt::TDT_DATATYPE_MAX;
    EXPECT_EQ(GetTdtDataTypeByAclDataType(ACL_TENSOR_DATA_UNDEFINED, tdtDataType4), ACL_ERROR_INVALID_PARAM);
}

TEST_F(UTEST_tensor_data_transfer, TestGetAclTypeByTdtDataType)
{
    tdt::TdtDataType tdtDataType1 = tdt::TDT_TENSOR;
    acltdtTensorType aclType1 = ACL_TENSOR_DATA_TENSOR;
    acltdtTensorType aclType2 = ACL_TENSOR_DATA_END_OF_SEQUENCE;
    tdt::TdtDataType tdtDataType2 = tdt::TDT_END_OF_SEQUENCE;
    EXPECT_EQ(GetAclTypeByTdtDataType(tdtDataType2, aclType2), ACL_SUCCESS);
    EXPECT_EQ(GetAclTypeByTdtDataType(tdtDataType1, aclType1), ACL_SUCCESS);
    tdt::TdtDataType tdtDataType3 = tdt::TDT_ABNORMAL;
    acltdtTensorType aclType3 = ACL_TENSOR_DATA_ABNORMAL;
    EXPECT_EQ(GetAclTypeByTdtDataType(tdtDataType3, aclType3), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestTensorDatasetDeserializes01)
{
    uint32_t deviceId = 0;
    const char *name = "Pooling";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);

    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *item = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_FLOAT16, data, 1);

    tdt::DataItem dataItem = {tdt::TDT_IMAGE_LABEL, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec;
    itemVec.push_back(dataItem);
    aclError ret = TensorDatasetSerializes(dataSet, itemVec);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = TensorDatasetDeserializes(itemVec, dataSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(item), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestTensorDatasetDeserializes02)
{
    uint32_t deviceId = 0;
    const char *name = "Pooling";
    acltdtDataset *dataSet = acltdtCreateDataset();
    tdt::DataItem dataItem = {tdt::TDT_TENSOR, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec;
    itemVec.push_back(dataItem);
    EXPECT_NE(TensorDatasetDeserializes(itemVec, dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestTensorDatasetDeserializes03)
{
    acltdtDataset *dataSet = acltdtCreateDataset();
    tdt::DataItem dataItem = {tdt::TDT_TENSOR, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec;
    itemVec.push_back(dataItem);

    const int64_t dims[] = {1, 3, 224, 224};
    void *data = (void*)0x1f;
    acltdtDataItem *item = acltdtCreateDataItem(ACL_TENSOR_DATA_TENSOR, dims, 4, ACL_FLOAT16, data, 1);
    aclError ret = acltdtAddDataItem(dataSet, item);

    std::vector<int64_t> dims2;
    dims2.push_back(1);
    EXPECT_NE(GetTensorShape(dataItem.tensorShape_, dims2), true);

    EXPECT_NE(TensorDatasetDeserializes(itemVec, dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataItem(item), ACL_SUCCESS);
}


TEST_F(UTEST_tensor_data_transfer, TestAcltdtReceiveTensor01)
{
    uint32_t deviceId = 0;
    const char *name = "Pooling";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    tdt::DataItem dataItem = {tdt::TDT_END_OF_SEQUENCE, "hidden", "3", "int64", 8, std::shared_ptr<void>(new int(0))};
    std::vector<tdt::DataItem> itemVec;
    itemVec.push_back(dataItem);
    EXPECT_NE(acltdtReceiveTensor(handle, dataSet, -1), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestAcltdtReceiveTensor02)
{
    uint32_t deviceId = 0;
    const char *name = "tensor";
    acltdtDataset *dataSet = acltdtCreateDataset();
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    handle->recvName = "test";

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostPopData(_, _))
        .WillOnce(Return((1)));
    aclError ret = acltdtReceiveTensor(handle, dataSet, -1);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    EXPECT_EQ(acltdtDestroyChannel(handle), ACL_SUCCESS);
    EXPECT_EQ(acltdtDestroyDataset(dataSet), ACL_SUCCESS);
}

TEST_F(UTEST_tensor_data_transfer, TestGetTensorShape)
{
    std::string dimStr1 = "[32,224]";
    std::vector<int64_t> dims;
    dims.push_back(1);
    bool ret = GetTensorShape(dimStr1, dims);
    EXPECT_EQ(ret, true);

    std::string dimStr2 = "[tensor]";
    ret = GetTensorShape(dimStr2, dims);
    EXPECT_EQ(ret, false);
}

TEST_F(UTEST_tensor_data_transfer, acltdtCreateChannel)
{
    uint32_t deviceId = 1;
    char *name = "name";

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), TdtHostInit(_))
        .WillOnce(Return((1)))
        .WillOnce(Return(0));
    acltdtChannelHandle *ret = acltdtCreateChannel(deviceId, name);
    EXPECT_EQ(ret, nullptr);
}

TEST_F(UTEST_tensor_data_transfer, acltdtCreateDataItemTest)
{
    acltdtTensorType tdtType = ACL_TENSOR_DATA_UNDEFINED;
    int64_t *dims = (int64_t *)0x11;
    size_t dimNum = 10;
    aclDataType dataType;
    void *data;
    size_t size;

    acltdtDataItem *ret = acltdtCreateDataItem(tdtType, dims, dimNum, dataType, data, size);
    EXPECT_EQ(ret, nullptr);

    dimNum = 129;
    ret = acltdtCreateDataItem(tdtType, dims, dimNum, dataType, data, size);
    EXPECT_EQ(ret, nullptr);
}

TEST_F(UTEST_tensor_data_transfer, acltdtReceiveTensorTest)
{
    uint32_t deviceId = 1;
    char *name = "name";
    acltdtChannelHandle *handle = acltdtCreateChannel(deviceId, name);
    handle->recvName = "tensor";
    acltdtDataset *dataset = acltdtCreateDataset();
    int32_t timeout = -1;
    acltdtDestroyChannel(handle);
    acltdtDestroyDataset(dataset);
}

TEST_F(UTEST_tensor_data_transfer, acltdtGetDimsFromItemTest)
{
    acltdtTensorType tdtType = ACL_TENSOR_DATA_UNDEFINED;
    int64_t *dims = nullptr;
    size_t dimNum = 10;
    aclDataType dataType;
    void *data;
    size_t size;
    acltdtDataItem *dataItem = (acltdtDataItem *)0x11;
    acltdtGetDimsFromItem(dataItem, dims, dimNum);
}

TEST_F(UTEST_tensor_data_transfer, TensorDatasetSerializesV2)
{
    acltdtDataset *dataset = acltdtCreateDataset();
    EXPECT_NE(dataset, nullptr);
    acltdtDataItem item;
    item.tdtType = ACL_TENSOR_DATA_TENSOR;
    dataset->blobs.push_back(&item);
    std::vector<aclTdtDataItemInfo> itemVec;
    auto ret = TensorDatasetSerializesV2(dataset, itemVec);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(itemVec.size(), 1);
    acltdtDestroyDataset(dataset);
}

TEST_F(UTEST_tensor_data_transfer, TensorDatasetDeserializesV2)
{
    acltdtDataset *dataset = acltdtCreateDataset();
    EXPECT_NE(dataset, nullptr);
    std::vector<aclTdtDataItemInfo> itemVec;
    aclTdtDataItemInfo info;
    info.ctrlInfo.dataType = 1;
    itemVec.push_back(info);
    auto ret = TensorDatasetDeserializesV2(itemVec, dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
    acltdtDestroyDataset(dataset);
}

TEST_F(UTEST_tensor_data_transfer, acltdtSendTensorV2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return((RT_ERROR_NONE)));
    acltdtChannelHandle *handle = acltdtCreateChannelWithDepth(0, "test", 3);
    EXPECT_NE(handle, nullptr);
    acltdtDataset *dataset = acltdtCreateDataset();
    EXPECT_NE(dataset, nullptr);
    int32_t timeout = 300;
    auto ret = acltdtSendTensorV2(handle, dataset, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
    acltdtDestroyChannel(handle);
    acltdtDestroyDataset(dataset);

}

TEST_F(UTEST_tensor_data_transfer, acltdtReceiveTensorV2)
{
    acltdtChannelHandle *handle = acltdtCreateChannelWithDepth(0, "TF_RECEIVE_1", 3);
    EXPECT_NE(handle, nullptr);
    acltdtDataset *dataset = acltdtCreateDataset();
    EXPECT_NE(dataset, nullptr);
    int32_t timeout = 300;
    auto ret = acltdtReceiveTensorV2(handle, dataset, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
    acltdtDestroyChannel(handle);
    acltdtDestroyDataset(dataset);
}

