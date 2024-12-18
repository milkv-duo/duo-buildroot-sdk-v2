#!/bin/bash
function _build_default_env()
{
  # Please keep these default value!!!
  BRAND=${BRAND:-cvitek}
  DEBUG=${DEBUG:-0}
  RELEASE_VERSION=${RELEASE_VERSION:-0}
  BUILD_VERBOSE=${BUILD_VERBOSE:-1}
  ATF_BL32=${ATF_BL32:-1}
  UBOOT_VBOOT=${UBOOT_VBOOT:-0}
  COMPRESSOR=${COMPRESSOR:-xz}
  COMPRESSOR_UBOOT=${COMPRESSOR_UBOOT:-lzma} # or none to disable
  MULTI_PROCESS_SUPPORT=${MULTI_PROCESS_SUPPORT:-0}
  ENABLE_BOOTLOGO=${ENABLE_BOOTLOGO:-0}
  TPU_REL=${TPU_REL:-0} # TPU release build
  SENSOR=${SENSOR:-sony_imx327}
}

function gettop()
{
  local TOPFILE=build/envsetup_soc.sh
  if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
    # The following circumlocution ensures we remove symlinks from TOP.
    (cd "$TOP"; PWD= /bin/pwd)
  else
    if [ -f $TOPFILE ] ; then
      # The following circumlocution (repeated below as well) ensures
      # that we record the true directory name and not one that is
      # faked up with symlink names.
      PWD= /bin/pwd
    else
      local HERE=$PWD
      T=
      while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
        \cd ..
        T=$(PWD= /bin/pwd -P)
      done
      \cd "$HERE"
      if [ -f "$T/$TOPFILE" ]; then
        echo "$T"
      fi
    fi
  fi
}

function _build_fsbl_env()
{
  export FSBL_PATH
}

function build_fsbl()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  _build_opensbi_env
  cd "$BUILD_PATH" || return
  make fsbl-build || return "$?"
)}

function clean_fsbl()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  cd "$BUILD_PATH" || return
  make fsbl-clean
)}

function _build_atf_env()
{
  export ATF_BL32 FAKE_BL31_32
}

function build_atf()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_atf_env
  cd "$BUILD_PATH" || return
  make arm-trusted-firmware || return "$?"
)}

function clean_atf()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_atf_env
  cd "$BUILD_PATH" || return
  make arm-trusted-firmware-clean
)}

function _build_uboot_env()
{
  _build_atf_env
  _build_fsbl_env
  export UBOOT_OUTPUT_FOLDER IMGTOOL_PATH FLASH_PARTITION_XML FIP_BIN_PATH
  export UBOOT_VBOOT RELEASE_VERSION ENABLE_BOOTLOGO STORAGE_TYPE COMPRESSOR_UBOOT
  export PANEL_TUNING_PARAM PANEL_LANE_NUM_TUNING_PARAM PANEL_LANE_SWAP_TUNING_PARAM
}

function build_fip_pre()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  cd "$BUILD_PATH" || return
  make fip-pre-merge || return "$?"
)}

function build_rtos()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$BUILD_PATH" || return
  make rtos || return "$?"
)}

function clean_rtos()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$BUILD_PATH" || return
  make rtos-clean
)}

function menuconfig_uboot()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  cd "$BUILD_PATH" || return
  make u-boot-menuconfig || return "$?"
)}

function _link_uboot_logo()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$BUILD_PATH" || return
  if [[ x"${PANEL_TUNING_PARAM}" = x"I80_panel_st7789v" ]]; then
    ln -sf "$COMMON_TOOLS_PATH"/bootlogo/logo_320x240.BMP "$COMMON_TOOLS_PATH"/bootlogo/logo.jpg
  fi
)}

function build_uboot()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  _build_opensbi_env
  _link_uboot_logo
  cd "$BUILD_PATH" || return
  make u-boot || return "$?"
)}

function build_uboot_env_tools()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  cd "$BUILD_PATH" || return
  make u-boot-env-tools || return "$?"
)}

function clean_uboot()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_uboot_env
  cd "$BUILD_PATH" || return
  make u-boot-clean
)}

function _build_kernel_env()
{
  export KERNEL_OUTPUT_FOLDER RAMDISK_OUTPUT_FOLDER SYSTEM_OUT_DIR
}

function menuconfig_kernel()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_kernel_env
  cd "$BUILD_PATH" || return
  make kernel-menuconfig || return "$?"
)}

