#include "../1880v2_test_util.h"
//#include <float.h>
//#undef printf
//#define printf(...) {}

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

static void print_conv_param(const conv_param_t *p);

static inline void bf16_relu(float *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static int conv_ref(
    const conv_param_t *p_param,
    const u16 *ifmap,
    const u16 *weight,
    const u32 *bias,
    u16 *ofmap)
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
  int do_relu = p_param->bReLU_EN;

  int ih_ext = calc_dilute_hw(ih, ins_h, ins_h_last,  pad_top, pad_bot);
  int iw_ext = calc_dilute_hw(iw, ins_w, ins_w_last,  pad_left, pad_right);
  int kh_ext = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int kw_ext = calc_dilute_hw(kw, dw - 1, 0, 0, 0);
  int oh = calc_output_hw(ih_ext, kh_ext, stride_h);
  int ow = calc_output_hw(iw_ext, kw_ext, stride_w);

  float *result = (float *)malloc(sizeof(float) * in * oc * oh * ow);
  if (!result)
    return BM_ERR_FAILURE;

  memset(result, 0, sizeof(float) * in * oc * oh * ow);
  int ret = BM_SUCCESS;


  for (int n = 0; n < in; ++n) {
    for (int c = 0; c < oc; ++c) {
      u16 *i_fmap_pad[ic];
      u16 *kernel_pad[ic];

      for (int iic = 0; iic < ic; ++iic) {
        i_fmap_pad[iic] = NULL;
        kernel_pad[iic] = NULL;
        fill_pad_fmap_bf16(
            (ifmap + n*ic*ih*iw + iic*ih*iw), &i_fmap_pad[iic], convert_fp32_bf16(0),
            pad_left, pad_right, pad_top, pad_bot,
            ins_h, ins_w, ins_h_last, ins_w_last, ih, iw);
        //kernel_dilation(
        fill_pad_fmap_bf16(
            (weight + c*ic*kh*kw + iic*kh*kw), &kernel_pad[iic], convert_fp32_bf16(0),
            0, 0, 0, 0,  // no padding
            dh - 1, dw - 1, 0, 0,
                  kh, kw);
      }
      for (int ph = 0; ph < oh; ++ph) {
        for (int pw = 0; pw < ow; ++pw) {
          float result_val = result[n*oc*oh*ow + c*oh*ow + ph*ow + pw];
          for (int idxh = 0; idxh < kh_ext; ++idxh) {
            for (int idxw = 0; idxw < kw_ext; ++idxw) {
              for (int iic = 0; iic < ic; ++iic){
                float ifv = convert_bf16_fp32(i_fmap_pad[iic][(idxh+ph*stride_h) * iw_ext + idxw + pw*stride_w]);
                float ikv = convert_bf16_fp32(kernel_pad[iic][idxh* kw_ext + idxw]);
                result_val += ifv*ikv;
              }
            }
          }
          result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] = result_val;
        }
      }

       if (p_param->using_bias) {
         for (int ph = 0; ph < oh; ++ph) {
           for (int pw = 0; pw < ow; ++pw) {
             result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] += convert_hex_fp32(bias[c]); //bias+c ;
           }
         }
       }

       if (do_relu)
         bf16_relu(&result[n*oc*oh*ow + c*oh*ow], oh * ow);
       for(int i = 0 ;i<ic;i++) {
         free(i_fmap_pad[i]);
         free(kernel_pad[i]);
       }
       if (ret != BM_SUCCESS)
         goto error_release;
    } //end for (int c = 0; c < oc; ++c)
  } //end for (int n = 0; n < in; n++)

    for (int i = 0; i < in * oc * oh * ow; i++) {
      ofmap[i] = convert_fp32_bf16(result[i]);
    }

error_release:
  free(result);

  return ret;
}

static u16 * transform_weight(const tl_shape_t *s, u16 before[])
{
  u32 ic = s->n;
  u32 oc = s->c;
  u32 kh = s->h;
  u32 kw = s->w;

  u32 size = ic * oc * kh * kw;
  u16 *after = (u16 *)malloc(sizeof(u16) * size);

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
    u16 *data)
{
  const tl_shape_t *s = &tl->shape;
  u32 ic = s->n;
  u32 oc = s->c;
  u32 kh = s->h;
  u32 kw = s->w;

  bmshape_t bms = BM_TENSOR_INT8((int)oc, (int)ic, (int)kh, (int)kw*2);
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  u16 *transformed_data = transform_weight(s, data);

  /* we put weight to region 1. CVI_RT_MemCopyS2D regard dev_mem as
   * absolute address, so we should pass abs address to copy weight
   * to right place.
   */

  //u64 ab_addr = bm_device_read_base_reg(*ctx, 1);
  //CVI_RT_MEM ab_dev_mem =
    //bmmem_device_prealloc(*ctx, NULL, ab_addr + gaddr, &bms);

  //int ret = CVI_RT_MemCopyS2D(*ctx, ab_dev_mem, (u8*)transformed_data);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, (u8*)transformed_data);

  assert(ret == BM_SUCCESS);
  tl_shape_t tdma_shape = { 1, oc, kh * kw, ic };

  tg_t tdma_tg;
  tdma_tg.base_reg_index = 0;
  tdma_tg.start_address = gaddr;
  tdma_tg.fmt = FMT_BF16;
  tdma_tg.shape.n = tdma_shape.n;
  tdma_tg.shape.c = tdma_shape.c;
  tdma_tg.shape.h = tdma_shape.h;
  tdma_tg.shape.w = tdma_shape.w;
  tdma_tg.stride = bmk1880v2_tensor_tgmem_default_stride(tdma_tg.shape, FMT_BF16);
  tdma_tg.base_reg_index = 1;

  tl_t tdma_tl = *tl;
  tdma_tl.shape = tdma_shape;
  tdma_tl.stride = bmk1880v2_tensor_lmem_default_stride(bk_ctx, tdma_shape, FMT_BF16, 0);

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tdma_tg;
  p.dst = &tdma_tl;

  bmk1880v2_tdma_g2l_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  CVI_RT_MemFree(*ctx, dev_mem);

  free(transformed_data);
}


