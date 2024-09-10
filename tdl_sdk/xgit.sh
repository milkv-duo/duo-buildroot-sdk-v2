#!/bin/bash

# 检查libsophon目录是否存在
if [ -d "../libsophon" ]; then
    # 定义仓库路径和名称的关联数组
    declare -A repos=(
        ["host-tools"]="host-tools"
        ["cvi_build"]="build"
        ["rom"]="rom"
        ["fsbl"]="fsbl"
        ["opensbi"]="opensbi"
        ["u-boot-2021.10"]="u-boot-2021.10"
        ["linux_5.10"]="linux_5.10"
        ["cvi_osdrv"]="osdrv"
        ["cvi_ramdisk"]="ramdisk"
        ["oss"]="oss"
        ["cvi_middleware"]="middleware"
        ["isp"]="middleware/v1/modules/isp"
        ["isp_tuning"]="isp_tuning"
        ["cvi_rtsp"]="cvi_rtsp"
        ["freertos"]="freertos"
        ["cvitek/mediapipe"]="mediapipe"
        ["sophon_media"]="sophon_media"
        ["libsophon"]="libsophon"
        ["libsophav"]="libsophon/libsophav"
        ["tpu-mlir"]="tpu-mlir"
        ["cviai"]="cviai"
        ["bootloader-arm64"]="ubuntu/bootloader-arm64"
    )
else
    # 定义仓库路径和名称的关联数组
    declare -A repos_with_libsophon=(
        ["host-tools"]="host-tools"
        ["build"]="build"
        ["fsbl"]="fsbl"
        ["opensbi"]="opensbi"
        ["release_bin_blp"]="rel_bin/release_bin_blp"
        ["release_bin_bldp"]="rel_bin/release_bin_bldp"
        ["release_bin_bld"]="rel_bin/release_bin_bld"
        ["release_bin_atf"]="rel_bin/release_bin_atf"
        ["release_bin_license"]="rel_bin/release_bin_license"
        ["u-boot"]="u-boot"
        ["u-boot-2021.10"]="u-boot-2021.10"
        ["linux-linaro-stable"]="linux-linaro-stable"
        ["linux"]="linux"
        ["linux_5.10"]="linux_5.10"
        ["osdrv"]="osdrv"
        ["ramdisk"]="ramdisk"
        ["oss"]="oss"
        ["middleware"]="middleware"
        ["isp"]="middleware/v1/modules/isp"
        ["isp_tuning"]="isp_tuning"
        ["ive"]="ive"
        ["ivs"]="ivs"
        ["cviai"]="cviai"
        ["cvi_pipeline"]="cvi_pipeline"
        ["cvi_rtsp"]="cvi_rtsp"
        ["cvibuilder"]="cvibuilder"
        ["cvikernel"]="cvikernel"
        ["cviruntime"]="cviruntime"
        ["cvimath"]="cvimath"
        ["flatbuffers"]="flatbuffers"
        ["cnpy"]="cnpy"
        ["freertos"]="freertos"
    )
fi


cd ..
# 遍历repos数组
for name in "${!repos[@]}"; do
    path="${repos[$name]}"
    echo "Processing $name at $path..."

    # 检查目录是否存在
    if [ ! -d "$path" ]; then
        echo "Directory '$path' not found, skipping..."
        continue
    fi

    # 进入项目目录
    pushd "$path"


    if 
    # 执行Git命令
    pwd
    git status
    git clean -dxf
    git stash push
    git reset --hard
    git checkout .
    git pull
    git status

    popd

    echo "--------------------------------------"
done

echo "All repositories processed."