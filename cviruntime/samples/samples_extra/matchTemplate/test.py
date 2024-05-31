#!/usr/bin/env python3
import os, cv2
import numpy as np
import pyruntime_cvi as pyruntime
from gen_mlir import gen_match_template_mlir

# read input
image = cv2.imread("input.png", cv2.IMREAD_GRAYSCALE)
template = cv2.imread("template.png", cv2.IMREAD_GRAYSCALE)
image = cv2.resize(image, (77, 109))
template = cv2.resize(template, (49, 80))
data = {}
data["input"] = image.astype(np.float32)
data["template"] = template.astype(np.float32)
np.savez("input.npz", **data)

ih, iw = image.shape
th, tw = template.shape
oh, ow = (ih - th + 1), (iw - tw + 1)

# ======= gen mlir file =======
status = gen_match_template_mlir(image.shape, template.shape, 'match_template.mlir',
                                 'TM_CCOEFF_NORMED')
if not status:
    raise ("generate match template mlir file failed.")

# ======= gen cvimodel =======
os.system('./convert.sh')

# ======= by cvimodel =======
model = pyruntime.Model("tmp/ccoeff_normed.cvimodel", 0, False)
if model == None:
    raise Exception("cannot load cvimodel")

# fill data to inputs
data0 = model.inputs[0].data
data1 = model.inputs[1].data
data0[:] = image.reshape(data0.shape)
data1[:] = template.reshape(data1.shape)
# forward
model.forward()
print(len(model.outputs))
for o in model.outputs:
    if o.name == "output_ArgMax":
        max_loc = o.data
    elif o.name == "output_values":
        max_value = o.data
    else:
        assert (0)
model_loc = (int(max_loc % ow), int(max_loc / ow))
print("model location(x,y):{} {}".format(model_loc, max_loc))

# ======== by opencv ==============
res = cv2.matchTemplate(image, template, cv2.TM_CCOEFF_NORMED)
_, ref_max_value, _, ref_max_loc = cv2.minMaxLoc(res)
print("opencv location(x,y):{}".format(ref_max_loc))

if model_loc == ref_max_loc:
    print("match success \n"
    "model_conf: {} ---> ref-opencv_conf: {}"\
        .format(max_value, ref_max_value))
else:
    print("match failed \n"
          "model_conf: {} ---> ref-opencv_conf: {}".format(max_value, ref_max_value))
