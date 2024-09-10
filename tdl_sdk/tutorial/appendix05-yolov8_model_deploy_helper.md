# 通用yolov8部署

首先获取yolov8官方仓库代码[ultralytics/ultralytics: NEW - YOLOv8 🚀 in PyTorch > ONNX > OpenVINO > CoreML > TFLite (github.com)](https://github.com/ultralytics/ultralytics)

```shell
git clone https://github.com/ultralytics/ultralytics.git
```

再下载对应的yolov8模型文件，以[yolov8n](https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.pt)为例，然后将下载的yolov8n.pt放在`ultralytics/weights/`目录下，如下命令行所示

```
cd ultralytics & mkdir weights
cd weights
wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.pt
```

## onnx模型导出

调整yolov8输出分支，去掉`forward`函数的解码部分，并将三个不同的feature map的box以及cls分开，得到6个分支

具体的可以在`ultralytics/`目录下新建一个文件，并贴上下列代码

```python
from ultralytics import YOLO
import types

input_size = (640, 640)

def forward2(self, x):
    x_reg = [self.cv2[i](x[i]) for i in range(self.nl)]
    x_cls = [self.cv3[i](x[i]) for i in range(self.nl)]
    return x_reg + x_cls


model_path = "./weights/yolov8s.pt"
model = YOLO(model_path)
model.model.model[-1].forward = types.MethodType(forward2, model.model.model[-1])
model.export(format='onnx', opset=11, imgsz=input_size)
```

运行上述代码之后，可以在`./weights/`目录下得到`yolov8n.onnx`文件，之后就是将`onnx`模型转换为cvimodel模型

## cvimodel导出

cvimodel转换操作可以参考[appendix02-yolov5_model_deploy_helper](./appendix02-yolov5_model_deploy_helper.md)

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
CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
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

转换测试了官网的yolov8n以及yolov8s模型，在COCO2017数据集上进行了测试，其中阈值设置为：

* conf: 0.001
* nms_thresh: 0.6

所有分辨率均为640 x 640

|  模型   |  部署版本  | 测试平台 | 推理耗时 (ms) | 带宽 (MB) | ION(MB) | MAP 0.5  | MAP 0.5-0.95 |                   备注                   |
| :-----: | :--------: | :------: | :-----------: | :-------: | :-----: | :------: | :----------: | :--------------------------------------: |
| yolov8n |  官方导出  | pytorch  |      N/A      |    N/A    |   N/A   |    53    |     37.3     |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |     54.91     |   44.16   |  8.64   | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |     40.21     |   44.32   |  8.62   | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |     17.81     |   40.46   |   8.3   | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |    N/A    |   N/A   |  51.32   |   36.4577    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |     45.62     |   31.56   |  7.54   | 51.2207  |   35.8048    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |     32.8      |   32.8    |  7.72   | 51.2207  |   35.8048    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     12.61     |   28.64   |  7.53   | 51.2207  |   35.8048    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |
| yolov8s |  官方导出  | pytorch  |      N/A      |    N/A    |   N/A   |   61.8   |     44.9     |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |    144.72     |  101.75   |  17.99  | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |      103      |  101.75   |  17.99  | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |     38.04     |   38.04   |  16.99  | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |    N/A    |   N/A   | 60.1534  |    44.034    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |    135.55     |   89.53   |  18.26  | 60.2784  |   43.4908    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |     95.95     |   89.53   |  18.26  | 60.2784  |   43.4908    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     32.88     |   58.44   |  16.9   | 60.2784  |   43.4908    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |