#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef struct {
  int random_seed;
  int input_n;
  int input_c;
  int input_h;
  int input_w;
  int kw;
  int kh;
  int dh;
  int dw;
  int pad_top;
  int pad_bot;
  int pad_left;
  int pad_right;
  int ins_h;
  int ins_h_last;
  int ins_w;
  int ins_w_last;
  int stride_h;
  int stride_w;
  int output_c;
  int using_bias;
  int bReLU_EN;
  int r_shift_m;
  int opd0_sign;
  int opd1_sign;
  int opd2_sign;
  int bf16_enable;
} conv_param_t;

static inline void bf16_relu(float *buf, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static int ps32_conv_ref(
    const conv_param_t *p_param,
    const uint16_t *ifmap,
    const uint16_t *weight,
    const uint32_t *bias,
    uint16_t *ofmap, int ps32_mode)
{
  int in = p_param->input_n;
  int ic = p_param->input_c;
  int ih = p_param->input_h;
  int iw = p_param->input_w;
  int oc = p_param->output_c;
  int kh = p_param->kh;
  int kw = p_param->kw;
  int dh = p_param->dh;
  int dw = p_param->dw;
  int stride_h = p_param->stride_h;
  int stride_w = p_param->stride_w;
  int pad_top = p_param->pad_top;
  int pad_bot = p_param->pad_bot;
  int pad_left = p_param->pad_left;
  int pad_right = p_param->pad_right;
  int ins_h = p_param->ins_h;
  int ins_h_last = p_param->ins_h_last;
  int ins_w = p_param->ins_w;
  int ins_w_last = p_param->ins_w_last;

  int ih_ext = calc_dilute_hw(ih, ins_h, ins_h_last,  pad_top, pad_bot);
  int iw_ext = calc_dilute_hw(iw, ins_w, ins_w_last,  pad_left, pad_right);
  int kh_ext = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int kw_ext = calc_dilute_hw(kw, dw - 1, 0, 0, 0);
  int oh = calc_output_hw(ih_ext, kh_ext, stride_h);
  int ow = calc_output_hw(iw_ext, kw_ext, stride_w);

  float *result = (float *)malloc(sizeof(float) * in * oc * oh * ow);
  if (!result)
    return -1;

  uint32_t bstride = in * oc * oh * ow;
  int ret = 0;

  if (ps32_mode == 2 || ps32_mode == 0)
    memset(result, 0, sizeof(float) * in * oc * oh * ow);
  else {
    for (int i = 0; i < in * oc * oh * ow; i++) {
      result[i] = cvk_convert_hex_fp32((ofmap[i + bstride * 0] << 16) | ofmap[i + bstride * 1]);
    }
  }

  for (int n = 0; n < in; ++n) {
    for (int c = 0; c < oc; ++c) {
      uint16_t *i_fmap_pad[ic];
      uint16_t *kernel_pad[ic];

      for (int iic = 0; iic < ic; ++iic) {
        i_fmap_pad[iic] = NULL;
        kernel_pad[iic] = NULL;
        fill_pad_fmap_bf16(
            (ifmap + n*ic*ih*iw + iic*ih*iw), &i_fmap_pad[iic], cvk_convert_fp32_bf16(0),
            pad_left, pad_right, pad_top, pad_bot,
            ins_h, ins_w, ins_h_last, ins_w_last, ih, iw);
        //kernel_dilation(
        fill_pad_fmap_bf16(
            (weight + c*ic*kh*kw + iic*kh*kw), &kernel_pad[iic], cvk_convert_fp32_bf16(0),
            0, 0, 0, 0,  // no padding
            dh - 1, dw - 1, 0, 0,
                  kh, kw);
      }
      for (int ph = 0; ph < oh; ++ph) {
        for (int pw = 0; pw < ow; ++pw) {
          float result_val= result[n*oc*oh*ow + c*oh*ow + ph*ow + pw];
          for (int idxh = 0; idxh < kh_ext; ++idxh)  {
            for (int idxw = 0; idxw < kw_ext; ++idxw) {
              for (int iic = 0; iic < ic; ++iic){
                float ifv = cvk_convert_bf16_fp32(i_fmap_pad[iic][(idxh+ph*stride_h) * iw_ext + idxw + pw*stride_w]);
                float ikv = cvk_convert_bf16_fp32(kernel_pad[iic][idxh* kw_ext + idxw]);
                result_val += ifv*ikv;
              }
            }
          }
          result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] = result_val;
		    }
      }
        for(int i = 0; i < ic; i++) {
          if (i_fmap_pad[i]) {
            free(i_fmap_pad[i]);
            i_fmap_pad[i] = NULL;
          }
          if (kernel_pad[i]) {
            free(kernel_pad[i]);
            kernel_pad[i] = NULL;
          }
        }
    } //end for (int c = 0; c < oc; ++c)
  }

  if( ps32_mode & 0x2) {
    for (int i = 0; i < in * oc * oh * ow; i ++) {
      ofmap[i] = cvk_convert_fp32_hex(result[i]) >> 16;
      ofmap[bstride + i] = cvk_convert_fp32_hex(result[i]) & 0xFFFF;
    }
  } else {
    for (int n = 0; n < in; ++n) {
      for (int c = 0; c < oc; ++c) {
        if (p_param->using_bias) {
          for (int ph = 0; ph < oh; ++ph) {
            for (int pw = 0; pw < ow; ++pw) {
              result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] += cvk_convert_hex_fp32(bias[c]); //bias+c ;
            }
          }
        }
        if (p_param->bReLU_EN)
          bf16_relu(&result[n*oc*oh*ow + c*oh*ow], oh * ow);
      }
    }
    for (int i = 0; i < in * oc * oh * ow; i++) {
      ofmap[i] = cvk_convert_fp32_bf16(result[i]);
    }
  }
  free(result);
  return ret;
}

