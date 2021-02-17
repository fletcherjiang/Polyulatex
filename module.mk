LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/stub/Makefile

acl_src_file := \
    runtime/device.cpp \
    runtime/event.cpp \
    runtime/stream.cpp \
    runtime/memory.cpp \
    runtime/context.cpp \
    runtime/callback.cpp \
    runtime/group.cpp \
    model/model.cpp \
    model/acl_aipp.cpp \
    model/aipp_param_check.cpp \
    common/acl.cpp \
    common/log_inner.cpp \
    common/log.cpp \
    common/json_parser.cpp \
    common/error_codes_api.cpp \
    single_op/compile/op_compiler.cpp \
    single_op/compile/op_compile_service.cpp \
    single_op/builtin/cast_op.cpp \
    single_op/builtin/transdata_op.cpp \
    single_op/op.cpp \
    single_op/op_executor.cpp \
    single_op/op_model_cache.cpp \
    single_op/op_model_manager.cpp \
    single_op/op_model_parser.cpp \
    types/acl_op.cpp \
    types/fp16.cpp \
    types/fp16_impl.cpp \
    types/op_attr.cpp \
    types/op_model.cpp \
    types/tensor_desc_internal.cpp \
    utils/array_utils.cpp \
    utils/attr_utils.cpp \
    utils/file_utils.cpp \
    utils/string_utils.cpp \
    utils/math_utils.cpp \
    toolchain/dump.cpp \
    toolchain/profiling.cpp \
    toolchain/profiling_manager.cpp \
    toolchain/resource_statistics.cpp \
    single_op/compile/op_kernel_selector.cpp \
    single_op/compile/op_kernel_registry.cpp \
    single_op/executor/op_task.cpp \
    single_op/executor/resource_manager.cpp \
    single_op/executor/stream_executor.cpp \

# libascendcl_host
include $(CLEAR_VARS)

LOCAL_MODULE := libascendcl

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY -Dgoogle=ascend_private

LOCAL_SRC_FILES := $(acl_src_file)

LOCAL_SRC_FILES += proto/om.proto

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/toolchain \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/json/include \
    $(TOPDIR)toolchain/ide/ide-daemon/external \
    $(TOPDIR)inc/driver \

LOCAL_SHARED_LIBRARIES:= libc_sec libruntime libslog libmsprof stub/libascend_hal
LOCAL_STATIC_LIBRARIES:= libge_common libgraph libmsprofiler libascend_protobuf libregister liberror_manager libadump_server libmmpa
LOCAL_WHOLE_STATIC_LIBRARIES:= libge_executor

LOCAL_LDFLAGS += -rdynamic -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL

include $(BUILD_HOST_SHARED_LIBRARY)

# libascendcl_host_static
include $(CLEAR_VARS)

LOCAL_MODULE := libascendcl

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY

LOCAL_SRC_FILES := $(acl_src_file)

LOCAL_SRC_FILES += proto/om.proto

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/toolchain \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/json/include \
    $(TOPDIR)toolchain/ide/ide-daemon/external \
    $(TOPDIR)inc/driver \

LOCAL_LDFLAGS += -rdynamic -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL

LOCAL_UNINSTALLABLE_MODULE := false

include $(BUILD_HOST_STATIC_LIBRARY)

# libascendcl_device
include $(CLEAR_VARS)

LOCAL_MODULE := libascendcl

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY -Dgoogle=ascend_private

LOCAL_SRC_FILES := $(acl_src_file)

LOCAL_SRC_FILES += proto/om.proto

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/toolchain \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/json/include \
    $(TOPDIR)toolchain/ide/ide-daemon/external \
    $(TOPDIR)inc/driver \

LOCAL_SHARED_LIBRARIES:= libc_sec libruntime libslog
LOCAL_STATIC_LIBRARIES:= libge_common libgraph libascend_protobuf libregister liberror_manager libadump_server libmmpa
LOCAL_WHOLE_STATIC_LIBRARIES:= libge_executor libmsprofiler

