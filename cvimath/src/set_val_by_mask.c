#include <cvimath_internal.h>
#include "gen_lut.h"

static inline int check_u8(cvk_tl_t* a, cvk_tl_t* b, cvk_tl_t* c) {
  return (a->fmt == CVK_FMT_U8 && b->fmt == CVK_FMT_U8 && c->fmt == CVK_FMT_U8);
}

static inline int check_same_fmt(cvk_tl_t* a, cvk_tl_t* b, cvk_tl_t* c) {
  return a->fmt == b->fmt && b->fmt == c->fmt;
}

int cvm_set_image_by_u8mask(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_buf,
                            cvk_tl_t* tl_mask, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_buf, tl_ofmap)) {
    // throw config error
    printf("input/buf/ofmap format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf, tl_mask) && !check_same_fmt(tl_ifmap, tl_buf, tl_ofmap)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  cvk_tl_t* high = tl_buf;
  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    // TODO: support it
    high = NULL;
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    tl_ofmap->fmt = tl_buf->fmt = tl_mask->fmt = CVK_FMT_I8;
    cvm_emit_mul_const(ctx, high, high, high->fmt, 0);
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // revert mask to set selected one as 0
  cvm_emit_mul_const(ctx, tl_mask, tl_mask, tl_mask->fmt, -1);

  // set mask selected one as 0
  // e.g: -1 - (-1) for this cast that turn to -1 * -1 + 255(0xff) = 256, get low part as 0
  cvk_tiu_mac_param_t p2;
  p2.res_high = high;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ofmap;
  p2.b_is_const = 0;
  p2.b = tl_mask;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;

  ctx->ops->tiu_mac(ctx, &p2);

  // revert back
  cvm_emit_mul_const(ctx, tl_mask, tl_mask, tl_mask->fmt, -1);

  // overwrite selected one
  p2.res_high = high;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap;
  p2.b_is_const = 0;
  p2.b = tl_mask;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;

  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_ifmap->fmt = tl_buf->fmt = tl_mask->fmt = tl_ofmap->fmt = fmt;

  return ret;
}

// dp means depthwise version
int cvm_set_image_by_u8mask_dp(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_mask,
                               cvk_tl_t* tl_kernel, cvk_tl_t* tl_bias, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_ifmap, tl_ofmap)) {
    // throw config error
    printf("input/buf/ofmap format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_ifmap, tl_mask) &&
      !check_same_fmt(tl_ifmap, tl_ifmap, tl_ofmap)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    // TODO: support it
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    tl_ifmap->fmt = tl_ofmap->fmt = tl_mask->fmt = CVK_FMT_I8;
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // mask 1 means overwrite new one, 0 means keep old
  // if = if * mask
  // mask = depthwise(mask) * kernel_1x1 + bias, kernel set to -1, bias set to 1
  // of = of * mask
  // mask = mask * 0 // reset high part
  // mask_of = of + 1 * if

  cvm_emit_mul(ctx, tl_ifmap, tl_mask, tl_ifmap, tl_mask->fmt);

  // revert 0/1 to 1/0
  cvk_tiu_depthwise_pt_convolution_param_t param;
  param.ofmap = tl_mask;
  param.ifmap = tl_mask;
  param.weight = tl_kernel;
  param.bias = tl_bias;
  param.ins_h = 0;
  param.ins_last_h = 0;
  param.ins_w = 0;
  param.ins_last_w = 0;
  param.stride_h = 1;
  param.stride_w = 1;
  param.dilation_h = 1;
  param.dilation_w = 1;
  param.pad_top = 0;
  param.pad_bottom = 0;
  param.pad_left = 0;
  param.pad_right = 0;
  param.relu_enable = 0;
  param.rshift_bits = 0;
  param.ins_val = 0;  // symmetric quantization
  param.ins_fp = 0;   // symmetric quantization
  ctx->ops->tiu_pt_depthwise_convolution(ctx, &param);

  // keep of
  cvm_emit_mul(ctx, tl_ofmap, tl_mask, tl_ofmap, tl_mask->fmt);

  // reset high part
  cvm_emit_mul_const(ctx, tl_mask, tl_mask, tl_mask->fmt, 0);

  cvk_tiu_mac_param_t p2;
  p2.res_high = tl_mask;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 1;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;

  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_ifmap->fmt = tl_mask->fmt = tl_ofmap->fmt = fmt;

  return ret;
}

// \is_less = 1 means that 1 indicate less and 0 is greater equal \threshold
// \is_less = 0 means that 1 indicate greater equal \threshold and 0 indicate less
static void __get_less_large_mask(cvk_context_t* ctx, cvk_tl_t* buf, cvk_tl_t* buf2,
                                  cvk_tl_t* tl_update_tbl, uint8_t threshold, bool is_less) {
  bool is_signed = buf->fmt == CVK_FMT_I8;

  // keep tl_update_tbl < threshold
  // mul to hoist int16 and add it with sign bit
  // TODO: try not use high part
  cvk_tiu_mul_param_t p1 = {0};
  p1.res_high = buf2;
  p1.res_low = buf;
  p1.a = tl_update_tbl;
  p1.b_const.val = 1;
  p1.b_const.is_signed = is_signed;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // just check sign bit for > thres or not
  // i16 diff
  cvk_fmt_t fmt = buf->fmt;
  buf2->fmt = buf->fmt = CVK_FMT_I8;
  is_signed = true;

  // e.g: 10 - 6 = 4, 2 - 6 = -4
  cvk_tiu_add_param_t p4;
  p4.res_high = 0;  // saturatue to int8
  p4.res_low = buf;
  p4.a_high = buf2;
  p4.a_low = buf;
  p4.b_is_const = 1;
  p4.b_const.val = -1 * (threshold);
  p4.b_const.is_signed = is_signed;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // saturate to int max
  // 4 * -127 = -128,  -4 * -127 = 127
  // hex represent is 0x80, 0x7F
  cvk_tiu_mul_param_t p;
  p.res_high = 0;
  p.res_low = buf;
  p.a = buf;
  p.b_is_const = 1;
  p.b_const.val = -127;  // revert to > 0
  if (!is_less) {
    p.b_const.val = 127;
  }
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 0;
  p.relu_enable = is_signed;
  ctx->ops->tiu_mul(ctx, &p);

  // set as mask(127->1)
  // hex represent is 0x80, 0x7F, right shift 7
  // 0x1, 0x0
  p.res_high = 0;
  p.res_low = buf;
  p.a = buf;
  p.b_is_const = 1;
  p.b_const.val = 1;
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 7;
  p.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p);

  // revert
  buf2->fmt = buf->fmt = fmt;
  return;
}

