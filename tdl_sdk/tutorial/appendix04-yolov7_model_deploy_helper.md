# 通用yolov7部署

## onnx模型导出

下载官方[yolov7](https://github.com/WongKinYiu/yolov7)仓库代码

```shell
git clone https://github.com/WongKinYiu/yolov7.git
```

在上述下载代码的目录中新建一个文件夹`weights`，然后将需要导出onnx的模型移动到yolov7/weights

```shell
cd yolov7 & mkdir weights
cp path/to/onnx ./weights/
```

在`yolov7/`目录下新建一个文件`onnx_export.py`，添加如下代码

```python
import torch
import torch.nn as nn
import onnx
import models
from models.common import Conv
from utils.activations import Hardswish, SiLU
import types

pt_path = "path/to/yolov7-tiny.pt"
save_path = pt_path.replace(".pt", ".onnx")

ckpt = torch.load(pt_path, map_location="cpu")
model = ckpt["model"].float().fuse().eval()

# Compatibility updates
for m in model.modules():
    if type(m) in [nn.Hardswish, nn.LeakyReLU, nn.ReLU, nn.ReLU6, nn.SiLU]:
        m.inplace = True  # pytorch 1.7.0 compatibility
    elif type(m) is nn.Upsample:
        m.recompute_scale_factor = None  # torch 1.11.0 compatibility
    elif type(m) is Conv:
        m._non_persistent_buffers_set = set()

# Update model
    for k, m in model.named_modules():
        m._non_persistent_buffers_set = set()  # pytorch 1.6.0 compatibility
        if isinstance(m, models.common.Conv):  # assign export-friendly activations
            if isinstance(m.act, nn.Hardswish):
                m.act = Hardswish()
            elif isinstance(m.act, nn.SiLU):
                m.act = SiLU()

def forward(self, x):
        # x = x.copy()  # for profiling
        z = []  # inference output

        for i in range(self.nl):
            x[i] = self.m[i](x[i])  # conv

            bs, _, ny, nx = x[i].shape  # x(bs,255,20,20) to x(bs,3,20,20,85)
            x[i] = x[i].view(bs, self.na, self.no, ny, nx).permute(0, 1, 3, 4, 2).contiguous()
            
            xywh, conf, score = x[i].split((4, 1, self.nc), 4)
            
            z.append(xywh[0])
            z.append(conf[0])
            z.append(score[0])
            
        return z


model.model[-1].forward = types.MethodType(forward, model.model[-1])
img = torch.zeros(1, 3, 640, 640)
torch.onnx.export(model, img, save_path, verbose=False,
                  opset_version=12, input_names=['images'])
```

## cvimodel导出

cvimodel转换操作可以参考[appendix02-yolov5_model_deploy_helper](./appendix02-yolov5_model_deploy_helper.md)

## 接口说明

yolov7模型与yolov5模型检测与解码过程基本类似，因此可以直接使用yolov5的接口，可以直接参考[appendix02-yolov5_model_deploy_helper](./appendix02-yolov5_model_deploy_helper.md)的接口说明

> **注意修改anchors为yolov7的anchors!!!**
>
> ```
> anchors:
>  - [12,16, 19,36, 40,28]  *# P3/8*
>  - [36,75, 76,55, 72,146]  *# P4/16*
>  - [142,110, 192,243, 459,401]  *# P5/32*
> ```

## 测试结果

测试了yolov7-tiny模型各个版本的指标，测试数据为COCO2017，其中阈值设置为：

* conf_threshold: 0.001
* nms_threshold: 0.65

分辨率均为640 x 640

|    模型     |  部署版本  | 测试平台 | 推理耗时 (ms) | 带宽 (MB) | ION(MB) | MAP 0.5  | MAP 0.5-0.95 |                   备注                   |
| :---------: | :--------: | :------: | :-----------: | :-------: | :-----: | :------: | :----------: | :--------------------------------------: |
| yolov7-tiny |  官方导出  | pytorch  |      N/A      |    N/A    |   N/A   |   56.7   |     38.7     |           pytorch官方fp32指标            |
|             |  官方导出  |  cv181x  |     75.4      |   85.31   |  17.54  | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|             |  官方导出  |  cv182x  |     56.6      |   85.31   |  17.54  | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|             |  官方导出  |  cv183x  |     21.85     |   71.46   |  16.15  | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|             | TDL_SDK导出 |   onnx   |      N/A      |    N/A    |   N/A   | 53.7094  |    36.438    |            TDL_SDK导出onnx指标            |
|             | TDL_SDK导出 |  cv181x  |     70.41     |   70.66   |  15.43  | 53.3681  |   32.6277    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|             | TDL_SDK导出 |  cv182x  |     52.01     |   70.66   |  15.43  | 53.3681  |   32.6277    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|             | TDL_SDK导出 |  cv183x  |     18.95     |   55.86   |  14.05  | 53.3681  |   32.6277    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |