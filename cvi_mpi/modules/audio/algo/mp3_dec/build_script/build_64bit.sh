#!/bin/sh

echo "start export------------------------------------"

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
CROSS_COMPILE=$TOOLCHAIN_PATH/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-glibc-linaro-2.23-2017.05-aarch64-linux-gnu/

export CFLGAS="-Wall -g  -fthread-jumps -fcse-follow-jumps -fcse-skip-blocks -fexpensive-optimizations -fregmove -fschedule-insns2 -fstrength-reduce -std=gnu11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,-gc-sections"
export CXXFLAGS="-std=gnu++11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,-gc-sections"
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
$DIR/../libmad64bit/configure --prefix=$DIR/../libmad64bit/install --enable-static --enable-shared --host=arm-linux-gnueabihf --with-gnu-ld=arm-linux-gnueabihf-ld  --build=arm CFLAGS=$CFLAGS

echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af $DIR/../libmad64bit/install/* $DIR/../libmad_output/64bit/
cd ..
rm -rf build

echo "script finished!!!"
