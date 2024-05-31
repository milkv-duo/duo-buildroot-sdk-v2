# Mobilev2 Sample without fuse proprocess and quant to BF16

### Download model and convert the model under docker (optional)
Mobilev2 Model could clone from:https://github.com/shicai/MobileNet-Caffe

#### For new toolchain guide
The following documents are required:
* tpu-mlir_xxxx.tar.gz (The release package of tpu-mlir)

Transform cvimodel shell:
``` shell
tar zxf tpu-mlir_xxxx.tar.gz
source tpu-mlir_xxxx/envsetup.sh

mkdir workspace && cd workspace
cp $TPUC_ROOT/regression/image/cat.jpg .

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


model_deploy.py \
--mlir mobilenet_v2.mlir \
--chip cv183x \
--quantize BF16 \
--test_input mobilenet_v2_in_f32.npz \
--test_reference mobilenet_v2_top_output.npz \
--excepts prob \
--tolerance 0.94,0.61 \
--model mobilenet_v2_bf16.cvimodel
```


#### For old toolchain guide
The following documents are required:

* cvitek_mlir_ubuntu-18.04.tar.gz

Transform model shell:
``` shell
tar zxf cvitek_mlir_ubuntu-18.04.tar.gz
source cvitek_mlir/cvitek_envs.sh

mkdir workspace && cd workspace
cp $MLIR_PATH/tpuc/regression/data/cat.jpg .

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

model_deploy.py \
--model_name mobilenet_v2 \
--mlir mobilenet_v2_fp32.mlir \
--quantize BF16 \
--chip cv183x \
--image cat.jpg \
--excepts prob \
--tolerance 0.94,0.94,0.61 \
--correctness 0.99,0.99,0.96 \
--cvimodel mobilenet_v2_bf16.cvimodel
```

Copy generated mobilenet_v2_bf16.cvimodel to EVB board

## How To Compile Sample In Docker
View the Top level directory README.md or View the cvitek_tpu_quick_start_guide.md

## Run Samples In EVB Borad
```
cd install_samples
./bin/cvi_sample_classifier_bf16 \
./mobilenet_v2_bf16.cvimodel \
./data/cat.jpg \
./data/synset_words.txt
```
