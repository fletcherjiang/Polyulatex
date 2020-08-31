# Copyright 2019-2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

add_definitions(-w)
project(ascendcl_utest)

set(CMAKE_CXX_STANDARD 11)

#include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${BASE_DIR})
include_directories(${BASE_DIR}/inc)
include_directories(${BASE_DIR}/inc/external)
include_directories(${BASE_DIR}/third_party/inc)
include_directories(${BASE_DIR}/third_party/inc/graphengine/inc)
include_directories(${BASE_DIR}/third_party/inc/graphengine/inc/framework)
include_directories(${BASE_DIR}/third_party/inc/graphengine/inc/external)
include_directories(${BASE_DIR}/third_party/inc/metadef/inc)
include_directories(${BASE_DIR}/third_party/inc/metadef/inc/external)
include_directories(${BASE_DIR}/common)
include_directories(${BASE_DIR}/single_op)
include_directories(${BASE_DIR}/toolchain)
include_directories(${BASE_DIR}/types)
include_directories(${BASE_DIR}/utils)
include_directories(${BASE_DIR}/third_party/inc/toolchain/ide/ide-daemon/external)
include_directories(${BASE_DIR}/third_party/inc/tdt)
include_directories(${BASE_DIR}/third_party/patch/securec)
include_directories(${CMAKE_BINARY_DIR})

set(UT_FILES
    "testcase/acl_common_unittest.cpp"
    "testcase/acl_toolchain_unittest.cpp"
)

