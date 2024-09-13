# 通用pp-yoloe部署cv181x

PP-YOLOE是基于PP-Yolov2的Anchor-free模型，官方仓库在[PaddleDetection](https://github.com/PaddlePaddle/PaddleDetection)

获取官方仓库代码并安装：

```shell
git clone https://github.com/PaddlePaddle/PaddleDetection.git

# CUDA10.2
python -m pip install paddlepaddle-gpu==2.3.2 -i https://mirror.baidu.com/pypi/simple
```

其他版本参照官方安装文档[开始使用_飞桨-源于产业实践的开源深度学习平台 (paddlepaddle.org.cn)](https://www.paddlepaddle.org.cn/install/quick?docurl=/documentation/docs/zh/install/pip/linux-pip.html)

## onnx导出

onnx导出可以参考官方文档[PaddleDetection/deploy/EXPORT_ONNX_MODEL.md at release/2.4 · PaddlePaddle/PaddleDetection (github.com)](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.4/deploy/EXPORT_ONNX_MODEL.md)

本文档提供官方版本直接导出方式以及算能版本导出onnx，算能版本导出的方式需要去掉检测头的解码部分，方便后续量化，解码部分交给TDL_SDK实现

### 官方版本导出

可以使用PaddleDetection/tools/export_model.py导出官方版本的onnx模型

使用以下命令可以实现自动导出onnx模型，导出的onnx模型路径在`output_inference_official/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco_official.onnx`

```shell
cd PaddleDetection
python \
tools/export_model_official.py \
-c configs/ppyoloe/ppyoloe_crn_s_300e_coco.yml \
-o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_crn_s_300e_coco.pdparams

paddle2onnx \
--model_dir \
output_inference/ppyoloe_crn_s_300e_coco \
--model_filename model.pdmodel \
--params_filename model.pdiparams \
--opset_version 11 \
--save_file output_inference_official/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco_official.onnx

```

参数说明：

* -c 模型配置文件
* -o paddle模型权重
* --model_dir 模型导出目录
* --model_filename paddle模型的名称
* --params_filename paddle模型配置
* --opset_version opset版本配置
* --save_file 导出onnx模型的相对路径

### 算能版本导出

为了更好地进行模型量化，需要将检测头解码的部分去掉，再导出onnx模型，使用以下方式导出不解码的onnx模型

在`tools/`目录下新建一个文件`export_model_no_decode.py`，并添加如下代码

```python
# Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import sys

# add python path of PaddleDetection to sys.path
parent_path = os.path.abspath(os.path.join(__file__, *(['..'] * 2)))
sys.path.insert(0, parent_path)

# ignore warning log
import warnings
warnings.filterwarnings('ignore')

import paddle
from ppdet.core.workspace import load_config, merge_config
from ppdet.utils.check import check_gpu, check_version, check_config
from ppdet.utils.cli import ArgsParser
from ppdet.engine import Trainer
from ppdet.slim import build_slim_model
import paddle.nn.functional as F

from ppdet.utils.logger import setup_logger
logger = setup_logger('export_model')
import types


def yoloe_forward(self):
        body_feats = self.backbone(self.inputs)
        neck_feats = self.neck(body_feats, self.for_mot)
        yolo_head_outs = self.yolo_head(neck_feats)
        return yolo_head_outs

def head_forward(self, feats, targets=None, aux_pred=None):
    
    cls_score_list, reg_dist_list = [], []
    for i, feat in enumerate(feats):
        _, _, h, w = feat.shape
        l = h * w
        avg_feat = F.adaptive_avg_pool2d(feat, (1, 1))
        cls_logit = self.pred_cls[i](self.stem_cls[i](feat, avg_feat) +
                                        feat)
        reg_dist = self.pred_reg[i](self.stem_reg[i](feat, avg_feat))
        reg_dist = reg_dist.reshape(
            [-1, 4, self.reg_channels, l]).transpose([0, 2, 3, 1])
        reg_dist = self.proj_conv(F.softmax(
                reg_dist, axis=1)).squeeze(1)
        reg_dist = reg_dist.reshape([-1, h, w, 4])
        cls_logit = cls_logit.transpose([0, 2, 3, 1])
        cls_score_list.append(cls_logit)
        reg_dist_list.append(reg_dist)

    return cls_score_list, reg_dist_list


def parse_args():
    parser = ArgsParser()
    parser.add_argument(
        "--output_dir",
        type=str,
        default="output_inference",
        help="Directory for storing the output model files.")
    parser.add_argument(
        "--export_serving_model",
        type=bool,
        default=False,
        help="Whether to export serving model or not.")
    parser.add_argument(
        "--slim_config",
        default=None,
        type=str,
        help="Configuration file of slim method.")
    args = parser.parse_args()
    return args


def run(FLAGS, cfg):
    # build detector
    trainer = Trainer(cfg, mode='test')

    # load weights
    if cfg.architecture in ['DeepSORT', 'ByteTrack']:
        trainer.load_weights_sde(cfg.det_weights, cfg.reid_weights)
    else:
        trainer.load_weights(cfg.weights)

    # change yoloe forward & yoloe-head forward
    trainer.model._forward = types.MethodType(yoloe_forward, trainer.model)
    trainer.model.yolo_head.forward = types.MethodType(head_forward, trainer.model.yolo_head)
    # model.model.model[-1].forward = types.MethodType(forward2, model.model.model[-1])

    # export model
    trainer.export(FLAGS.output_dir)    

    if FLAGS.export_serving_model:
        from paddle_serving_client.io import inference_model_to_serving
        model_name = os.path.splitext(os.path.split(cfg.filename)[-1])[0]

        inference_model_to_serving(
            dirname="{}/{}".format(FLAGS.output_dir, model_name),
            serving_server="{}/{}/serving_server".format(FLAGS.output_dir,
                                                         model_name),
            serving_client="{}/{}/serving_client".format(FLAGS.output_dir,
                                                         model_name),
            model_filename="model.pdmodel",
            params_filename="model.pdiparams")


def main():
    paddle.set_device("cpu")
    FLAGS = parse_args()
    cfg = load_config(FLAGS.config)
    merge_config(FLAGS.opt)

    if FLAGS.slim_config:
        cfg = build_slim_model(cfg, FLAGS.slim_config, mode='test')

    # FIXME: Temporarily solve the priority problem of FLAGS.opt
    merge_config(FLAGS.opt)
    check_config(cfg)
    if 'use_gpu' not in cfg:
        cfg.use_gpu = False
    check_gpu(cfg.use_gpu)
    check_version()

    run(FLAGS, cfg)


if __name__ == '__main__':
    main()

```

然后使用如下命令导出不解码的pp-yoloe的onnx模型

```shell
python \
tools/export_model_no_decode.py \
-c configs/ppyoloe/ppyoloe_crn_s_300e_coco.yml \
-o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_crn_s_300e_coco.pdparams

paddle2onnx \
--model_dir \
output_inference/ppyoloe_crn_s_300e_coco \
--model_filename model.pdmodel \
--params_filename model.pdiparams \
--opset_version 11 \
--save_file output_inference/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco.onnx
```

参数参考官方版本导出的参数设置

## cvimodel导出

参考[appendix02-yolov5_model_deploy_helper][./appendix02_yolov5_model_deploy_helper.md]

## 接口说明

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
CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, conf_threshold);
CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, nms_threshold);
```

其中`conf_threshold`为置信度阈值；`nms_threshold`为 nms 阈值

### 测试Demo

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
  printf("start pp-yoloe preprocess config \n");
  // // setup preprocess
  InputPreParam p_preprocess_cfg;

  float mean[3] = {123.675, 116.28, 103.52};
  float std[3] = {58.395, 57.12, 57.375};

  for (int i = 0; i < 3; i++) {
    p_preprocess_cfg.mean[i] = mean[i] / std[i];
    p_preprocess_cfg.factor[i] = 1.0 / std[i];
  }
  
  p_preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

  printf("start yolo algorithm config \n");
  // setup yolo param
  cvtdl_det_algo_param_t p_yolo_param;
  p_yolo_param.cls = 80;

  printf("setup pp-yoloe param \n");
  ret = CVI_TDL_Set_PPYOLOE_Param(tdl_handle, &p_preprocess_cfg, &p_yolo_param);
  printf("pp-yoloe set param success!\n");
  if (ret != CVI_SUCCESS) {
    printf("Can not set PPYoloE parameters %#x\n", ret);
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
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }
  printf("cvimodel open success!\n");
  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, conf_threshold);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, nms_threshold);

  std::cout << "model opened:" << model_path << std::endl;

  VIDEO_FRAME_INFO_S fdFrame;
  ret = CVI_TDL_ReadImage(str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  std::cout << "CVI_TDL_ReadImage done!\n";

  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_PPYoloE(tdl_handle, &fdFrame, &obj_meta);

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

测试了ppyoloe_crn_s_300e_coco模型onnx以及cvimodel在cv181x/2x/3x平台的性能对比，其中阈值参数为：

* conf: 0.01
* nms: 0.7
* 输入分辨率：640 x 640

|          模型           |  部署版本  | 测试平台 | 推理耗时 (ms) | 带宽 (MB) | ION(MB)  | MAP 0.5  | MAP 0.5-0.95 |                   备注                   |
| :---------------------: | :--------: | :------: | :-----------: | :-------: | :------: | :------: | :----------: | :--------------------------------------: |
| ppyoloe_crn_s_300e_coco |  官方导出  | pytorch  |      N/A      |    N/A    |   N/A    |   60.5   |     43.1     |           pytorch官方fp32指标            |
|                         |  官方导出  |  cv181x  |    103.62     |  110.59   |  14.68   | 量化失败 |   量化失败   | 官方脚本导出cvimodel, cv181x平台评测指标 |
|                         |  官方导出  |  cv182x  |     77.58     |  111.18   |  14.67   | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv182x平台评测指标 |
|                         |  官方导出  |  cv183x  |   量化失败    | 量化失败  | 量化失败 | 量化失败 |   量化失败   | 官方脚本导出cvimodel，cv183x平台评测指标 |
|                         | TDL_SDK导出 |   onnx   |      N/A      |    N/A    |   N/A    | 55.9497  |   39.8568    |            TDL_SDK导出onnx指标            |
|                         | TDL_SDK导出 |  cv181x  |    101.15     |   103.8   |  14.55   |  55.36   |   39.1982    |  TDL_SDI导出cvimodel, cv181x平台评测指标  |
|                         | TDL_SDK导出 |  cv182x  |     75.03     |  104.95   |  14.55   |  55.36   |   39.1982    |  TDL_SDI导出cvimodel, cv182x平台评测指标  |
|                         | TDL_SDK导出 |  cv183x  |     30.96     |   80.43   |   13.8   |  55.36   |   39.1982    |  TDL_SDI导出cvimodel, cv183x平台评测指标  |