// \is_less = 1 means that 1 indicate less and 0 is greater equal \threshold
// \is_less = 0 means that 1 indicate greater equal \threshold and 0 indicate less
static void __get_less_large_mask_dp(cvk_context_t* ctx, cvk_tl_t* tl_update_tbl,
                                     cvk_tl_t* tl_kernel, cvk_tl_t* tl_threshold, bool is_less) {
  bool is_signed = tl_update_tbl->fmt == CVK_FMT_I8;

  // 1. depthwise(tl_update_tbl) * kernel(1) + bias(-1 * threshold)
  // 2. mul to saturate it
  // 3. right shift

  cvk_tiu_depthwise_pt_convolution_param_t param;
  param.ofmap = tl_update_tbl;
  param.ifmap = tl_update_tbl;
  param.weight = tl_kernel;
  param.bias = tl_threshold;
  param.ins_h = 0;
  param.ins_last_h = 0;
  param.ins_w = 0;
  param.ins_last_w = 0;
  param.stride_h = 1;
  param.stride_w = 1;
  param.dilation_h = 1;
  param.dilation_w = 1;
  param.pad_top = 0;
  param.pad_bottom = 0;
  param.pad_left = 0;
  param.pad_right = 0;
  param.relu_enable = 0;
  param.rshift_bits = 0;
  param.ins_val = 0;  // symmetric quantization
  param.ins_fp = 0;   // symmetric quantization
  ctx->ops->tiu_pt_depthwise_convolution(ctx, &param);

  cvk_fmt_t fmt = tl_update_tbl->fmt;
  tl_update_tbl->fmt = CVK_FMT_I8;
  is_signed = true;

  // saturate to int max
  // 4 * -127 = -128,  -4 * -127 = 127
  // hex represent is 0x80, 0x7F
  cvk_tiu_mul_param_t p;
  p.res_high = 0;
  p.res_low = tl_update_tbl;
  p.a = tl_update_tbl;
  p.b_is_const = 1;
  p.b_const.val = -127;  // revert to > 0
  if (!is_less) {
    p.b_const.val = 127;
  }
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 0;
  p.relu_enable = is_signed;
  ctx->ops->tiu_mul(ctx, &p);

  // set as mask(127->1)
  // hex represent is 0x80, 0x7F, right shift 7
  // 0x1, 0x0
  p.res_high = 0;
  p.res_low = tl_update_tbl;
  p.a = tl_update_tbl;
  p.b_is_const = 1;
  p.b_const.val = 1;
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 7;
  p.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p);

  // revert
  tl_update_tbl->fmt = fmt;
  return;
}
/**
 * \high as output
 */
