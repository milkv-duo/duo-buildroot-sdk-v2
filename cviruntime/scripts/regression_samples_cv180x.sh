set -ex

echo "tpu samples regression for cv180x platform"

if [  -z "$MODEL_PATH" ] || [ ! -e $MODEL_PATH ]; then
  echo "MODEL_PATH $MODEL_PATH does not exist"
  echo "Please set MODEL_PATH to cvimodel_regression dir"
  exit 1
fi

if [ -z "$TPU_ROOT" ] || [ ! -e $TPU_ROOT ]; then
  echo "TPU_ROOT $TPU_ROOT does not exist"
  echo "Please set TPU_ROOT to cvitek_tpu_sdk dir"
  exit 1
fi

#export TPU_ENABLE_PMU=1

samples_list="run_classifier.sh run_classifier_fused_preprocess.sh"
#"run_classifier_multi_batch.sh"

samples_extra_list=""
  #"run_alphapose.sh"
  #"run_alphapose_fused_preprocess.sh"
  #"run_classifier_yuv420.sh"
  #"run_detector_yolov3_fused_preprocess.sh"
  #"run_detector_yolov3.sh"
  #"run_detector_yolov5_fused_preprocess.sh"
  #"run_detector_yolov5.sh"
  #"run_detector_yolox_s.sh"
  #"run_insightface_fused_preprocess.sh"

function test_sample() {
  for sample in $*
  do
    echo $sample
    sh -ex ./$sample
    if [ "$?" -ne "0" ]; then
      echo "$samples FAILED"
      return 1
    fi
  done
  return 0
}

cd $TPU_ROOT/samples
# test samples
test_sample ${samples_list}
if [ $? -ne 0 ]; then
  echo "test samples failed !!"
  exit 1
fi

cd $TPU_ROOT/samples/samples_extra
# test samples_extra
test_sample ${samples_extra_list}
if [ $? -ne 0 ]; then
  echo "test samples failed !!"
  exit 1
fi

exit 0