ifeq ($(device_os), android)
LOCAL_LDLIBS += -L$(PWD)/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29 -llog
endif

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,libge_common.a -Wl,--exclude-libs,libgraph.a -Wl,--exclude-libs,libascend_protobuf.a -Wl,--exclude-libs,libregister.a -Wl,--exclude-libs,liberror_manager.a -Wl,--exclude-libs,libadump_server.a -Wl,--exclude-libs,libmmpa.a -Wl,--exclude-libs,libge_executor.a

ifneq ($(device_os),android)
LOCAL_LDFLAGS += -lrt
endif

ifeq ($(TARGET_PRODUCT),lhisi)
ifeq ($(device_os),android)
    LOCAL_LDFLAGS += -L $(PWD)/sdk/hi3796/drv_android
else
    LOCAL_LDFLAGS += -L $(PWD)/sdk/hi3796/drv
endif
    LOCAL_LDFLAGS += -ldrvdevdrv -ldrv_dfx
else
    LOCAL_SHARED_LIBRARIES += libascend_hal
endif

include $(BUILD_SHARED_LIBRARY)

# libascendcl_device_static
include $(CLEAR_VARS)

LOCAL_MODULE := libascendcl

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY

LOCAL_SRC_FILES := $(acl_src_file)

LOCAL_SRC_FILES += proto/om.proto

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/toolchain \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/json/include \
    $(TOPDIR)toolchain/ide/ide-daemon/external \
    $(TOPDIR)inc/driver \

ifeq ($(device_os), android)
LOCAL_LDLIBS += -L$(PWD)/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29 -llog
endif

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL

LOCAL_UNINSTALLABLE_MODULE := false

include $(BUILD_STATIC_LIBRARY)

# libascendcl_host
include $(CLEAR_VARS)

LOCAL_MODULE := fwkacl/libascendcl

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY -Dgoogle=ascend_private

LOCAL_SRC_FILES := $(acl_src_file)

LOCAL_SRC_FILES += tensor_data_transfer/tensor_data_transfer.cpp

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)inc/tdt \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/toolchain \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/json/include \
    $(TOPDIR)toolchain/ide/ide-daemon/external \
    $(TOPDIR)inc/driver \

LOCAL_SHARED_LIBRARIES:= libc_sec libruntime libslog libmsprof libge_common libgraph libascend_protobuf libregister liberror_manager libge_runner libdatatransfer stub/libascend_hal
LOCAL_STATIC_LIBRARIES:= libadump_server libmmpa

LOCAL_LDFLAGS += -rdynamic -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL

include $(BUILD_HOST_SHARED_LIBRARY)

# libacl_op_compiler host
include $(CLEAR_VARS)

LOCAL_MODULE := libacl_op_compiler

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY -Dgoogle=ascend_private

LOCAL_SRC_FILES := single_op/op_compiler.cpp \
    single_op/compile/local_compiler.cpp \
    single_op/compile/op_compile_processor.cpp

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/common \
    third_party/protobuf/include

LOCAL_SHARED_LIBRARIES:= libc_sec libascendcl libruntime libslog libmsprof libge_runner liberror_manager libascend_protobuf
LOCAL_STATIC_LIBRARIES:= libmmpa

LOCAL_LDFLAGS += -rdynamic -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL
include $(BUILD_HOST_SHARED_LIBRARY)

# stub/libacl_op_compiler host
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_op_compiler
LOCAL_CFLAGS += -O2 -DOS_TYPE=0
LOCAL_SRC_FILES := ../out/acl/stub/op_compiler_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(TOPDIR)graphengine/inc/framework \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common

LOCAL_LDFLAGS += -lrt -ldl

include $(BUILD_HOST_SHARED_LIBRARY)


# stub/libascendcl host
include $(CLEAR_VARS)

LOCAL_MODULE := stub/libascendcl
LOCAL_CFLAGS += -O2 -DOS_TYPE=0

LOCAL_SRC_FILES := ../out/acl/stub/acl_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \

include $(BUILD_HOST_SHARED_LIBRARY)

# stub/libascendcl device
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libascendcl
LOCAL_CFLAGS += -O2 -DOS_TYPE=0

