# Yolov5s Sample with post_process

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
--model_name yolov5s \
--model_def ./yolov5s.onnx \
--test_input ./dog.jpg \
--test_result yolov5s_top_output.npz \
--input_shapes [[1,3,640,640]] \
--output_names 326,474,622
--resize_dims 640,640 \
--mean 0,0,0 \
--scale 0.00392,0.00392,0.00392 \
--pixel_format "rgb" \
--tolerance 0.99,0.99 \
--mlir yolov5s.mlir

run_calibration.py \
yolov5s.mlir \
--dataset=./COCO2017 \
--input_num=100 \
-o yolov5s_calibration_table

model_deploy.py \
--mlir yolov5s.mlir \
--calibration_table yolov5s_calibration_table \
--chip cv183x \
--quantize INT8 \
--quant_input \
--test_input ./dog.jpg \
--test_reference yolov5s_top_output.npz \
--tolerance 0.9,0.6 \
--fuse_preprocess \
--model yolov5s_fused_preprocess.cvimodel
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
  --model_type onnx \
  --model_name yolov5s \
  --model_def ./yolov5s_new.onnx \
  --image ./dog.jpg \
  --image_resize_dims 640,640 \
  --keep_aspect_ratio true \
  --raw_scale 1.0 \
  --model_channel_order "rgb" \
  --tolerance 0.99,0.99,0.99 \
  --mlir yolov5s_fp32.mlir

run_calibration.py \
yolov5s_fp32.mlir \
--dataset=./images \
--input_num=100 \
-o yolov5s_calibration_table

model_deploy.py \
--model_name yolov5s \
--mlir yolov5s_fp32.mlir \
--calibration_table yolov5s_calibration_table \
--fuse_preprocess \
--pixel_format RGB_PLANAR \
--aligned_input false \
--excepts output \
--chip cv183x \
--quantize INT8 \
--image ./dog.jpg \
--tolerance 0.9,0.9,0.5 \
--correctness 0.95,0.95,0.9 \
--cvimodel yolov5s_fused_preprocess.cvimodel
```

Copy generated yolov5s_fused_preprocess.cvimodel to EVB board

## How To Compile Vpss input Sample In Docker
View the Top level directory README.md

## Run Samples In EVB Borad
```
cd install_samples/samples_extra
./bin/cvi_sample_detector_yolo_v5_fused_preprocess \
./yolov5s_fused_preprocess.cvimodel \
./data/dog.jpg \
yolo_v5_out.jpg
```