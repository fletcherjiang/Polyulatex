#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "acl/acl.h"
#include "model/aipp_param_check.h"
#include "framework/executor/ge_executor.h"
#include "common/ge_types.h"
#include "acl_stub.h"


using namespace testing;
using namespace std;

class UTEST_ACL_Model : public testing::Test
{
    public:
        UTEST_ACL_Model(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {
            Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
        }
};

ge::Status GetModelDescInfo_Invoke(uint32_t modelId, std::vector<ge::TensorDesc>& inputDesc,
                                        std::vector<ge::TensorDesc>& outputDesc, bool new_model_desc)
{
    ge::TensorDesc desc1;
    inputDesc.push_back(desc1);
    outputDesc.push_back(desc1);
    return ge::SUCCESS;
}

ge::Status GetModelDescInfo_Invoke2(uint32_t modelId, std::vector<ge::TensorDesc>& inputDesc,
                                   std::vector<ge::TensorDesc>& outputDesc, bool new_model_desc)
{
    ge::TensorDesc desc1;
    ge::TensorDesc desc2;
    inputDesc.push_back(desc1);
    inputDesc.push_back(desc2);
    outputDesc.push_back(desc1);
    return ge::SUCCESS;
}

ge::Status GetDynamicBatchInfo_Invoke(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                      int32_t &dynamic_type)
{
    dynamic_type = 2;
    batch_info.push_back({224, 224});
    batch_info.push_back({600, 600});
    return ge::SUCCESS;
}

ge::Status GetDynamicBatchInfo_Invoke3(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                       int32_t &dynamic_type)
{
    dynamic_type = 3;
    batch_info.push_back({224, 224});
    batch_info.push_back({600, 600});
    return ge::SUCCESS;
}

ge::Status ExecModelInvoke(uint32_t model_id, void *stream, const ge::RunModelData &run_input_data,
                            const std::vector<ge::GeTensorDesc> &input_desc, ge::RunModelData &run_output_data,
                            std::vector<ge::GeTensorDesc> &output_desc, bool async_mode)
{
    ge::GeTensorDesc geDescTmp;
    output_desc.push_back(geDescTmp);
    output_desc.push_back(geDescTmp);
    return SUCCESS;
}

ge::Status GetDynamicBatchInfo_Invoke4(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                       int32_t &dynamic_type)
{
    dynamic_type = 3;
    batch_info.push_back({224, 224});
    batch_info.push_back({600, 600, 600});
    return ge::SUCCESS;
}

ge::Status GetDynamicBatchInfo_Invoke5(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
    int32_t &dynamic_type)
{
  return ge::SUCCESS;
}

ge::Status GetUserDesignateShapeOrderInvoke(uint32_t model_id, vector<string> &user_designate_shape_order) {
    user_designate_shape_order.emplace_back("resnet50");
    user_designate_shape_order.emplace_back("resnet50");
    return ge::SUCCESS;
}

ge::Status GetAippTypeFailInvoke(uint32_t model_id, uint32_t index,
    ge::InputAippType &type, size_t &aippindex) {
    type = ge::DATA_WITHOUT_AIPP;
    aippindex = 0xFFFFFFFF;
    return ge::FAILED;
}

ge::Status GetAippTypeSuccessInvoke(uint32_t model_id, uint32_t index,
    ge::InputAippType &type, size_t &aippindex) {
    type = ge::DYNAMIC_AIPP_NODE;
    return ge::SUCCESS;
}

ge::Status GetAippTypeNoAippInvoke(uint32_t model_id, uint32_t index,
    ge::InputAippType &type, size_t &aippindex) {
    type = ge::DATA_WITHOUT_AIPP;
    aippindex = 0xFFFFFFFF;
    return ge::SUCCESS;
}

ge::Status GetAippTypeStaticAippInvoke(uint32_t model_id, uint32_t index, ge::InputAippType &type, size_t &aippindex) {
    type = ge::DATA_WITH_STATIC_AIPP;
    aippindex = 0xFFFFFFFF;
    return ge::SUCCESS;
}

ge::Status GetOpAttr_Invoke(uint32_t model_id, const std::string &op_name, const std::string &attr_name, std::string &attr_value)
{
    return FAILED;
}

ge::Status GetOpAttr_Invoke_1(uint32_t model_id, const std::string &op_name, const std::string &attr_name, std::string &attr_value)
{
    attr_value = "";
    return SUCCESS;
}

ge::Status GetOpAttr_Invoke_2(uint32_t model_id, const std::string &op_name, const std::string &attr_name, std::string &attr_value)
{
    attr_value = "attr_finded";
    return SUCCESS;
}

TEST_F(UTEST_ACL_Model, aclmdlGetOpAttr)
{
    aclmdlDesc *mdlDesc = aclmdlCreateDesc();
    aclmdlDesc *mdlDescNullptr = nullptr;
    const char *opName = "anyOp";
    const char *opNameNullptr = nullptr;
    const char *attr = "_datadump_original_op_names";
    const char *attrNullptr = nullptr;

    const char *attrNotSupported = "anyAttr";
    const char *result = aclmdlGetOpAttr(mdlDesc, opName, attrNotSupported);
    EXPECT_EQ(result, nullptr);

    const char *resultNullptr_1 = aclmdlGetOpAttr(mdlDescNullptr, opName, attr);
    EXPECT_EQ(resultNullptr_1, nullptr);

    const char *resultNullptr_2 = aclmdlGetOpAttr(mdlDesc, opNameNullptr, attr);
    EXPECT_EQ(resultNullptr_2, nullptr);

    const char *resultNullptr_3 = aclmdlGetOpAttr(mdlDesc, opName, attrNullptr);
    EXPECT_EQ(resultNullptr_3, nullptr);

    const char *opName2 = "anyOp2";
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetOpAttr(_,_,_,_))
        .WillOnce(Invoke(GetOpAttr_Invoke));
    const char *resultGeFailed  = aclmdlGetOpAttr(mdlDesc, opName, attr);
    EXPECT_EQ(resultGeFailed, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetOpAttr(_,_,_,_))
        .WillOnce(Invoke(GetOpAttr_Invoke_1));
    const char *resultEmptyStr = aclmdlGetOpAttr(mdlDesc, opName, attr);
    EXPECT_EQ(string(resultEmptyStr), "");

    const char *opName3 = "anyOp3";
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetOpAttr(_,_,_,_))
        .WillOnce(Invoke(GetOpAttr_Invoke_2));
    const char *resultValue = aclmdlGetOpAttr(mdlDesc, opName3, attr);
    std::cout << string(resultValue) << "****************"<<std::endl;;
    EXPECT_EQ(string(resultValue), "attr_finded");

    const char *resultGeSuccess = aclmdlGetOpAttr(mdlDesc, opName3, attr);
    EXPECT_EQ(string(resultGeSuccess), "attr_finded");

    aclmdlDestroyDesc(mdlDesc);
}

