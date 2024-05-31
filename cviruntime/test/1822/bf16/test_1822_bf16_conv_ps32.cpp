#include "../1822_test_util.h"
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

static inline void bf16_relu(float *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static int ps32_conv_ref(
    const conv_param_t *p_param,
    const u16 *ifmap,
    const u16 *weight,
    const u32 *bias,
    u16 *ofmap, int ps32_mode)
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
    return BM_ERR_FAILURE;

  u32 bstride = in * oc * oh * ow;
  int ret = BM_SUCCESS;

  if (ps32_mode == 2 || ps32_mode == 0)
    memset(result, 0, sizeof(float) * in * oc * oh * ow);
  else {
    for (int i = 0; i < in * oc * oh * ow; i++) {
      result[i] = convert_hex_fp32((ofmap[i + bstride * 0] << 16) | ofmap[i + bstride * 1]);
    }
  }

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
          float result_val= result[n*oc*oh*ow + c*oh*ow + ph*ow + pw];
          for (int idxh = 0; idxh < kh_ext; ++idxh)  {
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
        for(int i = 0; i < ic; i++) {
          free(i_fmap_pad[i]);
          free(kernel_pad[i]);
        }
    } //end for (int c = 0; c < oc; ++c)
  }

  if( ps32_mode & 0x2) {
    for (int i = 0; i < in * oc * oh * ow; i ++) {
      ofmap[i] = convert_fp32_hex(result[i]) >> 16;
      ofmap[bstride + i] = convert_fp32_hex(result[i]) & 0xFFFF;
    }
  } else {
    for (int n = 0; n < in; ++n) {
      for (int c = 0; c < oc; ++c) {
        if (p_param->using_bias) {
          for (int ph = 0; ph < oh; ++ph) {
            for (int pw = 0; pw < ow; ++pw) {
              result[n*oc*oh*ow + c*oh*ow + ph*ow + pw] += convert_hex_fp32(bias[c]); //bias+c ;
            }
          }
        }
        if (p_param->bReLU_EN)
          bf16_relu(&result[n*oc*oh*ow + c*oh*ow], oh * ow);
      }
    }
    for (int i = 0; i < in * oc * oh * ow; i++) {
      ofmap[i] = convert_fp32_bf16(result[i]);
    }
  }
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
    bmctx_t *ctx,
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
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  u16 *transformed_data = transform_weight(s, data);

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
  tl_shape_t tdma_shape = { 1, oc, kh * kw, ic };

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
  free(transformed_data);
}

static u16 * transform_bias(int oc, u32 before[])
{
  u16 *after = (u16 *)malloc(sizeof(u16) * 2 * oc);
  if (!after)
    return NULL;

  for (int i = 0; i < oc; i++){
    after[i] = (before[i] >> 16) & 0xFFFF;
    after[i + oc] = before[i] & 0xFFFF;
  }
  return after;
}

static void put_conv_bias(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    u32 *data)
{

  int oc = tl->shape.c;

  bmshape_t bms = BM_TENSOR_INT8(2 * sizeof(short), oc, 1, 1);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  u16 *transformed_data = transform_bias(oc, data);
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

static u16 * alloc_input(const conv_param_t *p)
{
  int size = conv_input_size(p);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (int i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }

  return buf;
}

static u16 * alloc_weight(const conv_param_t *p)
{
  int size = conv_weight_size(p);

  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (int i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }

  return buf;
}

static u32 * alloc_bias(const conv_param_t *p)
{
  int oc = p->output_c;

  u32 *bias = (u32 *)malloc(sizeof(u32) * oc);
  float val = 100;
  for (int i = 0; i < oc; i++) {
    bias[i] = convert_fp32_hex(val);
    val += 1;
  }
  return bias;
}

static tl_t * conv_ifmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 1);
}

static u32 conv_ifmap_tensor_size(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->input_c;
  s.h = p->input_h;
  s.w = p->input_w;
  return bmk1822_lmem_tensor_to_size(ctx, s, fmt, 1);
}