static void _get_less_large_mask(cvk_context_t* ctx, cvk_tl_t* buf, cvk_tl_t* buf2,
                                 cvk_tl_t* tl_update_tbl, uint8_t threshold, bool is_less) {
  bool is_signed = buf->fmt == CVK_FMT_I8;
  // keep tl_update_tbl < threshold
  // mul to hoist int16 and add it with sign bit
  // TODO: try not use high part
  cvk_tiu_mul_param_t p1 = {0};
  p1.res_high = buf2;
  p1.res_low = buf;
  p1.a = tl_update_tbl;
  p1.b_const.val = 1;
  p1.b_const.is_signed = is_signed;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // just check sign bit for > thres or not
  // i16 diff
  cvk_fmt_t fmt = buf->fmt;
  buf2->fmt = buf->fmt = CVK_FMT_I8;
  is_signed = true;

  cvk_tiu_add_param_t p4;
  p4.res_high = 0;  // saturatue to int8
  p4.res_low = buf;
  p4.a_high = buf2;
  p4.a_low = buf;
  p4.b_is_const = 1;
  p4.b_const.val = -1 * (threshold);
  p4.b_const.is_signed = is_signed;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // saturate to int max
  cvk_tiu_mul_param_t p;
  p.res_high = 0;
  p.res_low = buf;
  p.a = buf;
  p.b_is_const = 1;
  p.b_const.val = -127;  // revert to > 0
  if (!is_less) {
    p.b_const.val = 127;
  }
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 0;
  p.relu_enable = is_signed;
  ctx->ops->tiu_mul(ctx, &p);

  // set as mask(127->1)
  p.res_high = 0;
  p.res_low = buf;
  p.a = buf;
  p.b_is_const = 1;
  p.b_const.val = 1;
  p.b_const.is_signed = is_signed;
  p.rshift_bits = 7;
  p.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p);

  // revert
  buf2->fmt = buf->fmt = fmt;
  return;
}

static void _get_less_mask(cvk_context_t* ctx, cvk_tl_t* buf, cvk_tl_t* buf2,
                           cvk_tl_t* tl_update_tbl, uint8_t threshold) {
  _get_less_large_mask(ctx, buf, buf2, tl_update_tbl, threshold, /*is_less=*/1);
}

static void _get_large_mask(cvk_context_t* ctx, cvk_tl_t* buf, cvk_tl_t* buf2,
                            cvk_tl_t* tl_update_tbl, uint8_t threshold) {
  _get_less_large_mask(ctx, buf, buf2, tl_update_tbl, threshold, /*is_less=*/0);
}

