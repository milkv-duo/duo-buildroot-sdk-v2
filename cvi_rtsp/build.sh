#!/bin/sh

FTP_SERVER_IP=${FTP_SERVER_IP:-10.80.0.5}

usage()
{
    echo "$0 usage"
    echo "    SDK_VER=xx CROSS_COMPILE=xxx ./build"
    echo "    ex:"
    echo "      CROSS_COMPILE=/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- SDK_VER=64bit ./build.sh"
    exit 1
}

if [ "${CROSS_COMPILE}" = "" ]; then
    echo "No CROSS_COMPILE path\n"
    usage
fi

if [ "${SDK_VER}" = "" ]; then
    echo "No SDK_VER\n"
    usage
fi

if [ "${LIVE555_DIR}" = "" ]; then
    mkdir -p prebuilt
    curl ftp://swftp:cvitek@${FTP_SERVER_IP}/sw_rls/third_party/latest/${SDK_VER}/live555.tar.gz \
    --output prebuilt/live555.tar.gz
    if [ $? != 0 ]; then
        echo "fail to download live555..."
        exit 1
    fi
    tar -xf prebuilt/live555.tar.gz -C prebuilt/
    LIVE555_DIR=$(pwd)/prebuilt/
elif [ ! -f ${LIVE555_DIR}/lib/libUsageEnvironment.a ]; then
    echo "please build_3rd_party first !!!!!!"
    exit 1
fi

CC=${CROSS_COMPILE}gcc
CXX=${CROSS_COMPILE}g++
AR=${CROSS_COMPILE}ar
RANLIB=${CROSS_COMPILE}ranlib

make clean;
make CC=${CC} CXX=${CXX} AR=${AR} RANLIB=${RANLIB} LIVE555_DIR=${LIVE555_DIR} MW_DIR=${MW_DIR}
test $? -ne 0 && echo "build cvi rtsp fail!!!" && exit 1
exit 0

