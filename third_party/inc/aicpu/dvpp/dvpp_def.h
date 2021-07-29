/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AICPU_INC_DVPP_KENERL_DEF_H_
#define AICPU_INC_DVPP_KENERL_DEF_H_

#include <cstdint>

namespace aicpu {
    namespace dvpp {
        namespace {
            constexpr uint32_t PiC_DESC_RESERVED_SIZE = 10;
            constexpr uint32_t STREAM_DESC_RESERVED_SIZE = 4;
            constexpr uint32_t DVPP_MAX_COMPONENT = 4;
        }

#pragma pack(push, 1)
        /**
         * dvpp picture description.
         */
        struct DvppPicDesc {
            uint64_t data = 0;          // data addr
            uint32_t format = 0;        // picture format.
            uint32_t width = 0;         // picture width
            uint32_t height = 0;        // picture height
            uint32_t widthStride = 0;   // picture width
            uint32_t heightStride = 0;  // picture height
            uint32_t size = 0;          // picture size.
            uint32_t retCode = 0;       // ret code
            uint16_t bitMap = 0;        // bit map
            uint8_t reserve[PiC_DESC_RESERVED_SIZE] = {0}; // reserve extend info
        };

        /**
         * dvpp batch picture description.
         */
        struct DvppBatchPicDesc {
              uint32_t batchSize = 0;            // batch size
              DvppPicDesc dvppPicDescs[1];       // batch picture desc
        };

        /**
         * dvpp roi config.
         * ROI(region of interest).
         */
        struct DvppRoiConfig {
            uint16_t leftOffset = 0;
            uint16_t rightOffset = 0;
            uint16_t upOffset = 0;
            uint16_t downOffset = 0;
        };

        /**
         * dvpp resize config.
         */
        struct DvppResizeConfig {
            uint32_t interpolation = 0;
            uint32_t len = 0;                  // extend length
            char extendInfo[0];                // extend info
        };

        /**
         * dvpp jpege config.
         */
        struct DvppJpegeConfig {
            uint32_t level = 0;
            uint32_t len = 0;                // extend length
            char extendInfo[0];              // extend info
        };

        /**
         * vdec channel desc.
         */
        struct DvppVdecDesc {
            uint32_t channelId = 0;                // decoding channel id: 0~15
            uint32_t enType = 0;                   // video encoding type
            uint32_t outPicFormat = 0;             // out pic format (DvppVpcOutputFormat)
            uint32_t outPicWidth = 0;              // out pic width (for vdec malloc memory only for 1951)
            uint32_t outPicHeight = 0;             // out pic height (for vdec malloc memory only for 1951)
            uint32_t outMode = 0;                  // out mode 0 : default mode 1: fast mode
            uint32_t refFrameNum = 0;              // refrence frame number (only for 1951)
            uint32_t sendFrameNotifyId = 0;        // notify id for send frame
            uint32_t getFrameNotifyId = 0;         // notify id for get frame
            uint32_t len = 0;                      // extend length
            char extendInfo[0];                    // extend info
        };

        /**
         * vpc/jpeg channel desc.
         */
        struct DvppChannelDesc {
            uint32_t notifyId = 0;             // notify id
            uint32_t channelMode = 0;          // channel mode
            uint32_t retCode = 0;              // result code
            uint32_t channelId = 0;            // 1:VPC 2:JPEGD 4:JPEGE
            uint32_t len = 0;                  // extend length
            char extendInfo[0];                // extend info
        };

        /**
         * vdec stream desc.
         */
        struct DvppStreamDesc {
            uint64_t data = 0;            // data addr
            uint32_t size = 0;            // frame size (vpc output pic size)
            uint32_t format = 0;          // stream format
            uint64_t timestamp = 0;       // time stamp
            bool eos = false;             // whether it is the end frame
            uint32_t retCode = 0;         // result code
            uint32_t reserve[STREAM_DESC_RESERVED_SIZE] = {0}; //reserve extend info
        };

        /**
         * venc channel desc.
         */
        struct DvppVencDesc {
            uint32_t channelId = 0;            // decoding channel id: default 0
            uint32_t enType = 0;               // video encoding type
            uint32_t picFormat = 0;            // input pic format
            uint32_t picWidth = 0;             // input pic width
            uint32_t picHeight = 0;            // input pic height
            uint32_t keyFrameInterval = 0;     // Interval of key frame
            uint32_t sendFrameNotifyId = 0;    // notify id for send frame
            uint32_t getFrameNotifyId = 0;         // notify id for get frame
            uint64_t bufAddr = 0;
            uint32_t bufSize = 0;
            uint32_t len = 0;                  //extend length
            char extendInfo[0];                //extend info
        };