set(SRC_FILES
    "${BASE_DIR}/common/acl.cpp"
    "${BASE_DIR}/common/log_inner.cpp"
    "${BASE_DIR}/common/log.cpp"
    "${BASE_DIR}/common/json_parser.cpp"
    "${BASE_DIR}/common/error_codes_api.cpp"
    "${BASE_DIR}/runtime/device.cpp"
    "${BASE_DIR}/runtime/context.cpp"
    "${BASE_DIR}/runtime/stream.cpp"
    "${BASE_DIR}/runtime/memory.cpp"
    "${BASE_DIR}/runtime/event.cpp"
    "${BASE_DIR}/runtime/callback.cpp"
    "${BASE_DIR}/runtime/group.cpp"
    "${BASE_DIR}/model/model.cpp"
    "${BASE_DIR}/stub/libjpeg_lhisi_stub.cpp"
    "${BASE_DIR}/model/model_config.cpp"
    "${BASE_DIR}/model/acl_aipp.cpp"
    "${BASE_DIR}/model/aipp_param_check.cpp"
    "${BASE_DIR}/single_op/op.cpp"
    "${BASE_DIR}/single_op/op_executor.cpp"
    "${BASE_DIR}/single_op/op_model_cache.cpp"
    "${BASE_DIR}/single_op/op_model_manager.cpp"
    "${BASE_DIR}/single_op/op_model_parser.cpp"
    "${BASE_DIR}/toolchain/acl_prof_api.cpp"
    "${BASE_DIR}/toolchain/dump.cpp"
    "${BASE_DIR}/toolchain/profiling.cpp"
    "${BASE_DIR}/toolchain/profiling_manager.cpp"
    "${BASE_DIR}/toolchain/resource_statistics.cpp"
    "${BASE_DIR}/types/acl_op.cpp"
    "${BASE_DIR}/types/fp16.cpp"
    "${BASE_DIR}/types/fp16_impl.cpp"
    "${BASE_DIR}/types/op_attr.cpp"
    "${BASE_DIR}/types/op_model.cpp"
    "${BASE_DIR}/types/tensor_desc_internal.cpp"
    "${BASE_DIR}/utils/array_utils.cpp"
    "${BASE_DIR}/utils/attr_utils.cpp"
    "${BASE_DIR}/utils/math_utils.cpp"
    "${BASE_DIR}/utils/file_utils.cpp"
    "${BASE_DIR}/utils/string_utils.cpp"
    "${BASE_DIR}/single_op/blas/gemm_ops.cpp"
    "${BASE_DIR}/single_op/blas/gemv_ops.cpp"
    "${BASE_DIR}/single_op/builtin/cast_op.cpp"
    "${BASE_DIR}/single_op/builtin/transdata_op.cpp"
    "${BASE_DIR}/single_op/compile/op_compiler.cpp"
    "${BASE_DIR}/single_op/op_compiler.cpp"
    "${BASE_DIR}/single_op/compile/local_compiler.cpp"
    "${BASE_DIR}/single_op/compile/op_compile_service.cpp"
    "${BASE_DIR}/single_op/compile/op_kernel_selector.cpp"
    "${BASE_DIR}/single_op/compile/op_kernel_registry.cpp"
    "${BASE_DIR}/single_op/compile/op_compile_processor.cpp"
    "${BASE_DIR}/single_op/executor/op_task.cpp"
    "${BASE_DIR}/single_op/executor/resource_manager.cpp"
    "${BASE_DIR}/single_op/executor/stream_executor.cpp"
    "${BASE_DIR}/single_op/dvpp/channel.cpp"
    "${BASE_DIR}/single_op/dvpp/jpeg.cpp"
    "${BASE_DIR}/single_op/dvpp/png.cpp"
    "${BASE_DIR}/single_op/dvpp/vpc.cpp"
    "${BASE_DIR}/single_op/dvpp/vdec.cpp"
    "${BASE_DIR}/single_op/dvpp/venc.cpp"
    "${BASE_DIR}/single_op/dvpp/base/image_processor.cpp"
    "${BASE_DIR}/single_op/dvpp/base/video_processor.cpp"
    "${BASE_DIR}/single_op/dvpp/mgr/dvpp_manager.cpp"
    "${BASE_DIR}/single_op/dvpp/v100/image_processor_v100.cpp"
    "${BASE_DIR}/single_op/dvpp/v200/image_processor_v200.cpp"
    "${BASE_DIR}/single_op/dvpp/v100/video_processor_v100.cpp"
    "${BASE_DIR}/single_op/dvpp/v200/video_processor_v200.cpp"
    "${BASE_DIR}/single_op/dvpp/common/dvpp_util.cpp"
    "${BASE_DIR}/types/dvpp.cpp"
    "${BASE_DIR}/single_op/retr/retr_accurate.cpp"
    "${BASE_DIR}/single_op/retr/retr_init.cpp"
    "${BASE_DIR}/single_op/retr/retr_internal.cpp"
    "${BASE_DIR}/single_op/retr/retr_ops.cpp"
    "${BASE_DIR}/single_op/retr/retr_release.cpp"
    "${BASE_DIR}/single_op/retr/retr_repo.cpp"
    "${BASE_DIR}/single_op/retr/retr_search.cpp"
    "${BASE_DIR}/types/retr.cpp"
    "${BASE_DIR}/tensor_data_transfer/tensor_data_transfer.cpp"
)

############ libut_ascendcl_src.a ############
add_library(ut_ascendcl_src STATIC
    ${SRC_FILES}
)

target_compile_definitions(ut_ascendcl_src PRIVATE
    google=ascend_private
    ENABLE_DVPP_INTERFACE
    RUN_TEST
)

target_compile_options(ut_ascendcl_src PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ut_ascendcl_src PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    json
    ascend_protobuf
    static_turbojpeg
)

############ ascendcl_utest ############
add_executable(ascendcl_utest ${UT_FILES})

target_compile_options(ascendcl_utest PRIVATE
    -g
)

target_compile_definitions(ascendcl_utest PRIVATE
    google=ascend_private
)

target_link_libraries(ascendcl_utest PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    ut_ascendcl_src
    json
    gtest
    gtest_main
    slog_stub
    c_sec
    mmpa_stub
    ge_stub
    tdt_stub
    toolchain_stub
    runtime_stub
    profiling_stub
    -lrt
    -ldl
    -lgcov
)
