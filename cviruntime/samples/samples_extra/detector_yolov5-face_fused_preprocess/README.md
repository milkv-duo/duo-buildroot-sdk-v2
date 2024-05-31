# Yolov5s-face Sample with post_process

### Download the model and convert the model under docker (optional)
#### For new toolchain guide
The following documents are required:
* tpu-mlir_xxxx.tar.gz (The release package of tpu-mlir)

Transform cvimodel shell:
``` shell
tar zxf tpu-mlir_xxxx.tar.gz
source tpu-mlir_xxxx/envsetup.sh

mkdir workspace && cd workspace
cp $TPUC_ROOT/regression/image/parade.jpg .
cp -rf $TPUC_ROOT/regression/dataset/WIDER .

model_transform.py \
--model_name yolov5s-face \
--model_def ./yolov5s-face.onnx \
--test_input ./parade.jpg \
--test_result yolov5s-face_top_output.npz \
--input_shapes [[1,3,640,640]]
--resize_dims 640,640 \
--mean 0,0,0 \
--scale 0.00392,0.00392,0.00392 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--mlir yolov5s-face.mlir

run_calibration.py \
yolov5s-face.mlir \
--dataset=./WIDER \
--input_num=100 \
-o yolov5s-face_calibration_table

model_deploy.py \
--mlir yolov5s-face.mlir \
--calibration_table face_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--test_input ./parade.jpg \
--test_reference yolov5s-face_top_output.npz \
--tolerance 0.9,0.6 \
--fuse_preprocess \
--model yolov5s-face_fused_preprocess.cvimodel
```

#### For old toolchain guide
The following documents are required:

* cvitek_mlir_ubuntu-18.04.tar.gz

Transform model shell:
``` shell
tar zxf cvitek_mlir_ubuntu-18.04.tar.gz
source cvitek_mlir/cvitek_envs.sh

mkdir workspace && cd workspace
cp $MLIR_PATH/tpuc/regression/data/parade.jpg .
# set your own calibration dataset, this is example
cp -rf $MLIR_PATH/tpuc/regression/data/images . 

model_transform.py \
  --model_type onnx \
  --model_name yolov5s-face \
  --model_def yolov5s-face.onnx \
  --image parade.jpg \
  --image_resize_dims 640,640 \
  --net_input_dims 640,640 \
  --keep_aspect_ratio true \
  --raw_scale 1.0 \
  --mean 0.,0.,0. \
  --std 1.,1.,1. \
  --input_scale 1.0 \
  --model_channel_order "rgb" \
  --tolerance 0.99,0.99,0.99 \
  --mlir yolov5s-face.mlir

run_calibration.py \
yolov5s-face.mlir \
--dataset=./images \
--input_num=100 \
-o yolov5s-face_calibration_table

model_deploy.py \
  --model_name yolov5s-face \
  --mlir yolov5s-face.mlir \
  --calibration_table yolov5s-face_calibration_table \
  --quantize INT8 \
  --chip cv183x \
  --image parade.jpg \
  --fuse_preprocess \
  --tolerance 0.9,0.9,0.7 \
  --correctness 0.99,0.99,0.93 \
  --cvimodel yolov5s-face_fused_preprocess.cvimodel  
```

Copy generated yolov5s-face_fused_preprocess.cvimodel to EVB board

## How To Compile Vpss input Sample In Docker
View the Top level directory README.md

## Run Samples In EVB Borad
```
cd install_samples/samples_extra
./bin/cvi_sample_detector_yolov5-face_fused_preprocess \
./yolov5s-face_fused_preprocess.cvimodel \
./data/parade.jpg \
out.jpg
```