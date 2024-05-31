#!/usr/bin/python3
import argparse
import pyruntime as rt
import numpy as np

def bf16_to_fp32(d_bf16):
  s = d_bf16.shape
  d_bf16 = d_bf16.flatten()
  assert d_bf16.dtype == np.uint16
  d_fp32 = np.empty_like(d_bf16, dtype=np.float32)
  for i in range(len(d_bf16)):
    d_fp32[i] = struct.unpack('<f', struct.pack('<HH', 0, d_bf16[i]))[0]
  return d_fp32.reshape(s)

def compare_one_array(a, b):
  if a.dtype == np.uint16:
    a = bf16_to_fp32(a)
  return np.array_equal(a.astype(np.float32).flatten(),
                        b.astype(np.float32).flatten())

def max_name_sz(tensors):
  max_sz = 0
  for out in tensors:
    sz = len(out.name)
    max_sz = max_sz if sz < max_sz else sz
  return max_sz


def compare_with_ref(tensors, refs):
  result = [0, 0]
  style = "{:<" + str(max_name_sz(tensors) + 4) + "}";
  print("To compare outputs with refernece npz:")
  for out in tensors:
    ref = refs[out.name]
    same = compare_one_array(out.data, ref)
    result[int(same)] += 1
    print("  {} [{}]".format(str.format(style, out.name),
          "PASS" if same else "FAIL"))
  print("{} passed, {} failed, Compare {}!!!"
        .format(result[1], result[0], "OK" if result[0] == 0 else "ERROR"))
  return result[0] == 0

def quant(x, scale):
  x = x * scale
  x = np.trunc(x + np.copysign(.5, x))
  x[x > 127.0] = 127.0
  x[x < -128.0] = -128.0
  return x.astype(np.int8)

def test(input_npz, cvimodel, ref_npz):
  # load cvimodel
  model = rt.Model(cvimodel, batch_num=1, output_all_tensors=True)
  # fill data to inputs
  data = model.inputs[0].data
  qscale = model.inputs[0].qscale
  # load input data and quant to int8
  input_npz = np.load(input_npz)
  input = input_npz[input_npz.files[0]]
  print(input.shape)
  input = quant(input, qscale)
  # fill input data to input tensor of model
  data[:] = input.reshape(data.shape)
  for out in model.outputs:
    print(out.name)
    print(out.data.dtype)
    print(out.data.shape)
  # forward
  model.forward()
  # compare result with reference
  refs = np.load(ref_npz)
  compare_with_ref(model.outputs, refs)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Test python runtime API.")
  parser.add_argument("--cvimodel", type=str, help="cvimodel")
  parser.add_argument("--input", type=str, help="input npz")
  parser.add_argument("--reference", type=str, help="reference to output npz")
  args = parser.parse_args()
  test(args.input, args.cvimodel, args.reference)