TEST_F(UTEST_ACL_Model, desc)
{
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    size_t size = aclmdlGetNumInputs(nullptr);
    EXPECT_EQ(size, 0);
    size = aclmdlGetNumInputs(desc);
    EXPECT_EQ(size, 0);

    size = aclmdlGetNumOutputs(nullptr);
    EXPECT_EQ(size, 0);
    size = aclmdlGetNumOutputs(desc);
    EXPECT_EQ(size, 0);

    aclError ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDesc(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetDesc)
{
    aclError ret = aclmdlGetDesc(nullptr, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_,_,_,_))
        .WillRepeatedly(Invoke(GetModelDescInfo_Invoke));
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_,_,_))
         .WillOnce(Invoke(GetDynamicBatchInfo_Invoke3));
    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillOnce(Invoke(GetDynamicBatchInfo_Invoke4));
    ret = aclmdlGetDesc(desc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetDesc_Fail)
{
    aclError ret = aclmdlGetDesc(nullptr, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillOnce(Return((PARAM_INVALID)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    ret = aclmdlGetDesc(desc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputSizeByIndex)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t size = aclmdlGetInputSizeByIndex(nullptr, 0);
    EXPECT_EQ(size, 0);

    size = aclmdlGetInputSizeByIndex(desc, 0);
    EXPECT_EQ(size, 1);

    size = aclmdlGetOutputSizeByIndex(nullptr, 0);
    EXPECT_EQ(size, 0);

    size = aclmdlGetOutputSizeByIndex(desc, 0);
    EXPECT_EQ(size, 1);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, Dataset)
{
    aclmdlDataset *dataset = aclmdlCreateDataset();
    EXPECT_NE(dataset, nullptr);

    aclError ret = aclmdlAddDatasetBuffer(dataset, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlAddDatasetBuffer(dataset, (aclDataBuffer *)0x01);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t size = aclmdlGetDatasetNumBuffers(nullptr);
    EXPECT_EQ(size, 0);

    size = aclmdlGetDatasetNumBuffers(dataset);
    EXPECT_EQ(size, 1);

    aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(dataset, 1);
    EXPECT_EQ(dataBuffer, nullptr);

    dataBuffer = aclmdlGetDatasetBuffer(dataset, 0);
    EXPECT_NE(dataBuffer, nullptr);

    ret = aclmdlAddDatasetBuffer(nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDataset(nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromFile)
{
    aclError ret = aclmdlLoadFromFile(nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    uint32_t modelId = 1;
    const char *modelPath = "/";

    ret = aclmdlLoadFromFile(modelPath, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelFromData(_,_,_,_,_,_))
        .WillOnce(Return((ACL_ERROR_GE_EXEC_LOAD_MODEL_REPEATED)));
    ret = aclmdlLoadFromFile(modelPath, &modelId);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadDataFromFile(_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromFile(modelPath, &modelId);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromFileWithMem)
{
    aclError ret = aclmdlLoadFromFileWithMem(nullptr, nullptr, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    uint32_t modelId = 1;
    const char *modelPath = "/";

    ret = aclmdlLoadFromFileWithMem(modelPath, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelFromData(_,_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromFileWithMem(modelPath, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadDataFromFile(_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromFileWithMem(modelPath, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromMem)
{
    aclError ret = aclmdlLoadFromMem(nullptr, 0, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    void *model = (void *)0x01;
    uint32_t modelId = 1;
    ret = aclmdlLoadFromMem(model, 0, &modelId);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlLoadFromMem(model, 1, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelFromData(_,_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromMem(model, 1, &modelId);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromMemWithMem)
{
    aclError ret = aclmdlLoadFromMemWithMem(nullptr, 0, nullptr, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    void *model = (void *)0x01;
    uint32_t modelId = 1;
    ret = aclmdlLoadFromMemWithMem(model, 0, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlLoadFromMemWithMem(model, 1, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelFromData(_,_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromMemWithMem(model, 1, &modelId, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromFileWithQ)
{
    aclError ret = aclmdlLoadFromFileWithQ(nullptr, nullptr, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    const char *modelPath = "/";
    uint32_t modelId = 1;

    uint32_t *input = new(std::nothrow) uint32_t[1];
    uint32_t *output = new(std::nothrow) uint32_t[1];
    ret = aclmdlLoadFromFileWithQ(modelPath, &modelId, input, 1, output, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelWithQ(_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromFileWithQ(modelPath, &modelId, input, 1, output, 1);
    EXPECT_NE(ret, ACL_SUCCESS);
    delete []input;
    delete []output;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadDataFromFile(_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlLoadFromFileWithQ(modelPath, &modelId, (uint32_t*)0x1, 1, (uint32_t*)0x2, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlLoadFromFileWithQ(modelPath, &modelId, (uint32_t*)0x1, 0, (uint32_t*)0x2, 1);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadFromMemWithQ)
{
    aclError ret = aclmdlLoadFromMemWithQ(nullptr, 0, nullptr, nullptr, 0, nullptr, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    uint32_t *input = new(std::nothrow) uint32_t[1];
    uint32_t *output = new(std::nothrow) uint32_t[1];
    const char *modelPath = "/";
    uint32_t modelId = 1;
    ret = aclmdlLoadFromMemWithQ(modelPath, 1, &modelId, input, 1, output, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelWithQ(_,_,_,_))
        .WillRepeatedly(Return(PARAM_INVALID));
    ret = aclmdlLoadFromMemWithQ(modelPath, 1, &modelId, input, 1, output, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    delete []input;
    delete []output;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), LoadModelWithQ(_,_,_,_))
        .WillRepeatedly(Return(PARAM_INVALID));
    ret = aclmdlLoadFromMemWithQ(modelPath, 0, &modelId, input, 1, output, 1);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlUnload)
{
    aclError ret = aclmdlUnload(0);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), UnloadModel(_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlUnload(0);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlQuerySize)
{
    const char *fileName = "/";
    size_t memSize;
    size_t weightSize;

    aclError ret = aclmdlQuerySize(nullptr, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlQuerySize(fileName, &memSize, &weightSize);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetMemAndWeightSize(_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlQuerySize(fileName, &memSize, &weightSize);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlQuerySizeFromMem)
{
    void *model = (void *)0x01;
    size_t memSize;
    size_t weightSize;

    aclError ret = aclmdlQuerySizeFromMem(nullptr, 1,  nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlQuerySizeFromMem(model, 1, &memSize, &weightSize);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetMemAndWeightSize(_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlQuerySizeFromMem(model, 1, &memSize, &weightSize);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlExecute)
{
    aclmdlDataset *dataset = aclmdlCreateDataset();
    EXPECT_NE(dataset, nullptr);

    aclDataBuffer *dataBuffer = (aclDataBuffer *)malloc(100);//aclCreateDataBuffer((void *)0x01, 100);
    aclError ret = aclmdlAddDatasetBuffer(dataset, dataBuffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecute(1, dataset, dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecute(1, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ExecModel(_,_,_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlExecute(1, dataset, dataset);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(dataBuffer);
    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlExecuteAsync)
{
    aclmdlDataset *dataset = aclmdlCreateDataset();
    EXPECT_NE(dataset, nullptr);

    aclDataBuffer *dataBuffer = (aclDataBuffer *)malloc(100);
    aclError ret = aclmdlAddDatasetBuffer(dataset, dataBuffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecuteAsync(1, dataset, dataset, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecuteAsync(1, nullptr, nullptr, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ExecModel(_,_,_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlExecuteAsync(1, dataset, dataset, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    free(dataBuffer);
    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlSetDynamicBatchSize)
{
    aclError ret = aclmdlSetDynamicBatchSize(1, (aclmdlDataset*)0x1, 0, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetDynamicBatchSize(1, dataset, 0, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclDataBuffer *buffer2 = aclCreateDataBuffer(nullptr, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetDynamicBatchSize(1, dataset, 1, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDynamicBatchSize(_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlSetDynamicBatchSize(1, dataset, 0, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclDestroyDataBuffer(buffer);
    aclDestroyDataBuffer(buffer2);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclGetDataBufferSizeV2)
{
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    EXPECT_EQ(aclGetDataBufferSizeV2(nullptr), 0);
    EXPECT_EQ(aclGetDataBufferSizeV2(buffer), 1);
    aclDestroyDataBuffer(buffer);
}

TEST_F(UTEST_ACL_Model, aclUpdateDataBuffer)
{
    EXPECT_EQ(aclUpdateDataBuffer(nullptr, nullptr, 1), ACL_ERROR_INVALID_PARAM);
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    EXPECT_EQ(aclUpdateDataBuffer(buffer, nullptr, 1), ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
}

TEST_F(UTEST_ACL_Model, aclmdlSetDynamicHWSize)
{
    aclError ret = aclmdlSetDynamicHWSize(1, (aclmdlDataset*)0x1, 0, 0, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetDynamicHWSize(1, dataset, 0, 1, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclDataBuffer *buffer2 = aclCreateDataBuffer(nullptr, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetDynamicHWSize(1, dataset, 1, 1, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDynamicImageSize(_,_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlSetDynamicHWSize(1, dataset, 0, 1, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclDestroyDataBuffer(buffer);
    aclDestroyDataBuffer(buffer2);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputDynamicDims01)
{
    aclError ret = aclmdlSetInputDynamicDims(1, (aclmdlDataset*)0x1, 0, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDynamicDims(_,_,_,_))
        .Times(2)
        .WillOnce(Return((SUCCESS)));
    aclmdlIODims dims[1];
    dims[0].dimCount = 1;
    dims[0].dims[0] = 1;
    ret = aclmdlSetInputDynamicDims(1, dataset, 0, dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclDataBuffer *buffer2 = aclCreateDataBuffer(nullptr, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetInputDynamicDims(1, dataset, 0, dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDynamicDims(_,_,_,_))
        .WillOnce(Return((PARAM_INVALID)));
    ret = aclmdlSetInputDynamicDims(1, dataset, 0, dims);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclDestroyDataBuffer(buffer);
    aclDestroyDataBuffer(buffer2);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputDynamicDims02)
{
    aclError ret = aclmdlSetInputDynamicDims(1, (aclmdlDataset*)0x1, 0, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlIODims dims[1];
    dims[0].dimCount = 1;
    dims[0].dims[0] = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurDynamicDims(_,_,_))
        .WillOnce(Return((FAILED)));
    ret = aclmdlSetInputDynamicDims(1, dataset, 0, dims);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputNameByIndex)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    const char *res = aclmdlGetInputNameByIndex(nullptr, 0);
    EXPECT_STREQ(res, "");

    res = aclmdlGetInputNameByIndex(desc, 3);
    EXPECT_STREQ(res, "");

    res = aclmdlGetInputNameByIndex(desc, 0);
    EXPECT_STRNE(res, "");

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputNameByIndex)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    const char *res = aclmdlGetOutputNameByIndex(nullptr, 0);
    EXPECT_STREQ(res, "");

    res = aclmdlGetOutputNameByIndex(desc, 3);
    EXPECT_STREQ(res, "");

    res = aclmdlGetOutputNameByIndex(desc, 0);
    EXPECT_STRNE(res, "");

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputFormat)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclFormat formatVal = aclmdlGetInputFormat(nullptr, 0);
    EXPECT_EQ(formatVal, ACL_FORMAT_UNDEFINED);

    formatVal = aclmdlGetInputFormat(desc, 3);
    EXPECT_EQ(formatVal, ACL_FORMAT_UNDEFINED);

    formatVal = aclmdlGetInputFormat(desc, 0);
    EXPECT_EQ(formatVal, ACL_FORMAT_NCHW);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputFormat)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclFormat formatVal = aclmdlGetOutputFormat(nullptr, 0);
    EXPECT_EQ(formatVal, ACL_FORMAT_UNDEFINED);

    formatVal = aclmdlGetOutputFormat(desc, 3);
    EXPECT_EQ(formatVal, ACL_FORMAT_UNDEFINED);

    formatVal = aclmdlGetOutputFormat(desc, 0);
    EXPECT_EQ(formatVal, ACL_FORMAT_NCHW);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputDataType)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclDataType typeVal = aclmdlGetInputDataType(nullptr, 0);
    EXPECT_EQ(typeVal, ACL_DT_UNDEFINED);

    typeVal = aclmdlGetInputDataType(desc, 3);
    EXPECT_EQ(typeVal, ACL_DT_UNDEFINED);

    typeVal = aclmdlGetInputDataType(desc, 0);
    EXPECT_EQ(typeVal, ACL_FLOAT);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputDataType)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclDataType typeVal = aclmdlGetOutputDataType(nullptr, 0);
    EXPECT_EQ(typeVal, ACL_DT_UNDEFINED);

    typeVal = aclmdlGetOutputDataType(desc, 3);
    EXPECT_EQ(typeVal, ACL_DT_UNDEFINED);

    typeVal = aclmdlGetOutputDataType(desc, 0);
    EXPECT_EQ(typeVal, ACL_FLOAT);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}


TEST_F(UTEST_ACL_Model, aclmdlGetInputIndexByName)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t idx = 0;
    ret = aclmdlGetInputIndexByName(desc, "resnet50", &idx);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputIndexByName(desc, "resnet18", &idx);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputIndexByName)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t idx = 0;
    ret = aclmdlGetOutputIndexByName(desc, "resnet50", &idx);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetOutputIndexByName(desc, "resnet18", &idx);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetAippType)
{
    uint32_t modelId = 0;
    size_t index = 0;
    aclmdlInputAippType type;
    size_t aippIndex;
    aclError ret = aclmdlGetAippType(modelId, index, &type, &aippIndex);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetAippType1)
{
    uint32_t modelId = 0;
    size_t index = 0;
    aclmdlInputAippType type;
    size_t aippIndex;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke((GetAippTypeFailInvoke)));
    aclError ret = aclmdlGetAippType(modelId, index, &type, &aippIndex);
    EXPECT_EQ(ret, ACL_ERROR_GE_FAILURE);
}

TEST_F(UTEST_ACL_Model, aclmdlGetDynamicBatch)
{
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlBatch batch;
    ret = aclmdlGetDynamicBatch(desc, &batch);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetDynamicHW)
{
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    aclmdlHW hw;
    aclError ret = aclmdlGetDynamicHW(nullptr, -1, &hw);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlGetDynamicHW(desc, -1, &hw);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputDynamicGearCount)
{
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t dimsCount;
    ret = aclmdlGetInputDynamicGearCount(desc, -1, &dimsCount);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke5)));
    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputDynamicGearCount(desc, -1, &dimsCount);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputDynamicGearCount(desc, 1, &dimsCount);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputDynamicDims)
{
    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke3)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetUserDesignateShapeOrder(_,_))
        .WillRepeatedly(Invoke((GetUserDesignateShapeOrderInvoke)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke2)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t dimsCount = 2;
    aclmdlIODims dims[2];
    ret = aclmdlGetInputDynamicDims(desc, 2, dims, dimsCount);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlGetInputDynamicDims(desc, -1, dims, dimsCount);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlGetInputDynamicDims(desc, 1, dims, dimsCount);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputDims)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlIODims dims;
    ret = aclmdlGetInputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputDims(desc, 1, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputDimsV2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlIODims dims;
    ret = aclmdlGetInputDimsV2(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputDimsV2(desc, 1, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputDims01)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
    EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclError ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlIODims dims;
    ret = aclmdlGetOutputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetOutputDims(desc, 1, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlGetOutputDims02)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));
    aclmdlDesc* desc = aclmdlCreateDesc();

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
    aclmdlGetDesc(desc, 1);
    aclmdlDestroyDesc(desc);
}

ge::Status GetDynamicBatchInfo_Invoke1(uint32_t model_id,
                                      std::vector<std::vector<int64_t>> &batch_info, int32_t &dynamic_type)
{
    dynamic_type = 2;
    batch_info.push_back({224, 224});
    batch_info.push_back({600, 600});
    return ge::SUCCESS;
}

ge::Status GetDynamicBatchInfo_Invoke2(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info, int32_t &dynamic_type)
{
    dynamic_type = 1;
    batch_info.push_back({224});
    batch_info.push_back({600});
    return ge::SUCCESS;
}

ge::Status GetCurShape_Invoke(const uint32_t model_id, std::vector<int64_t> &batch_info,
                              int32_t &dynamic_type)
{
    dynamic_type = 1;
    batch_info.push_back(224);
    return ge::SUCCESS;
}

ge::Status GetCurShape_Invoke1(const uint32_t model_id, std::vector<int64_t> &batch_info,
                               int32_t &dynamic_type)
{
    return ge::SUCCESS;
}

ge::Status GetCurShape_Invoke2(const uint32_t model_id, std::vector<int64_t> &batch_info,
                               int32_t &dynamic_type)
{
    batch_info.push_back(224);
    batch_info.push_back(224);
    batch_info.push_back(224);
    return ge::SUCCESS;
}

ge::Status GetCurShape_Invoke3(const uint32_t model_id, std::vector<int64_t> &batch_info,
                               int32_t &dynamic_type)
{
    return ge::FAILED;
}

ge::Status GetCurShape_Invoke4(const uint32_t model_id, std::vector<int64_t> &batch_info,
                               int32_t &dynamic_type)
{
    batch_info.push_back(224);
    batch_info.push_back(224);
    return ge::SUCCESS;
}

ge::Status GetModelAttr_Invoke(uint32_t model_id,
                               std::vector<std::string> &dynamic_output_shape_info)
{
    dynamic_output_shape_info.push_back({"1:0:1,3,224,224"});
    return ge::SUCCESS;
}

ge::Status GetModelAttr_Invoke1(uint32_t model_id,
                               std::vector<std::string> &dynamic_output_shape_info)
{
    dynamic_output_shape_info.push_back({"-1:0:1,3,224,224"});
    return ge::SUCCESS;
}


TEST_F(UTEST_ACL_Model, aclmdlGetCurOutputDims)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    aclmdlDesc* desc = aclmdlCreateDesc();
     EXPECT_NE(desc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke)));
     aclError ret = aclmdlGetDesc(desc, 1);
     EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke((GetCurShape_Invoke4)));

    aclmdlIODims dims;
    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetCurOutputDims(desc, 1, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke2)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke((GetCurShape_Invoke)));

    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke((GetCurShape_Invoke1)));

    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke((GetCurShape_Invoke2)));
    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke((GetCurShape_Invoke3)));
    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke(GetModelDescInfo_Invoke));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke(GetDynamicBatchInfo_Invoke1));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_, _,_))
        .WillRepeatedly(Invoke(GetCurShape_Invoke4));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelAttr(_, _))
        .WillRepeatedly(Invoke(GetModelAttr_Invoke));
    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetCurOutputDims(desc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS); //modify
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelAttr(_, _))
        .WillRepeatedly(Invoke((GetModelAttr_Invoke1)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_, _,_))
        .WillRepeatedly(Invoke((GetDynamicBatchInfo_Invoke2)));
    ret = aclmdlGetDesc(desc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret = aclmdlSetAIPPInputFormat(aippmdlAipp, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    ret = aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPCscParams(aippDynamicSet, 1, 256, 443, 0, 256, -86, -178, 256, 0, 350, 0, 0, 0, 0, 128, 128);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPCscParams(aippDynamicSet, 0, 256, 443, 0, 256, -86, -178, 256, 0, 350, 0, 0, 0, 0, 128, 128);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPRbuvSwapSwitch(aippDynamicSet, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPDtcPixelMean(aippDynamicSet, 0, 0, 0, 0, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPDtcPixelMin(aippDynamicSet, 0, 0, 0, 0, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPPixelVarReci(aippDynamicSet, 1, 1, 1, 0, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 0, 1, 1, 1, 1, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 224, 224, 16, 224, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPAxSwapSwitch(aippDynamicSet, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPSrcImageSize(aippDynamicSet, 224, 224);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 0, 0, 0, 1, 1, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 0, 0, 1, 1, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPPaddingParams(aippDynamicSet, 0, 0, 0, 0, 0, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPPaddingParams(aippDynamicSet, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_SetDynamicAippData_fail)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret = aclmdlSetAIPPInputFormat(aippmdlAipp, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    ret = aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPCscParams(aippDynamicSet, 1, 256, 443, 0, 256, -86, -178, 256, 0, 350, 0, 0, 0, 0, 128, 128);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetAIPPSrcImageSize(aippDynamicSet, 224, 224);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke(GetAippTypeSuccessInvoke));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDynamicAippData(_, _,_,_, _))
        .WillRepeatedly(Return(FAILED));

    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetAIPPByInputIndex_fail1)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret = aclmdlSetAIPPInputFormat(aippmdlAipp, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlSetAIPPByInputIndex(1, dataset, 6, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetAIPPByInputIndex_fail2)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret = aclmdlSetAIPPInputFormat(aippmdlAipp, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Return(ACL_ERROR_FAILURE));
    ret = aclmdlSetAIPPByInputIndex(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetAIPPByInputIndex_fail3)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret = aclmdlSetAIPPInputFormat(aippmdlAipp, ACL_YUV420SP_U8);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke((GetAippTypeFailInvoke)));

    ret = aclmdlSetAIPPByInputIndex(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_GE_FAILURE);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetAIPPByInputIndex_SUCCESS)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    auto ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke(GetModelDescInfo_Invoke2));
    ret = aclmdlSetAIPPByInputIndex(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

aclError aclmdlGetInputIndexByName_Invoke(const aclmdlDesc *modelDesc, const char *name, size_t *index)
{
    *index = 0;
    return ACL_SUCCESS;
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_Fail)
{
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    aclError ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_,_,_,_))
        .WillOnce(Return(ACL_ERROR_INVALID_PARAM))
        .WillRepeatedly(Return(ACL_SUCCESS));
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_GE_FAILURE);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_Fail1)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    auto ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Return(FAILED));

    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_Fail2)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    auto ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillOnce(Invoke(GetAippTypeFailInvoke))
        .WillRepeatedly(Invoke(GetAippTypeStaticAippInvoke));

    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_Fail3)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    auto ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke(GetAippTypeNoAippInvoke));
    ret = aclmdlSetInputAIPP(1, dataset, 6, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlGetFirstAippInfo)
{
    aclAippInfo aippInfo;
    aclError ret = aclmdlGetFirstAippInfo(1, 0, &aippInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Model, AippParamsCheck)
{
    aclmdlAIPP *aippmdlAipp = aclmdlCreateAIPP(0);
    aclError ret;
    uint32_t batchNumber = 2;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);

    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    //InputFormat not setted
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, "Ascend310");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //aipp not support Ascend910
    std::string socVersion = "Ascend910";
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_ARGB8888_U8);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //Ascend310 not support YUYV_U8
    socVersion = "Ascend310";
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUYV_U8);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //Ascend310 not support RAW10,for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW10);
    (void)GetSrcImageSize(aippDynamicSet);

    //Ascend310 not support RAW12,for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW12);
    (void)GetSrcImageSize(aippDynamicSet);

    //Ascend310 not support RAW16,for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW16);
    (void)GetSrcImageSize(aippDynamicSet);

    //Ascend310 not support RAW24,for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW24);
    (void)GetSrcImageSize(aippDynamicSet);

    //Ascend310 not support UV422SP_U8,for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV422SP_U8);
    (void)GetSrcImageSize(aippDynamicSet);

    //for cover GetSrcImageSize
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_XRGB8888_U8);
    (void)GetSrcImageSize(aippDynamicSet);

    //Ascend310 not support scf
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 0, 1, 1, 1, 1, 0);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //Ascend610 not support RAW24
    socVersion = "Ascend610";
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW24);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //ES not support RAW24
    socVersion = "Hi3796CV300ES";
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RAW24);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //YUV400_U8 not support csc
    ret = aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV400_U8);
    ret = aclmdlSetAIPPCscParams(aippDynamicSet, 1, 256, 443, 0, 256, -86, -178, 256, 0, 350, 0, 0, 0, 0, 128, 128);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //YUV420SP_U8,src_image_h and src_image_w must be even
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    (void)GetSrcImageSize(aippDynamicSet);
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 223, 223);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //ES,YUV420SP_U8, src_image_w must be multiples of 16
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 18, 224);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //ES,ARGB8888_U8,src_image_w*4 must be multiples of 16
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_ARGB8888_U8);
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 5, 224);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //ES,RGB888_U8, src_image_w*3 must be multiples of 16
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_RGB888_U8);
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 6, 224);
    (void)GetSrcImageSize(aippDynamicSet);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //enable scf, disable crop,scfInputSizeW==srcImageSizeW,scfInputSizeH==srcImageSizeH
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 224, 224);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 210, 210, 1, 1, 0);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //enable scf，crop,scfInputSizeW==cropSizeW,scfInputSizeH==cropSizeH
    (void)aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 224, 224);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 0, 0, 220, 220, 0);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 210, 210, 1, 1, 0);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the first batch, enable crop,scf,padding, aippOutputW and aippOutputH is 120,120
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 220, 220, 100, 100, 0);
    ret = aclmdlSetAIPPPaddingParams(aippDynamicSet, 1, 10, 10, 10, 10, 0);

    //the second batch, enable crop, cropStartPosW + cropSizeW > srcImageSizeW
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 5, 1, 221, 221, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable crop, cropStartPosH + cropSizeH > srcImageSizeH
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 1, 5, 221, 221, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    //ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable crop, cropStartPosW，cropStartPosH，cropSizeW，cropSizeH must be even
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 1, 1, 221, 221, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    //ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfInputSizeW is within [16,4096]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 10, 220, 100, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    //ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfInputSizeH is within [16,4096]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 20, 10, 100, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfOutputSizeW is within [16,1920]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 20, 20, 10, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfOutputSizeH is within [16,4096]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 20, 20, 100, 10, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfOutputSizeW/scfInputSizeW is within [1/16,16]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 20, 20, 1000, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, enable scf, scfOutputSizeH/scfInputSizeH在[1/16,16]
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 20, 20, 100, 1000, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the second batch, disable crop, enable scf，scfInputSizeW==srcImageSizeW，scfInputSizeH==srcImageSizeH
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 0, 0, 0, 20, 20, 1);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 10, 20, 100, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //the first batch aippOutputH and aippOutputW is 120,120，祎he second batch aippOutputH and aippOutputW is100,100
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 0, 0, 0, 20, 20, 1);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 224, 224, 100, 100, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    //aipp output size by ACL is 120,120, aipp output size in the model is 100,100
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 1, 224, 224, 120, 120, 1);
    ret = AippParamsCheck(aippDynamicSet, socVersion);
    EXPECT_EQ(ret, ACL_SUCCESS);

    //after padding,mini/1951:aippOutputW <= 1080;ES/CS:aippOutputW <= 4096
    (void)aclmdlSetAIPPSrcImageSize(aippDynamicSet, 4096, 4096);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 0, 5, 1, 221, 221, 0);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 0, 5, 1, 221, 221, 1);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 0, 224, 224, 120, 120, 0);
    ret = aclmdlSetAIPPScfParams(aippDynamicSet, 0, 224, 224, 120, 120, 1);
    ret = aclmdlSetAIPPPaddingParams(aippDynamicSet, 1, 10, 10, 10, 10, 0);
    ret = aclmdlSetAIPPPaddingParams(aippDynamicSet, 1, 10, 10, 10, 10, 1);
    ret = AippParamsCheck(aippDynamicSet, "Ascend310");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = AippParamsCheck(aippDynamicSet, "Hi3796CV300ES");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV420SP_U8);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 4, 2, 221, 221, 0);
    ret = AippParamsCheck(aippDynamicSet, "Hi3796CV300ES");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlSetAIPPInputFormat(aippDynamicSet, ACL_YUV422SP_U8);
    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 4, 2, 221, 221, 0);
    ret = AippParamsCheck(aippDynamicSet, "Hi3796CV300ES");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlSetAIPPCropParams(aippDynamicSet, 1, 3, 2, 221, 221, 0);
    ret = AippParamsCheck(aippDynamicSet, "Hi3796CV300ES");
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPP_Check)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelAttr(_, _))
        .WillRepeatedly(Invoke(GetModelAttr_Invoke));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_,_,_,_))
        .WillRepeatedly(Invoke(GetModelDescInfo_Invoke));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetDynamicBatchInfo(_,_,_))
         .WillRepeatedly(Invoke(GetDynamicBatchInfo_Invoke5));
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    aclError ret = aclmdlAddDatasetBuffer(dataset, buffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke(GetAippTypeNoAippInvoke));

    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlDestroyAIPP(aippDynamicSet);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclDestroyDataBuffer(buffer);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlCreateAndGetOpDesc)
{
    char opName[256];
    memset(opName, '\0', 256);
    aclTensorDesc *inputDesc = nullptr;
    aclTensorDesc *outputDesc = nullptr;
    size_t inputCnt = 0;
    size_t outputCnt = 0;
    aclError ret = aclmdlCreateAndGetOpDesc(0, 0, 0, opName, 256,  &inputDesc, &inputCnt, &outputDesc, &outputCnt);
    EXPECT_EQ(ret, ACL_SUCCESS);
    for (size_t i = 0; i < inputCnt; ++i) {
        const aclTensorDesc *desc = aclGetTensorDescByIndex(inputDesc, i);
    }
    for (size_t i = 0; i < outputCnt; ++i) {
        const aclTensorDesc *desc = aclGetTensorDescByIndex(outputDesc, i);
    }
    ret = aclmdlCreateAndGetOpDesc(0, 0, 0, opName, -1,  &inputDesc, &inputCnt, &outputDesc, &outputCnt);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclDestroyTensorDesc(inputDesc);
    aclDestroyTensorDesc(outputDesc);
}

