# 通用YOLOX部署cv181x

首先可以在github下载yolox的官方代码：[Megvii-BaseDetection/YOLOX: YOLOX is a high-performance anchor-free YOLO, exceeding yolov3~v5 with MegEngine, ONNX, TensorRT, ncnn, and OpenVINO supported. Documentation: https://yolox.readthedocs.io/ (github.com)](https://github.com/Megvii-BaseDetection/YOLOX/tree/main)

使用以下命令从源代码安装YOLOX

```shell
git clone git@github.com:Megvii-BaseDetection/YOLOX.git
cd YOLOX
pip3 install -v -e .  # or  python3 setup.py develop
```

## onnx模型导出

需要切换到刚刚下载的YOLOX仓库路径，然后创建一个weights目录，将预训练好的`.pth`文件移动至此

```shell
cd YOLOX & mkdir weights
cp path/to/pth ./weigths/
```

### 官方导出onnx

切换到tools路径

```shell
cd tools
```

在onnx中解码的导出方式

```shell
python \
export_onnx.py \
--output-name ../weights/yolox_m_official.onnx \
-n yolox-m \
--no-onnxsim \
-c ../weights/yolox_m.pth \
--decode_in_inference
```

相关参数含义如下：

* `--output-name` 表示导出onnx模型的路径和名称
* `-n` 表示模型名，可以选择
  * yolox-s, m, l, x
  * yolo-nano
  * yolox-tiny
  * yolov3
* `-c` 表示预训练的`.pth`模型文件路径
* `--decode_in_inference` 表示是否在onnx中解码

### TDL_SDK版本导出onnx

为了保证量化的精度，需要将YOLOX解码的head分为三个不同的branch输出，而不是官方版本的合并输出

通过以下的脚本和命令导出三个不同branch的head：

在`YOLOX/tools/`目录下新建一个文件`export_onnx_tdl_sdk.py`，并贴上以下代码

```python
#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# Copyright (c) Megvii, Inc. and its affiliates.

import argparse
import os
from loguru import logger

import torch
from torch import nn

import sys
sys.path.append("..")

from yolox.exp import get_exp
from yolox.models.network_blocks import SiLU
from yolox.utils import replace_module
import types

def make_parser():
    parser = argparse.ArgumentParser("YOLOX onnx deploy")
    parser.add_argument(
        "--output-name", type=str, default="yolox.onnx", help="output name of models"
    )
    parser.add_argument(
        "--input", default="images", type=str, help="input node name of onnx model"
    )
    parser.add_argument(
        "--output", default="output", type=str, help="output node name of onnx model"
    )
    parser.add_argument(
        "-o", "--opset", default=11, type=int, help="onnx opset version"
    )
    parser.add_argument("--batch-size", type=int, default=1, help="batch size")
    parser.add_argument(
        "--dynamic", action="store_true", help="whether the input shape should be dynamic or not"
    )
    parser.add_argument("--no-onnxsim", action="store_true", help="use onnxsim or not")
    parser.add_argument(
        "-f",
        "--exp_file",
        default=None,
        type=str,
        help="experiment description file",
    )
    parser.add_argument("-expn", "--experiment-name", type=str, default=None)
    parser.add_argument("-n", "--name", type=str, default=None, help="model name")
    parser.add_argument("-c", "--ckpt", default=None, type=str, help="ckpt path")
    parser.add_argument(
        "opts",
        help="Modify config options using the command-line",
        default=None,
        nargs=argparse.REMAINDER,
    )
    parser.add_argument(
        "--decode_in_inference",
        action="store_true",
        help="decode in inference or not"
    )

    return parser

def forward(self, xin, labels=None, imgs=None):
        outputs = []
        origin_preds = []
        x_shifts = []
        y_shifts = []
        expanded_strides = []

        for k, (cls_conv, reg_conv, stride_this_level, x) in enumerate(
            zip(self.cls_convs, self.reg_convs, self.strides, xin)
        ):
            x = self.stems[k](x)
            cls_x = x
            reg_x = x

            cls_feat = cls_conv(cls_x)
            cls_output = self.cls_preds[k](cls_feat)

            reg_feat = reg_conv(reg_x)
            reg_output = self.reg_preds[k](reg_feat)
            obj_output = self.obj_preds[k](reg_feat)
            
            outputs.append(reg_output.permute(0, 2, 3, 1))
            outputs.append(obj_output.permute(0, 2, 3, 1))
            outputs.append(cls_output.permute(0, 2, 3, 1))
            
        return outputs

@logger.catch
def main():
    args = make_parser().parse_args()
    logger.info("args value: {}".format(args))
    exp = get_exp(args.exp_file, args.name)
    exp.merge(args.opts)

    if not args.experiment_name:
        args.experiment_name = exp.exp_name

    model = exp.get_model()
    if args.ckpt is None:
        file_name = os.path.join(exp.output_dir, args.experiment_name)
        ckpt_file = os.path.join(file_name, "best_ckpt.pth")
    else:
        ckpt_file = args.ckpt

    # load the model state dict
    ckpt = torch.load(ckpt_file, map_location="cpu")

    model.eval()
    if "model" in ckpt:
        ckpt = ckpt["model"]
    model.load_state_dict(ckpt)
    model = replace_module(model, nn.SiLU, SiLU)
    
    # replace official head forward function
    if not args.decode_in_inference:
        model.head.forward = types.MethodType(forward, model.head)
    
    model.head.decode_in_inference = args.decode_in_inference

    logger.info("loading checkpoint done.")
    dummy_input = torch.randn(args.batch_size, 3, exp.test_size[0], exp.test_size[1])

    torch.onnx._export(
        model,
        dummy_input,
        args.output_name,
        input_names=[args.input],
        output_names=[args.output],
        dynamic_axes={args.input: {0: 'batch'},
                      args.output: {0: 'batch'}} if args.dynamic else None,
        opset_version=args.opset,
    )
    logger.info("generated onnx model named {}".format(args.output_name))

    if not args.no_onnxsim:
        import onnx
        from onnxsim import simplify

        # use onnx-simplifier to reduce reduent model.
        onnx_model = onnx.load(args.output_name)
        model_simp, check = simplify(onnx_model)
        assert check, "Simplified ONNX model could not be validated"
        onnx.save(model_simp, args.output_name)
        logger.info("generated simplified onnx model named {}".format(args.output_name))


if __name__ == "__main__":
    main()

```

然后输入以下命令

```shell
python \
export_onnx_tdl_sdk.py \
--output-name ../weights/yolox_s_9_branch.onnx \
-n yolox-s \
--no-onnxsim \
-c ../weights/yolox_s.pth`
```

## cvimodel导出

参考[appendix02-yolov5_model_deploy_helper][./appendix02_yolov5_model_deploy_helper.md]

## yolox接口使用说明

### 预处理参数设置

预处理参数设置通过一个结构体传入设置参数

```c++
typedef struct {
  float factor[3];
  float mean[3];
  meta_rescale_type_e rescale_type;

  PIXEL_FORMAT_E format;
} InputPreParam;
```

而对于YOLOX，需要传入以下四个参数：

* `factor `预处理scale参数
* `mean `预处理均值参数
* `format` 图片格式，`PIXEL_FORMAT_RGB_888_PLANAR`

其中预处理factor以及mean的公式为
$$
y=(x-mean)\times scale
$$

### 算法参数设置

```c++
typedef struct {
  uint32_t cls;
} cvtdl_det_algo_param_t;
```

需要传入分类的数量，例如

```c++
cvtdl_det_algo_param_t p_yolo_param;
p_yolo_param.cls = 80;
```

另外的模型置信度参数设置以及NMS阈值设置如下所示：

```c++
CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, conf_threshold);
CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, nms_threshold);
```

其中`conf_threshold`为置信度阈值；`nms_threshold`为 nms 阈值

### 测试代码

```c++
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "core.hpp"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.hpp"

