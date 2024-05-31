#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tdma_l2g_tensor_copy_cw_transposed_param_t l2g_cw_param_t;
typedef cvk_tdma_g2l_matrix_copy_row_col_transposed_param_t g2l_matrix_param_t;
typedef cvk_tdma_l2l_tensor_copy_param_t l2l_tensor_copy_param_t;

typedef struct{
    int8_t *conv_input;
    int8_t *conv_weight;
    int16_t *conv_bias;
    uint8_t *conv_output;
    int8_t *conv_output_ref;
    uint8_t *l2g_cw_src;
    uint8_t *l2g_cw_output;
    uint8_t *l2g_cw_output_ref;
    uint8_t *g2l_matrix_src;
    uint8_t *g2l_matrix_output;
    uint8_t *g2l_matrix_output_ref;
    uint8_t *l2l_tensor_src;
    uint8_t *l2l_tensor_output;
    uint8_t *l2l_tensor_output_ref;
}s_test_data;

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

conv_param_t conv_param;
l2g_cw_param_t l2g_cw_param;
g2l_matrix_param_t g2l_matrix_param;
l2l_tensor_copy_param_t l2l_tensor_copy_param;
s_test_data s8_test_data;
cvk_tiu_pt_convolution_param_t bmk_conv_param;

cvk_tl_t *skip_tensor_lmem[10];
uint32_t skip_tensor_num=0;

/* need to make sure the free order of test_alloc_tl for skip_tensor_lmem*/
void skip_tensor_lmem_size(cvk_context_t *cvk_ctx, const cvk_tl_t *p)
{
  uint32_t needed = align_up(p->shape.n * p->stride.n, cvk_ctx->info.eu_num);
  uint32_t start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  uint32_t remain_size = start_addr % cvk_ctx->info.lmem_bank_size ? (cvk_ctx->info.lmem_bank_size - start_addr % cvk_ctx->info.lmem_bank_size) : 0; // remain size for each lane
  if(remain_size)
  {
//    cvk_tl_shape_t src_shape2 = {1, cvk_ctx->info.eu_num, 1, remain_size};
    cvk_tl_shape_t src_shape2 = {1, cvk_ctx->info.npu_num, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape2, CVK_FMT_I8, 1); // skip the lmem size and next tl can alignment to bank size
  }
  skip_tensor_num++;
}

void skip_matrix_lmem_size(cvk_context_t *cvk_ctx, const cvk_ml_t *p)
{
  uint32_t needed = align_up(p->shape.n * p->stride.n, cvk_ctx->info.eu_num);
  uint32_t start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  uint32_t remain_size = start_addr % cvk_ctx->info.lmem_bank_size ? (cvk_ctx->info.lmem_bank_size - start_addr % cvk_ctx->info.lmem_bank_size) : 0; // remain size for each lane
  if(remain_size)
  {
    cvk_tl_shape_t src_shape2 = {1, cvk_ctx->info.npu_num, 1, remain_size};
    //cvk_tl_shape_t src_shape2 = {1, cvk_ctx->info.eu_num, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape2, CVK_FMT_I8, 1); // skip the lmem size and next tl can alignment to bank size
  }
  skip_tensor_num++;
}

void free_skip_tensor_lmem(cvk_context_t *cvk_ctx)
{
  if(skip_tensor_lmem[--skip_tensor_num]!=NULL)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, skip_tensor_lmem[skip_tensor_num]);
}

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
    uint8_t *data)
{
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

  printf("ifmap shape(%d, %d, %d, %d)\n",
         p->ifmap->shape.n, p->ifmap->shape.c,
         p->ifmap->shape.h, p->ifmap->shape.w);
  printf("ofmap shape(%d, %d, %d, %d)\n",
         p->ofmap->shape.n, p->ofmap->shape.c,
         p->ofmap->shape.h, p->ofmap->shape.w);

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
  skip_tensor_lmem_size(cvk_ctx, dst->ifmap);
  dst->weight = conv_weight_tensor(cvk_ctx, p);
  skip_tensor_lmem_size(cvk_ctx, dst->weight);
  dst->ofmap = conv_ofmap_tensor(cvk_ctx, p);
  skip_tensor_lmem_size(cvk_ctx, dst->ofmap);
  dst->bias = NULL;
  dst->ps32_mode = 0;
  if (p->using_bias)
  {
    dst->bias = conv_bias_tensor(cvk_ctx, p);
    skip_tensor_lmem_size(cvk_ctx, dst->bias);
  }
  dst->w_is_const = 0;
}

