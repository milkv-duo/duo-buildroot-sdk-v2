#include "../1880v2_test_util.h"

typedef bmk1880v2_tiu_depthwise_convolution_param_t param_t;
int random_seed;
static void print_pooling_param(param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  printf("  Pooling parameters:\n");
  // printf("    random_seed : %d \n", random_seed);
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n", p->pad_top, p->pad_bottom,
         p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  // printf("    ins0 = (%d, %d, %d, %d)\n",
  //       p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  // printf("    dilation = (%d, %d)\n",p->dilation_h, p->dilation_w);
  // printf("    rshift_bits = %d\n", p->rshift_bits);
  // printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == FMT_I8);
}

static u16 *alloc_input(int ic, int ih, int iw, fmt_t ifmt)
{
  u64 size = ic * ih * iw;
  u16 *data = (u16 *)malloc(sizeof(u16) * (size));
  if (!data)
    return NULL;

  if (ifmt == FMT_BF16) {
    for (u64 i = 0; i < size; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 5 ~ -5
      val = (float)(rand() - RAND_MAX2) * 5 / (float)RAND_MAX;
      val = i;
      data[i] = convert_fp32_bf16(val);
    }
  } else {
    u8 *d = (u8 *)data;
    for (u64 i = 0; i < size; i++) {
      d[i] = i % 10 * (i % 2 ? -1 : 1);
    }
  }

  return data;
}

static u16 *alloc_weight(int ic, int kh, int kw, fmt_t fmt)
{
  int size = ic * kh * kw;
  u16 *data = (u16 *)malloc(size * sizeof(u16));
  if (!data)
    return NULL;

  // printf("weight size is %d\n", size * 2);
  if (fmt == FMT_BF16) {
    for (int i = 0; i < size; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 5 ~ -5
      val = (float)(rand() - RAND_MAX2) * 5 / (float)RAND_MAX;
      val = i;
      data[i] = convert_fp32_bf16(val);
    }
  } else {
    u8 *d = (u8 *)data;
    for (int i = 0; i < size; i++) {
      d[i] = i % 5 * (i % 2 ? -1 : 1);
    }
  }
  return data;
}

static u32 *alloc_bias(int ic, fmt_t fmt)
{
  int c = ic;
  u64 size = c;
  u32 *bias = (u32 *)malloc(sizeof(u32) * c);
  if (!bias)
    return NULL;

  if (fmt == FMT_BF16) {
    for (int i = 0; i < c; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 2 ~ -2
      val = (float)(rand() - RAND_MAX2) * 2 / (float)RAND_MAX;
      val = i;
      bias[i] = convert_fp32_hex(val);
    }
  } else {
    u16 *d = (u16 *)bias;
    for (u64 i = 0; i < size; i++) {
      d[i] = i % 0xf * (i % 2 ? -1 : 1);
    }
  }
  return bias;
}

static u16 *alloc_output(int ic, int oh, int ow)
{
  u64 size = ic * oh * ow;
  return (u16 *)malloc(sizeof(u16) * size);
}

