/**
* @file group.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"
#include "securec.h"
#include "runtime/context.h"
#include "log_inner.h"
#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"


aclError aclrtSetGroup(int32_t groupId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSetGroup, groupId is %d.", groupId);
    rtError_t rtErr = rtSetGroup(groupId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("set group failed, runtime result = %d, groupId = %d",
            static_cast<int32_t>(rtErr), groupId);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSetGroup, groupId is %d.", groupId);

    return ACL_SUCCESS;
}

aclError aclrtGetGroupCount(uint32_t *count)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtGetGroupCount");
    ACL_REQUIRES_NOT_NULL(count);
    rtError_t rtErr = rtGetGroupCount(count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get group number failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtGetGroupCount, group number is %u.", *count);

    return ACL_SUCCESS;
}

aclrtGroupInfo *aclrtCreateGroupInfo()
{
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO);
    ACL_LOG_INFO("start to execute aclrtCreateGroupInfo");
    uint32_t count = 0;
    rtError_t rtErr = rtGetGroupCount(&count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get group number failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return nullptr;
    }
    if (count == 0) { // 0 represents that no group
        ACL_LOG_WARN("group number is 0, no memory allocation");
        return nullptr;
    }

    aclrtGroupInfo *groupInfo = new(std::nothrow) aclrtGroupInfo[count];
    if (groupInfo == nullptr) {
        ACL_LOG_ERROR("fail to new group info");
        return nullptr;
    }

    ACL_LOG_INFO("successfully execute aclrtCreateGroupInfo, group number is %u.", count);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO);

    return groupInfo;
}

aclError aclrtDestroyGroupInfo(aclrtGroupInfo *groupInfo)
{
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO);
    ACL_REQUIRES_NOT_NULL(groupInfo);
    ACL_DELETE_ARRAY_AND_SET_NULL(groupInfo);
    ACL_LOG_INFO("successfully execute aclrtDestroyGroupInfo");

    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO);

    return ACL_SUCCESS;
}

aclError aclrtGetAllGroupInfo(aclrtGroupInfo *groupInfo)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtGetAllGroupInfo");
    ACL_REQUIRES_NOT_NULL(groupInfo);
    uint32_t count = 0;
    rtError_t rtErr = rtGetGroupCount(&count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get group number failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    // -1 represents that get all group information
    rtErr = rtGetGroupInfo(-1, static_cast<rtGroupInfo_t *>(groupInfo), count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get group info failed, runtime result = %d, group number = %u",
            static_cast<int32_t>(rtErr), count);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("successfully execute aclrtGetAllGroupInfo, group number = %u", count);

    return ACL_SUCCESS;
}

static aclError FillAttrValue(const void *src, size_t srcLen, void *dst, size_t dstLen, size_t *paramRetSize)
{
    ACL_REQUIRES_NOT_NULL(src);
    ACL_REQUIRES_NOT_NULL(dst);
    if (srcLen > dstLen) {
        ACL_LOG_ERROR("attr real length = %zu is larger than input length = %zu", srcLen, dstLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    auto ret = memcpy_s(dst, dstLen, src, srcLen);
    if (ret != EOK) {
        ACL_LOG_ERROR("call memcpy_s failed, result = %d, srcLen = %zu, dstLen = %zu", ret, srcLen, dstLen);
        return ACL_ERROR_FAILURE;
    }
    *paramRetSize = srcLen;

    return ACL_SUCCESS;
}

aclError aclrtGetGroupInfoDetail(const aclrtGroupInfo *groupInfo, int32_t groupIndex, aclrtGroupAttr attr,
                                 void *attrValue, size_t valueLen, size_t *paramRetSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtGetGroupInfoDetail, groupIndex = %d", groupIndex);
    ACL_REQUIRES_NOT_NULL(groupInfo);
    ACL_REQUIRES_NOT_NULL(attrValue);
    ACL_REQUIRES_NOT_NULL(paramRetSize);
    uint32_t count = 0;
    rtError_t rtErr = rtGetGroupCount(&count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get group number failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    if ((groupIndex < 0) || (static_cast<uint32_t>(groupIndex) >= count)) {
        ACL_LOG_ERROR("the index value of group is invalid, groupIndex = %d, not in range [0, %u)", groupIndex, count);
        return ACL_ERROR_INVALID_PARAM;
    }

    aclError aclRet;
    switch (attr) {
        case ACL_GROUP_AICORE_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].aicoreNum),
                sizeof(groupInfo[groupIndex].aicoreNum), attrValue, valueLen, paramRetSize);
            break;
        case ACL_GROUP_AIV_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].aivectorNum),
                sizeof(groupInfo[groupIndex].aivectorNum), attrValue, valueLen, paramRetSize);
            break;
        case ACL_GROUP_AIC_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].aicpuNum),
                sizeof(groupInfo[groupIndex].aicpuNum), attrValue, valueLen, paramRetSize);
            break;
        case ACL_GROUP_SDMANUM_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].sdmaNum),
                sizeof(groupInfo[groupIndex].sdmaNum), attrValue, valueLen, paramRetSize);
            break;
        case ACL_GROUP_ASQNUM_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].activeStreamNum),
                sizeof(groupInfo[groupIndex].activeStreamNum), attrValue, valueLen, paramRetSize);
            break;
        case ACL_GROUP_GROUPID_INT:
            aclRet = FillAttrValue(static_cast<const void *>(&groupInfo[groupIndex].groupId),
                sizeof(groupInfo[groupIndex].groupId), attrValue, valueLen, paramRetSize);
            break;
        default:
            ACL_LOG_ERROR("invalid group attribute, attribute = %d", static_cast<int32_t>(attr));
            return ACL_ERROR_INVALID_PARAM;
    }

    ACL_LOG_INFO("end to execute aclrtGetGroupInfoDetail, groupIndex = %d", groupIndex);
    return aclRet;
}