static uint16_t * transform_weight(const cvk_tl_shape_t *s, uint16_t before[])
{
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  uint32_t size = ic * oc * kh * kw;
  uint16_t *after = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!after)
    return NULL;

  /*
   * (oc, ic, kh, kw) -> (1, oc, kh * kw, ic)
   */
  for (uint32_t oci = 0; oci < oc; oci++) {
    for (uint32_t ici = 0; ici < ic; ici++) {
      for (uint32_t khi = 0; khi < kh; khi++) {
        for (uint32_t kwi = 0; kwi < kw; kwi++) {
          uint32_t src_i = oci * ic * kh * kw + ici * kh * kw + khi * kw + kwi;
          uint32_t dst_i = oci * kh * kw * ic + khi * kw * ic + kwi * ic + ici;
          after[dst_i] = before[src_i];
        }
      }
    }
  }

  return after;
}

static void put_conv_weight(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint16_t *data)
{
#if 0
  const cvk_tl_shape_t *s = &tl->shape;
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  bmshape_t bms = BM_TENSOR_INT8((int)oc, (int)ic, (int)kh, (int)kw*2);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  uint16_t *transformed_data = transform_weight(s, data);

  /* we put weight to region 1. bm_memcpy_s2d regard dev_mem as
   * absolute address, so we should pass abs address to copy weight
   * to right place.
   */

  //u64 ab_addr = bm_device_read_base_reg(*ctx, 1);
  //bmmem_device_t ab_dev_mem =
    //bmmem_device_prealloc(*ctx, NULL, ab_addr + gaddr, &bms);

  //int ret = bm_memcpy_s2d(*ctx, ab_dev_mem, (u8*)transformed_data);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, (u8*)transformed_data);

  assert(ret == BM_SUCCESS);
  cvk_tl_shape_t tdma_shape = { 1, oc, kh * kw, ic };

  tg_t tdma_tg;
  tdma_tg.base_reg_index = 0;
  tdma_tg.start_address = gaddr;
  tdma_tg.fmt = FMT_BF16;
  tdma_tg.shape.n = tdma_shape.n;
  tdma_tg.shape.c = tdma_shape.c;
  tdma_tg.shape.h = tdma_shape.h;
  tdma_tg.shape.w = tdma_shape.w;
  tdma_tg.stride = bmk1822_tensor_tgmem_default_stride(tdma_tg.shape, FMT_BF16);
  tdma_tg.base_reg_index = 1;

  tl_t tdma_tl = *tl;
  tdma_tl.shape = tdma_shape;
  tdma_tl.stride = bmk1822_tensor_lmem_default_stride(bk_ctx, tdma_shape, FMT_BF16, 0);

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tdma_tg;
  p.dst = &tdma_tl;

  bmk1822_tdma_g2l_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  bmmem_device_free(*ctx, dev_mem);
  //delete[] transformed_data;
  return transformed_data;
#endif

  const cvk_tl_shape_t *s = &tl->shape;
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  uint16_t *transformed_data = transform_weight(s, data);

  cvk_tl_shape_t tdma_shape = tl_shape_t4(1, oc, kh * kw, ic);
  cvk_tl_t tdma_tl;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tdma_tl, tdma_shape, tl->fmt, tl->eu_align);
  tdma_tl.start_address = tl->start_address;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, &tdma_tl, (uint8_t *)transformed_data);

  free(transformed_data);
}


static uint16_t * transform_bias(int oc, uint32_t before[])
{
  uint16_t *after = (uint16_t *)malloc(sizeof(uint16_t) * 2 * oc);
  if (!after)
    return NULL;

  for (int i = 0; i < oc; i++){
    after[i] = (before[i] >> 16) & 0xffff;
    after[i + oc] = before[i] & 0xffff;
  }
  return after;
}

