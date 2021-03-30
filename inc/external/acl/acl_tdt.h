/**
* @file acl_tdt.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_ACL_ACL_TDT_H_
#define INC_EXTERNAL_ACL_ACL_TDT_H_

#include "acl/acl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

enum acltdtTensorType {
    ACL_TENSOR_DATA_UNDEFINED = -1,
    ACL_TENSOR_DATA_TENSOR,
    ACL_TENSOR_DATA_END_OF_SEQUENCE,
    ACL_TENSOR_DATA_ABNORMAL
};

typedef struct acltdtDataItem acltdtDataItem;
typedef struct acltdtDataset acltdtDataset;
typedef struct acltdtChannelHandle acltdtChannelHandle;

/**
 * @ingroup AscendCL
 * @brief Get tensor type from item
 *
 * @param dataItem [IN] pointer to the data item
 *
 * @retval Tensor type.
 * @retval ACL_DT_UNDEFINED if dataItem is null
 */
ACL_FUNC_VISIBILITY acltdtTensorType acltdtGetTensorTypeFromItem(const acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get data type from item
 *
 * @param dataItem [IN] pointer to the data item
 *
 * @retval Data type.
 * @retval ACL_DT_UNDEFINED if dataItem is null
 */
ACL_FUNC_VISIBILITY aclDataType acltdtGetDataTypeFromItem(const acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get data address from item
 *
 * @param dataItem [IN] pointer to data item
 *
 * @retval null for failed
 * @retval OtherValues success
*/
ACL_FUNC_VISIBILITY void *acltdtGetDataAddrFromItem(const acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get data size from item
 *
 * @param dataItem [IN] pointer to data item
 *
 * @retval 0 for failed
 * @retval OtherValues success
*/
ACL_FUNC_VISIBILITY size_t acltdtGetDataSizeFromItem(const acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get dim's number from item
 *
 * @param dataItem [IN] pointer to data item
 *
 * @retval 0 for failed
 * @retval OtherValues success
*/
ACL_FUNC_VISIBILITY size_t acltdtGetDimNumFromItem(const acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get dims from item
 *
 * @param  dataItem [IN]      the struct of data item
 * @param  dims [IN|OUT]      pointer to the dims of dataTtem
 * @param  dimNum [IN]        the size of the dims
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 */
ACL_FUNC_VISIBILITY aclError acltdtGetDimsFromItem(const acltdtDataItem *dataItem, int64_t *dims, size_t dimNum);

/**
 * @ingroup AscendCL
 * @brief Create the struct of data item
 *
 * @param tdtType [IN]  Tdt tensor type
 * @param dims [IN]     pointer of tdtDataItem's dims
 * @param dimNum [IN]   Dim number
 * @param dataType [IN] Data type
 * @param data [IN]     Data pointer
 * @param size [IN]     Data size
 *
 * @retval null for failed
 * @retval OtherValues success
 *
 * @see acltdtDestroyDataItem
 */
ACL_FUNC_VISIBILITY acltdtDataItem *acltdtCreateDataItem(acltdtTensorType tdtType,
                                                         const int64_t *dims,
                                                         size_t dimNum,
                                                         aclDataType dataType,
                                                         void *data,
                                                         size_t size);

/**
 * @ingroup AscendCL
 * @brief Destroy the struct of data item
 *
 * @param dataItem [IN]  pointer to the data item
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtCreateDataItem
 */
ACL_FUNC_VISIBILITY aclError acltdtDestroyDataItem(acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Create the tdt dataset
 *
 * @retval null for failed
 * @retval OtherValues success
 *
 * @see acltdtDestroyDataset
 */
ACL_FUNC_VISIBILITY acltdtDataset *acltdtCreateDataset();

/**
 * @ingroup AscendCL
 * @brief Destroy the tdt dataset
 *
 * @param dataset [IN]  pointer to the dataset
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtCreateDataset
 */
ACL_FUNC_VISIBILITY aclError acltdtDestroyDataset(acltdtDataset *dataset);

/**
 * @ingroup AscendCL
 * @brief Get the data item
 *
 * @param dataset [IN] pointer to the dataset
 * @param index [IN]   index of the dataset
 *
 * @retval null for failed
 * @retval OtherValues success
 *
 * @see acltdtAddDataItem
 */
ACL_FUNC_VISIBILITY acltdtDataItem *acltdtGetDataItem(const acltdtDataset *dataset, size_t index);

/**
 * @ingroup AscendCL
 * @brief Get the data item
 *
 * @param dataset [OUT] pointer to the dataset
 * @param dataItem [IN] pointer to the data item
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtGetDataItem
 */
ACL_FUNC_VISIBILITY aclError acltdtAddDataItem(acltdtDataset *dataset, acltdtDataItem *dataItem);

/**
 * @ingroup AscendCL
 * @brief Get the size of dataset
 *
 * @param dataset [IN]  pointer to the dataset
 *
 * @retval 0 for failed
 * @retval OtherValues success
 */
ACL_FUNC_VISIBILITY size_t acltdtGetDatasetSize(const acltdtDataset *dataset);

/**
 * @ingroup AscendCL
 * @brief Stop the channel
 *
 * @param handle [IN]  pointer to the channel handle
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtCreateChannel | acltdtDestroyChannel
 */
ACL_FUNC_VISIBILITY aclError acltdtStopChannel(acltdtChannelHandle *handle);

/**
 * @ingroup AscendCL
 * @brief Create the channel
 *
 * @param deviceId [IN]  the device id
 * @param name [IN]      the channel's name
 *
 * @retval null for failed
 * @retval OtherValues success
 *
 * @see acltdtStopChannel | acltdtDestroyChannel
 */
ACL_FUNC_VISIBILITY acltdtChannelHandle *acltdtCreateChannel(uint32_t deviceId, const char *name);

/**
 * @ingroup AscendCL
 * @brief Destroy the channel
 *
 * @param handle [IN]  pointer to the channel handle
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtCreateChannel | acltdtStopChannel
 */
ACL_FUNC_VISIBILITY aclError acltdtDestroyChannel(acltdtChannelHandle *handle);

/**
 * @ingroup AscendCL
 * @brief Send tensor to device
 *
 * @param handle [IN]  pointer to the channel handle
 * @param dataset [IN] pointer to the dataset
 * @param timeout [IN] to be reserved, now it must be -1
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtReceiveTensor
 */
ACL_FUNC_VISIBILITY aclError acltdtSendTensor(const acltdtChannelHandle *handle,
                                              const acltdtDataset *dataset,
                                              int32_t timeout);

/**
 * @ingroup AscendCL
 * @brief Receive tensor from device
 *
 * @param handle [IN]      pointer to the channel handle
 * @param dataset [OUT]    pointer to the dataset
 * @param timeout [IN]     to be reserved, now it must be -1
 *
 * @retval ACL_SUCCESS  The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see acltdtSendTensor
 */
ACL_FUNC_VISIBILITY aclError acltdtReceiveTensor(const acltdtChannelHandle *handle,
                                                 acltdtDataset *dataset,
                                                 int32_t timeout);

typedef struct tagMemQueueAttr acltdtQueueAttr;
typedef struct acltdtBuf acltdtBuf;
typedef struct acltdtQueueRouteList acltdtQueueRouteList;
typedef struct acltdtQueueRouteQueryInfo acltdtQueueRouteQueryInfo;
typedef struct acltdtQueueRoute acltdtQueueRoute;

#define ACL_TDTQUEUE_PERMISSION_MANAGER 1
#define ACL_TDTQUEUE_PERMISSION_READ 2
#define ACL_TDTQUEUE_PERMISSION_WRITE 4


enum acltdtQueueAttrType {
    ACL_QUEUE_NAME_PTR = 0,
    ACL_QUEUE_DEPTH_UINT32
};

enum acltdtQueueRouteKind {
    ACL_QUEUE_SRC_ID = 0,
    ACL_QUEUE_DST_ID = 1
};

enum acltdtQueueRouteQueryMode {
    ACL_QUEUE_ROUTE_QUERY_SRC = 0,
    ACL_QUEUE_ROUTE_QUERY_DST,
    ACL_QUEUE_ROUTE_QUERY_SRC_DST
};

enum acltdtQueueRouteQueryInfoParamType {
    ACL_QUEUE_ROUTE_QUERY_MODE_ENUM = 0,
    ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32,
    ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32
};


ACL_FUNC_VISIBILITY aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueue(uint32_t qid);

ACL_FUNC_VISIBILITY aclError acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission);

ACL_FUNC_VISIBILITY aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                    acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY acltdtBuf* acltdtCreateBuf(size_t size);

ACL_FUNC_VISIBILITY aclError acltdtDestroyBuf(acltdtBuf *buf);

ACL_FUNC_VISIBILITY aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size);

ACL_FUNC_VISIBILITY aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size);

ACL_FUNC_VISIBILITY acltdtQueueAttr *acltdtCreateQueueAttr();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr);

ACL_FUNC_VISIBILITY aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                const void *param);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                size_t *paramRetSize,
                                                void *param);

ACL_FUNC_VISIBILITY acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQid, uint32_t dstQid);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY aclError acltdtGetqidFromQueueRoute(const acltdtQueueRoute *route,
                                                   acltdtQueueRouteKind srcDst,
                                                   uint32_t *qid);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus);

ACL_FUNC_VISIBILITY acltdtQueueRouteList* acltdtCreateQueueRouteList();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList);

ACL_FUNC_VISIBILITY aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                                                 size_t index,
                                                 acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY  acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param);

ACL_FUNC_VISIBILITY aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                                          acltdtQueueRouteQueryInfoParamType type,
                                                          size_t len,
                                                          const void *value);

#ifdef __cplusplus
}
#endif

#endif //INC_EXTERNAL_ACL_ACL_TDT_H_

