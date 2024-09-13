#!/usr/bin/env python3
# -*- coding:utf-8 -*-
import argparse
import time
import sys
import torch
import torch.nn as nn

sys.path.insert(0, '../../')

from yolov6.models.yolo import *
from yolov6.models.effidehead import Detect
from yolov6.layers.common import *
from yolov6.utils.events import LOGGER
from yolov6.utils.checkpoint import load_checkpoint
from io import BytesIO

import types

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

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, default='./yolov6s.pt', help='weights path')
    parser.add_argument('--img-size', nargs='+', type=int, default=[640, 640], help='image size, the order is: height width')  # height, width
    parser.add_argument('--batch-size', type=int, default=1, help='batch size')
    parser.add_argument('--half', action='store_true', help='FP16 half-precision export')
    parser.add_argument('--inplace', action='store_true', help='set Detect() inplace=True')

    args = parser.parse_args()
    args.img_size *= 2 if len(args.img_size) == 1 else 1  # expand
    print(args)
    t = time.time()

    # Check device
    device = torch.device('cpu')
    assert not (device.type == 'cpu' and args.half), '--half only compatible with GPU export, i.e. use --device 0'
    # Load PyTorch model
    model = load_checkpoint(args.weights, map_location=device, inplace=True, fuse=True)  # load FP32 model
    for layer in model.modules():
        if isinstance(layer, RepVGGBlock):
            layer.switch_to_deploy()
        elif isinstance(layer, nn.Upsample) and not hasattr(layer, 'recompute_scale_factor'):
            layer.recompute_scale_factor = None  # torch 1.11.0 compatibility
    # Input
    img = torch.zeros(args.batch_size, 3, *args.img_size).to(device)  # image size(1,3,320,192) iDetection

    model.eval()
    for k, m in model.named_modules():
        if isinstance(m, ConvModule):  # assign export-friendly activations
            if hasattr(m, 'act') and isinstance(m.act, nn.SiLU):
                m.act = SiLU()
        elif isinstance(m, Detect):
            m.inplace = args.inplace
    
    model.detect.forward = types.MethodType(detect_forward, model.detect)

    y = model(img)  # dry run

    # ONNX export
   
    export_file = args.weights.replace('.pt', '.onnx')  # filename

    torch.onnx.export(model, img, export_file, verbose=False, opset_version=12,
                        training=torch.onnx.TrainingMode.EVAL,
                        do_constant_folding=True,
                        input_names=['images'],
                        output_names=['outputs'])
    
    print(f'export onnx model success! export model path:  {export_file}')