static void put_conv_bias(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint32_t *data)
{
#if 0
  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2 * sizeof(short), oc, 1, 1);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  uint16_t *transformed_data = transform_bias(oc, data);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, (u8 *)transformed_data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_BF16;
  tg.shape.n = 2;
  tg.shape.c = oc;
  tg.shape.h = 1;
  tg.shape.w = 1;
  tg.stride = bmk1822_tensor_tgmem_default_stride(tg.shape, FMT_BF16);

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1822_tdma_g2l_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  bmmem_device_free(*ctx, dev_mem);
#endif

  int oc = tl->shape.c;
  uint16_t *transformed_data = transform_bias(oc, data);

  cvk_tl_shape_t tdma_shape = tl_shape_t4(2, oc, 1, 1);
  cvk_tl_t tdma_tl;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tdma_tl, tdma_shape, tl->fmt, tl->eu_align);
  tdma_tl.start_address = tl->start_address;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, &tdma_tl, (uint8_t *)transformed_data);

  free(transformed_data);
}

static int conv_kh_ext(const conv_param_t *p)
{
  return (p->kh - 1) * p->dh + 1;
}

static int conv_kw_ext(const conv_param_t *p)
{
  return (p->kw - 1) * p->dw + 1;
}

static int conv_ih_ext(const conv_param_t *p)
{
  return (p->input_h - 1) * (p->ins_h + 1) +
      p->ins_h_last + 1 + p->pad_top + p->pad_bot;
}

static int conv_iw_ext(const conv_param_t *p)
{
  return (p->input_w - 1) * (p->ins_w + 1) +
      p->ins_w_last + 1 + p->pad_left + p->pad_right;
}

static int conv_oh(const conv_param_t *p)
{
  return (conv_ih_ext(p) - conv_kh_ext(p)) / p->stride_h + 1;
}

static int conv_ow(const conv_param_t *p)
{
  return (conv_iw_ext(p) - conv_kw_ext(p)) / p->stride_w + 1;
}

static int conv_input_size(const conv_param_t *p)
{
  int in = p->input_n;
  int ic = p->input_c;
  int ih = p->input_h;
  int iw = p->input_w;
  return in * ic * ih * iw;
}

static int conv_output_size(const conv_param_t *p)
{
  int in = p->input_n;
  int oc = p->output_c;
  int oh = conv_oh(p);
  int ow = conv_ow(p);

  return in * oc * oh * ow;
}

static int conv_weight_size(const conv_param_t *p)
{
  int oc = p->output_c;
  int ic = p->input_c;
  int kh = p->kh;
  int kw = p->kw;
  return oc * ic * kh * kw;
}

static uint16_t * alloc_input(const conv_param_t *p)
{
  int size = conv_input_size(p);
  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (int i = 0; i < size; i++) {
    buf[i] = cvk_convert_fp32_bf16(i);
  }

  return buf;
}

static uint16_t * alloc_weight(const conv_param_t *p)
{
  int size = conv_weight_size(p);

  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (int i = 0; i < size; i++) {
    buf[i] = cvk_convert_fp32_bf16(i);
  }

  return buf;
}

static uint32_t * alloc_bias(const conv_param_t *p)
{
  int oc = p->output_c;

  uint32_t *bias = (uint32_t *)malloc(sizeof(uint32_t) * oc);
  if (!bias)
    return NULL;

  float val = 100;
  for (int i = 0; i < oc; i++) {
    bias[i] = cvk_convert_fp32_hex(val);
    val += 1;
  }
  return bias;
}

static cvk_tl_t * conv_ifmap_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 1);
}

static uint32_t conv_ifmap_tensor_size(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, s, fmt, 1);
}

static cvk_tl_t * conv_weight_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16; //p->opd1_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 0);
}

static uint32_t conv_weight_tensor_to_size(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16; //p->opd1_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, s, fmt, 0);
}

static cvk_tl_t * conv_ofmap_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_tl_shape_t s;
  s.n = p->input_n * 4;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  cvk_tl_t *tl = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, CVK_FMT_BF16, 1);
  if (tl)
    tl->shape.n = p->input_n;
  return tl;
}

static uint32_t conv_ofmap_tensor_to_size(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_tl_shape_t s;
  s.n = p->input_n * sizeof(uint32_t) / sizeof(uint8_t);
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, s, CVK_FMT_BF16, 1);
}

static cvk_tl_t * conv_bias_tensor(
    cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16;//p->opd2_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 0);
}

static uint32_t conv_bias_tensor_size(
    cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = CVK_FMT_BF16;//p->opd2_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, s, fmt, 0);
}

static int conv_param_is_ok(const conv_param_t *p)
{
  int kh_ext = conv_kh_ext(p);
  int kw_ext = conv_kw_ext(p);
  int ih_ext = conv_ih_ext(p);
  int iw_ext = conv_iw_ext(p);

  if ((kh_ext > ih_ext)
      || (kw_ext > iw_ext)
      || (kh_ext <= p->pad_top)
      || (kh_ext <= p->pad_bot)
      || (kw_ext <= p->pad_left)
      || (kw_ext <= p->pad_right)
      || (p->pad_top >= (1 << 4))
      || (p->pad_bot >= (1 << 4))
      || (p->pad_left >= (1 << 4))
      || (p->pad_right >= (1 << 4))) {
    return 0;
  }

  return 1;
}

