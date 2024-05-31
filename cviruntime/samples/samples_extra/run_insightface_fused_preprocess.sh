#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_fd_fr_fused_preprocess ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel ]; then
  echo "$0 Model retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel not present"
  exit 1
fi
if [ ! -e $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel ]; then
  echo "$0 Model arcface_res50_fused_preprocess.cvimodel not present"
  exit 1
fi

# mnet25
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama1.jpg \
    ./data/obama2.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama1.jpg \
    ./data/obama3.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama2.jpg \
    ./data/obama3.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama1.jpg \
    ./data/trump1.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama1.jpg \
    ./data/trump2.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
./bin/cvi_sample_fd_fr_fused_preprocess \
    $MODEL_PATH/retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
    $MODEL_PATH/arcface_res50_fused_preprocess.cvimodel \
    ./data/obama1.jpg \
    ./data/trump3.jpg
test $? -ne 0 && echo "cvi_sample_fd_fr_fused_preprocess failed !!"  && exit 1
exit 0