LOCAL_SRC_FILES := ../out/acl/stub/acl_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \

LOCAL_LDFLAGS += -ldl
include $(BUILD_SHARED_LIBRARY)

# stub/libacl_dvpp host
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_dvpp
LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -DENABLE_DVPP_INTERFACE

LOCAL_SRC_FILES := ../out/acl/stub/dvpp_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \

LOCAL_LDFLAGS += -lrt -ldl
include $(BUILD_HOST_SHARED_LIBRARY)

# stub/libacl_dvpp device
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_dvpp
LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -DENABLE_DVPP_INTERFACE

LOCAL_SRC_FILES := ../out/acl/stub/dvpp_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \

LOCAL_LDFLAGS += -ldl
include $(BUILD_SHARED_LIBRARY)

acl_dvpp_src_file := \
    single_op/dvpp/channel.cpp \
    single_op/dvpp/vpc.cpp \
    single_op/dvpp/jpeg.cpp \
    single_op/dvpp/vdec.cpp \
    single_op/dvpp/venc.cpp \
    single_op/dvpp/png.cpp \
    single_op/dvpp/base/image_processor.cpp \
    single_op/dvpp/base/video_processor.cpp \
    single_op/dvpp/mgr/dvpp_manager.cpp \
    single_op/dvpp/v100/image_processor_v100.cpp \
    single_op/dvpp/v200/image_processor_v200.cpp \
    single_op/dvpp/v100/video_processor_v100.cpp \
    single_op/dvpp/v200/video_processor_v200.cpp \
    single_op/dvpp/common/dvpp_util.cpp \
    types/dvpp.cpp \

# libacl_dvpp host
include $(CLEAR_VARS)

LOCAL_MODULE := libacl_dvpp

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -DENABLE_DVPP_INTERFACE -fvisibility=hidden -DFUNC_VISIBILITY -std=c++11

LOCAL_SRC_FILES := $(acl_dvpp_src_file)

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(TOPDIR)inc \
    $(LOCAL_PATH)/inc \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \
    $(TOPDIR)third_party/libjpeg/include

ifneq ($(filter lhisi, $(product)),)
LOCAL_SRC_FILES += stub/libjpeg_lhisi_stub.cpp
else
LOCAL_STATIC_LIBRARIES:= libjpeg
endif
LOCAL_SHARED_LIBRARIES:= libruntime libslog libc_sec libascendcl

LOCAL_LDFLAGS += -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL -s
include $(BUILD_HOST_SHARED_LIBRARY)

# libacl_dvpp device
include $(CLEAR_VARS)

LOCAL_MODULE := libacl_dvpp

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -DENABLE_DVPP_INTERFACE -fvisibility=hidden -DFUNC_VISIBILITY -std=c++11

LOCAL_SRC_FILES := $(acl_dvpp_src_file)

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(TOPDIR)inc \
    $(LOCAL_PATH)/inc \
    $(TOPDIR)libc_sec/include \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common \
    $(TOPDIR)third_party/libjpeg/include

ifneq ($(filter lhisi, $(product)),)
LOCAL_SRC_FILES += stub/libjpeg_lhisi_stub.cpp
else
LOCAL_STATIC_LIBRARIES:= libjpeg
endif
LOCAL_SHARED_LIBRARIES:= libruntime libslog libc_sec libascendcl

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL -s
include $(BUILD_SHARED_LIBRARY)

# stub/libacl_cblas host
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_cblas
LOCAL_CFLAGS += -O2 -DOS_TYPE=0
LOCAL_SRC_FILES := ../out/acl/stub/cblas_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common

LOCAL_LDFLAGS += -lrt -ldl

include $(BUILD_HOST_SHARED_LIBRARY)

# stub/libacl_cblas device
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_cblas
LOCAL_CFLAGS += -O2 -DOS_TYPE=0
LOCAL_SRC_FILES := ../out/acl/stub/cblas_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common

LOCAL_LDFLAGS += -ldl
include $(BUILD_SHARED_LIBRARY)

