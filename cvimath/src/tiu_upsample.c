#include <cvimath_internal.h>
#include "gen_lut.h"

int cvm_upsample2d(cvk_context_t* ctx, cvk_tl_t* tl_input, cvk_tl_t* tl_weight,
                   cvk_tl_t* tl_output) {
  int ih = tl_input->shape.h;
  int iw = tl_input->shape.w;
  int sh = tl_weight->shape.h;
  int sw = tl_weight->shape.w;
  int kh = sh;
  int kw = sw;

  int pt = 0;
  int pl = 0;
  int pr = 0;
  int pb = 0;
  int dh = 1;
  int dw = 1;

  int ow = tl_output->shape.w;
  int oh = tl_output->shape.h;
  int kh_ext = (kh - 1) * dh + 1;
  int kw_ext = (kw - 1) * dw + 1;
  int ins_h = sh - 1;
  int ins_w = sw - 1;
  int pad_t = kh_ext - pt - 1;
  int pad_l = kw_ext - pl - 1;
  int pad_b = oh + pb - (ih - 1) * sh - 1;
  int pad_r = ow + pr - (iw - 1) * sw - 1;

  cvk_tiu_depthwise_pt_convolution_param_t param = {0};
  param.ofmap = tl_output;
  param.ifmap = tl_input;
  param.weight = tl_weight;
  param.bias = 0;
  param.ins_h = ins_h;
  param.ins_last_h = 0;
  param.ins_w = ins_w;
  param.ins_last_w = 0;
  param.stride_h = 1;
  param.stride_w = 1;
  param.dilation_h = 1;
  param.dilation_w = 1;
  param.pad_top = pad_t;
  param.pad_bottom = pad_b;
  param.pad_left = pad_l;
  param.pad_right = pad_r;
  param.relu_enable = 0;
  param.ins_val = 0;  // symmetric quantization
  param.ins_fp = 0;   // symmetric quantization
  ctx->ops->tiu_pt_depthwise_convolution(ctx, &param);

  return 0;
}
