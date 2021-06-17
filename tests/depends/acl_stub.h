#include "framework/executor/ge_executor.h"
#include "framework/generator/ge_generator.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/ge_tensor.h"
#include "graph/utils/type_utils.h"
#include "graph/model.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_error_codes.h"
#include "graph/types.h"
#include "graph/operator.h"
#include "ge/ge_api.h"
#include "common/ge_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/opsproto_manager.h"
#include "graph/operator_factory.h"
#include "graph/ge_local_context.h"
#include "graph/tensor.h"
#include "common/helper/om_file_helper.h"

#include "tdt/tdt_host_interface.h"
#include "runtime/dev.h"
#include "runtime/stream.h"
#include "runtime/context.h"
#include "runtime/event.h"
#include "runtime/mem.h"
#include "runtime/kernel.h"
#include "runtime/base.h"
#include "runtime/config.h"
#include "runtime/rt_mem_queue.h"

#include "adx_datadump_server.h"
#include "mmpa/mmpa_api.h"
#include "./jpeg/src/jpeg_stub.h"

#include <gmock/gmock.h>

using namespace tdt;
using namespace ge;

class aclStub {
public:
    // ge function
    virtual Status SetDump(const ge::DumpConfig &dumpConfig);
    virtual Status GEInitialize(const std::map<std::string, std::string>& options);
    virtual Status Finalize();
    virtual Status Ge_Generator_Finalize();
    virtual Status GEFinalize();
    virtual Status BuildSingleOpModel(ge::OpDescPtr &op_desc, const std::vector<GeTensor> &inputs,
                                        const std::vector<GeTensor> &outputs, OpEngineType engine_type,
                                        int32_t compile_flag, ModelBufferData &model_buff);
    virtual graphStatus SetShapeRange(const std::vector<std::pair<int64_t,int64_t>> &range);
    virtual bool ReadBytesFromBinaryFile(char const *file_name, char **buffer, int &length);
    virtual Status Initialize(const std::map<std::string, std::string> &options);
    virtual Status Initialize(const std::map<std::string, std::string> &options, OmgContext &omgContext);
    virtual Status LoadSingleOpV2(const std::string &modelName,
                                  const ModelData &modelData,
                                  void *stream,
                                  SingleOp **single_op,
                                  const uint64_t model_id);
    virtual Status LoadDynamicSingleOpV2(const std::string &model_name,
                               const ge::ModelData &modelData,
                               void *stream,
                               DynamicSingleOp **single_op,
                               const uint64_t model_id);
    virtual Status ExecuteAsync(DynamicSingleOp *executor,
                    const std::vector<GeTensorDesc> &input_desc,
                    const std::vector<DataBuffer> &inputs,
                    std::vector<GeTensorDesc> &output_desc,
                    std::vector<DataBuffer> &outputs);
    virtual Status ExecuteAsync(SingleOp *executor,
                                const std::vector<DataBuffer> &inputs,
                                std::vector<DataBuffer> &outputs);
    virtual bool GetBool(AttrUtils::ConstAttrHolderAdapter &&obj, const string &name, bool &value);
    virtual bool GetInt(AttrUtils::ConstAttrHolderAdapter&& obj, const std::string &name, int32_t &value);
    virtual bool GetListNamedAttrs(AttrUtils::ConstAttrHolderAdapter &&obj, std::string const &name, vector<GeAttrValue::NAMED_ATTRS> &value);
    virtual std::string RealPath(const char *path);
    virtual graphStatus GetOpsTypeList(std::vector<ge::AscendString> &all_ops);
    virtual Status GetModelDescInfo(uint32_t modelId, std::vector<TensorDesc>& inputDesc,
                                 std::vector<TensorDesc>& outputDesc, bool new_model_desc);
    virtual graphStatus GetShapeRange(std::vector<std::pair<int64_t,int64_t>> &range);
    virtual Format GetFormat();
    virtual Status GetDynamicBatchInfo(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                    int32_t &dynamic_type);
    virtual Status LoadModelFromData(uint32_t &model_id, const ModelData &modelData,
                                void *dev_ptr, size_t memsize, void *weight_ptr, size_t weightsize);
    virtual Status LoadDataFromFile(std::string const &path, ModelData &modelData);
    virtual Status LoadModelWithQ(uint32_t &model_id, const ge::ModelData &ge_model_data,
                const std::vector<uint32_t> &input_queue_ids, const std::vector<uint32_t> &output_queue_ids);
    virtual Status UnloadModel(uint32_t modelId);
    virtual Status GetMemAndWeightSize(const std::string &path, size_t &mem_size, size_t &weight_size);
    virtual Status GetMemAndWeightSize(const void *model_data, size_t model_size, size_t &mem_size, size_t &weight_size);
    virtual Status ExecModel(uint32_t model_id, void *stream, const ge::RunModelData &run_input_data,
                            const std::vector<ge::GeTensorDesc> &input_desc, ge::RunModelData &run_output_data,
                            std::vector<ge::GeTensorDesc> &output_desc, bool async_mode);
    virtual Status SetDynamicBatchSize(uint32_t model_id, void *dynamic_input_addr, uint64_t length, uint64_t batch_size);
    virtual Status SetDynamicImageSize(uint32_t model_id, void *dynamic_input_addr, uint64_t length, uint64_t image_height, uint64_t image_width);
    virtual Status SetDynamicDims(uint32_t model_id, void *dynamic_input_addr, uint64_t length,
                                  const vector<uint64_t> &dynamic_dims);
    virtual Status GetCurDynamicDims(uint32_t model_id, const vector<uint64_t> &dynamic_dims,
                                        vector<uint64_t> &cur_dynamic_dims);
    virtual Status GetAippType(uint32_t model_id, uint32_t index, ge::InputAippType &type, size_t &aippindex);
    virtual Status GetUserDesignateShapeOrder(uint32_t model_id, vector<string> &user_designate_shape_order);
    virtual Status GetCurShape(const uint32_t model_id, std::vector<int64_t> &batch_info, int32_t &dynamic_type);
    virtual Status GetModelAttr(uint32_t model_id,std::vector<std::string> &dynamic_output_shape_info);
    virtual Status GetOpAttr(uint32_t model_id, const std::string &op_name, const std::string &attr_name, std::string &attr_value);
    virtual Status GetAIPPInfo(uint32_t model_id, uint32_t index, AippConfigInfo &aipp_params);
    virtual Status GetBatchInfoSize(uint32_t model_id, size_t &shape_count);
    virtual Status GetOrigInputInfo(uint32_t model_id, uint32_t index, OriginInputInfo &origOutputInfo);
    virtual Status GetAllAippInputOutputDims(uint32_t model_id, uint32_t index,
                                        std::vector<InputOutputDims> &input_dims,
                                        std::vector<InputOutputDims> &output_dims);
    virtual Status SetDynamicAippData(uint32_t model_id, void *dynamic_input_addr, uint64_t length,
                                        const std::vector<kAippDynamicBatchPara> &aippBatchPara,
                                        const kAippDynamicPara &aippParms);
    virtual std::string GetErrorMessage();
    virtual int Init();
    virtual bool OpsProtoManager_Initialize(const std::map<std::string, std::string> &options);
    virtual Status TransShape(const TensorDesc &src_desc,
                                Format dst_format,
                                std::vector<int64_t> &dst_shape);
    virtual Status Init(uint8_t *model_data, const uint32_t model_data_size);
    virtual Status GetModelPartition(ModelPartitionType type, ModelPartition &partition);
    virtual graphStatus Load(const uint8_t *data, size_t len, Model &model);
    virtual bool HasAttr(AttrUtils::ConstAttrHolderAdapter&& obj, const string &name);
    virtual bool GetListTensor(AttrUtils::ConstAttrHolderAdapter&& obj, const string& name, vector<ConstGeTensorPtr>& value);

