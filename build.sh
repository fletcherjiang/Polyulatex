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
  echo "    -t Build and execute ut"
  echo "    -c Build ut with coverage tag"
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
  ENABLE_ACL_UT="off"
  ENABLE_ACL_COV="off"
  PRODUCT="normal"
  ENABLE_GITEE="off"
  # Process the options
  while getopts 'ustchj:p:g:vS:M' opt
  do
    OPTARG=$(echo ${OPTARG} | tr '[A-Z]' '[a-z]')
    case "${opt}" in
      u)
        ENABLE_ACL_UT="on"
        ;;
      t)
        ENABLE_ACL_COV="on"
        ;;
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

  if [[ "X$ENABLE_ACL_COV" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_ACL_COV=ON"
  fi

  if [[ "X$ENABLE_ACL_UT" = "Xon" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENABLE_ACL_UT=ON"
  fi

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

  if [[ "X$ENABLE_ACL_UT" = "Xon" || "X$ENABLE_ACL_COV" = "Xon" ]]; then
    make ascendcl_utest -j8
  else
    make ${VERBOSE} -j${THREAD_NUM} && make install
  fi
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
  ACL_EXTERNAL_PATH="inc/external/acl"
  ACL_PATH="acllib/"
  ACL_LIB64_PATH="acllib/lib64"
  ACL_INC_PATH="acllib/include/acl"
  FWK_PATH="fwkacllib/"
  FWK_LIB64_PATH="fwkacllib/lib64"
  FWK_INC_PATH="fwkacllib/include/acl"

  rm -rf ${OUTPUT_PATH:?}/${FWK_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${ACL_PATH}/
  
  mk_dir "${OUTPUT_PATH}/${FWK_LIB64_PATH}"
  mk_dir "${OUTPUT_PATH}/${FWK_LIB64_PATH}/stub"
  mk_dir "${OUTPUT_PATH}/${FWK_INC_PATH}"
  mk_dir "${OUTPUT_PATH}/${FWK_INC_PATH}/error_codes"
  mk_dir "${OUTPUT_PATH}/${FWK_INC_PATH}/ops"
  mk_dir "${OUTPUT_PATH}/${ACL_LIB64_PATH}"
  mk_dir "${OUTPUT_PATH}/${ACL_LIB64_PATH}/stub"
  mk_dir "${OUTPUT_PATH}/${ACL_INC_PATH}"
  mk_dir "${OUTPUT_PATH}/${ACL_INC_PATH}/error_codes"
  mk_dir "${OUTPUT_PATH}/${ACL_INC_PATH}/ops"

  find output/ -name acl_lib.tar -exec rm {} \;

  cd "${OUTPUT_PATH}"

  COMMON_LIB=("libacl_cblas.so" "libacl_dvpp.so")

  for lib in "${COMMON_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH}/stub \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH}/stub \;
  done

  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${ACL_LIB64_PATH}/stub \;

  if [ "x${PRODUCT}" != "xflr2" ] && [ "x${PRODUCT}" != "xflr3" ]
  then
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/fwkacl -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH} \;
  fi
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${FWK_LIB64_PATH}/stub \;

  COMMON_INC=("acl_base.h" "acl.h" "acl_mdl.h" "acl_op.h" "acl_prof.h" "acl_rt.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${ACL_INC_PATH} \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${FWK_INC_PATH} \;
  done

  COMMON_INC=("ge_error_codes.h" "rt_error_codes.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/error_codes -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${ACL_INC_PATH}/error_codes \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/error_codes -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${FWK_INC_PATH}/error_codes \;
  done

  COMMON_INC=("acl_cblas.h" "acl_dvpp.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${ACL_INC_PATH}/ops \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${FWK_INC_PATH}/ops \;
  done

  find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "acl_fv.h" -exec cp -f {} ${OUTPUT_PATH}/${ACL_INC_PATH}/ops \;
  find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "acl_tdt.h" -exec cp -f {} ${OUTPUT_PATH}/${FWK_INC_PATH} \;
  find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "acl_op_compiler.h" -exec cp -f {} ${OUTPUT_PATH}/${FWK_INC_PATH} \;

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

# generate output package in tar form, including ut/st libraries/executables for cann
generate_package_for_cann()
{
  cd "${BASEPATH}"

  ACL_LIB_PATH="lib"
  ACL_EXTERNAL_PATH="inc/external/acl"
  RUNTIME_PATH="runtime/"
  RUNTIME_LIB64_PATH="runtime/lib64"
  RUNTIME_INC_PATH="runtime/include/acl"
  COMPILER_PATH="compiler/"
  COMPILER_LIB64_PATH="compiler/lib64"
  COMPILER_INC_PATH="compiler/include/acl"

  rm -rf ${OUTPUT_PATH:?}/${COMPILER_PATH}/
  rm -rf ${OUTPUT_PATH:?}/${RUNTIME_PATH}/

  mk_dir "${OUTPUT_PATH}/${COMPILER_LIB64_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_LIB64_PATH}/stub"
  mk_dir "${OUTPUT_PATH}/${COMPILER_INC_PATH}"
  mk_dir "${OUTPUT_PATH}/${COMPILER_INC_PATH}/error_codes"
  mk_dir "${OUTPUT_PATH}/${COMPILER_INC_PATH}/ops"
  mk_dir "${OUTPUT_PATH}/${RUNTIME_LIB64_PATH}"
  mk_dir "${OUTPUT_PATH}/${RUNTIME_LIB64_PATH}/stub"
  mk_dir "${OUTPUT_PATH}/${RUNTIME_INC_PATH}"
  mk_dir "${OUTPUT_PATH}/${RUNTIME_INC_PATH}/error_codes"
  mk_dir "${OUTPUT_PATH}/${RUNTIME_INC_PATH}/ops"

  find output/ -name acl_lib.tar -exec rm {} \;

  cd "${OUTPUT_PATH}"

  COMMON_LIB=("libacl_cblas.so" "libacl_dvpp.so")

  for lib in "${COMMON_LIB[@]}";
  do
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH}/stub \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "$lib" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH}/stub \;
  done

  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH} \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_retr.so" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_LIB64_PATH}/stub \;

  if [ "x${PRODUCT}" != "xflr2" ] && [ "x${PRODUCT}" != "xflr3" ]
  then
    find ${OUTPUT_PATH}/${ACL_LIB_PATH}/fwkacl -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH} \;
    find ${OUTPUT_PATH}/${ACL_LIB_PATH} -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH} \;
  fi
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libascendcl.so" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH}/stub \;
  find ${OUTPUT_PATH}/${ACL_LIB_PATH}/stub -maxdepth 1 -name "libacl_op_compiler.so" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_LIB64_PATH}/stub \;

  COMMON_INC=("acl_base.h" "acl.h" "acl_mdl.h" "acl_op.h" "acl_prof.h" "acl_rt.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_INC_PATH} \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_INC_PATH} \;
  done

  COMMON_INC=("ge_error_codes.h" "rt_error_codes.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/error_codes -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_INC_PATH}/error_codes \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/error_codes -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_INC_PATH}/error_codes \;
  done

  COMMON_INC=("acl_cblas.h" "acl_dvpp.h")
  for inc in "${COMMON_INC[@]}";
  do
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_INC_PATH}/ops \;
    find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "$inc" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_INC_PATH}/ops \;
  done

  find ${BASEPATH}/${ACL_EXTERNAL_PATH}/ops -maxdepth 1 -name "acl_fv.h" -exec cp -f {} ${OUTPUT_PATH}/${RUNTIME_INC_PATH}/ops \;
  find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "acl_tdt.h" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_INC_PATH} \;
  find ${BASEPATH}/${ACL_EXTERNAL_PATH} -maxdepth 1 -name "acl_op_compiler.h" -exec cp -f {} ${OUTPUT_PATH}/${COMPILER_INC_PATH} \;

  if [ "x${PLATFORM}" = "xtrain" ]
  then
    tar -cf acl_lib.tar compiler
  elif [ "x${PLATFORM}" = "xinference" ]
  then
    tar -cf acl_lib.tar runtime
  elif [ "x${PLATFORM}" = "xall" ]
  then
    tar -cf acl_lib.tar compiler runtime
  fi
}

