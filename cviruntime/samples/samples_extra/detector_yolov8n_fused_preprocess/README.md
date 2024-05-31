# PPYOLOE_M Sample with post_process

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

assign_output.py --model yolov8n.onnx --output /model.22/dfl/conv/Conv,/model.22/Sigmoid

model_transform.py \
	--model_name yolov8n \
	--model_def yolov8n_new.onnx \
	--input_shapes [[1,3,640,640]] \
	--keep_aspect_ratio \
	--pixel_format "rgb" \
	--mean 0,0,0 \
	--scale 0.0039216,0.0039216,0.0039216 \
	--test_input dog.jpg \
	--test_result yolov8n_top_outputs.npz \
	--mlir yolov8n.mlir

run_calibration.py yolov8n.mlir \
	--dataset ./COCO2017 \
	--input_num 100 \
	-o yolov8n_cali_table

model_deploy.py \
	--mlir yolov8n.mlir \
	--quantize INT8 \
	--calibration_table yolov8n_cali_table \
	--chip cv183x \
	--test_input dog.jpg \
	--test_reference yolov8n_top_outputs.npz \
	--compare_all \
	--tolerance 0.94,0.67 \
	--fuse_preprocess \
	--model yolov8n_int8_fuse_preprocess.cvimodel
```
#### For old toolchain guide
The following documents are required:

* cvitek_mlir_ubuntu-18.04.tar.gz

Transform model shell:

```shell
tar zxf cvitek_mlir_ubuntu-18.04.tar.gz
source cvitek_mlir/cvitek_envs.sh

mkdir workspace && cd workspace
cp $MLIR_PATH/tpuc/regression/data/dog.jpg .

assign_output.py --model yolov8n.onnx --output /model.22/dfl/conv/Conv,/model.22/Sigmoid

model_transform.py \ 
--model_type onnx \ 
--model_name yolov8n \
--model_def yolov8n_new.onnx \
--keep_aspect_ratio True \
--image dog.jpg \
--image_resize_dims 640,640 \
--net_input_dims 640,640 \
--raw_scale 1.0 \
--mean 0.0,0.0,0.0 \
--std 1.0,1.0,1.0 \
--input_scale 1.0 \
--pixel_format RGB_PLANAR \
--model_channel_order "rgb" \
--tolerance 0.99,0.99,0.99 \
--mlir yolov8n.mlir

run_calibration.py yolov8n.mlir \
	--dataset /data/dataset/coco/val2017 \
	--input_num 100 \
	-o yolov8n_cali_table

model_deploy.py \
--model_name yolov8n \
--mlir yolov8n.mlir \
--calibration_table yolov8n_cali_table \
--quantize int8 \
--tolerance 0.94,0.94,0.67 \
--chip cv183x \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--image dog.jpg \
--cvimodel yolov8n_int8_fuse_preprocess.cvimodel
```


## How To Compile Vpss input Sample In Docker

View the Top level directory README.md

## Run Samples In EVB Borad

```shell
cd install_samples/samples_extra
./bin/cvi_sample_detector_yolov8n_fused_preprocess \
./yolov8n_int8_fuse_preprocess.cvimodel \
./data/dog.jpg \
yolov8n_out.jpg
```