#!/bin/sh

echo "start export------------------------------------"

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
export CROSS_COMPILE_PATH_32="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

export CROSS_COMPILE="$CROSS_COMPILE_PATH_32"

export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-glibc-linaro-2.23-2017.05-arm-linux-gnueabihf

export CFLAGS="-std=gnu11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections"
export CXXFLAGS="-std=gnu++11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections"
export LDFLAGS="-L$SYSROOT/usr/lib --gc-sections"
export CC="${CROSS_COMPILE}gcc --sysroot=${SYSROOT}"
export CXX="${CROSS_COMPILE}g++ --sysroot=${SYSROOT}"
export AR="$CROSS_COMPILE"ar
export LD="$CROSS_COMPILE"ld

DIR=`pwd`
NPROC=`nproc`

echo "start configure ------------------------------"

mkdir build
cd build
../../lame-3.100/configure --prefix=$DIR/../lame-3.100/cvi_output_32bit --enable-static --enable-shared --host=arm-linux-gnueabihf


echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af ../../lame-3.100/cvi_output_32bit/* ../../lame_output/32bit/
cd ..
rm -rf build

echo "script finished!!!"
