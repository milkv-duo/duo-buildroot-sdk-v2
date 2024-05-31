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

    def convert_graph(self):
        """convert all to mlir"""

        # add input op
        # add input op
        for idx, input in enumerate(self.input_nodes):
            input_shape = list()
            for i, dim in enumerate(input.type.tensor_type.shape.dim):
                # batch size
                # dim is zero, mean mutli batch
                if i == 0 and dim.dim_value <= 0:
                    input_shape.append(self.batch_size)
                else:
                    input_shape.append(dim.dim_value)

            if not self.preprocess_args:
                input_op = self.CVI.add_input_op(input.name, idx, **{})
            else:
                preprocess_hint = {
                    'mean': self.preprocess_args['perchannel_mean'],
                    'scale':  self.preprocess_args['perchannel_scale'],
                    'pixel_format': self.preprocess_args["pixel_format"],
                    'channel_order': self.preprocess_args["channel_order"],
                    'aligned': self.preprocess_args["aligned"],
                    'resize_dims': self.preprocess_args['resize_dims'],
                    'keep_aspect_ratio': self.preprocess_args['keep_aspect_ratio']
                }
                # add input op
                input_op = self.CVI.add_input_op(input.name, idx, **preprocess_hint)
            self.addOperand(input.name, input_op, input_shape, TensorType.ACTIVATION)

        def NoneAndRaise(node):
            raise RuntimeError("{} Op not support now".format(node.op_type))
        # add node op
        for n in self.converted_nodes:
            self.onnxop_factory.get(n.op_type, lambda x: NoneAndRaise(x))(n)

        self.add_softmax_op()
        # add return op
        return_op = list()
        # Set output
        op, _, _ = self.getOperand("prob")
        return_op.append(op)

        self.CVI.add_return_op(return_op)
        mlir_txt = self.CVI.print_module()
        with open(self.mlir_file_path, "w") as f:
            f.write(mlir_txt)

    def add_softmax_op(self):
        softmax_op_param = {
            'tpu': False,
            'do_quant': False,
            'operation_name': 'mysoftmax',
            'threshold_overwrite': 'none',
            'param': {
                'axis': 1
            }
        }
        op, input_shape, tensor_type = self.getOperand('output')
        operands = list()
        operands.append(op)
        output_shape = input_shape
        custom_op = self.CVI.add_custom_op("prob_softmax", operands, output_shape, **softmax_op_param)
        self.addOperand("prob", custom_op, output_shape, TensorType.ACTIVATION)

if __name__ == "__main__":
    onnx_model = onnx.load('model/resnet18.onnx')
    preprocessor = preprocess()
    preprocessor.config(net_input_dims="224,224",
               resize_dims="256,256", crop_method='center', keep_aspect_ratio=False,
               raw_scale=1.0, mean='0.406,0.456,0.485', std='0.225,0.224,0.229', input_scale=1.0,
               channel_order='bgr', pixel_format=None, data_format='nchw',
               aligned=False, gray=False)
    c = MyOnnxConverter('resnet18', 'model/resnet18.onnx',
                        'resnet18.mlir', batch_size=1, preprocessor=preprocessor)
    c.run()