static int bmk_conv_param_alloc_ok(
    const cvk_tiu_pt_convolution_param_t *p,
    const conv_param_t *param)
{
  if (!p->ifmap || !p->ofmap || !p->weight)
    return 0;
  if(p->ps32_mode==1)
      if (param->using_bias)
        if (!p->bias)
          return 0;

  return 1;
}

static void make_bmk_conv_param_ps32(
    cvk_context_t *cvk_ctx,
    cvk_tiu_pt_convolution_param_t *dst,
    const conv_param_t *p, uint32_t ps32_mode)
{
  memset(dst, 0, sizeof(*dst));

  dst->ins_h = p->ins_h;
  dst->ins_last_h = p->ins_h_last;
  dst->ins_w = p->ins_w;
  dst->ins_last_w = p->ins_w_last;
  dst->pad_top = p->pad_top;
  dst->pad_bottom = p->pad_bot;
  dst->pad_left = p->pad_left;
  dst->pad_right = p->pad_right;
  dst->stride_h = p->stride_h;
  dst->stride_w = p->stride_w;
  dst->dilation_h = p->dh;
  dst->dilation_w = p->dw;
  if(ps32_mode==2)
  {
    uint32_t ifmap_size = conv_ifmap_tensor_size(cvk_ctx, p);
    uint32_t weight_size = conv_weight_tensor_to_size(cvk_ctx, p);
    uint32_t ofmap_size = conv_ofmap_tensor_to_size(cvk_ctx, p);
    uint32_t bias_size = p->using_bias ? conv_bias_tensor_size(cvk_ctx, p) : 0;
    uint32_t total_size = ifmap_size + weight_size + ofmap_size + bias_size;

    // Allocation if size fit.
    if (total_size <= cvk_ctx->info.lmem_size) {
      dst->ifmap = conv_ifmap_tensor(cvk_ctx, p);
      dst->weight = conv_weight_tensor(cvk_ctx, p);
      dst->ofmap = conv_ofmap_tensor(cvk_ctx, p);
    } else {
      dst->ifmap = NULL;
      dst->weight = NULL;
      dst->ofmap = NULL;
    }
  }

  dst->ps32_mode = ps32_mode;

  dst->bias = NULL;
  dst->relu_enable = 0;
  dst->rshift_bits = 0;
  if(ps32_mode==1)
  {
    dst->relu_enable = p->bReLU_EN;
    dst->rshift_bits = p->r_shift_m;
    // only mode=1 can use bias
    if (p->using_bias)
      dst->bias = conv_bias_tensor(cvk_ctx, p);
  }
}

static void make_bmk_conv_param(
    cvk_context_t *cvk_ctx,
    cvk_tiu_pt_convolution_param_t *dst,
    const conv_param_t *p)
{
  memset(dst, 0, sizeof(*dst));

  uint32_t ifmap_size = conv_ifmap_tensor_size(cvk_ctx, p);
  uint32_t weight_size = conv_weight_tensor_to_size(cvk_ctx, p);
  uint32_t ofmap_size = conv_ofmap_tensor_to_size(cvk_ctx, p);
  uint32_t bias_size = p->using_bias ? conv_bias_tensor_size(cvk_ctx, p) : 0;
  uint32_t total_size = ifmap_size + weight_size + ofmap_size + bias_size;

  // Allocation if size fit.
  if (total_size <= cvk_ctx->info.lmem_size) {
    dst->ifmap = conv_ifmap_tensor(cvk_ctx, p);
    dst->weight = conv_weight_tensor(cvk_ctx, p);
    dst->ofmap = conv_ofmap_tensor(cvk_ctx, p);

    if (p->using_bias)
      dst->bias = conv_bias_tensor(cvk_ctx, p);
  } else {
    dst->ifmap = NULL;
    dst->weight = NULL;
    dst->ofmap = NULL;
  }

  dst->ins_h = p->ins_h;
  dst->ins_last_h = p->ins_h_last;
  dst->ins_w = p->ins_w;
  dst->ins_last_w = p->ins_w_last;
  dst->pad_top = p->pad_top;
  dst->pad_bottom = p->pad_bot;
  dst->pad_left = p->pad_left;
  dst->pad_right = p->pad_right;
  dst->stride_h = p->stride_h;
  dst->stride_w = p->stride_w;
  dst->dilation_h = p->dh;
  dst->dilation_w = p->dw;
  dst->relu_enable = p->bReLU_EN;
  dst->rshift_bits = p->r_shift_m;
  // dst->ifmap = conv_ifmap_tensor(cvk_ctx, p);
  // dst->weight = conv_weight_tensor(cvk_ctx, p);
  // dst->ofmap = conv_ofmap_tensor(cvk_ctx, p);
  // dst->bias = NULL;
  dst->ps32_mode = 0;
  // if (p->using_bias)
  //   dst->bias = conv_bias_tensor(cvk_ctx, p);
}

static void free_bmk_conv_param(
    cvk_context_t *cvk_ctx,
    cvk_tiu_pt_convolution_param_t *r,
    const conv_param_t *p)
{
  if (p->using_bias && r->bias)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->bias);

  if (r->ofmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ofmap);

  if (r->weight)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->weight);

  if (r->ifmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ifmap);
}

