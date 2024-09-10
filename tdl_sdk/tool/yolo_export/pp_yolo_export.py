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
