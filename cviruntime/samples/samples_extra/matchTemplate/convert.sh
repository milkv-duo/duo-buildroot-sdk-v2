#!/bin/bash
set -xe

export SET_CHIP_NAME="cv182x"
mkdir -p tmp

pushd tmp

tpuc-opt ../match_template.mlir \
 --shape-infer \
 --canonicalize \
 --extra-optimize \
 -o ccoeff_normed_fp32.mlir

model_deploy.py \
   --mlir ccoeff_normed_fp32.mlir  \
   --quantize BF16 \
   --quant_input \
   --chip cv182x \
   --test_input ../input.npz \
   --compare_all \
   --model ccoeff_normed.cvimodel
popd