static void init_conv_param(conv_param_t *p)
{
  printf("init_conv_param\n");
  p->random_seed = clock();
  srand(p->random_seed);

retry:

  memset(p, 0, sizeof(*p));
  p->input_n = 1;
  p->input_c = rand() % (10) + 2;
  p->kh = rand() % 6 + 1;
  p->kw = rand() % 6 + 1;
  p->input_h = rand() % 10 + p->kh;
  p->input_w = rand() % 10 + p->kw;
  p->output_c = rand() % 10 + 3;
  p->stride_h = rand() % (p->kh) + 1;
  p->stride_w = rand() % (p->kw) + 1;
  p->ins_h = rand() % p->kh;
  p->ins_w = rand() % p->kw;
  p->ins_h_last = rand() % p->kh;;
  p->ins_w_last = rand() % p->kw;;
  p->dh = rand() % 3 + 1;
  p->dw = rand() % 3 + 1;

  int kh_ext = conv_kh_ext(p);
  int kw_ext = conv_kw_ext(p);
  p->pad_top = rand() % kh_ext;
  p->pad_bot = rand() % kh_ext;
  p->pad_left = rand() % kw_ext;
  p->pad_right = rand() % kw_ext;

  if (!conv_param_is_ok(p)) {
    printf("retry init_conv_param\n");
    goto retry;
  }

  p->using_bias = rand() % 2;
  p->r_shift_m = rand() % 8;
  p->bReLU_EN = rand() % 2;
  p->opd0_sign = rand() % 2;
  p->opd1_sign = 1;
  p->opd2_sign = 1;

  assert(p->opd1_sign == 1 && p->opd2_sign == 1);

  int ih_ext = conv_ih_ext(p);
  int iw_ext = conv_iw_ext(p);
  assert(ih_ext >= kh_ext);
  assert(iw_ext >= kw_ext);
}

static void print_conv_param(const conv_param_t *p)
{
  printf("%s\n", "Conv parameters:");
  printf("  %s%d;\n", "p->random_seed = ", p->random_seed);

  printf("  %s%d;\n", "p->input_n = ", p->input_n);
  printf("  %s%d;\n", "p->input_c = ", p->input_c);
  printf("  %s%d;\n", "p->input_h = ", p->input_h);
  printf("  %s%d;\n", "p->input_w = ", p->input_w);
  printf("  %s%d;\n", "p->output_c = ", p->output_c);

  printf("  %s%d;\n", "p->kh = ", p->kh);
  printf("  %s%d;\n", "p->kw = ", p->kw);
  printf("  %s%d;\n", "p->dh = ", p->dh);
  printf("  %s%d;\n", "p->dw = ", p->dw);
  printf("  %s%d;\n", "p->pad_top = ", p->pad_top);
  printf("  %s%d;\n", "p->pad_bot = ", p->pad_bot);
  printf("  %s%d;\n", "p->pad_left = ", p->pad_left);
  printf("  %s%d;\n", "p->pad_right = ", p->pad_right);
  printf("  %s%d;\n", "p->stride_h = ", p->stride_h);
  printf("  %s%d;\n", "p->stride_w = ", p->stride_w);
  printf("  %s%d;\n", "p->ins_w = ", p->ins_w);
  printf("  %s%d;\n", "p->ins_h = ", p->ins_h);
  printf("  %s%d;\n", "p->ins_w_last = ", p->ins_w_last);
  printf("  %s%d;\n", "p->ins_h_last = ", p->ins_h_last);

  printf("  %s%d;\n", "p->r_shift_m = ", p->r_shift_m);
  printf("  %s%d;\n", "p->opd0_sign = ", p->opd0_sign);
  printf("  %s%d;\n", "p->opd1_sign = ", p->opd1_sign);
  printf("  %s%d;\n", "p->opd2_sign = ", p->opd2_sign);
  printf("  %s%d;\n", "p->bReLU_EN = ", p->bReLU_EN);
  printf("  %s%d;\n", "p->using_bias = ", p->using_bias);

  printf("  %s%d\n", "kh_ext = ", conv_kh_ext(p));
  printf("  %s%d\n", "kw_ext = ", conv_kw_ext(p));
  printf("  %s%d\n", "ih_ext = ", conv_ih_ext(p));
  printf("  %s%d\n", "iw_ext = ", conv_iw_ext(p));
  printf("  %s%d\n", "output_h = ", conv_oh(p));
  printf("  %s%d\n", "output_w = ", conv_ow(p));
}

