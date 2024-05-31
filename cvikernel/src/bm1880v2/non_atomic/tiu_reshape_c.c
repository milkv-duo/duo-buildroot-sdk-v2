/**
 * reshape channel under depthwise
 */
//

#include "gen_lut.h"
#include <bmkernel/bm1880v2/1880v2_fp_convert.h>

//#define DBG
// copy from \1880v2_test_util.h
static int calc_dilute_hw(int h, int ins_h, int ins_h_l, int pad_h_b, int pad_h_t)
{
  return (h - 1) * (ins_h + 1) + ins_h_l +
                    1 + pad_h_t + pad_h_b;
}

// get padding as 'SAME' mode in tensorflow
// https://www.jianshu.com/p/05c4f1621c7e
static int get_same_pad(int ih, int sh, int kh) {
  return (((ih + sh - 1) / sh) - 1) * sh + kh - ih;
}

// get real 'h' with pad/ins
static int pooling_ih_ext(int ins_h, int ins_last_h, int pad_top, int pad_bottom, int ih)
{
  int ins = ins_h;
  int ins_last = ins_last_h;
  int pad = pad_top + pad_bottom;
  return (ih - 1) * (ins + 1) + ins_last + 1 + pad;
}

// get real 'w' with pad/ins
static int pooling_iw_ext(int ins_w, int ins_last_w, int pad_left, int pad_right, int iw)
{
  int ins = ins_w;
  int ins_last = ins_last_w;
  int pad = pad_left + pad_right;
  return (iw - 1) * (ins + 1) + ins_last + 1 + pad;
}

// get output h with parameter
static int pooling_oh(
    int ins_h, int ins_last_h, int pad_top, int pad_bottom,
    int stride_h, int ih, int kh, int dh)
{
  int ih_ext = pooling_ih_ext(ins_h, ins_last_h, pad_top, pad_bottom, ih);
  int d_h = (kh -1) * dh + 1;
  return (ih_ext - d_h) / stride_h + 1;
}

// get output w with parameter
static int pooling_ow(
    int ins_w, int ins_last_w, int pad_left, int pad_right,
    int stride_w, int iw, int kw, int dw)
{
  int iw_ext = pooling_iw_ext(ins_w, ins_last_w, pad_left, pad_right, iw);
  int d_w = (kw -1) * dw +1;
  return (iw_ext - d_w) / stride_w + 1;
}

/**
 * \brief get extended bias
 * \return allocated new bias
 */
uint32_t* bm1880v2_reshape_channel_bias(uint8_t* bias,
    int ni, int ci, int hi, int wi,
    int old_bias_c, fmt_t fmt
    ) {

  assert(bias);
  assert((ni == 2 || ni == 1) && "not support bias batch > 1");
  assert(ci / old_bias_c > 0 && ci % old_bias_c == 0);
  int sz = fmt == FMT_BF16 ? 4 : 2;

  int d_c_bias_sz = ni * ci * hi * wi;
  uint8_t *new_bias = (uint8_t *)malloc(d_c_bias_sz * sz);
  int bias_hw = hi * wi;
  int duplicat_c = ci / old_bias_c;

  for (int c = 0; c < old_bias_c; c++) {
    int shift = (c * bias_hw) * sz;
    for (int i = 0; i < duplicat_c; i++) {
      int new_bias_shift = (c * duplicat_c + i) * bias_hw * sz;
      memcpy(&new_bias[new_bias_shift], &bias[shift], bias_hw * sz);
    }
  }
  return (uint32_t* )new_bias;
}

/*
 * \brief prepare load shape/stride
 * \return -1 means fail to reshape, 0 means success
 * \TODO check memory usage
 */