    // runtime function
    virtual rtError_t rtSubscribeReport(uint64_t threadId, rtStream_t stream);
    virtual rtError_t rtSetTaskFailCallback(rtTaskFailCallback callback);
    virtual rtError_t rtCallbackLaunch(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock);
    virtual rtError_t rtProcessReport(int32_t timeout);
    virtual rtError_t rtUnSubscribeReport(uint64_t threadId, rtStream_t stream);
    virtual rtError_t rtCtxCreateEx(rtContext_t *ctx, uint32_t flags, int32_t device);
    virtual rtError_t rtSetDevice(int32_t device);
    virtual rtError_t rtDeviceReset(int32_t device);
    virtual rtError_t rtSetDeviceWithoutTsd(int32_t device);
    virtual rtError_t rtDeviceResetWithoutTsd(int32_t device);
    virtual rtError_t rtDeviceSynchronize(void);
    virtual rtError_t rtGetDevice(int32_t *device);
    virtual rtError_t rtSetTSDevice(uint32_t tsId);
    virtual rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority);
    virtual rtError_t rtStreamDestroy(rtStream_t stream);
    virtual rtError_t rtStreamSynchronize(rtStream_t stream);
    virtual rtError_t rtStreamWaitEvent(rtStream_t stream, rtEvent_t event);
    virtual rtError_t rtCtxDestroyEx(rtContext_t ctx);
    virtual rtError_t rtCtxSetCurrent(rtContext_t ctx);
    virtual rtError_t rtCtxSynchronize();
    virtual rtError_t rtCtxGetCurrent(rtContext_t *ctx);
    virtual rtError_t rtGetPriCtxByDeviceId(int32_t device, rtContext_t *ctx);
    virtual rtError_t rtEventCreateWithFlag(rtEvent_t *event_, uint32_t flag);
    virtual rtError_t rtEventCreate(rtEvent_t *event);
    virtual rtError_t rtGetEventID(rtEvent_t event, uint32_t *eventId);
    virtual rtError_t rtEventDestroy(rtEvent_t event);
    virtual rtError_t rtEventRecord(rtEvent_t event, rtStream_t stream);
    virtual rtError_t rtEventReset(rtEvent_t event, rtStream_t stream);
    virtual rtError_t rtEventSynchronize(rtEvent_t event);
    virtual rtError_t rtEventQuery(rtEvent_t event);
    virtual rtError_t rtEventQueryWaitStatus(rtEvent_t event, rtEventWaitStatus *status);
    virtual rtError_t rtNotifyCreate(int32_t device_id, rtNotify_t *notify_);
    virtual rtError_t rtNotifyDestroy(rtNotify_t notify_);
    virtual rtError_t rtNotifyRecord(rtNotify_t notify_, rtStream_t stream_);
    virtual rtError_t rtGetNotifyID(rtNotify_t notify_, uint32_t *notify_id);
    virtual rtError_t rtNotifyWait(rtNotify_t notify_, rtStream_t stream_);
    virtual rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type);
    virtual rtError_t rtMallocCached(void **devPtr, uint64_t size, rtMemType_t type);
    virtual rtError_t rtFlushCache(void *devPtr, size_t size);
    virtual rtError_t rtInvalidCache(void *devPtr, size_t size);
    virtual rtError_t rtFree(void *devPtr);
    virtual rtError_t rtDvppMalloc(void **devPtr, uint64_t size);
    virtual rtError_t rtDvppFree(void *devPtr);
    virtual rtError_t rtMallocHost(void **hostPtr,  uint64_t size);
    virtual rtError_t rtFreeHost(void *hostPtr);
    virtual rtError_t rtMemset(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count);
    virtual rtError_t rtMemcpy(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind);
    virtual rtError_t rtMemcpyAsync(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind,
        rtStream_t stream);
    virtual rtError_t rtMemsetAsync(void *ptr, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stream);
    virtual rtError_t rtCpuKernelLaunch(const void *soName, const void *kernelName, uint32_t blockDim, const void *args,
        uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream);
    virtual rtError_t rtMemGetInfoEx(rtMemInfoType_t memInfoType, size_t *free, size_t *total);
    virtual rtError_t rtGetRunMode(rtRunMode *mode);
    virtual rtError_t rtGetDeviceCount(int32_t *count);
    virtual rtError_t rtEventElapsedTime(float *time, rtEvent_t start, rtEvent_t end);
    virtual rtError_t rtDevBinaryUnRegister(void *handle);
    virtual rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle);
    virtual rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc, const char *stubName,
        const void *devFunc, uint32_t funcMode);
    virtual rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim, void *args, uint32_t argsSize,
        rtSmDesc_t *smDesc, rtStream_t stream);
    virtual rtError_t rtGetSocVersion(char *version, const uint32_t maxLen);
    virtual rtError_t rtGetGroupCount(uint32_t *count);
    virtual rtError_t rtGetGroupInfo(int32_t groupid, rtGroupInfo_t* groupInfo, uint32_t count);
    virtual rtError_t rtSetGroup(int32_t groupid);
    virtual rtError_t rtGetDevicePhyIdByIndex(uint32_t devIndex, uint32_t *phyId);
    virtual rtError_t rtEnableP2P(uint32_t devIdDes, uint32_t phyIdSrc, uint32_t flag);
    virtual rtError_t rtDisableP2P(uint32_t devIdDes, uint32_t phyIdSrc);
    virtual rtError_t rtDeviceCanAccessPeer(int32_t* canAccessPeer, uint32_t device, uint32_t peerDevice);
    virtual rtError_t rtGetStreamId(rtStream_t stream_, int32_t *streamId);
    virtual rtError_t rtRegDeviceStateCallback(const char *regName, rtDeviceStateCallback callback);
    virtual rtError_t rtDeviceGetStreamPriorityRange(int32_t *leastPriority, int32_t *greatestPriority);
    virtual rtError_t rtGetDeviceCapability(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value);
    virtual rtError_t rtSetOpWaitTimeOut(uint32_t timeout);
    virtual rtError_t rtSetOpExecuteTimeOut(uint32_t timeout);

    virtual rtError_t rtMemQueueInitQS(int32_t devId);
    virtual rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid);

    virtual rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid);

    virtual rtError_t rtMemQueueInit(int32_t devId);

    virtual rtError_t rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *mbuf);

    virtual rtError_t rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **mbuf);

    virtual rtError_t rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout);

    virtual rtError_t rtMemQueueEnQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout);

    virtual rtError_t rtMemQueueDeQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout);

    virtual rtError_t rtMemQueueQuery(int32_t devId, rtMemQueueQueryCmd_t cmd, void *inBuff, uint32_t inLen,
                                    void *outBuff, uint32_t *outLen);

    virtual rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr);

    virtual rtError_t rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeout);

    virtual rtError_t rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack);

    virtual rtError_t rtQueryDevPid(rtBindHostpidInfo_t *info, pid_t *devPid);

    virtual rtError_t rtMbufInit(rtMemBuffCfg_t *cfg);

    virtual rtError_t rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size);

    virtual rtError_t rtMbufFree(rtMbufPtr_t mbuf);

    virtual rtError_t rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf);

    virtual rtError_t rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size);

    virtual rtError_t rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size);

    virtual rtError_t rtMemGrpCreate(const char *name, const rtMemGrpConfig_t *cfg);

    virtual rtError_t rtMemGrpAddProc(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr);

    virtual rtError_t rtMemGrpAttach(const char *name, int32_t timeout);

    virtual rtError_t rtMemGrpQuery(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output);

    virtual rtError_t rtMemcpy2d(void *dst, uint64_t dpitch, const void *src, uint64_t spitch, uint64_t width,
        uint64_t height, rtMemcpyKind kind);

    virtual rtError_t rtMemcpy2dAsync(void *dst, uint64_t dpitch, const void *src, uint64_t spitch, uint64_t width,
        uint64_t height, rtMemcpyKind kind, rtStream stream);

    // tdt function
    virtual int32_t TdtHostInit(uint32_t deviceId);
    virtual int32_t TdtHostPreparePopData();
    virtual int32_t TdtHostStop(const std::string &channelName);
    virtual int32_t TdtHostDestroy();
    virtual int32_t TdtHostPushData(const std::string &channelName, const std::vector<tdt::DataItem> &item, uint32_t deviceId);
    virtual int32_t TdtHostPopData(const std::string &channelName, std::vector<tdt::DataItem> &item);

    //prof function
    virtual int32_t MsprofFinalize();
    virtual int32_t MsprofInit(uint32_t aclDataType, void *data, uint32_t dataLen);

    // adx function
    virtual int AdxDataDumpServerInit();
    virtual int AdxDataDumpServerUnInit();

    // slog function
    virtual int dlog_getlevel(int module_id, int *enable_event);

    // mmpa function
    virtual INT32 mmScandir2(const CHAR *path, mmDirent2 ***entryList, mmFilter2 filterFunc,  mmSort2 sort);
    virtual void* mmAlignMalloc(mmSize mallocSize, mmSize alignSize);

    // jpeg function
    virtual void jpeg_CreateDecompress(j_decompress_ptr cinfo, int version, size_t structsize);
    virtual int jpeg_read_header(j_decompress_ptr cinfo, boolean require_image);
    virtual void jpeg_save_markers(j_decompress_ptr cinfo, int marker_code, unsigned int length_limit);
    virtual void jpeg_mem_src(j_decompress_ptr cinfo, const unsigned char *inbuffer, unsigned long insize);
};

