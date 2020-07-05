/**
* @file image_processor_v200.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef IMAGE_PROCESSOR_V200_H
#define IMAGE_PROCESSOR_V200_H

#include "single_op/dvpp/base/image_processor.h"

namespace acl {
    namespace dvpp {
    class ImageProcessorV200 : public ImageProcessor {
    public:
        /**
         * Vpc Convert Color.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppVpcConvertColorAsync(acldvppChannelDesc *channelDesc,
                                             acldvppPicDesc *inputDesc,
                                             acldvppPicDesc *outputDesc,
                                             aclrtStream stream) override;

        /**
         * Vpc Pyramid Down.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param reserve[in] reserve param
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppVpcPyrDownAsync(acldvppChannelDesc *channelDesc,
                                        acldvppPicDesc *inputDesc,
                                        acldvppPicDesc *outputDesc,
                                        void* reserve,
                                        aclrtStream stream) override;

        /**
         * Vpc equalize hist.
         * @param channelDesc[in] channel desc
         * @param inputDesc[in] input desc
         * @param outputDesc[in|out] output desc
         * @param lutMap[in] lut map param
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppVpcEqualizeHistAsync(const acldvppChannelDesc *channelDesc,
                                             const acldvppPicDesc *inputDesc,
                                             acldvppPicDesc *outputDesc,
                                             const acldvppLutMap *lutMap,
                                             aclrtStream stream) override;

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
        aclError acldvppVpcMakeBorderAsync(const acldvppChannelDesc *channelDesc,
                                           const acldvppPicDesc *inputDesc,
                                           acldvppPicDesc *outputDesc,
                                           const acldvppBorderConfig *borderConfig,
                                           aclrtStream stream) override;

        /**
         * DVPP set interpolation for resize config.
         * @param resizeConfig[in|out] resize config
         * @param interpolation[in] interplation algorithm
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig,
                                                     uint32_t interpolation) override;

        /**
         * DVPP set mode for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param mode[in] mode
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode) override;

        /**
         * Vpc calculate hist.
         * @param channelDesc[in] channel desc
         * @param srcPicDesc[in] input picture desc
         * @param hist[in|out] hist desc
         * @param reserve[in] reserve param
         * @param stream[in] task stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppVpcCalcHistAsync(acldvppChannelDesc *channelDesc,
                                         acldvppPicDesc *srcPicDesc,
                                         acldvppHist *hist,
                                         void *reserve,
                                         aclrtStream stream) override;

        /**
         * Dvpp create hist desc.
         * @return acldvppHist
         */
        acldvppHist* acldvppCreateHist() override;

        /**
         * Dvpp destroy hist desc.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppDestroyHist(acldvppHist *hist) override;

        /**
         * Dvpp set hist data to 0.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppClearHist(acldvppHist *hist) override;

        /**
         * Dvpp get hist dims.
         * @return dims of hist
         */
        uint32_t acldvppGetHistDims(acldvppHist *hist) override;

        /**
         * Dvpp get hist data by dim.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppGetHistData(acldvppHist *hist, uint32_t dim, uint32_t **data, uint16_t *len) override;

        /**
         * Dvpp get calc hist process return code.
         * @return return code of calc hist process
         */
        uint32_t acldvppGetHistRetCode(acldvppHist* hist) override;

         /**
          * Get image width and height of png (not supported).
          * @param data[in] image data in host memory
          * @param size[in] the size of image data
          * @param width[out] the width of image from image header
          * @param height[out] the height of image from image header
          * @param components[out] the components of image from image header
          * @return ACL_SUCCESS for ok, others for fail
          */
        aclError acldvppPngGetImageInfo(const void *data,
                                        uint32_t dataSize,
                                        uint32_t *width,
                                        uint32_t *height,
                                        int32_t *components) override;

        /**
         * Predict decode size of png image (not supported).
         * @param data[in] origin image data in host memory
         * @param dataSize[in] the size of origin image data
         * @param outputPixelFormat[in] the pixel format jpeg decode
         * @param decSize[out] the size predicted for decode image
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppPngPredictDecSize(const void *data,
                                          uint32_t dataSize,
                                          acldvppPixelFormat outputPixelFormat,
                                          uint32_t *decSize) override;

        /**
         * crop the input batch picture with resize config and border configs according to the specified area
         * as the output batch pictures
         * @param channelDesc [IN]         the channel destruction
         * @param srcBatchPicDescs [IN]    crop input batch picture destruction
         * @param roiNums [IN]    roi config numbers
         * @param size [IN]       roiNum size
         * @param dstBatchPicDescs [IN|OUT]    crop output batch picture destruction
         * @param cropAreas [IN]    crop area configs
         * @param borderCfgs [IN]    border configs
         * @param resizeConfig [IN]    resize config
         * @param stream [IN]       crop batch task stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppVpcBatchCropResizeMakeBorderAsync(acldvppChannelDesc *channelDesc,
                                                          acldvppBatchPicDesc *srcBatchPicDescs,
                                                          uint32_t *roiNums,
                                                          uint32_t size,
                                                          acldvppBatchPicDesc *dstBatchPicDescs,
                                                          acldvppRoiConfig *cropAreas[],
                                                          acldvppBorderConfig *borderCfgs[],
                                                          acldvppResizeConfig *resizeConfig,
                                                          aclrtStream stream) override;

        ~ImageProcessorV200() = default;

        // not allow copy constructor and assignment operators
        ImageProcessorV200(const ImageProcessorV200 &) = delete;

        ImageProcessorV200 &operator=(const ImageProcessorV200 &) = delete;

        ImageProcessorV200(ImageProcessorV200 &&) = delete;

        ImageProcessorV200 &&operator=(ImageProcessorV200 &&) = delete;

        // constructor
        explicit ImageProcessorV200(aclrtRunMode aclRunMode) : ImageProcessor(aclRunMode) {};

    protected:
        /**
         * Validate input pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateVpcInputFormat(acldvppPixelFormat format) override;

        /**
         * Validate vpc output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateVpcOutputFormat(acldvppPixelFormat format) override;

        /**
         * Validate dvpp resize config.
         * @param config[in] dvpp resize config.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateDvppResizeConfig(acldvppResizeConfig *config) override;

        /**
         * Validate convert color output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateConvertColorOutputFormat(acldvppPixelFormat format);

        /**
         * Validate convert color output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateEqualizeHistFormat(acldvppPixelFormat format);

        /**
         * Validate make border input pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateMakeBorderInputFormat(acldvppPixelFormat format);

        /**
         * Validate make border output pic format.
         * @param format[in] output pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidateMakeBorderOutputFormat(acldvppPixelFormat format);

        /**
         * Validate pyr down output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        virtual aclError ValidatePyrDownFormat(acldvppPixelFormat format);

        /**
         * @ingroup AscendCL
         * @brief Create dvpp lut map.
         *
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual acldvppLutMap *acldvppCreateLutMap() override;

        /**
         * @ingroup AscendCL
         * @brief Destroy lut map.
         *
         * @param lutMap [IN]    lut map
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual aclError acldvppDestroyLutMap(acldvppLutMap *lutMap) override;

        /**
         * @ingroup AscendCL
         * @brief Get lut map dims.
         *
         * @param lutMap [IN]    lut map
         * @retval 0 for failed.
         * @retval OtherValues success.
         */
        virtual uint32_t acldvppGetLutMapDims(const acldvppLutMap *lutMap) override;

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
                                              uint32_t *len) override;

        /**
         * @ingroup AscendCL
         * @brief Create dvpp border config.
         *
         * @retval null for failed.
         * @retval OtherValues success.
         */
        virtual acldvppBorderConfig *acldvppCreateBorderConfig() override;

        /**
         * @ingroup AscendCL
         * @brief Destroy border config.
         *
         * @param borderConfig [IN] border config
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError acldvppDestroyBorderConfig(acldvppBorderConfig *borderConfig) override;

        /**
         * Dvpp set wait task type.
         * @param channelDesc[in] dvpp channel desc
         * @return void
         */
        void SetDvppWaitTaskType(acldvppChannelDesc *channelDesc) override;

    private:
        /**
         * Get cmd list buff size.
         * @return cmd list buff size
         */
        size_t GetCmdlistBuffSize() override;

        /**
         * DVPP create hist desc on host.
         * @return acldvppHist
         */
        acldvppHist *CreateHistOnHost();

        /**
         * DVPP create hist desc on device.
         * @return acldvppHist
         */
        acldvppHist *CreateHistOnDevice();

        /**
         * Validate calc hist input pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateCalcHistFormat(acldvppPixelFormat format);

        /**
         * Validate convert color param.
         * @param inputDesc[in] input pic desc.
         * @param outputDesc[in] output pic desc.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidConvertColorParam(const acldvppPicDesc *inputDesc,
                                        const acldvppPicDesc *outputDesc);

        /**
         * Validate equalize hist param.
         * @param inputDesc[in] input pic desc.
         * @param outputDesc[in] output pic desc.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidEqualizeHistParam(const acldvppPicDesc *inputDesc,
                                        const acldvppPicDesc *outputDesc);

        /**
        * Validate jpeg output pic format.
        * @param format[in] input pic format.
        * @return ACL_SUCCESS for success, other for failure.
        */
        aclError ValidateJpegOutputFormat(acldvppPixelFormat format);

        /**
         * launch task for vpc batchcrop resize and makeBorder
         * @param channelDesc [IN]   the channel destruction
         * @param batchParams [IN]   batch params
         * @param borderCfgs [IN]    borderconfigs
         * @param resizeConfig [IN]  resize config
         * @param stream [IN]        task stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchTaskForVpcBatchCropResizeMakeBorder(acldvppChannelDesc *channelDesc,
                                                           VpcBatchParams &batchParams,
                                                           acldvppBorderConfig *borderCfgs[],
                                                           const acldvppResizeConfig *resizeConfig,
                                                           rtStream_t stream);
    };
    }
}
#endif // IMAGE_PROCESSOR_V200_H