static inline int _get_dup_shape(
    bmk1880v2_context_t *bk_ctx,
    int in, int ic, int ih, int iw, 
    int d_kh, int stride_h, int npu_num,
    bmk1880v2_tensor_lmem_shape_t* tl_shape, bmk1880v2_tensor_lmem_stride_t* tl_load_stride,
    bmk1880v2_tensor_tgmem_shape_t* tg_shape, bmk1880v2_tensor_tgmem_stride_t* tg_stride,
    fmt_t src_tg_fmt, fmt_t dst_tl_fmt
    ) {

  assert(in > 0 && ic > 0 && ih > 0 && iw > 0 && d_kh > 0 && stride_h > 0);
  assert(tl_shape && tl_load_stride && tg_shape && tg_stride);

  // 1. reshape and extend c, h axis in order
  int ch = ic * ih;
  int oc;
  int oh;

  // FIXME: check kernel setting
  oh = 0;

  for (int i = npu_num/ic; i > 0; i--) {
#if 0
    int hw = ih * iw;
    int _oh = hw / i / iw;
    if (hw % i == 0 && (hw / i) % stride_h == 0 && _oh >= stride_h) {
      oh = _oh;
      break;
    }
#else
    int _oh = ih / i;
    if (ih % i == 0 && (_oh) % stride_h == 0 && _oh >= stride_h /*&& _oh >= d_kh*/) {
      oh = _oh;
      break;
    }
#endif
  }


  if (!oh) {
    // FIXME: check terminal condition
    return -1;
  }

  oc = ch / oh;

#ifdef DBG
  printf ("ic:ih is %d %d, oc:oh is %d:%d\n", ic, ih, oc, oh);
#endif

  // tg/tl MUST be same shape size
  tl_shape->n = tg_shape->n = 1;
  tl_shape->c = tg_shape->c = oc;
  tl_shape->h = tg_shape->h = oh;
  tl_shape->w = tg_shape->w = iw;

  // init tl
  bmk1880v2_tensor_lmem_stride_t s = 
    bmk1880v2_tensor_lmem_default_stride(bk_ctx, *tl_shape, dst_tl_fmt, CTRL_NULL);
  tl_load_stride->n = s.n;
  tl_load_stride->c = s.c;
  tl_load_stride->h = s.h;
  tl_load_stride->w = s.w;

  // init tg
  bmk1880v2_tensor_tgmem_stride_t gs =
    bmk1880v2_tensor_tgmem_default_stride(*tg_shape, src_tg_fmt);

  tg_stride->n = gs.n;
  tg_stride->c = gs.c;
  tg_stride->h = gs.h;

  return 0;
}


/**
 * \brief get proper reshape size for depthwise conv with 'same' mode in h direction
 * \return -1 means alloc fail
 * \NOTICE: not support batch/ins_x/dilated_x/pad_top/pad_bottom
 */
int bm1880v2_reshape_channel_same(
    bmk1880v2_context_t *bk_ctx,
    int ic, int ih, int iw, int kh, int kw,
    int pad_right, int pad_left, int stride_h, int stride_w,
    bmk1880v2_tensor_lmem_shape_t* tl_load_shape,
    bmk1880v2_tensor_lmem_stride_t* new_tl_ifmap_stride,
    bmk1880v2_tensor_tgmem_shape_t* new_tg_ifmap_shape, 
    bmk1880v2_tensor_tgmem_stride_t* new_tg_ifmap_stride,
    bmk1880v2_tensor_lmem_shape_t* new_tl_weight_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_bias_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_ofmap_shape,
    fmt_t fmt, int eu_align) {

  assert(eu_align == 0 || eu_align == 1);

  bmk1880v2_chip_info_t chip_info = bmk1880v2_chip_info();
  // TODO: verify dilation_h/dilation_w
  int dilation_h = 1;
  int dilation_w = 1;
  // TODO: verify p->ins_h, p->ins_last_h
  int d_kh = calc_dilute_hw(kh, dilation_h - 1, 0, 0, 0);
  int h_after = calc_dilute_hw(ih, 0, 0, 0, 0);
  int in = 1;
  //int h_after = calc_dilute_hw(ih, p->ins_h, p->ins_last_h, p->pad_top, p->pad_bottom);
  //int w_after = calc_dilute_hw(iw, p->ins_w, p->ins_last_w, p->pad_left, p->pad_right);
  int ret = _get_dup_shape(bk_ctx, in, ic, h_after, iw, d_kh, stride_h, chip_info.npu_num,
      tl_load_shape, new_tl_ifmap_stride, new_tg_ifmap_shape, new_tg_ifmap_stride, 
      fmt, fmt);

  if (ret == -1) {
    return ret;
  }

  new_tl_weight_shape->n = 1;
  new_tl_weight_shape->c = tl_load_shape->c;
  new_tl_weight_shape->h = kh;
  new_tl_weight_shape->w = kw;

  new_tl_bias_shape->n = 2;
  new_tl_bias_shape->c = tl_load_shape->c;
  new_tl_bias_shape->h = 1;
  new_tl_bias_shape->w = 1;

  int pad_h = get_same_pad(tl_load_shape->h, stride_h, kh);
  //int no_pad_h = tl_load_shape->h;

  // reserve for padding
  new_tg_ifmap_shape->h += pad_h;
  tl_load_shape->h += pad_h;

  bmk1880v2_tensor_lmem_stride_t s = 
    bmk1880v2_tensor_lmem_default_stride(bk_ctx, *tl_load_shape, fmt, eu_align);

  new_tl_ifmap_stride->n = s.n;
  new_tl_ifmap_stride->c = s.c;
  new_tl_ifmap_stride->h = s.h;
  new_tl_ifmap_stride->w = s.w;

  // TODO: verity ins_x
  int oh = pooling_oh(0, 0, 0, 0,
      stride_h, tl_load_shape->h, kh, dilation_h);
  int ow = pooling_ow(0, 0, pad_left, pad_right,
      stride_w, tl_load_shape->w, kw, dilation_w);

#ifdef DBG
  printf("new oh/ow pad_h is %d/%d %d\n", oh, ow, pad_h);
#endif
  new_tl_ofmap_shape->n = in;
  new_tl_ofmap_shape->c = tl_load_shape->c;
  new_tl_ofmap_shape->h = oh;
  new_tl_ofmap_shape->w = ow;

  return ret;
}

