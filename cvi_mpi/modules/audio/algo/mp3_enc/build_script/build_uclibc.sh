#!/bin/sh

echo "start export------------------------------------"

export PATH=$PATH:$PWD/../../../../../../../host-tools/gcc/arm-cvitek-linux-uclibcgnueabihf/bin
echo $PATH

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
export CROSS_COMPILE_PATH_UCLIBC="$TOOLCHAIN_PATH"/gcc/arm-cvitek-linux-uclibcgnueabihf/bin/arm-cvitek-linux-uclibcgnueabihf-

export CROSS_COMPILE="$CROSS_COMPILE_PATH_UCLIBC"

export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-uclibc

export CFLAGS="-std=gnu11 -g -Wall -Wextra -fPIC -Os -ffunction-sections -fdata-sections"
export CXXFLAGS="-std=gnu++11 -g -Wall -Wextra -fPIC -Os -ffunction-sections -fdata-sections -Wl,-gc-sections"
export LDFLAGS="-L$SYSROOT/usr/lib -Wl,--gc-sections"
export CC="${CROSS_COMPILE}gcc --sysroot=${SYSROOT}"
export CXX="${CROSS_COMPILE}g++ --sysroot=${SYSROOT}"
export AR="$CROSS_COMPILE"ar
export LD="$CROSS_COMPILE"ld

DIR=`pwd`
NPROC=`nproc`

echo "start configure ------------------------------"

mkdir build
cd build
../../lame-3.100/configure --prefix=$DIR/../lame-3.100/cvi_output_uclibc --enable-static --enable-shared --host=arm-cvitek-linux-uclibcgnueabihf


echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af ../../lame-3.100/cvi_output_uclibc/* ../../lame_output/uclibc
cd ..
rm -rf build

echo "script finished!!!"