class MockFunctionTest : public aclStub {
public:
    MockFunctionTest() {};
    static MockFunctionTest &aclStubInstance();

    // ge function stub
    MOCK_METHOD1(SetDump, Status(const ge::DumpConfig &dump_config));
    MOCK_METHOD1(GEInitialize, Status(const std::map<std::string, std::string>& options));
    MOCK_METHOD0(Finalize, Status());
    MOCK_METHOD0(GEFinalize, Status());
    MOCK_METHOD6(BuildSingleOpModel, Status(ge::OpDescPtr &op_desc, const std::vector<GeTensor> &inputs,
                const std::vector<GeTensor> &outputs, OpEngineType engine_type,
                int32_t compile_flag, ModelBufferData &model_buff));
    MOCK_METHOD1(SetShapeRange, graphStatus(const std::vector<std::pair<int64_t,int64_t>> &range));
    MOCK_METHOD3(ReadBytesFromBinaryFile, bool(char const *file_name, char **buffer, int &length));
    MOCK_METHOD1(Initialize, Status(const std::map<std::string, std::string> &options));
    MOCK_METHOD2(Initialize, Status(const std::map<std::string, std::string> &options, OmgContext &omgContext));
    MOCK_METHOD0(Ge_Generator_Finalize, Status());
    MOCK_METHOD5(LoadSingleOpV2, Status(const std::string &modelName,const ModelData &modelData,void *stream,
                                  SingleOp **single_op,const uint64_t model_id));
    MOCK_METHOD5(LoadDynamicSingleOpV2, Status(const std::string &model_name,const ge::ModelData &modelData, void *stream,
                               DynamicSingleOp **single_op, const uint64_t model_id));
    MOCK_METHOD5(ExecuteAsync, Status(DynamicSingleOp *executor, const std::vector<GeTensorDesc> &input_desc,
                const std::vector<DataBuffer> &inputs, std::vector<GeTensorDesc> &output_desc,std::vector<DataBuffer> &outputs));
    MOCK_METHOD3(ExecuteAsync, Status(SingleOp *executor,const std::vector<DataBuffer> &inputs,std::vector<DataBuffer> &outputs));
    MOCK_METHOD3(GetBool, bool(AttrUtils::ConstAttrHolderAdapter obj, const string &name, bool &value));
    MOCK_METHOD3(GetInt, bool(AttrUtils::ConstAttrHolderAdapter obj, const std::string &name, int32_t &value));
    MOCK_METHOD3(GetListNamedAttrs, bool(ge::AttrUtils::ConstAttrHolderAdapter obj, std::string const &name, vector<GeAttrValue::NAMED_ATTRS> &value));
    MOCK_METHOD1(RealPath, std::string(const char *path));
    MOCK_METHOD1(GetOpsTypeList, graphStatus(std::vector<ge::AscendString> &all_ops));
    MOCK_METHOD4(GetModelDescInfo, Status(uint32_t modelId, std::vector<TensorDesc>& inputDesc,
                std::vector<TensorDesc>& outputDesc, bool new_model_desc));
    MOCK_METHOD1(GetShapeRange, graphStatus(std::vector<std::pair<int64_t,int64_t>> &range));
    MOCK_METHOD0(GetFormat, Format());
    MOCK_METHOD3(GetDynamicBatchInfo, Status(uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info, int32_t &dynamic_type));
    MOCK_METHOD6(LoadModelFromData, Status(uint32_t &model_id, const ModelData &modelData,
                                void *dev_ptr, size_t memsize, void *weight_ptr, size_t weightsize));
    MOCK_METHOD2(LoadDataFromFile, Status(std::string const &path, ModelData &modelData));
    MOCK_METHOD4(LoadModelWithQ, Status(uint32_t &model_id, const ge::ModelData &ge_model_data,
                const std::vector<uint32_t> &input_queue_ids, const std::vector<uint32_t> &output_queue_ids));
    MOCK_METHOD1(UnloadModel, Status(uint32_t modelId));
    MOCK_METHOD3(GetMemAndWeightSize, Status(const std::string &path, size_t &mem_size, size_t &weight_size));
    MOCK_METHOD4(GetMemAndWeightSize, Status(const void *model_data, size_t model_size, size_t &mem_size, size_t &weight_size));
    MOCK_METHOD7(ExecModel, Status(uint32_t model_id, void *stream, const ge::RunModelData &run_input_data,
                            const std::vector<ge::GeTensorDesc> &input_desc, ge::RunModelData &run_output_data,
                            std::vector<ge::GeTensorDesc> &output_desc, bool async_mode));
    MOCK_METHOD4(SetDynamicBatchSize, Status(uint32_t model_id, void *dynamic_input_addr, uint64_t length, uint64_t batch_size));
    MOCK_METHOD5(SetDynamicImageSize, Status(uint32_t model_id, void *dynamic_input_addr, uint64_t length, uint64_t image_height, uint64_t image_width));
    MOCK_METHOD4(SetDynamicDims, Status(uint32_t model_id, void *dynamic_input_addr, uint64_t length,
                                  const vector<uint64_t> &dynamic_dims));
    MOCK_METHOD3(GetCurDynamicDims, Status(uint32_t model_id, const vector<uint64_t> &dynamic_dims,
                                        vector<uint64_t> &cur_dynamic_dims));
    MOCK_METHOD4(GetAippType, Status(uint32_t model_id, uint32_t index, ge::InputAippType &type, size_t &aippindex));
    MOCK_METHOD2(GetUserDesignateShapeOrder, Status(uint32_t model_id, vector<string> &user_designate_shape_order));
    MOCK_METHOD3(GetCurShape, Status(const uint32_t model_id, std::vector<int64_t> &batch_info, int32_t &dynamic_type));
    MOCK_METHOD2(GetModelAttr, Status(uint32_t model_id,std::vector<std::string> &dynamic_output_shape_info));
    MOCK_METHOD4(GetOpAttr, Status(uint32_t model_id, const std::string &op_name, const std::string &attr_name, std::string &attr_value));
    MOCK_METHOD3(GetAIPPInfo, Status(uint32_t model_id, uint32_t index, AippConfigInfo &aipp_params));
    MOCK_METHOD2(GetBatchInfoSize, Status(uint32_t model_id, size_t &shape_count));
    MOCK_METHOD3(GetOrigInputInfo, Status(uint32_t model_id, uint32_t index, OriginInputInfo &origOutputInfo));
    MOCK_METHOD4(GetAllAippInputOutputDims, Status(uint32_t model_id, uint32_t index, std::vector<InputOutputDims> &input_dims, std::vector<InputOutputDims> &output_dims));
    MOCK_METHOD0(GetErrorMessage, std::string());
    MOCK_METHOD5(SetDynamicAippData, Status(uint32_t model_id, void *dynamic_input_addr, uint64_t length,
                                        const std::vector<kAippDynamicBatchPara> &aippBatchPara,
                                        const kAippDynamicPara &aippParms));
    MOCK_METHOD0(Init, int());
    MOCK_METHOD1(OpsProtoManager_Initialize, bool(const std::map<std::string, std::string> &options));
    MOCK_METHOD3(TransShape, Status(const TensorDesc &src_desc, Format dst_format,
                                    std::vector<int64_t> &dst_shape));
    MOCK_METHOD3(Load, graphStatus(const uint8_t *data, size_t len, Model &model));
    MOCK_METHOD2(Init, Status(uint8_t *model_data, const uint32_t model_data_size));
    MOCK_METHOD2(GetModelPartition, Status(ModelPartitionType type, ModelPartition &partition));
    MOCK_METHOD2(HasAttr, bool(AttrUtils::ConstAttrHolderAdapter obj, const string &name));
    MOCK_METHOD3(GetListTensor, bool(AttrUtils::ConstAttrHolderAdapter obj, const string& name, vector<ConstGeTensorPtr>& value));

