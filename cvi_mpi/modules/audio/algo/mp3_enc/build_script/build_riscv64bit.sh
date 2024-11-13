#!/bin/sh

echo "start export------------------------------------"

export PATH=$PATH:$PWD/../../../../../../../host-tools/gcc/riscv64-linux-x86_64/bin
echo $PATH

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
export CROSS_COMPILE_PATH_UCLIBC="$TOOLCHAIN_PATH"/gcc/riscv64-linux-x86_64/bin/riscv64-unknown-linux-gnu-

export CROSS_COMPILE="$CROSS_COMPILE_PATH_UCLIBC"

export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-riscv64

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
../../lame-3.100/configure --prefix=$DIR/../lame-3.100/cvi_output_riscv64 --enable-static --enable-shared --host=riscv64-unknown-linux


echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af ../../lame-3.100/cvi_output_riscv64/* ../../lame_output/riscv64
cd ..
rm -rf build

echo "script finished!!!"