int cvm_set_image_by_two_info_i8(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_buf2,
                                 cvk_tl_t* tl_mask, cvk_tl_t* tl_update_tbl, uint8_t threshold,
                                 cvk_tl_t* tl_ofmap) {
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_buf2, tl_update_tbl)) {
    // throw config error
    printf("input/buf/tl_update_tbl format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf2, tl_mask)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  if (tl_ofmap->fmt == CVK_FMT_BF16) {
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    // tl_update_tbl->fmt = tl_buf->fmt = tl_mask->fmt = tl_ofmap->fmt = CVK_FMT_I8;
    tl_buf2->fmt = tl_update_tbl->fmt = tl_ofmap->fmt = tl_mask->fmt = CVK_FMT_I8;
  } else {
    printf("not support fmt\n");
    return -3;
  }

  __get_less_large_mask(ctx, tl_update_tbl, tl_buf2, tl_update_tbl, threshold, 1);

  // set new mask
  cvm_emit_mul(ctx, tl_mask, tl_update_tbl, tl_mask, tl_mask->fmt);

  // restore
  tl_buf2->fmt = tl_update_tbl->fmt = tl_mask->fmt = tl_ofmap->fmt = fmt;

  return cvm_set_image_by_u8mask(ctx, tl_ifmap, tl_buf2, tl_mask, tl_ofmap);
}

int cvm_set_image_by_two_info_i8_dp(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_kernel,
                                    cvk_tl_t* tl_mask, cvk_tl_t* tl_update_tbl,
                                    cvk_tl_t* tl_threshold, cvk_tl_t* tl_ofmap) {
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_ofmap, tl_update_tbl)) {
    // throw config error
    printf("input/buf/tl_update_tbl format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_ofmap, tl_mask)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  if (tl_update_tbl->shape.h <= 1) {
    printf("tl_update_tbl will be as bias high part, the high should be >= 2\n");
    return -3;
  }

  if (tl_ofmap->fmt == CVK_FMT_BF16) {
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    // tl_update_tbl->fmt = tl_buf->fmt = tl_mask->fmt = tl_ofmap->fmt = CVK_FMT_I8;
    tl_update_tbl->fmt = tl_mask->fmt = CVK_FMT_I8;
  } else {
    printf("not support fmt\n");
    return -3;
  }

  __get_less_large_mask_dp(ctx, tl_update_tbl, tl_kernel, tl_threshold, 1);

  // set new mask
  cvm_emit_mul(ctx, tl_mask, tl_update_tbl, tl_mask, tl_mask->fmt);

  // dirty bias set to 1
  // tl_bias = tl_bias * 0
  // tl_update_tbl = tl_update_tbl * 0
  // tl_update-tbl_tl_bias = tl_update_tbl-tl_bias + 1, reshape tl_update_tbl, set to 1
  // tbl_tl_bias = tl_update copy high part to tbl_tl_bias high part, stride w = 2
  cvm_emit_mul_const(ctx, tl_threshold, tl_threshold, tl_threshold->fmt, 0);
  cvm_emit_mul_const(ctx, tl_update_tbl, tl_update_tbl, tl_update_tbl->fmt, 0);

  cvk_tl_stride_t tl_update_tbl_st = tl_update_tbl->stride;
  tl_update_tbl->stride = tl_threshold->stride;
  tl_update_tbl->shape = tl_threshold->shape;

  cvk_tiu_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_threshold;
  p4.a_high = tl_update_tbl;
  p4.a_low = tl_threshold;
  p4.b_is_const = 1;
  p4.b_const.val = 1;
  p4.b_const.is_signed = 1;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // clean high part
  tl_threshold->start_address++;  // continuous low/high
  tl_update_tbl->shape.n = 1;
  cvm_emit_mul_const(ctx, tl_threshold, tl_threshold, tl_threshold->fmt, 0);
  tl_threshold->start_address--;  // restore

  // restore
  tl_update_tbl->fmt = tl_mask->fmt = fmt;
  tl_update_tbl->stride = tl_update_tbl_st;
  tl_update_tbl->shape = tl_mask->shape;

  // set to -1 for \cvm_set_image_by_u8mask_dp
  cvm_emit_mul_const(ctx, tl_kernel, tl_kernel, tl_kernel->fmt, -1);

  return cvm_set_image_by_u8mask_dp(ctx, tl_ifmap, tl_mask, tl_kernel, tl_threshold, tl_ofmap);
}

