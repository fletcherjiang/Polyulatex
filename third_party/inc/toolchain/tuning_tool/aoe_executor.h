/**
 * @file aoe_executor.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef AOE_EXECUTOR_H
#define AOE_EXECUTOR_H
#include <map>
#include <string>
#include <future>
#include "graph/graph.h"
#include "aoe_runner.h"
namespace TuningTool {
using ExecuteBufferData = struct AoeDataInfo;
using ExecuteConfig = struct RunnerConfig;
using ExecuteResult = struct RunnerResult;
using ExecuteOnlineResult = struct RunnerRunResult;

// 同步接口
std::map<std::string, std::string> AoeGetCompileCommonOption();
int32_t AoeCompile(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    const std::string &savePath);
int32_t AoeRun(ExecuteConfig &runConfig, ExecuteResult &costTime);
int32_t AoeCompileRun(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    ExecuteConfig &runConfig, ExecuteResult &costTime);

// 异步接口
std::future<int32_t> AoeCompileAsync(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    const std::string &savePath);
std::future<int32_t> AoeRunAsync(ExecuteConfig &runConfig, ExecuteResult &costTime);
std::future<int32_t> AoeCompileRunAsync(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    ExecuteConfig &runConfig, ExecuteResult &costTime);
std::future<int32_t> AoeOpatCompileRunAsync(ge::Graph &graph, const std::map<std::string, std::string> &options,
    ExecuteConfig &runConfig, ExecuteResult &costTime);
}
#endif