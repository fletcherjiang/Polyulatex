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

#ifndef ERROR_MANAGER_H_
#define ERROR_MANAGER_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <mutex>
#include <cstring>

namespace error_message {
#ifdef __GNUC__
int FormatErrorMessage(char *str_dst, size_t dst_max, const char *format, ...) __attribute__((format(printf, 3, 4)));
#define TRIM_PATH(x) strrchr(x, '/') ? strrchr(x, '/') + 1 : x
#else
int FormatErrorMessage(char *str_dst, size_t dst_max, const char *format, ...);
#define TRIM_PATH(x) strrchr(x, '\\') ? strrchr(x, '\\') + 1 : x
#endif
}

#define LIMIT_PER_MESSAGE 512

///
/// @brief Report error message
/// @param [in] key: vector parameter key
/// @param [in] value: vector parameter value
///
#define REPORT_INPUT_ERROR(error_code, key, value)                                          \
  ErrorManager::GetInstance().ATCReportErrMessage(error_code, key, value)

///
/// @brief Report error message
/// @param [in] key: vector parameter key
/// @param [in] value: vector parameter value
///
#define REPORT_ENV_ERROR(error_code, key, value)                                            \
  ErrorManager::GetInstance().ATCReportErrMessage(error_code, key, value)

#define REPORT_INNER_ERROR(error_code, fmt, ...)                                                 \
do {                                                                                             \
  char error_message_str[LIMIT_PER_MESSAGE] = {0};                                               \
  error_message::FormatErrorMessage(error_message_str, LIMIT_PER_MESSAGE, fmt, ##__VA_ARGS__);   \
  error_message::FormatErrorMessage(                                                             \
          error_message_str, LIMIT_PER_MESSAGE, "%s[FUNC:%s][FILE:%s][LINE:%d]",                 \
          error_message_str, __FUNCTION__, TRIM_PATH(__FILE__), __LINE__);                       \
  ErrorManager::GetInstance().ReportInterErrMessage(error_code, std::string(error_message_str)); \
} while (0)

#define REPORT_CALL_ERROR(error_code, fmt, ...)                                                  \
do {                                                                                             \
  char error_message_str[LIMIT_PER_MESSAGE] = {0};                                               \
  error_message::FormatErrorMessage(error_message_str, LIMIT_PER_MESSAGE, fmt, ##__VA_ARGS__);   \
  error_message::FormatErrorMessage(                                                             \
          error_message_str, LIMIT_PER_MESSAGE, "%s[FUNC:%s][FILE:%s][LINE:%d]",                 \
          error_message_str, __FUNCTION__, TRIM_PATH(__FILE__), __LINE__);                       \
  ErrorManager::GetInstance().ReportInterErrMessage(error_code, std::string(error_message_str)); \
} while (0)

namespace error_message {
  // first stage
  constexpr char const *kInitialize   = "INIT";
  constexpr char const *kModelCompile = "COMP";
  constexpr char const *kModelLoad    = "LOAD";
  constexpr char const *kModelExecute = "EXEC";
  constexpr char const *kFinalize     = "FINAL";

  // SecondStage
  // INITIALIZE
  constexpr char const *kParser               = "PARSER";
  constexpr char const *kOpsProtoInit         = "OPS_PRO";
  constexpr char const *kSystemInit           = "SYS";
  constexpr char const *kEngineInit           = "ENGINE";
  constexpr char const *kOpsKernelInit        = "OPS_KER";
  constexpr char const *kOpsKernelBuilderInit = "OPS_KER_BLD";
  // MODEL_COMPILE
  constexpr char const *kPrepareOptimize    = "PRE_OPT";
  constexpr char const *kOriginOptimize     = "ORI_OPT";
  constexpr char const *kSubGraphOptimize   = "SUB_OPT";
  constexpr char const *kMergeGraphOptimize = "MERGE_OPT";
  constexpr char const *kPreBuild           = "PRE_BLD";
  constexpr char const *kStreamAlloc        = "STM_ALLOC";
  constexpr char const *kMemoryAlloc        = "MEM_ALLOC";
  constexpr char const *kTaskGenerate       = "TASK_GEN";
  // COMMON
  constexpr char const *kOther = "DEFAULT";

  struct Context {
    uint64_t work_stream_id;
    std::string first_stage;
    std::string second_stage;
    std::string log_header;
  };
}

// old, will be delete after all caller transfer to new
namespace ErrorMessage {
  // first stage
  constexpr char const *kInitialize   = "INIT";
  constexpr char const *kModelCompile = "COMP";
  constexpr char const *kModelLoad    = "LOAD";
  constexpr char const *kModelExecute = "EXEC";
  constexpr char const *kFinalize     = "FINAL";

  // SecondStage
  // INITIALIZE
  constexpr char const *kParser               = "PARSER";
  constexpr char const *kOpsProtoInit         = "OPS_PRO";
  constexpr char const *kSystemInit           = "SYS";
  constexpr char const *kEngineInit           = "ENGINE";
  constexpr char const *kOpsKernelInit        = "OPS_KER";
  constexpr char const *kOpsKernelBuilderInit = "OPS_KER_BLD";
  // MODEL_COMPILE
  constexpr char const *kPrepareOptimize    = "PRE_OPT";
  constexpr char const *kOriginOptimize     = "ORI_OPT";
  constexpr char const *kSubGraphOptimize   = "SUB_OPT";
  constexpr char const *kMergeGraphOptimize = "MERGE_OPT";
  constexpr char const *kPreBuild           = "PRE_BLD";
  constexpr char const *kStreamAlloc        = "STM_ALLOC";
  constexpr char const *kMemoryAlloc        = "MEM_ALLOC";
  constexpr char const *kTaskGenerate       = "TASK_GEN";
  // COMMON
  constexpr char const *kOther = "DEFAULT";

  struct Context {
    uint64_t work_stream_id;
    std::string first_stage;
    std::string second_stage;
    std::string log_header;
  };
}

class ErrorManager {
 public:
  ///
  /// @brief Obtain  ErrorManager instance
  /// @return ErrorManager instance
  ///
  static ErrorManager &GetInstance();

  ///
  /// @brief init
  /// @return int 0(success) -1(fail)
  ///
  int Init();

  ///
  /// @brief init
  /// @param [in] path: current so path
  /// @return int 0(success) -1(fail)
  ///
  int Init(std::string path);

  int ReportInterErrMessage(std::string error_code, const std::string &error_msg);

  ///
  /// @brief Report error message
  /// @param [in] error_code: error code
  /// @param [in] args_map: parameter map
  /// @return int 0(success) -1(fail)
  ///
  int ReportErrMessage(std::string error_code, const std::map<std::string, std::string> &args_map);

  ///
  /// @brief output error message
  /// @param [in] handle: print handle
  /// @return int 0(success) -1(fail)
  ///
  int OutputErrMessage(int handle);

  ///
  /// @brief output  message
  /// @param [in] handle: print handle
  /// @return int 0(success) -1(fail)
  ///
  int OutputMessage(int handle);

  std::string GetErrorMessage();

  std::string GetWarningMessage();

  ///
  /// @brief Report error message
  /// @param [in] key: vector parameter key
  /// @param [in] value: vector parameter value
  ///
  void ATCReportErrMessage(std::string error_code, const std::vector<std::string> &key = {},
                           const std::vector<std::string> &value = {});

  ///
  /// @brief report graph compile failed message such as error code and op_name in mstune case
  /// @param [in] graph_name: root graph name
  /// @param [in] msg: failed message map, key is error code, value is op_name
  /// @return int 0(success) -1(fail)
  ///
  int ReportMstuneCompileFailedMsg(const std::string &root_graph_name,
                                   const std::map<std::string, std::string> &msg);

  ///
  /// @brief get graph compile failed message in mstune case
  /// @param [in] graph_name: graph name
  /// @param [out] msg_map: failed message map, key is error code, value is op_name list
  /// @return int 0(success) -1(fail)
  ///
  int GetMstuneCompileFailedMsg(const std::string &graph_name,
                                std::map<std::string,
                                std::vector<std::string>> &msg_map);

  // @brief generate work_stream_id by current pid and tid, clear error_message stored by same work_stream_id
  // used in external api entrance, all sync api can use
  void GenWorkStreamIdDefault();

  // @brief generate work_stream_id by args sessionid and graphid, clear error_message stored by same work_stream_id
  // used in external api entrance
  void GenWorkStreamIdBySessionGraph(uint64_t session_id, uint64_t graph_id);

  const std::string &GetLogHeader();

  ErrorMessage::Context &GetErrorContext();
  error_message::Context &GetErrorManagerContext();

  void SetErrorContext(ErrorMessage::Context error_context);
  void SetErrorContext(error_message::Context error_context);

  void SetStage(const std::string &first_stage, const std::string &second_stage);

 private:
  struct ErrorInfoConfig {
    std::string error_id;
    std::string error_message;
    std::vector<std::string> arg_list;
  };

  struct ErrorItem {
    std::string error_id;
    std::string error_message;

    bool operator==(const ErrorItem &rhs) const {
      return (error_id == rhs.error_id) && (error_message == rhs.error_message);
    }
  };

  ErrorManager() {}
  ~ErrorManager() {}

  ErrorManager(const ErrorManager &) = delete;
  ErrorManager(ErrorManager &&) = delete;
  ErrorManager &operator=(const ErrorManager &) = delete;
  ErrorManager &operator=(ErrorManager &&) = delete;

  int ParseJsonFile(std::string path);

  int ReadJsonFile(const std::string &file_path, void *handle);

  void ClassifyCompileFailedMsg(const std::map<std::string, std::string> &msg,
                                std::map<std::string,
                                std::vector<std::string>> &classfied_msg);

  bool IsInnerErrorCode(const std::string &error_code);

  inline bool IsValidErrorCode(const std::string &error_code) {
    const uint32_t kErrorCodeValidLength = 6;
    return error_code.size() == kErrorCodeValidLength;
  }

  std::vector<ErrorItem> &GetErrorMsgContainerByWorkId(uint64_t work_id);
  std::vector<ErrorItem> &GetWarningMsgContainerByWorkId(uint64_t work_id);

  void ClearErrorMsgContainerByWorkId(uint64_t work_stream_id);
  void ClearWarningMsgContainerByWorkId(uint64_t work_stream_id);

  bool is_init_ = false;
  std::mutex mutex_;
  std::map<std::string, ErrorInfoConfig> error_map_;
  std::map<std::string, std::map<std::string, std::vector<std::string>>> compile_failed_msg_map_;

  std::map<uint64_t, std::vector<ErrorItem>> error_message_per_work_id_;
  std::map<uint64_t, std::vector<ErrorItem>> warning_messages_per_work_id_;

  thread_local static error_message::Context error_context_;
};

#endif  // ERROR_MANAGER_H_
