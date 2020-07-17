/**
* @file set_device_vxx.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_SET_DEVICE_VXX_H
#define ACL_SET_DEVICE_VXX_H

#include "acl/acl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup AscendCL
 * @brief Specify the device to use for the operation
 * implicitly create the default context and the default stream
 *
 * @par Function
 * The following use cases are supported:
 * @li Device can be specified in the process or thread.
 * If you call the aclrtSetDeviceWithoutTsdVXX interface multiple
 * times to specify the same device,
 * you only need to call the aclrtResetDeviceWithoutTsdVXX interface to reset the device.
 * @li The same device can be specified for operation
 *  in different processes or threads.
 * @li Device is specified in a process,
 * and multiple threads in the process can share this device to explicitly
 * create a Context (aclrtCreateContext interface).
 * @li In multi-device scenarios, you can switch to other devices
 * through the aclrtSetDeviceWithoutTsdVXX interface in the process.
 * @param  deviceId [IN]  the device id
 * @retval ACL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see aclrtResetDeviceWithoutTsdVXX |aclrtCreateContext
 */
ACL_FUNC_VISIBILITY aclError aclrtSetDeviceWithoutTsdVXX(int32_t deviceId);

/**
 * @ingroup AscendCL
 * @brief Specify the device to use for the operation
 * implicitly create the default context and the default stream
 *
 * @par Function
 * The following use cases are supported:
 * @li Device can be specified in the process or thread.
 * If you call the aclrtSetDeviceWithoutTsdVXX interface multiple
 * times to specify the same device,
 * you only need to call the aclrtResetDeviceWithoutTsdVXX interface to reset the device.
 * @li The same device can be specified for operation
 *  in different processes or threads.
 * @li Device is specified in a process,
 * and multiple threads in the process can share this device to explicitly
 * create a Context (aclrtCreateContext interface).
 * @li In multi-device scenarios, you can switch to other devices
 * through the aclrtSetDeviceWithoutTsdVXX interface in the process.
 * @param  deviceId [IN]  the device id
 * @retval ACL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see aclrtResetDeviceWithoutTsdVXX |aclrtCreateContext
 */
ACL_FUNC_VISIBILITY aclError aclrtResetDeviceWithoutTsdVXX(int32_t deviceId);

#ifdef __cplusplus
}
#endif
#endif //ACL_SET_DEVICE_VXX_H