static inline void bf16_relu(u16 *buf, u64 size, fmt_t fmt)
{
  if (fmt == FMT_BF16) {
    for (u64 i = 0; i < size; i++)
      if (convert_bf16_fp32(buf[i]) < 0)
        buf[i] = convert_fp32_bf16(0);
  } else {
    s8 *buf_s8 = (s8 *)buf;
    for (u64 i = 0; i < size; i++) {
      if (buf_s8[i] < 0)
        buf_s8[i] = 0;
    }
  }
}

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int native_pooling_avg_bf16(const u16 *i_fmap, const void *weight,
                            const u32 *bias, u16 *o_fmap, int input_n,
                            int input_c, int input_h, int input_w, int kh,
                            int kw, int pad_h_t, int pad_h_b, int pad_w_l,
                            int pad_w_r, int stride_h, int stride_w, int ins_h,
                            int ins_w, int ins_h_last, int ins_w_last, int dh,
                            int dw, int const_weight)
{
  if (kh * kw <= 0)
    return BM_ERR_INVALID_ARGUMENT;

  u16 avg_const_weight = *(u16 *)weight;
  u16 *weight_arr = (u16 *)weight;
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int d_kh = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int output_h = calc_output_hw(h_after, d_kh, stride_h);
  int output_w = calc_output_hw(w_after, d_kw, stride_w);
  // printf("output_h/output_w is %d/%d\n", output_h, output_w);
  float *avg_pooling_mac_a = (float *)malloc(d_kh * d_kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(d_kh * d_kw * sizeof(float));

  u16 *i_fmap_pad = NULL;
  u16 *i_kmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = (u16 *)weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, 0, pad_w_l, pad_w_r, pad_h_t,
                         pad_h_b, ins_h, ins_w, ins_h_last, ins_w_last, input_h,
                         input_w);

      // kernel_dilation(
      if (const_weight == 0)
        fill_pad_fmap_bf16((weight_arr), &i_kmap_pad, 0, 0, 0, 0,
                           0,  // no padding
                           dh - 1, dw - 1, 0, 0, kh, kw);

      float avg_pool_result;
      for (int ph = 0; ph < output_h; ++ph) {
        for (int pw = 0; pw < output_w; ++pw) {
          int hstart = ph * stride_h;
          int wstart = pw * stride_w;
          int pool_index = index_get(ph, output_w, pw);
          int mac_index = 0;

          float r = 0;
          for (int h = 0; h < d_kh; h++) {
            for (int w = 0; w < d_kw; w++) {
              int index = index_get((hstart + h), w_after, (w + wstart));
              mac_index = h * d_kw + w;

              avg_pooling_mac_a[mac_index] =
                  convert_bf16_fp32(i_fmap_pad[index]);

              avg_pooling_mac_b[h * d_kw + w] =
                  const_weight ? convert_bf16_fp32(avg_const_weight) :
                                 convert_bf16_fp32(i_kmap_pad[mac_index]);

#if 0
              printf ("ref[ni %u][ci %u][oh/ow %u/%u][kh/kw %u/%u] o[%d]"
                " %.1f * %.1f + %.1f = %.1f\n",
                n, c, ph, pw, h, w, pool_index,
                avg_pooling_mac_a[mac_index], avg_pooling_mac_b[h*d_kw+w],
                r, r + avg_pooling_mac_a[mac_index] * avg_pooling_mac_b[h*d_kw+w]);
#endif

              r += avg_pooling_mac_a[mac_index] *
                   avg_pooling_mac_b[h * d_kw + w];
            }
          }

          inner_float_product(avg_pooling_mac_a, avg_pooling_mac_b, d_kh * d_kw,
                              &avg_pool_result);

          if (bias) {
            avg_pool_result += convert_hex_fp32(bias[c]);
          }
          *(o_fmap + pool_index) = convert_fp32_bf16(avg_pool_result);
        }
      }
      weight_arr += kh * kw;
      i_fmap += input_w * input_h;
      o_fmap += output_w * output_h;
    }
  }
  free(i_fmap_pad);
  free(i_kmap_pad);
  free(avg_pooling_mac_a);
  free(avg_pooling_mac_b);

  return BM_SUCCESS;
}

static int get_fsz(fmt_t fmt)
{
  assert(fmt == FMT_BF16 || fmt == FMT_I8 || fmt == FMT_U8);
  return fmt == FMT_BF16 ? 2 : 1;
}

static void compare_results(param_t *p, u16 input[], u16 weight[], u32 bias[],
                            u16 output[], u16 output_ref[],
                            u32 org_o_shape_size, int is_valid_pack, int org_oc,
                            int org_oh, int org_ow)
{
  assert(input);
  assert(weight);
  printf("bias at %p\n", bias);
  int f_sz = get_fsz(p->ofmap->fmt);

  if (p->relu_enable) {
    bf16_relu(output_ref, org_o_shape_size, p->ofmap->fmt);
  }

  int cmp_res = -1;
  if (!is_valid_pack) {
    // we reshape c with SAME mode padding with garbage
    // \is_valid_pack set to false means we skip garbage part
    int org_hw = org_oh * org_ow;
    int new_hw = p->ofmap->shape.h * p->ofmap->shape.w;
    int duplicated_c = p->ofmap->shape.c / org_oc;

    assert(new_hw >= org_hw / duplicated_c);

    s8 *output_c = ((s8 *)output);
    s8 *output_ref_c = ((s8 *)output_ref);
    for (int c = 0; c < org_oc; c++) {
      cmp_res = array_cmp_int8("Comparing results ...\n",
                               output_c + c * duplicated_c * new_hw * f_sz,
                               output_ref_c + org_hw * c * f_sz, org_hw * f_sz);

      if (cmp_res != 0) {
        break;
      }
      // printf("compare [%d] pass, org len is %u, new len is %u\n", c,
      //    org_hw, duplicated_c * new_hw);
    }
  } else {
    cmp_res = array_cmp_int8("Comparing results ...\n", (s8 *)output_ref,
                             (s8 *)output, org_o_shape_size * f_sz);
  }
  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    // print_pooling_param(p);
    exit(-1);
  }

  free(output_ref);
}

