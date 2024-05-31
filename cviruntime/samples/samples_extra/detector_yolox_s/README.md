# Yolox_s Sample with post_process and withdou fuse_preprocess

### Download the model and convert the model under docker (optional)
#### For new toolchain guide
The following documents are required:
* tpu-mlir_xxxx.tar.gz (The release package of tpu-mlir)

Transform cvimodel shell:
``` shell
tar zxf tpu-mlir_xxxx.tar.gz
source tpu-mlir_xxxx/envsetup.sh

mkdir workspace && cd workspace
cp $TPUC_ROOT/regression/image/dog.jpg .
cp -rf $TPUC_ROOT/regression/dataset/COCO2017 .

model_transform.py \
--model_name yolox_s \
--model_def ./yolox_s.onnx \
--test_input ./dog.jpg \
--test_result yolox_s_top_output.npz \
--input_shapes [[1,3,640,640]]
--resize_dims 640,640 \
--mean 0,0,0 \
--scale 1.0,1.0,1.0 \
--pixel_format "bgr" \
--tolerance 0.99,0.99 \
--excepts 796_Sigmoid \
--mlir yolox_s.mlir

run_calibration.py \
yolox_s.mlir \
--dataset=./COCO2017 \
--input_num=100 \
-o yolox_s_calibration_table

model_deploy.py \
--mlir yolox_s.mlir \
--calibration_table yolox_s_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--test_input ./dog.jpg \
--test_reference yolox_s_top_output.npz \
--excepts 796_Sigmoid \
--tolerance 0.8,0.5 \
--model yolox_s.cvimodel
```

#### For old toolchain guide
The following documents are required:

* cvitek_mlir_ubuntu-18.04.tar.gz

Transform model shell:
``` shell
tar zxf cvitek_mlir_ubuntu-18.04.tar.gz
source cvitek_mlir/cvitek_envs.sh

mkdir workspace && cd workspace
cp $MLIR_PATH/tpuc/regression/data/dog.jpg .
cp -rf $MLIR_PATH/tpuc/regression/data/images .

model_transform.py \
  --model_type onnx \
  --model_name yolox_s \
  --model_def ./yolox_s.onnx \
  --image ./dog.jpg \
  --image_resize_dims 640,640 \
  --keep_aspect_ratio true \
  --model_channel_order "bgr" \
  --tolerance 0.99,0.99,0.99 \
  --mlir yolox_s_fp32.mlir

run_calibration.py \
yolox_s_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o yolox_s_calib.txt

model_deploy.py \
--model_name yolox \
--mlir yolox_s_fp32.mlir \
--calibration_table yolox_s_calib.txt \
--pixel_format BGR_PLANAR \
--excepts "796_Sigmoid" \
--chip cv183x \
--quantize INT8 \
--image dog.jpg \
--tolerance 0.85,0.85,0.4 \
--correctness 0.95,0.95,0.9 \
--cvimodel yolox_s.cvimodel
```
Copy generated yolox_s.cvimodel to EVB board

## How To Compile Vpss input Sample In Docker
View the Top level directory README.md

## Run Samples In EVB Borad
```
cd install_samples/samples_extra
./bin/cvi_sample_detector_yolox_s \
./yolox_s.cvimodel \
./data/dog.jpg \
yolox_s_out.jpg
```