    // tdt function stub
    MOCK_METHOD1(TdtHostInit, int32_t(uint32_t deviceId));
    MOCK_METHOD0(TdtHostPreparePopData, int32_t());
    MOCK_METHOD1(TdtHostStop, int32_t(const std::string &channelName));
    MOCK_METHOD0(TdtHostDestroy, int32_t());
    MOCK_METHOD3(TdtHostPushData, int32_t(const std::string &channelName, const std::vector<tdt::DataItem> &item, uint32_t deviceId));
    MOCK_METHOD2(TdtHostPopData, int32_t(const std::string &channelName, std::vector<tdt::DataItem> &item));

    // runtime function stub
    MOCK_METHOD2(rtSubscribeReport, rtError_t(uint64_t threadId, rtStream_t stream));
    MOCK_METHOD1(rtSetTaskFailCallback, rtError_t(rtTaskFailCallback callback));
    MOCK_METHOD4(rtCallbackLaunch, rtError_t(rtCallback_t callBackFunc, void *fnData, rtStream_t stream, bool isBlock));
    MOCK_METHOD1(rtProcessReport, rtError_t(int32_t timeout));
    MOCK_METHOD2(rtUnSubscribeReport, rtError_t(uint64_t threadId, rtStream_t stream));
    MOCK_METHOD3(rtCtxCreateEx, rtError_t(rtContext_t *ctx, uint32_t flags, int32_t device));
    MOCK_METHOD1(rtSetDevice, rtError_t(int32_t device));
    MOCK_METHOD1(rtDeviceReset, rtError_t(int32_t device));
    MOCK_METHOD1(rtSetDeviceWithoutTsd, rtError_t(int32_t device));
    MOCK_METHOD1(rtDeviceResetWithoutTsd, rtError_t(int32_t device));
    MOCK_METHOD0(rtDeviceSynchronize, rtError_t(void));
    MOCK_METHOD1(rtGetDevice, rtError_t(int32_t *device));
    MOCK_METHOD1(rtSetTSDevice, rtError_t(uint32_t tsId));
    MOCK_METHOD2(rtStreamCreate, rtError_t(rtStream_t *stream, int32_t priority));
    MOCK_METHOD1(rtStreamDestroy, rtError_t(rtStream_t stream));
    MOCK_METHOD1(rtStreamSynchronize, rtError_t(rtStream_t stream));
    MOCK_METHOD2(rtStreamWaitEvent, rtError_t(rtStream_t stream, rtEvent_t event));
    MOCK_METHOD1(rtCtxDestroyEx, rtError_t(rtContext_t ctx));
    MOCK_METHOD1(rtCtxSetCurrent, rtError_t(rtContext_t ctx));
    MOCK_METHOD0(rtCtxSynchronize, rtError_t());
    MOCK_METHOD1(rtCtxGetCurrent, rtError_t(rtContext_t *ctx));
    MOCK_METHOD2(rtGetPriCtxByDeviceId, rtError_t(int32_t device, rtContext_t *ctx));
    MOCK_METHOD2(rtEventCreateWithFlag, rtError_t(rtEvent_t *event_, uint32_t flag));
    MOCK_METHOD1(rtEventCreate, rtError_t(rtEvent_t *event));
    MOCK_METHOD2(rtGetEventID, rtError_t(rtEvent_t event, uint32_t *eventId));
    MOCK_METHOD1(rtEventDestroy, rtError_t(rtEvent_t event));
    MOCK_METHOD2(rtEventRecord, rtError_t(rtEvent_t event, rtStream_t stream));
    MOCK_METHOD2(rtEventReset, rtError_t(rtEvent_t event, rtStream_t stream));
    MOCK_METHOD1(rtEventSynchronize, rtError_t(rtEvent_t event));
    MOCK_METHOD1(rtEventQuery, rtError_t(rtEvent_t event));
    MOCK_METHOD2(rtEventQueryWaitStatus, rtError_t(rtEvent_t event, rtEventWaitStatus *status));
    MOCK_METHOD2(rtNotifyCreate, rtError_t(int32_t device_id, rtNotify_t *notify_));
    MOCK_METHOD1(rtNotifyDestroy, rtError_t(rtNotify_t notify_));
    MOCK_METHOD2(rtNotifyRecord, rtError_t(rtNotify_t notify_, rtStream_t stream_));
    MOCK_METHOD2(rtGetNotifyID, rtError_t(rtNotify_t notify_, uint32_t *notify_id));
    MOCK_METHOD2(rtNotifyWait, rtError_t(rtNotify_t notify_, rtStream_t stream_));
    MOCK_METHOD3(rtMalloc, rtError_t(void **devPtr, uint64_t size, rtMemType_t type));
    MOCK_METHOD3(rtMallocCached, rtError_t(void **devPtr, uint64_t size, rtMemType_t type));
    MOCK_METHOD2(rtFlushCache, rtError_t(void *devPtr, size_t size));
    MOCK_METHOD2(rtInvalidCache, rtError_t(void *devPtr, size_t size));
    MOCK_METHOD1(rtFree, rtError_t(void *devPtr));
    MOCK_METHOD2(rtDvppMalloc, rtError_t(void **devPtr, uint64_t size));
    MOCK_METHOD1(rtDvppFree, rtError_t(void *devPtr));
    MOCK_METHOD2(rtMallocHost, rtError_t(void **hostPtr,  uint64_t size));
    MOCK_METHOD1(rtFreeHost, rtError_t(void *hostPtr));
    MOCK_METHOD4(rtMemset, rtError_t(void *devPtr, uint64_t destMax, uint32_t value, uint64_t count));
    MOCK_METHOD5(rtMemcpy, rtError_t(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind));
    MOCK_METHOD6(rtMemcpyAsync, rtError_t(void *dst,  uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind,
        rtStream_t stream));
    MOCK_METHOD5(rtMemsetAsync, rtError_t(void *ptr, uint64_t destMax, uint32_t value, uint64_t count, rtStream_t stream));
    MOCK_METHOD7(rtCpuKernelLaunch, rtError_t(const void *soName, const void *kernelName, uint32_t blockDim, const void *args,
        uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream));
    MOCK_METHOD3(rtMemGetInfoEx, rtError_t(rtMemInfoType_t memInfoType, size_t *free, size_t *total));
    MOCK_METHOD1(rtGetRunMode, rtError_t(rtRunMode *mode));
    MOCK_METHOD1(rtGetDeviceCount, rtError_t(int32_t *count));
    MOCK_METHOD3(rtEventElapsedTime, rtError_t(float *time, rtEvent_t start, rtEvent_t end));
    MOCK_METHOD1(rtDevBinaryUnRegister, rtError_t(void *handle));
    MOCK_METHOD2(rtDevBinaryRegister, rtError_t(const rtDevBinary_t *bin, void **handle));
    MOCK_METHOD5(rtFunctionRegister, rtError_t(void *binHandle, const void *stubFunc, const char *stubName,
        const void *devFunc, uint32_t funcMode));
    MOCK_METHOD6(rtKernelLaunch, rtError_t(const void *stubFunc, uint32_t blockDim, void *args, uint32_t argsSize,
        rtSmDesc_t *smDesc, rtStream_t stream));
    MOCK_METHOD2(rtGetSocVersion, rtError_t(char *version, const uint32_t maxLen));
    MOCK_METHOD1(rtGetGroupCount, rtError_t(uint32_t *count));
    MOCK_METHOD3(rtGetGroupInfo, rtError_t(int32_t groupid, rtGroupInfo_t* groupInfo, uint32_t count));
    MOCK_METHOD1(rtSetGroup, rtError_t(int32_t groupid));
    MOCK_METHOD2(rtGetDevicePhyIdByIndex, rtError_t(uint32_t devIndex, uint32_t *phyId));
    MOCK_METHOD3(rtEnableP2P, rtError_t(uint32_t devIdDes, uint32_t phyIdSrc, uint32_t flag));
    MOCK_METHOD2(rtDisableP2P, rtError_t(uint32_t devIdDes, uint32_t phyIdSrc));
    MOCK_METHOD3(rtDeviceCanAccessPeer, rtError_t(int32_t* canAccessPeer, uint32_t device, uint32_t peerDevice));
    MOCK_METHOD2(rtGetStreamId, rtError_t(rtStream_t stream_, int32_t *streamId));
    MOCK_METHOD2(rtRegDeviceStateCallback, rtError_t(const char *regName, rtDeviceStateCallback callback));
    MOCK_METHOD2(rtDeviceGetStreamPriorityRange, rtError_t(int32_t *leastPriority, int32_t *greatestPriority));
    MOCK_METHOD4(rtGetDeviceCapability, rtError_t(int32_t device, int32_t moduleType, int32_t featureType, int32_t *value));
    MOCK_METHOD1(rtSetOpWaitTimeOut, rtError_t(uint32_t timeout));
    MOCK_METHOD1(rtSetOpExecuteTimeOut, rtError_t(uint32_t timeout));