/*
 * \brief duplicate weight for reshaped c
 */
uint8_t* bm1880v2_reshape_channel_weight(uint8_t* weight,
    int ni, int ci, int hi, int wi,
    int old_weight_c,
    fmt_t fmt) {

  assert(weight);
  assert(ci / old_weight_c > 0 && ci % old_weight_c == 0);

  int sz = fmt == FMT_BF16 ? 2 : 1;

  int new_weight_hw_shape_size = hi * wi;
  int new_weight_shape_size = ni * ci * hi * wi;
  int duplicat_c = ci / old_weight_c;
  uint8_t *new_weight = (uint8_t *)malloc(new_weight_shape_size * sz);


  for (int n = 0; n < ni; n++) {
    for (int c = 0; c < old_weight_c; c++) {
      int index = (n * old_weight_c + c) * new_weight_hw_shape_size * sz;
      for (int i = 0; i < duplicat_c; i++) {
        int new_weight_index = (n * old_weight_c * duplicat_c +
            c * duplicat_c + i) * new_weight_hw_shape_size * sz;
        memcpy(&new_weight[new_weight_index], &weight[index], new_weight_hw_shape_size * sz);
      }
    }
  }

  return new_weight;
}

/*
 * \brief prepare load shape/stride with pad
 * \return -1 means fail to reshape, 0 means success
 * \TODO check memory usage
 */
static inline int _get_dup_shape_same_pad(
    bmk1880v2_context_t *bk_ctx,
    int in, int ic, int ih, int iw, 
    int d_kh, int stride_h, int npu_num,
    bmk1880v2_tensor_lmem_shape_t* tl_load_shape, 
    bmk1880v2_tensor_lmem_stride_t* tl_load_stride,
    bmk1880v2_tensor_tgmem_shape_t* tg_shape, 
    bmk1880v2_tensor_tgmem_stride_t* tg_stride,
    fmt_t src_tg_fmt, fmt_t dst_tl_fmt
    ) {

  assert(in > 0 && ic > 0 && ih > 0 && iw > 0 && d_kh > 0 && stride_h > 0);
  assert(tl_load_shape && tl_load_stride && tg_shape && tg_stride);

  // 1. reshape and extend c, h axis in order
  int oc;
  int oh;

  // FIXME: check kernel setting
  oh = 0;

  // 2. get total output
  // 3. slice output
  assert((ih - d_kh) % stride_h == 0);
  int ih_ext = pooling_ih_ext(0, 0, 0, 0, ih);
  int _oh = (ih_ext - d_kh) / stride_h + 1;

  for (int i = npu_num/ic; i > 0; i--) {
    if (_oh % i == 0) {
      // add 1 for later padding
      oh = stride_h * (_oh / i - 1) + 1;
      oc = i * ic;
      break;
    }
  }

  if (!oh) {
    // FIXME: check terminal condition
    return -1;
  }

#ifdef DBG
  printf ("ic:ih is %d %d, oc:oh is %d:%d\n", ic, ih, oc, oh);
#endif

  // tg/tl MUST be same shape size
  tl_load_shape->n = tg_shape->n = 1;
  tl_load_shape->c = tg_shape->c = oc;
  tl_load_shape->h = tg_shape->h = oh;
  tl_load_shape->w = tg_shape->w = iw;

  // init tl
  bmk1880v2_tensor_lmem_stride_t s = 
    bmk1880v2_tensor_lmem_default_stride(bk_ctx, *tl_load_shape, dst_tl_fmt, CTRL_NULL);
  tl_load_stride->n = s.n;
  tl_load_stride->c = s.c;
  tl_load_stride->h = s.h;
  tl_load_stride->w = s.w;

  // init tg
  bmk1880v2_tensor_tgmem_stride_t gs =
    bmk1880v2_tensor_tgmem_default_stride(*tg_shape, src_tg_fmt);

  tg_stride->n = gs.n;
  tg_stride->c = gs.c;
  tg_stride->h = gs.h;

  return 0;
}

