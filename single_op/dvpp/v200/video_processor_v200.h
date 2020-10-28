/**
* @file video_processor_v200.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef VIDEO_PROCESSOR_V200_H
#define VIDEO_PROCESSOR_V200_H

#include "single_op/dvpp/base/video_processor.h"

namespace acl {
    namespace dvpp {
    class VideoProcessorV200 : public VideoProcessor {
    public:

        /**
         * Set rc model for venc channel desc.
         * @param channelDesc[IN/OUT] venc channel desc
         * @param rcMode[IN] venc rc mode(VBR=1, CBR=2)
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError aclvencSetChannelDescRcMode(aclvencChannelDesc *channelDesc, uint32_t rcMode) override;

        /**
         * VENC set buf addr for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param bufAddr[in] buf addr
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvencSetChannelDescBufAddr(aclvencChannelDesc *channelDesc, void *bufAddr) override;

        /**
         * VENC set buf size for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param bufSize[in] buf size
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvencSetChannelDescBufSize(aclvencChannelDesc *channelDesc, uint32_t bufSize) override;

        /**
         * Get output buffer size for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return output buffer size
         */
        uint32_t aclvencGetChannelDescBufSize(const aclvencChannelDesc *channelDesc, bool &isSupport) override;

        /**
         * Get output buffer address for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return output buffer address
         */
        void *aclvencGetChannelDescBufAddr(const aclvencChannelDesc *channelDesc, bool &isSupport) override;

        /**
         * VDEC set refFrameNum for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param refFrameNum[in] refFrameNum
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescRefFrameNum(aclvdecChannelDesc *channelDesc, uint32_t refFrameNum) override;

        /**
         * VDEC get refFrameNum for channel desc.
         * @param channelDesc[in] channel desc
         * @retval number of reference frames, default 0.
         */
        uint32_t aclvdecGetChannelDescRefFrameNum(const aclvdecChannelDesc *channelDesc) override;

        /**
         * VDEC set outPicFormat for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outPicFormat[in] outPicFormat
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                   acldvppPixelFormat outPicFormat) override;

        /**
         * VDEC set channel id for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param channelId[in] channel id
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc, uint32_t channelId) override;

        /**
         * VDEC set bit depth for channel desc.
         * @param channelDesc [IN]     vdec channel description.
         * @param bitDepth [IN]     bit depth.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescBitDepth(aclvdecChannelDesc *channelDesc, uint32_t bitDepth) override;

        /**
         * VDEC get bit depth for channel desc.
         * @param channelDesc [IN]    vdec channel description.
         * @retval bit depth， default 0.
         */
        uint32_t aclvdecGetChannelDescBitDepth(const aclvdecChannelDesc *channelDesc) override;

        /**
         * Vdec send frame
         * @param channelDesc[in] vdec channel desc
         * @param input[in] input stream desc
         * @param output[in|out] output pic desc
         * @param config[in] vdec frame config
         * @param userData[in] user data
         * @param isSkipFlag[in] skipped frame flag
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSendFrame(aclvdecChannelDesc *channelDesc,
                                  acldvppStreamDesc *input,
                                  acldvppPicDesc *output,
                                  aclvdecFrameConfig *config,
                                  void *userData,
                                  bool isSkipFlag) override;

        /**
         * Venc create channel desc
         * @return aclvencChannelDesc
         */
        aclvencChannelDesc *aclvencCreateChannelDesc() override;

        /**
         * Venc destroy channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvencDestroyChannelDesc(aclvencChannelDesc *channelDesc) override;

        ~VideoProcessorV200() = default;

        // not allow copy constructor and assignment operators
        VideoProcessorV200(const VideoProcessorV200 &) = delete;

        VideoProcessorV200 &operator=(const VideoProcessorV200 &) = delete;

        VideoProcessorV200(VideoProcessorV200 &&) = delete;

        VideoProcessorV200 &&operator=(VideoProcessorV200 &&) = delete;

        // constructor
        explicit VideoProcessorV200(aclrtRunMode aclRunMode) : VideoProcessor(aclRunMode) {};

        aclError aclvdecSetChannelDescMatrix(aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix matrixFormat) override;

        aclError aclvdecGetChannelDescMatrix(const aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix &matrixFormat) override;

    protected:
        aclError aclvencMallocOutMemory(aclvencChannelDesc *channelDesc) override;

        void aclvencFreeOutMemory(aclvencChannelDesc *channelDesc) override;

        /**
         * Venc launch release task.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchReleaseFrameTask(const aclvencChannelDesc *channelDesc) override;

        /**
         * Venc send not eos frame
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvencSendNomalFrame(aclvencChannelDesc *channelDesc,
                                               acldvppPicDesc *input,
                                               void *reserve,
                                               aclvencFrameConfig *config,
                                               void *userdata) override;

        /**
         * Venc send eos frame
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvencSendEosFrame(aclvencChannelDesc *channelDesc, aclvencFrameConfig *config) override;

        /**
         * Venc send eos frame including create and destroy frame config
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError SendEosForVenc(aclvencChannelDesc *channelDesc) override;

        /**
         * Vdec set wait task type.
         * @param channelDesc[in] vdec channel desc
         * @return void
         */
        void SetVdecWaitTaskType(aclvdecChannelDesc *channelDesc) override;

        /**
         * Venc set wait task type.
         * @param channelDesc[in] venc channel desc
         * @return void
         */
        void SetVencWaitTaskType(aclvencChannelDesc *channelDesc) override;

    private:
        /**
         * get venc frame callback function.
         * @return ACL_SUCCESS for ok, others for fail
         */
        static void GetVencFrameCallbackV200(void *callbackData);

        /**
         * Venc launch send frame task.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchVencSendFrameTask(const aclvencChannelDesc *channelDesc, acldvppPicDesc *inputPicDesc,
            aclvencFrameConfig *config);

        /**
         * Vdec launch task for get stream.
         * @param channelDesc[in] vdec channel desc
         * @param callbackInfoPtr[in] callback info data
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError LaunchTaskForGetStream(aclvdecChannelDesc *channelDesc,
            aicpu::dvpp::VdecCallbackInfoPtr callbackInfoPtr);

        /**
         * Vdec launch release frame task.
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError LaunchVdecReleaseFrameTask(const aclvdecChannelDesc *channelDesc);

        /**
         * get vdec frame callback function for runtime.
         * @param callbackData[in] callback user data
         * @return void
         */
        static void GetVdecFrameCallbackV200(void *callbackData);

        /**
         * Venc launch wait task.
         * @param channelDesc[in] venc channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchVencWaitTask(aclvencChannelDesc *channelDesc);
    };
    }
}
#endif // VIDEO_PROCESSOR_V200_H
