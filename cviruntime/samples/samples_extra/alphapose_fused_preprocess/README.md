# Alphapose Sample

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
cp $TPUC_ROOT/regression/image/pose_256_192.jpg .
cp -rf $TPUC_ROOT/regression/dataset/COCO2017 .
cp $TPUC_ROOT/regression/cali_tables/alphapose_res50_cvi_cali_table .

## yolov3
model_transform.py \
--model_name yolov3 \
--model_def ./yolov3_416_with_detection.prototxt \
--model_data ./yolov3_416.caffemodel \
--test_input ./dog.jpg \
--test_result yolov3_top_output.npz \
--input_shapes [[1,3,416,416]]
--resize_dims 416,416 \
--keep_aspect_ratio true \
--mean 0,0,0 \
--scale 0.00392,0.00392,0.00392 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--excepts output \
--mlir yolov3.mlir

run_calibration.py \
yolov3.mlir \
--dataset=./COCO2017 \
--input_num=100 \
-o yolov3_calibration_table

model_deploy.py \
--mlir yolov3.mlir \
--calibration_table yolov3_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--test_input ./dog.jpg \
--test_reference yolov3_top_output.npz \
--excepts output \
--tolerance 0.9,0.3 \
--fuse_preprocess \
--customization_format RGB_PLANAR \
--model yolo_v3_416_fused_preprocess_with_detection.cvimodel

## alphapose
model_transform.py \
--model_name alphapose \
--model_def ./alphapose_resnet50_256x192.onnx \
--test_input ./pose_256_192.jpg \
--test_result alphapose_top_output.npz \
--input_shapes [[1,3,256,192]] \
--resize_dims 256,192 \
--scale 0.00392,0.00392,0.00392 \
--mean 103.53,116.535,122.399 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--mlir alphapose.mlir

model_deploy.py \
--mlir alphapose.mlir \
--calibration_table alphapose_res50_cvi_cali_table \
--test_input pose_256_192.jpg \
--test_reference ./pose_256_192.jpg \
--excepts 404_Relu \
--chip cv183x \
--quantize INT8 \
--quant_input \
--tolerance 0.89,0.49 \
--fuse_preprocess \
--customization_format RGB_PLANAR \
--model alphapose_fused_preprocess.cvimodel
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
cp $MLIR_PATH/tpuc/regression/data/pose_256_192.jpg .
cp -rf $MLIR_PATH/tpuc/regression/data/images .

## yolov3
model_transform.py \
--model_type caffe \
--model_name yolov3_416 \
--model_def ./yolov3_416_with_detection.prototxt \
--model_data ./yolov3_416.caffemodel \
--image ./dog.jpg \
--image_resize_dims 416,416 \
--keep_aspect_ratio true \
--raw_scale 1 \
--model_channel_order "rgb" \
--tolerance 0.99,0.99,0.99 \
--excepts output \
--mlir yolov3_416_fp32.mlir

run_calibration.py \
yolov3_416_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o yolo_v3_calibration_table_autotune

model_deploy.py \
--model_name yolov3_416 \
--mlir yolov3_416_fp32.mlir \
--calibration_table yolo_v3_calibration_table_autotune \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--aligned_input false \
--excepts output \
--chip cv183x \
--quantize INT8 \
--image dog.jpg \
--tolerance 0.9,0.9,0.3 \
--correctness 0.95,0.95,0.9 \
--cvimodel yolo_v3_416_fused_preprocess_with_detection.cvimodel

## alphapose
model_transform.py \
--model_type onnx \
--model_name alphapose \
--model_def ./alphapose_resnet50_256x192.onnx \
--image ./pose_256_192.jpg \
--net_input_dims 256,192 \
--image_resize_dims 256,192 \
--raw_scale 1 \
--mean 0.406,0.457,0.48 \
--model_channel_order "rgb" \
--tolerance 0.99,0.99,0.99 \
--mlir alphapose_fp32.mlir

run_calibration.py \
alphapose_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o alphapose_calibration_table

model_deploy.py \
--model_name alphapose \
--mlir alphapose_fp32.mlir \
--calibration_table alphapose_calibration_table \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--aligned_input false \
--excepts 404_Relu \
--chip cv183x \
--quantize INT8 \
--image pose_256_192.jpg \
--tolerance 0.91,0.89,0.49 \
--correctness 0.95,0.95,0.9 \
--cvimodel alphapose_fused_preprocess.cvimodel
```

Copy generated yolo_v3_416_fused_preprocess_with_detection.cvimodel and alphapose_fused_preprocess.cvimodel to EVB board

## How To Compile Sample In Docker
View the Top level directory README.md or View the cvitek_tpu_quick_start_guide.md

## Run Samples In EVB Borad
```
cd install_samples/samples_extra
./bin/cvi_sample_alphapose_fused_preprocess \
yolo_v3_416_fused_preprocess_with_detection.cvimodel \
alphapose_fused_preprocess.cvimodel \
./data/pose_demo_2.jpg \
alphapose_out.jpg 
```