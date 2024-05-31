# Sample with Multiple batch fuse cvimodel 

### Download the mobilev2 model and convert the model under docker (optional)
Model could clone from:https://github.com/shicai/MobileNet-Caffe

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
--model tmp_model_bs1.cvimodel


model_transform.py \
--model_name mobilenet_v2 \
--model_def ./mobilenet_v2_deploy.prototxt \
--model_data ./mobilenet_v2.caffemodel \
--test_input ./cat.jpg \
--test_result mobilenet_v2_top_output.npz \
--input_shapes [[4,3,224,224]]
--resize_dims 256,256 \
--mean 103.94,116.78,123.68 \
--scale 0.017,0.017,0.017 \
--pixel_format "bgr" \
--tolerance 0.99,0.99 \
--excepts prob \
--mlir mobilenet_v2.mlir

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
--model tmp_model_bs4.cvimodel

model_tool --combine tmp_model_bs1.cvimodel tmp_model_bs4.cvimodel -o mobilenet_v2_bs1_bs4.cvimodel

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
cp -rf $MLIR_PATH/tpuc/regression/data/images .

model_transform.py \
--model_type caffe \
--model_name mobilenet_v2 \
--model_def mobilenet_v2_deploy.prototxt \
--model_data mobilenet_v2.caffemodel \
--image cat.jpg \
--image_resize_dims 256,256 \
--net_input_dims 224,224 \
--mean 103.94,116.78,123.68 \
--input_scale 0.017 \
--model_channel_order bgr \
--tolerance 0.999,0.999,0.998 \
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
--calibration_table mobilenet_v2_calibration_table_1000 \
--chip cv183x \
--image cat.jpg \
--merge_weight \
--tolerance 0.94,0.94,0.66 \
--excepts prob \
--correctness 0.99,0.99,0.99 \
--cvimodel tmp_model_bs1.cvimodel

model_transform.py \
--model_type caffe \
--model_name mobilenet_v2 \
--model_def mobilenet_v2_deploy.prototxt \
--model_data mobilenet_v2.caffemodel \
--image cat.jpg \
--image_resize_dims 256,256 \
--net_input_dims 224,224 \
--mean 103.94,116.78,123.68 \
--input_scale 0.017 \
--model_channel_order bgr \
--batch_size 4 \
--tolerance 0.999,0.999,0.998 \
--excepts prob \
--mlir mobilenet_v2_fp32.mlir

model_deploy.py \
--model_name mobilenet_v2 \
--mlir mobilenet_v2_fp32.mlir \
--calibration_table mobilenet_v2_calibration_table_1000 \
--chip cv183x \
--image cat.jpg \
--merge_weight \
--tolerance 0.94,0.94,0.66 \
--excepts prob \
--correctness 0.99,0.99,0.99 \
--cvimodel tmp_model_bs4.cvimodel

cvimodel_tool \
-a merge \
-i tmp_model_bs1.cvimodel tmp_model_bs4.cvimodel \
-o mobilenet_v2_bs1_bs4.cvimodel
```

Copy generated mobilenet_v2_bs1_bs4.cvimodel to Development board


## How To Compile Sample In Docker
View the Top level directory README.md or View the cvitek_tpu_quick_start_guide.md

## Run Samples In EVB Borad
```
cd install_samples
./bin/cvi_sample_classifier_multi_batch \
./mobilenet_v2_bs1_bs4.cvimodel \
./data/cat.jpg \
./data/synset_words.txt
```
