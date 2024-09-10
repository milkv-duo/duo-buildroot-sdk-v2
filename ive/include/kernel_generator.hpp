#pragma once
#include "tpu_data.hpp"

enum IVE_KERNEL {
  GAUSSIAN = 0,
  SOBEL_X,
  SOBEL_Y,
  SCHARR_X,
  SCHARR_Y,
  MORPH_RECT,
  MORPH_CROSS,
  MORPH_ELLIPSE,
  CUSTOM
};

IveKernel createKernel(CVI_RT_HANDLE rt_handle, uint32_t img_c, uint32_t k_h, uint32_t k_w,
                       IVE_KERNEL type);