TEST_F(UTEST_ACL_Model, aclGetTensorDescAddress)
{
    auto ret = aclGetTensorDescAddress(nullptr);
    EXPECT_EQ(ret, nullptr);
}

TEST_F(UTEST_ACL_Model, aclmdlLoadWithConfig)
{
    aclmdlConfigHandle *handle = aclmdlCreateConfigHandle();
    void *p = (void *)0x0001;
    aclError ret = aclmdlSetConfigOpt(handle, ACL_MDL_MEM_ADDR_PTR, &p, sizeof(p));
    EXPECT_EQ(ret, ACL_SUCCESS);
    size_t type = ACL_MDL_LOAD_FROM_MEM_WITH_MEM;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));

    size_t modelSize1 = 1;
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_MEM_SIZET, &modelSize1, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_MEM_SIZET, &modelSize1, sizeof(modelSize1));
    EXPECT_EQ(ret, ACL_SUCCESS);
    char *path = "/home";
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_PATH_PTR, &path, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_PATH_PTR, &path, sizeof(path));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WORKSPACE_ADDR_PTR, &p, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WORKSPACE_ADDR_PTR, &p, sizeof(p));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WORKSPACE_SIZET, &modelSize1, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WORKSPACE_SIZET, &modelSize1, sizeof(modelSize1));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WEIGHT_ADDR_PTR, &p, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WEIGHT_ADDR_PTR, &p, sizeof(p));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WEIGHT_SIZET, &modelSize1, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_WEIGHT_SIZET, &modelSize1, sizeof(modelSize1));
    EXPECT_EQ(ret, ACL_SUCCESS);

    vector<uint32_t> inputQ(100);
    vector<uint32_t> outputQ(100);
    uint32_t *inputQPtr = inputQ.data();
    uint32_t *outputQPtr = outputQ.data();

    type = ACL_MDL_LOAD_FROM_FILE_WITH_Q;
    uint32_t modelId;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlSetConfigOpt(handle, ACL_MDL_INPUTQ_ADDR_PTR, &inputQPtr, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_INPUTQ_ADDR_PTR, &inputQPtr, sizeof(inputQ.data()));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    size_t num = 100;
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_INPUTQ_NUM_SIZET, &num, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_INPUTQ_NUM_SIZET, &num, sizeof(size_t));
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_OUTPUTQ_NUM_SIZET, &num, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_OUTPUTQ_NUM_SIZET, &num, sizeof(size_t));
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_OUTPUTQ_ADDR_PTR, &outputQPtr, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_OUTPUTQ_ADDR_PTR, &outputQPtr, sizeof(outputQ.data()));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    int32_t priority = 1;

    ret = aclmdlSetConfigOpt(handle, ACL_MDL_PRIORITY_INT32, &priority, sizeof(priority));
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_PRIORITY_INT32, &priority, 0);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    type = ACL_MDL_LOAD_FROM_MEM;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    type = ACL_MDL_LOAD_FROM_FILE_WITH_MEM;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    type = ACL_MDL_LOAD_FROM_FILE;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    type = ACL_MDL_LOAD_FROM_FILE_WITH_Q;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    type = ACL_MDL_LOAD_FROM_MEM_WITH_Q;
    aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &type, sizeof(type));
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlDestroyConfigHandle(handle);
}

