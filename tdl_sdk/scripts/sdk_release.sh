#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CVI_TDL_ROOT=$(readlink -f $SCRIPT_DIR/../)
TMP_WORKING_DIR=$CVI_TDL_ROOT/tmp
BUILD_WORKING_DIR=$TMP_WORKING_DIR/build_sdk
BUILD_DOWNLOAD_DIR=$TMP_WORKING_DIR/_deps

if [[ "$1" == "Release" ]]; then
    BUILD_TYPE=Release
else
    BUILD_TYPE=SDKRelease
fi

FTP_SERVER_IP=${FTP_SERVER_IP:-10.80.0.5}

CONFIG_DUAL_OS="${CONFIG_DUAL_OS:-OFF}"
if [[ "$CONFIG_DUAL_OS" == "y" ]]; then
    CONFIG_DUAL_OS="ON"
fi

REPO_USER=""
CURRENT_USER="$(git config user.name)"
if [[ "${CURRENT_USER}" != "sw_jenkins" ]]; then
REPO_USER="$(git config user.name)@"
fi
echo "repo user : $REPO_USER"

if [ -d "${BUILD_WORKING_DIR}" ]; then
    echo "Cleanup tmp folder."
    rm -rf $BUILD_WORKING_DIR
fi

echo "Creating tmp working directory."
mkdir -p $BUILD_WORKING_DIR
pushd $BUILD_WORKING_DIR

# Check cmake version
CMAKE_VERSION="$(cmake --version | grep 'cmake version' | sed 's/cmake version //g')"
CMAKE_REQUIRED_VERSION="3.18.4"
echo "Checking cmake..."
if [ "$(printf '%s\n' "$CMAKE_REQUIRED_VERSION" "$CMAKE_VERSION" | sort -V | head -n1)" = "$CMAKE_REQUIRED_VERSION" ]; then
    echo "Current cmake version is ${CMAKE_VERSION}, satisfy the required version ${CMAKE_REQUIRED_VERSION}"
    CMAKE_BIN=$(which cmake)
else
    echo "Cmake minimum required version is ${CMAKE_REQUIRED_VERSION}, trying to download from ftp."
    if [ ! -f cmake-3.18.4-Linux-x86_64.tar.gz ]; then
        wget ftp://swftp:cvitek@${FTP_SERVER_IP}/sw_rls/third_party/cmake/cmake-3.18.4-Linux-x86_64.tar.gz
    fi
    tar zxf cmake-3.18.4-Linux-x86_64.tar.gz
    CMAKE_BIN=$PWD/cmake-3.18.4-Linux-x86_64/bin/cmake
fi

if [[ "$SDK_VER" == "uclibc" ]]; then
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-uclibc-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm/usr
elif [[ "$SDK_VER" == "32bit" ]]; then
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-gnueabihf-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm/usr
elif [[ "$SDK_VER" == "64bit" ]]; then
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-aarch64-linux.cmake
    if [[ "$CROSS_COMPILE" == "aarch64-none-linux-gnu-" ]]; then
        TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain1131-aarch64-linux.cmake
    elif [[ "$CROSS_COMPILE" == "aarch64-linux-" ]]; then
        TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain930-aarch64-linux.cmake
    fi
    SYSTEM_PROCESSOR=ARM64
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm64/usr
elif [[ "$SDK_VER" == "glibc_riscv64" ]]; then
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-riscv64-linux.cmake
    SYSTEM_PROCESSOR=RISCV
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
elif [[ "$SDK_VER" == "musl_riscv64" ]]; then
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-riscv64-musl.cmake
    SYSTEM_PROCESSOR=RISCV
else
    echo "Wrong SDK_VER=$SDK_VER"
    exit 1
fi

if [[ "$CHIP_ARCH" == "CV183X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV182X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV181X" ]]; then
    USE_TPU_IVE=OFF
elif [[ "$CHIP_ARCH" == "CV180X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "SOPHON" ]]; then
    USE_TPU_IVE=OFF
else
    echo "Unsupported chip architecture: ${CHIP_ARCH}"
    exit 1
fi

$CMAKE_BIN -G Ninja $CVI_TDL_ROOT \
        -DCVI_PLATFORM=$CHIP_ARCH \
        -DCVI_SYSTEM_PROCESSOR=$SYSTEM_PROCESSOR \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DENABLE_CVI_TDL_CV_UTILS=ON \
        -DMLIR_SDK_ROOT=$TPU_SDK_INSTALL_PATH \
        -DMIDDLEWARE_SDK_ROOT=$MW_PATH \
        -DTPU_IVE_SDK_ROOT=$IVE_SDK_INSTALL_PATH \
        -DCMAKE_INSTALL_PREFIX=$AI_SDK_INSTALL_PATH \
        -DTOOLCHAIN_ROOT_DIR=$HOST_TOOL_PATH \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
        -DKERNEL_ROOT=$KERNEL_ROOT \
        -DUSE_TPU_IVE=$USE_TPU_IVE \
        -DMW_VER=$MW_VER \
        -DBUILD_DOWNLOAD_DIR=$BUILD_DOWNLOAD_DIR \
        -DREPO_USER=$REPO_USER \
        -DCONFIG_DUAL_OS=$CONFIG_DUAL_OS \
        -DFTP_SERVER_IP=$FTP_SERVER_IP

ninja -j8 || exit 1
ninja install || exit 1
popd

# echo "trying to build sample in released folder."
# cp -rf $CVI_TDL_ROOT/scripts/compile_sample.sh ${AI_SDK_INSTALL_PATH}/sample
# pushd "${AI_SDK_INSTALL_PATH}/sample"
#     KERNEL_ROOT=$KERNEL_ROOT\
#     MW_PATH=$MW_PATH\
#     TPU_PATH=$TPU_SDK_INSTALL_PATH\
#     IVE_PATH=$IVE_SDK_INSTALL_PATH\
#     USE_TPU_IVE=$USE_TPU_IVE\
#     CHIP=$CHIP_ARCH\
#     SDK_VER=$SDK_VER\
#     source compile_sample.sh || exit 1
# popd

if [[ "$BUILD_TYPE" == "Release" ]]; then
    # Clone doc to aisdk
    remote_user="swftp"
    remote_host="10.80.0.5"
    remote_password="cvitek"
    current_date=$(date +"%Y-%m-%d")
    remote_base_path="/sw_rls/daily_build/cvitek_develop_docs/master/${current_date}/CV180x_CV181x/"
    files_to_download=(
        "en/01.software/TPU/TDL_SDK_Software_Development_Guide/build/TDLSDKSoftwareDevelopmentGuide_en.pdf"
        "zh/01.software/TPU/TDL_SDK_Software_Development_Guide/build/TDLSDKSoftwareDevelopmentGuide_zh.pdf"
        "zh/01.software/TPU/YOLO_Development_Guide/build/YOLODevelopmentGuide_zh.pdf"
        "en/01.software/TPU/YOLO_Development_Guide/build/YOLODevelopmentGuide_en.pdf"
    )
    echo "downloading..."
    for file_path in "${files_to_download[@]}"; do
        file_name=$(basename "$file_path")
        remote_path="${remote_base_path}${file_path}"
        wget --user="$remote_user" --password="$remote_password" "ftp://${remote_host}/${remote_path}" -P "${AI_SDK_INSTALL_PATH}/doc" || { echo "Failed to download $file_name"; exit 1; }
    done
fi

rm -rf ${AI_SDK_INSTALL_PATH}/sample/tmp_install
