#include "1822_test_util.h"

typedef bmk1822_tdma_l2tg_tensor_copy_cw_transposed_param_t l2tg_cw_param_t;
typedef bmk1822_tdma_tg2l_matrix_copy_row_col_transposed_param_t tg2l_matrix_param_t;
typedef bmk1822_tdma_l2l_tensor_copy_param_t l2l_tensor_copy_param_t;

typedef struct{
    s8 *conv_input;
    s8 *conv_weight;
    s16 *conv_bias;
    u8 *conv_output;
    s8 *conv_output_ref;
    u8 *l2g_cw_src;
    u8 *l2g_cw_output;
    u8 *l2g_cw_output_ref;
    u8 *g2l_matrix_src;
    u8 *g2l_matrix_output;
    u8 *g2l_matrix_output_ref;
    u8 *l2l_tensor_src;
    u8 *l2l_tensor_output;
    u8 *l2l_tensor_output_ref;
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
l2tg_cw_param_t l2tg_cw_param;
tg2l_matrix_param_t tg2l_matrix_param;
l2l_tensor_copy_param_t l2l_tensor_copy_param;
s_test_data s8_test_data;
bmk1822_tiu_convolution_param_t bmk_conv_param;

bmk1822_tensor_lmem_t *skip_tensor_lmem[10];
u32 skip_tensor_num=0;

/* need to make sure the free order of alloc_tl for skip_tensor_lmem*/
void skip_tensor_lmem_size(bmk_ctx_t *bmk, const bmk1822_tensor_lmem_t *p)
{
  u32 needed = align_up(p->shape.n * p->stride.n, BM1822_HW_EU_NUM);
  u32 start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  u32 remain_size = start_addr % BM1822_HW_LMEM_BANK_SIZE ? (BM1822_HW_LMEM_BANK_SIZE - start_addr % BM1822_HW_LMEM_BANK_SIZE) : 0; // remain size for each lane
  if(remain_size)
  {
//    tl_shape_t src_shape2 = {1, BM1822_HW_EU_NUM, 1, remain_size};
    tl_shape_t src_shape2 = {1, BM1822_HW_NPU_NUM, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = alloc_tl(bmk, src_shape2, FMT_I8, 1); // skip the lmem size and next tl can alignment to bank size
  }
  skip_tensor_num++;
}

void skip_matrix_lmem_size(bmk_ctx_t *bmk, const bmk1822_matrix_lmem_t *p)
{
  u32 needed = align_up(p->shape.n * p->stride.n, BM1822_HW_EU_NUM);
  u32 start_addr = p->start_address + needed; //src_shape.n*src_shape.c*src_shape.h*src_shape.w/32;
  u32 remain_size = start_addr % BM1822_HW_LMEM_BANK_SIZE ? (BM1822_HW_LMEM_BANK_SIZE - start_addr % BM1822_HW_LMEM_BANK_SIZE) : 0; // remain size for each lane
  if(remain_size)
  {
    tl_shape_t src_shape2 = {1, BM1822_HW_NPU_NUM, 1, remain_size};
    //tl_shape_t src_shape2 = {1, BM1822_HW_EU_NUM, 1, remain_size};
    skip_tensor_lmem[skip_tensor_num] = alloc_tl(bmk, src_shape2, FMT_I8, 1); // skip the lmem size and next tl can alignment to bank size
  }
  skip_tensor_num++;
}

void free_skip_tensor_lmem(bmk_ctx_t *ctx)
{
  if(skip_tensor_lmem[--skip_tensor_num]!=NULL)
    free_tl(ctx, skip_tensor_lmem[skip_tensor_num]);
}

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

static int matrix_dot_mult(
    s8 *A, s8 *B, int dim_n, int dim_m,
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
    const s8 *ifmap,
    const s8 *weight,
    const s16 *bias,
    s8 *ofmap)
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
  s8 *i_fmap_pad_ker = (s8 *)malloc(kh_ext * kw_ext);
  if (!result || !i_fmap_pad_ker) {
    free(result);
    free(i_fmap_pad_ker);
    return BM_ERR_FAILURE;
  }

  memset(result, 0, sizeof(int) * in * oc * oh * ow);

  int ret = BM_SUCCESS;

  s8 *i_fmap_pad = NULL;
  s8 *kernel_after = NULL;
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

      // ofmap is s8, signed
      ret = satu_2_8bit(&result[n*oc*oh*ow + c*oh*ow], oh * ow,
                        &ofmap[n*oc*oh*ow + c*oh*ow], r_shift_bits, /*round_floor=*/1,
                        /*sign_unsign=*/1);

      if (ret != BM_SUCCESS)
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

static u8 * transform_weight(const tl_shape_t *s, u8 before[])
{
  u32 ic = s->n;
  u32 oc = s->c;
  u32 kh = s->h;
  u32 kw = s->w;

  u32 size = ic * oc * kh * kw;
  u8 *after = (u8 *)malloc(sizeof(u8) * size);

  /*
   * (oc, ic, kh, kw) -> (1, oc, kh * kw, ic)
   */
  for (u32 oci = 0; oci < oc; oci++) {
    for (u32 ici = 0; ici < ic; ici++) {
      for (u32 khi = 0; khi < kh; khi++) {
        for (u32 kwi = 0; kwi < kw; kwi++) {
          u32 src_i = oci * ic * kh * kw + ici * kh * kw + khi * kw + kwi;
          u32 dst_i = oci * kh * kw * ic + khi * kw * ic + kwi * ic + ici;
          after[dst_i] = before[src_i];
        }
      }
    }
  }

  return after;
}

static void put_conv_weight(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    u8 *data)
{
  const tl_shape_t *s = &tl->shape;
  u32 ic = s->n;
  u32 oc = s->c;
  u32 kh = s->h;
  u32 kw = s->w;

  bmshape_t bms = BM_TENSOR_INT8((int)oc, (int)ic, (int)kh, (int)kw);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  u8 *transformed_data = transform_weight(s, data);

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

  tl_shape_t tdma_shape = { 1, oc, kh * kw, ic };

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
  tdma_tl.stride = bmk1822_tensor_lmem_default_stride(bk_ctx, tdma_shape, FMT_I8, 0);

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tdma_tg;
  p.dst = &tdma_tl;

  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  bmmem_device_free(*ctx, dev_mem);
  free(transformed_data);
}

static s8 * transform_bias(int oc, s16 before[])
{
  s8 *after = (s8 *)malloc(sizeof(s8) * 2 * oc);
  if (!after)
    return NULL;

  for (int i = 0; i < oc; i++){
    after[i] = before[i] & 0xff;
    after[i + oc] = (before[i] >> 8) & 0xff;
  }
  return after;
}

static void put_conv_bias(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    s16 *data)
{
  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2, oc, 1, 1);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  s8 *transformed_data = transform_bias(oc, data);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, (u8 *)transformed_data);
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

  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  bmmem_device_free(*ctx, dev_mem);
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

static s8 * alloc_input(const conv_param_t *p)
{
  int size = conv_input_size(p);

  s8 *buf = (s8 *)malloc(sizeof(s8) * size);
  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static s8 * alloc_weight(const conv_param_t *p)
{
  int size = conv_weight_size(p);

  s8 *buf = (s8 *)malloc(sizeof(s8) * size);
  for (int i = 0; i < size; i++)
    buf[i] = rand() % 256 - 128;

  return buf;
}

static s16 * alloc_bias(const conv_param_t *p)
{
  int oc = p->output_c;

  s16 *bias = (s16 *)malloc(sizeof(s16) * oc);
  for (int i = 0; i < oc; i++)
    bias[i] = rand() % 65536 - 32768;

  return bias;
}

static tl_t * conv_ifmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = p->opd0_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 1);
}

