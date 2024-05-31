import numpy as np
import pyruntime as rt
cvk_ctx = rt.CvkContext("CviContext")
a = np.array([[[[1]]]], dtype=np.int8)
b = cvk_ctx.lmem_alloc_tensor(a, 1)
b.shapes()
b.address()
c = cvk_ctx.lmem_alloc_tensor(a, 1)
c.shapes()
c.address()

cvk_ctx.tdma_g2l_tensor_copy(b, a)
d = np.array([[[[0]]]], dtype=np.int8)
cvk_ctx.tdma_l2g_tensor_copy(d, b)

print(a == d)