function setconfig_kernel()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_kernel_env
  cd "$BUILD_PATH" || return
  make kernel-setconfig "SCRIPT_ARG=$1" || return "$?"
)}

# shellcheck disable=SC2120
function build_kernel()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_kernel_env
  cd "$BUILD_PATH" || return
  make kernel || return "$?"

  # generate boot.itb image.
  if [[ ${1} != noitb ]]; then
    pack_boot || return "$?"
  fi
)}

function clean_kernel()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_kernel_env
  cd "$BUILD_PATH" || return
  make kernel-clean
)}

function build_bld()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$BUILD_PATH" || return
  make bld || return "$?"
)}

function clean_bld()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$BUILD_PATH" || return
  make bld-clean
)}

function _build_middleware_env()
{
  export MULTI_PROCESS_SUPPORT
}

function build_middleware()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_middleware_env
  cd "$BUILD_PATH" || return

  make "$ROOTFS_DIR" || return "$?"

  pushd "$MW_PATH"
  make all -j$(nproc)
  test $? -ne 0 && print_notice "build middleware failed !!" && popd && return 1
  make install DESTDIR="$SYSTEM_OUT_DIR" || return "$?"
  popd

  # add sdk version
  echo "SDK_VERSION=${SDK_VER}" > "$SYSTEM_OUT_DIR"/sdk-release
)}

function clean_middleware()
{
  pushd "$MW_PATH"
  make clean
  make uninstall
  popd
}

function clean_middleware_all()
{
  pushd "$MW_PATH"
  make clean_all
  popd
}

function _build_tpu_sdk_env()
{
  export SYSTEM_OUT_DIR OSS_TARBALL_PATH OSS_PATH
}

function build_tpu_sdk()
{(
  print_notice "Run ${FUNCNAME[0]}() function"

  _build_tpu_sdk_env
  # build tpu
  TPU_SDK_BUILD_PATH="$TPU_SDK_PATH"/build_sdk \
  TPU_SDK_INSTALL_PATH="$TPU_SDK_INSTALL_PATH" \
  "$TPU_SDK_PATH"/build_tpu_sdk.sh

  test "$?" -eq 0 || return 1
)}

function clean_tpu_sdk()
{
  rm -rf "$TPU_SDK_INSTALL_PATH"
  rm -rf "$TPU_SDK_PATH"/build_sdk

  rm -f "$SYSTEM_OUT_DIR"/lib/libcnpy.so*
  rm -f "$SYSTEM_OUT_DIR"/lib/libcvikernel.so*
  rm -f "$SYSTEM_OUT_DIR"/lib/libcviruntime.so*
  rm -f "$SYSTEM_OUT_DIR"/lib/libopencv_*
}

