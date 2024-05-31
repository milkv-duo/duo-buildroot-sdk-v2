#!/bin/bash
set -xe

echo "TPU_SDK_PATH=$TPU_SDK_PATH"

if [ -z $TPU_SDK_PATH ]; then
   echo "please set TPU_SDK_PATH"
fi

mkdir -p build
cd build
cmake .. \
      -DCMAKE_BUILD_TYPE=RELEASE \
      -DCMAKE_C_FLAGS_RELEASE=-O3 \
      -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
      -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-aarch64-linux.cmake \
      -DTPU_SDK_PATH=$TPU_SDK_PATH \
      -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
      -DCMAKE_INSTALL_PREFIX=./
make install