        /**
         * vdec frame config.
         */
        struct DvppVdecFrameConfig {
            uint32_t reserved = 0;          // reserved param
            uint32_t len = 0;               // extend length
            char extendInfo[0];             // extend info
        };

        /**
         * vdec bit depth config.
         */
        struct DvppVdecBitDepthConfig {
            uint16_t paramType = 1;          // TLV type
            uint16_t paramLen = 4;           // TLV data len
            uint32_t bitDepth = 0;           // TLV bit depth
        };

        /**
         * dvpp csc matrix config.
         */
        struct DvppCscMatrixConfig {
            uint16_t paramType = 3;          // TLV type
            uint16_t paramLen = 4;           // TLV data len
            uint32_t cscMatrix = 0;          // TLV csc matrix
        };

        /**
         * venc frame config.
         */
        struct DvppVencFrameConfig {
            bool forceIFrame = false;           // whether force restart of I frame interval
            bool eos = false;                   // whether it is the end frame
            uint32_t len = 0;                   // extend length
            char extendInfo[0];                 // extend info
        };

        /**
         * lut map
         */
        struct DvppLutMap {
            uint8_t dims = 3;                  // lens dim,default 3
            uint16_t lens[DVPP_MAX_COMPONENT]; // lens of map
            uint8_t *map = nullptr;            // lut map
            uint32_t len = 0;                  // extend length
            char extendInfo[0];                // extend info
        };

        /**
         * dvpp border config
         */
        struct DvppBorderConfig {
            uint32_t borderType = 0;          // border type
            uint16_t top = 0;
            uint16_t bottom = 0;
            uint16_t left = 0;
            uint16_t right = 0;
            double value[DVPP_MAX_COMPONENT];
            uint32_t len = 0;
            char extendInfo[0];
        };

        /**
         * vpc hist desc.
         */
        struct DvppHistDesc {
            uint32_t dims = 3;                  // dims of hist, default is 3
            uint16_t lens[DVPP_MAX_COMPONENT]; // length of each dim, each len default is 256
            uint32_t retCode = 0;               // dvpp process return code
            uint32_t reserve[4];                // reserve buffer
            uint32_t *hist;                     // hist data buffer, hist[lens[0]+...+lens[dims-1]]
            uint32_t len = 0;                   // extend length
            char extendInfo[0];                 // extend info
        };

       /**
        * venc rate control
        */
       struct VencRateControl {
           uint16_t paramType = 1;             // TLV type, 1:VENC_TLV_RATE_CONTROL
           uint16_t paramLen = 12;             // TLV data len
           uint32_t rcMode = 0;                // rate control mode
           uint32_t srcRate = 0;               // source frame rate
           uint32_t maxBitRate = 0;            // max frame rate
       };

       /**
        * venc ip proportion
        */
       struct VencIpProportion {
           uint16_t paramType = 2;             // TLV type, 2:VENC_TLV_IP_PROPORTION
           uint16_t paramLen = 4;              // TLV data len
           uint32_t maxIpProp = 0;             // max ip prop
       };

#pragma pack(pop)

