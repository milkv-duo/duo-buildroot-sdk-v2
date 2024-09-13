# 通用yolov6部署

## onnx模型导出

下载yolov6官方仓库 [meituan/YOLOv6](https://github.com/meituan/YOLOv6)，下载yolov6权重文件，在yolov6文件夹下新建一个目录`weights`，并将下载的权重文件放在目录`yolov6-main/weights/`下

修改`yolov6-main/deploy/export_onnx.py`文件，然后添加一个函数

```python
def detect_forward(self, x):
    
    final_output_list = []
    for i in range(self.nl):
        b, _, h, w = x[i].shape
        l = h * w
        x[i] = self.stems[i](x[i])
        cls_x = x[i]
        reg_x = x[i]
        cls_feat = self.cls_convs[i](cls_x)
        cls_output = self.cls_preds[i](cls_feat)
        reg_feat = self.reg_convs[i](reg_x)
        reg_output_lrtb = self.reg_preds[i](reg_feat)

        final_output_list.append(cls_output.permute(0, 2, 3, 1))
        final_output_list.append(reg_output_lrtb.permute(0, 2, 3, 1))

    return final_output_list
```

然后使用动态绑定的方式修改yolov6模型的`forward`，需要先`import types`，然后在`onnx export`之前添加下列代码

```python
...
print("===================")
print(model)
print("===================")

# 动态绑定修改模型detect的forward函数
model.detect.forward = types.MethodType(detect_forward, model.detect)

y = model(img)  # dry run

# ONNX export
try:
...
```

然后在`yolov6-main/`目录下输入如下命令，其中：

* `weights` 为pytorch模型文件的路径
* `img` 为模型输入尺寸
* `batch` 模型输入的batch
* `simplify` 简化onnx模型

```shell
python ./deploy/ONNX/export_onnx.py \
    --weights ./weights/yolov6n.pt \
    --img 640 \
    --batch 1
```

然后得到onnx模型

## cvimodel模型导出

cvimodel转换操作可以参考[appendix02-yolov5_model_deploy_helper](./appendix02-yolov5_model_deploy_helper.md)

## yolov6接口说明

提供预处理参数以及算法参数设置，其中参数设置：

* `InputPreParam `输入预处理设置

  $y=(x-mean)\times factor$

  * factor 预处理方差的倒数
  * mean 预处理均值
  * format 图片格式

* `cvtdl_det_algo_param_t`

  * cls 设置yolov6模型的分类

> yolov6是anchor-free的目标检测网络，不需要传入anchor

另外是yolov6的两个参数设置：

* `CVI_TDL_SetModelThreshold ` 设置置信度阈值，默认为0.5
* `CVI_TDL_SetModelNmsThreshold` 设置nms阈值，默认为0.5

```c++
// setup preprocess
InputPreParam p_preprocess_cfg;

for (int i = 0; i < 3; i++) {
    printf("asign val %d \n", i);
    p_preprocess_cfg.factor[i] = 0.003922;
    p_preprocess_cfg.mean[i] = 0.0;
}
p_preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

printf("start yolov algorithm config \n");
// setup yolov6 param
cvtdl_det_algo_param_t p_yolov6_param;
p_yolov6_param.cls = 80;

ret = CVI_TDL_Set_YOLOV6_Param(tdl_handle, &p_preprocess_cfg, &p_yolov6_param);
printf("yolov6 set param success!\n");
if (ret != CVI_SUCCESS) {
    printf("Can not set Yolov6 parameters %#x\n", ret);
    return ret;
}

ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, model_path.c_str());
if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
}
// set thershold
CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);
CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);

CVI_TDL_Yolov6(tdl_handle, &fdFrame, &obj_meta);

for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", 
           obj_meta.info[i].bbox.x1,
           obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, 
           obj_meta.info[i].bbox.y2, 
           obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }
```

## 测试结果

转换了yolov6官方仓库给出的yolov6n以及yolov6s，测试数据集为COCO2017

其中阈值参数设置为：

* conf_threshold: 0.03
* nms_threshold: 0.65

分辨率均为640x640

|  模型   |  部署版本  | 测试平台 | 推理耗时 (ms) |  带宽 (MB)  | ION(MB)  |   MAP 0.5   | MAP 0.5-0.95 |                   备注                   |
| :-----: | :--------: | :------: | :-----------: | :---------: | :------: | :---------: | :----------: | :--------------------------------------: |
| yolov6n |  官方导出  | pytorch  |      N/A      |     N/A     |   N/A    |    53.1     |     37.5     |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |  ion分配失败  | ion分配失败 |  11.58   |  量化失败   |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |     39.17     |    47.08    |  11.56   |  量化失败   |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |   量化失败    |  量化失败   | 量化失败 |  量化失败   |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |     N/A     |   N/A    |   51.6373   |   36.4384    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |     49.11     |    31.35    |   8.46   |   49.8226   |    34.284    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |     34.14     |    30.53    |   8.45   |   49.8226   |    34.284    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     10.89     |    21.22    |   8.49   |   49.8226   |    34.284    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |
| yolov6s |  官方导出  | pytorch  |      N/A      |     N/A     |   N/A    |    61.8     |      45      |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |  ion分配失败  | ion分配失败 |  27.56   |  量化失败   |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |     131.1     |   115.81    |  27.56   |  量化失败   |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |   量化失败    |  量化失败   | 量化失败 |  量化失败   |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |     N/A     |   N/A    |   60.1657   |   43.5878    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |  ion分配失败  | ion分配失败 |  25.33   | ion分配失败 | ion分配失败  |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |    126.04     |    99.16    |  25.32   |   56.2774   |   40.0781    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     38.55     |    57.26    |  23.59   |   56.2774   |   40.0781    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |

