#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

#define INVALIDE_STRIDE (-1)
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
} conv_param_t;

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

static int matrix_dot_mult(
    int8_t *A, int8_t *B, int dim_n, int dim_m,
    int opd0_sign)
{
  int sum = 0;
  for (int i=0; i<dim_n; i++){
    for (int j=0; j<dim_m; j++){
      int index = index_get(i, dim_m, j);
      if(opd0_sign) {
        sum += A[index] * B [index];
      } else {
        sum += (int)((uint8_t)A[index]) * B [index];
      }
    }
  }
  return sum;
}

static int conv_ref(
    const conv_param_t *p_param,
    const int8_t *ifmap,
    const int8_t *weight,
    const int16_t *bias,
    int8_t *ofmap)
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
  int input_sign = p_param->opd0_sign;
  int r_shift_bits = p_param->r_shift_m;
  int do_relu = p_param->bReLU_EN;

  int ih_ext = calc_dilute_hw(ih, ins_h, ins_h_last,  pad_top, pad_bot);
  int iw_ext = calc_dilute_hw(iw, ins_w, ins_w_last,  pad_left, pad_right);
  int kh_ext = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int kw_ext = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int oh = calc_output_hw(ih_ext, kh_ext, stride_h);
  int ow = calc_output_hw(iw_ext, kw_ext, stride_w);

  int *result = (int *)malloc(sizeof(int) * in * oc * oh * ow);
  int8_t *i_fmap_pad_ker = (int8_t *)malloc(kh_ext * kw_ext);
  if (!result || !i_fmap_pad_ker) {
    free(result);
    free(i_fmap_pad_ker);
    return -1;
  }

  memset(result, 0, sizeof(int) * in * oc * oh * ow);

  int ret = 0;

  int8_t *i_fmap_pad = NULL;
  int8_t *kernel_after = NULL;
  for (int n = 0; n < in; ++n) {
    for (int c = 0; c < oc; ++c) {
      for (int cc = 0; cc < ic; ++cc) {
        fill_pad_fmap_int8(
            (int8_t*)ifmap + n*ic*ih*iw + cc*ih*iw, &i_fmap_pad, 0,
            pad_left, pad_right, pad_top, pad_bot,
            ins_h, ins_w, ins_h_last, ins_w_last, ih, iw);

        //kernel_dilation(
        fill_pad_fmap_int8(
            (weight + c*ic*kh*kw + cc*kh*kw), &kernel_after, 0,
            0, 0, 0, 0,  // no padding
            dh - 1, dw - 1, 0, 0,
            kh, kw);

        for (int ph = 0; ph < oh; ++ph) {
          for (int pw = 0; pw < ow; ++pw) {
            for (int idxh = 0; idxh < kh_ext; ++idxh)
              for (int idxw = 0; idxw < kw_ext; ++idxw){
                i_fmap_pad_ker[idxh * kw_ext + idxw] =
                    i_fmap_pad[(idxh+ph*stride_h) * iw_ext +
                               idxw + pw*stride_w];
              }
            result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] +=
                matrix_dot_mult(i_fmap_pad_ker, kernel_after,
                                kh_ext, kw_ext, input_sign);
          }
        }
      }

      if (p_param->using_bias) {
        for (int ph = 0; ph < oh; ++ph) {
          for (int pw = 0; pw < ow; ++pw) {
            result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] += bias[c]; //bias+c ;
          }
        }
      }

      if (do_relu)
        relu(&result[n*oc*oh*ow + c*oh*ow], oh * ow);

      // ofmap is int8_t, signed
      ret = satu_2_8bit(&result[n*oc*oh*ow + c*oh*ow], oh * ow,
                        &ofmap[n*oc*oh*ow + c*oh*ow], r_shift_bits, /*round_floor=*/1,
                        /*sign_unsign=*/1);

      if (ret)
        goto error_release;
    } //end for (int c = 0; c < oc; ++c)
  } //end for (int n = 0; n < in; n++)

error_release:
  free(i_fmap_pad);
  free(kernel_after);
  free(i_fmap_pad_ker);
  free(result);

  return ret;
}

static uint8_t * transform_weight(const cvk_tl_shape_t *s, uint8_t before[])
{
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  uint32_t size = ic * oc * kh * kw;
  uint8_t *after = (uint8_t *)malloc(size);

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
    uint8_t *data)
{
#if 0
  const cvk_tl_shape_t *s = &tl->shape;
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  bmshape_t bms = BM_TENSOR_INT8((int)oc, (int)ic, (int)kh, (int)kw);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  uint8_t *transformed_data = transform_weight(s, data);

  /* we put weight to region 1. bm_memcpy_s2d regard dev_mem as
   * absolute address, so we should pass abs address to copy weight
   * to right place.
   */

  //u64 ab_addr = bm_device_read_base_reg(*ctx, 1);
  //bmmem_device_t ab_dev_mem =
    //bmmem_device_prealloc(*ctx, NULL, ab_addr + gaddr, &bms);

  //int ret = bm_memcpy_s2d(*ctx, ab_dev_mem, transformed_data);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, transformed_data);
  assert(ret == BM_SUCCESS);

  cvk_tl_shape_t tdma_shape = { 1, oc, kh * kw, ic };

  tg_t tdma_tg;
  tdma_tg.base_reg_index = 0;
  tdma_tg.start_address = gaddr;
  tdma_tg.fmt = FMT_I8;
  tdma_tg.shape.n = tdma_shape.n;
  tdma_tg.shape.c = tdma_shape.c;
  tdma_tg.shape.h = tdma_shape.h;
  tdma_tg.shape.w = tdma_shape.w;
  tdma_tg.stride = bmk1822_tensor_tgmem_default_stride(tdma_tg.shape, tdma_tg.fmt);
  tdma_tg.base_reg_index = 1;

  tl_t tdma_tl = *tl;
  tdma_tl.shape = tdma_shape;
  tdma_tl.stride = bmk1822_tensor_lmem_default_stride(cvk_ctx, tdma_shape, FMT_I8, 0);

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tdma_tg;
  p.dst = &tdma_tl;

  bmk1822_tdma_g2l_tensor_copy(cvk_ctx, &p);
  test_submit(ctx);
  bmmem_device_free(*ctx, dev_mem);
  delete[] transformed_data;
#endif

  const cvk_tl_shape_t *s = &tl->shape;
  uint32_t ic = s->n;
  uint32_t oc = s->c;
  uint32_t kh = s->h;
  uint32_t kw = s->w;

  uint8_t *transformed_data = transform_weight(s, data);

  cvk_tl_shape_t tdma_shape = tl_shape_t4(1, oc, kh * kw, ic);
  cvk_tl_t tdma_tl;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tdma_tl, tdma_shape, tl->fmt, tl->eu_align);
  tdma_tl.start_address = tl->start_address;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, &tdma_tl, transformed_data);

  free(transformed_data);
}

static int8_t * transform_bias(int oc, int16_t before[])
{
  int8_t *after = (int8_t *)malloc(2 * oc);
  if (!after)
    return NULL;

  for (int i = 0; i < oc; i++){
    after[i] = before[i] & 0xff;
    after[i + oc] = (before[i] >> 8) & 0xff;
  }
  return after;
}

static void put_conv_bias(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    int16_t *data)
{
#if 0
  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2, oc, 1, 1);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  int8_t *transformed_data = transform_bias(oc, data);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, (uint8_t *)transformed_data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape.n = 2;
  tg.shape.c = oc;
  tg.shape.h = 1;
  tg.shape.w = 1;
  tg.stride = bmk1822_tensor_tgmem_default_stride(tg.shape, tg.fmt);

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1822_tdma_g2l_tensor_copy(cvk_ctx, &p);
  test_submit(ctx);

  bmmem_device_free(*ctx, dev_mem);
  delete[] transformed_data;
#endif

  int oc = tl->shape.c;
  int8_t *transformed_data = transform_bias(oc, data);

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

static int8_t * alloc_input(const conv_param_t *p)
{
  int size = conv_input_size(p);

  int8_t *buf = (int8_t *)malloc(sizeof(int8_t) * size);
  if (!buf)
    return NULL;

  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static int8_t * alloc_weight(const conv_param_t *p)
{
  int size = conv_weight_size(p);

  int8_t *buf = (int8_t *)malloc(sizeof(int8_t) * size);
  if (!buf)
    return NULL;

  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static int16_t * alloc_bias(const conv_param_t *p)
{
  int oc = p->output_c;

  int16_t *bias = (int16_t *)malloc(sizeof(int16_t) * oc);
  if (!bias)
    return NULL;

  for (int i = 0; i < oc; i++)
    bias[i] = rand() % 65536 - 32768;

  return bias;
}

static cvk_tl_t * conv_ifmap_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = p->opd0_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 1);
}

static cvk_tl_t * conv_weight_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = p->opd1_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 0);
}

static cvk_tl_t * conv_ofmap_tensor(cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_tl_shape_t s;
  s.n = p->input_n;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, CVK_FMT_I8, 1);
}

static cvk_tl_t * conv_bias_tensor(
    cvk_context_t *cvk_ctx, const conv_param_t *p)
{
  cvk_fmt_t fmt = p->opd2_sign? CVK_FMT_I8: CVK_FMT_U8;
  cvk_tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, s, fmt, 0);
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

  if (param->using_bias)
    if (!p->bias)
      return 0;

  return 1;
}

static void make_bmk_conv_param(
    cvk_context_t *cvk_ctx,
    cvk_tiu_pt_convolution_param_t *dst,
    const conv_param_t *p)
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
  dst->relu_enable = p->bReLU_EN;
  dst->rshift_bits = p->r_shift_m;

  dst->ifmap = conv_ifmap_tensor(cvk_ctx, p);
  dst->weight = conv_weight_tensor(cvk_ctx, p);
  dst->ofmap = conv_ofmap_tensor(cvk_ctx, p);
  dst->bias = NULL;
  dst->ps32_mode = 0;
  if (p->using_bias)
    dst->bias = conv_bias_tensor(cvk_ctx, p);

  dst->w_is_const = 0;
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

static void _init_conv_param(conv_param_t *p, int stride_w, int stride_h)
{
  printf("init_conv_param\n");
  memset(p, 0, sizeof(*p));
  p->random_seed = clock();
  srand(p->random_seed);

retry:
  p->input_n = rand() % 5 + 1;
  p->input_c = rand() % (5 * 32) + 1;
  p->kh = rand() % 7 + 1;
  p->kw = rand() % 7 + 1;
  p->input_h = rand() % 40 + p->kh + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
  p->input_w = rand() % 40 + p->kw + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);
  p->output_c = rand() % 10 + 3;
  p->stride_h = INVALIDE_STRIDE == stride_h ? rand() % (p->kh) + 1 : stride_h;
  p->stride_w = INVALIDE_STRIDE == stride_w ? rand() % (p->kh) + 1 : stride_w;
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

static void init_conv_param(conv_param_t *p) {
  _init_conv_param(p, INVALIDE_STRIDE, INVALIDE_STRIDE);
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

// Calculate the right shift value, m
// Steps:
//  1. Get the abs() of each weight;
//  2. Summary all the abs() in one kernel;
//  3. Get Log2 of each sum;
//  4. Downward rounding;
// After every r_shift value got, sort and find the middle one.
static int calc_rshift_m(const conv_param_t *p, int8_t* weight)
{
  int kernel_cnt = p->output_c * p->input_c;
  int kernel_size = p->kh * p->kw;
  int *kernel_shifts = (int *)malloc(sizeof(int) * kernel_cnt);
  if (!kernel_shifts)
    return 1;

  memset(kernel_shifts, 0, sizeof(int) * kernel_cnt);

  // Part 1:
  // Get right shift value for each kernel
  int sum = 0;
  for (int i = 0; i < kernel_cnt; i++) {
    // Step 1 & 2: Get the sum of abs()
    for (int j = 0; j < kernel_size; j++) {
      sum += (int)(*weight < 0 ? -(*weight) : (*weight));
      weight++;
    }
    // Step 3 & 4: log2 and downward rounding
    sum >>= 1;
    while (sum) {
      sum >>= 1;
      kernel_shifts[i]++;
    }
  }

  // Part 2:
  // Find the middle of all the values
  int tag[32] = {0};
  for (int cnt = 0; cnt < kernel_cnt; cnt++) {
    if (kernel_shifts[cnt] < 32)
      tag[kernel_shifts[cnt]]++;
  }

  int rshift_m = 0;
  int mid = 0;
  do {
    mid += tag[rshift_m++];
  } while(mid < (kernel_cnt - 1) >> 1);

  free(kernel_shifts);

  return rshift_m - 1;
}

static int test_conv(
    conv_param_t *p_param, CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  int8_t *input = alloc_input(p_param);
  int8_t *weight = alloc_weight(p_param);
  int16_t *bias = alloc_bias(p_param);
  int8_t *output_ref = (int8_t *)malloc(sizeof(int8_t) * conv_output_size(p_param));
  if (!input || !weight || !bias || !output_ref) {
    ret = -1;
    goto fail_exit;
  }

  p_param->r_shift_m = calc_rshift_m(p_param, weight);

  ret = conv_ref(p_param, input, weight, bias, output_ref);
  if (ret)
    goto fail_exit;

  cvk_tiu_pt_convolution_param_t conv_param;
  make_bmk_conv_param(cvk_ctx, &conv_param, p_param);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, p_param);
  if (tl_alloc_success) {
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, conv_param.ifmap, (uint8_t *)input);
    put_conv_weight(rt_handle, cvk_ctx, conv_param.weight, (uint8_t *)weight);
    if (p_param->using_bias)
      put_conv_bias(rt_handle, cvk_ctx, conv_param.bias, bias);
    cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &conv_param);
    uint8_t *output = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, conv_param.ofmap);

    ret = array_cmp_int8(
        "Comparing results ...\n",
        output_ref, (int8_t *)output, conv_output_size(p_param));

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

  for (int i = 0; i < 5; i++) {
    printf("random_test_conv iteration: %d\n", i);
    conv_param_t test_conv_param;
    init_conv_param(&test_conv_param);
    ret |= test_conv(&test_conv_param, rt_handle, cvk_ctx);
    if (!test_conv_param.using_bias)
      test_conv_param.using_bias = 1;
    if (test_conv_param.output_c <= 32)
      test_conv_param.output_c += 32;
    ret |= test_conv(&test_conv_param, rt_handle, cvk_ctx);
  }

  // test stride extend (0, 31]
  int stride_list[] = {15, 16, 31};
  int stride_list_len = sizeof(stride_list) / sizeof(stride_list[0]);

  for (int stride_w_idx = 0; stride_w_idx < stride_list_len; stride_w_idx++) {
    for (int stride_h_idx = 0; stride_h_idx < stride_list_len; stride_h_idx++) {
      int stride_w = stride_list[stride_w_idx];
      int stride_h = stride_list[stride_h_idx];
      conv_param_t test_conv_param;
      _init_conv_param(&test_conv_param, stride_w, stride_h);

      ret |= test_conv(&test_conv_param, rt_handle, cvk_ctx);
    }
  }

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
