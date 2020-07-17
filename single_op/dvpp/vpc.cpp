/**
* @file vpc.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_dvpp.h"
#include "common/log_inner.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "toolchain/profiling_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

aclError acldvppVpcResizeAsync(acldvppChannelDesc *channelDesc,
                               acldvppPicDesc *inputDesc,
                               acldvppPicDesc *outputDesc,
                               acldvppResizeConfig *resizeConfig,
                               aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcResizeAsync(channelDesc, inputDesc, outputDesc, resizeConfig, stream);
}

aclError acldvppVpcCropAsync(acldvppChannelDesc *channelDesc,
                             acldvppPicDesc *inputDesc,
                             acldvppPicDesc *outputDesc,
                             acldvppRoiConfig *cropArea,
                             aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcCropAsync(channelDesc, inputDesc, outputDesc, cropArea, stream);
}


aclError acldvppVpcCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                     acldvppPicDesc *inputDesc,
                                     acldvppPicDesc *outputDesc,
                                     acldvppRoiConfig *cropArea,
                                     acldvppRoiConfig *pasteArea,
                                     aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcCropAndPasteAsync(channelDesc, inputDesc, outputDesc,
                                                       cropArea, pasteArea, stream);
}

aclError acldvppVpcConvertColorAsync(acldvppChannelDesc *channelDesc,
                                     acldvppPicDesc *inputDesc,
                                     acldvppPicDesc *outputDesc,
                                     aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcConvertColorAsync(channelDesc, inputDesc, outputDesc, stream);
}

aclError acldvppVpcPyrDownAsync(acldvppChannelDesc *channelDesc,
                                acldvppPicDesc *inputDesc,
                                acldvppPicDesc *outputDesc,
                                void* reserve,
                                aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcPyrDownAsync(channelDesc, inputDesc, outputDesc, reserve, stream);
}

aclError acldvppVpcBatchCropAsync(acldvppChannelDesc *channelDesc,
                                  acldvppBatchPicDesc *srcBatchPicDescs,
                                  uint32_t *roiNums,
                                  uint32_t size,
                                  acldvppBatchPicDesc *dstBatchPicDescs,
                                  acldvppRoiConfig *cropAreas[],
                                  aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcBatchCropAsync(channelDesc, srcBatchPicDescs, roiNums,
                                                    size, dstBatchPicDescs, cropAreas, stream);
}

aclError acldvppVpcBatchCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                          acldvppBatchPicDesc *srcBatchPicDescs,
                                          uint32_t *roiNums,
                                          uint32_t size,
                                          acldvppBatchPicDesc *dstBatchPicDescs,
                                          acldvppRoiConfig *cropAreas[],
                                          acldvppRoiConfig *pasteAreas[],
                                          aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcBatchCropAndPasteAsync(channelDesc, srcBatchPicDescs, roiNums,
                                                            size, dstBatchPicDescs, cropAreas, pasteAreas, stream);
}

aclError acldvppVpcEqualizeHistAsync(const acldvppChannelDesc *channelDesc,
                                     const acldvppPicDesc *inputDesc,
                                     acldvppPicDesc *outputDesc,
                                     const acldvppLutMap *lutMap,
                                     aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcEqualizeHistAsync(channelDesc, inputDesc, outputDesc, lutMap, stream);
}

aclError acldvppVpcMakeBorderAsync(const acldvppChannelDesc *channelDesc,
                                   const acldvppPicDesc *inputDesc,
                                   acldvppPicDesc *outputDesc,
                                   const acldvppBorderConfig *borderConfig,
                                   aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcMakeBorderAsync(channelDesc, inputDesc, outputDesc, borderConfig, stream);
}

aclError acldvppVpcCalcHistAsync(acldvppChannelDesc *channelDesc,
                                 acldvppPicDesc *srcPicDesc,
                                 acldvppHist *hist,
                                 void *reserve,
                                 aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcCalcHistAsync(channelDesc, srcPicDesc, hist, reserve, stream);
}


aclError acldvppVpcCropResizeAsync(acldvppChannelDesc *channelDesc,
                                   acldvppPicDesc *inputDesc,
                                   acldvppPicDesc *outputDesc,
                                   acldvppRoiConfig *cropArea,
                                   acldvppResizeConfig *resizeConfig,
                                   aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcCropResizeAsync(channelDesc, inputDesc, outputDesc, cropArea,
        resizeConfig, stream);
}

aclError acldvppVpcBatchCropResizeAsync(acldvppChannelDesc *channelDesc,
                                        acldvppBatchPicDesc *srcBatchPicDescs,
                                        uint32_t *roiNums,
                                        uint32_t size,
                                        acldvppBatchPicDesc *dstBatchPicDescs,
                                        acldvppRoiConfig *cropAreas[],
                                        acldvppResizeConfig *resizeConfig,
                                        aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcBatchCropResizeAsync(channelDesc, srcBatchPicDescs, roiNums, size,
        dstBatchPicDescs, cropAreas, resizeConfig, stream);
}


aclError acldvppVpcCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                        acldvppPicDesc *inputDesc,
                                        acldvppPicDesc *outputDesc,
                                        acldvppRoiConfig *cropArea,
                                        acldvppRoiConfig *pasteArea,
                                        acldvppResizeConfig *resizeConfig,
                                        aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcCropResizePasteAsync(channelDesc, inputDesc, outputDesc, cropArea,
        pasteArea, resizeConfig, stream);
}

aclError acldvppVpcBatchCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                             acldvppBatchPicDesc *srcBatchPicDescs,
                                             uint32_t *roiNums,
                                             uint32_t size,
                                             acldvppBatchPicDesc *dstBatchPicDescs,
                                             acldvppRoiConfig *cropAreas[],
                                             acldvppRoiConfig *pasteAreas[],
                                             acldvppResizeConfig *resizeConfig,
                                             aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcBatchCropResizePasteAsync(channelDesc, srcBatchPicDescs, roiNums, size,
        dstBatchPicDescs, cropAreas, pasteAreas, resizeConfig, stream);
}

aclError acldvppVpcBatchCropResizeMakeBorderAsync(acldvppChannelDesc *channelDesc,
                                                  acldvppBatchPicDesc *srcBatchPicDescs,
                                                  uint32_t *roiNums,
                                                  uint32_t size,
                                                  acldvppBatchPicDesc *dstBatchPicDescs,
                                                  acldvppRoiConfig *cropAreas[],
                                                  acldvppBorderConfig *borderCfgs[],
                                                  acldvppResizeConfig *resizeConfig,
                                                  aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppVpcBatchCropResizeMakeBorderAsync(channelDesc, srcBatchPicDescs, roiNums, size,
        dstBatchPicDescs, cropAreas, borderCfgs, resizeConfig, stream);
}

#ifdef __cplusplus
}
#endif
