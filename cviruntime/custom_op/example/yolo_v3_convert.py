#!/usr/bin/python3
"""
Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
"""

import onnx
from cvi_toolkit.transform.BaseConverter import TensorType
from cvi_toolkit.transform.onnx_converter import OnnxConverter
from cvi_toolkit.transform.tflite_converter_int8 import TFLiteConverter
from cvi_toolkit.transform.tensorflow_converter import TFConverter
from cvi_toolkit.utils.log_setting import setup_logger
from cvi_toolkit.data.preprocess import add_preprocess_parser, preprocess

logger = setup_logger('root', log_level="INFO")

class MyOnnxConverter(OnnxConverter):
    def __init__(self, model_name, onnx_model, mlir_file_path, batch_size=1, preprocessor=None):
        super().__init__(model_name, onnx_model, mlir_file_path, batch_size, preprocessor.to_dict())
        self.onnxop_factory['LeakyRelu'] = lambda node: self.convert_leaky_relu(node);

    def convert_leaky_relu(self, onnx_node):
        assert(onnx_node.op_type == "LeakyRelu")
        alpha = onnx_node.attrs.get("alpha", 0.01)
        custom_op_param = {
            'tpu': True,
            'do_quant': True,
            'operation_name': 'leaky_relu',
            'threshold_overwrite': 'backward',
            'param': {
                'negative_slope': float(alpha)
            }
        }
        op, input_shape, tensor_type = self.getOperand(onnx_node.inputs[0])
        operands = list()
        operands.append(op)
        output_shape = input_shape
        custom_op = self.CVI.add_custom_op("{}_{}".format(onnx_node.name, onnx_node.op_type),
                                           operands, output_shape, **custom_op_param)
        self.addOperand(onnx_node.name, custom_op, output_shape, TensorType.ACTIVATION)


if __name__ == "__main__":
    onnx_model = onnx.load('model/yolov3-416.onnx')
    preprocessor = preprocess()
    preprocessor.config(net_input_dims="416,416",
               resize_dims="416,416", crop_method='center', keep_aspect_ratio=True,
               raw_scale=1.0, mean='0,0,0', std='1,1,1', input_scale=1.0,
               channel_order='bgr', pixel_format=None, data_format='nchw',
               aligned=False, gray=False)
    c = MyOnnxConverter('yolo_v3', 'model/yolov3-416.onnx',
                        'yolo_v3_416.mlir', batch_size=1, preprocessor=preprocessor)
    c.run()