int main(int argc, char* argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }
  printf("start yolox preprocess config \n");
  // // setup preprocess
  InputPreParam p_preprocess_cfg;

  for (int i = 0; i < 3; i++) {
    p_preprocess_cfg.factor[i] = 1.0;
    p_preprocess_cfg.mean[i] = 0.0;
  }
  p_preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

  printf("start yolo algorithm config \n");
  // setup yolo param
  cvtdl_det_algo_param_t p_yolo_param;
  p_yolo_param.cls = 80;

  printf("setup yolox param \n");
  ret = CVI_TDL_Set_YOLOX_Param(tdl_handle, &p_preprocess_cfg, &p_yolo_param);
  printf("yolox set param success!\n");
  if (ret != CVI_SUCCESS) {
    printf("Can not set YoloX parameters %#x\n", ret);
    return ret;
  }

  std::string model_path = argv[1];
  std::string str_src_dir = argv[2];

  float conf_threshold = 0.5;
  float nms_threshold = 0.5;
  if (argc > 3) {
    conf_threshold = std::stof(argv[3]);
  }

  if (argc > 4) {
    nms_threshold = std::stof(argv[4]);
  }

  printf("start open cvimodel...\n");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }
  printf("cvimodel open success!\n");
  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, conf_threshold);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, nms_threshold);

  std::cout << "model opened:" << model_path << std::endl;

  VIDEO_FRAME_INFO_S fdFrame;
  ret = CVI_TDL_ReadImage(str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  std::cout << "CVI_TDL_ReadImage done!\n";

  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_YoloX(tdl_handle, &fdFrame, &obj_meta);

  printf("detect number: %d\n", obj_meta.size);
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }

  CVI_VPSS_ReleaseChnFrame(0, 0, &fdFrame);
  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}
