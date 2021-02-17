/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "toolchain/prof_engine.h"
#include "toolchain/prof_acl_api.h"
#include "toolchain/prof_callback.h"
#include "acl_stub.h"


namespace Msprof {
namespace Engine {
int Init(const std::string &module, const EngineIntf *engine)
{
    return 0;
}

int UnInit(const std::string &module)
{
    return 0;
}

int Reporter::Report(const ReporterData *data)
{
    return 0;
}

int Reporter::Flush()
{
    return 0;
}

}
}


int32_t ProfAclCfgToSampleCfg(const std::string &aclCfg, std::string &sampleCfg)
{
    return 0;
}


 int32_t aclStub::MsprofFinalize()
 {
     return 0;
 }

int32_t aclStub::MsprofInit(uint32_t aclDataType, void *data, uint32_t dataLen)
{
    return 0;
}

int32_t MsprofFinalize()
{
    return MockFunctionTest::aclStubInstance().MsprofFinalize(); 
}

int32_t MsprofInit(uint32_t aclDataType, void *data, uint32_t dataLen)
{
    return MockFunctionTest::aclStubInstance().MsprofInit(aclDataType, data, dataLen);
}