function build_sdk()
{
  if [[ -z $1 ]]; then
    echo "Please enter sdk type !"
    return 1;
  fi

  print_notice "Run ${FUNCNAME[0]}() $1 function"

  if [ ! -e "$TPU_SDK_INSTALL_PATH" ]; then
    echo "$TPU_SDK_INSTALL_PATH not present, run build_tpu_sdk first"
    return 1
  fi

  if [ "$SDK_VER" = 64bit ]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_64"
  elif [ "$SDK_VER" = 32bit ]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_32"
  elif [ "$SDK_VER" = uclibc ]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_UCLIBC"
  elif [ "$SDK_VER" = glibc_riscv64 ]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_GLIBC_RISCV64"
  elif [ "$SDK_VER" = musl_riscv64 ]; then
    HOST_TOOL_PATH="$CROSS_COMPILE_PATH_MUSL_RISCV64"
  else
    echo "Unknown SDK_VER=$SDK_VER"
    return 1
  fi
  local SDK_PATH=
  local SDK_INSTALL_PATH=
  if [[ "$1" = ive ]]; then
    SDK_PATH="$IVE_SDK_PATH"
    SDK_INSTALL_PATH="$IVE_SDK_INSTALL_PATH"
  elif [[ "$1" = ivs ]]; then
    SDK_PATH="$IVS_SDK_PATH"
    SDK_INSTALL_PATH="$IVS_SDK_INSTALL_PATH"
  elif [[ "$1" = cnv ]]; then
    SDK_PATH="$CNV_SDK_PATH"
    SDK_INSTALL_PATH="$CNV_SDK_INSTALL_PATH"
  fi
  pushd "$SDK_PATH"
  HOST_TOOL_PATH="$HOST_TOOL_PATH" \
  MW_PATH="$MW_PATH" \
  CHIP_ARCH="$CHIP_ARCH" \
  OPENCV_INSTALL_PATH="$TPU_SDK_INSTALL_PATH"/opencv \
  TRACER_INSTALL_PATH="$IVE_SDK_INSTALL_PATH" \
  TPU_SDK_INSTALL_PATH="$TPU_SDK_INSTALL_PATH" \
  IVE_SDK_INSTALL_PATH="$IVE_SDK_INSTALL_PATH" \
  IVS_SDK_INSTALL_PATH="$IVS_SDK_INSTALL_PATH" \
  CNV_SDK_INSTALL_PATH="$CNV_SDK_INSTALL_PATH" \
  KERNEL_HEADER_PATH="$KERNEL_PATH"/"$KERNEL_OUTPUT_FOLDER"/usr/ \
      scripts/sdk_release.sh
  test "$?" -ne 0 && print_notice "${FUNCNAME[0]}() failed !!" && popd && return 1
  popd

  # copy so
  cp -a "$SDK_INSTALL_PATH"/lib/*.so* "$SYSTEM_OUT_DIR"/lib/
}

function clean_sdk()
{
  [[ "$1" = ive ]] && rm -rf "$IVE_SDK_INSTALL_PATH"
  [[ "$1" = ivs ]] && rm -rf "$IVS_SDK_INSTALL_PATH"
  [[ "$1" = cnv ]] && rm -rf "$CNV_SDK_INSTALL_PATH"
  rm -f "$SYSTEM_OUT_DIR"/lib/libcvi_"$1"_tpu.so*
  rm -rf "${SYSTEM_OUT_DIR:?}"/usr/bin/"$1"
}

function build_ive_sdk()
{
  if [[ "$CHIP_ARCH" != CV181X ]] ; then
    build_sdk ive || return "$?"
  fi
}

function clean_ive_sdk()
{
  if [[ "$CHIP_ARCH" != CV181X ]] ; then
    clean_sdk ive
  fi
}

function build_ivs_sdk()
{
  if [[ "$CHIP_ARCH" == CV182X ]] || [[ "$CHIP_ARCH" == CV183X ]]; then
    build_sdk ivs || return "$?"
  fi
}

function clean_ivs_sdk()
{
  if [[ "$CHIP_ARCH" == CV182X ]] || [[ "$CHIP_ARCH" == CV183X ]]; then
    clean_sdk ivs
  fi
}

function build_tdl_sdk()
{
  print_notice "Run ${FUNCNAME[0]}() function"
  pushd "$TDL_SDK_PATH"
  ./build_tdl_sdk.sh all
  test "$?" -ne 0 && print_notice "${FUNCNAME[0]}() failed !!" && popd && return 1
  popd
}

function clean_tdl_sdk()
{
  print_notice "Run ${FUNCNAME[0]}() function"
  pushd "$TDL_SDK_PATH"
  ./build_tdl_sdk.sh clean
  popd
}

function build_cnv_sdk()
{
  build_sdk cnv || return "$?"
}

function clean_cnv_sdk()
{
  clean_sdk cnv
}

function build_osdrv()
{(
  print_notice "Run ${FUNCNAME[0]}() ${1} function"

  cd "$BUILD_PATH" || return
  make "$ROOTFS_DIR" || return "$?"

  local osdrv_target="$1"
  if [ -z "$osdrv_target" ]; then
    osdrv_target=all
  fi

  pushd "$OSDRV_PATH"
  make KERNEL_DIR="$KERNEL_PATH"/"$KERNEL_OUTPUT_FOLDER" INSTALL_DIR="$SYSTEM_OUT_DIR"/ko "$osdrv_target" || return "$?"
  popd
)}

function clean_osdrv()
{
  print_notice "Run ${FUNCNAME[0]}() function"

  pushd "$OSDRV_PATH"
  make KERNEL_DIR="$KERNEL_PATH"/"$KERNEL_OUTPUT_FOLDER" INSTALL_DIR="$SYSTEM_OUT_DIR"/ko clean || return "$?"
  popd
}

function _build_cvi_pipeline_env()
{
  export SYSTEM_OUT_DIR CROSS_COMPILE_PATH_32 CROSS_COMPILE_PATH_64 CROSS_COMPILE_PATH_UCLIBC
}

function build_cvi_pipeline()
{
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_cvi_pipeline_env

  pushd "$CVI_PIPELINE_PATH"
  ./build.sh "1"
  ./install_base_pkg.sh prebuilt "$(pwd)/install"
  ./download_models.sh "$(pwd)/install/cvi_models"
  make install DESTDIR="$(pwd)/install/system" LIBC_PATH="$TOOLCHAIN_PATH/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/lib/"
  test "$?" -eq 0 || return 1
  popd
}

function clean_cvi_pipeline()
{
  pushd "$CVI_PIPELINE_PATH"
  make clean
  rm -rf cvi_pipeline.tar.gz
  rm -rf prebuilt/*
  popd
}

function _build_cvi_rtsp_env()
{
  export CROSS_COMPILE
}

function build_cvi_rtsp()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  _build_cvi_rtsp_env

  cd "$CVI_RTSP_PATH" || return
  BUILD_SERVICE=1 MW_DIR=${MW_PATH} ./build.sh
  BUILD_SERVICE=1 make install DESTDIR="$(pwd)/install" || return "$?"
  make package DESTDIR="$(pwd)/install" || return "$?"

  if [[ "$FLASH_SIZE_SHRINK" != "y" ]]; then
    BUILD_SERVICE=1 make install DESTDIR="${SYSTEM_OUT_DIR}/usr" || return "$?"
  fi
)}

function clean_cvi_rtsp()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$CVI_RTSP_PATH" || return
  BUILD_SERVICE=1 make clean
)}

function build_pqtool_server()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$PQTOOL_SERVER_PATH" || return
  make all SDK_VER="$SDK_VER" MULTI_PROCESS_SUPPORT="$MULTI_PROCESS_SUPPORT"
  test "$?" -ne 0 && print_notice "build pqtool_server failed !!" && popd && return 1

  if [[ "$FLASH_SIZE_SHRINK" != "y" ]]; then
    make install DESTDIR="$SYSTEM_OUT_DIR" || return "$?"
  fi
)}

function clean_pqtool_server()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  cd "$PQTOOL_SERVER_PATH" || return
  make clean
  make uninstall DESTDIR="$SYSTEM_OUT_DIR"
)}

function build_3rd_party()
{
  mkdir -p "$OSS_TARBALL_PATH"

  if [ -d "${OSS_PATH}/oss_release_tarball" ]; then
    echo "oss prebuilt tarball found!"
  else
    echo "Try to download oss_release_tarball.tar tarball ..."
    #wget ...
    #tar -xvf ${OSS_PATH}/oss_release_tarball.tar -C ${OSS_PATH}
  fi
  echo "cp -rpf ${OSS_PATH}/oss_release_tarball/${SDK_VER}/*  ${OSS_TARBALL_PATH}"
  cp -rpf ${OSS_PATH}/oss_release_tarball/${SDK_VER}/*  ${OSS_TARBALL_PATH}

  local oss_list=(
    "zlib"
    "glog"
    "flatbuffers"
    "opencv"
    "live555"
    "sqlite3"
    "ffmpeg"
    "thttpd"
    "openssl"
    "libwebsockets"
    "json-c"
    "nanomsg"
    "miniz"
    "uv"
    "cvi-json-c"
    "cvi-miniz"
  )

  for name in "${oss_list[@]}"
  do
    if [ -f "${OSS_TARBALL_PATH}/${name}.tar.gz" ]; then
      echo "$name found"
      "$OSS_PATH"/run_build.sh -n "$name" -e -t "$OSS_TARBALL_PATH" -i "$TPU_SDK_INSTALL_PATH"
        echo "$name successfully downloaded and untared."
    else
      echo "$name not found"
    fi
  done
}

function clean_3rd_party()
{
  rm -rf "$OSS_PATH"/build
  rm -rf "$OSS_TARBALL_PATH"
}

function clean_ramdisk()
{
  rm -rf "${RAMDISK_PATH:?}"/"$RAMDISK_OUTPUT_BASE"
  rm -rf "$SYSTEM_OUT_DIR"
  rm -rf "$ROOTFS_DIR"
}

function prepare_git_hook()
{
   print_notice "Run ${FUNCNAME[0]}() function"
   if [[ -d ".git" ]]; then
	mkdir -p .git/hooks
	cp ${TOP_DIR}/build/hook/commit-msg .git/hooks/
	chmod +x .git/hooks/commit-msg
	cp ${TOP_DIR}/build/hook/prepare-commit-msg .git/hooks/
	chmod +x .git/hooks/prepare-commit-msg
   else
	print_notice "Abort .git is not exist !!!"
   fi
}

# shellcheck disable=SC2120
function build_all()
{(
  build_uboot || return $?
  build_kernel || return $?
  build_ramboot || return $?
  build_osdrv || return $?
  build_3rd_party || return $?
  build_middleware || return $?
  if [ "$TPU_REL" = 1 ]; then
    build_tpu_sdk || return $?
    build_ive_sdk || return $?
    build_ivs_sdk || return $?
    build_tdl_sdk  || return $?
  fi
  pack_cfg || return $?
  pack_rootfs || return $?
  pack_data || return $?
  pack_system || return $?
  copy_tools || return $?
  pack_upgrade || return $?
)}

function clean_all()
{
  clean_uboot
  clean_opensbi
  clean_rtos
  [[ "$ATF_SRC" == y ]] && clean_atf
  clean_kernel
  clean_ramdisk
  clean_3rd_party
  if [ "$TPU_REL" = 1 ]; then
    clean_ive_sdk
    clean_ivs_sdk
    clean_tpu_sdk
    clean_tdl_sdk
    clean_cnv_sdk
  fi
  clean_middleware
  clean_osdrv
}

function distclean_all()
{(
  print_notice "Run ${FUNCNAME[0]}() function"
  clean_all
  #repo forall -c "git clean -dfx"
)}

# shellcheck disable=SC2120
function envs_sdk_ver()
{
  if [ -n "$1" ]; then
    SDK_VER="$1"
  fi

  if [ "$SDK_VER" = 64bit ]; then
    CROSS_COMPILE="$CROSS_COMPILE_64"
    CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_64"
    SYSROOT_PATH="$SYSROOT_PATH_64"
  elif [ "$SDK_VER" = 32bit ]; then
    CROSS_COMPILE="$CROSS_COMPILE_32"
    CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_32"
    SYSROOT_PATH="$SYSROOT_PATH_32"
  elif [ "$SDK_VER" = uclibc ]; then
    CROSS_COMPILE="$CROSS_COMPILE_UCLIBC"
    CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_UCLIBC"
    SYSROOT_PATH="$SYSROOT_PATH_UCLIBC"
  elif [ "$SDK_VER" = glibc_riscv64 ]; then
    CROSS_COMPILE="$CROSS_COMPILE_GLIBC_RISCV64"
    CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_GLIBC_RISCV64"
    SYSROOT_PATH="$SYSROOT_PATH_GLIBC_RISCV64"
  elif [ "$SDK_VER" = musl_riscv64 ]; then
    CROSS_COMPILE="$CROSS_COMPILE_MUSL_RISCV64"
    CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_MUSL_RISCV64"
    SYSROOT_PATH="$SYSROOT_PATH_MUSL_RISCV64"
  else
    echo -e "Invalid SDK_VER=${SDK_VER}"
    exit 1
  fi

  TPU_OUTPUT_PATH="$OUTPUT_DIR"/tpu_"$SDK_VER"
  # ramdisk packages PATH
  pushd $BUILD_PATH || return $?
  CVI_TARGET_PACKAGES_LIBDIR=$(make print-target-packages-libdir)
  CVI_TARGET_PACKAGES_INCLUDE=$(make print-target-packages-include)
  popd
  export CVI_TARGET_PACKAGES_LIBDIR
  export CVI_TARGET_PACKAGES_INCLUDE

  OSS_TARBALL_PATH="$TPU_OUTPUT_PATH"/third_party
  TPU_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_tpu_sdk
  IVE_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_ive_sdk
  IVS_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_ivs_sdk
  CNV_SDK_INSTALL_PATH="$TPU_OUTPUT_PATH"/cvitek_cnv_sdk
  TPU_MODEL_PATH="$TPU_OUTPUT_PATH"/models
  IVE_CMODEL_INSTALL_PATH="$TPU_OUTPUT_PATH"/tools/ive_cmodel
}

function cvi_setup_env()
{
  local _tmp ret

  _build_default_env

  _tmp=$(python3 "${TOP_DIR}/build/scripts/boards_scan.py" --gen-board-env="${CHIP}_${BOARD}")
  ret=$?
  [[ "$ret" == 0 ]] || return "$ret"

  # shellcheck disable=SC1090
  source <(echo "${_tmp}")

  if [[ "$CHIP_ARCH" == "CV183X" ]];then
  export  CVIARCH="CV183X"
  fi
  if [[ "$CHIP_ARCH" == "CV182X" ]];then
  export  CVIARCH="CV182X"
  fi
  if [[ "$CHIP_ARCH" == "CV181X" ]];then
  export  CVIARCH="CV181X"
  fi
  if [[ "$CHIP_ARCH" == "CV180X" ]];then
  export  CVIARCH="CV180X"
  fi
  if [[ "$CHIP_ARCH" == "ATHENA2" ]];then
  export  CVIARCH="ATHENA2"
  fi

  export BRAND BUILD_VERBOSE DEBUG PROJECT_FULLNAME
  export OUTPUT_DIR ATF_PATH BM_BLD_PATH OPENSBI_PATH UBOOT_PATH FREERTOS_PATH
  export KERNEL_PATH RAMDISK_PATH OSDRV_PATH TOOLS_PATH COMMON_TOOLS_PATH

  PROJECT_FULLNAME="$CHIP"_"$BOARD"

  # output folder path
  INSTALL_PATH="$TOP_DIR"/install
  OUTPUT_DIR="$INSTALL_PATH"/soc_"$PROJECT_FULLNAME"
  ROOTFS_DIR="$OUTPUT_DIR"/rootfs
  SYSTEM_OUT_DIR="$OUTPUT_DIR"/rootfs/mnt/system

  # source file folders
  FSBL_PATH="$TOP_DIR"/fsbl
  ATF_PATH="$TOP_DIR"/arm-trusted-firmware
  UBOOT_PATH="$TOP_DIR/$UBOOT_SRC"
  FREERTOS_PATH="$TOP_DIR"/freertos
  ALIOS_PATH="$TOP_DIR"/alios
  KERNEL_PATH="$TOP_DIR"/"$KERNEL_SRC"
  OSDRV_PATH="$TOP_DIR"/osdrv
  RAMDISK_PATH="$TOP_DIR"/ramdisk
  BM_BLD_PATH="$TOP_DIR"/bm_bld
  TOOLCHAIN_PATH="$TOP_DIR"/host-tools
  OSS_PATH="$TOP_DIR"/oss
  OPENCV_PATH="$TOP_DIR"/opencv
  APPS_PATH="$TOP_DIR"/apps
  MW_PATH="$TOP_DIR"/cvi_mpi
  PQTOOL_SERVER_PATH="$MW_PATH"/modules/isp/"${CHIP_ARCH,,}"/isp-tool-daemon/isp_daemon_tool
  ISP_TUNING_PATH="$TOP_DIR"/isp_tuning
  TPU_SDK_PATH="$TOP_DIR"/cviruntime
  IVE_SDK_PATH="$TOP_DIR"/ive
  IVS_SDK_PATH="$TOP_DIR"/ivs
  CNV_SDK_PATH="$TOP_DIR"/cnv
  TDL_SDK_PATH="$TOP_DIR"/tdl_sdk
  CVI_PIPELINE_PATH="$TOP_DIR"/cvi_pipeline
  CVI_RTSP_PATH="$TOP_DIR"/cvi_rtsp
  OPENSBI_PATH="$TOP_DIR"/opensbi
  TOOLS_PATH="$BUILD_PATH"/tools
  COMMON_TOOLS_PATH="$TOOLS_PATH"/common
  VENC_PATH="$MW_PATH"/modules/venc
  IMGTOOL_PATH="$COMMON_TOOLS_PATH"/image_tool
  EMMCTOOL_PATH="$COMMON_TOOLS_PATH"/emmc_tool
  SCRIPTTOOL_PATH="$COMMON_TOOLS_PATH"/scripts
  ROOTFSTOOL_PATH="$COMMON_TOOLS_PATH"/rootfs_tool
  SPINANDTOOL_PATH="$COMMON_TOOLS_PATH"/spinand_tool
  BOOTLOGO_PATH="$COMMON_TOOLS_PATH"/bootlogo/logo.jpg

  # subfolder path for buidling, chosen accroding to .gitignore rules
  UBOOT_OUTPUT_FOLDER=build/"$PROJECT_FULLNAME"
  RAMDISK_OUTPUT_BASE=build/"$PROJECT_FULLNAME"
  KERNEL_OUTPUT_FOLDER=build/"$PROJECT_FULLNAME"
  RAMDISK_OUTPUT_FOLDER="$RAMDISK_OUTPUT_BASE"/workspace

  # toolchain
  export CROSS_COMPILE_64=aarch64-linux-gnu-
  export CROSS_COMPILE_32=arm-linux-gnueabihf-
  export CROSS_COMPILE_UCLIBC=arm-cvitek-linux-uclibcgnueabihf-
  export CROSS_COMPILE_64_NONOS=aarch64-elf-
  export CROSS_COMPILE_64_NONOS_RISCV64=riscv64-unknown-elf-
  export CROSS_COMPILE_GLIBC_RISCV64=riscv64-unknown-linux-gnu-
  export CROSS_COMPILE_MUSL_RISCV64=riscv64-unknown-linux-musl-
  export CROSS_COMPILE="$CROSS_COMPILE_64"

  # toolchain path
  CROSS_COMPILE_PATH_64="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
  CROSS_COMPILE_PATH_32="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf
  CROSS_COMPILE_PATH_UCLIBC="$TOOLCHAIN_PATH"/gcc/arm-cvitek-linux-uclibcgnueabihf
  CROSS_COMPILE_PATH_64_NONOS="$TOOLCHAIN_PATH"/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-elf
  CROSS_COMPILE_PATH_64_NONOS_RISCV64="$TOOLCHAIN_PATH"/gcc/riscv64-elf-x86_64
  CROSS_COMPILE_PATH_GLIBC_RISCV64="$TOOLCHAIN_PATH"/gcc/riscv64-linux-x86_64
  CROSS_COMPILE_PATH_MUSL_RISCV64="$TOOLCHAIN_PATH"/gcc/riscv64-linux-musl-x86_64
  CROSS_COMPILE_PATH="$CROSS_COMPILE_PATH_64"

  # add toolchain path
  pathprepend "$CROSS_COMPILE_PATH_64"/bin
  pathprepend "$CROSS_COMPILE_PATH_32"/bin
  pathprepend "$CROSS_COMPILE_PATH_64_NONOS"/bin
  pathprepend "$CROSS_COMPILE_PATH_64_NONOS_RISCV64"/bin
  pathprepend "$CROSS_COMPILE_PATH_GLIBC_RISCV64"/bin
  pathprepend "$CROSS_COMPILE_PATH_MUSL_RISCV64"/bin
  pathappend "$CROSS_COMPILE_PATH_UCLIBC"/bin

  # Check ccache is enable or not
  pathremove "$BUILD_PATH"/output/bin
  rm -rf "$BUILD_PATH"/output/bin/
  if [ "$USE_CCACHE" == "y" ];then
    if command -v ccache &> /dev/null;then
      mkdir -p "$BUILD_PATH"/output/bin
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-elf-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/aarch64-linux-gnu-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-linux-gnueabihf-c++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-gcc
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-g++
      ln -s "$(which ccache)" "$BUILD_PATH"/output/bin/arm-cvitek-linux-uclibcgnueabihf-c++
      pathprepend "$BUILD_PATH"/output/bin
    else
      echo "You have enabled ccache but there is no ccache in your PATH. Please cheack!"
      USE_CCACHE="n"
    fi
  fi

  # sysroot
  SYSROOT_PATH_64="$RAMDISK_PATH"/sysroot/sysroot-glibc-linaro-2.23-2017.05-aarch64-linux-gnu
  SYSROOT_PATH_32="$RAMDISK_PATH"/sysroot/sysroot-glibc-linaro-2.23-2017.05-arm-linux-gnueabihf
  SYSROOT_PATH_UCLIBC="$RAMDISK_PATH"/sysroot/sysroot-uclibc
  SYSROOT_PATH_GLIBC_RISCV64="$RAMDISK_PATH"/sysroot/sysroot-glibc-riscv64
  SYSROOT_PATH_MUSL_RISCV64="$RAMDISK_PATH"/sysroot/sysroot-musl-riscv64
  SYSROOT_PATH="$SYSROOT_PATH_64"

  # envs setup for specific ${SDK_VER}
  envs_sdk_ver

  if [ "${STORAGE_TYPE}" == "spinand" ]; then
    PAGE_SUFFIX=2k
    if [ ${NANDFLASH_PAGESIZE} == 4096 ]; then
      PAGE_SUFFIX=4k
    fi

    if [[ "$ENABLE_ALIOS" != "y" ]]; then
      pushd "$BUILD_PATH"/boards/"${CHIP_ARCH,,}"/"$PROJECT_FULLNAME"/partition/
      if [[ "$AB_SYSTEM" == "y" ]]; then
        ln -fs ../../../default/partition/partition_spinand_page_"$PAGE_SUFFIX"_ab.xml \
          partition_"$STORAGE_TYPE".xml
	  else
        ln -fs ../../../default/partition/partition_spinand_page_"$PAGE_SUFFIX".xml \
          partition_"$STORAGE_TYPE".xml
	  fi
      popd
    fi
  fi

  # configure flash partition table
  if [ -z "${STORAGE_TYPE}" ]; then
    FLASH_PARTITION_XML="$BUILD_PATH"/boards/default/partition/partition_none.xml
  else
    FLASH_PARTITION_XML="$BUILD_PATH"/boards/"${CHIP_ARCH,,}"/"$PROJECT_FULLNAME"/partition/partition_"$STORAGE_TYPE".xml
    if ! [ -e "$FLASH_PARTITION_XML" ]; then
      print_error "${FLASH_PARTITION_XML} does not exist!!"
      return 1
    fi
  fi

  export SYSTEM_OUT_DIR
  export CROSS_COMPILE_PATH
  # buildroot config
  export BR_DIR="$TOP_DIR"/buildroot-2021.05
  export BR_BOARD=cvitek_${CHIP_ARCH}_${SDK_VER}
  export BR_OVERLAY_DIR=${BR_DIR}/board/cvitek/${CHIP_ARCH}/overlay
  export BR_DEFCONFIG=${BR_BOARD}_defconfig
  export BR_ROOTFS_DIR="$OUTPUT_DIR"/tmp-rootfs
}

cvi_print_env()
{
  echo -e ""
  echo -e "\e[1;32m====== Environment Variables ======= \e[0m\n"
  echo -e "  PROJECT: \e[34m$PROJECT_FULLNAME\e[0m, DDR_CFG=\e[34m$DDR_CFG\e[0m"
  echo -e "  CHIP_ARCH: \e[34m$CHIP_ARCH\e[0m, DEBUG=\e[34m$DEBUG\e[0m"
  echo -e "  SDK VERSION: \e[34m$SDK_VER\e[0m, RPC=\e[34m$MULTI_PROCESS_SUPPORT\e[0m"
  echo -e "  ATF options: ATF_KEY_SEL=\e[34m$ATF_KEY_SEL\e[0m, BL32=\e[34m$ATF_BL32\e[0m"
  echo -e "  Linux source folder:\e[34m$KERNEL_SRC\e[0m, Uboot source folder: \e[34m$UBOOT_SRC\e[0m"
  echo -e "  CROSS_COMPILE_PREFIX: \e[34m$CROSS_COMPILE\e[0m"
  echo -e "  ENABLE_BOOTLOGO: $ENABLE_BOOTLOGO"
  echo -e "  Flash layout xml: $FLASH_PARTITION_XML"
  echo -e "  Sensor tuning bin: $SENSOR_TUNING_PARAM"
  echo -e "  Output path: \e[33m$OUTPUT_DIR\e[0m"
  echo -e ""
}

function print_usage()
{
  printf "  -------------------------------------------------------------------------------------------------------\n"
  printf "    Usage:\n"
  printf "    (1)\33[94m menuconfig \33[0m- Use menu to configure your board.\n"
  printf "        ex: $ menuconfig\n\n"
  printf "    (2)\33[96m defconfig \$CHIP_ARCH \33[0m- List EVB boards(\$BOARD) by CHIP_ARCH.\n"
  "${BUILD_PATH}/scripts/boards_scan.py" --list-chip-arch
  printf "        ex: $ defconfig cv181x\n\n"
  printf "    (3)\33[92m defconfig \$BOARD\33[0m - Choose EVB board settings.\n"
  printf "        ex: $ defconfig cv1813h_wevb_0007a_spinor\n"
  printf "        ex: $ defconfig cv1812cp_wevb_0006a_spinor\n"
  printf "  -------------------------------------------------------------------------------------------------------\n"
}

TOP_DIR=$(gettop)
BUILD_PATH="$TOP_DIR/build"
export TOP_DIR BUILD_PATH
"${BUILD_PATH}/scripts/boards_scan.py" --gen-build-kconfig
"${BUILD_PATH}/scripts/gen_sensor_config.py"
"${BUILD_PATH}/scripts/gen_panel_config.py"

# import common functions
# shellcheck source=./common_functions.sh
source "$TOP_DIR/build/common_functions.sh"
# shellcheck source=./release_functions.sh
#source "$TOP_DIR/build/release_functions.sh"
# shellcheck source=./riscv_functions.sh
source "$TOP_DIR/build/riscv_functions.sh"
# shellcheck source=./alios_functions.sh
#source "$TOP_DIR/build/alios_functions.sh"

print_usage
