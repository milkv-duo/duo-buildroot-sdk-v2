#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_detector_yolox_s ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/yolox_s.cvimodel ]; then
  echo "$0 Model yolox_s.cvimodel not present"
  exit 1
fi

./bin/cvi_sample_detector_yolox_s \
    $MODEL_PATH/yolox_s.cvimodel \
    ./data/dog.jpg \
    yolox_s_out.jpg

test $? -ne 0 && echo "cvi_sample_detector_yolox failed !!"  && exit 1
exit 0