        // Supported Pixel Format
        enum DvppPixelFormat {
            PIXEL_FORMAT_YUV_400 = 0, // 0
            PIXEL_FORMAT_YUV_SEMIPLANAR_420 = 1, // 1
            PIXEL_FORMAT_YVU_SEMIPLANAR_420 = 2, // 2
            PIXEL_FORMAT_YUV_SEMIPLANAR_422 = 3, // 3
            PIXEL_FORMAT_YVU_SEMIPLANAR_422 = 4, // 4
            PIXEL_FORMAT_YUV_SEMIPLANAR_444 = 5, // 5
            PIXEL_FORMAT_YVU_SEMIPLANAR_444 = 6, // 6
            PIXEL_FORMAT_YUYV_PACKED_422 = 7, // 7
            PIXEL_FORMAT_UYVY_PACKED_422 = 8, // 8
            PIXEL_FORMAT_YVYU_PACKED_422 = 9, // 9
            PIXEL_FORMAT_VYUY_PACKED_422 = 10, // 10
            PIXEL_FORMAT_YUV_PACKED_444 = 11, // 11
            PIXEL_FORMAT_RGB_888 = 12, // 12
            PIXEL_FORMAT_BGR_888 = 13, // 13
            PIXEL_FORMAT_ARGB_8888 = 14, // 14
            PIXEL_FORMAT_ABGR_8888 = 15, // 15
            PIXEL_FORMAT_RGBA_8888 = 16, // 16
            PIXEL_FORMAT_BGRA_8888 = 17, // 17
            PIXEL_FORMAT_YUV_SEMI_PLANNER_420_10BIT = 18, // 18
            PIXEL_FORMAT_YVU_SEMI_PLANNER_420_10BIT = 19, // 19
            PIXEL_FORMAT_YVU_PLANAR_420 = 20, // 20
            PIXEL_FORMAT_YVU_PLANAR_422,
            PIXEL_FORMAT_YVU_PLANAR_444,
            PIXEL_FORMAT_RGB_444 = 23,
            PIXEL_FORMAT_BGR_444,
            PIXEL_FORMAT_ARGB_4444,
            PIXEL_FORMAT_ABGR_4444,
            PIXEL_FORMAT_RGBA_4444,
            PIXEL_FORMAT_BGRA_4444,
            PIXEL_FORMAT_RGB_555,
            PIXEL_FORMAT_BGR_555,
            PIXEL_FORMAT_RGB_565,
            PIXEL_FORMAT_BGR_565,
            PIXEL_FORMAT_ARGB_1555,
            PIXEL_FORMAT_ABGR_1555,
            PIXEL_FORMAT_RGBA_1555,
            PIXEL_FORMAT_BGRA_1555,
            PIXEL_FORMAT_ARGB_8565,
            PIXEL_FORMAT_ABGR_8565,
            PIXEL_FORMAT_RGBA_8565,
            PIXEL_FORMAT_BGRA_8565,
            PIXEL_FORMAT_RGB_BAYER_8BPP = 50,
            PIXEL_FORMAT_RGB_BAYER_10BPP,
            PIXEL_FORMAT_RGB_BAYER_12BPP,
            PIXEL_FORMAT_RGB_BAYER_14BPP,
            PIXEL_FORMAT_RGB_BAYER_16BPP,
            PIXEL_FORMAT_BGR_888_PLANAR = 70,
            PIXEL_FORMAT_HSV_888_PACKAGE,
            PIXEL_FORMAT_HSV_888_PLANAR,
            PIXEL_FORMAT_LAB_888_PACKAGE,
            PIXEL_FORMAT_LAB_888_PLANAR,
            PIXEL_FORMAT_S8C1,
            PIXEL_FORMAT_S8C2_PACKAGE,
            PIXEL_FORMAT_S8C2_PLANAR,
            PIXEL_FORMAT_S16C1,
            PIXEL_FORMAT_U8C1,
            PIXEL_FORMAT_U16C1,
            PIXEL_FORMAT_S32C1,
            PIXEL_FORMAT_U32C1,
            PIXEL_FORMAT_U64C1,
            PIXEL_FORMAT_S64C1,
            PIXEL_FORMAT_YUV_SEMIPLANAR_440 = 1000,
            PIXEL_FORMAT_YVU_SEMIPLANAR_440,
            PIXEL_FORMAT_FLOAT32,
            PIXEL_FORMAT_BUTT,
            PIXEL_FORMAT_UNKNOWN = 10000
        };

        // Support vdec TLV Type
        enum VdecTLVType {
            VDEC_TLV_UNUSED = 0,
            VDEC_TLV_BIT_DEPTH,
            VDEC_TLV_TYPE_SIZE
        };

        // Supported Stream Format
        enum DvppStreamFormat {
            H265_MAIN_LEVEL = 0,
            H264_BASELINE_LEVEL,
            H264_MAIN_LEVEL,
            H264_HIGH_LEVEL
        };

        // Supported Channel Mode
        enum DvppChannelMode {
            DVPP_CHNMODE_VPC = 1,
            DVPP_CHNMODE_JPEGD = 2,
            DVPP_CHNMODE_JPEGE = 4,
            DVPP_CHNMODE_PNGD = 8
        };

        // supported TLV Type
        enum DvppTlvType {
            VENC_TLV_RATE_CONTROL = 1,
            VENC_TLV_IP_PROPORTION = 2,
            DVPP_TLV_CSC_MATRIX = 3
        };
    }
}

#endif  // AICPU_INC_DVPP_KENERL_DEF_H_
