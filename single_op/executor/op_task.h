/**
* @file op_task.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TRUNK_PROJ_OP_TASK_H
#define TRUNK_PROJ_OP_TASK_H

#include "acl/acl.h"

#include "types/acl_op.h"

namespace acl {
class OpTask {
public:
    virtual aclError ExecuteAsync(int numInputs,
                                  const aclDataBuffer *const inputs[],
                                  int numOutputs,
                                  aclDataBuffer *const outputs[],
                                  aclrtStream stream) = 0;

    void SetArgs(std::unique_ptr<uint8_t[]> &&args, uint32_t argSize);

protected:
    uint32_t argSize_ = 0;
    std::unique_ptr<uint8_t[]> args_ = nullptr;
};

class TbeOpTask : public OpTask {
public:
    TbeOpTask(const void *stubFunc, uint32_t blockDim);
    ~TbeOpTask() = default;

    aclError ExecuteAsync(int numInputs,
                          const aclDataBuffer *const inputs[],
                          int numOutputs,
                          aclDataBuffer *const outputs[],
                          aclrtStream stream) override;

private:
    const void *stubFunc_ = nullptr;
    uint32_t blockDim_ = 1;
};
} // namespace acl
#endif // TRUNK_PROJ_OP_TASK_H
