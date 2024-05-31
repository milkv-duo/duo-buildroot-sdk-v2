#!/usr/bin/python3
"""
Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
"""

from argparse import ArgumentParser
from cvi_toolkit.transform.BaseConverter import TensorType
from cvi_toolkit.transform.caffe_converter import CaffeConverter
from cvi_toolkit.utils.log_setting import setup_logger
from cvi_toolkit.data.preprocess import preprocess

logger = setup_logger('root', log_level="INFO")

class MyCaffeConverter(CaffeConverter):
    def __init__(self, model_name, prototxt, caffe_model, mlir_file_path, batch_size=1):
        super().__init__(model_name, prototxt, caffe_model, mlir_file_path, batch_size)
        self.caffeop_factory['Upsample'] = lambda layer: self.convert_unpooling_op(layer);


    def convert_unpooling_op(self, layer):
        assert(self.layerType(layer) == "Upsample")
        data, data_shape, _ = self.getOperand(layer.bottom[0])
        mask, mask_shape, _ = self.getOperand(layer.bottom[1])
        operands = list()
        operands.append(data)
        operands.append(mask)
        
        p = layer.upsample_param
        scale = p.scale
        if p.HasField("upsample_h"):
            unpool_h = p.upsample_h
        else:
            unpool_h = mask_shape[2]
        if p.HasField("upsample_w"):
            unpool_w = p.upsample_w
        else:
            unpool_w = mask_shape[3]

        output_shape = [data_shape[0], data_shape[1], unpool_h, unpool_w]

        custom_op_param = {
            'tpu': True,
            'do_quant': True,
            'operation_name': 'unpooling',
            'threshold_overwrite': 'backward',
            'param': {
                'unpool_h': unpool_h,
                'unpool_w': unpool_w,
                'scale': scale
            }
        }
        print("layer name: {}, top name: {}\n".format(layer.name, layer.top[0]))
        custom_op = self.CVI.add_custom_op(layer.name,
                                           operands, output_shape, **custom_op_param)
        self.addOperand(layer.top[0], custom_op, output_shape, TensorType.ACTIVATION)

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--model_path", type=str)
    parser.add_argument("--model_dat", type=str)
    parser.add_argument("--mlir_file_path", type=str)
    args = parser.parse_args()

    #preprocessor = preprocess()
    #preprocessor.config(net_input_dims="360,480",
    #                    resize_dims="360,480")
    
    c = MyCaffeConverter('segnet', args.model_path, args.model_dat,
                        args.mlir_file_path, batch_size=1)
    c.run()