static tl_t * conv_weight_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = p->opd1_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 0);
}

static tl_t * conv_ofmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return bmk1822_lmem_alloc_tensor(ctx, s, FMT_I8, 1);
}

static tl_t * conv_bias_tensor(
    bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = p->opd2_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 0);
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
    const bmk1822_tiu_convolution_param_t *p,
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
    bmk_ctx_t *ctx,
    bmk1822_tiu_convolution_param_t *dst,
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

  dst->ifmap = conv_ifmap_tensor(ctx, p);
  skip_tensor_lmem_size(ctx, dst->ifmap);
  dst->weight = conv_weight_tensor(ctx, p);
  skip_tensor_lmem_size(ctx, dst->weight);
  dst->ofmap = conv_ofmap_tensor(ctx, p);
  skip_tensor_lmem_size(ctx, dst->ofmap);
  dst->bias = NULL;
  dst->ps32_mode = 0;
  if (p->using_bias)
  {
    dst->bias = conv_bias_tensor(ctx, p);
    skip_tensor_lmem_size(ctx, dst->bias);
  }
  dst->w_is_const = 0;
}

static void free_bmk_conv_param(
    bmk_ctx_t *ctx,
    bmk1822_tiu_convolution_param_t *r,
    const conv_param_t *p)
{
  if (p->using_bias && r->bias)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, r->bias);
  }
  if (r->ofmap)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, r->ofmap);
  }
  if (r->weight)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, r->weight);
  }
  if (r->ifmap)
  {
    free_skip_tensor_lmem(ctx);
    free_tl(ctx, r->ifmap);
  }
}

