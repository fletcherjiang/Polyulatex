cmake_minimum_required(VERSION 3.14)
project (ascend)

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(ENABLE_OPEN_SRC "Enable acl compile in opensource." FALSE)

if(ENABLE_OPEN_SRC)
if(DEFINED ENV{ASCEND_CUSTOM_PATH})
    set(ASCEND_DIR $ENV{ASCEND_CUSTOM_PATH})
else()
    set(ASCEND_DIR /usr/local/Ascend)
endif()

if(DEFINED ENV{D_PKG_SERVER})
    set(ACL_PB_PKG $ENV{D_PKG_SERVER})
    message("Download packages from DPKG server")
endif()

set(TARGET_SYSTEM_NAME "Linux")
set(HI_PYTHON python3)

include(${BASE_DIR}/cmake/intf_pub_linux.cmake)
include(${BASE_DIR}/cmake/FindModule.cmake)
include(${BASE_DIR}/cmake/external_libs/protobuf_shared.cmake)
include(${BASE_DIR}/cmake/external_libs/protoc.cmake)
include(${BASE_DIR}/cmake/external_libs/json.cmake)
include(${BASE_DIR}/cmake/external_libs/securec.cmake)
include(${BASE_DIR}/cmake/external_libs/jpeg.cmake)

if(DEFINED ENV{D_LINK_PATH})
    # D_LINK_PATH is set
    set(ACL_LIB_PATH $ENV{D_LINK_PATH})
    message("Link path is ${ACL_LIB_PATH}")
    set(ACL_SYS_ARCH "")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        # x86 ubuntu
        set(ACL_SYS_ARCH "x86_64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
        # arm euleros
        set(ACL_SYS_ARCH "aarch64")
    else()
        message(FATAL_ERROR "Running on a unsupported architecture: ${SYSTEM_TYPE}, build terminated")
    endif()
    set(ACL_LIB_PATH ${ACL_LIB_PATH}/${ACL_SYS_ARCH})
    message("Acl lib path is ${ACL_LIB_PATH}")
    find_module(static_mmpa libmmpa.a ${ACL_LIB_PATH})
    find_module(ascend_hal_stub libascend_hal.so ${ACL_LIB_PATH})
    find_module(ge_executor_shared libge_executor.so ${ACL_LIB_PATH})
    find_module(slog libalog.so ${ACL_LIB_PATH})
    find_module(alog libalog.so ${ACL_LIB_PATH})
    find_module(runtime libruntime.so ${ACL_LIB_PATH})
    find_module(adump_server libadump_server.a ${ACL_LIB_PATH})
    find_module(ge_common libge_common.so ${ACL_LIB_PATH})
    find_module(graph libgraph.so ${ACL_LIB_PATH})
    find_module(ge_runner libge_runner.so ${ACL_LIB_PATH})
    find_module(register libregister.so ${ACL_LIB_PATH})
    find_module(datatransfer libdatatransfer.so ${ACL_LIB_PATH})
    find_module(error_manager liberror_manager.so ${ACL_LIB_PATH})
    find_module(msprofiler_ext libmsprofiler.a ${ACL_LIB_PATH})
    find_module(msprofiler_fwkacl_ext libmsprofiler_fwkacl.a ${ACL_LIB_PATH})
else()
    set(ASCEND_ACL_DIR ${ASCEND_DIR}/acllib/lib64)
    set(ASCEND_RUNTIME_DIR ${ASCEND_DIR}/fwkacllib/lib64)
    set(ASCEND_ATC_DIR ${ASCEND_DIR}/atc/lib64)
    set(ASCEND_DRIVER_DIR ${ASCEND_DIR}/driver/lib64)

    find_module(slog libalog.so ${ASCEND_ATC_DIR})
    find_module(alog libalog.so ${ASCEND_ATC_DIR})
    find_module(static_mmpa libmmpa.a ${ASCEND_ATC_DIR})
    find_module(ge_executor_shared libge_executor.so ${ASCEND_ACL_DIR})
    find_module(ge_common libge_common.so ${ASCEND_ACL_DIR})
    find_module(graph libgraph.so ${ASCEND_ACL_DIR})
    find_module(error_manager liberror_manager.so ${ASCEND_ACL_DIR})
    find_module(msprofiler_ext libmsprofiler.a ${ASCEND_ACL_DIR})

    if(PLATFORM STREQUAL "train")
        find_module(adump_server libadump_server.a ${ASCEND_RUNTIME_DIR})
        find_module(runtime libruntime.so ${ASCEND_RUNTIME_DIR})
        find_module(ascend_hal_stub libascend_hal.so ${ASCEND_DRIVER_DIR}/driver)
        find_module(ge_runner libge_runner.so ${ASCEND_RUNTIME_DIR})
        find_module(register libregister.so ${ASCEND_RUNTIME_DIR})
        find_module(datatransfer libdatatransfer.so ${ASCEND_RUNTIME_DIR})
        find_module(msprofiler_fwkacl_ext libmsprofiler_fwkacl.a ${ASCEND_RUNTIME_DIR})
        if(PRODUCT STREQUAL "flr3")
            message(FATAL_ERROR "This platform is not supported in train mode, build terminated")
        endif()
    elseif(PLATFORM STREQUAL "inference")
        find_module(adump_server libadump_server.a ${ASCEND_ACL_DIR})
        find_module(runtime libruntime.so ${ASCEND_ACL_DIR})
        find_module(register libregister.so ${ASCEND_ACL_DIR})
        if(NOT ((PRODUCT STREQUAL "flr2") OR (PRODUCT STREQUAL "flr3")))
            find_module(ge_runner libge_runner.so ${ASCEND_RUNTIME_DIR})
            find_module(datatransfer libdatatransfer.so ${ASCEND_RUNTIME_DIR})
            find_module(ascend_hal_stub libascend_hal.so ${ASCEND_DRIVER_DIR})
            find_module(msprofiler_fwkacl_ext libmsprofiler_fwkacl.a ${ASCEND_RUNTIME_DIR})
        endif()
    elseif(PLATFORM STREQUAL "all")
        find_module(ge_runner libge_runner.so ${ASCEND_RUNTIME_DIR})
        find_module(register libregister.so ${ASCEND_RUNTIME_DIR})
        find_module(datatransfer libdatatransfer.so ${ASCEND_RUNTIME_DIR})
        find_module(adump_server libadump_server.a ${ASCEND_RUNTIME_DIR})
        find_module(msprofiler_fwkacl_ext libmsprofiler_fwkacl.a ${ASCEND_RUNTIME_DIR})
        find_module(runtime libruntime.so ${ASCEND_RUNTIME_DIR})
        find_module(ascend_hal_stub libascend_hal.so ${ASCEND_DRIVER_DIR})
    else()
        message(STATUS "PLATFORM param is invalid, should be train or inference, you choose nothing!")
    endif()
endif()
endif()

###################################################################################################
if(NOT ENABLE_OPEN_SRC)
    set(PROTO_LIST
        "${TOP_DIR}/metadef/proto/om.proto"
    )
else()
    set(PROTO_LIST
        "${BASE_DIR}/third_party/proto/metadef/proto/om.proto"
    )
endif()

protobuf_generate(acl PROTO_SRCS PROTO_HDRS ${PROTO_LIST})

set(SRC_LIST
    runtime/device.cpp
    runtime/event.cpp
    runtime/stream.cpp
    runtime/memory.cpp
    runtime/context.cpp
    runtime/callback.cpp
    runtime/group.cpp
    model/model.cpp
    model/acl_aipp.cpp
    model/aipp_param_check.cpp
    model/model_config.cpp
    common/acl.cpp
    common/log_inner.cpp
    common/log.cpp
    common/json_parser.cpp
    common/error_codes_api.cpp
    single_op/compile/op_compiler.cpp
    single_op/compile/op_compile_service.cpp
    single_op/builtin/cast_op.cpp
    single_op/builtin/transdata_op.cpp
    single_op/op.cpp
    single_op/op_executor.cpp
    single_op/op_model_cache.cpp
    single_op/op_model_manager.cpp
    single_op/op_model_parser.cpp
    types/acl_op.cpp
    types/fp16.cpp
    types/fp16_impl.cpp
    types/op_attr.cpp
    types/op_model.cpp
    types/tensor_desc_internal.cpp
    utils/array_utils.cpp
    utils/attr_utils.cpp
    utils/file_utils.cpp
    utils/string_utils.cpp
    utils/math_utils.cpp
    toolchain/dump.cpp
    toolchain/profiling.cpp
    toolchain/profiling_manager.cpp
    toolchain/resource_statistics.cpp
    toolchain/acl_prof_api.cpp
    single_op/compile/op_kernel_selector.cpp
    single_op/compile/op_kernel_registry.cpp
    single_op/executor/op_task.cpp
    single_op/executor/resource_manager.cpp
    single_op/executor/stream_executor.cpp
)

############ libascendcl.so ############
if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
add_library(ascendcl SHARED ${SRC_LIST} ${PROTO_SRCS} $<TARGET_OBJECTS:msprofiler>)
else()
add_library(ascendcl SHARED ${SRC_LIST} ${PROTO_HDRS} $<TARGET_OBJECTS:$<IF:$<TARGET_EXISTS:msprofiler>,msprofiler,msprofiler_object>>)
endif()

if(ENABLE_OPEN_SRC)
    add_library(msprofiler_object OBJECT IMPORTED GLOBAL)
    if (msprofiler_ext_LIBRARY_DIR)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_object)
        execute_process(
            COMMAND ar x ${msprofiler_ext_LIBRARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_object
        )
        file(GLOB MSPROFILER_OBJECT_LIST ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_object/*.o)
        set_property(TARGET msprofiler_object PROPERTY IMPORTED_OBJECTS ${MSPROFILER_OBJECT_LIST})
    endif()
endif()

target_include_directories(ascendcl PRIVATE
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/toolchain/ide/ide-daemon/external
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/inc/driver
else()
    ${BASE_DIR}/third_party/inc/
    ${BASE_DIR}/third_party/inc/metadef/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc/framework
    ${BASE_DIR}/third_party/inc/metadef/inc/external
    ${BASE_DIR}/third_party/inc/graphengine/inc/external
    ${BASE_DIR}/third_party/inc/toolchain/ide/ide-daemon/external
    ${BASE_DIR}/third_party/inc/driver
endif()
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
    ${BASE_DIR}/toolchain
)

target_compile_options(ascendcl PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden -O2 -fno-common>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8 /Od>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)
target_compile_definitions(ascendcl PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    LOG_CPP
    FUNC_VISIBILITY
    google=ascend_private
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
)
target_link_options(ascendcl PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)
target_link_directories(ascendcl PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:${TOP_DIR}/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29>
)

set(linklibs ascend_hal_stub)
if (NOT ENABLE_OPEN_SRC)
    if (PRODUCT)
        if (${PRODUCT} STREQUAL  "npuf10")
            set(linklibs )
            if (${TARGET_SYSTEM_NAME} STREQUAL  "Android")
                target_link_directories(ascendcl PRIVATE
                    ${TOP_DIR}/vendor/sdk/hi3796/drv_android
                    ${TOP_DIR}/sdk/hi3796/drv_android
                )
            else()
                target_link_directories(ascendcl PRIVATE
                    ${TOP_DIR}/vendor/sdk/hi3796/drv
                    ${TOP_DIR}/sdk/hi3796/drv
                )
            endif()
            list(APPEND linklibs drvdevdrv drv_dfx)
        else ()
            list(APPEND linklibs ascend_hal_stub)
        endif()
    endif()
else ()
    if(PRODUCT STREQUAL "flr2")
        set(linklibs )
    endif()
    if(PRODUCT STREQUAL "flr3")
        set(linklibs )
        if (${TARGET_SYSTEM_NAME} STREQUAL  "Android")
            list(APPEND linklibs ${ASCEND_DIR}/drv_android)
        else()
            list(APPEND linklibs ${ASCEND_DIR}/drv)
        endif()
        list(APPEND linklibs drvdevdrv drv_dfx)
    endif()
endif()

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
set(linklibs ascend_hal)
endif()

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
target_link_libraries(ascendcl PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    ge_executor
    -Wl,--no-whole-archive
    ge_common_static
    graph_static
    ascend_protobuf_static
    register_static
    error_manager_static
    adump_server
    static_mmpa
    -Wl,--no-as-needed
    c_sec
    runtime
    alog
    ${linklibs}
    -Wl,--as-needed
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:-llog>
    json
)
else()
target_link_libraries(ascendcl PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    -Wl,--no-whole-archive
    adump_server
    static_mmpa
    -Wl,--no-as-needed
    ge_common
    graph
    ascend_protobuf
    register
    error_manager
    ge_executor_shared
    c_sec
    runtime
    alog
    ${linklibs}
    -Wl,--as-needed
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:-llog>
    json
)
endif()

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
set_target_properties(ascendcl PROPERTIES
    LINK_FLAGS "/WHOLEARCHIVE:ge_executor"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME libascendcl
)
endif()

if (NOT ENABLE_OPEN_SRC)
############ libascendcl.a ############
add_library(ascendcl_static STATIC ${SRC_LIST} ${PROTO_SRCS})

target_include_directories(ascendcl_static PRIVATE
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/graphengine/inc/framework
    ${BASE_DIR}/inc/external
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/toolchain/ide/ide-daemon/external
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/inc/driver
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
    ${BASE_DIR}/toolchain
)

target_compile_options(ascendcl_static PRIVATE
     $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden -O2 -fno-common>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8 /Od>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)
target_compile_definitions(ascendcl_static PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    FUNC_VISIBILITY
    LOG_CPP
    google=ascend_private
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
)
target_link_options(ascendcl_static PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)
target_link_directories(ascendcl_static PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:${TOP_DIR}/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29>
)

target_link_libraries(ascendcl_static PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    json
    ascend_protobuf
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:-llog>
)

set_target_properties(ascendcl_static PROPERTIES
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libascendcl,ascendcl>
)
endif()

if(NOT (ENABLE_OPEN_SRC AND ((PRODUCT STREQUAL "flr2") OR (PRODUCT STREQUAL "flr3"))))
############ fwk/libascendcl.so ############
add_library(fwk_ascendcl SHARED
    tensor_data_transfer/tensor_data_transfer.cpp
    ${SRC_LIST}
    $<TARGET_OBJECTS:$<IF:$<TARGET_EXISTS:msprofiler_fwkacl>,msprofiler_fwkacl,msprofiler_fwkacl_object>>
)

if (ENABLE_OPEN_SRC)
    add_library(msprofiler_fwkacl_object OBJECT IMPORTED GLOBAL)
    if (msprofiler_fwkacl_ext_LIBRARY_DIR)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwkacl_object)
        execute_process(
            COMMAND ar x ${msprofiler_fwkacl_ext_LIBRARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwkacl_object
        )
        file(GLOB MSPROFILER_FWKACL_OBJECT_LIST ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwkacl_object/*.o)
        set_property(TARGET msprofiler_fwkacl_object PROPERTY IMPORTED_OBJECTS ${MSPROFILER_FWKACL_OBJECT_LIST})
    endif()
endif()

target_include_directories(fwk_ascendcl PRIVATE
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/inc/tdt
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/toolchain/ide/ide-daemon/external
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/inc/driver
else()
    ${BASE_DIR}/third_party/inc
    ${BASE_DIR}/third_party/inc/tdt
    ${BASE_DIR}/third_party/inc/metadef/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc/framework
    ${BASE_DIR}/third_party/inc/metadef/inc/external
    ${BASE_DIR}/third_party/inc/graphengine/inc/external
    ${BASE_DIR}/third_party/inc/toolchain/ide/ide-daemon/external
    ${BASE_DIR}/third_party/inc/driver
endif()
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
    ${BASE_DIR}/toolchain
)
target_compile_options(fwk_ascendcl PRIVATE
    -O2
    -fvisibility=hidden
    -fno-common
)
target_compile_definitions(fwk_ascendcl PRIVATE
    OS_TYPE=0
    FUNC_VISIBILITY
    google=ascend_private
)
target_link_options(fwk_ascendcl PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)
target_link_directories(fwk_ascendcl PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:${TOP_DIR}/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29>
)

target_link_libraries(fwk_ascendcl PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    adump_server
    static_mmpa
    -Wl,--no-as-needed
    c_sec
    runtime
    slog
    ge_common
    graph
    ascend_protobuf
    register
    error_manager
    ge_runner
    datatransfer
    ascend_hal_stub
    -Wl,--as-needed
    json
    -ldl
    -lrt
)

set_target_properties(fwk_ascendcl PROPERTIES
    OUTPUT_NAME ascendcl
    LIBRARY_OUTPUT_DIRECTORY fwkacl
)
endif()

##################################################################################################################
add_custom_target(
    stub_src ALL
    DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/acl_stub.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/dvpp_stub.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/cblas_stub.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/op_compiler_stub.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/retr_stub.cpp
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/acl_stub.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/dvpp_stub.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/cblas_stub.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/op_compiler_stub.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/retr_stub.cpp
    COMMAND echo "Generating stub files:acl_stub.cpp, dvpp_stub.cpp, cblas_stub.cpp, op_compiler_stub.cpp, retr_stub.cpp."
            && ${HI_PYTHON} ${BASE_DIR}/stub/gen_stubapi.py ${BASE_DIR}/inc/external/acl
            ${CMAKE_CURRENT_BINARY_DIR}/acl_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/dvpp_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/cblas_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/op_compiler_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/retr_stub.cpp
            &&  echo "Generating stub files end."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS stub/gen_stubapi.py ${BASE_DIR}/inc/external/acl
)
#----------------------------------------------------------------------------------------------------------------#

############ stub/libascendcl.so ############
add_library(stub_ascendcl SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/acl_stub.cpp
)

set_target_properties(stub_ascendcl
    PROPERTIES
    OUTPUT_NAME ascendcl
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_include_directories(stub_ascendcl PRIVATE
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
)


target_link_libraries(stub_ascendcl PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)

#----------------------------------------------------------------------------------------------------------------#
############ stub/libacl_dvpp.so ############
add_library(stub_acl_dvpp SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/dvpp_stub.cpp
)

set_target_properties(stub_acl_dvpp
    PROPERTIES
    OUTPUT_NAME acl_dvpp
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_include_directories(stub_acl_dvpp PRIVATE
    ${BASE_DIR}/inc/external
    ${BASE_DIR}
    ${BASE_DIR}/common
)

target_compile_definitions(stub_acl_dvpp PRIVATE
    ENABLE_DVPP_INTERFACE
)

target_link_libraries(stub_acl_dvpp PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)
#----------------------------------------------------------------------------------------------------------------#
############ stub/libacl_cblas.so ############
add_library(stub_acl_cblas SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/cblas_stub.cpp
)

target_include_directories(stub_acl_cblas PRIVATE
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
)
set_target_properties(stub_acl_cblas
    PROPERTIES
    OUTPUT_NAME acl_cblas
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_link_libraries(stub_acl_cblas PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)
#----------------------------------------------------------------------------------------------------------------#
############ stub/libacl_retr.so ############
add_library(stub_acl_retr SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/retr_stub.cpp
)
target_include_directories(stub_acl_retr PRIVATE
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
)
set_target_properties(stub_acl_retr
    PROPERTIES
    OUTPUT_NAME acl_retr
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_link_libraries(stub_acl_retr PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)
#----------------------------------------------------------------------------------------------------------------#
############ stub/libacl_op_compiler.so ############
add_library(stub_acl_op_compiler SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/op_compiler_stub.cpp
)
target_include_directories(stub_acl_op_compiler PRIVATE
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${TOP_DIR}/graphengine/inc/framework
    ${BASE_DIR}/common
)
set_target_properties(stub_acl_op_compiler
    PROPERTIES
    OUTPUT_NAME acl_op_compiler
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_link_libraries(stub_acl_op_compiler PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)
##################################################################################################################

############ libacl_dvpp.so ############
add_library(acl_dvpp SHARED
    single_op/dvpp/channel.cpp
    single_op/dvpp/vpc.cpp
    single_op/dvpp/jpeg.cpp
    single_op/dvpp/vdec.cpp
    single_op/dvpp/venc.cpp
    single_op/dvpp/png.cpp
    single_op/dvpp/base/image_processor.cpp
    single_op/dvpp/base/video_processor.cpp
    single_op/dvpp/mgr/dvpp_manager.cpp
    single_op/dvpp/v100/image_processor_v100.cpp
    single_op/dvpp/v200/image_processor_v200.cpp
    single_op/dvpp/v100/video_processor_v100.cpp
    single_op/dvpp/v200/video_processor_v200.cpp
    single_op/dvpp/common/dvpp_util.cpp
    types/dvpp.cpp
    $<$<STREQUAL:${PRODUCT},npuf10>:stub/libjpeg_lhisi_stub.cpp>
)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
    set(JPEG_INCCLUDE_PATH )
else()
    set(JPEG_INCCLUDE_PATH ${TOP_DIR}/open_source/libjpeg-turbo/include)
endif()

target_include_directories(acl_dvpp PRIVATE
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/inc/mmpa
    ${TOP_DIR}/metadef/inc
else()
    ${BASE_DIR}/third_party/inc/
    ${BASE_DIR}/third_party/inc/mmpa
    ${BASE_DIR}/third_party/inc/metadef/inc
endif()
    ${BASE_DIR}/inc/external
    ${BASE_DIR}/inc
    ${BASE_DIR}
    ${BASE_DIR}/common
    ${BASE_DIR}/toolchain
    ${JPEG_INCCLUDE_PATH}
)
target_compile_options(acl_dvpp PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden -O2 -fno-common>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
)
target_compile_definitions(acl_dvpp PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    FUNC_VISIBILITY
    ENABLE_DVPP_INTERFACE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
)
target_link_options(acl_dvpp PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
    -s
)

target_link_libraries(acl_dvpp PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<STREQUAL:${PRODUCT},npuf10>>:static_turbojpeg>
    static_mmpa
    -Wl,--no-as-needed
    runtime
    alog
    c_sec
    ascendcl
    error_manager
    -Wl,--as-needed
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
set_target_properties(acl_dvpp PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME libacl_dvpp
)
endif()

############ libacl_cblas.so ############
add_library(acl_cblas SHARED
    single_op/blas/gemm_ops.cpp
    single_op/blas/gemv_ops.cpp
)

target_include_directories(acl_cblas PRIVATE
    ${BASE_DIR}
    ${BASE_DIR}/inc/ops
    ${BASE_DIR}/common
    ${BASE_DIR}/type
    ${BASE_DIR}/inc
    ${BASE_DIR}/inc/external
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/metadef/inc/external
else()
    ${BASE_DIR}/third_party/inc/
    ${BASE_DIR}/third_party/inc/metadef/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc/framework
    ${BASE_DIR}/third_party/inc/metadef/inc/external
endif()
)

target_compile_options(acl_cblas PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden -O2 -fno-common>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(acl_cblas PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    FUNC_VISIBILITY
)

target_link_options(acl_cblas PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)

target_link_libraries(acl_cblas PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--no-as-needed
    c_sec
    ascendcl
    alog
    error_manager
    -Wl,--as-needed
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
set_target_properties(acl_cblas PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME libacl_cblas
)
endif()

############ libacl_retr.so ############
add_library(acl_retr SHARED
    single_op/retr/retr_ops.cpp
    types/retr.cpp
    single_op/retr/retr_internal.cpp
    single_op/retr/retr_init.cpp
    single_op/retr/retr_release.cpp
    single_op/retr/retr_repo.cpp
    single_op/retr/retr_accurate.cpp
    single_op/retr/retr_search.cpp
)

target_include_directories(acl_retr PRIVATE
    ${BASE_DIR}
    ${BASE_DIR}/inc/external
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
else()
    ${BASE_DIR}/third_party/inc/
    ${BASE_DIR}/third_party/inc/metadef/inc
endif()
)

target_compile_options(acl_retr PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden -O2 -Werror -fno-common>
    -ftrapv
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(acl_retr PRIVATE
    _FORTIFY_SOURCE=2
    OS_TYPE=0
    FUNC_VISIBILITY
)

target_link_options(acl_retr PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)

target_link_libraries(acl_retr PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--no-as-needed
    ascendcl
    alog
    runtime
    c_sec
    error_manager
    -Wl,--as-needed
    -ldl
)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
set_target_properties(acl_retr PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME libacl_retr
)
endif()

if(NOT (ENABLE_OPEN_SRC AND ((PRODUCT STREQUAL "flr2") OR (PRODUCT STREQUAL "flr3"))))
############ libacl_op_compiler.so ############
add_library(acl_op_compiler SHARED
    single_op/op_compiler.cpp
    single_op/compile/local_compiler.cpp
    single_op/compile/op_compile_processor.cpp
)

target_include_directories(acl_op_compiler PRIVATE
    ${BASE_DIR}
    ${BASE_DIR}/common
    ${BASE_DIR}/inc/external
if(NOT ENABLE_OPEN_SRC)
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
else()
    ${BASE_DIR}/third_party/inc/
    ${BASE_DIR}/third_party/inc/metadef/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc
    ${BASE_DIR}/third_party/inc/graphengine/inc/framework
    ${BASE_DIR}/third_party/inc/metadef/inc/external
    ${BASE_DIR}/third_party/inc/graphengine/inc/external
endif()
)

target_compile_options(acl_op_compiler PRIVATE
    -ftrapv
    -O2
    -Werror
    -Wno-deprecated-declarations
    -fvisibility=hidden
    -fno-common
)

target_compile_definitions(acl_op_compiler PRIVATE
    _FORTIFY_SOURCE=2
    OS_TYPE=0
    FUNC_VISIBILITY
    google=ascend_private
)

target_link_options(acl_op_compiler PRIVATE
    -rdynamic
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)

target_link_libraries(acl_op_compiler PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    static_mmpa
    -Wl,--no-as-needed
    ascendcl
    slog
    runtime
    c_sec
    ge_runner
    error_manager
    ascend_protobuf
    -Wl,--as-needed
    -ldl
     $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
)
endif()

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
install(TARGETS  ascendcl acl_dvpp acl_cblas acl_retr acl_op_compiler ascendcl_static OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
install(FILES $<TARGET_PDB_FILE:ascendcl acl_dvpp acl_cblas acl_retr> DESTINATION ${INSTALL_LIBRARY_DIR} OPTIONAL)
elseif(NOT ENABLE_OPEN_SRC)
install(TARGETS  ascendcl acl_dvpp acl_cblas acl_retr acl_op_compiler ascendcl_static OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)
else()
install(TARGETS  ascendcl acl_dvpp acl_cblas acl_retr OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)
if(NOT (ENABLE_OPEN_SRC AND ((PRODUCT STREQUAL "flr2") OR (PRODUCT STREQUAL "flr3"))))
install(TARGETS  acl_op_compiler OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif()
endif()

install(TARGETS stub_ascendcl stub_acl_dvpp stub_acl_cblas stub_acl_retr stub_acl_op_compiler OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/stub
)

if(NOT (ENABLE_OPEN_SRC AND ((PRODUCT STREQUAL "flr2") OR (PRODUCT STREQUAL "flr3"))))
install(TARGETS fwk_ascendcl OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/fwkacl
)
endif()

add_dependencies(stub_ascendcl stub_src)
add_dependencies(stub_acl_op_compiler stub_src)
add_dependencies(stub_acl_retr stub_src)
add_dependencies(stub_acl_cblas stub_src)
add_dependencies(stub_acl_dvpp stub_src)
