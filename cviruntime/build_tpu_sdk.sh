#!/bin/bash

DIR="$( cd "$(dirname "$0")" ; pwd -P )"

echo "TPU_SDK_BUILD_PATH=$TPU_SDK_BUILD_PATH"
echo "TPU_SDK_INSTALL_PATH=$TPU_SDK_INSTALL_PATH"
echo "TOP_DIR=$TOP_DIR"

TOOLCHAIN_FILE_PATH=$DIR/scripts/toolchain.cmake
echo "TOOLCHAIN_FILE_PATH=$TOOLCHAIN_FILE_PATH"
TOOLCHAIN_AARCH64=$DIR/scripts/toolchain-aarch64-linux.cmake
TOOLCHAIN_ARM=$DIR/scripts/toolchain-linux-gnueabihf.cmake
TOOLCHAIN_UCLIBC=$DIR/scripts/toolchain-linux-uclibc.cmake
TOOLCHAIN_RISCV64=$DIR/scripts/toolchain-riscv64-linux-x86_64.cmake
TOOLCHAIN_RISCV64_MUSL=$DIR/scripts/toolchain-riscv64-linux-musl-x86_64.cmake

if [ ! -e "$OSS_TARBALL_PATH" ]; then
    echo "${OSS_TARBALL_PATH} not present, run build_3rd_party first"
    exit 1
fi

mkdir -p "$TPU_SDK_BUILD_PATH"/build_sdk
mkdir -p "$TPU_SDK_INSTALL_PATH"

"$OSS_PATH"/run_build.sh -n zlib -e -t "$OSS_TARBALL_PATH" -i "$TPU_SDK_INSTALL_PATH"
"$OSS_PATH"/run_build.sh -n flatbuffers -e -t "$OSS_TARBALL_PATH" -i "$TPU_SDK_INSTALL_PATH"/flatbuffers
"$OSS_PATH"/run_build.sh -n opencv -e -t "$OSS_TARBALL_PATH" -i "$TPU_SDK_INSTALL_PATH"/opencv

#
# build
#
BUILD_TYPE="RELEASE"
if [ "$BUILD_TYPE" == "RELEASE" ]; then
  BUILD_FLAG="-DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3"
else
  BUILD_FLAG="-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=-ggdb"
fi
BUILD_PATH=$TPU_SDK_BUILD_PATH

CHIP_ID="${CHIP_ARCH,,}"
echo "CHIP_ID=$CHIP_ID"

# build host flatbuffers
FLATBUFFERS_HOST_PATH=$BUILD_PATH/install_flatbuffers_host
mkdir -p $FLATBUFFERS_HOST_PATH
if [ ! -e $BUILD_PATH/build_flatbuffers_host ]; then
  mkdir -p $BUILD_PATH/build_flatbuffers_host
fi
pushd $BUILD_PATH/build_flatbuffers_host
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=$FLATBUFFERS_HOST_PATH \
    $TOP_DIR/flatbuffers
cmake --build . --target install
test $? -ne 0 && echo "build flatbuffers failed !!" && popd && exit 1
popd

# build target flat buffer
# move to build_oss

# generate target-independent flatbuffer schema
CVIMODEL_HOST_PATH=$BUILD_PATH/install_cvimodel_host
if [ ! -e $BUILD_PATH/build_cvimodel ]; then
  mkdir -p $BUILD_PATH/build_cvimodel
fi
pushd $BUILD_PATH/build_cvimodel
cmake -G Ninja -DFLATBUFFERS_PATH=$FLATBUFFERS_HOST_PATH \
    -DCMAKE_INSTALL_PREFIX=$CVIMODEL_HOST_PATH \
    $TOP_DIR/cvibuilder
cmake --build . --target install
test $? -ne 0 && echo "build cvibuilder failed !!" && popd && exit 1
popd

# build cvikernel
if [ ! -e $BUILD_PATH/build_cvikernel ]; then
  mkdir -p $BUILD_PATH/build_cvikernel