# acl_cblas host
acl_cblas_src_file := \
    single_op/blas/gemm_ops.cpp \
    single_op/blas/gemv_ops.cpp \

include $(CLEAR_VARS)
LOCAL_MODULE := libacl_cblas

LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY

LOCAL_SRC_FILES := $(acl_cblas_src_file)

LOCAL_C_INCLUDES := \
            $(LOCAL_PATH) \
            $(LOCAL_PATH)/inc/ops \
            $(LOCAL_PATH)/common \
            $(LOCAL_PATH)/type \
            $(LOCAL_PATH)/inc \
            $(TOPDIR)libc_sec/include \
            $(TOPDIR)inc \
            $(TOPDIR)metadef/inc \
            $(TOPDIR)graphengine/inc/framework \
            $(TOPDIR)inc/external \
            $(TOPDIR)metadef/inc/external

LOCAL_SHARED_LIBRARIES:= libascendcl libslog

LOCAL_LDFLAGS += -lrt -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL
include $(BUILD_HOST_SHARED_LIBRARY)

# acl_cblas device
include $(CLEAR_VARS)
LOCAL_MODULE := libacl_cblas
LOCAL_CFLAGS += -O2 -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY
LOCAL_SRC_FILES := $(acl_cblas_src_file)

LOCAL_C_INCLUDES := \
            $(LOCAL_PATH) \
            $(LOCAL_PATH)/inc/ops \
            $(LOCAL_PATH)/common \
            $(LOCAL_PATH)/type \
            $(LOCAL_PATH)/inc \
            $(TOPDIR)libc_sec/include \
            $(TOPDIR)inc \
            $(TOPDIR)metadef/inc \
            $(TOPDIR)graphengine/inc/framework \
            $(TOPDIR)inc/external \
            $(TOPDIR)metadef/inc/external

LOCAL_SHARED_LIBRARIES:= libascendcl libslog

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

acl_retr_src_file := \
    single_op/retr/retr_ops.cpp \
    types/retr.cpp \
    single_op/retr/retr_internal.cpp \
    single_op/retr/retr_init.cpp \
    single_op/retr/retr_release.cpp \
    single_op/retr/retr_repo.cpp \
    single_op/retr/retr_accurate.cpp \
    single_op/retr/retr_search.cpp \

# libacl_retr host
include $(CLEAR_VARS)

LOCAL_MODULE := libacl_retr

LOCAL_CFLAGS += -D_FORTIFY_SOURCE=2 -ftrapv -O2 -Werror -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY

LOCAL_SRC_FILES := $(acl_retr_src_file)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TOPDIR)inc \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)inc/external \

LOCAL_SHARED_LIBRARIES := libascendcl libslog libruntime libc_sec

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL
include $(BUILD_HOST_SHARED_LIBRARY)

# libacl_retr device
include $(CLEAR_VARS)

LOCAL_MODULE := libacl_retr

LOCAL_CFLAGS += -D_FORTIFY_SOURCE=2 -ftrapv -O2 -Werror -DOS_TYPE=0 -fvisibility=hidden -DFUNC_VISIBILITY

LOCAL_SRC_FILES := $(acl_retr_src_file)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TOPDIR)inc \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)inc/external \

LOCAL_SHARED_LIBRARIES := libascendcl libslog libruntime libc_sec

LOCAL_LDFLAGS += -rdynamic -ldl -Wl,-Bsymbolic -Wl,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)

# stub/libacl_retr host
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_retr
LOCAL_CFLAGS += -O2 -DOS_TYPE=0
LOCAL_SRC_FILES := ../out/acl/stub/retr_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common

LOCAL_LDFLAGS += -lrt -ldl

include $(BUILD_HOST_SHARED_LIBRARY)

# stub/libacl_retr device
include $(CLEAR_VARS)
LOCAL_MODULE := stub/libacl_retr
LOCAL_CFLAGS += -O2 -DOS_TYPE=0
LOCAL_SRC_FILES := ../out/acl/stub/retr_stub.cpp \

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc/external \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/common

LOCAL_LDFLAGS += -ldl
include $(BUILD_SHARED_LIBRARY)
