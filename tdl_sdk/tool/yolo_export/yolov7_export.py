import torch
import torch.nn as nn
import models
from models.common import Conv
from utils.activations import Hardswish, SiLU
import types
import argparse

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, default='yolov7s.pt', help='model.py path')
    parser.add_argument('--img-size', nargs='+', type=int, default=[640, 640], help='image size, the order is: height width')  # height, width
    args = parser.parse_args()
    return args

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

def parse_model(model):
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

if __name__ == '__main__':
    args = get_args()
    save_path = args.weights.replace(".pt", ".onnx")
    ckpt = torch.load(args.weights, map_location="cpu")
    model = ckpt["model"].float().fuse().eval()
    print("Parsing model...")
    parse_model(model)
    
    model.model[-1].forward = types.MethodType(forward, model.model[-1])
    img = torch.zeros(1, 3, *args.img_size)
    torch.onnx.export(model, img, save_path, verbose=False,
                  opset_version=12, input_names=['images'])
    
    print(f'export onnx model path: {save_path}')
    
