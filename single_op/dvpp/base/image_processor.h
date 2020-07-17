/**
* @file image_processor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <map>
#include <string>
#include <typeinfo>
#include "runtime/rt.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"

namespace acl {
    namespace dvpp {
    struct VpcBatchParams {
        // src batch pic descs
        acldvppBatchPicDesc *srcBatchPicDescs_;
        // dst batch pic descs
        acldvppBatchPicDesc *dstBatchPicDescs_;
        // crop areas: pointer array
        acldvppRoiConfig **cropAreas_;
        // paste areas: pointer array
        acldvppRoiConfig **pasteAreas_;
        // total roi numbers
        uint32_t totalRoiNums_;
        // roi numbers for each src pic
        void *roiNums_;
        // size of roiNums
        uint32_t batchSize_;
    };

    class ImageProcessor {
    public:
        /**
         * Dvpp create channel
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppCreateChannel(acldvppChannelDesc *channelDesc);

        /**
         * Dvpp destory channel
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyChannel(acldvppChannelDesc *channelDesc);

        /**
         * Vpc resize.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param resizeConfig[in] resize config
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcResizeAsync(acldvppChannelDesc *channelDesc,
                                               acldvppPicDesc *inputDesc,
                                               acldvppPicDesc *outputDesc,
                                               acldvppResizeConfig *resizeConfig,
                                               aclrtStream stream);

        /**
         * Vpc crop.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param cropArea[in] crop area config
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcCropAsync(acldvppChannelDesc *channelDesc,
                                             acldvppPicDesc *inputDesc,
                                             acldvppPicDesc *outputDesc,
                                             acldvppRoiConfig *cropArea,
                                             aclrtStream stream);
        /**
         * Vpc crop and resize config.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param cropArea[in] crop area config
         * @param resizeConfig[in] resize config
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                   acldvppPicDesc *inputDesc,
                                                   acldvppPicDesc *outputDesc,
                                                   acldvppRoiConfig *cropArea,
                                                   acldvppResizeConfig *resizeConfig,
                                                   aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop.
         *
         * @par Function
         * crop the input batch picture according to the specified area
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop ared configs
         * @param stream [IN]    crop batch task stream
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig
         */
        virtual aclError acldvppVpcBatchCropAsync(acldvppChannelDesc *channelDesc,
                                                  acldvppBatchPicDesc *srcBatchPicDescs,
                                                  uint32_t *roiNums,
                                                  uint32_t size,
                                                  acldvppBatchPicDesc *dstBatchPicDescs,
                                                  acldvppRoiConfig *cropAreas[],
                                                  aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop and resize config
         *
         * @par Function
         * crop the input batch picture according with resize config to the specified area
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop ared configs
         * @param resizeConfig[in] resize config
         * @param stream [IN]    crop batch and resize config task stream
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig | acldvppCreateResizeConfig
         */
        virtual aclError acldvppVpcBatchCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                        acldvppBatchPicDesc *srcBatchPicDescs,
                                                        uint32_t *roiNums,
                                                        uint32_t size,
                                                        acldvppBatchPicDesc *dstBatchPicDescs,
                                                        acldvppRoiConfig *cropAreas[],
                                                        acldvppResizeConfig *resizeConfig,
                                                        aclrtStream stream);

        /**
         * Vpc crop and paste.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param cropArea[in] crop area config
         * @param pasteArea[in] paste area config
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                                     acldvppPicDesc *inputDesc,
                                                     acldvppPicDesc *outputDesc,
                                                     acldvppRoiConfig *cropArea,
                                                     acldvppRoiConfig *pasteArea,
                                                     aclrtStream stream);
        /**
         * Vpc crop, resize config and paste.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param cropArea[in] crop area config
         * @param pasteArea[in] paste area config
         * @param stream[in] runtime stream
         * @param resizeConfig[in] resize config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                        acldvppPicDesc *inputDesc,
                                                        acldvppPicDesc *outputDesc,
                                                        acldvppRoiConfig *cropArea,
                                                        acldvppRoiConfig *pasteArea,
                                                        acldvppResizeConfig *resizeConfig,
                                                        aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop and paste.
         *
         * @par Function
         * crop the input batch picture according to the specified area,
         * and paste the pictures to the specified position of the target pictures
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param pasteAreas [IN]    paste area configs
         * @param stream [IN]    crop batch task stream
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig
         */
        virtual aclError acldvppVpcBatchCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                                          acldvppBatchPicDesc *srcBatchPicDescs,
                                                          uint32_t *roiNums,
                                                          uint32_t size,
                                                          acldvppBatchPicDesc *dstBatchPicDescs,
                                                          acldvppRoiConfig *cropAreas[],
                                                          acldvppRoiConfig *pasteAreas[],
                                                          aclrtStream stream);
        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop, config resize and paste.
         *
         * @par Function
         * crop the input batch picture according to the specified area,
         * and paste the pictures to the specified position of the target pictures
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param pasteAreas [IN]    paste area configs
         * @param stream [IN]    crop batch and resize config task stream
         * @param resizeConfig [IN]    resize config
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig | acldvppCreateResizeConfig
         */
        virtual aclError acldvppVpcBatchCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                             acldvppBatchPicDesc *srcBatchPicDescs,
                                                             uint32_t *roiNums,
                                                             uint32_t size,
                                                             acldvppBatchPicDesc *dstBatchPicDescs,
                                                             acldvppRoiConfig *cropAreas[],
                                                             acldvppRoiConfig *pasteAreas[],
                                                             acldvppResizeConfig *resizeConfig,
                                                             aclrtStream stream);

        /**
         * Vpc convert color.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcConvertColorAsync(acldvppChannelDesc *channelDesc,
                                                     acldvppPicDesc *inputDesc,
                                                     acldvppPicDesc *outputDesc,
                                                     aclrtStream stream);

        /**
         * Vpc pyramid down.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param reserve[in] reserve param
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcPyrDownAsync(acldvppChannelDesc *channelDesc,
                                                acldvppPicDesc *inputDesc,
                                                acldvppPicDesc *outputDesc,
                                                void* reserve,
                                                aclrtStream stream);

        /**
         * Vpc equalize hist.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param lutMap[in] lut map param
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcEqualizeHistAsync(const acldvppChannelDesc *channelDesc,
                                                     const acldvppPicDesc *inputDesc,
                                                     acldvppPicDesc *outputDesc,
                                                     const acldvppLutMap *lutMap,
                                                     aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief Vpc make border.
         *
         * @param channelDesc[in]    channel desc
         * @param inputDesc[in]   input desc
         * @param outputDesc[in|out]    output desc
         * @param borderConfig[in]    border config param
         * @param stream[in]    runtime stream
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel|acldvppCreatePicDesc|acldvppCreateBorderConfig
         */
        virtual aclError acldvppVpcMakeBorderAsync(const acldvppChannelDesc *channelDesc,
                                                   const acldvppPicDesc *inputDesc,
                                                   acldvppPicDesc *outputDesc,
                                                   const acldvppBorderConfig *borderConfig,
                                                   aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief Vpc batch crop, resize config and make border.
         *
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param resizeConfig [IN]    resize config
         * @param borderCfgs [IN]    border configs
         * @param stream [IN]    crop batch, resize config and make border task stream
         *
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel|acldvppCreatePicDesc|acldvppCreateBorderConfig|acldvppCreateResizeConfig
         */
        virtual aclError acldvppVpcBatchCropResizeMakeBorderAsync(acldvppChannelDesc *channelDesc,
                                                                  acldvppBatchPicDesc *srcBatchPicDescs,
                                                                  uint32_t *roiNums,
                                                                  uint32_t size,
                                                                  acldvppBatchPicDesc *dstBatchPicDescs,
                                                                  acldvppRoiConfig *cropAreas[],
                                                                  acldvppBorderConfig *borderCfgs[],
                                                                  acldvppResizeConfig *resizeConfig,
                                                                  aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief Vpc calculate hist.
         *
         * @param channelDesc[in] channel desc
         * @param srcPicDesc[in] input picture desc
         * @param hist[in|out] hist desc
         * @param reserve[in] reserve param
         * @param stream[in] task stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppVpcCalcHistAsync(acldvppChannelDesc *channelDesc,
                                                 acldvppPicDesc *srcPicDesc,
                                                 acldvppHist *hist,
                                                 void *reserve,
                                                 aclrtStream stream);

        /**
         * Jpeg decode.
         * @param channelDesc[in] channel desc
         * @param data[in] decode input picture destruction's data
         * @param size[in|out] decode input picture destruction's size
         * @param outputDesc[in|out] decode output picture destruction
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppJpegDecodeAsync(acldvppChannelDesc *channelDesc,
                                                const void *data,
                                                uint32_t size,
                                                acldvppPicDesc *outputDesc,
                                                aclrtStream stream);

        /**
         * Jpeg encode.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] encode input picture destruction
         * @param data[in] encode output picture destruction's data
         * @param size[in|out] encode output picture destruction's size
         * @param config[in] jpeg encode config
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppJpegEncodeAsync(acldvppChannelDesc *channelDesc,
                                                acldvppPicDesc *inputDesc,
                                                const void *data,
                                                uint32_t *size,
                                                acldvppJpegeConfig *config,
                                                aclrtStream stream);

        /**
         * Png decode.
         * @param channelDesc[in] channel desc
         * @param data[in] decode input picture destruction's data
         * @param size[in|out] decode input picture destruction's size
         * @param outputDesc[in|out] decode output picture destruction
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppPngDecodeAsync(acldvppChannelDesc *channelDesc,
                                               const void *data,
                                               uint32_t size,
                                               acldvppPicDesc *outputDesc,
                                               aclrtStream stream);
        /**
         * Read word.
         * @param pStart[in] image data pointer
         * @param pos[in] the offset of image data
         * @return uint16_t
         */
        uint16_t ReadWord(const unsigned char* const pStart, uint32_t& pos);

        /**
         * inner function in HandleExifProtocol
         */
        bool GetOrientationValue(uint32_t ifd0StartIndex, uint16_t endianValue, const uint8_t* inputAddr,
                                 uint16_t app1Size, uint16_t &orientation);

        /**
         * inner function in JudgeNeedOrientation
         */
        bool HandleExifProtocol(uint32_t endianOffset, uint16_t endianValue, const uint8_t* inputData,
                                uint16_t app1Size, uint16_t &orientation);

        /**
         * Judge orientation image.
         * @param data[in] image data in host memory
         * @param jpegdDataSize[in] the size of image data
         * @return ACL_SUCCESS for ok, others for fail
         */
        bool JudgeNeedOrientation(const unsigned char *data, uint32_t jpegdDataSize);

        /**
         * Get image width and height of jpeg.
         * @param data[in] image data in host memory
         * @param size[in] the size of image data
         * @param width[out] the width of image from image header
         * @param height[out] the height of image from image header
         * @param components[out] the components of image from image header
         * @param format [out] the format of image from image header
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppJpegGetImageInfo(const void *data,
                                                 uint32_t size,
                                                 uint32_t *width,
                                                 uint32_t *height,
                                                 int32_t *components,
                                                 acldvppJpegFormat *format);

        /**
         * Predict encode size of jpeg image.
         * @param inputDesc[in] dvpp image desc
         * @param config[in] jpeg encode config
         * @param size[out] the size predicted of image
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppJpegPredictEncSize(const acldvppPicDesc *inputDesc,
                                                   const acldvppJpegeConfig *config,
                                                   uint32_t *size);

        /**
         * Predict decode size of jpeg image.
         * @param data[in] origin image data in host memory
         * @param dataSize[in] the size of origin image data
         * @param outputPixelFormat[in] the pixel format jpeg decode
         * @param decSize[out] the size predicted for decode image
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppJpegPredictDecSize(const void *data,
                                                   uint32_t dataSize,
                                                   acldvppPixelFormat outputPixelFormat,
                                                   uint32_t *decSize);

         /**
          * Get image width and height of png.
          * @param data[in] image data in host memory
          * @param size[in] the size of image data
          * @param width[out] the width of image from image header
          * @param height[out] the height of image from image header
          * @param components[out] the components of image from image header
          * @return ACL_SUCCESS for ok, others for fail
          */
         virtual aclError acldvppPngGetImageInfo(const void* data,
                                                 uint32_t dataSize,
                                                 uint32_t *width,
                                                 uint32_t *height,
                                                 int32_t *components) = 0;

        /**
         * Predict decode size of png image.
         * @param data[in] origin image data in host memory
         * @param dataSize[in] the size of origin image data
         * @param outputPixelFormat[in] the pixel format jpeg decode
         * @param decSize[out] the size predicted for decode image
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppPngPredictDecSize(const void *data,
                                                  uint32_t dataSize,
                                                  acldvppPixelFormat outputPixelFormat,
                                                  uint32_t *decSize) = 0;

        /**
         * Dvpp create channel desc.
         * @return acldvppChannelDesc
         */
        virtual acldvppChannelDesc *acldvppCreateChannelDesc();

        /**
         * DVPP destroy channel desc.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyChannelDesc(acldvppChannelDesc *channelDesc);

        /**
         * Dvpp create pic desc.
         * @return acldvppPicDesc
         */
        virtual acldvppPicDesc *acldvppCreatePicDesc();

        /**
         * DVPP destroy pic desc.
         * @param picDesc[in] pic desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyPicDesc(acldvppPicDesc *picDesc);

        /**
         * Dvpp create resize config.
         * @return acldvppResizeConfig
         */
        virtual acldvppResizeConfig *acldvppCreateResizeConfig();

        /**
         * DVPP destroy resize config.
         * @param resizeConfig[in] resize config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyResizeConfig(acldvppResizeConfig *resizeConfig);

        /**
         * DVPP set interpolation for resize config.
         * @param resizeConfig[in|out] resize config
         * @param interpolation[in] interplation algorithm
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig,
                                                             uint32_t interpolation) = 0;

        /**
         * DVPP set mode for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param mode[in] mode
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode) = 0;

        /**
         * @ingroup AscendCL
         * @brief Create dvpp lut map.
         *
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual acldvppLutMap *acldvppCreateLutMap();

        /**
         * @ingroup AscendCL
         * @brief Destroy lut map.
         *
         * @param lutMap [IN]    lut map
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual aclError acldvppDestroyLutMap(acldvppLutMap *lutMap);

        /**
         * @ingroup AscendCL
         * @brief Get lut map dims.
         *
         * @param lutMap [IN]    lut map
         * @retval 0 for failed.
         * @retval OtherValues success.
         */
        virtual uint32_t acldvppGetLutMapDims(const acldvppLutMap *lutMap);

        /**
        * @ingroup AscendCL
        * @brief Get lut map data.
        *
        * @param lutMap [IN]    lut map
        * @param dim [IN]    input dim of map
        * @param data [OUT]    the dim of lut map's data
        * @param len [OUT]    the dim of lut map's length
        * @retval ACL_SUCCESS The function is successfully executed.
        * @retval OtherValues Failure
        */
        virtual aclError acldvppGetLutMapData(const acldvppLutMap *lutMap,
                                              uint32_t dim,
                                              uint8_t **data,
                                              uint32_t *len);

        /**
         * @ingroup AscendCL
         * @brief Create dvpp border config.
         *
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual acldvppBorderConfig *acldvppCreateBorderConfig();

        /**
         * @ingroup AscendCL
         * @brief Destroy border config.
         *
         * @param borderConfig [IN] border config
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError acldvppDestroyBorderConfig(acldvppBorderConfig *borderConfig);

        /**
         * DVPP create roi config.
         * @param left[in] left location
         * @param right[in] right location
         * @param top[in] top location
         * @param bottom[in] bottom location
         * @return acldvppRoiConfig
         */
        virtual acldvppRoiConfig *acldvppCreateRoiConfig(uint32_t left,
                                                         uint32_t right,
                                                         uint32_t top,
                                                         uint32_t bottom);

        /**
         * DVPP destroy roi config.
         * @param roiConfig[in] roi config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyRoiConfig(acldvppRoiConfig *roiConfig);

        /**
         * Dvpp create jpege config.
         * @return acldvppJpegeConfig
         */
        virtual acldvppJpegeConfig *acldvppCreateJpegeConfig();

        /**
         * DVPP destroy jpege config.
         * @param jpegeConfig[in] jpege config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyJpegeConfig(acldvppJpegeConfig *jpegeConfig);

        /**
         * Dvpp create hist desc.
         * @return acldvppHist
         */
        virtual acldvppHist* acldvppCreateHist();

        /**
         * Dvpp destroy hist desc.
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppDestroyHist(acldvppHist *hist);

        /**
         * Dvpp set hist data to 0.
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppClearHist(acldvppHist *hist);

        /**
         * Dvpp get hist dims.
         * @return dims of hist
         */
        virtual uint32_t acldvppGetHistDims(acldvppHist *hist);

        /**
         * Dvpp get hist data by dim.
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError acldvppGetHistData(acldvppHist *hist, uint32_t dim, uint32_t **data, uint16_t *len);

        /**
         * Dvpp get calc hist process return code.
         * @return return code of calc hist process
         */
        virtual uint32_t acldvppGetHistRetCode(acldvppHist* hist);

        ~ImageProcessor() = default;

        // not allow copy constructor and assignment operators
        ImageProcessor(const ImageProcessor &) = delete;

        ImageProcessor &operator=(const ImageProcessor &) = delete;

        ImageProcessor(ImageProcessor &&) = delete;

        ImageProcessor &&operator=(ImageProcessor &&) = delete;

    protected:
        /**
         * Constructor.
         * @param aclRunMode[in] acl run mode
         */
        explicit ImageProcessor(aclrtRunMode aclRunMode) : aclRunMode_(aclRunMode) {};

        /**
         * Validate input pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateVpcInputFormat(acldvppPixelFormat format) = 0;

        /**
         * Validate vpc output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateVpcOutputFormat(acldvppPixelFormat format) = 0;

        /**
         * Validate dvpp resize config.
         * @param config[in] dvpp resize config.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateDvppResizeConfig(acldvppResizeConfig *config) = 0;

        /**
        * Validate jpeg input pic format.
        * @param format[in] input pic format.
        * @return ACL_SUCCESS for success, other for failure.
        */
        virtual aclError ValidateJpegInputFormat(acldvppPixelFormat format);

        /**
        * Validate jpeg output pic format.
        * @param format[in] input pic format.
        * @return ACL_SUCCESS for success, other for failure.
        */
        virtual aclError ValidateJpegOutputFormat(acldvppPixelFormat format) = 0;

        /**
         * Dvpp set wait task type.
         * @param channelDesc[in] dvpp channel desc
         * @return void
         */
        virtual void SetDvppWaitTaskType(acldvppChannelDesc *channelDesc);

        /**
         * Launch dvpp task.
         * @param channelDesc[in] dvpp channel desc
         * @param args[in] args
         * @param argsSize[in] args size
         * @param kernelName[in] dvpp kernel name
         * @param stream[in] stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchDvppTask(const acldvppChannelDesc *channelDesc, const char *args, uint32_t argsSize,
            const char *kernelName, aclrtStream stream);

    protected:
        // acl run mode
        aclrtRunMode aclRunMode_ = ACL_HOST;

        /**
         * Set data buffer for batch pic desc
         * @param batchPicDesc[in|out] batch pic desc
         * @param batchSize[in] batch size
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError SetDataBufferForBatchPicDesc(acldvppBatchPicDesc *batchPicDesc, uint32_t batchSize);

        /**
         * Valid batch Param and convert roi nums.
         * @param srcBatchPicDescs[in] src batch pic descs
         * @param dstBatchPicDescs[in] dst batch pic descs
         * @param roiNums[in] roi nums
         * @param size[in] roiNums size
         * @param deviceRoiNums[in|out] convert roiNums type to uint8_t
         * @param totalRoiNums[in|out] total roi nums
         * @param maxRoiNums[in] max roi nums
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError ValidateAndConvertVpcBatchParams(const acldvppBatchPicDesc *srcBatchPicDescs,
                                                  const acldvppBatchPicDesc *dstBatchPicDescs,
                                                  const uint32_t *roiNums,
                                                  uint32_t size,
                                                  uint16_t *deviceRoiNums,
                                                  uint32_t &totalRoiNums,
                                                  uint32_t maxRoiNums,
                                                  acldvppResizeConfig *resizeConfig);
    private:
        /**
         * @ingroup AscendCL
         * @brief validate param for dvpp vpc crop, paste and resize config
         * @param channelDesc [IN]    the channel destruction
         * @param inputDesc [IN]    src pic desc
         * @param outputDesc [IN]   dst pic desc
         * @param cropArea [IN]    crop area
         * @param pasteArea [IN]    paste area
         * @param pasteAreaSwitch [IN]  switch of checking paste area or not
         * @param resizeConfig [IN]    resize config
         * @param resizeConfigSwitch [IN]    resize config switch
         * @param resizeConfigSize [IN]    size of resize config
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         */
        aclError ValidateParamForDvppVpcCropResizePaste(const acldvppChannelDesc *channelDesc,
                                                        const acldvppPicDesc *inputDesc,
                                                        const acldvppPicDesc *outputDesc,
                                                        const acldvppRoiConfig *cropArea,
                                                        const acldvppRoiConfig *pasteArea,
                                                        const bool &pasteAreaSwitch,
                                                        acldvppResizeConfig *resizeConfig,
                                                        const bool &resizeConfigSwitch,
                                                        uint32_t &resizeConfigSize);
        /**
         * Get cmd list buff size.
         * @return cmd list buff size
         */
        virtual size_t GetCmdlistBuffSize();

        /**
         * DVPP create channel desc on host.
         * @return acldvppChannelDesc
         */
        acldvppChannelDesc *CreateChannelDescOnHost();

        /**
         * DVPP create channel desc on device.
         * @return acldvppChannelDesc
         */
        acldvppChannelDesc *CreateChannelDescOnDevice();

        /**
         * DVPP create channel desc on host.
         * @return acldvppPicDesc
         */
        acldvppPicDesc *CreatePicDescOnHost();

        /**
         * DVPP create channel desc on device.
         * @return acldvppPicDesc
         */
        acldvppPicDesc *CreatePicDescOnDevice();

        /**
         * Create dvpp channel with notify.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError DvppCreateChannelWithNotify(acldvppChannelDesc *channelDesc);

        /**
         * Create dvpp channel without notify.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError DvppCreateChannelWithoutNotify(acldvppChannelDesc *channelDesc);

        /**
         * Destory stream for dvpp channel.
         * @param stream[in] stream
         * @return void
         */
        void DestroyStream(rtStream_t stream);

        /**
         * Destory dvpp channel with notify.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError DvppDestroyChannelWithNotify(acldvppChannelDesc *channelDesc);

        /**
         * Destory dvpp channel without notify.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError DvppDestroyChannelWithoutNotify(acldvppChannelDesc *channelDesc);

        /**
         * Create notify for dvpp channel.
         * @param channelDesc[in] channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError CreateNotifyForDvppChannel(acldvppChannelDesc *channelDesc);

        /**
         * Destory notify and stream for dvpp channel.
         * @param channelDesc[in] channel desc
         * @param stream[in] stream
         * @return void
         */
        void DestroyNotifyAndStream(acldvppChannelDesc *channelDesc, rtStream_t stream);

        /**
         * Launch dvpp wait task.
         * @param channelDesc[in] dvpp channel desc
         * @param stream[in] stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchDvppWaitTask(const acldvppChannelDesc *channelDesc, aclrtStream stream);

        /**
         * Launch task for vpc batch crop
         * @param channelDesc[in] channel desc
         * @param batchParams[in] vpc batch params
         * @param resizeConfig[in] resize config
         * @param stream[in] stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchTaskForVpcBatchCrop(acldvppChannelDesc *channelDesc,
                                           VpcBatchParams &batchParams,
                                           const acldvppResizeConfig *resizeConfig,
                                           rtStream_t stream);

        /**
         * Launch task for vpc batch crop and paste
         * @param channelDesc[in] channel desc
         * @param batchParams[in] vpc batch params
         * @param resizeConfig[in] resize config
         * @param stream[in] stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchTaskForVpcBatchCropAndPaste(acldvppChannelDesc *channelDesc,
                                                   VpcBatchParams &batchParams,
                                                   const acldvppResizeConfig *resizeConfig,
                                                   rtStream_t stream);

        /**
         * To avoid acl_dvpp   depends on  the libjpeg(8d).
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError AvoidDependOnLibjpeg8d(void *compressInfoPtr);

        /**
         * Reset event for dvpp.
         * @return void
         */
        void ResetEvent(acldvppChannelDesc *channelDesc, rtStream_t rtStream);

        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop and resize config
         *
         * @par Function
         * crop the input batch picture according with resize config to the specified area
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop ared configs
         * @param resizeConfig[in] resize config
         * @param stream [IN]    crop batch task stream
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig | acldvppCreateResizeConfig
         */
        aclError DvppVpcCropResizeAsync(acldvppChannelDesc *channelDesc,
                                        acldvppPicDesc *inputDesc,
                                        acldvppPicDesc *outputDesc,
                                        acldvppRoiConfig *cropArea,
                                        acldvppResizeConfig *resizeConfig,
                                        const bool &resizeConfigSwitch,
                                        aclrtStream stream);

        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop, config resize and paste.
         *
         * @par Function
         * crop the input batch picture according to the specified area,
         * and paste the pictures to the specified position of the target pictures
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param pasteAreas [IN]    paste area configs
         * @param stream [IN]    crop batch and resize config task stream
         * @param resizeConfig [IN]    resize config
         * @param resizeConfigSwitch [IN]    resize config effect or not
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig | acldvppCreateResizeConfig
         */
        aclError DvppVpcBatchCropResizeAsync(acldvppChannelDesc *channelDesc,
                                             acldvppBatchPicDesc *srcBatchPicDescs,
                                             uint32_t *roiNums,
                                             uint32_t size,
                                             acldvppBatchPicDesc *dstBatchPicDescs,
                                             acldvppRoiConfig *cropAreas[],
                                             acldvppResizeConfig *resizeConfig,
                                             const bool &resizeConfigSwitch,
                                             aclrtStream stream);
        /**
         * Vpc crop, resize config and paste.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param cropArea[in] crop area config
         * @param pasteArea[in] paste area config
         * @param stream[in] runtime stream
         * @param resizeConfig[in] resize config
         * @param resizeConfigSwitch [in] use resize config or not
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError DvppVpcCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                             acldvppPicDesc *inputDesc,
                                             acldvppPicDesc *outputDesc,
                                             acldvppRoiConfig *cropArea,
                                             acldvppRoiConfig *pasteArea,
                                             acldvppResizeConfig *resizeConfig,
                                             const bool &resizeConfigSwitch,
                                             aclrtStream stream);
        /**
         * @ingroup AscendCL
         * @brief dvpp vpc batch crop, config resize and paste.
         *
         * @par Function
         * crop the input batch picture according to the specified area,
         * and paste the pictures to the specified position of the target pictures
         * as the output batch pictures
         * @param channelDesc [IN]    the channel destruction
         * @param srcBatchPicDescs [IN|OUT]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]    roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param pasteAreas [IN]    paste area configs
         * @param stream [IN]    crop batch and resize config task stream
         * @param resizeConfig [IN]    resize config
         * @param resizeConfigSwitch [IN]    use resize config or not
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         *
         * @see acldvppCreateChannel | acldvppCreateBatchPicDesc | acldvppCreateRoiConfig | acldvppCreateResizeConfig
         */
        aclError DvppVpcBatchCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                  acldvppBatchPicDesc *srcBatchPicDescs,
                                                  uint32_t *roiNums,
                                                  uint32_t size,
                                                  acldvppBatchPicDesc *dstBatchPicDescs,
                                                  acldvppRoiConfig *cropAreas[],
                                                  acldvppRoiConfig *pasteAreas[],
                                                  acldvppResizeConfig *resizeConfig,
                                                  const bool &resizeConfigSwitch,
                                                  aclrtStream stream);
    };
    }
}
#endif // IMAGE_PROCESSOR_H