/**
 * \brief get proper reshape size for depthwise conv with 'same' mode in h direction
 * 'pad' means \ih is padded
 * \return -1 means alloc fail
 * \NOTICE: not support batch/ins_x/dilated_x/pad_top/pad_bottom
 */
int bm1880v2_reshape_channel_same_pad(
    bmk1880v2_context_t *bk_ctx,
    int ic, int ih, int iw, int kh, int kw,
    int pad_right, int pad_left, int stride_h, int stride_w,
    bmk1880v2_tensor_lmem_shape_t* tl_load_shape,
    bmk1880v2_tensor_lmem_stride_t* new_tl_ifmap_stride,
    bmk1880v2_tensor_tgmem_shape_t* new_tg_ifmap_shape, 
    bmk1880v2_tensor_tgmem_stride_t* new_tg_ifmap_stride,
    bmk1880v2_tensor_lmem_shape_t* new_tl_weight_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_bias_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_ofmap_shape,
    fmt_t fmt, int eu_align) {

  assert(eu_align == 0 || eu_align == 1);

  bmk1880v2_chip_info_t chip_info = bmk1880v2_chip_info();
  // TODO: verify dilation_h/dilation_w
  int dilation_h = 1;
  int dilation_w = 1;
  // TODO: verify p->ins_h, p->ins_last_h
  int d_kh = calc_dilute_hw(kh, dilation_h - 1, 0, 0, 0);
  int h_after = calc_dilute_hw(ih, 0, 0, 0, 0);
  int in = 1;
  //int h_after = calc_dilute_hw(ih, p->ins_h, p->ins_last_h, p->pad_top, p->pad_bottom);
  //int w_after = calc_dilute_hw(iw, p->ins_w, p->ins_last_w, p->pad_left, p->pad_right);
  int ret = _get_dup_shape_same_pad(bk_ctx, in, ic, 
      h_after, iw, d_kh, stride_h, chip_info.npu_num,
      tl_load_shape, new_tl_ifmap_stride, new_tg_ifmap_shape, new_tg_ifmap_stride, 
      fmt, fmt);

  if (ret == -1) {
    return ret;
  }

  new_tl_weight_shape->n = 1;
  new_tl_weight_shape->c = tl_load_shape->c;
  new_tl_weight_shape->h = kh;
  new_tl_weight_shape->w = kw;

  new_tl_bias_shape->n = 2;
  new_tl_bias_shape->c = tl_load_shape->c;
  new_tl_bias_shape->h = 1;
  new_tl_bias_shape->w = 1;

  int pad_h = get_same_pad(tl_load_shape->h, stride_h, kh);
  //int no_pad_h = tl_load_shape->h;

  // reserve for padding
  new_tg_ifmap_shape->h += pad_h;
  tl_load_shape->h += pad_h;

  bmk1880v2_tensor_lmem_stride_t s = 
    bmk1880v2_tensor_lmem_default_stride(bk_ctx, *tl_load_shape, fmt, eu_align);

  new_tl_ifmap_stride->n = s.n;
  new_tl_ifmap_stride->c = s.c;
  new_tl_ifmap_stride->h = s.h;
  new_tl_ifmap_stride->w = s.w;

  // TODO: verity ins_x
  int oh = pooling_oh(0, 0, 0, 0,
      stride_h, tl_load_shape->h, kh, dilation_h);
  int ow = pooling_ow(0, 0, pad_left, pad_right,
      stride_w, tl_load_shape->w, kw, dilation_w);

#ifdef DBG
  printf("new oh/ow pad_h is %d/%d %d\n", oh, ow, pad_h);
#endif
  new_tl_ofmap_shape->n = in;
  new_tl_ofmap_shape->c = tl_load_shape->c;
  new_tl_ofmap_shape->h = oh;
  new_tl_ofmap_shape->w = ow;

  return ret;
}