static int test_ps32_ut(
    conv_param_t *p_param, CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  printf("test_ps32_ut\n");
  int ret = 0;
  uint16_t *input = alloc_input(p_param);
  uint16_t *weight = alloc_weight(p_param);
  uint32_t *bias = alloc_bias(p_param);
  uint16_t *output_ref = (uint16_t *)malloc(sizeof(uint32_t) * conv_output_size(p_param));
  if (!input || !weight || !bias || !output_ref) {
    ret = -1;
    goto fail_exit;
  }

  ret = ps32_conv_ref(p_param, input, weight, bias, output_ref, 2);
  if (ret)
    goto fail_exit;

  cvk_tiu_pt_convolution_param_t conv_param;
  make_bmk_conv_param_ps32(cvk_ctx, &conv_param, p_param, 2);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, p_param);
  if (tl_alloc_success) {

    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, conv_param.ifmap, (uint8_t *)input);
    put_conv_weight(rt_handle, cvk_ctx, conv_param.weight, weight);
    cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &conv_param);
    cvk_tl_t ps32_ofmap;
    ps32_ofmap = *conv_param.ofmap;
    ps32_ofmap.shape.n = ps32_ofmap.shape.n * sizeof(short);
    uint16_t *output = (uint16_t*) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, &ps32_ofmap);

    ret = array_cmp_int8(
        "Comparing M2 begin_mode results ...\n",
        (int8_t*)output_ref, (int8_t *)output, conv_output_size(p_param) * sizeof(int));

    if (ret) {
      print_conv_param(p_param);
      printf("Comparison M2 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M2 PASS\n");

    free(output);
  }
  free_bmk_conv_param(cvk_ctx, &conv_param, p_param);
  
  printf("test_ps32_intermediate_mode\n");
  for (int i=0; i < conv_input_size(p_param); i++)
    input[i] = cvk_convert_fp32_bf16(i);

  for (int i=0; i < conv_weight_size(p_param); i++)
    weight[i] = cvk_convert_fp32_bf16(i);

  ret = ps32_conv_ref(p_param, input, weight, bias, output_ref, 3);
  if (ret)
    return ret;

  make_bmk_conv_param_ps32(cvk_ctx, &conv_param, p_param, 3);
  tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, p_param);

  if (tl_alloc_success) {
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, conv_param.ifmap, (uint8_t *)input);
    put_conv_weight(rt_handle, cvk_ctx, conv_param.weight, weight);

    cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &conv_param);

    cvk_tl_t ps32_ofmap;
    ps32_ofmap = *conv_param.ofmap;
    ps32_ofmap.shape.n = ps32_ofmap.shape.n * sizeof(short);

    uint16_t *output = (uint16_t*) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, &ps32_ofmap);

    ret = array_cmp_int8(
        "Comparing M3 intermediate results ...\n",
        (int8_t*)output_ref, (int8_t *)output, conv_output_size(p_param) * sizeof(int));

    if (ret) {
      print_conv_param(p_param);
      printf("Comparison M3 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M3 PASS\n");

    free(output);
  }
  free_bmk_conv_param(cvk_ctx, &conv_param, p_param);

  printf("test_ps32_end_mode\n");
  for (int i=0; i < conv_input_size(p_param); i++)
    input[i] = cvk_convert_fp32_bf16(i);

  for (int i=0; i < conv_weight_size(p_param); i++)
    weight[i] = cvk_convert_fp32_bf16(i);

  ret = ps32_conv_ref(p_param, input, weight, bias, output_ref, 1);
  if (ret)
    return ret;

  make_bmk_conv_param_ps32(cvk_ctx, &conv_param, p_param, 1);

  tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, p_param);

  if (tl_alloc_success) {

    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, conv_param.ifmap, (uint8_t *)input);
    put_conv_weight(rt_handle, cvk_ctx, conv_param.weight, weight);
    if (p_param->using_bias) {
      put_conv_bias(rt_handle, cvk_ctx, conv_param.bias, bias);
    }
    cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &conv_param);
    uint16_t *output = (uint16_t*) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, conv_param.ofmap);

    ret = array_cmp_int8(
        "Comparing M1 end results ...\n",
        (int8_t*)output_ref, (int8_t *)output, conv_output_size(p_param) * 2);

    if (ret) {
      print_conv_param(p_param);
      printf("Comparison M1 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M1 PASS\n");

    free(output);
  }
  free_bmk_conv_param(cvk_ctx, &conv_param, p_param);

fail_exit:
  free(input);
  free(weight);
  free(bias);
  free(output_ref);

  return ret;
}

