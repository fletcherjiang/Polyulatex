/**
 * @file aoe_runner.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef AOE_RUNNER_H
#define AOE_RUNNER_H

#include <cstddef>
#include <cstdint>
#include "aoe_types.h"
#include "tune_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TUNING_TOOL_OFFLINE
AoeStatus AoeOfflineRunnerInit(const RunnerInitConfig &initConfig);
void AoeOfflineRunnerFinalize();
AoeStatus AoeOfflineRun(RunnerConfig &config, RunnerResult &result);
AoeStatus AoeOfflineLoadFromFile(const std::string &modelPath, uint32_t &modelId);
AoeStatus AoeOfflineLoadFromMem(const AoeDataInfo &model, uint32_t &modelId);
AoeStatus AoeOfflineRunModel(const uint32_t &modelId, RunnerConfig &config, RunnerResult &result);
void AoeOfflineUnloadModel(const uint32_t &modelId);
#endif

#ifdef TUNING_TOOL_ONLINE
AoeStatus AoeOnlineRunnerInit(uintptr_t runSession, const RunnerInitConfig &initConfig);
void AoeOnlineRunnerFinalize();
AoeStatus AoeOnlineRun(const ge::Graph &graph, RunnerConfig &runConfig, std::vector<RunnerRunResult> &result);
#endif

#ifdef __cplusplus
}
#endif
#endif