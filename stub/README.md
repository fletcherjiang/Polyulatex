# "stub"  usage:

## Description

- File libacl_cblas.so ,libacl_dvpp.so , libacl_retr.so ,libascendcl.so in this directory are used to comiple the code based on the ACL APIs.

# Attention

- Don't link other library except libacl_cblas.so ,libacl_dvpp.so , libacl_retr.so ,libascendcl.so. Linking unnecessary .so files may lead to compatibility problems during subsequent version upgrade.

# Usage

## Compile: compile the application invoking the ACL APIs.

Makefile:

'''

Acllib_INCLUDE_DIR := $(ASCEND_PATH)/acllib/include
LOCAL_MODULE_NAME := resnet50
CC := g++
CFLAGS := -std=c++11 -fPIC -O0 -g -Wall
SRCS := $(wildcard $(LOCAL_DIR)/main.cpp $(LOCAL_DIR)/model_process.cpp $(LOCAL_DIR)/sample_process.cpp $(LOCAL_DIR)/utils.cpp)

INCLUDES := -I $(Acllib_INCLUDE_DIR)/acl \
            -I $(Acllib_INCLUDE_DIR)/acl/ops

LIBS := -L ${ASCEND_PATH}/acllib/lib64/stub \
    -lascendcl

resnet50:
    mkdir -p out
    $(CC) $(SRCS) $(INCLUDES) $(LIBS) $(CFLAGS) -o ./out/$(LOCAL_MODULE_NAME)
clean:
    rm -rf out

'''
make

## Run the application after set the LD_LIBRARY_PATH to include the real path of the library which locates in the directory of acllib/lib64

export LD_LIBRARY_PATH= $(ASCEND_PATH)/acllib/lib64
 -  ./ resnet50