static int test_ic_tiling_conv(
    conv_param_t *p_param, CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  printf("test tiled ps32 conv\n");
  int ret = 0;
  uint32_t output_size = sizeof(uint16_t) * conv_output_size(p_param);
  uint16_t *input = alloc_input(p_param);
  uint16_t *weight = alloc_weight(p_param);
  uint32_t *bias = alloc_bias(p_param);
  uint16_t *output_ref = (uint16_t *)malloc(output_size);
  if (!input || !weight || !bias || !output_ref) {
    ret = -1;
    goto fail_exit;
  }

  p_param->r_shift_m = 0;
  memset((uint8_t*)output_ref, 0, conv_output_size(p_param)*2);
  ret = ps32_conv_ref(p_param, input, weight, bias, output_ref, 0);
  if (ret)
    goto fail_exit;

  cvk_tiu_pt_convolution_param_t conv_tmp_param;
  cvk_tiu_pt_convolution_param_t conv_param;
  make_bmk_conv_param(cvk_ctx, &conv_param, p_param);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, p_param);
  if (tl_alloc_success) {
    if (p_param->using_bias) {
      conv_tmp_param.bias = conv_param.bias;
      put_conv_bias(rt_handle, cvk_ctx, conv_param.bias, bias);
    }
    // for ps32_md[1] = 1, relu_enable & rshift_bits need to set to 0
    // so we store those parameters to conv_tmp_para
    conv_tmp_param.relu_enable = conv_param.relu_enable;
    conv_tmp_param.rshift_bits = conv_param.rshift_bits;
    conv_tmp_param.bias        = conv_param.bias;

    uint32_t ic_step = 1;
    uint32_t n_step = 1;
    cvk_tl_t ifmap = *conv_param.ifmap;
    cvk_tl_t ofmap = *conv_param.ofmap;
    cvk_tg_shape_t s;
    s.n = conv_param.ifmap->shape.n;
    s.c = conv_param.ifmap->shape.c;
    s.h = conv_param.ifmap->shape.h;
    s.w = conv_param.ifmap->shape.w;
    cvk_tg_t *tg_ifmap = alloc_tensor_dev_mem(rt_handle, cvk_ctx, s, CVK_FMT_BF16);
    tensor_copy_s2d(rt_handle, tg_ifmap, (uint8_t *)input);

    s.n = conv_param.weight->shape.n;
    s.c = conv_param.weight->shape.c;
    s.h = conv_param.weight->shape.h;
    s.w = conv_param.weight->shape.w;
    uint16_t *transformed_weight =
      transform_weight(&conv_param.weight->shape, (uint16_t *)weight);
    cvk_tg_t *tg_weight = alloc_tensor_dev_mem(rt_handle, cvk_ctx, s, CVK_FMT_BF16);
    tensor_copy_s2d(rt_handle, tg_weight, (uint8_t *)transformed_weight);
    free(transformed_weight);

    cvk_tl_shape_t cur_tl_ifmap_shape = {
        n_step,
        ic_step,
        ifmap.shape.h,
        ifmap.shape.w
    };

    cvk_tg_shape_t cur_tg_ifmap_shape = {
      cur_tl_ifmap_shape.n,
      cur_tl_ifmap_shape.c,
      cur_tl_ifmap_shape.h,
      cur_tl_ifmap_shape.w
    };

    cvk_tg_stride_t cur_tg_ifmap_stride = {
      tg_ifmap->stride.n,
      tg_ifmap->stride.c,
      tg_ifmap->stride.h,
      fmt_size(CVK_FMT_BF16)
    };

    cvk_tg_t cur_tg_ifmap;
    cur_tg_ifmap.base_reg_index = 0;
    cur_tg_ifmap.start_address = tg_ifmap->start_address;
    cur_tg_ifmap.shape = cur_tg_ifmap_shape;
    cur_tg_ifmap.stride = cur_tg_ifmap_stride;
    cur_tg_ifmap.fmt = CVK_FMT_BF16;

    cvk_tl_t cur_tl_ifmap;
    cur_tl_ifmap.shape = cur_tl_ifmap_shape;
    cur_tl_ifmap.stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, cur_tl_ifmap_shape, CVK_FMT_BF16, 1);
    cur_tl_ifmap.start_address = ifmap.start_address;
    cur_tl_ifmap.fmt = ifmap.fmt;

    cvk_tl_t cur_tl_ofmap;
    cur_tl_ofmap.start_address = ofmap.start_address;
    cur_tl_ofmap.shape = ofmap.shape;
    cur_tl_ofmap.shape.n = n_step;
    cur_tl_ofmap.stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, cur_tl_ofmap.shape, CVK_FMT_BF16, 1);
    cur_tl_ofmap.fmt = ofmap.fmt;

    cvk_tl_t cur_tl_weight;
    cur_tl_weight.start_address = conv_param.weight->start_address;
    cur_tl_weight.shape = conv_param.weight->shape;
    cur_tl_weight.shape.n = ic_step;
    cur_tl_weight.stride.n = 2;
    cur_tl_weight.stride.c = cur_tl_weight.shape.n * cur_tl_weight.shape.h * cur_tl_weight.shape.w * 2;
    cur_tl_weight.stride.h = cur_tl_weight.shape.n * cur_tl_weight.shape.w * 2;
    cur_tl_weight.stride.w = cur_tl_weight.shape.n * 2;
    cur_tl_weight.fmt = conv_param.weight->fmt;

    const cvk_tl_t *saved_tl_weight = conv_param.weight;
    const cvk_tl_t *saved_tl_ifmap = conv_param.ifmap;
    for (uint32_t ci = 0; ci < ifmap.shape.c; ci += ic_step) {
      {
        uint32_t ic = tg_weight->shape.n;
        uint32_t oc = tg_weight->shape.c;
        uint32_t kh = tg_weight->shape.h;
        uint32_t kw = tg_weight->shape.w;

        cvk_tg_t cur_tdma_tg_weight;
        cur_tdma_tg_weight.base_reg_index = tg_weight->base_reg_index;
        cur_tdma_tg_weight.start_address = tg_weight->start_address + ci * (tg_weight->fmt == CVK_FMT_BF16 ? 2 : 1);
        cur_tdma_tg_weight.fmt = tg_weight->fmt;
        cur_tdma_tg_weight.shape = tg_shape_t4(1, oc, kh * kw, ic);
        cur_tdma_tg_weight.stride =
          cvk_ctx->ops->tg_default_stride(cvk_ctx, cur_tdma_tg_weight.shape, CVK_FMT_BF16);
        cur_tdma_tg_weight.shape = tg_shape_t4(1, oc, kh * kw, ic_step);

        cvk_tl_t cur_tdma_tl_weight;
        cur_tdma_tl_weight = cur_tl_weight;
        cur_tdma_tl_weight.shape.n = cur_tdma_tg_weight.shape.n;
        cur_tdma_tl_weight.shape.c = cur_tdma_tg_weight.shape.c;
        cur_tdma_tl_weight.shape.h = cur_tdma_tg_weight.shape.h;
        cur_tdma_tl_weight.shape.w = cur_tdma_tg_weight.shape.w;
        cur_tdma_tl_weight.stride = cvk_ctx->ops->tl_default_stride(
            cvk_ctx, cur_tdma_tl_weight.shape, cur_tdma_tl_weight.fmt, 0);

        cvk_tdma_g2l_tensor_copy_param_t p1;
        memset(&p1, 0, sizeof(p1));
        p1.src = &cur_tdma_tg_weight;
        p1.dst = &cur_tdma_tl_weight;
        cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p1);
        CVI_RT_Submit(cvk_ctx);
      }
      {
        cvk_tdma_g2l_tensor_copy_param_t p2;
        memset(&p2, 0, sizeof(p2));
        cur_tg_ifmap.start_address =
          tg_ifmap->start_address + ci * tg_ifmap->stride.c;
        p2.src = &cur_tg_ifmap;
        p2.dst = &cur_tl_ifmap;
        cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p2);
        CVI_RT_Submit(cvk_ctx);
      }

      conv_param.ifmap = &cur_tl_ifmap;
      conv_param.weight = &cur_tl_weight;

      // for ps32_md[1] = 1, relu_enable & rshift_bits need to set to 0
      // so we store those parameters to conv_tmp_para
      if (ci == (ifmap.shape.c - 1))
      {
        conv_param.relu_enable = conv_tmp_param.relu_enable;
        conv_param.rshift_bits = conv_tmp_param.rshift_bits;
        conv_param.bias        = conv_tmp_param.bias;
        conv_param.ps32_mode   = 1;
      }
      else if (ci == 0)
      {
        conv_param.relu_enable = 0;
        conv_param.rshift_bits = 0;
        conv_param.bias        = 0;;
        conv_param.ps32_mode   = 2;
      }
      else
      {
        conv_param.relu_enable = 0;
        conv_param.rshift_bits = 0;
        conv_param.bias        = 0;;
        conv_param.ps32_mode   = 3;
      }
      cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &conv_param);
      conv_param.weight = saved_tl_weight;
      conv_param.ifmap = saved_tl_ifmap;
    }

    uint16_t *output = (uint16_t*) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, conv_param.ofmap);

    free_tensor_dev_mem(rt_handle, tg_ifmap);
    free_tensor_dev_mem(rt_handle, tg_weight);

    ret = array_cmp_int8(
        "Comparing results ...\n",
        (int8_t*) output_ref, (int8_t *)output, conv_output_size(p_param)*2);
    if (ret) {
      print_conv_param(p_param);
      printf("Comparison FAILED\n");
      ret = -1;
    }
    free(output);
  }
  free_bmk_conv_param(cvk_ctx, &conv_param, p_param);

fail_exit:
  free(input);
  free(weight);
  free(output_ref);
  free(bias);

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }
  int round_mode;
  round_mode = cvk_set_store_feround();

  for (int i = 0; i < 15; i++) {
    printf("random_test_conv iteration: %d\n", i);
    conv_param_t test_conv_param;
    init_conv_param(&test_conv_param);
    //print_conv_param(&test_conv_param);
    ret |= test_ic_tiling_conv(&test_conv_param, rt_handle, cvk_ctx);
    if (ret)
      break;
    ret |= test_ps32_ut(&test_conv_param, rt_handle, cvk_ctx);
    if (ret)
      break;

    if (!test_conv_param.using_bias)
      test_conv_param.using_bias = 1;
    if (test_conv_param.output_c <= 9)
      test_conv_param.output_c += 3;
    //print_conv_param(&test_conv_param);
    ret |= test_ic_tiling_conv(&test_conv_param, rt_handle, cvk_ctx);
    if (ret)
      break;

    ret |= test_ps32_ut(&test_conv_param, rt_handle, cvk_ctx);
    if (ret)
      break;
  }
  cvk_restore_feround(round_mode);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
