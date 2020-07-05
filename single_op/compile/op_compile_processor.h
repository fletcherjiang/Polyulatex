/**
* @file op_compile_processor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef OP_COMPILE_PROCESSOR_H_
#define OP_COMPILE_PROCESSOR_H_

#include <mutex>

#include "types/acl_op.h"

namespace acl {
class OpCompileProcessor {
public:
    ~OpCompileProcessor();

    static OpCompileProcessor &GetInstance()
    {
        static OpCompileProcessor instance;
        return instance;
    }

    aclError OpCompile(AclOp &aclOp);

    void SetCompileOpt(std::string &opt, std::string &value);
    void GetGlobalCompileOpts(std::map<std::string, std::string> &currentOptions);
    void SetCompileFlag(int32_t flag);

private:
    OpCompileProcessor();

    aclError SetOption();

    aclError Init();

    bool isInit_ = false;
    std::mutex mutex_;

    std::map<std::string, std::string> globalCompileOpts{};
    std::mutex globalCompileOptsMutex;
};
} // namespace acl

#endif // OP_COMPILE_PROCESSOR_H_