TEST_F(UTEST_ACL_Model, aclmdlGetRealTensorName)
{
    aclmdlDesc *mdlDesc = aclmdlCreateDesc();
    aclmdlTensorDesc desc;
    desc.name = "dhsdhasiodhsaiodhsiashdisdhsiahdisahdisoahisahdihdisahdaoidhaihdsaihdsaihdsahdishaodhsiahihdoiahdsioadhisahdasidhsaidashdiaoiahdisohdosahdsahdiasoidashoidaoidhahdaoidahioadhiahdsahdiahdaiodaidahdhdahidahdaoda";
    // desc.dimsV2 = {1};
    // desc.size = 1;
    mdlDesc->inputDesc.push_back(desc);
    mdlDesc->outputDesc.push_back(desc);

    aclmdlTensorDesc desc1;
    desc1.name = "a6872_1bu_idc";
    mdlDesc->inputDesc.push_back(desc1);

    aclmdlIODims dims;
    aclmdlIODims dims1;
    auto ret = aclmdlGetInputDimsV2(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlGetInputDimsV2(mdlDesc, 1, &dims1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    // printf("dims.name = %s", dims.name);
    EXPECT_STREQ(dims.name, "acl_modelId_0_input_0");
    EXPECT_STREQ(dims1.name, "a6872_1bu_idc");

    const char *str = aclmdlGetTensorRealName(mdlDesc, dims.name);
    EXPECT_STREQ(desc.name.c_str(), str);

    str = aclmdlGetTensorRealName(mdlDesc, dims1.name);
    EXPECT_STREQ(desc1.name.c_str(), str);

    str = aclmdlGetTensorRealName(mdlDesc, "xxxdwdsdasd");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_0_input_xxx");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "modelId_0_input_xxx");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_xxx_input_0");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_0_input_0");
    EXPECT_EQ(str, desc.name.c_str());

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_0_output_0");
    EXPECT_EQ(str, desc.name.c_str());

    ret = aclmdlGetOutputDims(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_STREQ(dims.name, "acl_modelId_0_output_0");

    ret = aclmdlGetInputDims(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_STREQ(dims.name, "acl_modelId_0_input_0");

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetCurShape(_,_,_))
        .WillOnce(Return(SUCCESS));
    ret = aclmdlGetCurOutputDims(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_STREQ(dims.name, "acl_modelId_0_output_0");

    str = aclmdlGetTensorRealName(mdlDesc, "xxxx_modelId_0_output_0");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_100_input_0");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_0_input_100");
    EXPECT_EQ(str, nullptr);

    str = aclmdlGetTensorRealName(mdlDesc, "acl_modelId_0_output_100");
    EXPECT_EQ(str, nullptr);

    aclmdlDestroyDesc(mdlDesc);
}

TEST_F(UTEST_ACL_Model, aclmdlSetTensorDesc)
{
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    aclError ret = aclmdlAddDatasetBuffer(dataset, buffer);
    aclDataBuffer *buffer1 = aclCreateDataBuffer((void*)0x2, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer1);
    aclmdlDataset *datasetOut = aclmdlCreateDataset();
    aclDataBuffer *buffer2 = aclCreateDataBuffer((void*)0x3, 1);
    ret = aclmdlAddDatasetBuffer(datasetOut, buffer);

    int64_t shape[2] = {16, 32};
    aclTensorDesc *inputDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    size_t index = 1;
    ret = aclmdlSetDatasetTensorDesc (dataset, inputDesc, index);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetDatasetTensorDesc (dataset, nullptr, 0);

    ret = aclmdlExecute(1, dataset, datasetOut);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size_t index1 = 2;
    ret = aclmdlSetDatasetTensorDesc (dataset, inputDesc, index1);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclmdlDestroyDataset(dataset);    
    aclmdlDestroyDataset(datasetOut);
    aclDestroyDataBuffer(buffer);
    aclDestroyDataBuffer(buffer1);
    aclDestroyDataBuffer(buffer2);
    aclDestroyTensorDesc(inputDesc); 
}

TEST_F(UTEST_ACL_Model, aclmdlGetDatasetTensorDesc)
{
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aclDataBuffer *buffer = aclCreateDataBuffer((void*)0x1, 1);
    aclError ret = aclmdlAddDatasetBuffer(dataset, buffer);
    aclDataBuffer *buffer1 = aclCreateDataBuffer((void*)0x2, 1);
    ret = aclmdlAddDatasetBuffer(dataset, buffer1);
    aclmdlDataset *datasetOut = aclmdlCreateDataset();
    aclDataBuffer *buffer2 = aclCreateDataBuffer((void*)0x3, 1);
    ret = aclmdlAddDatasetBuffer(datasetOut, buffer);
    aclDataBuffer *buffer3 = aclCreateDataBuffer((void*)0x4, 1);
    ret = aclmdlAddDatasetBuffer(datasetOut, buffer);

    int64_t shape[2] = {16, 32};
    aclTensorDesc *inputDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    size_t index = 1;
    ret = aclmdlSetDatasetTensorDesc (dataset, inputDesc, index);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlSetDatasetTensorDesc (dataset, nullptr, 0);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), ExecModel(_,_,_,_,_,_,_))
        .WillOnce(Invoke(ExecModelInvoke));
    ret = aclmdlExecute(1, dataset, datasetOut);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclTensorDesc *outputDesc = aclmdlGetDatasetTensorDesc(nullptr, 0);
    EXPECT_EQ(outputDesc, nullptr);

    outputDesc = aclmdlGetDatasetTensorDesc(datasetOut, 2);
    EXPECT_EQ(outputDesc, nullptr);

    outputDesc = aclmdlGetDatasetTensorDesc(datasetOut, 1);
    EXPECT_NE(outputDesc, nullptr);

    aclmdlDestroyDataset(dataset);    
    aclmdlDestroyDataset(datasetOut);
    aclDestroyDataBuffer(buffer);
    aclDestroyDataBuffer(buffer1);
    aclDestroyDataBuffer(buffer2);
    aclDestroyDataBuffer(buffer3);
    aclDestroyTensorDesc(inputDesc);
}

