#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CVI_TDL_ROOT=$(readlink -f $SCRIPT_DIR/../)
TMP_WORKING_DIR=$CVI_TDL_ROOT/tmp
BUILD_WORKING_DIR=$TMP_WORKING_DIR/build_sdk
BUILD_DOWNLOAD_DIR=$TMP_WORKING_DIR/_deps

CHIP_ARCH="CV181X"
SDK_VER="musl_riscv64"
BOARD="wevb_riscv64_sd"
TOP_DIR=$(dirname "$(pwd)")
INSTALL_PATH="$TOP_DIR"/install
PROJECT_FULLNAME="$CHIP_ARCH"_"$BOARD"
OUTPUT_DIR="$INSTALL_PATH"/soc_"$PROJECT_FULLNAME"
TPU_OUTPUT_PATH="$OUTPUT_DIR"/tpu_"$SDK_VER"
TPU_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_tpu_sdk
MW_PATH="$TOP_DIR"/cvi_mpi
IVE_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_ive_sdk
AI_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_ai_sdk
TOOLCHAIN_PATH="$TOP_DIR"/host-tools
KERNEL_PATH="$TOP_DIR"/linux_5.10

if [[ "$1" == "Release" ]]; then
    BUILD_TYPE=Release
else
    BUILD_TYPE=SDKRelease
fi

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
        wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz
    fi
    tar zxf cmake-3.18.4-Linux-x86_64.tar.gz
    CMAKE_BIN=$PWD/cmake-3.18.4-Linux-x86_64/bin/cmake
fi

  # toolchain path
  CROSS_COMPILE_PATH_64="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
  CROSS_COMPILE_PATH_32="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf
  CROSS_COMPILE_PATH_UCLIBC="$TOOLCHAIN_PATH"/gcc/arm-cvitek-linux-uclibcgnueabihf
  CROSS_COMPILE_PATH_GLIBC_RISCV64="$TOOLCHAIN_PATH"/gcc/riscv64-linux-x86_64
  CROSS_COMPILE_PATH_MUSL_RISCV64="$TOOLCHAIN_PATH"/gcc/riscv64-linux-musl-x86_64

if [[ "$SDK_VER" == "uclibc" ]]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_UCLIBC"
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-uclibc-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm/usr
    export PATH=$PATH:"$CROSS_COMPILE_PATH_UCLIBC"/bin
elif [[ "$SDK_VER" == "32bit" ]]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_32"
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-gnueabihf-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm/usr
    export PATH=$PATH:"$CROSS_COMPILE_PATH_32"/bin
elif [[ "$SDK_VER" == "64bit" ]]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_64"
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-aarch64-linux.cmake
    SYSTEM_PROCESSOR=ARM64
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm64/usr
    export PATH=$PATH:"$CROSS_COMPILE_PATH_64"/bin
elif [[ "$SDK_VER" == "glibc_riscv64" ]]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_GLIBC_RISCV64"
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-riscv64-linux.cmake
    SYSTEM_PROCESSOR=RISCV
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
    export PATH=$PATH:"$CROSS_COMPILE_PATH_GLIBC_RISCV64"/bin
elif [[ "$SDK_VER" == "musl_riscv64" ]]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_MUSL_RISCV64"
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
    TOOLCHAIN_FILE=$CVI_TDL_ROOT/toolchain/toolchain-riscv64-musl.cmake
    SYSTEM_PROCESSOR=RISCV
    export PATH=$PATH:"$CROSS_COMPILE_PATH_MUSL_RISCV64"/bin
else
    echo "Wrong SDK_VER=$SDK_VER"
    exit 1
fi

  # Check ccache is enable or not
  pathremove "$BUILD_PATH"/output/bin
  rm -rf "$BUILD_PATH"/output/bin/
  if [ "$USE_CCACHE" == "y" ];then
    if command -v ccache &> /dev/null;then
      mkdir -p "$BUILD_PATH"/output/bin
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-c++
      pathappend "$BUILD_PATH"/output/bin
    else
      echo "You have enabled ccache but there is no ccache in your PATH. Please cheack!"
      USE_CCACHE="n"
    fi
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
    CHIP_ARCH=CV186X
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
        -DBUILD_DOWNLOAD_DIR=$BUILD_DOWNLOAD_DIR \
        -DREPO_USER=$REPO_USER \
        -DCONFIG_DUAL_OS=$CONFIG_DUAL_OS

ninja -j8 || exit 1
ninja install || exit 1
popd

echo "trying to build sample in released folder."
cp -rf $CVI_TDL_ROOT/scripts/compile_sample.sh ${AI_SDK_INSTALL_PATH}/sample
pushd "${AI_SDK_INSTALL_PATH}/sample"
    KERNEL_ROOT=$KERNEL_ROOT\
    MW_PATH=$MW_PATH\
    TPU_PATH=$TPU_SDK_INSTALL_PATH\
    IVE_PATH=$IVE_SDK_INSTALL_PATH\
    USE_TPU_IVE=$USE_TPU_IVE\
    CHIP=$CHIP_ARCH\
    SDK_VER=$SDK_VER\
    source compile_sample.sh || exit 1
popd

rm -rf ${AI_SDK_INSTALL_PATH}/sample/tmp_install