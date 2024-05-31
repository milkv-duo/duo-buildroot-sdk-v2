#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_detector_yolo_v3_fused_preprocess ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/yolo_v3_416_fused_preprocess_with_detection.cvimodel ]; then
  echo "$0 Model yolo_v3_416_fused_preprocess_with_detection.cvimodel not present"
  exit 1
fi

./bin/cvi_sample_detector_yolo_v3_fused_preprocess \
    $MODEL_PATH/yolo_v3_416_fused_preprocess_with_detection.cvimodel \
    ./data/dog.jpg \
    yolo_v3_out.jpg

test $? -ne 0 && echo "cvi_sample_detector_yolo_v3_fused_preprocess failed !!"  && exit 1
exit 0