int cvm_gen_image_diff(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_ifmap2,
                       cvk_tl_t* tl_buf, cvk_tl_t* tl_buf2, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_ifmap2, tl_ofmap)) {
    // throw config error
    printf("input/buf/ofmap format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf, tl_buf2)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    // TODO: support it
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // get large one
  cvk_tiu_max_param_t p13 = {0};
  p13.max = tl_buf;
  p13.a = tl_ifmap;
  p13.b_is_const = 0;
  p13.b = tl_ifmap2;
  p13.layer_id = 0;
  ctx->ops->tiu_max(ctx, &p13);

  // compare to get a > b or a < b, 1 means a > b
  // cvk_tiu_sub_param_t p5;
  // p5.res_high = 0; // saturatue to int8
  // p5.res_low = tl_ofmap;
  // p5.a_high= tl_buf2;
  // p5.a_low = tl_buf;
  // p5.b_high = tl_buf2;
  // p5.b_low = tl_ifmap2;
  // p5.rshift_bits = 0;
  // ctx->ops->tiu_sub(ctx, &p5);
  tl_ifmap2->fmt = tl_buf->fmt = tl_buf2->fmt = CVK_FMT_I8;
  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, tl_buf2->fmt, 0);
  cvk_tiu_mac_param_t p2;
  p2.res_high = tl_buf2;
  p2.res_low = tl_buf;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 1;
  p2.b_const.val = -1;
  p2.b_const.is_signed = 1;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // mul 255 and rightshift to get 0/1, 1 means tl_ifmap > tl_ifmap2
  // get positive
  tl_buf->fmt = CVK_FMT_U8;
  cvk_tiu_mul_param_t p1 = {0};
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 255;
  p1.b_const.is_signed = 0;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // that max = 127
  tl_buf->fmt = CVK_FMT_I8;
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = -127;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // 127 >> 7 to 0/1
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 7;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  tl_ifmap->fmt = tl_ofmap->fmt = tl_buf->fmt = CVK_FMT_I8;
  // keep a that a > b
  cvm_emit_mul(ctx, tl_buf, tl_ifmap, tl_ofmap, tl_ofmap->fmt);

  // mul -1 for get - b under a > b
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, tl_buf->fmt, -1);

  // get a - b = a + (-1) * b
  // cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, tl_buf2->fmt, 0);
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);
  cvm_emit_mul_const(ctx, tl_ofmap, tl_ofmap, tl_ofmap->fmt, 1);

  // hoist to int16
  tl_buf2->fmt = CVK_FMT_I8;
  p1.res_high = tl_buf2;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // get revert 0/-1 to 1/0, get a < b case
  cvk_tiu_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_buf;
  p4.a_high = tl_buf2;
  p4.a_low = tl_buf;
  p4.b_is_const = 1;
  p4.b_const.val = 1;
  p4.b_const.is_signed = 1;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // remove a < b in b
  cvm_emit_mul(ctx, tl_buf, tl_ifmap2, tl_ifmap2, tl_ifmap2->fmt);

  // mul -1 for -a
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, tl_buf->fmt, -1);

  // a<b part, b + a * -1
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ifmap2;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // output is u8, a > b part merge a < b
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 0;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_buf->fmt = tl_buf2->fmt = tl_ifmap->fmt = tl_ifmap2->fmt = tl_ofmap->fmt = fmt;

  return ret;
}

