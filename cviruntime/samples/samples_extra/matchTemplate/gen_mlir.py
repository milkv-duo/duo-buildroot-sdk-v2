import argparse
from utils.misc import *

def gen_match_template_mlir(input_shape: list, template_shape: list,
                             mlir_file: str = "match_template.mlir",
                             mode: str = "TM_CCOEFF_NORMED") -> bool:
  try:
    assert(len(input_shape) == 2 and len(template_shape) == 2)
    ih, iw = input_shape
    th, tw = template_shape
    oh, ow = ih - th + 1, iw - tw + 1
    assert(oh > 0 and ow > 0)

    with open(mlir_file, "w") as f:
      f.write('#loc = loc(unknown)\n'
              + 'module attributes {module.chip = "ALL", module.name = "MatchTemplate", '
              + 'module.platform = "ONNX", module.state = "TOP_F32", module.weight_file = '
              + '"match_template_top_f32_all_origin_weight.npz"} {\n')
      f.write('  func.func @main(%arg0: tensor<{}x{}xf32> loc(unknown), %arg1: tensor<{}x{}xf32> \
    loc(unknown)) -> (tensor<1xf32>, tensor<1xf32>)'.format(ih, iw, th, tw) + '{\n')
      f.write('    %0 = "top.None"() : () -> none loc(#loc)\n')
      f.write('    %1 = "top.Input"(%arg0) : (tensor<{}x{}xf32>) -> tensor<{}x{}xf32> loc(#loc1)\n'.format(ih, iw, ih, iw))
      f.write('    %2 = "top.Input"(%arg1) : (tensor<{}x{}xf32>) -> tensor<{}x{}xf32> loc(#loc2)\n'.format(th, tw, th, tw))
      f.write('    %3 = "top.MatchTemplate"(%1, %2) {' + 'mode = "{}"'.format(mode) + '} : '
              + '(tensor<{}x{}xf32>, tensor<{}x{}xf32>) -> tensor<{}x{}xf32> loc(#loc3)\n'.format(ih, iw, th, tw, oh, ow))
      f.write('    %4 = "top.Reshape"(%3) : (tensor<{}x{}xf32>) -> tensor<{}xf32> loc(#loc4)\n'.format(oh, ow, oh * ow))
      f.write('    %5:2 = "top.Arg"(%4) {axis = 0 : i64, keepdims = true, mode = "ArgMax", select_last_index = true} :'
              + '(tensor<{}xf32>) -> (tensor<1xf32>, tensor<1xf32>) loc(#loc7)\n'.format(oh * ow))
      f.write('    return %5#0, %5#1 : tensor<1xf32>, tensor<1xf32> loc(#loc)\n')
      f.write('  } loc(#loc)\n'
              + '} loc(#loc)\n'
              + '#loc1 = loc("input")\n'
              + '#loc2 = loc("template")\n'
              + '#loc3 = loc("match")\n'
              + '#loc4 = loc("6_Reshape")\n'
              + '#loc5 = loc("output_ArgMax")\n'
              + '#loc6 = loc("output_values")\n'
              + '#loc7 = loc(fused[#loc5, #loc6])')
    return True
  except:
    return False

if __name__ == '__main__':
  # python gen_mlir.py --input_shape [109,77] --template_shape [80,49]
  parser = argparse.ArgumentParser()
  parser.add_argument("--input_shape", type=str2shape, required=True,
                      help="list of input shape, like:[109, 77]")
  parser.add_argument("--template_shape", type=str2shape, required=True,
                      help="list of template shape, like:[80, 49]")
  parser.add_argument("--mode", default="TM_CCOEFF_NORMED", type=str.upper,
                      choices=['TM_CCOEFF_NORMED', 'TM_SQDIFF'],
                      help="MatchTemplate mode")
  parser.add_argument("--mlir", default="match_template.mlir",
                      help="output mlir model file")
  args = parser.parse_args()

  status = gen_match_template_mlir(args.input_shape[0], args.template_shape[0], args.mlir, args.mode)

  if status:
    print("======== success gen mlir file ========")
  else:
    print("======== failed gen mlir file ========")