static u16 * transform_bias(int oc, u32 before[])
{
  u16 *after = (u16 *)malloc(sizeof(u16) * 2 * oc);
  if (!after)
    return NULL;

  for (int i = 0; i < oc; i++){
    after[i] = (before[i] >> 16) & 0xffff;
    after[i + oc] = before[i] & 0xffff;
  }
  return after;
}

static void put_conv_bias(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    u32 *data)
{

  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2 * sizeof(short), oc, 1, 1);
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  u16 *transformed_data = transform_bias(oc, data);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, (u8 *)transformed_data);
  free(transformed_data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_BF16;
  tg.shape.n = 2;
  tg.shape.c = oc;
  tg.shape.h = 1;
  tg.shape.w = 1;
  tg.stride = bmk1880v2_tensor_tgmem_default_stride(tg.shape, FMT_BF16);

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1880v2_tdma_g2l_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, dev_mem);
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

static u16 * alloc_input(const conv_param_t *p)
{
  int size = conv_input_size(p);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (int i = 0; i < size; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    buf[i] = convert_fp32_bf16(val);
  }
  return buf;
}

static u16 * alloc_weight(const conv_param_t *p)
{
  int size = conv_weight_size(p);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (int i = 0; i < size; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; // 5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    buf[i] = convert_fp32_bf16(val);
  }

  return buf;
}

static u32 * alloc_bias(const conv_param_t *p)
{
  int oc = p->output_c;
  u32 *bias = (u32 *)malloc(sizeof(u32) * oc);
  for (int i = 0; i < oc; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; // 5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    bias[i] = convert_fp32_hex(val);
  }

  return bias;
}

static tl_t * conv_ifmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;//p->opd0_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, 1);
}

static tl_t * conv_weight_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;//p->opd1_sign? FMT_I8: FMT_U8;
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
  fmt_t fmt = FMT_BF16;
  s.n = p->input_n;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, 1);
}

static tl_t * conv_bias_tensor(
    bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;
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

#if 1
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
#endif

static int test_conv(
    conv_param_t &p_param, CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  u16 *input = alloc_input(&p_param);
  u16 *weight = alloc_weight(&p_param);
  u32 *bias = alloc_bias(&p_param);

  //print_conv_param(&p_param);

  u16 *output_ref = (u16 *)malloc(sizeof(u16) * conv_output_size(&p_param));
  if (!output_ref)
    return BM_ERR_FAILURE;

  bmerr_t ret = conv_ref(&p_param, input, weight, bias, output_ref);

  assert(ret == BM_SUCCESS);

  bmk1880v2_tiu_convolution_param_t conv_param;
  make_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);
  if (tl_alloc_success) {
    put_bf16_tensor_g2l(&ctx, bk_ctx, conv_param.ifmap, (u16 *)input, FMT_BF16);
    put_conv_weight(&ctx, bk_ctx, conv_param.weight, (u16 *)weight);
    if (p_param.using_bias)
      put_conv_bias(&ctx, bk_ctx, conv_param.bias, bias);
    bmk1880v2_tiu_convolution(bk_ctx, &conv_param);
    u16 *output = (u16 *) get_bf16_tensor_l2g(&ctx, bk_ctx, conv_param.ofmap, FMT_BF16 );

    int has_error = array_cmp_int8(
        "Comparing results ...\n",
        (s8*)output_ref, (s8*)output, conv_output_size(&p_param)*2);

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
  int round_mode;
  round_mode = set_store_feround();
  int test_finished_num = 0;

  for (int i = 0; i < 20; i++) {
    printf("random_test_conv iteration: %d\n", i);
    conv_param_t test_conv_param;
    init_conv_param(test_conv_param);

    test_finished_num += test_conv(test_conv_param, ctx, bk_ctx);

    if (!test_conv_param.using_bias)
      test_conv_param.using_bias = 1;

    if (test_conv_param.output_c <= 32)
    {
      test_conv_param.output_c += 32;
    }
    test_finished_num += test_conv(test_conv_param, ctx, bk_ctx);
  }
  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