static void init_conv_param(conv_param_t &p)
{
retry:
  p.input_n = 1;
  p.input_c = 16;
  p.input_h = 2;
  p.input_w = 600;

  p.kh = 2;
  p.kw = 16;
  p.output_c = 16;

  p.stride_h = 1;
  p.stride_w = 15;
  p.ins_h = 0;
  p.ins_w = 0;
  p.ins_h_last = 0;;
  p.ins_w_last = 0;;
  p.dh = 1;
  p.dw = 1;

  int kh_ext = conv_kh_ext(&p);
  int kw_ext = conv_kw_ext(&p);
  p.pad_top = 1;
  p.pad_bot = 0;
  p.pad_left = 0;
  p.pad_right = 0;

  if (!conv_param_is_ok(&p)) {
    printf("retry init_conv_param\n");
    goto retry;
  }

  p.using_bias = 0;
  p.r_shift_m = 7;
  p.bReLU_EN = 1;

  p.opd0_sign = 0;
  p.opd1_sign = 1;
  p.opd2_sign = 1;

  assert(p.opd1_sign == 1 && p.opd2_sign == 1);

  int ih_ext = conv_ih_ext(&p);
  int iw_ext = conv_iw_ext(&p);
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
static int calc_rshift_m(const conv_param_t *p, s8* weight)
{
  int kernel_cnt = p->output_c * p->input_c;
  int kernel_size = p->kh * p->kw;
  int *kernel_shifts = (int *)malloc(sizeof(int) * kernel_cnt);

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


static void l2tg_tensor_copy_cw_transposed_ref(
    l2tg_cw_param_t *p, u8 ref_data[], u8 src_data[])
{
  tl_shape_t s = p->src->shape;
  u32 n = s.n;
  u32 c = s.c;
  u32 h = s.h;
  u32 w = s.w;

  for (u32 ni = 0; ni < n; ni++) {
    for (u32 ci = 0; ci < c; ci++) {
      for (u32 hi = 0; hi < h; hi++) {
        for (u32 wi = 0; wi < w; wi++) {
          u32 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u32 dst_i = ni * c * h * w + wi * h * c + hi * c + ci;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static void test_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  u64 size = tl_shape_size(&p->src->shape);

  s8_test_data.l2g_cw_src = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2g_cw_src)
    return;

  for (u64 i = 0; i < size; i++)
    s8_test_data.l2g_cw_src[i] = rand()%0x100;

  s8_test_data.l2g_cw_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2g_cw_output_ref)
    return;

  l2tg_tensor_copy_cw_transposed_ref(p, s8_test_data.l2g_cw_output_ref, s8_test_data.l2g_cw_src);

  put_tensor_g2l(ctx, bmk, p->src, s8_test_data.l2g_cw_src);
}

static void destroy_param_l2g(bmctx_t *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  free_tg_gmem(ctx, p->dst);
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->src);
}

static void test_l2tg_cw_transpose(bmctx_t *ctx, bmk_ctx_t *bmk, l2tg_cw_param_t *p)
{
  tl_shape_t src_shape = {1, 0x100, 1, 0x020};
  tg_shape_t dst_shape = {1, 0x020, 1, 0x100};

//  tl_shape_t src_shape = {1, 0x100, 1, 0x080};
//  tg_shape_t dst_shape = {1, 0x080, 1, 0x100};

  p->src = alloc_tl(bmk, src_shape, FMT_I8, 1);
  p->dst = alloc_tg_gmem(ctx, dst_shape, FMT_I8);
  skip_tensor_lmem_size(bmk, p->src);
  test_param_l2g(ctx, bmk, p);
}

static void tg2l_matrix_copy_row_col_transposed_ref(
    tg2l_matrix_param_t *p, u8 ref_data[], u8 src_data[])
{
  u64 row = p->src->shape.row;
  u64 col = p->src->shape.col;

  for (u64 ri = 0; ri < row; ri++) {
    for (u64 ci = 0; ci < col; ci++) {
      u64 src_i = ri * col + ci;
      u64 dst_i = ci * row + ri;
      ref_data[dst_i] = src_data[src_i];
    }
  }
}

static void test_param_g2l(bmctx_t *ctx, tg2l_matrix_param_t *p)
{
  u64 size = ml_shape_size(&p->dst->shape);

  s8_test_data.g2l_matrix_src = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.g2l_matrix_src)
    return;

  for (u64 i = 0; i < size; i++)
    s8_test_data.g2l_matrix_src[i] = rand()%0x100;

  s8_test_data.g2l_matrix_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.g2l_matrix_output_ref)
    return;

  tg2l_matrix_copy_row_col_transposed_ref(p, s8_test_data.g2l_matrix_output_ref, s8_test_data.g2l_matrix_src);

  put_mg_gmem(ctx, p->src, s8_test_data.g2l_matrix_src);
}

static void destroy_param_g2l(bmctx_t *ctx, bmk_ctx_t *bmk, tg2l_matrix_param_t *p)
{
  free_mg_gmem(ctx, p->src);
  free_skip_tensor_lmem(bmk);
  free_ml(bmk, p->dst);
}


static void test_tg2l_matrix_transpose(bmctx_t *ctx, bmk_ctx_t *bmk, tg2l_matrix_param_t *p)
{
  //tg2l_matrix_param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  mg_shape_t src_shape={0x100, 0x20};
  ml_shape_t dst_shape={0x20, 0x10, 0x10, 0x100};

//  mg_shape_t src_shape={0x100, 0x80};
//  ml_shape_t dst_shape={0x80, 0x10, 0x10, 0x100};

  int dst_align = 1;

  p->src = alloc_mg_gmem(ctx, src_shape);
  p->dst = alloc_ml(bmk, dst_shape, dst_align);
  skip_matrix_lmem_size(bmk, p->dst);
  test_param_g2l(ctx, p);
}

static void l2l_tensor_copy_ref(l2l_tensor_copy_param_t *p, u8 ref_data[], u8 src_data[])
{
  u64 size = tl_shape_size(&p->src->shape);

  for (u64 i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static void test_l2l_param(bmctx_t *ctx, bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  u64 size = tl_shape_size(&p->src->shape);

  s8_test_data.l2l_tensor_src = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2l_tensor_src)
    return;

  for (u64 i = 0; i < size; i++)
    s8_test_data.l2l_tensor_src[i] = rand()%0x100;

  s8_test_data.l2l_tensor_output_ref = (u8 *)malloc(sizeof(u8) * size);
  if (!s8_test_data.l2l_tensor_output_ref)
    return;

  l2l_tensor_copy_ref(p, s8_test_data.l2l_tensor_output_ref, s8_test_data.l2l_tensor_src);

  put_tensor_g2l(ctx, bmk, p->src, s8_test_data.l2l_tensor_src);
}

static void destroy_param_l2l(bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->dst);
  free_skip_tensor_lmem(bmk);
  free_tl(bmk, p->src);
}

static void test_l2l_tensor_copy(bmctx_t *ctx, bmk_ctx_t *bmk, l2l_tensor_copy_param_t *p)
{
  tl_shape_t src_shape = {1, 0x10, 0x1, 0x100};
  tl_shape_t dst_shape = {1, 0x10, 0x1, 0x100};

//  tl_shape_t src_shape = {1, 0x10, 0x1, 0x400};
//  tl_shape_t dst_shape = {1, 0x10, 0x1, 0x400};

  p->src = alloc_tl(bmk, src_shape, FMT_I8, 1);
  skip_tensor_lmem_size(bmk, p->src);
  p->dst = alloc_tl(bmk, dst_shape, FMT_I8, 1);
  skip_tensor_lmem_size(bmk, p->dst);
  test_l2l_param(ctx, bmk, p);
}

static int setup_conv(
    conv_param_t &p_param, bmctx_t ctx, bmk_ctx_t *bk_ctx)
{
  s8_test_data.conv_input = alloc_input(&p_param);
  s8_test_data.conv_weight = alloc_weight(&p_param);
  s8_test_data.conv_bias = alloc_bias(&p_param);
  p_param.r_shift_m = calc_rshift_m(&p_param, s8_test_data.conv_weight);
  s8_test_data.conv_output_ref = (s8 *)malloc(sizeof(s8) * conv_output_size(&p_param));
  if (!s8_test_data.conv_output_ref)
    return BM_ERR_FAILURE;

  bmerr_t ret = conv_ref(&p_param, s8_test_data.conv_input, s8_test_data.conv_weight, s8_test_data.conv_bias, s8_test_data.conv_output_ref);
  assert(ret == BM_SUCCESS);
  make_bmk_conv_param(bk_ctx, &bmk_conv_param, &p_param);

  bmk_conv_param_alloc_ok(&bmk_conv_param, &p_param);

  put_tensor_g2l(&ctx, bk_ctx, bmk_conv_param.ifmap, (u8 *)s8_test_data.conv_input);
  put_conv_weight(&ctx, bk_ctx, bmk_conv_param.weight, (u8 *)s8_test_data.conv_weight);
  if (p_param.using_bias)
    put_conv_bias(&ctx, bk_ctx, bmk_conv_param.bias, s8_test_data.conv_bias);

  return 1;
}

void get_result(bmctx_t *ctx, bmk_ctx_t *bmk)
{
  s8_test_data.conv_output = get_tensor_l2g(ctx, bmk, bmk_conv_param.ofmap);
  s8_test_data.l2g_cw_output = get_tg_gmem(ctx, l2tg_cw_param.dst);
  s8_test_data.g2l_matrix_output = get_matrix_l2g(ctx, bmk, tg2l_matrix_param.dst);
  s8_test_data.l2l_tensor_output = get_tensor_l2g(ctx, bmk, l2l_tensor_copy_param.dst);
}

void check_result()
{
    int has_error = array_cmp_int8(
        "conv Comparing results ...\n",
        s8_test_data.conv_output_ref, (s8 *)s8_test_data.conv_output, conv_output_size(&conv_param));

    if (has_error) {
      print_conv_param(&conv_param);
      printf("Comparison FAILED\n");
      exit(-1);
    }

  for (u64 i = 0; i < tl_shape_size(&l2tg_cw_param.src->shape); i++) {
    if (s8_test_data.l2g_cw_output[i] != s8_test_data.l2g_cw_output_ref[i]) {
      fprintf(stderr, "l2g_cw comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2g_cw_output[i], s8_test_data.l2g_cw_output_ref[i]);
      exit(-1);
    }
  }
  for (u64 i = 0; i < ml_shape_size(&tg2l_matrix_param.dst->shape); i++) {
    if (s8_test_data.g2l_matrix_output[i] != s8_test_data.g2l_matrix_output_ref[i]) {
      fprintf(stderr, "g2l_matrix comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.g2l_matrix_output[i], s8_test_data.g2l_matrix_output_ref[i]);
      exit(-1);
    }
  }

  for (u64 i = 0; i < tl_shape_size(&l2l_tensor_copy_param.src->shape); i++) {
    if (s8_test_data.l2l_tensor_output[i] != s8_test_data.l2l_tensor_output_ref[i]) {
      fprintf(stderr, "l2l_tensor comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, s8_test_data.l2l_tensor_output[i], s8_test_data.l2l_tensor_output_ref[i]);
      exit(-1);
    }
  }


}

void trigger_max_power(bmctx_t *ctx, bmk_ctx_t *bmk)
{
 bmk1822_parallel_enable(bmk);
 bmk1822_tdma_l2g_tensor_copy_cw_transposed(bmk, &l2tg_cw_param);
 bmk1822_tdma_g2l_matrix_copy_row_col_transposed(bmk, &tg2l_matrix_param);
 bmk1822_tdma_l2l_tensor_copy(bmk, &l2l_tensor_copy_param);
 bmk1822_tiu_convolution(bmk, &bmk_conv_param);
 bmk1822_parallel_disable(bmk);
 bmk1822_parallel_enable(bmk);
 bmk1822_tdma_l2g_tensor_copy_cw_transposed(bmk, &l2tg_cw_param);
 bmk1822_tdma_g2l_matrix_copy_row_col_transposed(bmk, &tg2l_matrix_param);
 bmk1822_tdma_l2l_tensor_copy(bmk, &l2l_tensor_copy_param);
 bmk1822_tiu_convolution(bmk, &bmk_conv_param);
 bmk1822_parallel_disable(bmk);
 test_submit(ctx);
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

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  printf("conv max_power test\n");
  init_conv_param(conv_param);
  setup_conv(conv_param, ctx, bk_ctx);

  test_l2tg_cw_transpose(&ctx, bk_ctx, &l2tg_cw_param);
  test_tg2l_matrix_transpose(&ctx, bk_ctx, &tg2l_matrix_param);
  test_l2l_tensor_copy(&ctx, bk_ctx, &l2l_tensor_copy_param);

  trigger_max_power(&ctx, bk_ctx);
  get_result(&ctx, bk_ctx);
  check_result();

  destroy_param_l2l(bk_ctx,&l2l_tensor_copy_param);
  destroy_param_g2l(&ctx, bk_ctx, &tg2l_matrix_param);
  destroy_param_l2g(&ctx, bk_ctx, &l2tg_cw_param);
  free_bmk_conv_param(bk_ctx, &bmk_conv_param, &conv_param);
  free_s8_data();
  test_exit(&ctx);
  return 0;
}