int cvm_gen_image_diff_dp(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_ifmap2,
                          cvk_tl_t* tl_buf, cvk_tl_t* tl_buf2, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_ifmap2, tl_ofmap)) {
    // throw config error
    printf("input/buf/ofmap format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf, tl_buf2)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    // TODO: support it
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // tl_buf = max(\tl_ifmap, \tl_ifmap2)
  // tl_buf2-tl_buf = tl_buf2-tl_buf + (- 1 * tl_ifmap2), == 0 means \tl_ifmap < \tl_ifmap2,
  // otherwise \tl_ifmap > \tl_ifmap2 tl_buf = tl_buf * 255 to get 0/1, 1 means \tl_ifmap >
  // \tl_ifmap3 get large one
  cvk_tiu_max_param_t p13 = {0};
  p13.max = tl_buf;
  p13.a = tl_ifmap;
  p13.b_is_const = 0;
  p13.b = tl_ifmap2;
  p13.layer_id = 0;
  ctx->ops->tiu_max(ctx, &p13);

  // compare to get a > b or a < b, 1 means a > b
  // cvk_tiu_sub_param_t p5;
  // p5.res_high = 0; // saturatue to int8
  // p5.res_low = tl_ofmap;
  // p5.a_high= tl_buf2;
  // p5.a_low = tl_buf;
  // p5.b_high = tl_buf2;
  // p5.b_low = tl_ifmap2;
  // p5.rshift_bits = 0;
  // ctx->ops->tiu_sub(ctx, &p5);
  tl_ifmap2->fmt = tl_buf->fmt = tl_buf2->fmt = CVK_FMT_I8;
  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, tl_buf2->fmt, 0);
  cvk_tiu_mac_param_t p2;
  p2.res_high = tl_buf2;
  p2.res_low = tl_buf;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 1;
  p2.b_const.val = -1;
  p2.b_const.is_signed = 1;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // mul 255 and rightshift to get 0/1, 1 means tl_ifmap > tl_ifmap2
  // get positive
  tl_buf->fmt = CVK_FMT_U8;
  cvk_tiu_mul_param_t p1 = {0};
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 255;
  p1.b_const.is_signed = 0;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // that max = 127
  tl_buf->fmt = CVK_FMT_I8;
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = -127;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // 127 >> 7 to 0/1
  p1.res_high = 0;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 7;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  tl_ifmap->fmt = tl_ofmap->fmt = tl_buf->fmt = CVK_FMT_I8;
  // keep a that a > b
  cvm_emit_mul(ctx, tl_buf, tl_ifmap, tl_ofmap, tl_ofmap->fmt);

  // mul -1 for get - b under a > b
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, tl_buf->fmt, -1);

  // get a - b = a + (-1) * b
  // cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, tl_buf2->fmt, 0);
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);
  cvm_emit_mul_const(ctx, tl_ofmap, tl_ofmap, tl_ofmap->fmt, 1);

  // hoist to int16
  tl_buf2->fmt = CVK_FMT_I8;
  p1.res_high = tl_buf2;
  p1.res_low = tl_buf;
  p1.a = tl_buf;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  // get revert 0/-1 to 1/0, get a < b case
  cvk_tiu_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_buf;
  p4.a_high = tl_buf2;
  p4.a_low = tl_buf;
  p4.b_is_const = 1;
  p4.b_const.val = 1;
  p4.b_const.is_signed = 1;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // remove a < b in b
  cvm_emit_mul(ctx, tl_buf, tl_ifmap2, tl_ifmap2, tl_ifmap2->fmt);

  // mul -1 for -a
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, tl_buf->fmt, -1);

  // a<b part, b + a * -1
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ifmap2;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // output is u8, a > b part merge a < b
  p2.res_high = tl_buf2;  // dont care add garbage
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap2;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 0;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_buf->fmt = tl_buf2->fmt = tl_ifmap->fmt = tl_ifmap2->fmt = tl_ofmap->fmt = fmt;

  return ret;
}
int cvm_update_tbl_by_threshold(cvk_context_t* ctx, cvk_tl_t* tl_mask, cvk_tl_t* tl_buf,
                                cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_update_tbl,
                                uint8_t threshold_a, uint8_t threshold_b, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  (void)threshold_b;
  (void)tl_mask;
  (void)tl_buf2;
  (void)tl_buf3;
  (void)tl_update_tbl;
  cvk_fmt_t fmt = tl_ofmap->fmt;
  if (!check_u8(tl_ofmap, tl_buf, tl_buf)) {
    // throw config error
    printf("ofmap/buf format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ofmap, tl_buf, tl_buf)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  cvk_tl_t* high = tl_buf;
  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    high = NULL;
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    // cvm_emit_mul_const(ctx, high, high, high->fmt, 0);
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // mask = get_less_u8(diff[i], thresh_a)
  _get_less_mask(ctx, high, tl_buf2, tl_update_tbl, threshold_a);

  // mask_1 = get_less_i8(update_tbl[i], thresh_b), 0/1
  tl_buf2->fmt = tl_ofmap->fmt = tl_buf3->fmt = CVK_FMT_I8;
  _get_less_mask(ctx, tl_buf2, tl_buf3, tl_ofmap, threshold_b);

  // mask_2 = mask * mask_1 // keep for next triple if-else
  // tl_update_tbl as buf
  tl_update_tbl->fmt = tl_buf->fmt = CVK_FMT_I8;
  cvm_emit_mul(ctx, tl_buf, tl_buf2, tl_update_tbl, tl_buf2->fmt);

  cvm_emit_mul_const(ctx, tl_update_tbl, tl_update_tbl, tl_buf2->fmt, -1);
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf3, tl_buf3->fmt, 0);  // diff 0 used
  // update_tbl[i] = update_tbl[i] - mask_2 * update_tbl[i], set 0
  // sub itself leverage int16 is ok, plz refer \cvm_set_image_by_u8mask
  cvk_tiu_mac_param_t p2;
  p2.res_high = tl_buf3;  // diff itsef MUST set high part as 0
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_update_tbl;
  p2.b_is_const = 0;
  p2.b = tl_ofmap;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // mask_1 = (mask_1 - 1), hoist it
  cvk_tiu_mul_param_t p1 = {0};
  p1.res_high = tl_buf3;
  p1.res_low = tl_buf2;
  p1.a = tl_buf2;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  cvk_tiu_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_buf2;
  p4.a_high = tl_buf3;
  p4.a_low = tl_buf2;
  p4.b_is_const = 1;
  p4.b.high = 0;
  p4.b_const.val = -1;
  p4.b_const.is_signed = 1;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  // mask_2 = mask * mask_1
  cvm_emit_mul(ctx, tl_buf, tl_buf2, tl_update_tbl, tl_buf2->fmt);

  // update_tbl[i] = update_tbl[i] + mask_2 // (update_tbl[i]-1)
  // int8, hoist it
  p1.res_high = tl_buf3;
  p1.res_low = tl_ofmap;
  p1.a = tl_ofmap;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  p2.res_high = tl_buf3;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 1;  // keep origin
  p2.a = tl_update_tbl;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 1;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // mask = (mask - 1) * -1 // export, rever 0/1 to 1/0
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf3, tl_buf3->fmt, 0);  // diff 0 used
  p4.res_high = 0;
  p4.res_low = tl_buf;
  p4.a_high = tl_buf3;
  p4.a_low = tl_buf;
  p4.b_is_const = 1;
  p4.b.high = 0;
  p4.b_const.val = -1;
  p4.b_const.is_signed = 1;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  ctx->ops->tiu_add(ctx, &p4);

  cvm_emit_mul_const(ctx, tl_buf, tl_buf, tl_buf->fmt, -1);

  // update_tbl[i] = update_tbl[i] + mask // update_tbl[i]++, return
  // int8, hoist it
  p1.res_high = tl_buf3;
  p1.res_low = tl_ofmap;
  p1.a = tl_ofmap;
  p1.b_const.val = 1;
  p1.b_const.is_signed = 1;
  p1.b_is_const = true;
  p1.rshift_bits = 0;
  p1.layer_id = 0;
  p1.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p1);

  p2.res_high = tl_buf3;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 1;  // keep origin
  p2.a = tl_buf;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 1;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_buf2->fmt = tl_buf3->fmt = tl_ofmap->fmt = tl_update_tbl->fmt = tl_buf->fmt = fmt;

  return ret;
}

