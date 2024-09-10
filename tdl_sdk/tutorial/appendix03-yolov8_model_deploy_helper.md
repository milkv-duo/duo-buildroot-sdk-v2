# 通用yolov8部署

## onnx模型导出

调整yolov8输出分支，去掉`forward`函数的解码部分，并将三个不同的feature map的box以及cls分开，得到6个分支

```python
from ultralytics import YOLO
import types

input_size = (640, 640)

def forward2(self, x):
    x_reg = [self.cv2[i](x[i]) for i in range(self.nl)]
    x_cls = [self.cv3[i](x[i]) for i in range(self.nl)]
    return x_reg + x_cls


model_path = "/home/peng.mi/codes/yolov8/weights/yolov8s.pt"
model = YOLO(model_path)
model.model.model[-1].forward = types.MethodType(forward2, model.model.model[-1])
model.export(format='onnx', opset=11, imgsz=input_size)
```

## cvimodel导出

需要tpu-mlir v1.2.30-g3bca0299-20230615

参考下列脚本转换cvimodel，调整对应路径以及模型名称

```shell
# source tpu_mlir_1_2/envsetup.sh

# setup
# change setting to your model and path
##############################################

processor="cv181x"

model_name="yolov8n"
version_name="float"
output_layer="fp32"
root=${model_name}
img_dir="../model_yolov5n_onnx/COCO2017"
img="../model_yolov5n_onnx/image/dog.jpg"

mlir="${root}/mlir/${version_name}_fp32.mlir"
table="${root}/calibration_table/${version_name}_cali_table"
cvimodel="${root}/int8/${model_name}_${output_layer}_${processor}.cvimodel"

model_onnx="${root}/onnx/${model_name}.onnx"
in_npz="${model_name}_in_f32.npz"
out_npz="${root}/npz/${model_name}_top_outputs.npz"

mkdir "${root}/mlir"
mkdir "${root}/calibration_table"
mkdir "${root}/int8"
mkdir "${root}/npz"


# mlir step
#################################
if [ $1 = 1 -o $1 = "all" ] ; then

    model_transform.py \
    --model_name ${model_name} \
    --model_def ${model_onnx} \
    --input_shapes [[1,3,640,640]] \
    --mean 0.0,0.0,0.0 \
    --scale 0.0039216,0.0039216,0.0039216 \
    --keep_aspect_ratio \
    --pixel_format rgb \
    --test_input ${img} \
    --test_result ${out_npz} \
    --mlir ${mlir}
fi

# calibration_table step
################################
if [ $1 = 2 -o $1 = "all" ] ; then

    run_calibration.py ${mlir} \
    --dataset ${img_dir} \
    --input_num 100 \
    -o ${table}

fi

if [ $1 = 3 -o $1 = "all" ] ; then

    model_deploy.py \
    --mlir ${mlir} \
    --quant_input \
    --quant_output \
    --quantize INT8 \
    --calibration_table ${table} \
    --chip ${processor} \
    --test_input ${in_npz} \
    --test_reference ${out_npz} \
    --tolerance 0.85,0.45 \
    --model ${cvimodel}

fi
```

## yolov8接口调用

首先创建一个`cvitdl_handle`，然后打开对应的`cvimodel`，在运行推理接口之前，可以设置自己模型的两个阈值

* `CVI_TDL_SetModelThreshold` 设置conf阈值
* `CVI_TDL_SetModelNmsThreshold` 设置nms阈值

最终推理的结果通过解析`cvtdl_object_t.info`获取

```c++
// create handle
cvitdl_handle_t tdl_handle = NULL;
ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

// read image
VIDEO_FRAME_INFO_S bg;
ret = CVI_TDL_ReadImage(strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);

// open model and set conf & nms threshold
ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, path_to_model);
CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.001);
CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.7);
if (ret != CVI_SUCCESS) {
	printf("open model failed with %#x!\n", ret);
    return ret;
}

// start infer
cvtdl_object_t obj_meta = {0};
CVI_TDL_Detection(tdl_handle, &bg,CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, &obj_meta);

// analysis result
std::stringstream ss;
ss << "boxes=[";
for (uint32_t i = 0; i < obj_meta.size; i++) {
ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
   << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
   << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << "],";
}
```

## 测试结果

* conf: 0.001
* nms_thresh: 0.6

yolov8n: 37.3

yolov8s: 44.9

| 模型          | 推理时间(ms) | ion(MB) | map     |
| ------------- | ------------ | ------- | ------- |
| yolov8n(int8) | 45.63        | 7.54    | 35.8433 |
| yolov8n(fp32) | 47.19        | 11.20   | 35.8387 |
| yolov8s(int8) | 135.57       | 18.26   | 43.5708 |
| yolov8s(fp32) | 137.13       | 21.73   | 43.5712 |
