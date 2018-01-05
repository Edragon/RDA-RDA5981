#!/bin/env bash

function notempty() {
    [ "x${1}" != "x" ]
}

# $1 provide the default value, also as an output value
# $2 prompt
# $3 validata function
function input() {
    var="${1}"
    prompt="${2}"
    validate="${3:-notempty}"
    defValue=`eval echo \\$$var`
    tmp=""
    until eval ${validate} "${tmp}"
    do
        if [ "x${defValue}" = "x" ]; then
            echo "${prompt}:"
        else
            echo "${prompt}[$defValue]:"
        fi
        read tmp
        test "x${tmp}" = "x" && tmp="$defValue"
    done
    eval "$var=$tmp"
}

if [ -z "$BASE_DIR" ]; then
BASE_DIR=..
fi

if [ -z "$BUILD_PATH" ]; then
BUILD_PATH=${BASE_DIR}/BUILD
fi

if [ -z "$TARGET" ]; then
    TARGET=UNO_91H
    input TARGET "please choose TARGET(UNO_91H,UNO_81C_U04)" notempty
fi

if [ -z "$TOOL_CHAIN" ]; then
TOOL_CHAIN=ARM
fi

MBEDOS_TARGET=${BUILD_PATH}/mbed-os
IOTOS_TARGET=${BUILD_PATH}/IoT-OS

#DEFAULT_ENABLED_MACRO=""
DEFAULT_ENABLED_MACRO="-DBD_FEATURE_ENABLE_OTA -DMBED_HEAP_STATS_ENABLED -DRDA_SMART_CONFIG -DTEST_BOARD -DRELEASE_VERSION"

mbed compile \
    --library \
    --no-archive \
    --source ${BASE_DIR}/mbed-os \
    --source ${BASE_DIR}/mbed-os/TESTS/config/baidu \
    --source ${BASE_DIR}/IoT-OS/src/iot-baidu-ca/include ${DEFAULT_ENABLED_MACRO} \
    --build=${MBEDOS_TARGET} \
    -m ${TARGET} -t ${TOOL_CHAIN} \
    $* \
    || exit 1


mv ${BASE_DIR}/xmly/OTA_Config.c ${MBEDOS_TARGET}/hal/targets/hal/TARGET_RDA/TARGET_UNO_91H/OTA_Config.c

mbed compile \
    --library \
    --source ${BASE_DIR}/IoT-OS \
    --source ${MBEDOS_TARGET} ${DEFAULT_ENABLED_MACRO} \
    --build=${IOTOS_TARGET} \
    -m ${TARGET} -t ${TOOL_CHAIN} \
    $* \
    || exit 1

rm -rf \
    ${IOTOS_TARGET}/{features,hal,mbedtls,projects,rtos,.temp} \
    ${IOTOS_TARGET}/baidu*.h

rm -rf \
    ${MBEDOS_TARGET}/baidu*.h \
    ${MBEDOS_TARGET}/hal/targets/hal/TARGET_RDA/TARGET_UNO_91H/rda_customize.{o,d}

cp ${BASE_DIR}/mbed-os/hal/targets/hal/TARGET_RDA/TARGET_UNO_91H/rda_customize.c ${MBEDOS_TARGET}/hal/targets/hal/TARGET_RDA/TARGET_UNO_91H/rda_customize.c
cp ${BASE_DIR}/xmly/RDA5991H.sct ${BUILD_PATH}/mbed-os/hal/targets/cmsis/TARGET_RDA/TARGET_UNO_91H/TOOLCHAIN_ARM_STD/RDA5991H.sct

mbed compile \
    --source ${BASE_DIR}/xmly \
    --source ${IOTOS_TARGET} \
    --source ${MBEDOS_TARGET} ${DEFAULT_ENABLED_MACRO} \
    -m ${TARGET} -t ${TOOL_CHAIN} \
    $* \
    || exit 1

mv ${IOTOS_TARGET}/IoT-OS.ar ${IOTOS_TARGET}/duer-os-light.ar
mv ${IOTOS_TARGET} ${BUILD_PATH}/duer-os-light
cp -r ${BASE_DIR}/xmly ${BUILD_PATH}/demo
cp -r ${BASE_DIR}/mbed-os/tools ${BUILD_PATH}/mbed-os/tools

rm -rf \
    ${BUILD_PATH}/demo/{.git,build.sh,.gitignore,ci.yml,RDA5991H.sct} \
    ${BUILD_PATH}/${TARGET} \
    ${BUILD_PATH}/.mbedignore

mv ${MBEDOS_TARGET}/hal/targets/hal/TARGET_RDA/TARGET_UNO_91H/OTA_Config.c ${BASE_DIR}/xmly/OTA_Config.c

mv ${BUILD_PATH} ${BASE_DIR}/RELEASE_${TARGET}
