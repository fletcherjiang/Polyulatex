/**
* @file string_utils.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "string_utils.h"

namespace acl {
namespace string_utils {
void Split(const std::string &str, char delim, std::vector<std::string> &elems)
{
    elems.clear();
    if (str.empty()) {
        elems.emplace_back("");
        return;
    }

    std::stringstream ss(str);
    std::string item;

    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }

    auto strSize = str.size();
    if (strSize > 0 && str[strSize - 1] == delim) {
        elems.emplace_back("");
    }
}

bool IsDigit(const std::string &str)
{
    for (const char &c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}
} // namespace string_utils
} // namespace acl