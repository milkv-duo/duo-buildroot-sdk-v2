from ultralytics import YOLO
import types
import argparse

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--weights', type=str, default='./yolov8s.pt', help='weights path')
    parser.add_argument('--img-size', nargs='+', type=int, default=[640, 640], help='image size, the order is: height width')  # height, width
    args = parser.parse_args()
    return args

def forward2(self, x):
    # print(self.nl)
    x_reg = [self.cv2[i](x[i]) for i in range(self.nl)]
    x_cls = [self.cv3[i](x[i]) for i in range(self.nl)]
    return x_reg + x_cls

def export_onnx(model_path, img_size):
    model = YOLO(model_path)
    model.model.model[-1].forward = types.MethodType(forward2, model.model.model[-1])
    model.export(format='onnx', opset=11, imgsz=img_size)

if __name__ == '__main__':
    args = get_args()
    export_onnx(args.weights, args.img_size)
    