#!/bin/bash
set -e

if [[ -z "$MODEL_PATH" ]]; then
  MODEL_PATH=$TPU_ROOT/../cvimodel_regression
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
  "resnet50"
  "resnet18"
  "mobilenet_v1"
  "mobilenet_v2"
  "squeezenet_v1.1"
  "shufflenet_v2"
  "googlenet"
  "inception_v3"
  "inception_v4"
  "vgg16"
  "densenet_121"
  "densenet_201"
  "senet_res50"
  "resnext50"
  "res2net50"
  "ecanet50"
  "efficientnet_b0"
  "efficientnet-lite_b0"
  "nasnet_mobile"
  ####################
  # Detection        #
  ####################
  "retinaface_mnet25"
  "retinaface_mnet25_600"
  "retinaface_res50"
  "ssd300"
  "mobilenet_ssd"
  "yolo_v1_448"
  "yolo_v2_416"
  "yolo_v2_1080"
  "yolo_v3_320"
  "yolo_v3_416"
  "yolo_v3_tiny"
  "yolo_v3_608"
  "yolo_v3_spp"
  "yolo_v4"
  "yolo_v4_s"
  "yolo_v5_s"
  "yolox_s"
  "faster_rcnn"
  ####################
  # Face Recog       #
  ####################
  "arcface_res50"
  ####################
  # Pose             #
  ####################
  "alphapose"
  ####################
  # SuperRes         #
  ####################
  "espcn_3x"
  ####################
  # Segementation    #
  ####################
  "unet"
  "erfnet"
  "enet"
  # "icnet"         ## ION size
  # "fcn-8s"        ## ION size
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
      --input $MODEL_PATH/${model}_bs1_in_fp32.npz \
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
    --input $MODEL_PATH/${model}_bs1_in_fp32.npz \
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
