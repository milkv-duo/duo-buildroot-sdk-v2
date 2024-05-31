# Test SDK on target board

- unzip tpu_sdk, and setup environment variables

  ```sh
  $ tar zxf cvitek_tpu_sdk.tar.gz
  $ export TPU_ROOT=$PWD/cvitek_tpu_sdk
  ```

- upzip cvimodel release

  ```sh
  $ tar zxf cvimodel_release.tar.gz
  $ export MODEL_PATH=$PWD/cvimodel_release
  ```

- run samples

  ```sh
  $ tar zxf cvimodel_samples.tar.gz
  $ cd cvimodel_samples
  $ $TPU_ROOT/samples/bin/cvi_sample_model_info \
      $MODEL_PATH/mobilenet_v2.cvimodel
  $ $TPU_ROOT/samples/run_classifier.sh
  $ $TPU_ROOT/samples/run_detector.sh
  $ $TPU_ROOT/samples/run_alphapose.sh
  $ $TPU_ROOT/samples/run_insightface.sh
  ```

- unzip cvimodel regression package, and run model regression

  ```sh
  $ tar zxf cvimodel_regression.tar.gz
  $ MODEL_PATH=$PWD/cvimodel_regression $TPU_ROOT/regression_models.sh
  ```