static tl_t * conv_weight_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16; //p->opd1_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 0);
}

static u32 conv_weight_tensor_to_size(bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16; //p->opd1_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = p->input_c;
  s.c = p->output_c;
  s.h = p->kh;
  s.w = p->kw;
  return bmk1822_lmem_tensor_to_size(ctx, s, fmt, 0);
}

static tl_t * conv_ofmap_tensor(bmk_ctx_t *ctx, const conv_param_t *p)
{
  tl_shape_t s;
  s.n = p->input_n;
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return bmk1822_lmem_alloc_ps32_tensor(ctx, s, FMT_BF16, 1);
}

static u32 conv_ofmap_tensor_to_size(bmk_ctx_t *ctx, const conv_param_t *p)
{
  tl_shape_t s;
  s.n = p->input_n * sizeof(u32) / sizeof(u8);
  s.c = p->output_c;
  s.h = conv_oh(p);
  s.w = conv_ow(p);
  return bmk1822_lmem_tensor_to_size(ctx, s, FMT_BF16, 1);
}

static tl_t * conv_bias_tensor(
    bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;//p->opd2_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return bmk1822_lmem_alloc_tensor(ctx, s, fmt, 0);
}

static u32 conv_bias_tensor_size(
    bmk_ctx_t *ctx, const conv_param_t *p)
{
  fmt_t fmt = FMT_BF16;//p->opd2_sign? FMT_I8: FMT_U8;
  tl_shape_t s;
  s.n = 2;
  s.c = p->output_c;
  s.h = 1;
  s.w = 1;
  return bmk1822_lmem_tensor_to_size(ctx, s, fmt, 0);
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
  if(p->ps32_mode==1)
      if (param->using_bias)
        if (!p->bias)
          return 0;

  return 1;
}

static void make_bmk_conv_param_ps32(
    bmk_ctx_t *ctx,
    bmk1822_tiu_convolution_param_t *dst,
    const conv_param_t *p, u32 ps32_mode)
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
    u32 ifmap_size = conv_ifmap_tensor_size(ctx, p);
    u32 weight_size = conv_weight_tensor_to_size(ctx, p);
    u32 ofmap_size = conv_ofmap_tensor_to_size(ctx, p);
    u32 bias_size = p->using_bias ? conv_bias_tensor_size(ctx, p) : 0;
    u32 total_size = ifmap_size + weight_size + ofmap_size + bias_size;

    // Allocation if size fit.
    // Assertion check in bmk1822_lmem_alloc_ps32_tensor().
    bmk1822_chip_info_t chip_info = bmk1822_chip_info();
    if (total_size <= chip_info.lmem_size) {
      dst->ifmap = conv_ifmap_tensor(ctx, p);
      dst->weight = conv_weight_tensor(ctx, p);
      dst->ofmap = conv_ofmap_tensor(ctx, p);
    } else {
      dst->ifmap = nullptr;
      dst->weight = nullptr;
      dst->ofmap = nullptr;
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
      dst->bias = conv_bias_tensor(ctx, p);
  }
}

