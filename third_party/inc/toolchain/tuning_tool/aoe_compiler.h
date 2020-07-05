/**
 * @file aoe_compiler.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef AOE_COMPILER_H
#define AOE_COMPILER_H

#include <map>
#include <string>
#include "aoe_types.h"
#include "graph/graph.h"
#include "ge/ge_api.h"
#include "tune_api.h"

#ifdef TUNING_TOOL_OFFLINE
#include "ge/ge_ir_build.h"

/**
 * @brief       Init build environment, only call once, concurrency is not supported
 * @param [in]  std::map<std::string, std::string>  &options    init option
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOfflineCompilerInit(const std::map<std::string, std::string> &options);

/**
 * @brief       build model
 * @param [in]  Graph &graph                                    graph to build
 * @param [in]  std::map<std::string, std::string>  &options    build option
 * @param [out] ModelBufferData &data                           model data
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOfflineCompile(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    ge::ModelBufferData &data);

/**
 * @brief       build model with raw data
 * @param [in]  AoeDataInfo &graph                              graph to build
 * @param [in]  std::map<std::string, std::string>  &options    build option
 * @param [out] AOEBufferData &data                             model data
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOfflineCompileModel(const AoeDataInfo &graph,
    const std::map<std::string, std::string> &options, AOEBufferData &data);

/**
 * @brief       save model data to model file
 * @param [in]  ModelBufferData &data                           model data
 * @param [in]  TPath omPath                                    model file path
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOfflineSaveModel(ge::ModelBufferData &data, const std::string &omPath);

/**
 * @brief       split graph to sub graph
 * @param [in]  Graph &graph                                    graph to build
 * @param [in]  std::map<std::string, std::string>  &options    build option
 * @param [in]  TPath filePath                                  sub graph file path
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOfflineSplitGraph(ge::Graph &graph, std::map<std::string, std::string> &options,
    const std::string &filePath);

/**
 * @brief       set attribute to graph
 * @param [in]  Graph &graph                 graph to build
 * @param [in]  ge::aclgrphAttrType attr     graph attribute
 * @param [in]  const string &filePath       attribute config file
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeGraphSetOpAttr(ge::Graph &graph, ge::aclgrphAttrType attr, const std::string &filePath);

/**
 * @brief       finalize build environment, only call once, concurrency is not supported
 * @return      NA
 */
extern "C" void AoeOfflineCompilerExit(void);
#endif

#ifdef TUNING_TOOL_ONLINE
/**
 * @brief       Init build environment, only call once, concurrency is not supported
 * @param [in]  Session *session        ge session
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOnlineCompilerInit(ge::Session *session);

/**
 * @brief       split graph to sub graph
 * @param [in]  Graph &graph                                    graph to build
 * @param [in]  std::map<std::string, std::string>  &options    build option
 * @param [in]  TPath filePath                                  sub graph file path
 * @return      success ==0, fail ==-1
 */
extern "C" AoeStatus AoeOnlineSplitGraph(ge::Graph &graph, std::map<std::string, std::string> &options,
    const std::string &filepath);

/**
 * @brief       finalize build environment, only call once, concurrency is not supported
 * @return      NA
 */
extern "C" void AoeOnlineCompilerExit(void);
#endif

#endif
