/**
 * @file stackcore.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.\n
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n
 *
 * 描述：stackcore 头文件。\n
 */

/** @defgroup stackcore StackCore */
#ifndef LIB_STACKCORE_H
#define LIB_STACKCORE_H

/**
 * @ingroup stackcore
 * @brief init stackcore, which register signal hander for exception core
 */
#ifdef __cplusplus
extern "C"{
#endif
int StackInit();
#ifdef __cplusplus
}
#endif
#endif