static int pooling_ih_ext(int ins_h, int ins_last_h, int pad_top,
                          int pad_bottom, int ih)
{
  int ins = ins_h;
  int ins_last = ins_last_h;
  int pad = pad_top + pad_bottom;
  return (ih - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_iw_ext(int ins_w, int ins_last_w, int pad_left,
                          int pad_right, int iw)
{
  int ins = ins_w;
  int ins_last = ins_last_w;
  int pad = pad_left + pad_right;
  return (iw - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_oh(int ins_h, int ins_last_h, int pad_top, int pad_bottom,
                      int stride_h, int ih, int kh, int dh)
{
  int ih_ext = pooling_ih_ext(ins_h, ins_last_h, pad_top, pad_bottom, ih);
  int d_h = (kh - 1) * dh + 1;
  return (ih_ext - d_h) / stride_h + 1;
}

static int pooling_ow(int ins_w, int ins_last_w, int pad_left, int pad_right,
                      int stride_w, int iw, int kw, int dw)
{
  int iw_ext = pooling_iw_ext(ins_w, ins_last_w, pad_left, pad_right, iw);
  int d_w = (kw - 1) * dw + 1;
  return (iw_ext - d_w) / stride_w + 1;
}

static void free_depthwise_struct(param_t *p)
{

  free((void *)p->ofmap);
  free((void *)p->ifmap);
  free((void *)p->weight);
  if (p->bias) {
    free((void *)p->bias);
  }

  p->ofmap = NULL;
  p->ifmap = NULL;
  p->weight = NULL;
  p->bias = NULL;
}

static void free_depthwise_param(bmk_ctx_t *ctx, param_t *p)
{
  if (p->ofmap)
    free_tl(ctx, p->ofmap);

  if (p->weight)
    free_tl(ctx, p->weight);

  if (p->bias)
    free_tl(ctx, p->bias);

  if (p->ifmap)
    free_tl(ctx, p->ifmap);
}

static param_t random_depthwise_param(bmk_ctx_t *ctx, int _ih, int _iw,
                                      int _stride_h, fmt_t _fmt)
{
  param_t p;

  memset(&p, 0, sizeof(p));

  // retry:
  random_seed = clock();
  srand(random_seed);
  int using_bias = rand() % 2;
  int n = rand() % 5 + 1;
  n = 1;
  int c = rand() % (3 * BM1880V2_HW_NPU_NUM) + 1;
  c = 3;
  int ih = rand() % 30 + 3;
  int iw = rand() % 30 + 6;
  int kh = rand() % 7 + 1;
  int kw = rand() % 7 + 1;

  p.ins_h = rand() % kh;
  p.ins_w = rand() % kw;
  p.ins_last_h = rand() % kh;
  p.ins_last_w = rand() % kw;
  p.stride_h = rand() % kh + 1;
  p.stride_w = rand() % kw + 1;
  p.pad_top = rand() % kh;
  p.pad_bottom = rand() % kh;
  p.pad_left = rand() % kw;
  p.pad_right = rand() % kw;
  p.rshift_bits = rand() % 32;
  p.dilation_h = rand() % 4 + 1;
  p.dilation_w = rand() % 4 + 1;

  // default
  fmt_t ifmt = FMT_BF16;
  fmt_t other_fmt = FMT_BF16;
  ih = 24;
  iw = 16;
  kw = 2;
  kh = 4;
  p.stride_h = 3;
  p.stride_w = 2;
  p.rshift_bits = 0;

  ih = _ih;
  p.stride_h = _stride_h;
  iw = _iw;
  ifmt = _fmt;
  other_fmt = FMT_I8;
  if (ifmt == FMT_BF16) {
    other_fmt = FMT_BF16;
  }

  p.pad_left = 0;
  p.pad_right = 1;
  p.pad_top = 0;
  p.pad_bottom = 0;
  // TODO: pad / ins / dilation
  p.ins_h = 0;
  p.ins_last_h = 0;
  p.ins_w = 0;
  p.ins_last_w = 0;
  p.dilation_h = 1;
  p.dilation_w = 1;

  int oh = pooling_oh(p.ins_h, p.ins_last_h, p.pad_top, p.pad_bottom,
                      p.stride_h, ih, kh, p.dilation_h);
  int ow = pooling_ow(p.ins_w, p.ins_last_w, p.pad_left, p.pad_right,
                      p.stride_w, iw, kw, p.dilation_w);

  tl_shape_t ofmap_shape;
  ofmap_shape.n = n;
  ofmap_shape.c = c;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;
  tl_shape_t ifmap_shape;
  ifmap_shape.n = n;
  ifmap_shape.c = c;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;
  tl_shape_t weight_shape;
  weight_shape.n = 1;
  weight_shape.c = c;
  weight_shape.h = kh;
  weight_shape.w = kw;
  tl_shape_t bias_shape;
  bias_shape.n = 2;
  bias_shape.c = c;
  bias_shape.h = 1;
  bias_shape.w = 1;
  p.relu_enable = rand() % 2;

  // fake init for ref
  bmk1880v2_tensor_lmem_t *bias, *weight, *ofmap, *ifmap;
  ifmap = (bmk1880v2_tensor_lmem_t *)malloc(sizeof(bmk1880v2_tensor_lmem_t));
  if (using_bias) {
    bias = (bmk1880v2_tensor_lmem_t *)malloc(sizeof(bmk1880v2_tensor_lmem_t));
  }
  weight = (bmk1880v2_tensor_lmem_t *)malloc(sizeof(bmk1880v2_tensor_lmem_t));
  ofmap = (bmk1880v2_tensor_lmem_t *)malloc(sizeof(bmk1880v2_tensor_lmem_t));

  p.bias = NULL;
  if (using_bias) {
    bias->start_address = -1;
    bias->fmt = other_fmt;
    bias->shape = bias_shape;
    bias->stride = bmk1880v2_tensor_lmem_default_stride(
        ctx, bias->shape, other_fmt, /*eu_align*/0);
    p.bias = bias;
  }

  weight->start_address = -1;
  weight->fmt = other_fmt;
  weight->shape = weight_shape;
  weight->stride = bmk1880v2_tensor_lmem_default_stride(
      ctx, weight->shape, other_fmt, /*align*/1);
  p.weight = weight;

  ofmap->start_address = -1;
  ofmap->fmt = other_fmt;
  ofmap->shape = ofmap_shape;
  ofmap->stride = bmk1880v2_tensor_lmem_default_stride(ctx, ofmap->shape,
                                                            other_fmt, /*align*/1);
  p.ofmap = ofmap;

  ifmap->start_address = -1;
  ifmap->fmt = ifmt;
  ifmap->shape = ifmap_shape;
  ifmap->stride = bmk1880v2_tensor_lmem_default_stride(ctx, ifmap->shape,
                                                            ifmt, /*align*/1);
  p.ifmap = ifmap;

#if 0
  int d_kh = calc_dilute_hw(kh, p.dilation_h - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, p.dilation_w - 1, 0, 0, 0);
  if ((kh > pooling_ih_ext(&p, ih))
      || (kw > pooling_iw_ext(&p, iw))
      || (oh < d_kh)
      || (ow < d_kw)
      || (p.pad_top >= (1 << 4))
      || (p.pad_bottom >= (1 << 4))
      || (p.pad_left >= (1 << 4))
      || (p.pad_right >= (1 << 4))
      || !p.ofmap
      || !p.ifmap
      || !p.weight
      || (using_bias && !p.bias)
) {
    LOG(INFO) << "retry init_pooling_param";
    assert(0 && "it MUST valid param pass");
    goto retry;
  }
#endif
  return p;
}

static void put_bias_tensor(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, const tl_t *tl,
                            u32 data[])
{
  int c = tl->shape.c;

  u16 *hi_lo = (u16 *)malloc(sizeof(u16) * 2 * c);
  if (!hi_lo)
    return;

  if (tl->fmt == FMT_BF16) {
    for (int i = 0; i < c; i++) {
      hi_lo[i] = (data[i] >> 16) & 0xffff;
      hi_lo[i + c] = (data[i] & 0xffff);
    }
  } else {
    u8 *hi_lo_u8 = (u8 *)hi_lo;
    u16 *data_u16 = (u16 *)data;
    for (int i = 0; i < c; i++) {
      hi_lo_u8[i] = data_u16[i] & 0xff;
      hi_lo_u8[i + c] = (data_u16[i] >> 8) & 0xff;
    }
  }
  put_bf16_tensor_g2l(ctx, bk_ctx, tl, (u16 *)hi_lo, tl->fmt);

  free(hi_lo);
}

/**
 * \brief
 */
static int reshape_valid_output(bmk_ctx_t *bk_ctx,
                                const bmk1880v2_tensor_lmem_t *ofmap,
                                int org_oc, int org_oh, int org_ow,
                                bmk1880v2_tensor_lmem_shape_t *tl_shape,
                                bmk1880v2_tensor_lmem_stride_t *tl_load_stride,
                                bmk1880v2_tensor_tgmem_shape_t *tg_shape,
                                bmk1880v2_tensor_tgmem_stride_t *tg_stride,
                                fmt_t fmt)
{

  assert(fmt == FMT_BF16 || fmt == FMT_I8 || fmt == FMT_U8);

  // skip redundant one
  // store to sys and re-slice, maybe use next layer
  // sys->local skip redundant one

  tg_shape->n = tl_shape->n = 1;
  tg_shape->c = tl_shape->c = org_oc;
  tg_shape->h = tl_shape->h = org_oh;
  tg_shape->w = tl_shape->w = org_ow;

  bmk1880v2_tensor_lmem_stride_t s =
      bmk1880v2_tensor_lmem_default_stride(bk_ctx, *tl_shape, fmt, /*eu_align*/0);

  tl_load_stride->n = s.n;
  tl_load_stride->c = s.c;
  tl_load_stride->h = s.h;
  tl_load_stride->w = s.w;

  int duplicat_c = ofmap->shape.c / org_oc;
  tg_stride->n = tg_stride->c =
      duplicat_c * ofmap->shape.h * ofmap->shape.w * get_fsz(fmt);
  tg_stride->h = org_ow * get_fsz(fmt);

  return 0;
}

static bmerr_t init_ref(int ic, int ih, int iw, int kh, int kw, int pad_right,
                        int pad_left, int stride_h, int stride_w, fmt_t fmt,
                        u16 *input, u16 *weight, u32 *bias, u16 *output_ref)
{
  bmerr_t ret;
  int in = 1;
  int ins_h = 0;
  int ins_w = 0;
  int ins_last_h = 0;
  int ins_last_w = 0;
  int dilation_h = 1;
  int dilation_w = 1;
  int pad_top = 0;
  int pad_bottom = 0;
  int rshift_bits = 0;

  if (fmt == FMT_BF16) {
    ret = native_pooling_avg_bf16(
        input, weight, bias ? bias : NULL, output_ref, in, ic, ih, iw, kh, kw,
        pad_top, pad_bottom, pad_left, pad_right, stride_h, stride_w, ins_h,
        ins_w, ins_last_h, ins_last_w, dilation_h, dilation_w, 0);
  } else {
    int opd0_sign = fmt == FMT_I8;
    int res0_sign = true;  //(ofmap->fmt == FMT_I8);
    ret = native_pooling_ave_int8(
        (s8 *)input, (s8 *)weight, bias ? (s16 *)bias : NULL, (s8 *)output_ref,
        in, ic, ih, iw, kh, kw, pad_top, pad_bottom, pad_left, pad_right,
        stride_h, stride_w, ins_h, ins_w, ins_last_h, ins_last_w, opd0_sign,
        res0_sign, rshift_bits, 0);
  }
  return ret;
}

static int test_depthwise(CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx, int ic, int ih,
                          int iw, int kh, int kw, int pad_right, int pad_left,
                          int stride_h, int stride_w, bool has_bias, fmt_t ifmt)
{
  // print_pooling_param(param);
  param_t param;
  param_t *p = &param;
  assert(ifmt == FMT_BF16 || ifmt == FMT_I8 || ifmt == FMT_U8);
  memset(p, 0, sizeof(*p));

  int in = 1;
  // TODO: verify dialate > 1
  int dilation_h = 1;
  int dilation_w = 1;
  int relu_enable = 0;
  int rshift_bits = 0;

  // TODO: verity ins_x
  int org_oh = pooling_oh(0, 0, 0, 0, stride_h, ih, kh, dilation_h);
  int org_ow =
      pooling_ow(0, 0, pad_left, pad_right, stride_w, iw, kw, dilation_w);
  int org_oc = ic;
  int org_o_shape_size = in * org_oc * org_oh * org_ow;
  u16 *output;
  bmk1880v2_tdma_tg2l_tensor_copy_param_t p1;
  bmk1880v2_tdma_l2tg_tensor_copy_param_t p2;
  memset(&p1, 0, sizeof(p1));
  memset(&p2, 0, sizeof(p2));
  // weight / ofmap not support U8 format
  fmt_t other_fmt = ifmt == FMT_BF16 ? FMT_BF16 : FMT_I8;

  // alloc testbench, input/ref
  u16 *input = alloc_input(ic, ih, iw, ifmt);
  u16 *weight = alloc_weight(ic, kh, kw, ifmt);
  u32 *bias = NULL;
  if (has_bias)
    bias = alloc_bias(ic, ifmt);

  u16 *output_ref = alloc_output(ic, org_oh, org_ow);

  // init ref
  init_ref(ic, ih, iw, kh, kw, pad_right, pad_left, stride_h, stride_w, ifmt,
           input, weight, bias, output_ref);
  // assert(ret == BM_SUCCESS);

  // init param
  // TODO: verify pad_top/pad_bottom
  // TODO: verify ins_h_x
  p->pad_left = pad_left;
  p->pad_right = pad_right;
  p->pad_top = 0;
  p->pad_bottom = 0;
  p->ins_h = 0;
  p->ins_last_h = 0;
  p->ins_w = 0;
  p->ins_last_w = 0;
  p->dilation_h = dilation_h;
  p->dilation_w = dilation_w;
  p->stride_h = stride_h;
  p->stride_w = stride_w;
  p->relu_enable = relu_enable;
  p->rshift_bits = rshift_bits;
  p->bias = NULL;

  // prepard load / input / weight / bias / output new shape / stride
  bmk1880v2_tensor_lmem_shape_t tl_load_shape;
  bmk1880v2_tensor_lmem_stride_t tl_load_stride;
  bmk1880v2_tensor_tgmem_shape_t tg_shape;
  bmk1880v2_tensor_tgmem_stride_t tg_stride;
  bmk1880v2_tensor_lmem_shape_t tl_weight_shape;
  bmk1880v2_tensor_lmem_shape_t tl_bias_shape;
  bmk1880v2_tensor_lmem_shape_t tl_output_shape;
  bmk1880v2_tensor_lmem_t *tmp_tl_load;
  bmk1880v2_tensor_tgmem_t *tmp_tg;

  // get reshaped information
  int r = bm1880v2_reshape_channel_same(
      bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left, stride_h, stride_w,
      &tl_load_shape, &tl_load_stride, &tg_shape, &tg_stride, &tl_weight_shape,
      &tl_bias_shape, &tl_output_shape, ifmt, /*align*/1);

  if (r == -1) {
    printf("could not reshape it, 81\n");
    free_depthwise_param(bk_ctx, p);

    free(input);
    free(weight);
    free(bias);
    free(output_ref);
    return -1;
  }

  // prepare input tg
  {
    bmk1880v2_tensor_tgmem_shape_t put_tg_shape;

    put_tg_shape.n = in;
    put_tg_shape.c = ic;
    put_tg_shape.h = ih;
    put_tg_shape.w = iw;
    bmk1880v2_tensor_tgmem_t *put_tg =
        alloc_tg_bf16_gmem(&ctx, put_tg_shape, ifmt);
    put_tg_bf16_gmem(&ctx, put_tg, (u8 *)input);
    free_tg_gmem(&ctx, put_tg);
  }

  // prepare load input, put to tg and load back
  {
    tmp_tl_load = alloc_tl(bk_ctx, tl_load_shape, ifmt, /*eu_align*/0);
    assert(tmp_tl_load);

    tmp_tg = alloc_tg_bf16_gmem(&ctx, tg_shape, ifmt);
    tmp_tg->stride = tg_stride;

    p1.src = tmp_tg;
    p1.dst = tmp_tl_load;

    bmk1880v2_tdma_g2l_bf16_tensor_copy(bk_ctx, &p1);
    test_submit(&ctx);
    free_tg_gmem(&ctx, tmp_tg);


    // fit for hw
    tmp_tl_load->stride = bmk1880v2_tensor_lmem_default_stride(
        bk_ctx, tmp_tl_load->shape, ifmt, /*align*/1);
    p->ifmap = tmp_tl_load;
  }

  // prepare load bias, put to tg and load back
  if (has_bias) {
    // bias must i8
    fmt_t bias_fmt = ifmt == FMT_BF16 ? FMT_BF16 : FMT_I8;
    p->bias =
        bmk1880v2_lmem_alloc_tensor(bk_ctx, tl_bias_shape, bias_fmt, 0);

    // duplicate bias and replace old
    u32 *new_bias = bm1880v2_reshape_channel_bias(
        (u8 *)bias, tl_bias_shape.n, tl_bias_shape.c, tl_bias_shape.h,
        tl_bias_shape.w, org_oc, ifmt);

    // free old one
    free(bias);
    bias = new_bias;
    put_bias_tensor(&ctx, bk_ctx, p->bias, bias);
  }

  // prepare load weight, put to tg and load back
  {
    p->weight = bmk1880v2_lmem_alloc_tensor(bk_ctx, tl_weight_shape,
                                                 other_fmt, /*align*/1);
    assert(p->weight);

    // duplicate kernel with c
    u8 *new_weight = bm1880v2_reshape_channel_weight(
        (u8 *)weight, tl_weight_shape.n, tl_weight_shape.c, tl_weight_shape.h,
        tl_weight_shape.w, org_oc, ifmt);

    // free old one
    free(weight);
    weight = (u16 *)new_weight;
    put_bf16_tensor_g2l(&ctx, bk_ctx, p->weight, (u16 *)weight, ifmt);
  }

  // prepard ofmap
  {
    // we allocate 'same' mode shape
    p->ofmap = bmk1880v2_lmem_alloc_tensor(bk_ctx, tl_output_shape,
                                                other_fmt, /*align*/1);
    assert(p->ofmap);
  }

  // printf("p->ifmap at %p, c is %d\n", p->ifmap, tmp_tl_load->shape.c);

  // emit
  if (ifmt == FMT_BF16) {
    bmk1880v2_tiu_depthwise_convolution(bk_ctx, p);
  } else {
    bmk1880v2_tiu_depthwise_convolution(bk_ctx, p);
  }

  // output = (u16 *)get_bf16_tensor_l2g(&ctx, bk_ctx, p->ofmap, ifmt);

  // check with no pad if true
  int is_valid_pack = false;
  bmk1880v2_tensor_lmem_shape_t r_ofmap_shape;
  bmk1880v2_tensor_lmem_stride_t r_ofmap_stride;
  bmk1880v2_tensor_tgmem_shape_t r_tg_shape;
  bmk1880v2_tensor_tgmem_stride_t r_tg_stride;

  reshape_valid_output(bk_ctx, p->ofmap, org_oc, org_oh, org_ow, &r_ofmap_shape,
                       &r_ofmap_stride, &r_tg_shape, &r_tg_stride, ifmt);

  p1.dst = p->ofmap;

  if (is_valid_pack) {
    bmk1880v2_tensor_tgmem_shape_t dst_shape;
    dst_shape.n = p->ofmap->shape.n;
    dst_shape.c = p->ofmap->shape.c;
    dst_shape.h = p->ofmap->shape.h;
    dst_shape.w = p->ofmap->shape.w;
    tg_t *tg_tmp = alloc_tg_bf16_gmem(&ctx, dst_shape, ifmt);

    p2.src = p->ofmap;
    p2.dst = tg_tmp;

    // store for later reshape
    bmk1880v2_tdma_l2g_bf16_tensor_copy(bk_ctx, &p2);
    test_submit(&ctx);

    // free useless for later reallocate
    free_depthwise_param(bk_ctx, p);

    p->ofmap = bmk1880v2_lmem_alloc_tensor(bk_ctx, r_ofmap_shape, ifmt,
                                                /*eu_align*/0);
    assert(p->ofmap);

    tg_tmp->shape = r_tg_shape;
    tg_tmp->stride = r_tg_stride;

    p1.src = tg_tmp;
    p1.dst = p->ofmap;
    bmk1880v2_tdma_g2l_bf16_tensor_copy(bk_ctx, &p1);
    free_tg_gmem(&ctx, tg_tmp);
  }


  fmt_t ofmap_fmt = ifmt == FMT_BF16 ? FMT_BF16 : FMT_I8;
  output = (u16 *)get_bf16_tensor_l2g(&ctx, bk_ctx, p1.dst, ofmap_fmt);
  compare_results(p, input, weight, bias, output, output_ref, org_o_shape_size,
                  is_valid_pack, org_oc, org_oh, org_ow);

  // free resource
  if (is_valid_pack) {
    free_tl(bk_ctx, p->ofmap);
  } else {
    free_depthwise_param(bk_ctx, p);
  }

  free(input);
  free(weight);
  free(bias);
  free(output);

  return 1;
}

static void init_input(param_t *p, int *ic, int *ih, int *iw, int *kh, int *kw,
                       int *pad_right, int *pad_left)
{
  *ic = p->ifmap->shape.c;
  *ih = p->ifmap->shape.h;
  *iw = p->ifmap->shape.w;
  *kh = p->weight->shape.h;
  *kw = p->weight->shape.w;
  *pad_right = p->pad_right;
  *pad_left = p->pad_left;
}

static int test_depthwise_pooling(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int loop = 1;
  int test_finished_num = 0;
  int ihs[] = {24, 96, 120, 480, 0};
  int iws[] = {16, 17, 19, 23, 128, 256, 0};
  int stride_hs[] = {3, 4, 0};
  fmt_t formats[] = {FMT_I8, FMT_U8, FMT_BF16, FMT_F32};
  int ic, ih, iw, kh, kw, pad_right, pad_left;
  fmt_t ifmt;
  param_t param;
  assert(print_pooling_param);

  ifmt = FMT_U8;
  param = random_depthwise_param(bk_ctx, 36, 11, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);
  print_pooling_param(&param);
  free_depthwise_struct(&param);

  ifmt = FMT_U8;
  param = random_depthwise_param(bk_ctx, 24, 29, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = FMT_BF16;
  param = random_depthwise_param(bk_ctx, 480, 53, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = FMT_I8;
  param = random_depthwise_param(bk_ctx, 480, 61, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = FMT_U8;
  param = random_depthwise_param(bk_ctx, 24, 17, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = FMT_BF16;
  param = random_depthwise_param(bk_ctx, 48, 65, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = FMT_I8;
  param = random_depthwise_param(bk_ctx, 48, 63, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num +=
      test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                     param.stride_h, param.stride_w, param.bias, ifmt);

  for (int i = 0; i < loop; i++) {
    for (int i = 0; ihs[i] != 0; i++) {
      for (int j = 0; iws[j] != 0; j++) {
        for (int k = 0; stride_hs[k] != 0; k++) {
          for (int l = 0; formats[l] != 0; l++) {
            if (ihs[i] >= 480 && formats[l] == FMT_BF16) {
              continue;
            }
            param = random_depthwise_param(bk_ctx, ihs[i], iws[j], stride_hs[k],
                                           formats[l]);
            ifmt = formats[l];
            printf("test[%d] ih/iw/sh/fmt is {%d, %d, %d, %d}\n",
                   test_finished_num, ihs[i], iws[j], stride_hs[k], formats[l]);

            init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
            free_depthwise_struct(&param);
            int r = test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right,
                                   pad_left, param.stride_h, param.stride_w,
                                   param.bias, ifmt);
            test_finished_num += r;
          }
        }
      }
    }
  }
  printf("Test finished %u\n", test_finished_num);

  return test_finished_num;
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();
  int ret = test_depthwise_pooling(&ctx, bk_ctx);
  assert(ret >= 0);
  printf("pass\n");
  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
