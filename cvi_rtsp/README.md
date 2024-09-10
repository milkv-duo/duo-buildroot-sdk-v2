# cvi_rtsp

## Build ##
1. excute build.sh with auto download live555 prebuilt
   ex: CROSS_COMPILE=/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- ./build.sh

2. execute build.sh with self build live555
   ex: CROSS_COMPILE=/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- LIVE555_DIR="live555 dir" ./build.sh

3. if you want to build examples, please add example to SUBDIR in Makefile, then uncomment "example: src"
   run build script with MW_DIR parameter
   ex:
       CROSS_COMPILE=/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- LIVE555_DIR=$(pwd)/prebuilt MW_DIR=/middleware ./build.sh

## live555 prebuilt ##
1. download prebuit live555
   - ftp://10.58.65.3/sw_rls/cv1835/third_party/latest/live555.tar.gz
