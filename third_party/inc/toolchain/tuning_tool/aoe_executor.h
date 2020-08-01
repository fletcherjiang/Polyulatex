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

struct ExecuteInitConfig {
    uint32_t jobType;                           // job type
    uint32_t taskType;                          // tune task type
    uint32_t executorType;                      // executor type
    uint32_t parallelNum = 1;                   // parallel num
    uintptr_t session = 0;                      // session
    std::string socVer;                         // soc version
    std::string profPath;                       // profiling save data file path
    std::string parserPath;                     // profiling parse path
    std::string mdlBankPath;                    // model bank path
    std::string opBankPath;                     // op bank path
    std::string opatPath;                       // op tune work path
    std::string ncaConfigFile;                  // nca config file
    std::string tuneType;                       // tune Type <train infer>
    std::map<std::string, std::string> option;  // init option
    std::string serverIp;                       // server ip
    std::string serverPort;                     // server port
};

enum ExecutorType {
    EXE_DEFAULT                     = 0,                                   // follow initialize executor type
    EXE_OFFLINE_COMPILE             = 1 << 0,                              // support offline compiler
    EXE_OFFLINE_RUN                 = 1 << 1,                              // support offline runner
    EXE_ONLINE_COMPILE              = 1 << 2,                              // support online compiler
    EXE_ONLINE_RUN                  = 1 << 3,                              // support online runner
    EXE_REMOTE_COMPILE              = 1 << 4,                              // use remote compiler
    EXE_REMOTE_RUN                  = 1 << 5,                              // use remote runner
    EXE_REMOTE_COMPILE_RUN          = 1 << 6,                              // remote support build and run together
};


const uint32_t EXE_OFFLINE_EXECUTOR            = EXE_OFFLINE_COMPILE | EXE_OFFLINE_RUN;
const uint32_t EXE_ONLINE_EXECUTOR             = EXE_ONLINE_COMPILE | EXE_ONLINE_RUN;
const uint32_t EXE_REMOTE_EXECUTOR             = EXE_REMOTE_COMPILE | EXE_REMOTE_RUN;
const uint32_t EXE_REMOTE_COMPILE_RUN_EXECUTOR = EXE_REMOTE_EXECUTOR | EXE_REMOTE_COMPILE_RUN;

// 同步接口
extern "C" std::map<std::string, std::string> AoeExecutorGetCompileOption();
extern "C" AoeStatus AoeExecutorInitialize(ExecuteInitConfig &config);
extern "C" AoeStatus AoeExecutorFinalize();
extern "C" AoeStatus AoeExecutorCompile(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    const std::string &savePath, uint32_t exeType = EXE_DEFAULT);
extern "C" AoeStatus AoeExecutorRun(ExecuteConfig &runConfig, ExecuteResult &costTime, uint32_t exeType = EXE_DEFAULT);
extern "C" AoeStatus AoeExecutorOnlineRun(const ge::Graph &graph, ExecuteConfig &runConfig,
    std::vector<ExecuteOnlineResult> &result, uint32_t exeType = EXE_DEFAULT);
extern "C" AoeStatus AoeExecutorCompileRun(const ge::Graph &graph, const std::map<std::string, std::string> &options,
    ExecuteConfig &runConfig, ExecuteResult &costTime, uint32_t exeType = EXE_DEFAULT);
extern "C" AoeStatus AoeExecutorCompileSpiltGraph(const ge::Graph &graph,
    const std::map<std::string, std::string> &option, uint32_t exeType = EXE_DEFAULT);

// 异步接口
extern "C" std::future<AoeStatus> AoeExecutorCompileAsync(const ge::Graph &graph,
    const std::map<std::string, std::string> &options, const std::string &savePath, uint32_t exeType = EXE_DEFAULT);
extern "C" std::future<AoeStatus> AoeExecutorRunAsync(ExecuteConfig &runConfig,
    ExecuteResult &costTime, uint32_t exeType = EXE_DEFAULT);
extern "C" std::future<AoeStatus> AoeExecutorCompileRunAsync(const ge::Graph &graph,
     const std::map<std::string, std::string> &options, ExecuteConfig &runConfig,
     ExecuteResult &costTime, uint32_t exeType = EXE_DEFAULT);
extern "C" std::future<AoeStatus> AoeExecutorOpatCompileRunAsync(ge::Graph &graph,
     const std::map<std::string, std::string> &options, ExecuteConfig &runConfig,
     ExecuteResult &costTime, uint32_t exeType = EXE_DEFAULT);
}
#endif