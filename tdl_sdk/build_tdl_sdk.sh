#!/bin/bash

# print usage
print_usage() {
    echo "Usage: source ${BASH_SOURCE[0]} [option] [type]"
    echo "Options:"
    echo "  (no option) Build modules only"
    echo "  sample      Build samples only"
    echo "  all         Build both modules and sample"
}

# Check parameter
if [ "$#" -gt 2 ]; then
    echo "Error: Too many arguments"
    print_usage
    return 1
fi

# get tdl_sdk root dir
CVI_TDL_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TMP_WORKING_DIR="$CVI_TDL_ROOT"/tmp
BUILD_WORKING_DIR="$TMP_WORKING_DIR"/build_sdk
BUILD_DOWNLOAD_DIR="$TMP_WORKING_DIR"/_deps
TDL_SDK_INSTALL_PATH="$CVI_TDL_ROOT"/install

# Set build opetion and type
BUILD_OPTION=
BUILD_TYPE=SDKRelease
if [ "$#" -eq 0 ]; then
    echo "Using ./${BASH_SOURCE[0]}"
    echo "Compiling modules..."
elif [ "$1" = "sample" ]; then
    echo "Using ./${BASH_SOURCE[0]} sample"
    echo "Compiling sample..."
    BUILD_OPTION=sample
elif [ "$1" = "all" ]; then
    echo "Using ./${BASH_SOURCE[0]} all"
    echo "Compiling modules and sample..."
    BUILD_OPTION=all
elif [ "$1" = "clean" ]; then
    echo "Using ./${BASH_SOURCE[0]} clean"
    echo "Cleaning build..."
    if [ -d "${BUILD_DOWNLOAD_DIR}" ]; then
        echo "Cleanup tmp folder."
        rm -rf $BUILD_DOWNLOAD_DIR
    fi
    if [ -d "${TDL_SDK_INSTALL_PATH}" ]; then
        echo "Cleanup install folder."
        rm -rf $TDL_SDK_INSTALL_PATH
    fi
    exit 0
else
    echo "Error: Invalid option"
    print_usage
    exit 1
fi

# check system type
CONFIG_DUAL_OS=OFF
if [ -n "$ALIOS_PATH" ]; then
    CONFIG_DUAL_OS=ON
fi


# set host-tool
HOST_TOOL_PATH="$CROSS_COMPILE_PATH"
TARGET_MACHINE="$(${CROSS_COMPILE}gcc -dumpmachine)"
TOOLCHAIN_FILE="$CVI_TDL_ROOT"/toolchain/"$TARGET_MACHINE".cmake

# set dependency
MW_VER=v2
MPI_PATH="$TOP_DIR"/cvi_mpi
TPU_SDK_INSTALL_PATH="$OUTPUT_DIR"/tpu_"$SDK_VER"/cvitek_tpu_sdk
IVE_SDK_INSTALL_PATH="$OUTPUT_DIR"/tpu_"$SDK_VER"/cvitek_ive_sdk

# init build working dir
if [ -d "${BUILD_WORKING_DIR}" ]; then
    echo "Cleanup tmp folder."
    rm -rf $BUILD_WORKING_DIR
fi
echo "Creating tmp working directory."
mkdir -p $BUILD_WORKING_DIR

# into tmp/build_sdk
pushd $BUILD_WORKING_DIR

# Check cmake version
CMAKE_VERSION="$(cmake --version | grep 'cmake version' | sed 's/cmake version //g')"
CMAKE_REQUIRED_VERSION="3.18.4"
CMAKE_TAR="cmake-3.18.4-Linux-x86_64.tar.gz"
CMAKE_DOWNLOAD_URL="https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz"
echo "Checking cmake..."
if [ "$(printf '%s\n' "$CMAKE_REQUIRED_VERSION" "$CMAKE_VERSION" | sort -V | head -n1)" = "$CMAKE_REQUIRED_VERSION" ]; then
    CMAKE_BIN=$(command -v cmake)
else
    echo "Cmake version need ${CMAKE_REQUIRED_VERSION}, trying to download from ftp."
    if [ ! -f "$CMAKE_TAR" ]; then
        wget "$CMAKE_DOWNLOAD_URL"
    fi
    tar -zxf $CMAKE_TAR
    CMAKE_BIN=$PWD/cmake-3.18.4-Linux-x86_64/bin/cmake
fi

# check if use TPU_IVE
if [[ "$CHIP_ARCH" == "CV183X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV182X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV181X" ]]; then
    USE_TPU_IVE=OFF
elif [[ "$CHIP_ARCH" == "CV180X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "SOPHON" ]]; then
    MPI_PATH="$TOP_DIR"/middleware/"$MW_VER"
    USE_TPU_IVE=OFF
else
    echo "Unsupported chip architecture: ${CHIP_ARCH}"
    exit 1
fi

# build start
$CMAKE_BIN -G Ninja $CVI_TDL_ROOT -DCVI_PLATFORM=$CHIP_ARCH \
                                  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
                                  -DENABLE_CVI_TDL_CV_UTILS=ON \
                                  -DMLIR_SDK_ROOT=$TPU_SDK_INSTALL_PATH \
                                  -DMIDDLEWARE_SDK_ROOT=$MPI_PATH \
                                  -DTPU_IVE_SDK_ROOT=$IVE_SDK_INSTALL_PATH \
                                  -DCMAKE_INSTALL_PREFIX=$TDL_SDK_INSTALL_PATH \
                                  -DTOOLCHAIN_ROOT_DIR=$HOST_TOOL_PATH \
                                  -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
                                  -DKERNEL_ROOT=$KERNEL_ROOT \
                                  -DUSE_TPU_IVE=$USE_TPU_IVE \
                                  -DBUILD_DOWNLOAD_DIR=$BUILD_DOWNLOAD_DIR \
                                  -DCONFIG_DUAL_OS=$CONFIG_DUAL_OS \
                                  -DBUILD_OPTION=$BUILD_OPTION \
                                  -DTARGET_MACHINE=$TARGET_MACHINE \
                                  -DMW_VER=$MW_VER \

test $? -ne 0 && echo "cmake tdl_sdk failed !!" && popd && exit 1

ninja -j8 || exit 1
ninja install || exit 1
popd
# build end

