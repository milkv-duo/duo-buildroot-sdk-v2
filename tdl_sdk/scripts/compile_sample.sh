#!/bin/bash
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

MW_PATH="${MW_PATH:-$CURRENT_DIR/3rd/middleware/v2}"
TPU_PATH="${TPU_SDK_INSTALL_PATH:-$CURRENT_DIR/3rd/tpu}"
IVE_PATH="${IVE_SDK_INSTALL_PATH:-$CURRENT_DIR/3rd/ive}"
KERNEL_ROOT="${KERNEL_ROOT:-$CURRENT_DIR}"
USE_TPU_IVE="${USE_TPU_IVE:-ON}"
CHIP_ARCH="${CHIP_ARCH:-CV180X}"
SDK_VER="${SDK_VER:-musl_riscv64}"


if [[ "$CHIP_ARCH" == "CV183X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV182X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "CV181X" ]]; then
    USE_TPU_IVE=OFF
elif [[ "$CHIP_ARCH" == "CV180X" ]]; then
    USE_TPU_IVE=ON
elif [[ "$CHIP_ARCH" == "SOPHON" ]] || [[ "$CHIP_ARCH" == "CV186X" ]]; then
    CHIP_ARCH=CV186X
    USE_TPU_IVE=OFF
else
    echo "Unsupported chip architecture: ${CHIP_ARCH}"
    exit 1
fi

echo "enter compile_sample.sh !!!"
echo "MW_PATH: $MW_PATH"
echo "TPU_PATH: $TPU_PATH"
echo "IVE_PATH: $IVE_PATH"
echo "KERNEL_ROOT: $KERNEL_ROOT"
echo "USE_TPU_IVE: $USE_TPU_IVE"
echo "CHIP_ARCH: $CHIP_ARCH"
echo "SDK_VER: $SDK_VER"

MAKE_OPTS=("KERNEL_ROOT=$KERNEL_ROOT" "MW_PATH=$MW_PATH" "TPU_PATH=$TPU_PATH"
           "IVE_PATH=$IVE_PATH" "USE_TPU_IVE=$USE_TPU_IVE"
           "CHIP=$CHIP_ARCH" "SDK_VER=$SDK_VER" "-j10")

build_and_clean() {
    pushd $1 || exit 1
    make "${MAKE_OPTS[@]}" || exit 1
    make clean || exit 1
    echo "$1 done"
    popd
}

build_and_clean "${CURRENT_DIR}/cvi_tdl"
build_and_clean "${CURRENT_DIR}/cvi_tdl_app"
build_and_clean "${CURRENT_DIR}/cvi_md"
build_and_clean "${CURRENT_DIR}/cvi_preprocess"
build_and_clean "${CURRENT_DIR}/cvi_yolo"

if [ -d "${CURRENT_DIR}/cvi_draw_rect" ]; then
    build_and_clean "${CURRENT_DIR}/cvi_draw_rect"
fi

echo "finish build sample!!!"