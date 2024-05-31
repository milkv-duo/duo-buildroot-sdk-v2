set -e

echo "new cvimodels regression for cv180x platform"

if [[ -z "$MODEL_PATH" ]]; then
  MODEL_PATH=$TPU_ROOT/../cvimodel_regression_int8_cv180x
fi
if [ ! -e $MODEL_PATH ]; then
  echo "MODEL_PATH $MODEL_PATH does not exist"
  echo "Please set MODEL_PATH to cvimodel_regression dir"
  exit 1
fi
export MODEL_PATH=$MODEL_PATH

if [ -f "/sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem" ]; then
  total_ion_size=$(cat /sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem)
else
  # if ion size is unknown then execute basic tests.
  total_ion_size=20000001
fi

# ION requirement >= 10 MB
if [ "$total_ion_size" -gt "10000000" ]; then
model_list="blazeface espcn_3x mobilenet_v2_cf nasnet_mobile shufflenet_v2 squeezenet_v1.1_cf "
fi

# ION requirement >= 20 MB
if [ "$total_ion_size" -gt "20000000" ]; then
model_list="$model_list resnet18_v1 googlenet_cf"
fi

if [ "$total_ion_size" -gt "35000000" ]; then
model_list="$model_list densenet121-12 efficientdet-d0"
fi

# turn on PMU
export TPU_ENABLE_PMU=1

if [ ! -e sdk_regression_out ]; then
  mkdir sdk_regression_out
fi

ERR=0
cd sdk_regression_out

if [ -z $1 ]; then
  for model in ${model_list}
  do
    echo "test $model"
    model_runner \
      --input $MODEL_PATH/${model}_in_f32.npz \
      --model $MODEL_PATH/${model}_bs1.cvimodel \
      --reference $MODEL_PATH/${model}_bs1_out_all.npz 2>&1 | tee $model.log
    if [ "$?" -ne "0" ]; then
      echo "$model test FAILED" >> verdict.log
      ERR=1
    else
      echo "$model test PASSED" >> verdict.log
    fi
  done
fi

# VERDICT
if [ $ERR -eq 0 ]; then
  echo $0 ALL TEST PASSED
else
  echo $0 FAILED
fi

exit $ERR