fi
pushd $BUILD_PATH/build_cvikernel
cmake -G Ninja $BUILD_FLAG \
    -DCHIP=$CHIP_ID \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
    -DCMAKE_INSTALL_PREFIX=$TPU_SDK_INSTALL_PATH \
    $TOP_DIR/cvikernel
cmake --build . --target install -- -v
test $? -ne 0 && echo "build cvikernel failed !!" && popd && exit 1
popd

# build cnpy
if [ ! -e $BUILD_PATH/build_cnpy ]; then
  mkdir -p $BUILD_PATH/build_cnpy
fi
pushd $BUILD_PATH/build_cnpy
cmake -G Ninja $BUILD_FLAG \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
    -DCMAKE_INSTALL_PREFIX=$TPU_SDK_INSTALL_PATH \
    $TOP_DIR/cnpy
cmake --build . --target install
test $? -ne 0 && echo "build cnpy failed !!" && popd && exit 1
popd

# build runtime

if [ ! -e $BUILD_PATH/build_cviruntime ]; then
  mkdir $BUILD_PATH/build_cviruntime
fi
pushd $BUILD_PATH/build_cviruntime
cmake -G Ninja -DCHIP=$CHIP_ID -DRUNTIME=SOC $BUILD_FLAG \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
    -DCVIKERNEL_PATH=$TPU_SDK_INSTALL_PATH \
    -DCNPY_PATH=$TPU_SDK_INSTALL_PATH/lib \
    -DFLATBUFFERS_PATH=$TPU_SDK_INSTALL_PATH/flatbuffers \
    -DCVIBUILDER_PATH=$CVIMODEL_HOST_PATH \
    -DCMAKE_INSTALL_PREFIX=$TPU_SDK_INSTALL_PATH \
    -DENABLE_TEST=OFF \
    $TOP_DIR/cviruntime
cmake --build . --target install -- -v
test $? -ne 0 && echo "build cviruntime failed !!" && popd && exit 1
popd

# build cvimath
if [ ! -e $BUILD_PATH/build_cvimath ]; then
  mkdir $BUILD_PATH/build_cvimath
fi
pushd $BUILD_PATH/build_cvimath

cmake -G Ninja  \
    -DTOOLCHAIN_ROOT_DIR=$TOOLCHAIN_GCC_PATH \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
    -DTPU_SDK_ROOT=$TPU_SDK_INSTALL_PATH \
    -DCMAKE_INSTALL_PREFIX=$TPU_SDK_INSTALL_PATH \
    $TOP_DIR/cvimath
cmake --build . --target install -- -v
test $? -ne 0 && echo "build cvimath failed !!" && popd && exit 1
popd

if [ ! -e $BUILD_PATH/build_samples ]; then
  mkdir $BUILD_PATH/build_samples
fi
pushd $BUILD_PATH/build_samples
cmake -G Ninja $BUILD_FLAG \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE_PATH \
    -DTPU_SDK_PATH=$TPU_SDK_INSTALL_PATH \
    -DOPENCV_PATH=$TPU_SDK_INSTALL_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=$TPU_SDK_INSTALL_PATH/samples \
    $DIR/samples
cmake --build . --target install -- -v
test $? -ne 0 && echo "build samples failed !!" && popd && exit 1
popd

# Copy some files for release build
mkdir -p $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_FILE_PATH $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_AARCH64 $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_ARM $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_UCLIBC $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_RISCV64 $TPU_SDK_INSTALL_PATH/cmake
cp $TOOLCHAIN_RISCV64_MUSL $TPU_SDK_INSTALL_PATH/cmake

# copy lib
mkdir -p "$SYSTEM_OUT_DIR"/lib/
cp -a "$TPU_SDK_INSTALL_PATH"/lib/*.so* "$SYSTEM_OUT_DIR"/lib/
cp -a "$TPU_SDK_INSTALL_PATH"/opencv/lib/*.so* "$SYSTEM_OUT_DIR"/lib/
