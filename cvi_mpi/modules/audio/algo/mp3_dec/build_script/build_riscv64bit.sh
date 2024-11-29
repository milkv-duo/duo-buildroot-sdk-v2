#!/bin/sh

echo "start export------------------------------------"

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
export CROSS_COMPILE="$TOOLCHAIN_PATH"/gcc/riscv64-linux-x86_64/bin/riscv64-unknown-linux-gnu-
export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-riscv64

export CFLGAS="-Wall -g  -fthread-jumps -fcse-follow-jumps -fcse-skip-blocks -fexpensive-optimizations -fregmove -fschedule-insns2 -fstrength-reduce -std=gnu11 -g -Wall -Wextra -fPIC -Os -ffunction-sections -fdata-sections -Wl,--gc-sections"
export CXXFLAGS="-std=gnu++11 -g -Wall -Wextra -fPIC -Os -ffunction-sections -fdata-sections -Wl,--gc-sections"
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
$DIR/../libmadriscv64bit/configure --prefix=$DIR/../libmadriscv64bit/install --enable-static --enable-shared --host=riscv64-unknown-linux --with-gnu-ld=riscv64-unknown-linux-gnu-ld --build=x86

echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af $DIR/../libmadriscv64bit/install/* $DIR/../libmad_output/riscv64/
cd ..
rm -rf build

echo "script finished!!!"
