#!/bin/sh

if [ -z $MODEL_PATH ]; then
  echo "$0 MODEL_PATH not set"
  exit 1
fi
if [ ! -e ./bin/cvi_sample_classifier_fused_preprocess ]; then
  echo "$0 Please run at the same dir as the script"
  exit 1
fi
if [ ! -e $MODEL_PATH/mobilenet_v2_fused_preprocess.cvimodel ]; then
  echo "$0 Model mobilenet_v2_fused_preprocess.cvimodel not present"
  exit 1
fi

./bin/cvi_sample_classifier_fused_preprocess \
    $MODEL_PATH/mobilenet_v2_fused_preprocess.cvimodel \
    ./data/cat.jpg \
    ./data/synset_words.txt
test $? -ne 0 && echo "cvi_sample_classifier_fused_preprocess failed !!"  && exit 1
exit 0