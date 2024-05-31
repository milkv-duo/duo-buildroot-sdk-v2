#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_detector_yolo_v5_fused_preprocess ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/ppyoloe_m_int8.cvimodel ]; then
  echo "$0 Model ppyoloe_m_int8.cvimodel not present"
  exit 1
fi

./bin/cvi_sample_detector_ppyoloem_fused_preprocess \
    $MODEL_PATH/ppyoloe_m_int8.cvimodel \
    ./data/dog.jpg \
    ppyoloem_out.jpg

test $? -ne 0 && echo "cvi_sample_detector_ppyoloem_fused_preprocess failed !!"  && exit 1
exit 0