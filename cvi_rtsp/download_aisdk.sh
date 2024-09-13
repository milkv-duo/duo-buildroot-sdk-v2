#!/bin/bash

SDK_VER="$1"
VER="$2"
DEST="$3"

usage() {
    echo "$0 SDK_VER [ 32bit | 64bit | uclibc ]  VER dest"
    exit 1
}

curl "ftp://10.58.65.3/prebuilt/easy_build/aisdk_lib/aisdk_lib_${VER}_${SDK_VER}.tar.gz" --output ${DEST}/aisdk_lib_${VER}_${SDK_VER}.tar.gz

