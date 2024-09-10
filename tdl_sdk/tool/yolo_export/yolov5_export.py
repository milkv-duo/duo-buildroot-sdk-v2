import argparse
import time
import torch
from models.experimental import attempt_load
import types

def split_forward(self, x):
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


def export_onnx(model, file, img_size, opset):

    print(f'starting export {file} to onnx...')
    save_path = file.replace(".pt", ".onnx")

    output_names = ['output0']
    img = torch.zeros(1, 3, img_size[0], img_size[1])
    torch.onnx.export(
        model,  # --dynamic only compatible with cpu
        img,
        save_path,
        verbose=False,
        opset_version=opset,
        do_constant_folding=True,  # WARNING: DNN inference with torch>=1.12 may require do_constant_folding=False
        input_names=['images'],
        output_names=output_names)
    print(f"finish export onnx {save_path}!")


def run(weights='yolov5s.pt', img_size=(640, 640)):
    t = time.time()
    model = attempt_load(weights, inplace=True, fuse=True)  # load FP32 model

    # model branch split
    model.model[-1].forward = types.MethodType(split_forward, model.model[-1])
    export_onnx(model, weights, img_size, 12)

def parse_opt():
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, default='yolov5s.pt', help='model.pt path(s)')
    parser.add_argument('--imgsz', '--img', '--img-size', nargs='+', type=int, default=[640, 640], help='image h w')
    opt = parser.parse_args()
    return opt

if __name__ == '__main__':
    args = parse_opt()
    run(args.weights, args.imgsz)
