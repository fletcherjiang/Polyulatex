/**
* @file callback_info_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef _AICPU_DVPP_CALLBACK_INFO_MANAGER_H_
#define _AICPU_DVPP_CALLBACK_INFO_MANAGER_H_

#include <mutex>
#include <memory>
#include <unordered_map>
#include "acl/ops/acl_dvpp.h"

namespace aicpu {
    namespace dvpp {
    struct VdecGetFrameCallbackInfo;
    struct VencGetFrameCallbackInfo;
    // get frame callback info ptr
    using VdecCallbackInfoPtr = std::shared_ptr<VdecGetFrameCallbackInfo>;
    using VencCallbackInfoPtr = std::shared_ptr<VencGetFrameCallbackInfo>;

    // record call back info for vdec
    struct VdecGetFrameCallbackInfo {
        acldvppStreamDesc *inputStreamDesc = nullptr;    // input stream desc
        acldvppPicDesc *outputPicDesc = nullptr;         // output pic desc
        void *callbackData = nullptr;                    // callbackData set by user
        bool eos = false;                      // eos

        VdecGetFrameCallbackInfo(acldvppStreamDesc *input, acldvppPicDesc *output, void *callbackData, bool eos)
            : inputStreamDesc(input),
              outputPicDesc(output),
              callbackData(callbackData),
              eos(eos)
        {}
    };

    struct VencGetFrameCallbackInfo {
        aclvencCallback callbackFunc;          // callback func set by acl user
        acldvppPicDesc *inputPicDesc;          // input pic desc
        acldvppStreamDesc *outputStreamDesc;   // output stream desc
        uint8_t outputMemMode = 0;             // output pic memory module
        void *callbackData = nullptr;                    // callbackData set by user

        VencGetFrameCallbackInfo(aclvencCallback callbackFunc, acldvppPicDesc *input,
                                 acldvppStreamDesc *output, uint8_t memMode, void *callbackData)
            : callbackFunc(callbackFunc),
              inputPicDesc(input),
              outputStreamDesc(output),
              outputMemMode(memMode),
              callbackData(callbackData)
        {}
    };

    template<typename T>
    class CallbackInfoManager {
    public:
        static CallbackInfoManager &Instance();

        /**
         * erase callbackInfo from map.
         * @param uint64_t addr call back info addr
         * @return erased addr count
         */
        size_t Erase(uintptr_t addr);

        /**
         * take and remove callback info by addr.
         * @param addr callback info addr
         * @return callback info, if failed, return nullptr
         */
        T Take(uintptr_t addr);

        /**
         * insert callbackInfo to map.
         * @param uintptr_t addr call back info addr
         * @callbackInfo callback info ptr
         * @return true: success, false: failed
         */
        bool Insert(uintptr_t addr, T &callbackInfo);

        /**
         * get callback info by addr.
         * @param addr callback info addr
         * @return callback info, if failed, return nullptr
         */
        T Get(uintptr_t addr);

    private:
        CallbackInfoManager() = default;
        ~CallbackInfoManager() = default;

    private:
        // used for lock callbackInfos_
        mutable std::mutex mutex_;

        // keep callback addr and dvpp vdec desc info
        std::unordered_map<uintptr_t, T> callbackInfos_;
    };

    template<typename T>
    CallbackInfoManager<T> &CallbackInfoManager<T>::Instance()
    {
        static CallbackInfoManager instance;
        return instance;
    }

    /**
     * erase callbackInfo from map.
     * @param uint64_t addr call back info addr
     * @return erased addr count
     */
    template<typename T>
    size_t CallbackInfoManager<T>::Erase(uintptr_t addr)
    {
        std::lock_guard<std::mutex> mapLock(mutex_);
        return callbackInfos_.erase(addr);
    }

    /**
     * task callbackInfo from map.
     * @param uint64_t addr call back info addr
     * @return erased addr count
     */
    template<typename T>
    T CallbackInfoManager<T>::Take(uintptr_t addr)
    {
        std::lock_guard<std::mutex> mapLock(mutex_);
        auto iter = callbackInfos_.find(addr);
        if (iter == callbackInfos_.end()) {
            return nullptr;
        }
        auto callBackInfo = iter->second;
        (void) callbackInfos_.erase(iter);
        return callBackInfo;
    }

    /**
     * get callbackInfo from map.
     * @param uint64_t addr call back info addr
     * @return erased addr count
     */
    template<typename T>
    T CallbackInfoManager<T>::Get(uintptr_t addr)
    {
        std::lock_guard<std::mutex> mapLock(mutex_);
        auto iter = callbackInfos_.find(addr);
        if (iter == callbackInfos_.end()) {
            return nullptr;
        }
        return iter->second;
    }

    /**
     * insert callbackInfo to map.
     * @param uint64_t addr call back info addr
     * @callbackInfo callback info ptr
     * @return true: success, false: failed
     */
    template<typename T>
    bool CallbackInfoManager<T>::Insert(uintptr_t addr, T &callbackInfo)
    {
        std::lock_guard<std::mutex> mapLock(mutex_);
        callbackInfos_[addr] = callbackInfo;
        return true;
    }
    }
}

#endif