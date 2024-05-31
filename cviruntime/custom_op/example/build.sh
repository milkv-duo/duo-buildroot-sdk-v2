#!/bin/bash
set -e

DIR="$( cd "$(dirname "$0")" ; pwd -P )"
INSTALL_PATH=$DIR/install
if [[ ! -e $INSTALL_PATH ]]; then
  mkdir $DIR/install
fi
if [ -z "$ARM_TOOLCHAIN_GCC_PATH" ]; then
  ARM_TOOLCHAIN_GCC_PATH=$TPU_BASE/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
fi
export PATH=$ARM_TOOLCHAIN_GCC_PATH/bin:$PATH
export TOOLCHAIN_FILE_PATH=$DIR/cmake/toolchain-aarch64-linux.cmake
export MLIR_INCLUDE=$TPU_BASE/cvitek_mlir/include
export CVIRUNTIME_INCLUDE=$MLIR_INCLUDE
export AARCH64_SYSROOT_PATH=$TPU_BASE/cvitek_sysroot

if [[ ! -e $DIR/build ]]; then
  mkdir $DIR/build
fi
pushd $DIR/build
rm -rf *
cmake -DMLIR_INCLUDE=$MLIR_INCLUDE \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH ..
make install
rm -rf *
cmake -DCVIRUNTIME_INCLUDE=$CVIRUNTIME_INCLUDE \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH ../runtime
make install
rm -rf *
cmake -DCMAKE_SYSROOT=$AARCH64_SYSROOT_PATH \
      -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
      -DCVIRUNTIME_INCLUDE=$CVIRUNTIME_INCLUDE \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH ../runtime
make install
popd