    MOCK_METHOD1(rtMemQueueInitQS, rtError_t(int32_t devId));
    MOCK_METHOD3(rtMemQueueCreate, rtError_t(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid));
    MOCK_METHOD2(rtMemQueueDestroy, rtError_t(int32_t devId, uint32_t qid));
    MOCK_METHOD1(rtMemQueueInit, rtError_t(int32_t devId));
    MOCK_METHOD3(rtMemQueueEnQueue, rtError_t(int32_t devId, uint32_t qid, void *mbuf));
    MOCK_METHOD3(rtMemQueueDeQueue, rtError_t(int32_t devId, uint32_t qid, void **mbuf));
    MOCK_METHOD4(rtMemQueuePeek, rtError_t(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout));
    MOCK_METHOD4(rtMemQueueEnQueueBuff, rtError_t(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout));
    MOCK_METHOD4(rtMemQueueDeQueueBuff, rtError_t(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout));
    MOCK_METHOD6(rtMemQueueQuery, rtError_t(int32_t devId, rtMemQueueQueryCmd_t cmd, void *inBuff, uint32_t inLen,
                                    void *outBuff, uint32_t *outLen));

    MOCK_METHOD4(rtMemQueueGrant, rtError_t(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr));
    MOCK_METHOD3(rtMemQueueAttach, rtError_t(int32_t devId, uint32_t qid, int32_t timeout));
    MOCK_METHOD3(rtEschedSubmitEventSync, rtError_t(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack));
    MOCK_METHOD2(rtQueryDevPid, rtError_t(rtBindHostpidInfo_t *info, pid_t *devPid));
    MOCK_METHOD1(rtMbufInit, rtError_t(rtMemBuffCfg_t *cfg));
    MOCK_METHOD2(rtMbufAlloc, rtError_t(rtMbufPtr_t *mbuf, uint64_t size));
    MOCK_METHOD1(rtMbufFree, rtError_t(rtMbufPtr_t mbuf));
    MOCK_METHOD2(rtMbufGetBuffAddr, rtError_t(rtMbufPtr_t mbuf, void **databuf));
    MOCK_METHOD2(rtMbufGetBuffSize, rtError_t(rtMbufPtr_t mbuf, uint64_t *size));
    MOCK_METHOD3(rtMbufGetPrivInfo, rtError_t(rtMbufPtr_t mbuf, void **priv, uint64_t *size));
    MOCK_METHOD2(rtMemGrpCreate, rtError_t(const char *name, const rtMemGrpConfig_t *cfg));
    MOCK_METHOD3(rtMemGrpAddProc, rtError_t(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr));
    MOCK_METHOD2(rtMemGrpAttach, rtError_t(const char *name, int32_t timeout));
    MOCK_METHOD3(rtMemGrpQuery, rtError_t(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output));

