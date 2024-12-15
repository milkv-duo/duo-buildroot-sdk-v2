#!/bin/bash


#NDK路径
#export ANDROID_NDK_ROOT=/home/byhook/android/android-ndk-r10e
#
#export AOSP_API="android-21"
#export TOOLCHAIN_BASE="aarch64-linux-android"
#export TOOLNAME_BASE="aarch64-linux-android"
#export AOSP_ARCH="arch-arm64"
#export HOST="aarch64-linux"
#export AOSP_FLAGS="-funwind-tables -fexceptions -frtti"
##FF_EXTRA_CFLAGS=""
#export FF_CFLAGS="-O3 -Wall -pipe -ffast-math -fstrict-aliasing -Werror=strict-aliasing -Wno-psabi -Wa,--noexecstack "

#CFLAGS = -std=gnu11 -g -Wall -Wextra -Werror -fPIC -O3 -ffunction-sections -fdata-sections
#CXXFLAGS = -std=gnu++11 -g -Wall -Wextra -Werror -fPIC -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections

LIBS_DIR=$PWD/../lame-3.100/cvi_output_64bit
echo "output to LIBS_DIR="$LIBS_DIR

#cd lame-3.100

#PLATFORM=$ANDROID_NDK_ROOT/platforms/$AOSP_API/$AOSP_ARCH

export TOOLCHAIN_PATH=$PWD/../../../../../../../host-tools
CROSS_COMPILE=$TOOLCHAIN_PATH/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
export RAMDISK_PATH=$PWD/../../../../../../../ramdisk
SYSROOT=$RAMDISK_PATH/sysroot/sysroot-glibc-linaro-2.23-2017.05-aarch64-linux-gnu/

PREFIX=$LIBS_DIR/


FLAGS="--enable-static --enable-shared --host=aarch64-linux"

export CFLAGS="-std=gnu11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections"
export CXXFLAGS="-std=gnu++11 -g -Wall -Wextra -fPIC -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections"
export LDFLAGS="-L$SYSROOT/usr/lib --gc-sections"
export CC="${CROSS_COMPILE}gcc --sysroot=${SYSROOT}"
export CXX="${CROSS_COMPILE}g++ --sysroot=${SYSROOT}"
export AR="${CROSS_COMPILE}ar"
export LD="${CROSS_COMPILE}ld"
NPROC=`nproc`


mkdir build
cd build
../../lame-3.100/configure $FLAGS \
--prefix=$PREFIX

#$ADDITIONAL_CONFIGURE_FLAG

echo "configure  finished!!-------------------------"
make clean
make -j $NPROC
make install
cp -af ../../lame-3.100/cvi_output_64bit/* ../../lame_ouput/64bit/
cd ..
rm -rf build