static void make_bmk_conv_param(
    bmk_ctx_t *ctx,
    bmk1822_tiu_convolution_param_t *dst,
    const conv_param_t *p)
{
  memset(dst, 0, sizeof(*dst));

  u32 ifmap_size = conv_ifmap_tensor_size(ctx, p);
  u32 weight_size = conv_weight_tensor_to_size(ctx, p);
  u32 ofmap_size = conv_ofmap_tensor_to_size(ctx, p);
  u32 bias_size = p->using_bias ? conv_bias_tensor_size(ctx, p) : 0;
  u32 total_size = ifmap_size + weight_size + ofmap_size + bias_size;

  // Allocation if size fit.
  // Assertion check in bmk1822_lmem_alloc_ps32_tensor().
  bmk1822_chip_info_t chip_info = bmk1822_chip_info();
  if (total_size <= chip_info.lmem_size) {
    dst->ifmap = conv_ifmap_tensor(ctx, p);
    dst->weight = conv_weight_tensor(ctx, p);
    dst->ofmap = conv_ofmap_tensor(ctx, p);

    if (p->using_bias)
      dst->bias = conv_bias_tensor(ctx, p);
  } else {
    dst->ifmap = nullptr;
    dst->weight = nullptr;
    dst->ofmap = nullptr;
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
  // dst->ifmap = conv_ifmap_tensor(ctx, p);
  // dst->weight = conv_weight_tensor(ctx, p);
  // dst->ofmap = conv_ofmap_tensor(ctx, p);
  // dst->bias = NULL;
  dst->ps32_mode = 0;
  // if (p->using_bias)
  //   dst->bias = conv_bias_tensor(ctx, p);
}

static void free_bmk_conv_param(
    bmk_ctx_t *ctx,
    bmk1822_tiu_convolution_param_t *r,
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
  p.random_seed = clock();
  srand(p.random_seed);

retry:

  p.input_n = 1;
  p.input_c = rand() % (10) + 2;
  p.kh = rand() % 6 + 1;
  p.kw = rand() % 6 + 1;
  p.input_h = rand() % 10 + p.kh;
  p.input_w = rand() % 10 + p.kw;
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

static int test_ps32_ut(
    conv_param_t &p_param, bmctx_t ctx, bmk_ctx_t *bk_ctx)
{
  printf("test_ps32_ut\n");
  u16 *input = alloc_input(&p_param);
  u16 *weight = alloc_weight(&p_param);
  u32 *bias = alloc_bias(&p_param);
  u16 *output_ref = (u16 *)malloc(sizeof(u16) * conv_output_size(&p_param) * sizeof(short));
  if (!output_ref)
    return BM_ERR_FAILURE;

  bmerr_t ret = ps32_conv_ref(&p_param, input, weight, bias, output_ref, 2);
  assert(ret == BM_SUCCESS);
  bmk1822_tiu_convolution_param_t conv_param;
  make_bmk_conv_param_ps32(bk_ctx, &conv_param, &p_param, 2);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);
  if (tl_alloc_success) {

    put_bf16_tensor_g2l(&ctx, bk_ctx, conv_param.ifmap, (u16 *)input, FMT_BF16);
    put_conv_weight(&ctx, bk_ctx, conv_param.weight, (u16 *)weight);
    bmk1822_tiu_convolution(bk_ctx, &conv_param);
    bmk1822_tensor_lmem_t ps32_ofmap;
    ps32_ofmap = *conv_param.ofmap;
    ps32_ofmap.shape.n = ps32_ofmap.shape.n * sizeof(short);
    u16 *output = (u16*) get_bf16_tensor_l2g(&ctx, bk_ctx, &ps32_ofmap, FMT_BF16);

    int has_error = array_cmp_int8(
        "Comparing M2 begin_mode results ...\n",
        (s8*)output_ref, (s8 *)output, conv_output_size(&p_param) * sizeof(int));

    if (has_error) {
      print_conv_param(&p_param);
      printf("Comparison M2 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M2 PASS\n");

    free(output);
  }
  free_bmk_conv_param(bk_ctx, &conv_param, &p_param);
  printf("test_ps32_intermediate_mode\n");
  for (int i=0; i < conv_input_size(&p_param); i++)
    input[i] = convert_fp32_bf16(i);

  for (int i=0; i < conv_weight_size(&p_param); i++)
    weight[i] = convert_fp32_bf16(i);

  ret = ps32_conv_ref(&p_param, input, weight, bias, output_ref, 3);
  assert(ret == BM_SUCCESS);

  make_bmk_conv_param_ps32(bk_ctx, &conv_param, &p_param, 3);
  tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);

  if (tl_alloc_success) {
    put_bf16_tensor_g2l(&ctx, bk_ctx, conv_param.ifmap, (u16 *)input, FMT_BF16);
    put_conv_weight(&ctx, bk_ctx, conv_param.weight, (u16 *)weight);

    bmk1822_tiu_convolution(bk_ctx, &conv_param);

    bmk1822_tensor_lmem_t ps32_ofmap;
    ps32_ofmap = *conv_param.ofmap;
    ps32_ofmap.shape.n = ps32_ofmap.shape.n * sizeof(short);

    u16 *output = (u16*) get_bf16_tensor_l2g(&ctx, bk_ctx, &ps32_ofmap, FMT_BF16);

    int has_error = array_cmp_int8(
        "Comparing M3 intermediate results ...\n",
        (s8*)output_ref, (s8 *)output, conv_output_size(&p_param) * sizeof(int));

    if (has_error) {
      print_conv_param(&p_param);
      printf("Comparison M3 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M3 PASS\n");

    free(output);
  }
  free_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  printf("test_ps32_end_mode\n");
  for (int i=0; i < conv_input_size(&p_param); i++)
    input[i] = convert_fp32_bf16(i);

  for (int i=0; i < conv_weight_size(&p_param); i++)
    weight[i] = convert_fp32_bf16(i);

  ret = ps32_conv_ref(&p_param, input, weight, bias, output_ref, 1);
  assert(ret == BM_SUCCESS);

  make_bmk_conv_param_ps32(bk_ctx, &conv_param, &p_param, 1);

  tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);

  if (tl_alloc_success) {

    put_bf16_tensor_g2l(&ctx, bk_ctx, conv_param.ifmap, (u16 *)input, FMT_BF16);
    put_conv_weight(&ctx, bk_ctx, conv_param.weight, (u16 *)weight);
    if (p_param.using_bias) {
      put_conv_bias(&ctx, bk_ctx, conv_param.bias, bias);
    }
    bmk1822_tiu_convolution(bk_ctx, &conv_param);
    u16 *output = (u16*) get_bf16_tensor_l2g(&ctx, bk_ctx, conv_param.ofmap, FMT_BF16);

    int has_error = array_cmp_int8(
        "Comparing M1 end results ...\n",
        (s8*)output_ref, (s8 *)output, conv_output_size(&p_param) * 2);

    if (has_error) {
      print_conv_param(&p_param);
      printf("Comparison M1 FAILED\n");
      exit(-1);
    } else
      printf("Comparison M1 PASS\n");

    free(output);
  }
  free_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  free(input);
  free(weight);
  free(bias);
  free(output_ref);

  return tl_alloc_success ? 1 : 0;
}

static int test_ic_tiling_conv(
    conv_param_t &p_param, bmctx_t ctx, bmk_ctx_t *bk_ctx)
{
  printf("test tiled ps32 conv\n");
  u16 *input = alloc_input(&p_param);
  u16 *weight = alloc_weight(&p_param);
  u32 *bias = alloc_bias(&p_param);
  p_param.r_shift_m = 0;
  u16 *output_ref = (u16 *)malloc(sizeof(u16) * conv_output_size(&p_param));
  if (!output_ref)
    return BM_ERR_FAILURE;

  memset((u8*)output_ref, 0, conv_output_size(&p_param)*2);
  bmerr_t ret = ps32_conv_ref(&p_param, input, weight, bias, output_ref, 0);
  assert(ret == BM_SUCCESS);

  bmk1822_tiu_convolution_param_t conv_tmp_param;
  bmk1822_tiu_convolution_param_t conv_param;
  make_bmk_conv_param(bk_ctx, &conv_param, &p_param);

  int tl_alloc_success = bmk_conv_param_alloc_ok(&conv_param, &p_param);
  if (tl_alloc_success) {
    if (p_param.using_bias) {
      conv_tmp_param.bias = conv_param.bias;
      put_conv_bias(&ctx, bk_ctx, conv_param.bias, bias);
    }
    // for ps32_md[1] = 1, relu_enable & rshift_bits need to set to 0
    // so we store those parameters to conv_tmp_para
    conv_tmp_param.relu_enable = conv_param.relu_enable;
    conv_tmp_param.rshift_bits = conv_param.rshift_bits;
    conv_tmp_param.bias        = conv_param.bias;

    u32 ic_step = 1;
    u32 n_step = 1;
    tl_t ifmap = *conv_param.ifmap;
    tl_t ofmap = *conv_param.ofmap;
    tg_shape_t s;
    s.n = conv_param.ifmap->shape.n;
    s.c = conv_param.ifmap->shape.c;
    s.h = conv_param.ifmap->shape.h;
    s.w = conv_param.ifmap->shape.w;
    tg_t *tg_ifmap = alloc_tg_bf16_gmem(&ctx, s, FMT_BF16);
    put_tg_bf16_gmem(&ctx, tg_ifmap, (u8 *)input);

    s.n = conv_param.weight->shape.n;
    s.c = conv_param.weight->shape.c;
    s.h = conv_param.weight->shape.h;
    s.w = conv_param.weight->shape.w;
    u16 *transformed_weight =
      transform_weight(&conv_param.weight->shape, (u16 *)weight);
    tg_t *tg_weight = alloc_tg_bf16_gmem(&ctx, s, FMT_BF16);
    put_tg_bf16_gmem(&ctx, tg_weight, (u8 *)transformed_weight);
    free(transformed_weight);

    tl_shape_t cur_tl_ifmap_shape = {
        n_step,
        ic_step,
        ifmap.shape.h,
        ifmap.shape.w
    };

    tg_shape_t cur_tg_ifmap_shape = {
      cur_tl_ifmap_shape.n,
      cur_tl_ifmap_shape.c,
      cur_tl_ifmap_shape.h,
      cur_tl_ifmap_shape.w
    };

    tg_stride_t cur_tg_ifmap_stride = {
      tg_ifmap->stride.n,
      tg_ifmap->stride.c,
      tg_ifmap->stride.h,
    };

    tg_t cur_tg_ifmap;
    cur_tg_ifmap.base_reg_index = 0;
    cur_tg_ifmap.start_address = tg_ifmap->start_address;
    cur_tg_ifmap.shape = cur_tg_ifmap_shape;
    cur_tg_ifmap.stride = cur_tg_ifmap_stride;
    cur_tg_ifmap.fmt = FMT_BF16;

    tl_t cur_tl_ifmap;
    cur_tl_ifmap.shape = cur_tl_ifmap_shape;
    cur_tl_ifmap.stride =
      bmk1822_tensor_lmem_default_stride(bk_ctx, cur_tl_ifmap_shape, FMT_BF16, 1);
    cur_tl_ifmap.start_address = ifmap.start_address;
    cur_tl_ifmap.fmt = ifmap.fmt;

    tl_t cur_tl_ofmap;
    cur_tl_ofmap.start_address = ofmap.start_address;
    cur_tl_ofmap.shape = ofmap.shape;
    cur_tl_ofmap.shape.n = n_step;
    cur_tl_ofmap.stride =
      bmk1822_tensor_lmem_default_stride(bk_ctx, cur_tl_ofmap.shape, FMT_BF16, 1);
    cur_tl_ofmap.fmt = ofmap.fmt;

    tl_t cur_tl_weight;
    cur_tl_weight.start_address = conv_param.weight->start_address;
    cur_tl_weight.shape = conv_param.weight->shape;
    cur_tl_weight.shape.n = ic_step;
    cur_tl_weight.stride = {
      2,
      cur_tl_weight.shape.n * cur_tl_weight.shape.h * cur_tl_weight.shape.w * 2,
      cur_tl_weight.shape.n * cur_tl_weight.shape.w * 2,
      cur_tl_weight.shape.n * 2
    };
    cur_tl_weight.fmt = conv_param.weight->fmt;

    const tl_t *saved_tl_weight = conv_param.weight;
    const tl_t *saved_tl_ifmap = conv_param.ifmap;
    for (u32 ci = 0; ci < ifmap.shape.c; ci += ic_step) {
      {
        u32 ic = tg_weight->shape.n;
        u32 oc = tg_weight->shape.c;
        u32 kh = tg_weight->shape.h;
        u32 kw = tg_weight->shape.w;

        tg_t cur_tdma_tg_weight;
        cur_tdma_tg_weight.base_reg_index = tg_weight->base_reg_index;
        cur_tdma_tg_weight.start_address = tg_weight->start_address + ci * (tg_weight->fmt == FMT_BF16 ? 2 : 1);
        cur_tdma_tg_weight.fmt = tg_weight->fmt;
        cur_tdma_tg_weight.shape = {1, oc, kh * kw, ic};
        cur_tdma_tg_weight.stride =
          bmk1822_tensor_tgmem_default_stride(cur_tdma_tg_weight.shape, FMT_BF16);
        cur_tdma_tg_weight.shape = {1, oc, kh * kw, ic_step};

        tl_t cur_tdma_tl_weight;
        cur_tdma_tl_weight = cur_tl_weight;
        cur_tdma_tl_weight.shape.n = cur_tdma_tg_weight.shape.n;
        cur_tdma_tl_weight.shape.c = cur_tdma_tg_weight.shape.c;
        cur_tdma_tl_weight.shape.h = cur_tdma_tg_weight.shape.h;
        cur_tdma_tl_weight.shape.w = cur_tdma_tg_weight.shape.w;
        cur_tdma_tl_weight.stride = bmk1822_tensor_lmem_default_stride(
            bk_ctx, cur_tdma_tl_weight.shape, cur_tdma_tl_weight.fmt, 0);

        bmk1822_tdma_tg2l_tensor_copy_param_t p1;
        memset(&p1, 0, sizeof(p1));
        p1.src = &cur_tdma_tg_weight;
        p1.dst = &cur_tdma_tl_weight;
        bmk1822_tdma_g2l_bf16_tensor_copy(bk_ctx, &p1);
        test_submit(&ctx);
      }
      {
        bmk1822_tdma_tg2l_tensor_copy_param_t p2;
        memset(&p2, 0, sizeof(p2));
        cur_tg_ifmap.start_address =
          tg_ifmap->start_address + ci * tg_ifmap->stride.c;
        p2.src = &cur_tg_ifmap;
        p2.dst = &cur_tl_ifmap;
        bmk1822_tdma_g2l_bf16_tensor_copy(bk_ctx, &p2);
        test_submit(&ctx);
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
      bmk1822_tiu_convolution(bk_ctx, &conv_param);
      conv_param.weight = saved_tl_weight;
      conv_param.ifmap = saved_tl_ifmap;
    }

    u16 *output = (u16*) get_bf16_tensor_l2g(&ctx, bk_ctx, conv_param.ofmap, FMT_BF16);

    free_tg_gmem(&ctx, tg_ifmap);
    free_tg_gmem(&ctx, tg_weight);

    int has_error = array_cmp_int8(
        "Comparing results ...\n",
        (s8*) output_ref, (s8 *)output, conv_output_size(&p_param)*2);
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
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int test_finished_num = 0;
  int round_mode;
  round_mode = set_store_feround();

  for (int i = 0; i < 15; i++) {
    printf("random_test_conv iteration: %d\n", i);
    conv_param_t test_conv_param;
    init_conv_param(test_conv_param);
    //print_conv_param(&test_conv_param);
    test_finished_num += test_ic_tiling_conv(test_conv_param, ctx, bk_ctx);
    test_finished_num += test_ps32_ut(test_conv_param, ctx, bk_ctx);
    if (!test_conv_param.using_bias)
      test_conv_param.using_bias = 1;
    if (test_conv_param.output_c <= 9)
      test_conv_param.output_c += 3;
    //print_conv_param(&test_conv_param);
    test_finished_num += test_ic_tiling_conv(test_conv_param, ctx, bk_ctx);
    test_finished_num += test_ps32_ut(test_conv_param, ctx, bk_ctx);
  }
  printf("test_finished_num: %d\n", test_finished_num);
  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
