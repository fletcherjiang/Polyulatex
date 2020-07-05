/**
* @file string_utils.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <vector>
#include <string>
#include <sstream>

namespace acl {
namespace string_utils {
template<typename T>
std::string VectorToString(const std::vector<T> &values)
{
    std::stringstream ss;
    ss << '[';
    auto size = values.size();
    for (size_t i = 0; i < size; ++i) {
        ss << values[i];
        if (i != size - 1) {
            ss << ", ";
        }
    }
    ss << ']';
    return ss.str();
}

template<typename T>
std::string VectorToString(const std::vector<std::vector<T>> &values)
{
    if (values.empty()) {
        return "[]";
    }

    auto size = values.size();
    std::stringstream ss;
    ss << '[';
    for (size_t i = 0; i < size; ++i) {
        ss << '[';
        const auto &subVec = values[i];
        auto subSize = subVec.size();
        for (size_t j = 0; j < subSize; ++j) {
            ss << subVec[j];
            if (j != subSize - 1) {
                ss << ", ";
            }
        }
        ss << "]";
        if (i != size - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

template<typename T>
std::string VectorToString(const std::vector<std::pair<T, T>> &values)
{
    if (values.empty()) {
        return "[]";
    }

    auto size = values.size();
    std::stringstream ss;
    ss << '[';
    for (size_t i = 0; i < size; ++i) {
        ss << '[';
        ss << values[i].first;
        ss << ", ";
        ss << values[i].second;
        ss << "]";
        if (i != size - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

void Split(const std::string &str, char delim, std::vector<std::string> &res);

bool IsDigit(const std::string &str);
} // namespace string_utils
} // namespace acl

#endif // STRING_UTILS_H