static void free_bmk_conv_param(
    cvk_context_t *cvk_ctx,
    cvk_tiu_pt_convolution_param_t *r,
    const conv_param_t *p)
{
  if (p->using_bias && r->bias)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->bias);
  }
  if (r->ofmap)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ofmap);
  }
  if (r->weight)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->weight);
  }
  if (r->ifmap)
  {
    free_skip_tensor_lmem(cvk_ctx);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ifmap);
  }
}

static void init_conv_param(conv_param_t *p)
{
retry:
  p->input_n = 1;
  p->input_c = 8; // 16 -> 8 for 180x
  p->input_h = 2;
  p->input_w = 600;

  p->kh = 2;
  p->kw = 16;
  p->output_c = 8; // 16 -> 8 for 180x

  p->stride_h = 1;
  p->stride_w = 15;
  p->ins_h = 0;
  p->ins_w = 0;
  p->ins_h_last = 0;;
  p->ins_w_last = 0;;
  p->dh = 1;
  p->dw = 1;

  int kh_ext = conv_kh_ext(p);
  int kw_ext = conv_kw_ext(p);
  p->pad_top = 1;
  p->pad_bot = 0;
  p->pad_left = 0;
  p->pad_right = 0;

  if (!conv_param_is_ok(p)) {
    printf("retry init_conv_param\n");
    goto retry;
  }

  p->using_bias = 0;
  p->r_shift_m = 7;
  p->bReLU_EN = 1;

  p->opd0_sign = 0;
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


static void l2g_tensor_copy_cw_transposed_ref(
    l2g_cw_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  cvk_tl_shape_t s = p->src->shape;
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = s.w;

  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hi = 0; hi < h; hi++) {
        for (uint32_t wi = 0; wi < w; wi++) {
          uint32_t src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          uint32_t dst_i = ni * c * h * w + wi * h * c + hi * c + ci;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  s8_test_data.l2g_cw_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2g_cw_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.l2g_cw_src[i] = rand()%0x100;

  s8_test_data.l2g_cw_output_ref = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2g_cw_output_ref)
    return;

  l2g_tensor_copy_cw_transposed_ref(p, s8_test_data.l2g_cw_output_ref, s8_test_data.l2g_cw_src);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, s8_test_data.l2g_cw_src);
}

static void destroy_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  free_tensor_dev_mem(rt_handle, p->dst);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void test_l2g_cw_transpose(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2g_cw_param_t *p)
{
  cvk_tl_shape_t src_shape = {1, 0x100, 1, 0x020};
  cvk_tg_shape_t dst_shape = {1, 0x020, 1, 0x100};

//  cvk_tl_shape_t src_shape = {1, 0x100, 1, 0x080};
//  cvk_tg_shape_t dst_shape = {1, 0x080, 1, 0x100};

  p->src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape, CVK_FMT_I8, 1);
  p->dst = alloc_tensor_dev_mem(rt_handle, cvk_ctx, dst_shape, CVK_FMT_I8);

  printf("l2g cw src shape(%d, %d, %d, %d)\n",
         p->src->shape.n, p->src->shape.c,
         p->src->shape.h, p->src->shape.w);
  printf("l2g cw dst shape(%d, %d, %d, %d)\n",
         p->dst->shape.n, p->dst->shape.c,
         p->dst->shape.h, p->dst->shape.w);

  skip_tensor_lmem_size(cvk_ctx, p->src);
  test_param_l2g(rt_handle, cvk_ctx, p);
}

static void g2l_matrix_copy_row_col_transposed_ref(
    g2l_matrix_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t row = p->src->shape.row;
  uint64_t col = p->src->shape.col;

  for (uint64_t ri = 0; ri < row; ri++) {
    for (uint64_t ci = 0; ci < col; ci++) {
      uint64_t src_i = ri * col + ci;
      uint64_t dst_i = ci * row + ri;
      ref_data[dst_i] = src_data[src_i];
    }
  }
}

static void test_param_g2l(CVI_RT_HANDLE rt_handle, g2l_matrix_param_t *p)
{
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  s8_test_data.g2l_matrix_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.g2l_matrix_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.g2l_matrix_src[i] = rand()%0x100;

  s8_test_data.g2l_matrix_output_ref = (uint8_t *)malloc(size);
  if (!s8_test_data.g2l_matrix_output_ref)
    return;

  g2l_matrix_copy_row_col_transposed_ref(p, s8_test_data.g2l_matrix_output_ref, s8_test_data.g2l_matrix_src);

  matrix_copy_s2d(rt_handle, p->src, s8_test_data.g2l_matrix_src);
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, g2l_matrix_param_t *p)
{
  free_matrix_dev_mem(rt_handle, p->src);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dst);
}


