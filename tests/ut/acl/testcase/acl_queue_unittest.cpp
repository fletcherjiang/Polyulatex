#include "acl/acl_base.h"
#include "acl/acl.h"
#include "acl/acl_tdt_queue.h"
#include "log_inner.h"

#include "tensor_data_transfer/queue.h"
#include "tensor_data_transfer/queue_manager.h"
#include "tensor_data_transfer/queue_process.h"
#include "tensor_data_transfer/queue_process_mdc.h"
#include "tensor_data_transfer/queue_process_host.h"
#include "tensor_data_transfer/queue_process_ccpu.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "acl_stub.h"

#define protected public
#define private public
#undef private
#undef protected

using namespace testing;
using namespace std;
using namespace acl;

namespace acl {
    extern aclError CheckQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *queryInfo);
}

class UTEST_QUEUE : public testing::Test
{
    public:
        UTEST_QUEUE(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_QUEUE, acltdtCreateQueueAttr_acltdtDestroyQueueAttr)
{
    acltdtQueueAttr* attr = acltdtCreateQueueAttr();
    EXPECT_NE(attr, nullptr);
    EXPECT_EQ(string(attr->name).empty(), true);
    EXPECT_EQ(attr->depth, 2);
    EXPECT_EQ(attr->workMode, RT_MQ_MODE_DEFAULT);
    EXPECT_EQ(attr->flowCtrlFlag, false);
    EXPECT_EQ(attr->flowCtrlDropTime, 0);
    EXPECT_EQ(attr->overWriteFlag, false);
    auto ret = acltdtDestroyQueueAttr(attr);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtSetQueueAttr)
{
    size_t len = sizeof(size_t);
    const char* name = "123456789";
    auto ret = acltdtSetQueueAttr(nullptr, ACL_TDT_QUEUE_NAME_PTR, len, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueAttr attr = {0};
    ret = acltdtSetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = acltdtSetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, &name);
    std::string oriName(name);
    std::string tmpName(attr.name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(tmpName, oriName);

    char nameVec[] = "222";
    char *nameVecPtr = &nameVec[0];
    ret = acltdtSetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, &nameVecPtr);
    oriName = std::string(nameVec);
    tmpName = std::string(attr.name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(tmpName, oriName);

    // 129 bytes
    const char* nameFake = "66666666666666666666669666668888866666666666666666666668888888888888888844444444444444444444444444444444488888888888888888888888";
    
    ret = acltdtSetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, &nameFake);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    len = sizeof(uint32_t);
    uint32_t depth = 3;
    ret = acltdtSetQueueAttr(&attr, ACL_TDT_QUEUE_DEPTH_UINT32, len, &depth);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(depth, attr.depth);
}

TEST_F(UTEST_QUEUE, acltdtGetQueueAttr)
{
    size_t len = sizeof(size_t);
    const char* name = nullptr;
    size_t retSize = 0;
    auto ret = acltdtGetQueueAttr(nullptr, ACL_TDT_QUEUE_NAME_PTR, len, &retSize, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueAttr attr = {0};
    ret = acltdtGetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, nullptr, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtGetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, &retSize, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    const char *preName = "1234";
    memcpy_s(attr.name, 128, preName, strlen(preName) + 1);
    string copyName = string(attr.name);
    EXPECT_EQ(copyName, "1234");
    ret = acltdtGetQueueAttr(&attr, ACL_TDT_QUEUE_NAME_PTR, len, &retSize, &name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(string(name), copyName);
    EXPECT_EQ(retSize, len);

    attr.depth = 5;
    uint32_t depth = 9999;
    len = sizeof(uint32_t);
    ret = acltdtGetQueueAttr(&attr, ACL_TDT_QUEUE_DEPTH_UINT32, len, &retSize, &depth);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(depth, 5);
    EXPECT_EQ(retSize, len);
}

TEST_F(UTEST_QUEUE, QueueRoute_create_destroy)
{
    acltdtQueueRoute *route =  acltdtCreateQueueRoute(1, 2);
    EXPECT_NE(route, nullptr);
    EXPECT_EQ(route->srcId, 1);
    EXPECT_EQ(route->dstId, 2);
    EXPECT_EQ(route->status, 0);
    auto ret = acltdtDestroyQueueRoute(route);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtGetQueueRouteParam)
{
    size_t len = sizeof(size_t);
    uint32_t src = 9999;
    size_t retSize = 0;
    auto ret = acltdtGetQueueRouteParam(nullptr, ACL_TDT_QUEUE_ROUTE_SRC_UINT32, len, &retSize, &src);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueRoute route = {0};
    ret = acltdtGetQueueRouteParam(&route, ACL_TDT_QUEUE_ROUTE_SRC_UINT32, len, nullptr, &src);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtGetQueueRouteParam(&route, ACL_TDT_QUEUE_ROUTE_SRC_UINT32, len, &retSize, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    route.srcId = 666;
    route.dstId = 999;
    route.status = 1;
    ret = acltdtGetQueueRouteParam(&route, ACL_TDT_QUEUE_ROUTE_SRC_UINT32, len, &retSize, &src);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(retSize, 4);
    EXPECT_EQ(src, 666);

    uint32_t dst = 888;
    ret = acltdtGetQueueRouteParam(&route, ACL_TDT_QUEUE_ROUTE_DST_UINT32, len, &retSize, &dst);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(retSize, 4);
    EXPECT_EQ(dst, 999);

    int32_t status = 0;
    ret = acltdtGetQueueRouteParam(&route, ACL_TDT_QUEUE_ROUTE_STATUS_INT32, len, &retSize, &status);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(retSize, 4);
    EXPECT_EQ(status, 1);
}

TEST_F(UTEST_QUEUE, QueueRouteList_create_destroy)
{
    acltdtQueueRouteList *routeList =  acltdtCreateQueueRouteList();
    EXPECT_NE(routeList, nullptr);
    EXPECT_EQ(routeList->routeList.size(), 0);
    auto ret = acltdtDestroyQueueRouteList(routeList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtAddQueueRoute)
{
    acltdtQueueRouteList routeList;
    acltdtQueueRoute route = {0};
    auto ret = acltdtAddQueueRoute(nullptr, &route);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtAddQueueRoute(&routeList, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    route.srcId = 111;
    route.dstId = 222;
    ret = acltdtAddQueueRoute(&routeList, &route);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(routeList.routeList.size(), 1);
    EXPECT_EQ(routeList.routeList[0].srcId, 111);
    EXPECT_EQ(routeList.routeList[0].dstId, 222);
    ret = acltdtAddQueueRoute(&routeList, &route);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(routeList.routeList.size(), 2);
}

TEST_F(UTEST_QUEUE, acltdtGetQueueRoute)
{
    acltdtQueueRouteList routeList;
    acltdtQueueRoute route = {0};
    auto ret = acltdtGetQueueRoute(nullptr, 0, &route);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtGetQueueRoute(&routeList, 0, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtGetQueueRoute(&routeList, 0, &route);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    acltdtQueueRoute tmpRoute = {111, 222, 0};
    routeList.routeList.push_back(tmpRoute);
    ret = acltdtGetQueueRoute(&routeList, 0, &route);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(route.srcId, 111);
    EXPECT_EQ(route.dstId, 222);
    EXPECT_EQ(route.status, 0);
    size_t size = acltdtGetQueueRouteNum(nullptr);
    EXPECT_EQ(size, 0);
    size = acltdtGetQueueRouteNum(&routeList);
    EXPECT_EQ(size, 1);
}

TEST_F(UTEST_QUEUE, CheckQueueRouteQueryInfo)
{
    acltdtQueueRouteQueryInfo queryInfo = {0};
    queryInfo.srcId = 0;
    queryInfo.dstId = 1;
    queryInfo.isConfigMode = false;
    auto ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    queryInfo.mode = ACL_TDT_QUEUE_ROUTE_QUERY_SRC;
    queryInfo.isConfigMode = true;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    queryInfo.isConfigSrc = false;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    queryInfo.isConfigSrc = true;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);

    queryInfo.mode = ACL_TDT_QUEUE_ROUTE_QUERY_DST;
    queryInfo.isConfigDst = false;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    queryInfo.isConfigDst = true;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    queryInfo.mode = ACL_TDT_QUEUE_ROUTE_QUERY_SRC_AND_DST;
    queryInfo.isConfigDst = false;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    queryInfo.isConfigDst = true;
    queryInfo.isConfigSrc = true;
    ret = CheckQueueRouteQueryInfo(&queryInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, QueryQueueRoute_create_destroy)
{
    acltdtQueueRouteQueryInfo *info = acltdtCreateQueueRouteQueryInfo();
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->isConfigSrc, false);
    EXPECT_EQ(info->isConfigDst, false);
    EXPECT_EQ(info->isConfigMode, false);
    auto ret = acltdtDestroyQueueRouteQueryInfo(info);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtSetQueueRouteQueryInfo)
{
    size_t len = sizeof(uint32_t);
    uint32_t src = 999;
    auto ret = acltdtSetQueueRouteQueryInfo(nullptr, ACL_TDT_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, &src);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueRouteQueryInfo *info = acltdtCreateQueueRouteQueryInfo();
    EXPECT_NE(info, nullptr);
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_TDT_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = acltdtSetQueueRouteQueryInfo(info, ACL_TDT_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, &src);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->srcId, 999);
    EXPECT_EQ(info->isConfigSrc, true);

    uint32_t dst = 888;
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_TDT_QUEUE_ROUTE_QUERY_DST_ID_UINT32, len, &dst);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->dstId, 888);
    EXPECT_EQ(info->isConfigDst, true);

    acltdtQueueRouteQueryMode mode = ACL_TDT_QUEUE_ROUTE_QUERY_DST;
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_TDT_QUEUE_ROUTE_QUERY_MODE_ENUM, sizeof(mode), &mode);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->mode, ACL_TDT_QUEUE_ROUTE_QUERY_DST);
    EXPECT_EQ(info->isConfigMode, true);

    ret = acltdtDestroyQueueRouteQueryInfo(info);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

// host chip default 
TEST_F(UTEST_QUEUE, acltdtAllocBuf_host)
{
    acltdtBuf buf = nullptr;
    size_t size = 100;
    auto ret = acltdtAllocBuf(size, &buf);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(UTEST_QUEUE, acltdtFreeBuf_host)
{
    acltdtBuf buf = nullptr;
    auto ret = acltdtFreeBuf(buf);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(UTEST_QUEUE, acltdtGetBufData_host)
{
    acltdtBuf buf = nullptr;
    void *dataPtr = nullptr;
    size_t size = 0;
    auto ret = acltdtGetBufData(buf, &dataPtr, &size);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(UTEST_QUEUE, acltdtEnqueue_host)
{
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 3000;
    auto ret = acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(UTEST_QUEUE, acltdtDequeue_host)
{
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 3000;
    auto ret = acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(UTEST_QUEUE, acltdtCreateQueue_host)
{
    uint32_t qid = 0;
    acltdtQueueAttr attr;
    auto ret = acltdtCreateQueue(&attr, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtCreateQueue(nullptr, &qid);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtCreateQueue(&attr, &qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtDestroyQueue_host)
{
    uint32_t qid = 0;
    auto ret = acltdtDestroyQueue(qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtGrantQueue_host)
{
    uint32_t qid = 0;
    int32_t pid = 888;
    uint32_t permission = 2;
    int32_t timeout = 0;
    auto ret = acltdtGrantQueue(qid, pid, permission, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtAttachQueue_host)
{
    uint32_t qid = 0;
    uint32_t permission = 1;
    int32_t timeout = 0;
    auto ret = acltdtAttachQueue(qid, timeout, &permission);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtBindQueueRoutes_host)
{
    acltdtQueueRouteList qRouteList;
    acltdtQueueRoute route = {0};
    qRouteList.routeList.push_back(route);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFree(_))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    auto ret = acltdtBindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtUnbindQueueRoutes_host)
{
    acltdtQueueRouteList qRouteList;
    acltdtQueueRoute route = {0};
    qRouteList.routeList.push_back(route);
    auto ret = acltdtUnbindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtQueryQueueRoutes_host)
{
    acltdtQueueRouteList qRouteList;
    acltdtQueueRouteQueryInfo queryInfo;
    acltdtQueueRoute route = {0};
    qRouteList.routeList.push_back(route);
    auto ret = acltdtQueryQueueRoutes(&queryInfo, &qRouteList);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    queryInfo.mode = ACL_TDT_QUEUE_ROUTE_QUERY_SRC_AND_DST;
    queryInfo.srcId = 0;
    queryInfo.dstId = 1;
    queryInfo.isConfigDst = true;
    queryInfo.isConfigMode = true;
    queryInfo.isConfigSrc = true;
    ret = acltdtQueryQueueRoutes(&queryInfo, &qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}


// mdc chip
TEST_F(UTEST_QUEUE, acltdtAllocBuf_mdc)
{
    acltdtBuf buf = nullptr;
    size_t size = 100;
    void *dataPtr = nullptr;
    QueueProcessorMdc queueProcess;
    auto ret = queueProcess.acltdtGetBufData(buf, &dataPtr, &size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = queueProcess.acltdtAllocBuf(size, &buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(buf, nullptr);

    ret = queueProcess.acltdtGetBufData(buf, &dataPtr, &size);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(dataPtr, nullptr);

    ret = queueProcess.acltdtFreeBuf(buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtEnqueue_mdc)
{
    QueueProcessorMdc queueProcess;
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 30;
    auto ret = queueProcess.acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    size_t size = 10;
    ret = queueProcess.acltdtAllocBuf(size, &buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(buf, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemQueueEnQueue(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return((ACL_ERROR_RT_QUEUE_FULL)));
    ret = queueProcess.acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    ret = queueProcess.acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = queueProcess.acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);

    ret = queueProcess.acltdtFreeBuf(buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtDequeue_mdc)
{
    QueueProcessorMdc queueProcess;
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 30;
    auto ret = queueProcess.acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemQueueDeQueue(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return((ACL_ERROR_RT_QUEUE_EMPTY)));
    ret = queueProcess.acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    ret = queueProcess.acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
}

TEST_F(UTEST_QUEUE, acltdtCreateQueue_mdc)
{
    QueueProcessorMdc queueProcess;
    uint32_t qid = 0;
    acltdtQueueAttr attr;
    auto ret = queueProcess.acltdtCreateQueue(&attr, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = queueProcess.acltdtCreateQueue(nullptr, &qid);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = queueProcess.acltdtCreateQueue(&attr, &qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtDestroyQueue_mdc)
{
    QueueProcessorMdc queueProcess;
    uint32_t qid = 0;
    auto ret = queueProcess.acltdtDestroyQueue(qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtGrantQueue_mdc)
{
    QueueProcessorMdc queueProcess;
    uint32_t qid = 0;
    int32_t pid = 888;
    uint32_t permission = 2;
    int32_t timeout = 0;
    auto ret = queueProcess.acltdtGrantQueue(qid, pid, permission, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtAttachQueue_mdc)
{
    QueueProcessorMdc queueProcess;
    uint32_t qid = 0;
    uint32_t permission = 1;
    int32_t timeout = 0;
    auto ret = queueProcess.acltdtAttachQueue(qid, timeout, &permission);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtBindQueueRoutes_mdc)
{
    QueueProcessorMdc queueProcess;
    acltdtQueueRouteList qRouteList;
    acltdtQueueRoute route = {0};
    qRouteList.routeList.push_back(route);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemQueueEnQueue(_, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    auto ret = queueProcess.acltdtBindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtUnbindQueueRoutes_mdc)
{
    QueueProcessorMdc queueProcess;
    acltdtQueueRouteList qRouteList;
    acltdtQueueRoute route = {0};
    qRouteList.routeList.push_back(route);
    auto ret = queueProcess.acltdtUnbindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtQueryQueueRoutes_mdc)
{
    QueueProcessorMdc queueProcess;
    acltdtQueueRouteList qRouteList;
    acltdtQueueRouteQueryInfo queryInfo;
    auto ret = queueProcess.acltdtQueryQueueRoutes(&queryInfo, &qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

// ccpu chip
TEST_F(UTEST_QUEUE, acltdtAllocBuf_ccpu)
{
    acltdtBuf buf = nullptr;
    size_t size = 100;
    QueueProcessorCcpu queueProcess;
    auto ret = queueProcess.acltdtAllocBuf(size, &buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtFreeBuf_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtBuf buf = nullptr;
    auto ret = queueProcess.acltdtFreeBuf(buf);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtGetBufData_ccpu)
{
    acltdtBuf buf = nullptr;
    void *dataPtr = nullptr;
    size_t size = 0;
    QueueProcessorCcpu queueProcess;
    auto ret = queueProcess.acltdtGetBufData(buf, &dataPtr, &size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(UTEST_QUEUE, acltdtEnqueue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 3000;
    auto ret = queueProcess.acltdtEnqueue(qid, buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(UTEST_QUEUE, acltdtDequeue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtBuf buf = nullptr;
    uint32_t qid = 0;
    int32_t timeout = 30;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemQueueDeQueue(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return((ACL_ERROR_RT_QUEUE_EMPTY)));
    auto ret = queueProcess.acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    ret = queueProcess.acltdtDequeue(qid, &buf, timeout);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
}

TEST_F(UTEST_QUEUE, acltdtCreateQueue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    uint32_t qid = 0;
    acltdtQueueAttr attr;
    auto ret = queueProcess.acltdtCreateQueue(&attr, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = queueProcess.acltdtCreateQueue(nullptr, &qid);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = queueProcess.acltdtCreateQueue(&attr, &qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtDestroyQueue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    uint32_t qid = 0;
    auto ret = queueProcess.acltdtDestroyQueue(qid);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtGrantQueue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    uint32_t qid = 0;
    int32_t pid = 888;
    uint32_t permission = 2;
    int32_t timeout = 0;
    auto ret = queueProcess.acltdtGrantQueue(qid, pid, permission, timeout);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtAttachQueue_ccpu)
{
    QueueProcessorCcpu queueProcess;
    uint32_t qid = 0;
    uint32_t permission = 1;
    int32_t timeout = 0;
    auto ret = queueProcess.acltdtAttachQueue(qid, timeout, &permission);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtBindQueueRoutes_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtQueueRouteList qRouteList;
    auto ret = queueProcess.acltdtBindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtUnbindQueueRoutes_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtQueueRouteList qRouteList;
    auto ret = queueProcess.acltdtUnbindQueueRoutes(&qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_QUEUE, acltdtQueryQueueRoutes_ccpu)
{
    QueueProcessorCcpu queueProcess;
    acltdtQueueRouteList qRouteList;
    acltdtQueueRouteQueryInfo queryInfo;
    auto ret = queueProcess.acltdtQueryQueueRoutes(&queryInfo, &qRouteList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}