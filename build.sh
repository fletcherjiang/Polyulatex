#!/bin/bash
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

set -e
BASEPATH=$(cd "$(dirname $0)"; pwd)
OUTPUT_PATH="${BASEPATH}/output"
export BUILD_PATH="${BASEPATH}/build/"

# print usage message
usage()
{
  echo "Usage:"
  echo "sh build.sh [-j[n]] [-h] [-v] [-s] [-t] [-u] [-c] [-S on|off] [-M]"
  echo ""
  echo "Options:"
  echo "    -h Print usage"
  echo "    -j[n] Set the number of threads used for building ACL, default is 8"
  echo "    -p Build inference or train"
  echo "    -v Display build command"
  echo "    -S Enable enable download cmake compile dependency from gitee , default off"
  echo "to be continued ..."
}

# check value of input is 'on' or 'off'
# usage: check_on_off arg_value arg_name
check_on_off()
{
  if [[ "X$1" != "Xon" && "X$1" != "Xoff" ]]; then
    echo "Invalid value $1 for option -$2"
    usage
    exit 1
  fi
}

# parse and set options
checkopts()
{
  VERBOSE=""
  THREAD_NUM=8
  PLATFORM="all"
  PRODUCT="normal"
  ENABLE_GITEE="off"
  # Process the options
  while getopts 'ustchj:p:g:vS:M' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      h)
        usage
        exit 0
        ;;
      j)
        THREAD_NUM=$OPTARG
        ;;
      v)
        VERBOSE="VERBOSE=1"
        ;;
      p)
        PLATFORM=$OPTARG
        ;;
      g)
        PRODUCT=$OPTARG
        ;;
      S)
        check_on_off $OPTARG S
        ENABLE_GITEE="$OPTARG"
        echo "enable download from gitee"
        ;;
      *)
        echo "Undefined option: ${opt}"
        usage
        exit 1
    esac
  done
}

mk_dir() {
    local create_dir="$1"  # the target to make

    mkdir -pv "${create_dir}"
    echo "created ${create_dir}"
}

# create build path
build_acl()
{
  echo "create build directory and build ACL";
  mk_dir "${BUILD_PATH}"
  cd "${BUILD_PATH}"
  CMAKE_ARGS="-DBUILD_PATH=$BUILD_PATH"


  if [[ "X$ENABLE_GITEE" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_GITEE=ON"
  fi
  
  CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_OPEN_SRC=True -DCMAKE_INSTALL_PREFIX=${OUTPUT_PATH} -DPLATFORM=${PLATFORM} -DPRODUCT=${PRODUCT}"
  echo "${CMAKE_ARGS}"
  cmake ${CMAKE_ARGS} ..
  if [ 0 -ne $? ]
  then
    echo "execute command: cmake ${CMAKE_ARGS} .. failed."
    return 1
  fi

  make ${VERBOSE} -j${THREAD_NUM} && make install
  if [ 0 -ne $? ]
  then
    echo "execute command: make ${VERBOSE} -j${THREAD_NUM} && make install failed."
    return 1
  fi
  echo "ACL build success!"
}

# generate output package in tar form, including ut/st libraries/executables
generate_package()
{
  cd "${BASEPATH}"

  ACL_LIB_PATH="lib"
  ACL_PATH="acllib/lib64"
  FWK_PATH="fwkacllib/lib64"

  COMMON_LIB=("libacl_cblas.so" "libacl_dvpp.so")

  rm -rf ${OUTPUT_PATH:?}/${FWK_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${ACL_PATH}/
  
  mk_dir "${OUTPUT_PATH}/${FWK_PATH}"
  mk_dir "${OUTPUT_PATH}/${FWK_PATH}/stub"
  mk_dir "${OUTPUT_PATH}/${ACL_PATH}"
  mk_dir "${OUTPUT_PATH}/${ACL_PATH}/stub"

  find output/ -name acl_lib.tar -exec rm {} \;

  cd "${OUTPUT_PATH}"

  for lib in "${COMMON_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH}/stub \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/stub \;
  done

  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_PATH}/stub \;

  if [ "x${PRODUCT}" != "xflr2" ] && [ "x${PRODUCT}" != "xflr3" ]
  then
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/fwkacl -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH} \;
  fi
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_PATH}/stub \;

  if [ "x${PLATFORM}" = "xtrain" ]
  then
    tar -cf acl_lib.tar fwkacllib
  elif [ "x${PLATFORM}" = "xinference" ]
  then
    tar -cf acl_lib.tar acllib
  elif [ "x${PLATFORM}" = "xall" ]
  then
    tar -cf acl_lib.tar fwkacllib acllib
  fi
}

main()
{
  checkopts "$@"

  # Acl build start
  echo "---------------- ACL build start ----------------"
  g++ -v
  mk_dir ${OUTPUT_PATH}
  build_acl || { echo "ACL build failed."; return; }
  echo "---------------- ACL build finished ----------------"

  chmod -R 750 ${OUTPUT_PATH}
  find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

  echo "---------------- ACL output generated ----------------"
  generate_package
  echo "---------------- ACL package archive generated ----------------"
}

main "$@"