```

## 测试结果

测试了yolox模型onnx以及在cv181x/2x/3x各个平台的性能指标，其中参数设置：

* conf: 0.001
* nms: 0.65
* 分辨率：640 x 640

|  模型   |  部署版本  | 测试平台 | 推理耗时 (ms) |  带宽 (MB)  | ION(MB)  | MAP 0.5  | MAP 0.5-0.95 |                   备注                   |
| :-----: | :--------: | :------: | :-----------: | :---------: | :------: | :------: | :----------: | :--------------------------------------: |
| yolox-s |  官方导出  | pytorch  |      N/A      |     N/A     |   N/A    |   59.3   |     40.5     |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |    131.95     |   104.46    |  16.43   | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |     95.75     |   104.85    |  16.41   | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |   量化失败    |  量化失败   | 量化失败 | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |     N/A     |   N/A    | 53.1767  |   36.4747    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |    127.91     |    95.44    |  16.24   | 52.4016  |   35.4241    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |     91.67     |    95.83    |  16.22   | 52.4016  |   35.4241    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     30.6      |    65.25    |  14.93   | 52.4016  |   35.4241    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |
| yolox-m |  官方导出  | pytorch  |      N/A      |     N/A     |   N/A    |   65.6   |     46.9     |           pytorch官方fp32指标            |
|         |  官方导出  |  cv181x  |  ion分配失败  | ion分配失败 |  39.18   | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|         |  官方导出  |  cv182x  |     246.1     |   306.31    |  39.16   | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|         |  官方导出  |  cv183x  |   量化失败    |  量化失败   | 量化失败 | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|         | TDL_SDK导出 |   onnx   |      N/A      |     N/A     |   N/A    | 59.9411  |   43.0057    |            TDL_SDK导出onnx指标            |
|         | TDL_SDK导出 |  cv181x  |  ion分配失败  | ion分配失败 |  38.95   | 59.3559  |   42.1688    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|         | TDL_SDK导出 |  cv182x  |     297.5     |   242.65    |  38.93   | 59.3559  |   42.1688    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|         | TDL_SDK导出 |  cv183x  |     75.8      |   144.97    |   33.5   | 59.3559  |   42.1688    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |



