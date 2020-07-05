#ifndef AICPU_INC_RETR_KERNEL_DEF_H_
#define AICPU_INC_RETR_KERNEL_DEF_H_

#include <cstdint>

namespace aicpu {
namespace retr {

#pragma pack(push, 1)
struct AicpuFvInitPara {
    uint64_t fsNum = 0;
    uint32_t maxTopNumFor1N = 4800;
    uint32_t maxTopNumForNM = 500;
    uint32_t len = 0;
    int32_t retCode = -2;
    char extend_info[0];
};

struct AicpuFvFeatureInfo {
    uint32_t id0 = 0;
    uint32_t id1 = 0;
    uint32_t offset = 0;
    uint32_t featureLen = 0;
    uint32_t featureCount = 0;
    uint8_t *featureData = nullptr;
    uint32_t featureDataLen = 0;
    int32_t retCode = -2;
};

struct AicpuFvQueryTable {
    uint32_t queryCnt = 0;
    uint32_t tableLen = 0;
    uint8_t *tableData = nullptr;
    uint32_t tableDataLen = 0;
};

struct AicpuFvRepoRange {
    uint32_t id0Min = 0;
    uint32_t id0Max = 0;
    uint32_t id1Min = 0;
    uint32_t id1Max = 0;
    uint32_t retCode = -2;
};

struct AicpuFvSearchInput {
    AicpuFvQueryTable queryTable;
    AicpuFvRepoRange repoRange;
    uint32_t topK = 0;
};

struct AicpuFvSearchResult {
    uint32_t queryCnt = 0;
    uint32_t *resultNum = nullptr;
    uint32_t resultNumDataLen = 0;
    uint32_t *id0 = nullptr;
    uint32_t *id1 = nullptr;
    uint32_t *resultOffset = nullptr;
    float *resultDistance = nullptr;
    uint32_t dataLen = 0;
    uint32_t retCode = -2;
};
#pragma pack(pop)
}
}

#endif

