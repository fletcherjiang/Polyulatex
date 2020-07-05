#ifndef AICPU_TASK_STRUCT_H_
#define AICPU_TASK_STRUCT_H_

#include <cstdint> 

namespace aicpu {

#pragma pack(push, 1)
struct AicpuParamHead {
    uint32_t length;
    uint32_t ioAddrNum;
    uint32_t extInfoLength;
    uint64_t extInfoAddr;
};
#pragma pack(pop)

}

#endif