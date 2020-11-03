/**
* @file video_processor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <map>
#include <string>
#include <typeinfo>
#include "runtime/rt.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"

namespace acl {
    namespace dvpp {
    class VideoProcessor {
    public:
        /**
         * Vdec create channel
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecCreateChannel(aclvdecChannelDesc *channelDesc);

        /**
         * Vdec destroy channel
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecDestroyChannel(aclvdecChannelDesc *channelDesc);

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
        virtual aclError aclvdecSendFrame(aclvdecChannelDesc *channelDesc,
                                          acldvppStreamDesc *input,
                                          acldvppPicDesc *output,
                                          aclvdecFrameConfig *config,
                                          void *userData,
                                          bool isSkipFlag);

        /**
         * Vdec create channel desc
         * @return aclvdecChannelDesc
         */
        virtual aclvdecChannelDesc *aclvdecCreateChannelDesc();

        /**
         * Vdec destroy channel desc
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecDestroyChannelDesc(aclvdecChannelDesc *channelDesc);

        /**
         * Dvpp create stream desc
         * @return acldvppStreamDesc
         */
        virtual acldvppStreamDesc *acldvppCreateStreamDesc();

        /**
         * Vdec destroy stream desc
         * @param streamDesc vdec stream desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError acldvppDestroyStreamDesc(acldvppStreamDesc *streamDesc);

        /**
         * Vdec create frame config
         * @return aclvdecFrameConfig
         */
        virtual aclvdecFrameConfig *aclvdecCreateFrameConfig();

        /**
         * VENC set buf size for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param bufSize[in] buf size
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSetChannelDescBufSize(aclvencChannelDesc *channelDesc, uint32_t bufSize);

        /**
         * VENC set buf addr for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param bufAddr[in] buf addr
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSetChannelDescBufAddr(aclvencChannelDesc *channelDesc, void *bufAddr);

        /**
         * Set rc model for venc channel desc.
         * @param channelDesc[IN/OUT] venc channel desc
         * @param rcMode[IN] venc rc mode(VBR=1, CBR=2)
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError aclvencSetChannelDescRcMode(aclvencChannelDesc *channelDesc, uint32_t rcMode);

        /**
         * Set source rate for venc channel desc.
         * @param channelDesc[IN/OUT] venc channel desc
         * @param srcRate[IN] source rate
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError aclvencSetChannelDescSrcRate(aclvencChannelDesc *channelDesc, uint32_t srcRate);

        /**
         * Set max bit rate for venc channel desc.
         * @param channelDesc[IN/OUT] venc channel desc
         * @param maxBitRate[IN] max bit rate
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError aclvencSetChannelDescMaxBitRate(aclvencChannelDesc *channelDesc, uint32_t maxBitRate);

        /**
         * Set ip proportion for venc channel desc.
         * @param channelDesc[OUT] venc channel desc
         * @param ipProp[IN] I frame and P frame proportion
         * @return ACL_SUCCESS for success, other for failure
         */
        virtual aclError aclvencSetChannelDescIPProp(aclvencChannelDesc *channelDesc, uint32_t ipProp);

        /**
         * Get output buffer size for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return output buffer size
         */
        virtual uint32_t aclvencGetChannelDescBufSize(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * Get output buffer address for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return output buffer address
         */
        virtual void *aclvencGetChannelDescBufAddr(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * @ingroup AscendCL
         * @brief Get rc mode for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return rc mode, default 0
         */
        virtual uint32_t aclvencGetChannelDescRcMode(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * @ingroup AscendCL
         * @brief Get source rate for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return source rate, default 0
         */
        virtual uint32_t aclvencGetChannelDescSrcRate(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * @ingroup AscendCL
         * @brief Get max bit rate for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return max bit rate, default 0
         */
        virtual uint32_t aclvencGetChannelDescMaxBitRate(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * Get ip proportion for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return I frame and P frame proportion
         */
        virtual uint32_t aclvencGetChannelDescIPProp(const aclvencChannelDesc *channelDesc, bool &isSupport);

        /**
         * VDEC set outout pic Wdith for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outPicWidth[in] outPicWidth
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescOutPicWidth(aclvdecChannelDesc *channelDesc, uint32_t outPicWidth);

        /**
         * VDEC set outout pic Height for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outHeight[in] outHeight
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescOutPicHeight(aclvdecChannelDesc *channelDesc, uint32_t outPicHeight);

        /**
         * VDEC set refFrameNum for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param refFrameNum[in] refFrameNum
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescRefFrameNum(aclvdecChannelDesc *channelDesc, uint32_t refFrameNum);

        /**
         * VDEC get refFrameNum for channel desc.
         * @param channelDesc[in] channel desc
         * @retval number of reference frames, default 0.
         */
        virtual uint32_t aclvdecGetChannelDescRefFrameNum(const aclvdecChannelDesc *channelDesc);

        /**
         * VDEC set outPicFormat for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outPicFormat[in] outPicFormat
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                           acldvppPixelFormat outPicFormat);

        /**
         * VDEC set channel id for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param channelId[in] channel id
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc, uint32_t channelId);

        /**
         * VDEC set bit depth for channel desc.
         * @param channelDesc [IN]     vdec channel description.
         * @param bitDepth [IN]     bit depth.
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecSetChannelDescBitDepth(aclvdecChannelDesc *channelDesc, uint32_t bitDepth);

        /**
         * VDEC get bit depth for channel desc.
         * @param channelDesc [IN]    vdec channel description.
         * @retval bit depth， default 0.
         */
        virtual uint32_t aclvdecGetChannelDescBitDepth(const aclvdecChannelDesc *channelDesc);

        /**
         * set csc matrix in vdec channelDesc
         *
         * @param channelDesc [OUT]             the channel destruction
         * @param matrixFormat [IN]             the csc matrix format
         *
         * @retval ACL_SUCCESS The function is successfully executed.
         * @retval OtherValues Failure
         */
        virtual aclError aclvdecSetChannelDescMatrix(aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix matrixFormat);

        /**
         * get csc matrix in vdec channelDesc
         *
         * @param channelDesc [IN]              the vdec channel destruction
         * @param matrixFormat [OUT]            the csc matrix format
         */
        virtual aclError aclvdecGetChannelDescMatrix(const aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix &matrixFormat);

        /**
         * Vdec destroy frame config
         * @param vdecFrameConfig vdec frame config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvdecDestroyFrameConfig(aclvdecFrameConfig *vdecFrameConfig);

        /**
         * Venc create channel desc
         * @return aclvencChannelDesc
         */
        virtual aclvencChannelDesc *aclvencCreateChannelDesc();

        /**
         * Venc destroy channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencDestroyChannelDesc(aclvencChannelDesc *channelDesc);

        /**
         * Venc create frame config
         * @return aclvencFrameConfig
         */
        virtual aclvencFrameConfig *aclvencCreateFrameConfig();

        /**
         * Venc destroy frame config
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencDestroyFrameConfig(aclvencFrameConfig *vencFrameConfig);

        /**
         * Venc create channel
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencCreateChannel(aclvencChannelDesc *channelDesc);

        /**
         * Venc destroy channel
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencDestroyChannel(aclvencChannelDesc *channelDesc);

        /**
         * Venc set inPicFormat for channel desc.
         * @param channelDesc[out] channel desc
         * @param picFormat[in] input picture format
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSetChannelDescPicFormat(aclvencChannelDesc *channelDesc,
            acldvppPixelFormat picFormat);

        /**
         * Venc send frame
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSendFrame(aclvencChannelDesc *channelDesc, acldvppPicDesc *input, void *reserve,
            aclvencFrameConfig *config, void *userdata);

        ~VideoProcessor() = default;

        // not allow copy constructor and assignment operators
        VideoProcessor(const VideoProcessor &) = delete;

        VideoProcessor &operator=(const VideoProcessor &) = delete;

        VideoProcessor(VideoProcessor &&) = delete;

        VideoProcessor &&operator=(VideoProcessor &&) = delete;

    protected:
        /**
         * Constructor.
         * @param aclRunMode[in] acl run mode
         */
        explicit VideoProcessor(aclrtRunMode aclRunMode)
        {
            aclRunMode_ = aclRunMode;
        };

        /**
         * Venc malloc output buff.
         * @param channelDesc[in/out] venc channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencMallocOutMemory(aclvencChannelDesc *channelDesc)
        {
            return ACL_SUCCESS;
        }

        /**
         * Venc free output buff.
         * @param channelDesc[in/out] venc channel desc
         */
        virtual void aclvencFreeOutMemory(aclvencChannelDesc *channelDesc) {}

        /**
         * Venc create channel desc on host.
         * @return aclvencChannelDesc
         */
        virtual aclvencChannelDesc *CreateVencChannelDescOnHost();

        /**
         * Venc create channel desc on device.
         * @return aclvencChannelDesc
         */
        virtual aclvencChannelDesc *CreateVencChannelDescOnDevice();

        /**
         * Venc launch release task.
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError LaunchReleaseFrameTask(const aclvencChannelDesc *channelDesc);

        /**
         * Venc send not eos frame
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSendNomalFrame(aclvencChannelDesc *channelDesc,
                                               acldvppPicDesc *input,
                                               void *reserve,
                                               aclvencFrameConfig *config,
                                               void *userdata);

        /**
         * Venc send eos frame
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError aclvencSendEosFrame(aclvencChannelDesc *channelDesc, aclvencFrameConfig *config);

        /**
         * Venc send eos frame including create and destroy frame config
         * @param channelDesc channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError SendEosForVenc(aclvencChannelDesc *channelDesc);

        /**
         * Clear cached tasks for vdec
         * @param channelDesc vdec channel desc
         * @return void
         */
        void ClearCachedTasks(aclvdecChannelDesc *channelDesc);

        /**
         * Check and copy vdec input and output desc to device.
         * @param input[in] input stream desc
         * @param output[in] output pic desc
         * @param isSkipFlag[in] skipped frame flag
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError CheckAndCopyVdecInfoData(acldvppStreamDesc *input, acldvppPicDesc *output, bool isSkipFlag);

        /**
         * Vdec launch task for send stream.
         * @param channelDesc[in] vdec channel desc
         * @param input[in] input stream desc
         * @param output[in] output pic desc
         * @param eos[in] eos flag
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchTaskForSendStream(aclvdecChannelDesc *channelDesc,
                                         acldvppStreamDesc *input,
                                         acldvppPicDesc *output,
                                         bool eos);

        /**
         * Vdec get callback information.
         * @param channelDesc[in] vdec channel desc
         * @param input[out] input stream desc
         * @param output[out] output pic desc
         * @param userData[out] user callback data
         * @param eos[out] eos flag
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError GetVdecCallInfo(aclvdecChannelDesc *channelDesc, acldvppStreamDesc *&input,
            acldvppPicDesc *&output, void *&userData, bool &eos);

        /**
         * Vdec set wait task type.
         * @param channelDesc[in] vdec channel desc
         * @return void
         */
        virtual void SetVdecWaitTaskType(aclvdecChannelDesc *channelDesc);

        /**
         * Venc set wait task type.
         * @param channelDesc[in] venc channel desc
         * @return void
         */
        virtual void SetVencWaitTaskType(aclvencChannelDesc *channelDesc);

    protected:
        // acl run mode
        static aclrtRunMode aclRunMode_;

    private:
        /**
         * Create and record all notify and stream for vdec.
         * @param channelDesc[in] vdec channel desc
         * @param isNeddNotify[in] whether create notify or not
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError CreateNotifyAndStreamForVdecChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify);

        /**
         * Destroy all notify and stream for vdec channel.
         * @param channelDesc[in] vdec channel desc
         * @param isNeedNotify[in] whether destroy notify or not
         * @return void
         */
        void DestroyAllNotifyAndStreamForVdecChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify);

        /**
         * Create vdec channel.
         * @param channelDesc[in] vdec channel desc
         * @param isNeddNotify[in] whether create notify or not
         * @param kernelName[in] kernel name
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError VdecCreateChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify, const char *kernelName);

        /**
         * Destroy vdec channel.
         * @param channelDesc[in] vdec channel desc
         * @param isNeddNotify[in] whether create notify or not
         * @param kernelName[in] kernel name
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError VdecDestroyChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify, const char *kernelName);

        /**
         * Create and record all notify for vdec.
         * @param channelDesc vdec channel desc
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError CreateNotifyForVdecChannel(aclvdecChannelDesc *channelDesc);

        /**
         * Create and record all event for vdec.
         * @param channelDesc vdec channel desc
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError CreateEventForVdecChannel(aclvdecChannelDesc *channelDesc);

        /**
         * Destroy all notify for vdec channel.
         * @param channelDesc[in] vdec channel desc
         * @return void
         */
        void DestroyAllNotifyForVdecChannel(aclvdecChannelDesc *channelDesc);

        /**
         * Destroy all event for vdec channel.
         * @param channelDesc[in] vdec channel desc
         * @return void
         */
        void DestroyAllEventForVdecChannel(aclvdecChannelDesc *channelDesc);

        /**
         * Send eos for vdec.
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for success, other for failed
         */
        aclError SendEosForVdec(aclvdecChannelDesc *channelDesc);

        /**
         * Vdec launch task for get stream.
         * @param channelDesc[in] vdec channel desc
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError LaunchTaskForGetStream(aclvdecChannelDesc *channelDesc);

        /**
         * Vdec launch send frame task.
         * @param channelDesc[in] vdec channel desc
         * @param inputStreamDesc[in] input stream desc
         * @param outputPicDesc[in] output stream desc
         * @param eos[in] end of sequence flag
         * @param kernelName[in] kernel name
         * @return ACL_SUCCESS for ok, others for fail
         */
        virtual aclError LaunchSendFrameTask(const aclvdecChannelDesc *channelDesc,
                                             acldvppStreamDesc *inputStreamDesc,
                                             acldvppPicDesc *outputPicDesc,
                                             bool eos,
                                             const char *kernelName);

        /**
         * Vdec launch get frame task.
         * @param channelDesc[in] vdec channel desc
         * @param kernelName[in] kernel name
         * @return ACL_SUCCESS for ok, others for fail
         */
        static aclError LaunchGetFrameTask(const aclvdecChannelDesc *channelDesc, const char *kernelName);

        /**
         * get vdec frame callback function for runtime.
         * @param callbackData[in] callback user data
         * @return void
         */
        static void GetVdecFrameCallback(void *callbackData);

        /**
         * Vdec create channel desc on host.
         * @return aclvdecChannelDesc
         */
        aclvdecChannelDesc *CreateVdecChannelDescOnHost();

        /**
         * Vdec create channel desc on device.
         * @return aclvdecChannelDesc
         */
        aclvdecChannelDesc *CreateVdecChannelDescOnDevice();

        /**
         * Dvpp create stream desc on host.
         * @return aclvdecChannelDesc
         */
        acldvppStreamDesc *CreateDvppStreamDescOnHost();

        /**
         * Dvpp create stream desc on device.
         * @return aclvdecChannelDesc
         */
        acldvppStreamDesc *CreateDvppStreamDescOnDevice();

        /**
         * Destroy venc stream and notify.
         */
        void DestroyVencAllNotifyAndStream(aclvencChannelDesc *channelDesc);

        /**
         * Destroy venc notify.
         */
        void DestroyVencAllNotify(aclvencChannelDesc *channelDesc);

        /**
         * Destroy venc event.
         */
        void DestroyVencAllEvent(aclvencChannelDesc *channelDesc);

        /**
         * Venc create get and send stream and notify.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError CreateNotifyAndStremForVencChannel(aclvencChannelDesc *channelDesc);

        /**
         * Venc create get and send frame notify.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError CreateNotifyForVencChannel(aclvencChannelDesc *channelDesc);

        /**
         * Venc create get and send frame event.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError CreateEventForVencChannel(aclvencChannelDesc *channelDesc);

        /**
         * Venc memcpy output from host to device.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError MemcpyDataToDevice(acldvppStreamDesc *output);

        /**
         * Venc choose mode, system and user, user not supported current.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError ChooseMemMode(aclvencChannelDesc *channelDesc, acldvppStreamDesc *&output);

        /**
         * Venc launch send frame task.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError LaunchVencSendFrameTask(const aclvencChannelDesc *channelDesc,
                                         acldvppPicDesc *inputPicDesc,
                                         acldvppStreamDesc *outputStreamDesc,
                                         aclvencFrameConfig *config);

        /**
         * get venc frame callback function.
         * @return ACL_SUCCESS for ok, others for fail
         */
        static void GetVencFrameCallback(void *callbackData);

        /**
         * Venc check parameter
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError CheckVencParameter(aclvencChannelDesc *channelDesc, acldvppPicDesc *input);

        /**
         * Venc set channelDesc parameter
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError SetVencParamToVencChannel(aclvencChannelDesc *channelDesc);

        aclError SetVdecParamToVdecChannel(aclvdecChannelDesc *channelDesc);
    };
    }
}
#endif // VIDEO_PROCESSOR_H