static void test_g2l_matrix_transpose(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, g2l_matrix_param_t *p)
{
  //g2l_matrix_param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  cvk_mg_shape_t src_shape={0x100, 0x20};
  cvk_ml_shape_t dst_shape={0x20, 0x10, 0x10, 0x100};

//  cvk_mg_shape_t src_shape={0x100, 0x80};
//  cvk_ml_shape_t dst_shape={0x80, 0x10, 0x10, 0x100};

  int dst_align = 1;
  cvk_fmt_t fmt = CVK_FMT_I8;

  p->src = alloc_matrix_dev_mem(rt_handle, src_shape, fmt);
  p->dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, dst_shape, fmt, dst_align);

  printf("g2l matrix tp src shape(row=%d, col=%d)\n",
         p->src->shape.row, p->src->shape.col);
  printf("gl2 matrix tp dst shape(n=%d, c=%d, w=%d, col=%d)\n",
         p->dst->shape.n, p->dst->shape.c,
         p->dst->shape.w, p->dst->shape.col);

  skip_matrix_lmem_size(cvk_ctx, p->dst);
  test_param_g2l(rt_handle, p);
}

static void l2l_tensor_copy_ref(l2l_tensor_copy_param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  for (uint64_t i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_l2l_param(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  s8_test_data.l2l_tensor_src = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2l_tensor_src)
    return;

  for (uint64_t i = 0; i < size; i++)
    s8_test_data.l2l_tensor_src[i] = rand()%0x100;

  s8_test_data.l2l_tensor_output_ref = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!s8_test_data.l2l_tensor_output_ref)
    return;

  l2l_tensor_copy_ref(p, s8_test_data.l2l_tensor_output_ref, s8_test_data.l2l_tensor_src);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, s8_test_data.l2l_tensor_src);
}

static void destroy_param_l2l(cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
  free_skip_tensor_lmem(cvk_ctx);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void test_l2l_tensor_copy(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, l2l_tensor_copy_param_t *p)
{
  cvk_tl_shape_t src_shape = {1, 0x4, 0x1, 0x40}; // for 180x
  cvk_tl_shape_t dst_shape = {1, 0x4, 0x1, 0x40}; // for 180x

  //cvk_tl_shape_t src_shape = {1, 0x10, 0x1, 0x100};
  //cvk_tl_shape_t dst_shape = {1, 0x10, 0x1, 0x100};

//  cvk_tl_shape_t src_shape = {1, 0x10, 0x1, 0x400};
//  cvk_tl_shape_t dst_shape = {1, 0x10, 0x1, 0x400};

  p->src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, src_shape, CVK_FMT_I8, 1);

  printf("l2l src shape(%d, %d, %d, %d)\n",
         p->src->shape.n, p->src->shape.c,
         p->src->shape.h, p->src->shape.w);

  skip_tensor_lmem_size(cvk_ctx, p->src);
  p->dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, dst_shape, CVK_FMT_I8, 1);
  skip_tensor_lmem_size(cvk_ctx, p->dst);
  test_l2l_param(rt_handle, cvk_ctx, p);
}

static int setup_conv(
    conv_param_t *p_param, CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  s8_test_data.conv_input = alloc_input(p_param);
  s8_test_data.conv_weight = alloc_weight(p_param);
  s8_test_data.conv_bias = alloc_bias(p_param);
  p_param->r_shift_m = calc_rshift_m(p_param, s8_test_data.conv_weight);
  s8_test_data.conv_output_ref = (int8_t *)malloc(sizeof(int8_t) * conv_output_size(p_param));
  if (!s8_test_data.conv_output_ref)
    return -1;

  int ret = conv_ref(p_param, s8_test_data.conv_input, s8_test_data.conv_weight, s8_test_data.conv_bias, s8_test_data.conv_output_ref);
  if (ret)
    return ret;

  make_bmk_conv_param(cvk_ctx, &bmk_conv_param, p_param);

  bmk_conv_param_alloc_ok(&bmk_conv_param, p_param);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, bmk_conv_param.ifmap, (uint8_t *)s8_test_data.conv_input);
  put_conv_weight(rt_handle, cvk_ctx, bmk_conv_param.weight, (uint8_t *)s8_test_data.conv_weight);
  if (p_param->using_bias)
    put_conv_bias(rt_handle, cvk_ctx, bmk_conv_param.bias, s8_test_data.conv_bias);

  return 0;
}

void get_result(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  s8_test_data.conv_output = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, bmk_conv_param.ofmap);
  s8_test_data.l2g_cw_output = tensor_copy_d2s(rt_handle, l2g_cw_param.dst);
  s8_test_data.g2l_matrix_output = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, g2l_matrix_param.dst);
  s8_test_data.l2l_tensor_output = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, l2l_tensor_copy_param.dst);
}