int cvm_set_image_by_two_info_u8(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_buf,
                                 cvk_tl_t* tl_buf2, cvk_tl_t* tl_update_tbl, uint8_t threshold,
                                 cvk_tl_t* tl_ofmap) {
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_buf, tl_update_tbl)) {
    // throw config error
    printf("input/buf/tl_update_tbl format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf, tl_buf2)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  cvk_tl_t* high = tl_buf;
  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    high = NULL;
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    // tl_buf2->fmt = tl_update_tbl->fmt = tl_ofmap->fmt = tl_buf->fmt = CVK_FMT_I8;
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // large equal, u8 compare
  _get_large_mask(ctx, high, tl_buf2, tl_update_tbl, threshold - 1);
  // return 0;

  // restore
  tl_buf2->fmt = tl_update_tbl->fmt = tl_buf->fmt = tl_ofmap->fmt = fmt;

  return cvm_set_image_by_u8mask(ctx, tl_ifmap, tl_buf2, high, tl_ofmap);
}

int cvm_blend_image_by_tbl(cvk_context_t* ctx, cvk_tl_t* tl_ifmap, cvk_tl_t* tl_buf,
                           cvk_tl_t* tl_buf2, cvk_tl_t* tl_update_tbl, uint8_t threshold,
                           uint8_t w1, uint8_t w2, cvk_tl_t* tl_ofmap) {
  int ret = 0;
  cvk_fmt_t fmt = tl_ifmap->fmt;
  if (!check_u8(tl_ifmap, tl_buf, tl_ofmap) || !check_u8(tl_buf2, tl_update_tbl, tl_update_tbl)) {
    // throw config error
    printf("input/input1/input2/tl_update_tbl/buf/ofmap format should config CVK_FMT_U8\n");
    return -1;
  }

  if (!check_same_fmt(tl_ifmap, tl_buf, tl_update_tbl) &&
      !check_same_fmt(tl_buf2, tl_update_tbl, tl_ofmap)) {
    printf("all tensor's fmt shoud be equal\n");
    return -2;
  }

  cvk_tl_t* high = tl_buf;
  if (tl_ofmap->fmt == CVK_FMT_BF16) {
    // TODO: support it
    high = NULL;
  } else if (tl_ofmap->fmt == CVK_FMT_U8) {
    // hw limitation that input should be i8
    tl_buf2->fmt = tl_buf->fmt = tl_update_tbl->fmt = CVK_FMT_I8;
    cvm_emit_mul_const(ctx, high, high, high->fmt, 0);
  } else {
    printf("not support fmt\n");
    return -3;
  }

  // get g_update_tbl[i]>threshold
  _get_large_mask(ctx, high, tl_buf2, tl_update_tbl, threshold);

  // dirty tl_update_tbl
  // TODO: not copy again
  cvm_emit_mul_const(ctx, high, tl_update_tbl, tl_buf->fmt, 1);

  tl_buf2->fmt = tl_buf->fmt = tl_ofmap->fmt = CVK_FMT_U8;
  // ofmap * w1, keep high part
  cvk_tiu_mul_param_t p;
  p.res_high = tl_buf2;
  p.res_low = tl_buf;
  p.a = tl_ofmap;
  p.b_is_const = 1;
  p.b_const.val = w1;
  p.b_const.is_signed = 0;
  p.rshift_bits = 0;
  p.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &p);

  // buf2 = buf2 + w2*pY[i], i16 output, it should be >= 0
  cvk_tiu_mac_param_t p2;
  p2.res_high = tl_buf2;
  p2.res_low = tl_buf;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ifmap;
  p2.b_is_const = 1;
  p2.b_const.val = w2;
  p2.b_const.is_signed = 0;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // keep update_tbl[i]>threshold mask, it ok for signed that just keep data
  tl_buf2->fmt = tl_update_tbl->fmt = CVK_FMT_I8;
  cvm_emit_mul(ctx, tl_buf2, tl_update_tbl, tl_buf2, tl_update_tbl->fmt);

  // mul -1 for sub it, dirty tl_update_tbl
  cvm_emit_mul_const(ctx, tl_update_tbl, tl_update_tbl, tl_update_tbl->fmt, -1);

  high->fmt = tl_ofmap->fmt = CVK_FMT_I8;
  // NOTICE: we only keep low part as U8
  // set update_tbl[i]>threshold as 0
  // sub itself leverage int16 is ok, plz refer \cvm_set_image_by_u8mask
  p2.res_high = high;
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_ofmap;
  p2.b_is_const = 0;
  p2.b = tl_update_tbl;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // ouput as U8, get high part
  tl_buf->fmt = tl_ofmap->fmt = tl_buf2->fmt = CVK_FMT_U8;
  p2.res_high = high;  // dont care
  p2.res_low = tl_ofmap;
  p2.res_is_int8 = 0;  // keep origin
  p2.a = tl_buf2;
  p2.b_is_const = 1;
  p2.b_const.val = 1;
  p2.b_const.is_signed = 0;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;
  ctx->ops->tiu_mac(ctx, &p2);

  // restore
  tl_buf2->fmt = tl_ifmap->fmt = tl_buf->fmt = tl_update_tbl->fmt = tl_ofmap->fmt = fmt;

  return ret;
}
