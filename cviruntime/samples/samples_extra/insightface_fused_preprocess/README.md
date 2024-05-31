# Face detection and recognition Sample

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
cp $TPUC_ROOT/regression/image/Aaron_Eckhart_0001.jpg .
cp -rf $TPUC_ROOT/regression/dataset/LFW .
cp -rf $TPUC_ROOT/regression/dataset/WIDER .

## retinaface fuse_post_process
model_transform.py \
--model_name mnet \
--model_def ./mnet_600_with_detection.prototxt \
--model_data ./mnet.caffemodel \
--test_input ./parade.jpg \
--test_result mnet_top_output.npz \
--input_shapes [[1,3,600,600]]
--resize_dims 600,600 \
--mean 0,0,0 \
--scale 1,1,1 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--excepts data \
--mlir mnet.mlir

run_calibration.py \
mnet.mlir \
--dataset=./WIDER \
--input_num=100 \
-o mnet_calibration_table

model_deploy.py \
--mlir mnet.mlir \
--calibration_table mnet_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--customization_format RGB_PLANAR \
--test_input ./parade.jpg \
--test_reference mnet_top_output.npz \
--fuse_preprocess \
--excepts data \
--tolerance 0.8,0.5 \
--model retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel

## arcface
model_transform.py \
--model_name arcface_res50 \
--model_def ./arcface_res50.prototxt \
--model_data ./arcface_res50.caffemodel \
--test_input ./Aaron_Eckhart_0001.jpg \
--test_result arcface_res50_top_output.npz \
--input_shapes [[1,3,112,112]]
--resize_dims 112,112 \
--mean 127.5,127.5,127.5 \
--scale 0.0078125,0.0078125,0.0078125 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--excepts data \
--mlir arcface_res50.mlir

run_calibration.py \
arcface_res50.mlir \
--dataset=./LFW \
--input_num=100 \
-o arcface_res50_calibration_table

model_deploy.py \
--mlir arcface_res50.mlir \
--calibration_table arcface_res50_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--customization_format RGB_PLANAR \
--test_input ./Aaron_Eckhart_0001.jpg \
--test_reference arcface_res50_top_output.npz \
--fuse_preprocess \
--excepts data \
--tolerance 0.9,0.6 \
--model arcface_res50_fused_preprocess.cvimodel
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
cp $MLIR_PATH/tpuc/regression/data/Aaron_Eckhart_0001.jpg .
cp -rf $MLIR_PATH/tpuc/regression/data/images .

## retinaface fuse_post_process
model_transform.py \
--model_type caffe \
--model_name mnet \
--model_def ./mnet_600_with_detection.prototxt \
--model_data ./mnet.caffemodel \
--image ./parade.jpg \
--image_resize_dims 600,600 \
--model_channel_order "rgb" \
--tolerance 0.99,0.99,0.99 \
--mlir mnet_416_fp32.mlir

run_calibration.py \
mnet_416_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o retinaface_mnet25_calibration_table

model_deploy.py \
--model_name mnet \
--mlir mnet_416_fp32.mlir \
--calibration_table retinaface_mnet25_calibration_table \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--aligned_input false \
--chip cv183x \
--quantize INT8 \
--image parade.jpg \
--tolerance 0.90,0.85,0.54 \
--correctness 0.95,0.95,0.9 \
--cvimodel retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel

## arcface
model_transform.py \
--model_type caffe \
--model_name mnet \
--model_def ./arcface_res50.prototxt \
--model_data ./arcface_res50.caffemodel \
--image ./Aaron_Eckhart_0001.jpg \
--image_resize_dims 112,112 \
--input_scale 0.0078125 \
--mean 127.5,127.5,127.5 \
--model_channel_order "rgb" \
--tolerance 0.99,0.99,0.99 \
--mlir arcface_res50_fp32.mlir

run_calibration.py \
arcface_res50_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o arcface_res50_calibration_table

model_deploy.py \
--model_name arcface_res50 \
--mlir arcface_res50_fp32.mlir \
--calibration_table arcface_res50_calibration_table \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--aligned_input false \
--chip cv183x \
--quantize INT8 \
--image pose_256_192.jpg \
--excepts stage1_unit1_sc_scale \
--tolerance 0.6,0.6,0 \
--correctness 0.95,0.95,0.9 \
--cvimodel arcface_res50_fused_preprocess.cvimodel
```

Copy generated retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel and arcface_res50_fused_preprocess.cvimodel to Development board

## How To Compile Vpss input Sample In Docker
View the Top level directory README.md

## Run Samples In EVB Borad
```
cd install_samples/samples_extra
## test sample people
./bin/cvi_sample_fd_fr_fused_preprocess \
retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
arcface_res50_fused_preprocess.cvimodel \
./data/obama1.jpg \
./data/obama2.jpg

## test different people
./bin/cvi_sample_fd_fr_fused_preprocess \
retinaface_mnet25_600_fused_preprocess_with_detection.cvimodel \
arcface_res50_fused_preprocess.cvimodel \
./data/obama1.jpg \
./data/trump1.jpg
```