#!/bin/bash
set -x

cd /mnt/data/nfs/
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..
export TPU_ROOT=$PWD/cvitek_tpu_sdk

# For batch_size = 1
MODEL_PATH=$PWD/cvimodel_regression_int8_cv183x $TPU_ROOT/regression_models.sh
