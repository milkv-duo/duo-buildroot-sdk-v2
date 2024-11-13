#!/bin/sh

echo "start export------------------------------------"

export TOOLCHAIN_PATH=$PWD/../../../../../../../../host-tools
export CROSS_COMPILE="$TOOLCHAIN_PATH"/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-
export RAMDISK_PATH=$PWD/../../../../../../../../ramdisk
SYSROOT=${TOOLCHAIN_PATH}/gcc/riscv64-linux-musl-x86_64/sysroot/

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
mkdir -p ../libmad_output/musl64
cd build
$DIR/../libmadmusl64/configure --prefix=$DIR/../libmadmusl64/install --enable-static --enable-shared --host=riscv64-unknown-linux --with-gnu-ld=riscv64-unknown-linux-gnu-ld --build=x86

echo "start make------------------------------------"
make clean
make -j $NPROC
make install
cp -af $DIR/../libmadmusl64/install/* $DIR/../libmad_output/musl64/
make -C $DIR/../cvi_interface/
cd ..
rm -rf build
rm -rf $DIR/../libmad_output/musl64/include/mad.h
chmod 664 $DIR/../libmad_output/musl64/lib/*
cp -af $DIR/../libmad_output/musl64/* $DIR/../../../../prebuilt/musl_riscv64/libmad
rm -rf $DIR/../libmad_output
echo "script finished!!!"