int check_result()
{
  int has_error = array_cmp_int8(
      "conv Comparing results ...\n",
      s8_test_data.conv_output_ref, (int8_t *)s8_test_data.conv_output, conv_output_size(&conv_param));

  if (has_error) {
    print_conv_param(&conv_param);
    printf("Comparison FAILED\n");
    return -1;
  }

  for (uint64_t i = 0; i < tl_shape_size(&l2g_cw_param.src->shape, l2g_cw_param.src->fmt); i++) {
    if (s8_test_data.l2g_cw_output[i] != s8_test_data.l2g_cw_output_ref[i]) {
      fprintf(stderr, "l2g_cw comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2g_cw_output[i], s8_test_data.l2g_cw_output_ref[i]);
      return -1;
    }
  }
  for (uint64_t i = 0; i < ml_shape_size(&g2l_matrix_param.dst->shape, g2l_matrix_param.dst->fmt); i++) {
    if (s8_test_data.g2l_matrix_output[i] != s8_test_data.g2l_matrix_output_ref[i]) {
      fprintf(stderr, "g2l_matrix comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.g2l_matrix_output[i], s8_test_data.g2l_matrix_output_ref[i]);
      return -1;
    }
  }

  for (uint64_t i = 0; i < tl_shape_size(&l2l_tensor_copy_param.src->shape, l2l_tensor_copy_param.src->fmt); i++) {
    if (s8_test_data.l2l_tensor_output[i] != s8_test_data.l2l_tensor_output_ref[i]) {
      fprintf(stderr, "l2l_tensor comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2l_tensor_output[i], s8_test_data.l2l_tensor_output_ref[i]);
      return -1;
    }
  }

  return 0;
}

void trigger_max_power(cvk_context_t *cvk_ctx)
{
 cvk_ctx->ops->parallel_enable(cvk_ctx);
 cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &l2g_cw_param);
 cvk_ctx->ops->tdma_g2l_matrix_copy_row_col_transposed(cvk_ctx, &g2l_matrix_param);
 cvk_ctx->ops->tdma_l2l_tensor_copy(cvk_ctx, &l2l_tensor_copy_param);
 cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &bmk_conv_param);
 cvk_ctx->ops->parallel_disable(cvk_ctx);
 cvk_ctx->ops->parallel_enable(cvk_ctx);
 cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &l2g_cw_param);
 cvk_ctx->ops->tdma_g2l_matrix_copy_row_col_transposed(cvk_ctx, &g2l_matrix_param);
 cvk_ctx->ops->tdma_l2l_tensor_copy(cvk_ctx, &l2l_tensor_copy_param);
 cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &bmk_conv_param);
 cvk_ctx->ops->parallel_disable(cvk_ctx);
 CVI_RT_Submit(cvk_ctx);
}

void free_s8_data()
{
  free(s8_test_data.conv_input);
  free(s8_test_data.conv_weight);
  free(s8_test_data.conv_bias);
  free(s8_test_data.conv_output);
  free(s8_test_data.conv_output_ref);
  free(s8_test_data.l2g_cw_src);
  free(s8_test_data.l2g_cw_output);
  free(s8_test_data.l2g_cw_output_ref);
  free(s8_test_data.g2l_matrix_src);
  free(s8_test_data.g2l_matrix_output);
  free(s8_test_data.g2l_matrix_output_ref);
  free(s8_test_data.l2l_tensor_src);
  free(s8_test_data.l2l_tensor_output);
  free(s8_test_data.l2l_tensor_output_ref);
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

  printf("conv max_power test\n");
  init_conv_param(&conv_param);
  ret |= setup_conv(&conv_param, rt_handle, cvk_ctx);

  test_l2g_cw_transpose(rt_handle, cvk_ctx, &l2g_cw_param);
  test_g2l_matrix_transpose(rt_handle, cvk_ctx, &g2l_matrix_param);
  test_l2l_tensor_copy(rt_handle, cvk_ctx, &l2l_tensor_copy_param);

  trigger_max_power(cvk_ctx);
  get_result(rt_handle, cvk_ctx);
  check_result();

  destroy_param_l2l(cvk_ctx,&l2l_tensor_copy_param);
  destroy_param_g2l(rt_handle, cvk_ctx, &g2l_matrix_param);
  destroy_param_l2g(rt_handle, cvk_ctx, &l2g_cw_param);
  free_bmk_conv_param(cvk_ctx, &bmk_conv_param, &conv_param);
  free_s8_data();

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
