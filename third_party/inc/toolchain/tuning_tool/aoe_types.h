/**
 * @file aoe_types.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.\n
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n
 *
 */

#ifndef AOE_TYPES_H
#define AOE_TYPES_H

#include <vector>
#include <memory>
#include "graph/graph.h"

using AoeStatus = int32_t;
const AoeStatus AOE_SUCCESS = 0;
const AoeStatus AOE_FAILURE = -1;

struct AoeDataInfo {
    uint8_t *ptr = nullptr;
    size_t size = 0;
};

/**
 * @ingroup aoe
 *
 * aoe status
 */
enum MsTuneStatus {
    MSTUNE_SUCCESS,  /** tune success */
    MSTUNE_FAILED,   /** tune failed */
};

// Option key: for train options sets
const std::string MSTUNE_SELF_KEY = "mstune";
const std::string MSTUNE_GEINIT_KEY = "initialize";
const std::string MSTUNE_GESESS_KEY = "session";

struct AOEBufferData {
    std::shared_ptr<uint8_t> data = nullptr;
    uint64_t length;
};

struct RunnerInitConfig {
    // onilne online
    std::string profPath;
    std::string parserPath;
    // ncs only
    std::vector<uint32_t> devList;
};

struct RunnerConfig {
    bool isProf;
    uint32_t loop;
    // offline only
    std::vector<AoeDataInfo> input;
    std::vector<AoeDataInfo> output;
    std::string modelPath; // run with model file
    AoeDataInfo modelData; // run with model data
    uint32_t modelId; // run with model Id
    // online only
    uint32_t devId;
    std::vector<std::vector<ge::Tensor>> inputs;
    std::vector<ge::Graph> dependGraph; // run graph (for training)
};

struct RunnerOpInfo {
    std::string opName;
    uint64_t opCostTime;
    uint64_t aicoreCostTime;
    // gradient_split only
    std::string modelName;
    std::string opType;
    std::vector<uint64_t> start;
    std::vector<uint64_t> end;
};

struct RunnerModelInfo {
    uint64_t totalCostTime;
};

// online run result
struct RunnerRunResult {
    std::vector<RunnerModelInfo> modelInfo;
    std::vector<RunnerOpInfo> opInfo;
};

// offline run result
struct RunnerResult {
    uint64_t totalCostTime;
    std::map<std::string, uint64_t> opCostTime;
    std::map<std::string, uint64_t> aicoreCostTime;
};

#endif