main()
{
  checkopts "$@"

  # Acl build start
  echo "---------------- ACL build start ----------------"
  g++ -v
  mk_dir ${OUTPUT_PATH}
  build_acl
  if [[ "$?" -ne 0 ]]; then
    echo "ACL build failed.";
    exit 1;
  fi
  echo "---------------- ACL build finished ----------------"

  rm -f ${OUTPUT_PATH}/libgmock*.so
  rm -f ${OUTPUT_PATH}/libgtest*.so
  rm -f ${OUTPUT_PATH}/lib*_stub.so

  chmod -R 750 ${OUTPUT_PATH}
  find ${OUTPUT_PATH} -name "*.so*" -print0 | xargs -0 chmod 500

  echo "---------------- ACL output generated ----------------"

  if [[ "X$ENABLE_ACL_UT" = "Xon" || "X$ENABLE_ACL_COV" = "Xon" ]]; then
    cp ${BUILD_PATH}/tests/ut/acl/ascendcl_utest ${OUTPUT_PATH}

    RUN_TEST_CASE=${OUTPUT_PATH}/ascendcl_utest && ${RUN_TEST_CASE}
    if [[ "$?" -ne 0 ]]; then
      echo "!!! UT FAILED, PLEASE CHECK YOUR CHANGES !!!"
      echo -e "\033[31m${RUN_TEST_CASE}\033[0m"
      exit 1;
    fi
    echo "Generated coverage statistics, please wait..."
    cd ${BASEPATH}
    rm -rf ${BASEPATH}/cov
    mkdir ${BASEPATH}/cov
    lcov -c -d build/tests/ut/acl -o cov/tmp.info
    lcov -r cov/tmp.info '*/output/*' '*/build/opensrc/*' '*/build/proto/*' '*/third_party/*' '*/tests/*' '/usr/local/*' '/usr/include/*' -o cov/coverage.info
    cd ${BASEPATH}/cov
    genhtml coverage.info
  fi

  if [[ "X$ENABLE_ACL_COV" = "Xoff" ]]; then
    if [[ "X$ALL_IN_ONE_ENABLE" = "X1" ]]; then
      generate_package_for_cann
    else
      generate_package
    fi
    echo "---------------- ACL package archive generated ----------------"
  fi
}

main "$@"
