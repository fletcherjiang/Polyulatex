/**
* @file json_parser.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "json_parser.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "nlohmann/json.hpp"
#include "mmpa/mmpa_api.h"
#include "log_inner.h"

namespace acl {
    // 配置文件最大字节数目10MBytes
    constexpr uint32_t MAX_CONFIG_FILE_BYTE = 10 * 1024 * 1024;
    // 配置文件最大递归深度
    constexpr size_t MAX_CONFIG_OBJ_DEPTH = 10;
    // 配置文件最大数组个数
    constexpr size_t MAX_CONFIG_ARRAY_DEPTH = 10;

    void GetMaxNestedLayers(const char *fileName, size_t length, size_t &maxObjDepth, size_t &maxArrayDepth)
    {
        if (length <= 0) {
            ACL_LOG_INNER_ERROR("[Check][Length]the length of file %s must be larger than 0.", fileName);
            return;
        }

        char *pBuffer = new(std::nothrow) char[length];
        ACL_REQUIRES_NOT_NULL_RET_VOID(pBuffer);
        std::shared_ptr<char> buffer(pBuffer, [](char *p) { delete[] p; });

        std::ifstream fin(fileName);
        if (!fin.is_open()) {
            ACL_LOG_INNER_ERROR("[Open][File]read file %s failed.", fileName);
            return;
        }
        fin.seekg(0, fin.beg);
        fin.read(buffer.get(), length);

        size_t arrayDepth = 0;
        size_t objDepth = 0;
        for (size_t i = 0; i < length; ++i) {
            char v = buffer.get()[i];
            switch (v) {
                case '{': {
                    ++objDepth;
                    if (objDepth > maxObjDepth) {
                        maxObjDepth = objDepth;
                    }
                    break;
                }
                case '}': {
                    if (objDepth > 0) {
                        --objDepth;
                    }
                    break;
                }
                case '[': {
                    ++arrayDepth;
                    if (arrayDepth > maxArrayDepth) {
                        maxArrayDepth = arrayDepth;
                    }
                    break;
                }
                case ']': {
                    if (arrayDepth > 0) {
                        --arrayDepth;
                    }
                    break;
                }
                case '\0': {
                    fin.close();
                    return;
                }
                default : {
                    continue;
                }
            }
        }

        fin.close();
        return;
    }

    bool JsonParser::IsValidFileName(const char *fileName)
    {
        char trustedPath[MMPA_MAX_PATH] = {'\0'};
        int32_t ret = mmRealPath(fileName, trustedPath, sizeof(trustedPath));
        if (ret != EN_OK) {
            ACL_LOG_INNER_ERROR("[Trans][RealPath]the file path %s is not like a real path, mmRealPath return %d, "
                "errcode is %d", fileName, ret, mmGetErrorCode());
            return false;
        }

        mmStat_t stat = {0};
        ret = mmStatGet(trustedPath, &stat);
        if (ret != EN_OK) {
            ACL_LOG_INNER_ERROR("[Get][FileStatus]cannot get config file status, which path is %s, "
                "maybe not exist, return %d, errcode %d", trustedPath, ret, mmGetErrorCode());
            return false;
        }
        if ((stat.st_mode & S_IFMT) != S_IFREG) {
            ACL_LOG_INNER_ERROR("[Config][ConfigFile]config file is not a common file, which path is %s, "
                "mode is %u", trustedPath, stat.st_mode);
            return false;
        }
        if (stat.st_size > MAX_CONFIG_FILE_BYTE) {
            ACL_LOG_INNER_ERROR("[Check][FileSize]config file %s size[%ld] is larger than "
                "max config file Bytes[%u]", trustedPath, stat.st_size, MAX_CONFIG_FILE_BYTE);
            return false;
        }
        return true;
    }

    bool JsonParser::ParseJson(const char *fileName, nlohmann::json &js, std::string *strConfig, size_t fileLength)
    {
        std::ifstream fin(fileName);
        if (!fin.is_open()) {
            ACL_LOG_INNER_ERROR("[Read][File]read file %s failed.", fileName);
            return false;
        }

        // checking the depth of file
        size_t maxObjDepth = 0;
        size_t maxArrayDepth = 0;
        GetMaxNestedLayers(fileName, fileLength, maxObjDepth, maxArrayDepth);
        if ((maxObjDepth > MAX_CONFIG_OBJ_DEPTH) || (maxArrayDepth > MAX_CONFIG_ARRAY_DEPTH)) {
            ACL_LOG_INNER_ERROR("[Check][MaxArrayDepth]invalid json file, the object's depth[%zu] is larger than %zu, or "
                          "the array's depth[%zu] is larger than %zu.",
                          maxObjDepth, MAX_CONFIG_OBJ_DEPTH, maxArrayDepth, MAX_CONFIG_ARRAY_DEPTH);
            fin.close();
            return false;
        }
        ACL_LOG_INFO("json file's obj's depth is %zu, array's depth is %zu", maxObjDepth, maxArrayDepth);

        try {
            fin >> js;
        } catch (const nlohmann::json::exception &e) {
            ACL_LOG_INNER_ERROR("[Check][JsonFile]invalid json file, exception:%s.", e.what());
            fin.close();
            return false;
        }

        if (strConfig == nullptr) {
            fin.close();
            return true;
        }

        try {
            std::ostringstream oss;
            fin.seekg(0);
            oss << fin.rdbuf();
            *strConfig = oss.str();
        } catch (...) {
            ACL_LOG_INNER_ERROR("[Get][JsonStr]get json string failed, errno =%d!", errno);
            fin.close();
            return false;
        }

        fin.close();
        return true;
    }

    aclError JsonParser::ParseJsonFromFile(const char *fileName, nlohmann::json &js, std::string *strJsonCtx,
        const char *subStrKey)
    {
        if (fileName == nullptr) {
            ACL_LOG_WARN("filename is nullptr, no need to parse json");
            return ACL_SUCCESS;
        }
        ACL_LOG_INFO("before ParseJsonFromFile in ParseJsonFromFile");
        if (!IsValidFileName(fileName)) {
            ACL_LOG_INNER_ERROR("[Check][File]invalid config file[%s]", fileName);
            return ACL_ERROR_INVALID_FILE;
        }
        std::ifstream fin(fileName);
        if (!fin.is_open()) {
            ACL_LOG_INNER_ERROR("[Read][File]read file %s failed.", fileName);
            return ACL_ERROR_INVALID_FILE;
        }
        fin.seekg(0, std::ios::end);
        std::streampos fp = fin.tellg();
        if (static_cast<int>(fp) == 0) {
            ACL_LOG_INFO("parsefile is null");
            fin.close();
            return ACL_SUCCESS;
        }
        fin.close();
        if (!ParseJson(fileName, js, strJsonCtx, static_cast<size_t>(fp))) {
            ACL_LOG_INNER_ERROR("[Parse][File]parse config file[%s] to json failed.", fileName);
            return ACL_ERROR_PARSE_FILE;
        }
        if (subStrKey != nullptr) {
            std::string subInputStr(subStrKey);
            if (js.find(subInputStr) == js.end()) {
                ACL_LOG_WARN("find key[%s] in json file %s failed", subStrKey, fileName);
                return ACL_ERROR_INVALID_PARAM;
            }
            js = js.at(subInputStr);
            if (strJsonCtx != nullptr) {
                *strJsonCtx = js.dump();
            }
        }

        ACL_LOG_INFO("parse json from file[%s] successfully.", fileName);
        return ACL_SUCCESS;
    }
}