TEST_F(UTEST_ACL_Model, aclmdlGetRealTensorName2)
{
    aclmdlDesc *mdlDesc = aclmdlCreateDesc();
    aclmdlTensorDesc desc;
    desc.name = "dhsdhasiodhsaiodhsiashdisdhsiahdisahdisoahisahdihdisahdaoidhaihdsaihdsaihdsahdishaodhsiahihdoiahdsioadhisahdasidhsaidashdiaoiahdisohdosahdsahdiasoidashoidaoidhahdaoidahioadhiahdsahdiahdaiodaidahdhdahidahdaoda";

    mdlDesc->inputDesc.push_back(desc);
    mdlDesc->outputDesc.push_back(desc);

    aclmdlTensorDesc desc1;
    desc1.name = "acl_modelId_0_input_0";
    mdlDesc->inputDesc.push_back(desc1);

    aclmdlIODims dims;
    auto ret = aclmdlGetInputDimsV2(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_STREQ(dims.name, "acl_modelId_0_input_0_a");

    const char *realName = aclmdlGetTensorRealName(mdlDesc, dims.name);
    EXPECT_STREQ(realName, desc.name.c_str());

    desc1.name = "acl_modelId_0_input_0_a";
    mdlDesc->inputDesc.push_back(desc1);
    ret = aclmdlGetInputDimsV2(mdlDesc, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_STREQ(dims.name, "acl_modelId_0_input_0_b");

    realName = aclmdlGetTensorRealName(mdlDesc, dims.name);
    EXPECT_STREQ(realName, desc.name.c_str());

    aclmdlDestroyDesc(mdlDesc);
}

TEST_F(UTEST_ACL_Model, aclmdlGetInputSizeByIndex2)
{
    aclmdlDesc *mdlDesc = aclmdlCreateDesc();
    aclmdlTensorDesc desc;
    desc.dims.push_back(-1);
    desc.dims.push_back(2);
    desc.shapeRanges.push_back(std::make_pair(1, 3));
    desc.dataType = ACL_FLOAT16;
    mdlDesc->inputDesc.push_back(desc);
    size_t size = aclmdlGetInputSizeByIndex(mdlDesc, 0);
    EXPECT_NE(size, 0);

    size = aclmdlGetInputSizeByIndex(mdlDesc, 2);
    EXPECT_EQ(size, 0);
    aclmdlTensorDesc desc1;
    desc1.dims.push_back(-1);
    desc1.dims.push_back(2);
    desc1.shapeRanges.push_back(std::make_pair(-1, -1));
    mdlDesc->inputDesc.push_back(desc1);
    size = aclmdlGetInputSizeByIndex(mdlDesc, 1);
    EXPECT_EQ(size, 0);

    aclmdlDestroyDesc(mdlDesc);
}

TEST_F(UTEST_ACL_Model, aclGetDataBufferSize)
{
    aclDataBuffer *dataBuffer = nullptr;
    EXPECT_EQ(aclGetDataBufferSize(dataBuffer), 0);

    dataBuffer = aclCreateDataBuffer((void*)0x1, 1);
    EXPECT_NE(aclGetDataBufferSize(dataBuffer), 0);
    aclDestroyDataBuffer(dataBuffer);
}

TEST_F(UTEST_ACL_Model, aclmdlSetAIPPPixelVarReciTest)
{
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    aclError ret = aclmdlSetAIPPPixelVarReci(aippDynamicSet, 1, 1, 1, 0, 3);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclmdlDestroyAIPP(aippDynamicSet);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPPTest01)
{
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    aclmdlDataset *dataset = aclmdlCreateDataset();
    aippDynamicSet->aippParms.inputFormat = CCE_YUV400_U8;
    aippDynamicSet->aippParms.srcImageSizeW = 1;
    aippDynamicSet->aippParms.srcImageSizeH = 511373560;
    aippDynamicSet->batchSize = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke(GetAippTypeSuccessInvoke));
    aclError ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);

    aippDynamicSet->aippParms.srcImageSizeH = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetOrigInputInfo(_, _, _))
        .WillOnce(Return(FAILED));
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclmdlDestroyAIPP(aippDynamicSet);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlSetInputAIPPTest02)
{
    uint32_t batchNumber = 1;
    aclmdlAIPP *aippDynamicSet = aclmdlCreateAIPP(batchNumber);
    aippDynamicSet->aippParms.inputFormat = CCE_YUV420SP_U8;
    aippDynamicSet->aippParms.srcImageSizeW = 1;
    aippDynamicSet->aippParms.srcImageSizeH = 2;
    aclmdlDataset *dataset = aclmdlCreateDataset();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetModelDescInfo(_, _,_,_))
        .WillRepeatedly(Invoke((GetModelDescInfo_Invoke)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAippType(_, _,_,_))
        .WillRepeatedly(Invoke(GetAippTypeSuccessInvoke));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAllAippInputOutputDims(_, _, _, _))
        .WillOnce(Return(FAILED));
    aclError ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAllAippInputOutputDims(_, _, _, _))
        .WillOnce(Return(SUCCESS));
    ret = aclmdlSetInputAIPP(1, dataset, 0, aippDynamicSet);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclmdlDestroyAIPP(aippDynamicSet);
    aclmdlDestroyDataset(dataset);
}

