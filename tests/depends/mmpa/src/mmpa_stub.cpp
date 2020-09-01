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

#include "mmpa/mmpa_api.h"

#include <string.h>

INT32 mmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc,  mmSort sort)
{
    return 0;
}

VOID mmScandirFree(mmDirent **entryList, INT32 count)
{
}

INT32 mmScandir2(const CHAR *path, mmDirent2 ***entryList, mmFilter2 filterFunc,  mmSort2 sort)
{
    return 0;
}

VOID mmScandirFree2(mmDirent2 **entryList, INT32 count)
{
}

INT32 mmAccess2(const CHAR *pathName, INT32 mode)
{
    return 0;
}

INT32 mmGetEnv(const CHAR *name, CHAR *value, UINT32 len)
{
    char environment[MMPA_MAX_PATH] = "llt/acl/ut/json/profilingConfig.json";
    (void)memcpy_s(value, MMPA_MAX_PATH, environment, MMPA_MAX_PATH);
    return 0;
}

INT32 mmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
{
    INT32 ret = EN_OK;
    if (path == nullptr || realPath == nullptr || realPathLen < MMPA_MAX_PATH) {
        return EN_INVALID_PARAM;
    }

    char *ptr = realpath(path, realPath);
    if (ptr == nullptr) {
        ret = EN_ERROR;
    }
    return ret;
}

INT32 mmStatGet(const CHAR *path,  mmStat_t *buffer)
{
    if ((path == nullptr) || (buffer == nullptr)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = stat(path, buffer);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

INT32 mmGetErrorCode()
{
    return 0;
}

mmTimespec mmGetTickCount()
{
    mmTimespec time;
    time.tv_nsec = 1;
    time.tv_sec = 1;
    return time;
}

INT32 mmGetTid()
{
    INT32 ret = (INT32)syscall(SYS_gettid);

    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    return ret;
}

INT32 mmIsDir(const CHAR *fileName)
{
    return 0;
}
INT32 mmGetPid()
{
    return (INT32)getpid();
}

INT32 mmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
{
    timeVal->tv_sec = 2;
    return 0;
}
mmSize mmGetPageSize()
{
    return 2;
}

VOID *mmAlignMalloc(mmSize mallocSize, mmSize alignSize)
{
    return malloc(mallocSize);
}

VOID mmAlignFree(VOID *addr)
{
    if (addr != nullptr) {
        free(addr);
    }
}

