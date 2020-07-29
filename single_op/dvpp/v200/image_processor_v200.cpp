/**
* @file image_processor_v200.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "image_processor_v200.h"

#include <set>
#include <memory>
#include "securec.h"
#include "common/log_inner.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/common/dvpp_util.h"

namespace {
    // 3 interplation type(value: 0 default Bilinear/1 Bilinear/2 Nearest neighbor)
    constexpr uint32_t DVPP_RESIZE_INTERPLATION_TYPE_UPPER = 2;
    constexpr uint16_t LUT_MAP_DEFAULT_VALUE = 256;
    constexpr uint16_t HIST_DEFAULT_LENGTH = 256;
    constexpr uint32_t PiC_DESC_RESERVED_SIZE = 3;
}

namespace acl {
    namespace dvpp {
    aclError ImageProcessorV200::acldvppVpcConvertColorAsync(acldvppChannelDesc *channelDesc,
                                                             acldvppPicDesc *inputDesc,
                                                             acldvppPicDesc *outputDesc,
                                                             aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcConvertColorAsync");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);

        // check convert color param
        aclError validConvertColorRet = ValidConvertColorParam(inputDesc, outputDesc);
        if (validConvertColorRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("verify convert color param failed, result = %d.", validConvertColorRet);
            return validConvertColorRet;
        }

        // ConvertColor have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddress = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddress[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddress[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddress[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);

        aclError cpyPicRet = ACL_SUCCESS;
        if (aclRunMode_ == ACL_HOST) {
            cpyPicRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyPicRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy convert color input pic desc failed, result = %d.", cpyPicRet);
                return cpyPicRet;
            }
            cpyPicRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyPicRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy convert color output pic desc failed, result = %d.", cpyPicRet);
                return cpyPicRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_CONVERT_COLOR, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            cpyPicRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyPicRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy convert color output pic desc from device failed, runtime result = %d.", cpyPicRet);
                return cpyPicRet;
            }
        }

        ACL_LOG_INFO("end to execute acldvppVpcConvertColorAsync");
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppVpcPyrDownAsync(acldvppChannelDesc *channelDesc,
                                                        acldvppPicDesc *inputDesc,
                                                        acldvppPicDesc *outputDesc,
                                                        void* reserve,
                                                        aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcPyrDownAsync");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        if (reserve != nullptr) {
            ACL_LOG_ERROR("paramete reserve must be null.");
            return ACL_ERROR_INVALID_PARAM;
        }

        aclError validPyrDownInputRet = ValidatePyrDownFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validPyrDownInputRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("input acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validPyrDownInputRet, inputDesc->dvppPicDesc.format);
            return validPyrDownInputRet;
        }

        aclError validPyrDownOutputRet = ValidatePyrDownFormat(
            static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validPyrDownOutputRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("output acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validPyrDownOutputRet, outputDesc->dvppPicDesc.format);
            return validPyrDownOutputRet;
        }

        // PyrDown have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy input pic desc failed, result = %d.", cpyRet);
                return cpyRet;
            }
            cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_PYR_DOWN, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            auto cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output pic desc from device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        ACL_LOG_INFO("end to execute acldvppVpcPyrDownAsync");
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppVpcEqualizeHistAsync(const acldvppChannelDesc *channelDesc,
                                                             const acldvppPicDesc *inputDesc,
                                                             acldvppPicDesc *outputDesc,
                                                             const acldvppLutMap *lutMap,
                                                             aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcEqualizeHistAsync.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(lutMap);
        ACL_REQUIRES_NOT_NULL(lutMap->dvppLutMap.map);

        // check param
        aclError validEqualizeHistRet = ValidEqualizeHistParam(inputDesc, outputDesc);
        if (validEqualizeHistRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("verify equalize hist param failed, result = %d.", validEqualizeHistRet);
            return validEqualizeHistRet;
        }

        uint32_t lutMapSize = 0;
        for (uint32_t index = 0; index < lutMap->dvppLutMap.dims; ++index) {
            lutMapSize += lutMap->dvppLutMap.lens[index];
        }
        // EqualizeHist have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        uint32_t cpyLutMapSize = CalDevDvppStructRealUsedSize(&lutMap->dvppLutMap);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + cpyLutMapSize + lutMapSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddress = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddress[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddress[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddress[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);
        constexpr uint32_t headOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + headOffset,
                                  argsSize - headOffset,
                                  &(lutMap->dvppLutMap),
                                  cpyLutMapSize);
        if (memcpyRet != EOK) {
            ACL_LOG_ERROR("copy lut map to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }
        memcpyRet = memcpy_s(args.get() + headOffset + cpyLutMapSize,
                             argsSize - headOffset - cpyLutMapSize,
                             lutMap->dvppLutMap.map, lutMapSize);
        if (memcpyRet != EOK) {
            ACL_LOG_ERROR("copy map to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        aclError valCpyRet = ACL_SUCCESS;
        if (aclRunMode_ == ACL_HOST) {
            valCpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy equalize hist input pic desc failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
            valCpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy equalize hist output pic desc failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_EQUALIZE_HIST, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            valCpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy equalize hist output pic desc from device failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
        }

        ACL_LOG_INFO("end to execute acldvppVpcEqualizeHistAsync");
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::LaunchTaskForVpcBatchCropResizeMakeBorder(acldvppChannelDesc *channelDesc,
                                                                           VpcBatchParams &batchParams,
                                                                           acldvppBorderConfig *borderCfgs[],
                                                                           const acldvppResizeConfig *resizeConfig,
                                                                           rtStream_t stream)
    {
        // check corp area and makeBorderConfigs, batchParams.totalRoiNums_ at least >= 1
        for (uint32_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            ACL_REQUIRES_NOT_NULL(batchParams.cropAreas_[index]);
            ACL_REQUIRES_NOT_NULL(borderCfgs[index]);
        }

        // calculate crop config total size
        uint32_t cropAreaSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t totalCropConfigSize = cropAreaSize * batchParams.totalRoiNums_;
        uint32_t resizeConfigSize =  CalDevDvppStructRealUsedSize(&resizeConfig->dvppResizeConfig);
        uint32_t makeBorderSize = CalDevDvppStructRealUsedSize(&borderCfgs[0]->dvppBorderConfig);
        uint32_t totalMakeBorderSize = makeBorderSize * batchParams.totalRoiNums_;

        // BatchCrop have 3 inputs
        constexpr int32_t addrNum = 3;
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + addrNum * sizeof(uint64_t) +
            totalCropConfigSize + batchParams.batchSize_ * sizeof(uint16_t) + resizeConfigSize + totalMakeBorderSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = addrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(batchParams.srcBatchPicDescs_->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(batchParams.dstBatchPicDescs_->dataBuffer.data);

        uint32_t cropAreaOffset = sizeof(aicpu::AicpuParamHead) + addrNum * sizeof(uint64_t);
        // copy crop config
        for (size_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            auto memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset,
                &(batchParams.cropAreas_[index]->dvppRoiConfig), cropAreaSize);
            if (memcpyRet != EOK) {
                ACL_LOG_ERROR("copy crop area to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            cropAreaOffset += cropAreaSize;
        }
        // copy roiNums
        auto memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset,
                                  batchParams.roiNums_, sizeof(uint16_t) * batchParams.batchSize_);
        if (memcpyRet != EOK) {
            ACL_LOG_ERROR("copy roiNums to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        // copy resize config
        uint32_t resizeConfigOffset = cropAreaOffset + sizeof(uint16_t) * batchParams.batchSize_;
        memcpyRet = memcpy_s(args.get() + resizeConfigOffset, argsSize - resizeConfigOffset,
            &(resizeConfig->dvppResizeConfig), resizeConfigSize);
        if (memcpyRet != EOK) {
            ACL_LOG_ERROR("copy resize config to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        uint32_t borderConfigOffset = resizeConfigOffset + resizeConfigSize;
        // copy makeBorderConfig
        for (size_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            memcpyRet = memcpy_s(args.get() + borderConfigOffset, argsSize - borderConfigOffset,
                &(borderCfgs[index]->dvppBorderConfig), makeBorderSize);
            if (memcpyRet != EOK) {
                ACL_LOG_ERROR("copy makeborder config to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            borderConfigOffset += makeBorderSize;
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_BATCH_CROP_RESIZE_MAKEBORDER, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppVpcBatchCropResizeMakeBorderAsync(acldvppChannelDesc *channelDesc,
                                                                          acldvppBatchPicDesc *srcBatchPicDescs,
                                                                          uint32_t *roiNums,
                                                                          uint32_t size,
                                                                          acldvppBatchPicDesc *dstBatchPicDescs,
                                                                          acldvppRoiConfig *cropAreas[],
                                                                          acldvppBorderConfig *borderCfgs[],
                                                                          acldvppResizeConfig *resizeConfig,
                                                                          aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcBatchCropResizeMakeBorderAsync.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(cropAreas);
        ACL_REQUIRES_NOT_NULL(roiNums);
        ACL_REQUIRES_NOT_NULL(borderCfgs);
        ACL_REQUIRES_NOT_NULL(resizeConfig);
        ACL_REQUIRES_POSITIVE(size);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);

        // valid input param
        std::unique_ptr<uint16_t[]> roiNumsPtr(new (std::nothrow)uint16_t[size]);
        if (roiNumsPtr == nullptr) {
            ACL_LOG_ERROR("create batch crop roiNums pointer failed, roiNums size = %u.", size);
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t totalRoiNums = 0;
        aclError validParamRet = ValidateAndConvertVpcBatchParams(srcBatchPicDescs,
                                                                  dstBatchPicDescs,
                                                                  roiNums,
                                                                  size,
                                                                  roiNumsPtr.get(),
                                                                  totalRoiNums,
                                                                  BATCH_ROI_MAX_SIZE,
                                                                  resizeConfig);
        if (validParamRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("verify batch crop param failed, result = %d.", validParamRet);
            return validParamRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy input batch pic desc data
            aclError cpyAsyncRet = CopyDvppBatchPicDescAsync(srcBatchPicDescs,
                                                             ACL_MEMCPY_HOST_TO_DEVICE,
                                                             size,
                                                             stream);
            if (cpyAsyncRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("async copy input batch pic desc to device failed, result = %d.", cpyAsyncRet);
                return cpyAsyncRet;
            }

            // copy output batch pic desc data
            cpyAsyncRet = CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                                    ACL_MEMCPY_HOST_TO_DEVICE,
                                                    totalRoiNums,
                                                    stream);
            if (cpyAsyncRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("async copy output batch pic desc to device failed, result = %d.", cpyAsyncRet);
                return cpyAsyncRet;
            }
        } else {
            // set data buffer for input batch pic desc
            aclError setDataRet = SetDataBufferForBatchPicDesc(srcBatchPicDescs, size);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("dvpp batch crop set data buffer for src batch pic desc failed, result = %d.",
                    setDataRet);
                return setDataRet;
            }

            setDataRet = SetDataBufferForBatchPicDesc(dstBatchPicDescs, totalRoiNums);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("dvpp batch crop set data buffer for dst batch pic desc failed, result = %d.",
                    setDataRet);
                return setDataRet;
            }
        }

        void *roiNumsAddress = static_cast<void *>(roiNumsPtr.get());
        VpcBatchParams batchParams = {srcBatchPicDescs,
                                      dstBatchPicDescs,
                                      cropAreas,
                                      nullptr,
                                      totalRoiNums,
                                      roiNumsAddress,
                                      size};

        // launch task
        aclError launchTaskRet = LaunchTaskForVpcBatchCropResizeMakeBorder(channelDesc, batchParams,
            borderCfgs, resizeConfig, stream);
        if (launchTaskRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch task for vpc batch crop, resize and borders config failed, result = %d.",
                launchTaskRet);
            return launchTaskRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy output data
            aclError validCpyRet = CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                                             ACL_MEMCPY_DEVICE_TO_HOST,
                                                             totalRoiNums,
                                                             stream);
            if (validCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output batch pic desc to host failed, result = %d.", validCpyRet);
                return validCpyRet;
            }
        }

        ACL_LOG_INFO("Launch vpc batch crop, resize and make borders tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppVpcMakeBorderAsync(const acldvppChannelDesc *channelDesc,
                                                           const acldvppPicDesc *inputDesc,
                                                           acldvppPicDesc *outputDesc,
                                                           const acldvppBorderConfig *borderConfig,
                                                           aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcMakeBorderAsync.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(borderConfig);

        // check param
        aclError validFormatRet = ValidateMakeBorderInputFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validFormatRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("verify make border input picture format failed, format = %u, result = %d.",
                          inputDesc->dvppPicDesc.format, validFormatRet);
            return validFormatRet;
        }
        validFormatRet = ValidateMakeBorderOutputFormat(
            static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validFormatRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("verify make border output picture format failed, format = %u, result = %d.",
                          outputDesc->dvppPicDesc.format, validFormatRet);
            return validFormatRet;
        }
        // MakeBorder have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        uint32_t cpyMakeBorderSize = CalDevDvppStructRealUsedSize(&borderConfig->dvppBorderConfig);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + cpyMakeBorderSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto address = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        address[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        address[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        address[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);
        constexpr uint32_t headOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + headOffset,
                                  argsSize - headOffset,
                                  &(borderConfig->dvppBorderConfig),
                                  cpyMakeBorderSize);
        if (memcpyRet != EOK) {
            ACL_LOG_ERROR("copy make border config to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        aclError valCpyRet = ACL_SUCCESS;
        if (aclRunMode_ == ACL_HOST) {
            valCpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy make border input pic desc failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
            valCpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy make border output pic desc failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_MAKE_BORDER, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            valCpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (valCpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy make border output pic desc from device failed, result = %d.", valCpyRet);
                return valCpyRet;
            }
        }

        ACL_LOG_INFO("end to execute acldvppVpcMakeBorderAsync");
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppVpcCalcHistAsync(acldvppChannelDesc *channelDesc,
                                                         acldvppPicDesc *srcPicDesc,
                                                         acldvppHist *hist,
                                                         void *reserve,
                                                         aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcCalcHistAsync");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(srcPicDesc);
        ACL_REQUIRES_NOT_NULL(srcPicDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(hist);
        ACL_REQUIRES_NOT_NULL(hist->dvppHistDesc.hist);
        ACL_REQUIRES_NOT_NULL(hist->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(hist->shareBuffer.data);

        if (reserve != nullptr) {
            ACL_LOG_ERROR("reserve param must be null, but addr = %p.", reserve);
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t histSize = 0;
        for (uint32_t i = 0; i < hist->dvppHistDesc.dims; ++i) {
            histSize += hist->dvppHistDesc.lens[i];
        }
        auto histLen = histSize * sizeof(uint32_t);
        if (histLen != hist->shareBuffer.length) {
            ACL_LOG_ERROR("the length of shareBuffer[%u] must be equal to hist data size[%u]",
                          hist->shareBuffer.length, histLen);
            return ACL_ERROR_INVALID_PARAM;
        }
        aclError validVpcInputRet = ValidateCalcHistFormat(
            static_cast<acldvppPixelFormat>(srcPicDesc->dvppPicDesc.format));
        if (validVpcInputRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("input acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validVpcInputRet, srcPicDesc->dvppPicDesc.format);
            return validVpcInputRet;
        }

        // CalcHist have 4 inputs
        constexpr int32_t ioAddrNum = 4;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(srcPicDesc->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(hist->dataBuffer.data);
        ioAddr[3] = reinterpret_cast<uintptr_t>(hist->shareBuffer.data);

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(srcPicDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy src pic desc failed, result = %d.", cpyRet);
                return cpyRet;
            }

            cpyRet = CopyDvppHistDescAsync(hist, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output hist desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_CALC_HIST, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            auto cpyRet = CopyDvppHistDescAsync(hist, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output hist desc from device failed, result = %d.", cpyRet);
                return cpyRet;
            }

            cpyRet = aclrtMemcpyAsync(hist->dvppHistDesc.hist, hist->shareBuffer.length,
                                      hist->shareBuffer.data, histLen,
                                      ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output hist data from device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        ACL_LOG_INFO("end to execute acldvppVpcCalcHistAsync");
        return ACL_SUCCESS;
    }

    acldvppHist* ImageProcessorV200::acldvppCreateHist()
    {
        acldvppHist *aclHist = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclHist = CreateHistOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclHist = CreateHistOnDevice();
                break;
            }
            default: {
                ACL_LOG_ERROR("unknown acl run mode %d.", aclRunMode_);
                return nullptr;
            }
        }

        if (aclHist == nullptr) {
            ACL_LOG_ERROR("create hist is failed, hist address = %p.", aclHist);
            return nullptr;
        }
        ACL_LOG_INFO("hist data host address = %p.", aclHist->dvppHistDesc.hist);
        ACL_LOG_INFO("hist data device address = %p.", aclHist->shareBuffer.data);
        return aclHist;
    }

    acldvppHist *ImageProcessorV200::CreateHistOnHost()
    {
        acldvppHist *aclHist = nullptr;
        // alloc host memory
        uint32_t aclHistSize = CalAclDvppStructSize(aclHist);
        char *hostAddr = new (std::nothrow)char[aclHistSize];
        if (hostAddr == nullptr) {
            ACL_LOG_ERROR("apply host memory for acldvppHist failed. size = %u.", aclHistSize);
            return nullptr;
        }

        // create acldvppHist in host addr
        aclHist = new (hostAddr)acldvppHist;
        if (aclHist == nullptr) {
            ACL_LOG_ERROR("create acldvppHist with function new failed");
            ACL_DELETE_ARRAY_AND_SET_NULL(hostAddr);
            return nullptr;
        }

        // malloc device memory for dvppHistDesc
        void *devPtr = nullptr;
        size_t devSize = CalDevDvppStructRealUsedSize(&aclHist->dvppHistDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devPtr, devSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_ERROR("malloc device memory for acl dvpp pic desc failed, size = %u, runtime result = %d",
                devSize, rtErr);
            aclHist->~acldvppHist();
            ACL_DELETE_ARRAY_AND_SET_NULL(hostAddr);
            return nullptr;
        }
        aclHist->dataBuffer.data = devPtr;
        aclHist->dataBuffer.length = devSize;

        // set lens for each dim
        uint32_t histSize = 0;
        for (uint32_t i = 0; i < aclHist->dvppHistDesc.dims; ++i) {
            aclHist->dvppHistDesc.lens[i] = HIST_DEFAULT_LENGTH;
            histSize += aclHist->dvppHistDesc.lens[i];
        }
        auto histLen = histSize * sizeof(uint32_t);

        // malloc device memory for hist data
        void *histData = nullptr;
        rtErr = rtDvppMalloc(&histData, histLen);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_ERROR("malloc device memory for acl dvpp hist data failed, size = %u, runtime result = %d",
                histLen, rtErr);
            aclHist->~acldvppHist();
            (void)rtFree(devPtr);
            devPtr = nullptr;
            ACL_DELETE_ARRAY_AND_SET_NULL(hostAddr);
            return nullptr;
        }
        aclHist->shareBuffer.data = histData;
        aclHist->shareBuffer.length = histLen;

        // apply host memory for hist data
        if (histSize <= 0) {
            ACL_LOG_ERROR("histSize must be positive, histSize = %u.", histSize);
            aclHist->~acldvppHist();
            (void)rtFree(histData);
            histData = nullptr;
            (void)rtFree(devPtr);
            devPtr = nullptr;
            ACL_DELETE_ARRAY_AND_SET_NULL(hostAddr);
            return nullptr;
        }

        aclHist->dvppHistDesc.hist = new (std::nothrow)uint32_t[histSize];
        if (aclHist->dvppHistDesc.hist == nullptr) {
            ACL_LOG_ERROR("create hist data with function new failed on host side.");
            aclHist->~acldvppHist();
            (void)rtFree(histData);
            histData = nullptr;
            (void)rtFree(devPtr);
            devPtr = nullptr;
            ACL_DELETE_ARRAY_AND_SET_NULL(hostAddr);
            return nullptr;
        }

        return aclHist;
    }

    acldvppHist *ImageProcessorV200::CreateHistOnDevice()
    {
        acldvppHist *aclHist = nullptr;
        // alloc device memory
        void *devAddr = nullptr;
        uint32_t aclHistSize = CalAclDvppStructSize(aclHist);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclHistSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_ERROR("malloc device memory failed, size = %u, runtime result = %d", aclHistSize, rtErr);
            return nullptr;
        }

        // create acldvppHist in device addr
        aclHist = new (devAddr)acldvppHist;
        if (aclHist == nullptr) {
            ACL_LOG_ERROR("create acldvppHist with function new failed");
            (void)rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set lens for each dim
        uint32_t histSize = 0;
        for (uint32_t i = 0; i < aclHist->dvppHistDesc.dims; ++i) {
            aclHist->dvppHistDesc.lens[i] = HIST_DEFAULT_LENGTH;
            histSize += aclHist->dvppHistDesc.lens[i];
        }
        auto histLen = histSize * sizeof(uint32_t);

        // malloc device memory for hist data
        void *histData = nullptr;
        rtErr = rtDvppMalloc(&histData, histLen);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_ERROR("malloc device memory for acl dvpp hist data failed, size = %u, runtime result = %d",
                histLen, rtErr);
            aclHist->~acldvppHist();
            (void)rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }
        aclHist->shareBuffer.data = histData;
        aclHist->shareBuffer.length = histLen;

        // set data buffer
        aclHist->dvppHistDesc.hist = reinterpret_cast<uint32_t *>(histData);
        auto offset = offsetof(acldvppHist, dvppHistDesc);
        aclHist->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppHistDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclHist->dataBuffer.length = CalDevDvppStructRealUsedSize(&aclHist->dvppHistDesc);

        return aclHist;
    }

    aclError ImageProcessorV200::acldvppDestroyHist(acldvppHist *hist)
    {
        if (hist == nullptr) {
            return ACL_SUCCESS;
        }
        FreeDeviceBuffer(hist->shareBuffer);

        switch (aclRunMode_) {
            case ACL_HOST: {
                FreeDeviceBuffer(hist->dataBuffer);
                ACL_DELETE_ARRAY_AND_SET_NULL(hist->dvppHistDesc.hist);
                hist->~acldvppHist();
                ACL_DELETE_ARRAY_AND_SET_NULL(hist);
                break;
            }
            case ACL_DEVICE: {
                hist->~acldvppHist();
                FreeDeviceAddr(static_cast<void *>(hist));
                break;
            }
            default: {
                ACL_LOG_ERROR("unknown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }

        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppClearHist(acldvppHist *hist)
    {
        if (hist == nullptr || hist->shareBuffer.data == nullptr || hist->dvppHistDesc.hist == nullptr) {
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t histSize = 0;
        for (uint32_t i = 0; i < hist->dvppHistDesc.dims; ++i) {
            histSize += hist->dvppHistDesc.lens[i];
        }
        auto histLen = histSize * sizeof(uint32_t);
        if (histLen != hist->shareBuffer.length) {
            ACL_LOG_ERROR("the length of shareBuffer[%u] must equal to hist data size[%u]",
                          hist->shareBuffer.length, histLen);
            return ACL_ERROR_INVALID_PARAM;
        }

        switch (aclRunMode_) {
            case ACL_HOST: {
                errno_t err = memset_s(hist->dvppHistDesc.hist, hist->shareBuffer.length, 0, histLen);
                if (err != EOK) {
                    ACL_LOG_ERROR("set hist data to 0 failed, destMax = %u, count = %u.",
                                hist->shareBuffer.length, histLen);
                    return ACL_ERROR_INTERNAL_ERROR;
                }
                break;
            }
            case ACL_DEVICE: {
                aclError aclRet = aclrtMemset(hist->shareBuffer.data, hist->shareBuffer.length, 0, histLen);
                if (aclRet != ACL_SUCCESS) {
                    ACL_LOG_ERROR("aclrtMemset hist data to 0 fail, destMax = %u, count = %u.",
                                hist->shareBuffer.length, histLen);
                    return aclRet;
                }
                break;
            }
            default: {
                ACL_LOG_ERROR("unknown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        return ACL_SUCCESS;
    }

    uint32_t ImageProcessorV200::acldvppGetHistDims(acldvppHist *hist)
    {
        if (hist == nullptr) {
            ACL_LOG_ERROR("param hist is nullptr");
            return 0;
        }
        return hist->dvppHistDesc.dims;
    }

    aclError ImageProcessorV200::acldvppGetHistData(acldvppHist *hist, uint32_t dim, uint32_t **data, uint16_t *len)
    {
        if (hist == nullptr) {
            ACL_LOG_ERROR("param hist is nullptr.");
            return ACL_ERROR_INVALID_PARAM;
        }
        if (dim >= hist->dvppHistDesc.dims) {
            ACL_LOG_ERROR("input dim[%u] should be smaller than hist's dims[%u].", dim, hist->dvppHistDesc.dims);
            return ACL_ERROR_INVALID_PARAM;
        }
        if (data == nullptr) {
            ACL_LOG_ERROR("the param of data is nullptr.");
            return ACL_ERROR_INVALID_PARAM;
        }
        if (len == nullptr) {
            ACL_LOG_ERROR("the param of len is nullptr.");
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t offset = 0;
        for (uint32_t i = 0; i < dim; ++i) {
            offset += hist->dvppHistDesc.lens[i];    // each len default is 256
        }
        *data = hist->dvppHistDesc.hist + offset;
        *len = hist->dvppHistDesc.lens[dim];
        ACL_LOG_INFO("hist data address = %p, len = %u.", *data, *len);
        return ACL_SUCCESS;
    }

    uint32_t ImageProcessorV200::acldvppGetHistRetCode(acldvppHist* hist)
    {
        if (hist == nullptr) {
            ACL_LOG_ERROR("the param of hist is nullptr.");
            return ACL_ERROR_INVALID_PARAM;
        }
        return hist->dvppHistDesc.retCode;
    }

    aclError ImageProcessorV200::acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig,
                                                                     uint32_t interpolation)
    {
        ACL_LOG_DEBUG("start to execute acldvppSetResizeConfigInterpolation.");
        ACL_REQUIRES_NOT_NULL(resizeConfig);
        // 3 interplation type(value: 0 default Bilinear/1 Bilinear/2 Nearest neighbor)
        if (interpolation > DVPP_RESIZE_INTERPLATION_TYPE_UPPER) {
            ACL_LOG_ERROR("the current interpolation[%u] is not support.", interpolation);
            return ACL_ERROR_INVALID_PARAM;
        }
        resizeConfig->dvppResizeConfig.interpolation = interpolation;
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode)
    {
        ACL_LOG_INFO("start to execute acldvppSetChannelDescMode.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        if ((mode > (DVPP_CHNMODE_VPC | DVPP_CHNMODE_JPEGD | DVPP_CHNMODE_JPEGE)) ||
            (mode < DVPP_CHNMODE_VPC)) {
            ACL_LOG_ERROR("the current mode[%u] is not support", mode);
            return ACL_ERROR_INVALID_PARAM;
        }
        channelDesc->dvppDesc.channelMode = mode;
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::ValidateVpcInputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> vpcInputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_440
        };

        auto iter = vpcInputFormatSet.find(format);
        if (iter != vpcInputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateVpcOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> vpcOutputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888
        };

        auto iter = vpcOutputFormatSet.find(format);
        if (iter != vpcOutputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateCalcHistFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> inputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888
        };

        auto iter = inputFormatSet.find(format);
        if (iter != inputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateDvppResizeConfig(acldvppResizeConfig *config)
    {
        ACL_REQUIRES_NOT_NULL(config);
        // 3 interplation type(value: 0 default Bilinear/1 Bilinear/2 Nearest neighbor)
        if (config->dvppResizeConfig.interpolation > DVPP_RESIZE_INTERPLATION_TYPE_UPPER) {
            ACL_LOG_ERROR("the current interpolation[%u] is not support", config->dvppResizeConfig.interpolation);
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::ValidateConvertColorOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> cvtColorFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888
        };

        auto iter = cvtColorFormatSet.find(format);
        if (iter != cvtColorFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidatePyrDownFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> pyrDownFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400
        };

        auto iter = pyrDownFormatSet.find(format);
        if (iter != pyrDownFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateEqualizeHistFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> equalizeHistFormat = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888
        };

        auto iter = equalizeHistFormat.find(format);
        if (iter != equalizeHistFormat.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateMakeBorderInputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> makeBorderFormat = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400
        };

        auto iter = makeBorderFormat.find(format);
        if (iter != makeBorderFormat.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidateMakeBorderOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> makeBorderFormat = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888
        };

        auto iter = makeBorderFormat.find(format);
        if (iter != makeBorderFormat.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV200::ValidConvertColorParam(const acldvppPicDesc *inputDesc,
                                                        const acldvppPicDesc *outputDesc)
    {
        // check vpc input format
        aclError validConvertColorInputRet = ValidateVpcInputFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validConvertColorInputRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("input acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validConvertColorInputRet, inputDesc->dvppPicDesc.format);
            return validConvertColorInputRet;
        }
        // check vpc output foramt
        aclError validConvertColorOutputRet = ValidateConvertColorOutputFormat(
            static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validConvertColorOutputRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("output acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validConvertColorOutputRet, outputDesc->dvppPicDesc.format);
            return validConvertColorOutputRet;
        }

        uint32_t srcWidth = inputDesc->dvppPicDesc.width;
        uint32_t srcHeight = inputDesc->dvppPicDesc.height;
        uint32_t srcWidthStride = inputDesc->dvppPicDesc.widthStride;
        uint32_t srcHeightStride = inputDesc->dvppPicDesc.heightStride;
        // check source width and height, 0 is invalid value
        bool validPicParam = (srcWidth == 0) || (srcHeight == 0) ||
                             (srcWidthStride == 0) || (srcHeightStride == 0);
        if (validPicParam) {
            ACL_LOG_ERROR("verify src picture width and height failed, 0 is invalid value, "
                          "width = %u, height = %u, widthStride = %u, heightStride = %u",
                          srcWidth, srcHeight, srcWidthStride, srcHeightStride);
            return ACL_ERROR_INVALID_PARAM;
        }
        uint32_t dstWidth = outputDesc->dvppPicDesc.width;
        uint32_t dstHeight = outputDesc->dvppPicDesc.height;
        // dst width or height is allowed to be 0
        validPicParam = ((dstWidth != 0) && (dstWidth != srcWidth)) ||
                        ((dstHeight != 0) && (dstHeight != srcHeight));
        if (validPicParam) {
            ACL_LOG_ERROR("convert color dst pic width or height must match src pic or be 0, srcWidth = %u, "
                          "srcHeight = %u, dstWidth = %u, dstHeight = %u", srcWidth, srcHeight, dstWidth, dstHeight);
            return ACL_ERROR_INVALID_PARAM;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV200::ValidEqualizeHistParam(const acldvppPicDesc *inputDesc,
                                                        const acldvppPicDesc *outputDesc)
    {
        // check vpc input format
        aclError validFormatRet = ValidateEqualizeHistFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validFormatRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("input acldvppPicDesc format verify failed, result = %d, format = %u.",
                          validFormatRet, inputDesc->dvppPicDesc.format);
            return validFormatRet;
        }
        // check vpc output foramt
        if (inputDesc->dvppPicDesc.format != outputDesc->dvppPicDesc.format) {
            ACL_LOG_ERROR("output format must match input format, but got input format = %u, output format = %u.",
                          inputDesc->dvppPicDesc.format, outputDesc->dvppPicDesc.format);
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t srcWidth = inputDesc->dvppPicDesc.width;
        uint32_t srcHeight = inputDesc->dvppPicDesc.height;
        uint32_t srcWidthStride = inputDesc->dvppPicDesc.widthStride;
        uint32_t srcHeightStride = inputDesc->dvppPicDesc.heightStride;
        uint32_t dstWidth = outputDesc->dvppPicDesc.width;
        uint32_t dstHeight = outputDesc->dvppPicDesc.height;
        uint32_t dstWidthStride = outputDesc->dvppPicDesc.widthStride;
        uint32_t dstHeightStride = outputDesc->dvppPicDesc.heightStride;
        bool validPicParam = (srcWidth != dstWidth) || (srcHeight != dstHeight) ||
                             (srcWidthStride != dstWidthStride) || (srcHeightStride != dstHeightStride);
        if (validPicParam) {
            ACL_LOG_ERROR("equalize hist input param must match output, but got srcWidth = %u, srcHeight = %u, "
                          "srcWidthStride = %u, srcHeightStride = %u, dstWidth = %u dstHeight = %u, "
                          "dstWidthStride = %u, dstHeightStride = %u", srcWidth, srcHeight, srcWidthStride,
                          srcHeightStride, dstWidth, dstHeight, dstWidthStride, dstHeightStride);
            return ACL_ERROR_INVALID_PARAM;
        }

        return ACL_SUCCESS;
    }

    acldvppLutMap *ImageProcessorV200::acldvppCreateLutMap()
    {
        ACL_LOG_INFO("start to execute acldvppCreateLutMap");
        acldvppLutMap *aclLutMap = nullptr;
        // alloc memory
        uint32_t acldvppLutMapSize = acl::dvpp::CalAclDvppStructSize(aclLutMap);
        void *structAddr = malloc(acldvppLutMapSize);
        if (structAddr == nullptr) {
            ACL_LOG_ERROR("malloc acldvppLutMap struct memory failed. size is %u.", acldvppLutMapSize);
            return nullptr;
        }

        // create acldvppLutMap in memory
        aclLutMap = new (structAddr)acldvppLutMap();
        if (aclLutMap == nullptr) {
            ACL_LOG_ERROR("create acldvppLutMap with function new failed.");
            ACL_FREE(structAddr);
            return nullptr;
        }

        // fill value
        for (uint32_t index = 0; index < aclLutMap->dvppLutMap.dims; ++index) {
            aclLutMap->dvppLutMap.lens[index] = LUT_MAP_DEFAULT_VALUE;
        }
        uint32_t mapSize = aclLutMap->dvppLutMap.dims * LUT_MAP_DEFAULT_VALUE;
        uint8_t *mapAddr = new (std::nothrow)uint8_t[mapSize];
        if (mapAddr == nullptr) {
            ACL_LOG_ERROR("malloc acldvppLutMap map memory failed. size is %u.", mapSize);
            ACL_FREE(structAddr);
            return nullptr;
        }

        aclLutMap->dvppLutMap.map = mapAddr;
        return aclLutMap;
    }

    aclError ImageProcessorV200::acldvppDestroyLutMap(acldvppLutMap *lutMap)
    {
        if (lutMap != nullptr) {
            ACL_DELETE_ARRAY_AND_SET_NULL(lutMap->dvppLutMap.map);
            ACL_FREE(lutMap);
        }
        return ACL_SUCCESS;
    }

    uint32_t ImageProcessorV200::acldvppGetLutMapDims(const acldvppLutMap *lutMap)
    {
        if (lutMap == nullptr) {
            ACL_LOG_ERROR("param lutMap is nullptr.");
            return 0;
        }
        return static_cast<uint32_t>(lutMap->dvppLutMap.dims);
    }

    aclError ImageProcessorV200::acldvppGetLutMapData(const acldvppLutMap *lutMap,
                                                      uint32_t dim,
                                                      uint8_t **data,
                                                      uint32_t *len)
    {
        ACL_REQUIRES_NOT_NULL(lutMap);
        ACL_REQUIRES_NOT_NULL(lutMap->dvppLutMap.map);
        if (dim >= lutMap->dvppLutMap.dims) {
            ACL_LOG_ERROR("dim[%u] should be smaller than dvppLutMap dims[%u]", dim, lutMap->dvppLutMap.dims);
            return ACL_ERROR_INVALID_PARAM;
        }

        uint32_t dataSize = 0;
        for (uint32_t index = 0; index < dim; ++index) {
            dataSize += lutMap->dvppLutMap.lens[index];
        }
        *len = static_cast<uint32_t>(lutMap->dvppLutMap.lens[dim]);
        *data = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(lutMap->dvppLutMap.map) + dataSize);
        return ACL_SUCCESS;
    }

    acldvppBorderConfig *ImageProcessorV200::acldvppCreateBorderConfig()
    {
        ACL_LOG_INFO("start to execute acldvppCreateBorderConfig");
        acldvppBorderConfig *aclBorderConfig = nullptr;
        // alloc memory
        uint32_t acldvppLutMapSize = acl::dvpp::CalAclDvppStructSize(aclBorderConfig);
        void *structAddr = malloc(acldvppLutMapSize);
        if (structAddr == nullptr) {
            ACL_LOG_ERROR("malloc acldvppBorderConfig struct memory failed. size is %u.", acldvppLutMapSize);
            return nullptr;
        }

        // create acldvppLutMap in memory
        aclBorderConfig = new (structAddr)acldvppBorderConfig();
        if (aclBorderConfig == nullptr) {
            ACL_LOG_ERROR("create acldvppBorderConfig with function new failed.");
            ACL_FREE(structAddr);
            return nullptr;
        }

        return aclBorderConfig;
    }

    aclError ImageProcessorV200::acldvppDestroyBorderConfig(acldvppBorderConfig *borderConfig)
    {
        ACL_FREE(borderConfig);
        return ACL_SUCCESS;
    }

    void ImageProcessorV200::SetDvppWaitTaskType(acldvppChannelDesc *channelDesc)
    {
        channelDesc->dvppWaitTaskType = EVENT_TASK;
        ACL_LOG_INFO("dvpp wait task type is event.");
    }

    aclError ImageProcessorV200::acldvppPngGetImageInfo(const void *data,
                                                        uint32_t dataSize,
                                                        uint32_t *width,
                                                        uint32_t *height,
                                                        int32_t *components)
    {
        ACL_LOG_ERROR("get png image info is not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessorV200::acldvppPngPredictDecSize(const void *data,
                                                          uint32_t dataSize,
                                                          acldvppPixelFormat outputPixelFormat,
                                                          uint32_t *decSize)
    {
        ACL_LOG_ERROR("get png decode size is not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    size_t ImageProcessorV200::GetCmdlistBuffSize()
    {
        return acl::dvpp::DVPP_CMDLIST_BUFFER_SIZE_V200;
    }

    aclError ImageProcessorV200::ValidateJpegOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> jpegOutputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_UNKNOWN
        };

        auto iter = jpegOutputFormatSet.find(format);
        if (iter != jpegOutputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }
    }
}
