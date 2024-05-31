#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_alphapose_fused_preprocess ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/yolo_v3_416_fused_preprocess_with_detection.cvimodel ]; then
  echo "$0 Model yolo_v3_416_with_detection.cvimodel not present"
  exit 1
fi
if [ ! -e $MODEL_PATH/alphapose_fused_preprocess.cvimodel ]; then
  echo "$0 Model alphapose_fused_preprocess.cvimodel not present"
  exit 1
fi

./bin/cvi_sample_alphapose_fused_preprocess \
    $MODEL_PATH/yolo_v3_416_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/alphapose_fused_preprocess.cvimodel \
    ./data/pose_demo_2.jpg \
    alphapose_out.jpg $1 $2

test $? -ne 0 && echo "cvi_sample_alphapose_fused_preprocess failed !!"  && exit 1
exit 0