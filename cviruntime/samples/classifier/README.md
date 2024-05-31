# Mobilev2 Sample without fuse proprocess and quant to int8

Copy Unzipped mobilenet_v2.cvimodel to EVB board

### Download the model and convert the model under docker (optional)
Mobilev2 model could clone from:https://github.com/shicai/MobileNet-Caffe

#### For new toolchain guide
The following documents are required:
* tpu-mlir_xxxx.tar.gz (The release package of tpu-mlir)

Transform cvimodel shell:
``` shell
tar zxf tpu-mlir_xxxx.tar.gz
source tpu-mlir_xxxx/envsetup.sh

mkdir workspace && cd workspace
cp $TPUC_ROOT/regression/image/cat.jpg .
cp -rf $TPUC_ROOT/regression/dataset/ILSVRC2012 .

model_transform.py \
--model_name mobilenet_v2 \
--model_def ./mobilenet_v2_deploy.prototxt \
--model_data ./mobilenet_v2.caffemodel \
--test_input ./cat.jpg \
--test_result mobilenet_v2_top_output.npz \
--input_shapes [[1,3,224,224]]
--resize_dims 256,256 \
--mean 103.94,116.78,123.68 \
--scale 0.017,0.017,0.017 \
--pixel_format "bgr" \
--tolerance 0.99,0.99 \
--excepts prob \
--mlir mobilenet_v2.mlir

run_calibration.py \
mobilenet_v2.mlir \
--dataset=./ILSVRC2012 \
--input_num=100 \
-o mobilenet_v2_calibration_table

model_deploy.py \
--mlir mobilenet_v2.mlir \
--calibration_table mobilenet_v2_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--test_input mobilenet_v2_in_f32.npz \
--test_reference mobilenet_v2_top_output.npz \
--excepts prob \
--tolerance 0.9,0.6 \
--model mobilenet_v2.cvimodel
```



#### For old toolchain guide
The following documents are required:

* cvitek_mlir_ubuntu-18.04.tar.gz

Transform cvimodel shell:
``` shell
tar zxf cvitek_mlir_ubuntu-18.04.tar.gz
source cvitek_mlir/cvitek_envs.sh

mkdir workspace && cd workspace
cp $MLIR_PATH/tpuc/regression/data/cat.jpg .
cp -rf $MLIR_PATH/tpuc/regression/data/images .

model_transform.py \
--model_type caffe \
--model_name mobilenet_v2 \
--model_def ./mobilenet_v2_deploy.prototxt \
--model_data ./mobilenet_v2.caffemodel \
--image ./cat.jpg \
--image_resize_dims 256,256 \
--net_input_dims 224,224 \
--mean 103.94,116.78,123.68 \
--input_scale 0.017 \
--model_channel_order "bgr" \
--tolerance 0.99,0.99,0.99 \
--excepts prob \
--mlir mobilenet_v2_fp32.mlir

run_calibration.py \
mobilenet_v2_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o mobilenet_v2_calibration_table

model_deploy.py \
--model_name mobilenet_v2 \
--mlir mobilenet_v2_fp32.mlir \
--calibration_table mobilenet_v2_calibration_table \
--chip cv183x \
--quantize INT8 \
--image cat.jpg \
--excepts prob \
--tolerance 0.9,0.9,0.6 \
--correctness 0.95,0.95,0.9 \
--cvimodel mobilenet_v2.cvimodel
```
Copy generated mobilenet_v2.cvimodel to EVB board


## How To Compile Sample In Docker
View the Top level directory README.md or View the cvitek_tpu_quick_start_guide.md

## Run Samples In EVB Borad
```
cd install_samples
./bin/cvi_sample_classifier \
./mobilenet_v2.cvimodel \
./data/cat.jpg \
./data/synset_words.txt
```