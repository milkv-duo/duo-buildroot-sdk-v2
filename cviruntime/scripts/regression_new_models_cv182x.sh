#!/bin/bash
set -e

echo "new cvimodels regression for cv182x platform"

if [[ -z "$MODEL_PATH" ]]; then
  MODEL_PATH=$TPU_ROOT/../cvimodel_regression_int8_cv182x
fi
if [ ! -e $MODEL_PATH ]; then
  echo "MODEL_PATH $MODEL_PATH does not exist"
  echo "Please set MODEL_PATH to cvimodel_regression dir"
  exit 1
fi
export MODEL_PATH=$MODEL_PATH

model_list=(
  ####################
  # Classification   #
  ####################
  "resnet50_v1"
  "resnet18_v1"
  "mobilenet_v2_cf"
  "squeezenet_v1.1_cf"
  "shufflenet_v2"
  "googlenet_cf"
  "inception_v3"
  "densenet121-12"
  "densenet201"
  "se-resnet50"
  "resnext50_cf"
  "efficientnet"
  "nasnet_mobile"
  ####################
  # Detection        #
  ####################
  "retinaface"
  "retinaface_mnet_with_det"
  "blazeface"
  "mobilenetv2_ssd_cf"
  "efficientdet-d0"
  "yolov3_416_with_det"
  "yolov3_tiny"
  "yolov4s"
  "yolov5s"
  "yolox_s"
  "yolov8n"
  "pp_yoloe_m"
  "pp_yolox"
  ""
  ####################
  # Face Recog       #
  ####################
  "arcface_res50"
  ####################
  # Super Res       #
  ####################
  "espcn_3x"
  ####################
  # Pose             #
  ####################
  "alphapose_res50"
)

# turn on PMU
export TPU_ENABLE_PMU=1

if [ ! -e sdk_regression_out ]; then
  mkdir sdk_regression_out
fi

ERR=0
pushd sdk_regression_out

if [ -z $1 ]; then
  for model in ${model_list[@]}
  do
    echo "test $model"
    model_runner \
      --input $MODEL_PATH/${model}_in_f32.npz \
      --model $MODEL_PATH/${model}_bs1.cvimodel \
      --reference $MODEL_PATH/${model}_bs1_out_all.npz 2>&1 | tee $model.log
    if [ "${PIPESTATUS[0]}" -ne "0" ]; then
      echo "$model test FAILED" >> verdict.log
      ERR=1
    else
      echo "$model test PASSED" >> verdict.log
    fi
  done
else
  model=$1
  count=$2
  echo "test $model"
  model_runner \
    --input $MODEL_PATH/${model}_in_f32.npz \
    --model $MODEL_PATH/${model}_bs1.cvimodel \
    --reference $MODEL_PATH/${model}_bs1_out_all.npz \
    --count ${count} 2>&1 | tee $model.log
  if [ "${PIPESTATUS[0]}" -ne "0" ]; then
    echo "$model test FAILED" >> verdict.log
    ERR=1
  else
    echo "$model test PASSED" >> verdict.log
  fi
fi

popd

# VERDICT
if [ $ERR -eq 0 ]; then
  echo $0 ALL TEST PASSED
else
  echo $0 FAILED
fi

exit $ERR
