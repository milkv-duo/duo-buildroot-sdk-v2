#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IVE_ROOT=$(readlink -f $SCRIPT_DIR/../)
TMP_WORKING_DIR=$IVE_ROOT/tmp

echo "Creating tmp working directory."

mkdir -p $TMP_WORKING_DIR/build_sdk
pushd $TMP_WORKING_DIR/build_sdk

if [[ "$SDK_VER" == "uclibc" ]]; then
    TOOLCHAIN_FILE=$IVE_ROOT/toolchain/toolchain-uclibc-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/arm/usr
elif [[ "$SDK_VER" == "32bit" ]]; then
    TOOLCHAIN_FILE=$IVE_ROOT/toolchain/toolchain-gnueabihf-linux.cmake
    SYSTEM_PROCESSOR=ARM
    KERNEL_ROOT="${IVE_ROOT}"/build/"${PROJECT_FULLNAME}"/arm/usr
elif [[ "$SDK_VER" == "64bit" ]]; then
    TOOLCHAIN_FILE=$IVE_ROOT/toolchain/toolchain-aarch64-linux.cmake
    SYSTEM_PROCESSOR=ARM64
    KERNEL_ROOT="${IVE_ROOT}"/build/"${PROJECT_FULLNAME}"/arm64/usr
elif [[ "$SDK_VER" == "glibc_riscv64" ]]; then
    TOOLCHAIN_FILE=$IVE_ROOT/toolchain/toolchain-riscv64-linux.cmake
    SYSTEM_PROCESSOR=RISCV
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
elif [[ "$SDK_VER" == "musl_riscv64" ]]; then
    KERNEL_ROOT="${KERNEL_PATH}"/build/"${PROJECT_FULLNAME}"/riscv/usr/
    TOOLCHAIN_FILE=$IVE_ROOT/toolchain/toolchain-riscv64-musl.cmake
    SYSTEM_PROCESSOR=RISCV
else
    echo "Wrong SDK_VER=$SDK_VER"
    exit 1
fi

CURRENT_USER="$(git config user.name)"
if [[ "${CURRENT_USER}" != "sw_jenkins" ]]; then
    REPO_USER="$(git config user.name)@"
fi

echo "repo user : $REPO_USER"
echo "CHIP_ARCH:" $CHIP_ARCH
echo "TOOLCHAIN:" $TOOLCHAIN_FILE
echo "HOST_TOOL_PATH:" $HOST_TOOL_PATH
cmake -G Ninja $IVE_ROOT -DCVI_PLATFORM=$CHIP_ARCH \
                         -DCMAKE_BUILD_TYPE=SDKRelease \
                         -DKERNEL_HEADERS_ROOT=$KERNEL_HEADER_PATH \
                         -DMLIR_SDK_ROOT=$TPU_SDK_INSTALL_PATH \
                         -DMIDDLEWARE_SDK_ROOT=$MW_PATH \
                         -DCMAKE_INSTALL_PREFIX=$IVE_SDK_INSTALL_PATH \
                         -DTOOLCHAIN_ROOT_DIR=$HOST_TOOL_PATH \
                         -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
                         -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
                         -DREPO_USER=$REPO_USER \
                         -DKERNEL_ROOT=$KERNEL_ROOT

ninja -j8 || exit 1
ninja install || exit 1
popd

echo "Cleanup tmp folder."
rm -r $TMP_WORKING_DIR