TEST_F(UTEST_ACL_Model, aclmdlGetFirstAippInfoTest)
{
    uint32_t modelId;
    size_t index;
    aclAippInfo aippInfo;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAIPPInfo(_, _, _))
        .WillOnce(Return(FAILED));
    aclError ret = aclmdlGetFirstAippInfo(modelId, index, &aippInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAIPPInfo(_, _, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetBatchInfoSize(_, _))
        .WillOnce(Return(FAILED));
    ret = aclmdlGetFirstAippInfo(modelId, index, &aippInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAIPPInfo(_, _, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetBatchInfoSize(_, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetOrigInputInfo(_, _, _))
        .WillOnce(Return(FAILED));
    ret = aclmdlGetFirstAippInfo(modelId, index, &aippInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAIPPInfo(_, _, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetBatchInfoSize(_, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAllAippInputOutputDims(_, _, _, _))
        .WillOnce(Return(FAILED));
    ret = aclmdlGetFirstAippInfo(modelId, index, &aippInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAIPPInfo(_, _, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetBatchInfoSize(_, _))
        .WillOnce(Return(SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), GetAllAippInputOutputDims(_, _, _, _))
        .WillOnce(Return(SUCCESS));
    ret = aclmdlGetFirstAippInfo(modelId, index, &aippInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
}

extern aclError AippScfSizeCheck(const aclmdlAIPP *aippParmsSet, int32_t batchIndex);
TEST_F(UTEST_ACL_Model, AippScfSizeCheckTest)
{
    uint32_t batchNumber = 10;
    aclmdlAIPP *aippParmsSet = aclmdlCreateAIPP(batchNumber);
    int32_t batchIndex = 0;
    aclError ret = AippScfSizeCheck(aippParmsSet, batchIndex);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclmdlDestroyAIPP(aippParmsSet);
}
