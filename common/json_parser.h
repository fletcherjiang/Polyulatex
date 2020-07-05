/**
* @file json_parser.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_JSON_PARSER_H_
#define ACL_JSON_PARSER_H_

#include "nlohmann/json.hpp"
#include "acl/acl_base.h"

namespace acl {
    class JsonParser {
    public:
        static aclError ParseJsonFromFile(const char *fileName, nlohmann::json &js, std::string *strJsonCtx,
            const char *subStrKey);

    private:
        static bool IsValidFileName(const char *fileName);

        static bool ParseJson(const char *fileName, nlohmann::json &js, std::string *strConfig, size_t fileLength);
    };
}
#endif // ACL_JSON_PARSER_H_
