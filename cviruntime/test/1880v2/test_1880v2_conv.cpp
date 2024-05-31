#include "1880v2_test_util.h"

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
    CVI_RT_HANDLE *ctx,
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
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  u8 *transformed_data = transform_weight(s, data);

  /* we put weight to region 1. CVI_RT_MemCopyS2D regard dev_mem as
   * absolute address, so we should pass abs address to copy weight
   * to right place.
   */

  //u64 ab_addr = bm_device_read_base_reg(*ctx, 1);
  //CVI_RT_MEM ab_dev_mem =
    //bmmem_device_prealloc(*ctx, NULL, ab_addr + gaddr, &bms);

  //int ret = CVI_RT_MemCopyS2D(*ctx, ab_dev_mem, transformed_data);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, transformed_data);
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
  tdma_tg.stride = bmk1880v2_tensor_tgmem_default_stride(tdma_tg.shape, tdma_tg.fmt);
  tdma_tg.base_reg_index = 1;

  tl_t tdma_tl = *tl;
  tdma_tl.shape = tdma_shape;
  tdma_tl.stride = bmk1880v2_tensor_lmem_default_stride(bk_ctx, tdma_shape, FMT_I8, 0);

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tdma_tg;
  p.dst = &tdma_tl;

  bmk1880v2_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  CVI_RT_MemFree(*ctx, dev_mem);
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
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    s16 *data)
{
  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2, oc, 1, 1);
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  s8 *transformed_data = transform_bias(oc, data);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, (u8 *)transformed_data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape.n = 2;
  tg.shape.c = oc;
  tg.shape.h = 1;
  tg.shape.w = 1;
  tg.stride = bmk1880v2_tensor_tgmem_default_stride(tg.shape, tg.fmt);

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1880v2_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, dev_mem);
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
  return bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, 1);
}

static tl_t * conv_weight_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = p->opd1_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, 0);
}

static tl_t * conv_ofmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return bmk1880v2_lmem_alloc_tensor(ctx, s, FMT_I8, 1);
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
  return bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, 0);
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
    const bmk1880v2_tiu_convolution_param_t *p,
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
    bmk1880v2_tiu_convolution_param_t *dst,
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
  dst->weight = conv_weight_tensor(ctx, p);
  dst->ofmap = conv_ofmap_tensor(ctx, p);
  dst->bias = NULL;
  dst->ps32_mode = 0;
  if (p->using_bias)
    dst->bias = conv_bias_tensor(ctx, p);

  dst->w_is_const = 0;
}

static void free_bmk_conv_param(
    bmk_ctx_t *ctx,
    bmk1880v2_tiu_convolution_param_t *r,
    const conv_param_t *p)
{
  if (p->using_bias && r->bias)
    free_tl(ctx, r->bias);
  if (r->ofmap)
    free_tl(ctx, r->ofmap);
  if (r->weight)
    free_tl(ctx, r->weight);
  if (r->ifmap)
    free_tl(ctx, r->ifmap);
}

static void init_conv_param(conv_param_t &p)
{
  printf("init_conv_param\n");
  memset(&p, 0, sizeof(p));
  p.random_seed = clock();
  srand(p.random_seed);

retry:
  p.input_n = rand() % 5 + 1;
  p.input_c = rand() % (5 * 32) + 1;
  p.kh = rand() % 7 + 1;
  p.kw = rand() % 7 + 1;
  p.input_h = rand() % 40 + p.kh;
  p.input_w = rand() % 40 + p.kw;
  p.output_c = rand() % 10 + 3;
  p.stride_h = rand() % (p.kh) + 1;
  p.stride_w = rand() % (p.kw) + 1;
  p.ins_h = rand() % p.kh;
  p.ins_w = rand() % p.kw;
  p.ins_h_last = rand() % p.kh;;
  p.ins_w_last = rand() % p.kw;;
  p.dh = rand() % 3 + 1;
  p.dw = rand() % 3 + 1;

  int kh_ext = conv_kh_ext(&p);
  int kw_ext = conv_kw_ext(&p);
  p.pad_top = rand() % kh_ext;
  p.pad_bot = rand() % kh_ext;
  p.pad_left = rand() % kw_ext;
  p.pad_right = rand() % kw_ext;

  if (!conv_param_is_ok(&p)) {
    printf("retry init_conv_param\n");
    goto retry;
  }

  p.using_bias = rand() % 2;
  p.r_shift_m = rand() % 8;
  p.bReLU_EN = rand() % 2;

  p.opd0_sign = rand() % 2;
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

  // Tscan does not recognize C++ zero-initialized.
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

static int test_conv(
    conv_param_t &p_param, CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  s8 *input = alloc_input(&p_param);
  s8 *weight = alloc_weight(&p_param);
  s16 *bias = alloc_bias(&p_param);
  p_param.r_shift_m = calc_rshift_m(&p_param, weight);

  s8 *output_ref = (s8 *)malloc(sizeof(s8) * conv_output_size(&p_param));
  if (!output_ref)
    return BM_ERR_FAILURE;

  bmerr_t ret = conv_ref(&p_param, input, weight, bias, output_ref);
  assert(ret == BM_SUCCESS);

  bmk1880v2_tiu_convolution_param_t conv_param;
  make_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);
  if (tl_alloc_success) {
    put_tensor_g2l(&ctx, bk_ctx, conv_param.ifmap, (u8 *)input);
    put_conv_weight(&ctx, bk_ctx, conv_param.weight, (u8 *)weight);
    if (p_param.using_bias)
      put_conv_bias(&ctx, bk_ctx, conv_param.bias, bias);
    bmk1880v2_tiu_convolution(bk_ctx, &conv_param);
    u8 *output = get_tensor_l2g(&ctx, bk_ctx, conv_param.ofmap);

    int has_error = array_cmp_int8(
        "Comparing results ...\n",
        output_ref, (s8 *)output, conv_output_size(&p_param));

    if (has_error) {
      print_conv_param(&p_param);
      printf("Comparison FAILED\n");
      exit(-1);
    }
    free(output);
  }

  free_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  free(input);
  free(weight);
  free(output_ref);
  free(bias);

  return tl_alloc_success ? 1 : 0;
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  int test_finished_num = 0;
  for (int i = 0; i < 5; i++) {
    printf("random_test_conv iteration: %d\n", i);
    conv_param_t test_conv_param;
    init_conv_param(test_conv_param);
    test_finished_num += test_conv(test_conv_param, ctx, bk_ctx);
    if (!test_conv_param.using_bias)
      test_conv_param.using_bias = 1;
    if (test_conv_param.output_c <= 32)
      test_conv_param.output_c += 32;
    test_finished_num += test_conv(test_conv_param, ctx, bk_ctx);
  }

  test_exit(&ctx);
  return 0;
}
