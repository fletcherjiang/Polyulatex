/**
 * @file retr.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "acl/ops/acl_fv.h"
#include <new>
#include "common/log_inner.h"
#include "acl/acl.h"
#include "single_op/retr/retr_internal.h"

namespace {
    constexpr uint32_t RETR_SHORT_FEATURE_LENGTH_36_B = 36;
    constexpr uint32_t RETR_REPO_RANGE_MAX = 1023;
    constexpr uint32_t RETR_QUERY_TABLE_LEN = 32768;  // 32k
    constexpr uint32_t RETR_FEATURE_COUNT_MAX = 10000000;
    constexpr uint32_t RETR_QUERY_COUNT_MAX = 1024;
    constexpr uint32_t RETR_TOPK_MAX = 4800;
    constexpr const uint64_t INIT_MAX_NUM = 600000000;
}  // namespace

template <typename Type>
void aclfvDestoryType(Type type)
{
    if (type != nullptr) {
        if (type->dataBuffer.data != nullptr) {
            aclError ret = aclrtFree(type->dataBuffer.data);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Free][Mem]free device mem failed, result = %d.", ret);
            }
            type->dataBuffer.data = nullptr;
        }
        ACL_DELETE_AND_SET_NULL(type);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

aclfvInitPara *aclfvCreateInitPara(uint64_t fsNum)
{
    ACL_LOG_INFO("aclfvCreateInitPara start.");
    if (fsNum == 0 || fsNum > INIT_MAX_NUM) {
        ACL_LOG_ERROR("[Check][FsNum]fsNum:%llu error, should not be equal 0 or greater than %llu.",
            fsNum, INIT_MAX_NUM);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("should not be equal 0 or greater than %llu.",
            INIT_MAX_NUM);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"fsNum", std::to_string(fsNum), errMsg}));
        return nullptr;
    }
    auto initPara = new (std::nothrow) aclfvInitPara();
    if (initPara == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][InitPara]create acl fv init param failed.");
        return nullptr;
    }

    initPara->initPara.fsNum = fsNum;

    void *devPtr = nullptr;
    size_t size = sizeof(initPara->initPara);
    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc device memory for acl fv init param failed, result = %d.", ret);
        ACL_DELETE_AND_SET_NULL(initPara);
        return nullptr;
    }
    initPara->dataBuffer.data = devPtr;
    initPara->dataBuffer.length = size;

    ACL_LOG_INFO("aclfvCreateInitPara success.");
    return initPara;
}

aclError aclfvDestroyInitPara(aclfvInitPara *initPara)
{
    ACL_LOG_INFO("aclfvDestroyInitPara start.");
    aclfvDestoryType(initPara);
    ACL_LOG_INFO("aclfvDestroyInitPara success.");
    return ACL_SUCCESS;
}

aclError aclfvSet1NTopNum(aclfvInitPara *initPara, uint32_t maxTopNumFor1N)
{
    ACL_LOG_INFO("aclfvSet1NTopNum start.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(initPara);
    // maxTopNumFor1N need in [2, 4800]
    if (maxTopNumFor1N < 2 || maxTopNumFor1N > 4800) {
        ACL_LOG_ERROR("[Check][MaxTopNumFor1N]maxTopNumFor1N[%u] should be between in [2, 4800].", maxTopNumFor1N);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"maxTopNumFor1N", std::to_string(maxTopNumFor1N),
            "should be between in [2, 4800]"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    initPara->initPara.maxTopNumFor1N = maxTopNumFor1N;
    ACL_LOG_INFO("aclfvSet1NTopNum success.");
    return ACL_SUCCESS;
}

aclError aclfvSetNMTopNum(aclfvInitPara *initPara, uint32_t maxTopNumForNM)
{
    ACL_LOG_INFO("aclfvSetNMTopNum start.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(initPara);
    // maxTopNumForNM need in [500, 4800]
    if (maxTopNumForNM < 500 || maxTopNumForNM > 4800) {
        ACL_LOG_ERROR("[Check][MaxTopNumForNM]maxTopNumForNM[%u] should be between in [500, 4800].", maxTopNumForNM);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"maxTopNumForNM", std::to_string(maxTopNumForNM),
            "should be between in [500, 4800]"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    initPara->initPara.maxTopNumForNM = maxTopNumForNM;
    ACL_LOG_INFO("aclfvSetNMTopNum success.");
    return ACL_SUCCESS;
}

aclfvFeatureInfo *aclfvCreateFeatureInfo(uint32_t id0, uint32_t id1, uint32_t offset, uint32_t featureLen,
    uint32_t featureCount, uint8_t *featureData, uint32_t featureDataLen)
{
    ACL_LOG_INFO("aclfvCreateFeatureInfo start.");
    if (featureData == nullptr) {
        ACL_LOG_ERROR("[Check][FeatureData]create acl retr feature info failed, featureData is null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"featureData"}));
        return nullptr;
    }

    if (id0 > RETR_REPO_RANGE_MAX || id1 > RETR_REPO_RANGE_MAX) {
        ACL_LOG_ERROR("[Check][Id]id0:%u or id1:%u is not in range[0-%u].", id0, id1, RETR_REPO_RANGE_MAX);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"id0", std::to_string(id0), "should be between in [500, 4800]"}));
        return nullptr;
    }

    if (featureLen != RETR_SHORT_FEATURE_LENGTH_36_B) {
        ACL_LOG_ERROR("[Check][featureLen]featureLen:%u should be %u.", featureLen,
            RETR_SHORT_FEATURE_LENGTH_36_B);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"featureLen", std::to_string(featureLen), "should be between in [500, 4800]"}));
        return nullptr;
    }

    if (featureCount > RETR_FEATURE_COUNT_MAX) {
        ACL_LOG_INNER_ERROR("[Check][FeatureCount]the value of featureCount[%u] can't be larger than "
            "RETR_FEATURE_COUNT_MAX[%u].", featureCount, RETR_FEATURE_COUNT_MAX);
        return nullptr;
    }

    if (featureDataLen != featureLen * featureCount) {
        ACL_LOG_INNER_ERROR("[Check][FeatureDataLen]featureDataLen:%u should be equal to featureLen:%u * featureCount:%u.",
            featureDataLen,
            featureLen,
            featureCount);
        return nullptr;
    }

    auto aclFeatureInfo = new (std::nothrow) aclfvFeatureInfo();
    if (aclFeatureInfo == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][FeatureInfo]create acl retr feature info failed.");
        return nullptr;
    }

    aclFeatureInfo->retrFeatureInfo.id0 = id0;
    aclFeatureInfo->retrFeatureInfo.id1 = id1;
    aclFeatureInfo->retrFeatureInfo.offset = offset;
    aclFeatureInfo->retrFeatureInfo.featureLen = featureLen;
    aclFeatureInfo->retrFeatureInfo.featureCount = featureCount;
    aclFeatureInfo->retrFeatureInfo.featureData = featureData;
    aclFeatureInfo->retrFeatureInfo.featureDataLen = featureDataLen;

    void *devPtr = nullptr;
    size_t size = sizeof(aclFeatureInfo->retrFeatureInfo);
    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][Mem]malloc device memory for acl retr feature info, result = %d.", ret);
        ACL_DELETE_AND_SET_NULL(aclFeatureInfo);
        return nullptr;
    }
    aclFeatureInfo->dataBuffer.data = devPtr;
    aclFeatureInfo->dataBuffer.length = size;

    ACL_LOG_INFO("aclfvCreateFeatureInfo success.");
    return aclFeatureInfo;
}

aclError aclfvDestroyFeatureInfo(aclfvFeatureInfo *featureInfo)
{
    ACL_LOG_INFO("aclfvDestroyFeatureInfo start.");
    if (featureInfo != nullptr) {
        if (featureInfo->dataBuffer.data != nullptr) {
            aclError ret = aclrtFree(featureInfo->dataBuffer.data);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Free][Mem]free device mem failed, result = %d.", ret);
            }
            featureInfo->dataBuffer.data = nullptr;
        }
        ACL_DELETE_AND_SET_NULL(featureInfo);
    }
    ACL_LOG_INFO("aclfvDestroyFeatureInfo success.");
    return ACL_SUCCESS;
}

aclfvRepoRange *aclfvCreateRepoRange(uint32_t id0Min, uint32_t id0Max, uint32_t id1Min, uint32_t id1Max)
{
    ACL_LOG_INFO("aclfvCreateRepoRange start.");

    if (id0Min > id0Max || id0Max > RETR_REPO_RANGE_MAX) {
        ACL_LOG_ERROR("[Check][Id]id0Min:%u or id0Max:%u of repoRange is not in range[0-%u] or "
            "id0Min is greater than id0Max.", id0Min, id0Max, RETR_REPO_RANGE_MAX);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("%u, %u", id0Min, id0Max);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"id0Min, id0Max", errMsg,
            "repoRange is not in range or id0Min is greater than id0Max"}));
        return nullptr;
    }

    if (id1Min > id1Max || id1Max > RETR_REPO_RANGE_MAX) {
        ACL_LOG_ERROR("[Check][Ids]id1Min:%u or id1Max:%u of repoRange is not in range[0-%u] or "
            "id1Min is greater than id1Max.", id1Min, id1Max, RETR_REPO_RANGE_MAX);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("%u, %u", id0Min, id0Max);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"id0Min, id0Max", errMsg,
            "repoRange is not in range or id1Min is greater than id1Max"}));
        return nullptr;
    }

    auto aclRepoRange = new (std::nothrow) aclfvRepoRange();
    if (aclRepoRange == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][RepoRange]create acl retr RepoRange failed.");
        return nullptr;
    }

    aclRepoRange->retrRepoRange.id0Min = id0Min;
    aclRepoRange->retrRepoRange.id0Max = id0Max;
    aclRepoRange->retrRepoRange.id1Min = id1Min;
    aclRepoRange->retrRepoRange.id1Max = id1Max;

    void *devPtr = nullptr;
    size_t size = sizeof(aclRepoRange->retrRepoRange);
    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc device memory for acl retr RepoRange, result = %d.", ret);
        ACL_DELETE_AND_SET_NULL(aclRepoRange);
        return nullptr;
    }
    aclRepoRange->dataBuffer.data = devPtr;
    aclRepoRange->dataBuffer.length = size;

    ACL_LOG_INFO("aclfvCreateRepoRange success.");
    return aclRepoRange;
}

aclError aclfvDestroyRepoRange(aclfvRepoRange *repoRange)
{
    ACL_LOG_INFO("aclfvDestroyRepoRange start.");
    if (repoRange != nullptr) {
        if (repoRange->dataBuffer.data != nullptr) {
            aclError ret = aclrtFree(repoRange->dataBuffer.data);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Free][Mem]free device mem failed, result = %d.", ret);
            }
            repoRange->dataBuffer.data = nullptr;
        }
        ACL_DELETE_AND_SET_NULL(repoRange);
    }
    ACL_LOG_INFO("aclfvDestroyRepoRange success.");
    return ACL_SUCCESS;
}

aclfvQueryTable *aclfvCreateQueryTable(uint32_t queryCnt, uint32_t tableLen, uint8_t *tableData, uint32_t tableDataLen)
{
    if (tableData == nullptr) {
        ACL_LOG_ERROR("[Check][TableData]create acl retr query table failed, table data is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"tableData"}));
        return nullptr;
    }

    if (tableLen != RETR_QUERY_TABLE_LEN) {
        ACL_LOG_ERROR("[Check][TableLen]tableLen:%u of query table should be 32K(%u).",
            tableLen, RETR_QUERY_TABLE_LEN);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("should be 32K(%u)",
            RETR_QUERY_TABLE_LEN);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"tableLen", std::to_string(tableLen), errMsg}));
        return nullptr;
    }

    if (queryCnt > RETR_QUERY_COUNT_MAX) {
        ACL_LOG_ERROR("[Check][QueryCnt]queryCnt[%u] can't be larger than RETR_QUERY_COUNT_MAX[%u].",
            queryCnt, RETR_QUERY_COUNT_MAX);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("can't be larger than RETR_QUERY_COUNT_MAX[%u]",
            RETR_QUERY_COUNT_MAX);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"queryCnt", std::to_string(queryCnt), errMsg}));
        return nullptr;
    }

    if (tableDataLen != queryCnt * tableLen) {
        ACL_LOG_ERROR("[Check][TableDataLen]tableDataLen:%u of query table should be equal to "
            "queryCnt:%u * tableLen:%u.", tableDataLen, queryCnt, tableLen);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("should be equal to queryCnt:%u * tableLen:%u",
            queryCnt, tableLen);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"tableDataLen", std::to_string(tableDataLen), errMsg}));
        return nullptr;
    }

    auto retrQueryTable = new (std::nothrow) aclfvQueryTable();
    if (retrQueryTable == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][RetrQueryTable]create acl retr query table failed.");
        return nullptr;
    }

    retrQueryTable->queryTable.queryCnt = queryCnt;
    retrQueryTable->queryTable.tableLen = tableLen;
    retrQueryTable->queryTable.tableData = tableData;
    retrQueryTable->queryTable.tableDataLen = tableDataLen;
    return retrQueryTable;
}

aclfvSearchInput *aclfvCreateSearchInput(aclfvQueryTable *queryTable, aclfvRepoRange *repoRange, uint32_t topK)
{
    if (queryTable == nullptr) {
        ACL_LOG_ERROR("[Check][QueryTable]create acl retr search input failed, query table is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"queryTable"}));
        return nullptr;
    }

    if (repoRange == nullptr) {
        ACL_LOG_ERROR("[Check][RepoRange]create acl retr search input failed, repo range is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"repoRange"}));
        return nullptr;
    }

    if (topK > RETR_TOPK_MAX) {
        ACL_LOG_ERROR("[Check][TopK]topK[%u] can't be larger than RETR_TOPK_MAX[%u].", topK, RETR_TOPK_MAX);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("can't be larger than RETR_TOPK_MAX[%u]",
            RETR_TOPK_MAX);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"topK", std::to_string(topK), errMsg}));
        return nullptr;
    }

    auto retrSearchInput = new (std::nothrow) aclfvSearchInput();
    if (retrSearchInput == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][RetrSearchInput]create acl retr search input failed.");
        return nullptr;
    }

    retrSearchInput->searchInput.queryTable = queryTable->queryTable;
    retrSearchInput->searchInput.repoRange = repoRange->retrRepoRange;
    retrSearchInput->searchInput.topK = topK;

    void *devPtr = nullptr;
    size_t size = sizeof(retrSearchInput->searchInput);
    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc device memory for acl retr search input failed, "
            "result = %d.", ret);
        ACL_DELETE_AND_SET_NULL(retrSearchInput);
        return nullptr;
    }
    retrSearchInput->dataBuffer.data = devPtr;
    retrSearchInput->dataBuffer.length = size;
    return retrSearchInput;
}

aclfvSearchResult *aclfvCreateSearchResult(uint32_t queryCnt, uint32_t *resultNum, uint32_t resultNumDataLen,
    uint32_t *id0, uint32_t *id1, uint32_t *resultOffset, float *resultDistance, uint32_t dataLen)
{
    if (resultNum == nullptr) {
        ACL_LOG_ERROR("[Check][Mem]create acl retr search result failed, "
            "resultNum is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"resultNum"}));
        return nullptr;
    }

    if (id0 == nullptr) {
        ACL_LOG_ERROR("[Create][id0]create acl retr search result failed, "
            "id0 is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"id0"}));
        return nullptr;
    }

    if (id1 == nullptr) {
        ACL_LOG_ERROR("[Check][id1]create acl retr search result failed, "
            "id1 is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"id1"}));
        return nullptr;
    }

    if (resultOffset == nullptr) {
        ACL_LOG_ERROR("[Check][resultDistance]create acl retr search result failed, "
            "resultOffset is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"resultOffset"}));
        return nullptr;
    }

    if (resultDistance == nullptr) {
        ACL_LOG_ERROR("create acl retr search result failed, resultDistance is nullptr.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"resultDistance"}));
        return nullptr;
    }

    if (queryCnt > RETR_QUERY_COUNT_MAX) {
        ACL_LOG_ERROR("[Check][queryCnt]queryCnt[%u] can't be larger than RETR_QUERY_COUNT_MAX[%u].",
            queryCnt, RETR_QUERY_COUNT_MAX);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("can't be larger than RETR_QUERY_COUNT_MAX[%u]",
            RETR_QUERY_COUNT_MAX);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"queryCnt", std::to_string(queryCnt), errMsg}));
        return nullptr;
    }

    if (resultNumDataLen != queryCnt * sizeof(uint32_t)) {
        ACL_LOG_ERROR("[Check][ResultNumDataLen]resultNumDataLen:%u of search result should be equal to "
            "queryCnt:%u * sizeof(uint32_t).", resultNumDataLen, queryCnt);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("earch result should be equal to "
            "queryCnt:%u * sizeof(uint32_t)", queryCnt);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"resultNumDataLen", std::to_string(resultNumDataLen), errMsg}));
        return nullptr;
    }

    auto retrSearchResult = new (std::nothrow) aclfvSearchResult();
    if (retrSearchResult == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][RetrSearchResult]create acl retr search result failed.");
        return nullptr;
    }

    retrSearchResult->searchResult.queryCnt = queryCnt;
    retrSearchResult->searchResult.resultNum = resultNum;
    retrSearchResult->searchResult.resultNumDataLen = resultNumDataLen;
    retrSearchResult->searchResult.id0 = id0;
    retrSearchResult->searchResult.id1 = id1;
    retrSearchResult->searchResult.resultOffset = resultOffset;
    retrSearchResult->searchResult.resultDistance = resultDistance;
    retrSearchResult->searchResult.dataLen = dataLen;

    void *devPtr = nullptr;
    size_t size = sizeof(retrSearchResult->searchResult);
    aclError ret = aclrtMalloc(&devPtr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][DevPtr]malloc device memory for acl retr search input failed, result = %d.", ret);
        ACL_DELETE_AND_SET_NULL(retrSearchResult);
        return nullptr;
    }
    retrSearchResult->dataBuffer.data = devPtr;
    retrSearchResult->dataBuffer.length = size;
    return retrSearchResult;
}

aclError aclfvDestroyQueryTable(aclfvQueryTable *retrQueryTable)
{
    ACL_DELETE_AND_SET_NULL(retrQueryTable);
    return ACL_SUCCESS;
}

aclError aclfvDestroySearchInput(aclfvSearchInput *retrSearchInput)
{
    if (retrSearchInput != nullptr) {
        if (retrSearchInput->dataBuffer.data != nullptr) {
            aclError ret = aclrtFree(retrSearchInput->dataBuffer.data);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][Mem]free device mem failed, result = %d.", ret);
            }
            retrSearchInput->dataBuffer.data = nullptr;
        }
        ACL_DELETE_AND_SET_NULL(retrSearchInput);
    }
    return ACL_SUCCESS;
}

aclError aclfvDestroySearchResult(aclfvSearchResult *retrSearchResult)
{
    if (retrSearchResult != nullptr) {
        if (retrSearchResult->dataBuffer.data != nullptr) {
            aclError ret = aclrtFree(retrSearchResult->dataBuffer.data);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Free][Mem]free device mem failed, result = %d.", ret);
            }
            retrSearchResult->dataBuffer.data = nullptr;
        }
        ACL_DELETE_AND_SET_NULL(retrSearchResult);
    }
    return ACL_SUCCESS;
}

#ifdef __cplusplus
}
#endif
