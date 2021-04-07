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
    int32_t i = 0;
}

class UTEST_queue : public testing::Test
{
    public:
        UTEST_queue(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_queue, acltdtCreateQueueAttr_acltdtDestroyQueueAttr)
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

TEST_F(UTEST_queue, acltdtSetQueueAttr)
{
    size_t len = sizeof(size_t);
    const char* name = "123456789";
    auto ret = acltdtSetQueueAttr(nullptr, ACL_QUEUE_NAME_PTR, len, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueAttr attr = {0};
    ret = acltdtSetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = acltdtSetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, &name);
    std::string oriName(name);
    std::string tmpName(attr.name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(tmpName, oriName);

    char nameVec[] = "222";
    char *nameVecPtr = &nameVec[0];
    ret = acltdtSetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, &nameVecPtr);
    oriName = std::string(nameVec);
    tmpName = std::string(attr.name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(tmpName, oriName);

    // 129 bytes
    const char* nameFake = "66666666666666666666669666668888866666666666666666666668888888888888888844444444444444444444444444444444488888888888888888888888";
    
    ret = acltdtSetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, &nameFake);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    len = sizeof(uint32_t);
    uint32_t depth = 3;
    ret = acltdtSetQueueAttr(&attr, ACL_QUEUE_DEPTH_UINT32, len, &depth);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(depth, attr.depth);
}

TEST_F(UTEST_queue, acltdtGetQueueAttr)
{
    size_t len = sizeof(size_t);
    const char* name = nullptr;
    size_t retSize = 0;
    auto ret = acltdtGetQueueAttr(nullptr, ACL_QUEUE_NAME_PTR, len, &retSize, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueAttr attr = {0};
    ret = acltdtGetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, nullptr, &name);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    ret = acltdtGetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, &retSize, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    const char *preName = "1234";
    memcpy_s(attr.name, 128, preName, strlen(preName) + 1);
    string copyName = string(attr.name);
    EXPECT_EQ(copyName, "1234");
    ret = acltdtGetQueueAttr(&attr, ACL_QUEUE_NAME_PTR, len, &retSize, &name);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(string(name), copyName);
    EXPECT_EQ(retSize, len);

    attr.depth = 5;
    uint32_t depth = 9999;
    len = sizeof(uint32_t);
    ret = acltdtGetQueueAttr(&attr, ACL_QUEUE_DEPTH_UINT32, len, &retSize, &depth);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(depth, 5);
    EXPECT_EQ(retSize, len);
}

TEST_F(UTEST_queue, QueueRoute_create_destroy)
{
    acltdtQueueRoute *route =  acltdtCreateQueueRoute(1, 2);
    EXPECT_NE(route, nullptr);
    EXPECT_EQ(route->srcId, 1);
    EXPECT_EQ(route->dstId, 2);
    EXPECT_EQ(route->status, 0);
    auto ret = acltdtDestroyQueueRoute(route);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_queue, acltdtGetQueueRouteParam)
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

TEST_F(UTEST_queue, QueueRouteList_create_destroy)
{
    acltdtQueueRouteList *routeList =  acltdtCreateQueueRouteList();
    EXPECT_NE(routeList, nullptr);
    EXPECT_EQ(routeList->routeList.size(), 0);
    auto ret = acltdtDestroyQueueRouteList(routeList);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_queue, acltdtAddQueueRoute)
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

TEST_F(UTEST_queue, acltdtGetQueueRoute)
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
}

TEST_F(UTEST_queue, QueryQueueRoute_create_destroy)
{
    acltdtQueueRouteQueryInfo *info = acltdtCreateQueueRouteQueryInfo();
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->isConfigSrc, false);
    EXPECT_EQ(info->isConfigDst, false);
    EXPECT_EQ(info->isConfigMode, false);
    auto ret = acltdtDestroyQueueRouteQueryInfo(info);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_queue, acltdtSetQueueRouteQueryInfo)
{
    size_t len = sizeof(uint32_t);
    uint32_t src = 999;
    auto ret = acltdtSetQueueRouteQueryInfo(nullptr, ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, &src);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    acltdtQueueRouteQueryInfo *info = acltdtCreateQueueRouteQueryInfo();
    EXPECT_NE(info, nullptr);
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, nullptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    ret = acltdtSetQueueRouteQueryInfo(info, ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32, len, &src);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->srcId, 999);
    EXPECT_EQ(info->isConfigSrc, true);

    uint32_t dst = 888;
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32, len, &dst);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->dstId, 888);
    EXPECT_EQ(info->isConfigDst, true);

    acltdtQueueRouteQueryMode mode = ACL_TDT_QUEUE_ROUTE_QUERY_DST;
    ret = acltdtSetQueueRouteQueryInfo(info, ACL_QUEUE_ROUTE_QUERY_MODE_ENUM, sizeof(mode), &mode);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(info->mode, ACL_TDT_QUEUE_ROUTE_QUERY_DST);
    EXPECT_EQ(info->isConfigMode, true);

    ret = acltdtDestroyQueueRouteQueryInfo(info);
    EXPECT_EQ(ret, ACL_SUCCESS);
}


TEST_F(UTEST_queue, acltdtCreateQueue)
{
    uint32_t qid = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
         .WillOnce(Return((RT_ERROR_NONE)));
    auto ret = acltdtCreateQueue(nullptr, &qid);
}