    //prof function stub
    MOCK_METHOD0(MsprofFinalize, int32_t());
    MOCK_METHOD3(MsprofInit, int32_t(uint32_t aclDataType, void *data, uint32_t dataLen));

    // adx function stub
    MOCK_METHOD0(AdxDataDumpServerInit, int());
    MOCK_METHOD0(AdxDataDumpServerUnInit, int());

    //slog function stub
    MOCK_METHOD2(dlog_getlevel, int(int module_id, int *enable_event));

    // mmpa function stub
    MOCK_METHOD4(mmScandir2, INT32(const CHAR *path, mmDirent2 ***entryList, mmFilter2 filterFunc,  mmSort2 sort));
    MOCK_METHOD2(mmAlignMalloc, void*(mmSize mallocSize, mmSize alignSize));

    // jpeg function
    MOCK_METHOD3(jpeg_CreateDecompress, void(j_decompress_ptr cinfo, int version, size_t structsize));
    MOCK_METHOD2(jpeg_read_header, int(j_decompress_ptr cinfo, boolean require_image));
    MOCK_METHOD3(jpeg_save_markers, void(j_decompress_ptr cinfo, int marker_code, unsigned int length_limit));
    MOCK_METHOD3(jpeg_mem_src, void(j_decompress_ptr cinfo, const unsigned char *inbuffer, unsigned long insize));
};