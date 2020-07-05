/**
 * @file ide_daemon_api.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.\n
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n
 *
 * 描述：算子dump接口头文件。\n
 */

/** @defgroup dump dump接口 */
#ifndef IDE_DAEMON_API_H
#define IDE_DAEMON_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup dump。
 *
 * dump ip信息缓冲区长度
 */
typedef void *IDE_SESSION;

/**
 * @ingroup dump。
 *
 * dump ip信息缓冲区长度
 */
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

/**
 * @ingroup dump。
 *
 * dump ip信息缓冲区长度
 */
#define IDE_DAEMON_IP_LEN (16)

/**
 * @ingroup dump。
 *
 * dump 连接信息
 */
typedef struct tagConnInfo {
    char ip[IDE_DAEMON_IP_LEN];     /**< IP地址 */
    int port;                       /**< 端口号 */
    int deviceID;                   /**< 设备ID号 */
} connInfo_t;

/**
 * @ingroup dump。
 *
 * dump 错误信息
 */
typedef enum tagIdeError {
    IDE_DAEMON_NONE_ERROR = 0,                    /**< 无错误 */
    IDE_DAEMON_UNKNOW_ERROR = 1,                  /**< 未知错误 */
    IDE_DAEMON_WRITE_ERROR = 2,                   /**< 写入失败 */
    IDE_DAEMON_NO_SPACE_ERROR = 3,                /**< 磁盘已满 */
    IDE_DAEMON_INVALID_PATH_ERROR = 4,            /**< 无效路径 */
    IDE_DAEMON_INVALID_PARAM_ERROR = 5,           /**< 无效参数 */
    IDE_DAEMON_TCP_CONNECT_ERROR = 6,             /**< TCP连接失败 */
    IDE_DAEMON_TCP_CHANNEL_ERROR = 7,             /**< TCP通道异常 */
    IDE_DAEMON_MALLOC_ERROR = 8,                  /**< 申请堆内存失败 */
    IDE_DAEMON_HDC_CHANNEL_ERROR = 9,             /**< HDC通路异常 */
    IDE_DAEMON_CHANNEL_ERROR = 10,                /**< 通路异常 */
    IDE_DAEMON_MKDIR_ERROR = 11,                  /**< 创建目录失败 */
    IDE_DAEMON_MEMCPY_ERROR = 12,                 /**< 内存拷贝失败 */
    IDE_DAEMON_MEMSET_ERROR = 13,                 /**< 内存清零失败 */
    IDE_DAEMON_INVALID_IP_ERROR = 14,             /**< 无效的IP地址 */
    IDE_DAEMON_INTERGER_REVERSED_ERROR = 15,      /**< 整形溢出 */
    IDE_DAEMON_DUMP_QUEUE_FULL = 16,              /**< dump队列已满 */
    NR_IDE_DAEMON_ERROR,                          /**< 枚举最大值 */
}ideError_t;

/**
 * @ingroup dump。
 *
 * dump 错误信息
 */
typedef ideError_t IdeErrorT;

/**
 * @ingroup dump。
 *
 * dump回传数据块标识信息
 */
enum IdeDumpFlag {
    IDE_DUMP_NONE_FLAG  = 0,            /**< 无标志位 */
};

/**
 * @ingroup dump。
 *
 * dump回传数据块
 */
struct IdeDumpChunk {
    char                *fileName;      /**< 文件名，绝对路径 */
    unsigned char       *dataBuf;       /**< 写入的数据Buffer */
    unsigned int        bufLen;         /**< 写入的数据Buffer长度 */
    unsigned int        isLastChunk;    /**< 是否最后一块数据   0:非最后一块数据;1：最后一块数据 */
    long long           offset;         /**< 文件写入的偏移位   -1为追加形式写入 */
    enum IdeDumpFlag    flag;           /**< 标志位 */
};

/**
 * @ingroup dump
 * @par 描述: 创建Dump通路。
 *
 * @attention 无
 * @param  privInfo [IN] 启动Dump通路数据(格式host:port;device_id(HDC), local;device_id(Local))
 * @retval #非空 创建会话成功
 * @retval #NULL 创建会话失败
 * @par 依赖:
 * @li ide_daemon_api.cpp：该接口所属的开发包。
 * @li ide_daemon_api.h：该接口声明所在的头文件。
 * @see 无
 * @since
 */
extern IDE_SESSION IdeDumpStart(const char *privInfo);

/**
 * @ingroup dump
 * @par 描述: 进行数据Dump,Dump完成数据落盘后返回。
 *
 * @attention 无
 * @param  session [IN] 会话句柄
 * @param  dumpChunk [IN] Dump的数据结构体
 * @retval #IDE_DAEMON_NONE_ERROR 写数据成功
 * @retval #IDE_DAEMON_INVALID_PARAM_ERROR 非法参数
 * @retval #IDE_DAEMON_UNKNOW_ERROR 写数据失败
 * @par 依赖:
 * @li ide_daemon_api.cpp：该接口所属的开发包。
 * @li ide_daemon_api.h：该接口声明所在的头文件。
 * @see 无
 * @since
 */
extern IdeErrorT IdeDumpData(IDE_SESSION session, const struct IdeDumpChunk *dumpChunk);

/**
 * @ingroup dump
 * @par 描述: 关闭Dump通路。
 *
 * @attention 无
 * @param  session [IN] 会话句柄
 * @retval #IDE_DAEMON_NONE_ERROR 关闭会话成功
 * @retval #IDE_DAEMON_INVALID_PARAM_ERROR 非法参数
 * @retval #IDE_DAEMON_UNKNOW_ERROR 关闭会话失败
 * @par 依赖:
 * @li ide_daemon_api.cpp：该接口所属的开发包。
 * @li ide_daemon_api.h：该接口声明所在的头文件。
 * @see 无
 * @since
 */
extern IdeErrorT IdeDumpEnd(IDE_SESSION session);

#ifdef __cplusplus
}
#endif

#endif
/*
 * History: \n
 * 2018-10-10, huawei, 初始化该文件。 \n
 * 2020-02-10, huawei, 更改API规范化。 \n
 *
 * vi: set expandtab ts=4 sw=4 tw=120:
 */
