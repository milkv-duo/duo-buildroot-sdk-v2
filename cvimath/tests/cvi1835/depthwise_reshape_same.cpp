#include <cvimath_internal.h>
#include <test_cvikernel_util.h>

#include <test_native_ref.h>  // calc_dilute_hw

#define NPU_NUM (1 << 5)
typedef cvk_tiu_depthwise_pt_convolution_param_t param_t;

int random_seed;
static void print_pooling_param(param_t *p) {
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  printf("  Pooling parameters:\n");
  // printf("    random_seed : %d \n", random_seed);
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n", p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  // printf("    ins0 = (%d, %d, %d, %d)\n",
  //       p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  // printf("    dilation = (%d, %d)\n",p->dilation_h, p->dilation_w);
  // printf("    rshift_bits = %d\n", p->rshift_bits);
  // printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == CVK_FMT_I8);
}

static uint16_t *alloc_input(int ic, int ih, int iw, cvk_fmt_t ifmt) {
  uint64_t size = ic * ih * iw;
  uint16_t *data = (uint16_t *)new uint16_t[(size)];
  if (ifmt == CVK_FMT_BF16) {
    for (uint64_t i = 0; i < size; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 5 ~ -5
      val = (float)(rand() - RAND_MAX2) * 5 / (float)RAND_MAX;
      val = i;
      data[i] = convert_fp32_bf16(val);
    }
  } else {
    uint8_t *d = (uint8_t *)data;
    for (uint64_t i = 0; i < size; i++) {
      d[i] = i % 10 * (i % 2 ? -1 : 1);
    }
  }

  return data;
}

static uint16_t *alloc_weight(int ic, int kh, int kw, cvk_fmt_t fmt) {
  int size = ic * kh * kw;
  uint16_t *data = (uint16_t *)malloc(size * sizeof(uint16_t));
  // printf("weight size is %d\n", size * 2);
  if (fmt == CVK_FMT_BF16) {
    for (int i = 0; i < size; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 5 ~ -5
      val = (float)(rand() - RAND_MAX2) * 5 / (float)RAND_MAX;
      val = i;
      data[i] = convert_fp32_bf16(val);
    }
  } else {
    uint8_t *d = (uint8_t *)data;
    for (int i = 0; i < size; i++) {
      d[i] = i % 5 * (i % 2 ? -1 : 1);
    }
  }
  return data;
}

static uint32_t *alloc_bias(int ic, cvk_fmt_t fmt) {
  int c = ic;
  uint64_t size = c;
  uint32_t *bias = (uint32_t *)malloc(sizeof(uint32_t) * c);
  if (fmt == CVK_FMT_BF16) {
    for (int i = 0; i < c; i++) {
      float val = 0;
      int RAND_MAX2 = RAND_MAX / 2;  // 2 ~ -2
      val = (float)(rand() - RAND_MAX2) * 2 / (float)RAND_MAX;
      val = i;
      bias[i] = convert_fp32_hex(val);
    }
  } else {
    uint16_t *d = (uint16_t *)bias;
    for (uint64_t i = 0; i < size; i++) {
      d[i] = i % 0xf * (i % 2 ? -1 : 1);
    }
  }
  return bias;
}

static uint16_t *alloc_output(int ic, int oh, int ow) {
  uint64_t size = ic * oh * ow;
  return (uint16_t *)new uint16_t[(size)];
}

static inline void cvm_relu(uint16_t *buf, uint64_t size, cvk_fmt_t fmt) {
  if (fmt == CVK_FMT_BF16) {
    for (uint64_t i = 0; i < size; i++)
      if (convert_bf16_fp32(buf[i]) < 0) buf[i] = convert_fp32_bf16(0);
  } else {
    int8_t *buf_int8_t = (int8_t *)buf;
    for (uint64_t i = 0; i < size; i++) {
      if (buf_int8_t[i] < 0) buf_int8_t[i] = 0;
    }
  }
}

static int index_get(int h, int w1, int w2) { return h * w1 + w2; }

int native_pooling_avg_bf16(const uint16_t *i_fmap, const void *weight, const uint32_t *bias,
                            uint16_t *o_fmap, int input_n, int input_c, int input_h, int input_w,
                            int kh, int kw, int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
                            int stride_h, int stride_w, int ins_h, int ins_w, int ins_h_last,
                            int ins_w_last, int dh, int dw, int const_weight) {
  if (kh * kw <= 0) return BM_ERR_INVALID_ARGUMENT;

  uint16_t avg_const_weight = *(uint16_t *)weight;
  uint16_t *weight_arr = (uint16_t *)weight;
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int d_kh = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int output_h = calc_output_hw(h_after, d_kh, stride_h);
  int output_w = calc_output_hw(w_after, d_kw, stride_w);
  // printf("output_h/output_w is %d/%d\n", output_h, output_w);
  float *avg_pooling_mac_a = (float *)malloc(d_kh * d_kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(d_kh * d_kw * sizeof(float));

  uint16_t *i_fmap_pad = NULL;
  uint16_t *i_kmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0) weight_arr = (uint16_t *)weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, 0, pad_w_l, pad_w_r, pad_h_t, pad_h_b, ins_h, ins_w,
                         ins_h_last, ins_w_last, input_h, input_w);

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

              avg_pooling_mac_a[mac_index] = convert_bf16_fp32(i_fmap_pad[index]);

              avg_pooling_mac_b[h * d_kw + w] = const_weight
                                                    ? convert_bf16_fp32(avg_const_weight)
                                                    : convert_bf16_fp32(i_kmap_pad[mac_index]);

#if 0
              printf ("ref[ni %u][ci %u][oh/ow %u/%u][kh/kw %u/%u] o[%d]"
                " %.1f * %.1f + %.1f = %.1f\n",
                n, c, ph, pw, h, w, pool_index,
                avg_pooling_mac_a[mac_index], avg_pooling_mac_b[h*d_kw+w],
                r, r + avg_pooling_mac_a[mac_index] * avg_pooling_mac_b[h*d_kw+w]);
#endif

              r += avg_pooling_mac_a[mac_index] * avg_pooling_mac_b[h * d_kw + w];
            }
          }

          inner_float_product(avg_pooling_mac_a, avg_pooling_mac_b, d_kh * d_kw, &avg_pool_result);

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

static int get_fsz(cvk_fmt_t fmt) {
  assert(fmt == CVK_FMT_BF16 || fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8);
  return fmt == CVK_FMT_BF16 ? 2 : 1;
}

static void compare_results(param_t *p, uint16_t input[], uint16_t weight[], uint32_t bias[],
                            uint16_t output[], uint16_t output_ref[], uint32_t org_o_shape_size,
                            int is_valid_pack, int org_oc, int org_oh, int org_ow) {
  assert(input);
  assert(weight);
  (void)input;
  (void)weight;
  printf("bias at %p\n", bias);
  int f_sz = get_fsz(p->ofmap->fmt);

  if (p->relu_enable) {
    cvm_relu(output_ref, org_o_shape_size, p->ofmap->fmt);
  }

  int cmp_res = -1;
  if (!is_valid_pack) {
    // we reshape c with SAME mode padding with garbage
    // \is_valid_pack set to false means we skip garbage part
    int org_hw = org_oh * org_ow;
    int new_hw = p->ofmap->shape.h * p->ofmap->shape.w;
    int duplicated_c = p->ofmap->shape.c / org_oc;

    assert(new_hw >= org_hw / duplicated_c);

    int8_t *output_c = ((int8_t *)output);
    int8_t *output_ref_c = ((int8_t *)output_ref);
    for (int c = 0; c < org_oc; c++) {
      cmp_res =
          array_cmp_int8("Comparing results ...\n", output_c + c * duplicated_c * new_hw * f_sz,
                         output_ref_c + org_hw * c * f_sz, org_hw * f_sz);

      if (cmp_res != 0) {
        break;
      }
      // printf("compare [%d] pass, org len is %u, new len is %u\n", c,
      //    org_hw, duplicated_c * new_hw);
    }
  } else {
    cmp_res = array_cmp_int8("Comparing results ...\n", (int8_t *)output_ref, (int8_t *)output,
                             org_o_shape_size * f_sz);
  }
  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    // print_pooling_param(p);
    exit(-1);
  }

  delete[] output_ref;
}

static int pooling_ih_ext(int ins_h, int ins_last_h, int pad_top, int pad_bottom, int ih) {
  int ins = ins_h;
  int ins_last = ins_last_h;
  int pad = pad_top + pad_bottom;
  return (ih - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_iw_ext(int ins_w, int ins_last_w, int pad_left, int pad_right, int iw) {
  int ins = ins_w;
  int ins_last = ins_last_w;
  int pad = pad_left + pad_right;
  return (iw - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_oh(int ins_h, int ins_last_h, int pad_top, int pad_bottom, int stride_h, int ih,
                      int kh, int dh) {
  int ih_ext = pooling_ih_ext(ins_h, ins_last_h, pad_top, pad_bottom, ih);
  int d_h = (kh - 1) * dh + 1;
  return (ih_ext - d_h) / stride_h + 1;
}

static int pooling_ow(int ins_w, int ins_last_w, int pad_left, int pad_right, int stride_w, int iw,
                      int kw, int dw) {
  int iw_ext = pooling_iw_ext(ins_w, ins_last_w, pad_left, pad_right, iw);
  int d_w = (kw - 1) * dw + 1;
  return (iw_ext - d_w) / stride_w + 1;
}

static void free_depthwise_struct(param_t *p) {
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

static void free_depthwise_param(cvk_context_t *ctx, param_t *p) {
  if (p->ofmap) free_tl(ctx, p->ofmap);

  if (p->weight) free_tl(ctx, p->weight);

  if (p->bias) free_tl(ctx, p->bias);

  if (p->ifmap) free_tl(ctx, p->ifmap);
}

static param_t random_depthwise_param(cvk_context_t *ctx, int _ih, int _iw, int _stride_h,
                                      cvk_fmt_t _fmt) {
  param_t p;

  // retry:
  random_seed = clock();
  srand(random_seed);
  int using_bias = rand() % 2;
  int n = rand() % 5 + 1;
  n = 1;
  int c = rand() % (3 * NPU_NUM) + 1;
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
  cvk_fmt_t ifmt = CVK_FMT_BF16;
  cvk_fmt_t other_fmt = CVK_FMT_BF16;
  ih = 24;
  iw = 16;
  kw = 5;
  kh = 5;
  p.stride_h = 1;
  p.stride_w = 1;

  p.rshift_bits = 0;

  ih = _ih;
  p.stride_h = _stride_h;
  iw = _iw;
  ifmt = _fmt;
  other_fmt = CVK_FMT_I8;
  if (ifmt != CVK_FMT_BF16) {
  } else {
    other_fmt = CVK_FMT_BF16;
  }

  p.pad_left = 2;
  p.pad_right = 2;
  p.pad_top = 0;
  p.pad_bottom = 0;
  // TODO: pad / ins / dilation
  p.ins_h = 0;
  p.ins_last_h = 0;
  p.ins_w = 0;
  p.ins_last_w = 0;
  p.dilation_h = 1;
  p.dilation_w = 1;

  int oh =
      pooling_oh(p.ins_h, p.ins_last_h, p.pad_top, p.pad_bottom, p.stride_h, ih, kh, p.dilation_h);
  int ow =
      pooling_ow(p.ins_w, p.ins_last_w, p.pad_left, p.pad_right, p.stride_w, iw, kw, p.dilation_w);

  cvk_tl_shape_t ofmap_shape;
  ofmap_shape.n = n;
  ofmap_shape.c = c;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;
  cvk_tl_shape_t ifmap_shape;
  ifmap_shape.n = n;
  ifmap_shape.c = c;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;
  cvk_tl_shape_t weight_shape;
  weight_shape.n = 1;
  weight_shape.c = c;
  weight_shape.h = kh;
  weight_shape.w = kw;
  cvk_tl_shape_t bias_shape;
  bias_shape.n = 2;
  bias_shape.c = c;
  bias_shape.h = 1;
  bias_shape.w = 1;
  p.relu_enable = rand() % 2;

  // fake init for ref
  cvk_tl_t *bias, *weight, *ofmap, *ifmap;
  ifmap = (cvk_tl_t *)malloc(sizeof(cvk_tl_t));
  if (using_bias) {
    bias = (cvk_tl_t *)malloc(sizeof(cvk_tl_t));
  }
  weight = (cvk_tl_t *)malloc(sizeof(cvk_tl_t));
  ofmap = (cvk_tl_t *)malloc(sizeof(cvk_tl_t));

  p.bias = NULL;
  if (using_bias) {
    bias->start_address = -1;
    bias->fmt = other_fmt;
    bias->shape = bias_shape;
    bias->stride = ctx->ops->tl_default_stride(ctx, bias->shape, other_fmt, /*eu_align*/ 0);
    p.bias = bias;
  }

  weight->start_address = -1;
  weight->fmt = other_fmt;
  weight->shape = weight_shape;
  weight->stride = ctx->ops->tl_default_stride(ctx, weight->shape, other_fmt, /*align*/ 1);
  p.weight = weight;

  ofmap->start_address = -1;
  ofmap->fmt = other_fmt;
  ofmap->shape = ofmap_shape;
  ofmap->stride = ctx->ops->tl_default_stride(ctx, ofmap->shape, other_fmt, /*align*/ 1);
  p.ofmap = ofmap;

  ifmap->start_address = -1;
  ifmap->fmt = ifmt;
  ifmap->shape = ifmap_shape;
  ifmap->stride = ctx->ops->tl_default_stride(ctx, ifmap->shape, ifmt, /*align*/ 1);
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

static void put_bias_tensor(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx, const cvk_tl_t *tl,
                            uint32_t data[]) {
  int c = tl->shape.c;

  uint16_t *hi_lo = (uint16_t *)malloc(sizeof(uint16_t) * 2 * c);
  if (tl->fmt == CVK_FMT_BF16) {
    for (int i = 0; i < c; i++) {
      hi_lo[i] = (data[i] >> 16) & 0xffff;
      hi_lo[i + c] = (data[i] & 0xffff);
    }
  } else {
    uint8_t *hi_lo_uint8_t = (uint8_t *)hi_lo;
    uint16_t *data_uint16_t = (uint16_t *)data;
    for (int i = 0; i < c; i++) {
      hi_lo_uint8_t[i] = data_uint16_t[i] & 0xff;
      hi_lo_uint8_t[i + c] = (data_uint16_t[i] >> 8) & 0xff;
    }
  }
  put_bf16_tensor_g2l(ctx, bk_ctx, tl, (uint16_t *)hi_lo, tl->fmt);

  free(hi_lo);
}

/**
 * \brief
 */
static int reshape_valid_output(cvk_context_t *bk_ctx, const cvk_tl_t *ofmap, int org_oc,
                                int org_oh, int org_ow, cvk_tl_shape_t *tl_shape,
                                cvk_tl_stride_t *tl_load_stride, cvk_tg_shape_t *tg_shape,
                                cvk_tg_stride_t *tg_stride, cvk_fmt_t fmt) {
  assert(fmt == CVK_FMT_BF16 || fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8);

  // skip redundant one
  // store to sys and re-slice, maybe use next layer
  // sys->local skip redundant one

  tg_shape->n = tl_shape->n = 1;
  tg_shape->c = tl_shape->c = org_oc;
  tg_shape->h = tl_shape->h = org_oh;
  tg_shape->w = tl_shape->w = org_ow;

  cvk_tl_stride_t s = bk_ctx->ops->tl_default_stride(bk_ctx, *tl_shape, fmt, /*eu_align*/ 0);

  tl_load_stride->n = s.n;
  tl_load_stride->c = s.c;
  tl_load_stride->h = s.h;
  tl_load_stride->w = s.w;

  int duplicat_c = ofmap->shape.c / org_oc;
  tg_stride->n = tg_stride->c = duplicat_c * ofmap->shape.h * ofmap->shape.w * get_fsz(fmt);
  tg_stride->h = org_ow * get_fsz(fmt);

  return 0;
}

static bmerr_t init_ref(int ic, int ih, int iw, int kh, int kw, int pad_right, int pad_left,
                        int stride_h, int stride_w, cvk_fmt_t fmt, uint16_t *input,
                        uint16_t *weight, uint32_t *bias, uint16_t *output_ref) {
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

  if (fmt == CVK_FMT_BF16) {
    ret = native_pooling_avg_bf16(input, weight, bias ? bias : NULL, output_ref, in, ic, ih, iw, kh,
                                  kw, pad_top, pad_bottom, pad_left, pad_right, stride_h, stride_w,
                                  ins_h, ins_w, ins_last_h, ins_last_w, dilation_h, dilation_w, 0);
  } else {
    int opd0_sign = fmt == CVK_FMT_I8;
    int res0_sign = true;  //(ofmap->fmt == CVK_FMT_I8);
    ret = native_pooling_ave_int8((int8_t *)input, (int8_t *)weight, bias ? (int16_t *)bias : NULL,
                                  (int8_t *)output_ref, in, ic, ih, iw, kh, kw, pad_top, pad_bottom,
                                  pad_left, pad_right, stride_h, stride_w, ins_h, ins_w, ins_last_h,
                                  ins_last_w, opd0_sign, res0_sign, rshift_bits, 0);
  }
  return ret;
}

static int test_depthwise(CVI_RT_HANDLE ctx, cvk_context_t *bk_ctx, int ic, int ih, int iw, int kh,
                          int kw, int pad_right, int pad_left, int stride_h, int stride_w,
                          bool has_bias, cvk_fmt_t ifmt) {
  // print_pooling_param(param);
  param_t param;
  param_t *p = &param;
  assert(ifmt == CVK_FMT_BF16 || ifmt == CVK_FMT_I8 || ifmt == CVK_FMT_U8);

  int in = 1;
  // TODO: verify dialate > 1
  int dilation_h = 1;
  int dilation_w = 1;
  int relu_enable = 0;
  int rshift_bits = 0;

  // TODO: verity ins_x
  int org_oh = pooling_oh(0, 0, 0, 0, stride_h, ih, kh, dilation_h);
  int org_ow = pooling_ow(0, 0, pad_left, pad_right, stride_w, iw, kw, dilation_w);
  int org_oc = ic;
  int org_o_shape_size = in * org_oc * org_oh * org_ow;
  uint16_t *output;
  cvk_tdma_g2l_tensor_copy_param_t p1;
  cvk_tdma_l2g_tensor_copy_param_t p2;
  // weight / ofmap not support U8 format
  cvk_fmt_t other_fmt = ifmt == CVK_FMT_BF16 ? CVK_FMT_BF16 : CVK_FMT_I8;

  // alloc testbench, input/ref
  uint16_t *input = alloc_input(ic, ih, iw, ifmt);
  uint16_t *weight = alloc_weight(ic, kh, kw, ifmt);
  uint32_t *bias = NULL;
  if (has_bias) bias = alloc_bias(ic, ifmt);

  uint16_t *output_ref = alloc_output(ic, org_oh, org_ow);

  // init ref
  init_ref(ic, ih, iw, kh, kw, pad_right, pad_left, stride_h, stride_w, ifmt, input, weight, bias,
           output_ref);
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
  cvk_tl_shape_t tl_load_shape;
  cvk_tl_stride_t tl_load_stride;
  cvk_tg_shape_t tg_shape;
  cvk_tg_stride_t tg_stride;
  cvk_tl_shape_t tl_weight_shape;
  cvk_tl_shape_t tl_bias_shape;
  cvk_tl_shape_t tl_output_shape;
  cvk_tl_t *tmp_tl_load;
  cvk_tg_t *tmp_tg;

  // get reshaped information
  int r = cvm_reshape_channel_same(bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left, stride_h,
                                   stride_w, &tl_load_shape, &tl_load_stride, &tg_shape, &tg_stride,
                                   &tl_weight_shape, &tl_bias_shape, &tl_output_shape, ifmt,
                                   /*align*/ 1);

  if (r == -1) {
    printf("could not reshape it, 81\n");
    free_depthwise_param(bk_ctx, p);

    delete[] input;
    free(weight);
    free(bias);
    return -1;
  }

  // prepare input tg
  {
    cvk_tg_shape_t put_tg_shape;

    put_tg_shape.n = in;
    put_tg_shape.c = ic;
    put_tg_shape.h = ih;
    put_tg_shape.w = iw;
    cvk_tg_t *put_tg = alloc_tg_bf16_gmem(&ctx, bk_ctx, put_tg_shape, ifmt);
    put_tg_bf16_gmem(&ctx, put_tg, (uint8_t *)input);
    free_tg_gmem(&ctx, put_tg);
  }

  // prepare load input, put to tg and load back
  {
    tmp_tl_load = alloc_tl_bf16(bk_ctx, tl_load_shape, ifmt, /*eu_align*/ 0);
    assert(tmp_tl_load);

    tmp_tg = alloc_tg_bf16_gmem(&ctx, bk_ctx, tg_shape, ifmt);
    tmp_tg->stride = tg_stride;

    p1.src = tmp_tg;
    p1.dst = tmp_tl_load;

    bk_ctx->ops->tdma_g2l_bf16_tensor_copy(bk_ctx, &p1);
    test_submit_comp(&ctx, bk_ctx);
    free_tg_gmem(&ctx, tmp_tg);

    // fit for hw
    tmp_tl_load->stride =
        bk_ctx->ops->tl_default_stride(bk_ctx, tmp_tl_load->shape, ifmt, /*align*/ 1);
    p->ifmap = tmp_tl_load;
  }

  // prepare load bias, put to tg and load back
  if (has_bias) {
    // bias must i8
    cvk_fmt_t bias_fmt = ifmt == CVK_FMT_BF16 ? CVK_FMT_BF16 : CVK_FMT_I8;
    p->bias = bk_ctx->ops->lmem_alloc_tensor(bk_ctx, tl_bias_shape, bias_fmt, 0);

    // duplicate bias and replace old
    uint32_t *new_bias = cvm_reshape_channel_bias((uint8_t *)bias, tl_bias_shape.n, tl_bias_shape.c,
                                                  tl_bias_shape.h, tl_bias_shape.w, org_oc, ifmt);

    // free old one
    free(bias);
    bias = new_bias;
    put_bias_tensor(&ctx, bk_ctx, p->bias, bias);
  }

  // prepare load weight, put to tg and load back
  {
    p->weight = bk_ctx->ops->lmem_alloc_tensor(bk_ctx, tl_weight_shape, other_fmt, /*align*/ 1);
    assert(p->weight);

    // duplicate kernel with c
    uint8_t *new_weight =
        cvm_reshape_channel_weight((uint8_t *)weight, tl_weight_shape.n, tl_weight_shape.c,
                                   tl_weight_shape.h, tl_weight_shape.w, org_oc, ifmt);

    // free old one
    free(weight);
    weight = (uint16_t *)new_weight;
    put_bf16_tensor_g2l(&ctx, bk_ctx, p->weight, (uint16_t *)weight, ifmt);
  }

  // prepard ofmap
  {
    // we allocate 'same' mode shape
    p->ofmap = bk_ctx->ops->lmem_alloc_tensor(bk_ctx, tl_output_shape, other_fmt, /*align*/ 1);
    assert(p->ofmap);
  }

  // printf("p->ifmap at %p, c is %d\n", p->ifmap, tmp_tl_load->shape.c);

  // emit
  if (ifmt == CVK_FMT_BF16) {
    bk_ctx->ops->tiu_pt_depthwise_convolution(bk_ctx, p);
  } else {
    bk_ctx->ops->tiu_pt_depthwise_convolution(bk_ctx, p);
  }

  // output = (uint16_t *)get_bf16_tensor_l2g(&ctx, bk_ctx, p->ofmap, ifmt);

  // check with no pad if true
  int is_valid_pack = false;
  cvk_tl_shape_t r_ofmap_shape;
  cvk_tl_stride_t r_ofmap_stride;
  cvk_tg_shape_t r_tg_shape;
  cvk_tg_stride_t r_tg_stride;

  reshape_valid_output(bk_ctx, p->ofmap, org_oc, org_oh, org_ow, &r_ofmap_shape, &r_ofmap_stride,
                       &r_tg_shape, &r_tg_stride, ifmt);

  p1.dst = p->ofmap;

  if (is_valid_pack) {
    cvk_tg_shape_t dst_shape;
    dst_shape.n = p->ofmap->shape.n;
    dst_shape.c = p->ofmap->shape.c;
    dst_shape.h = p->ofmap->shape.h;
    dst_shape.w = p->ofmap->shape.w;
    cvk_tg_t *cvk_tg_tmp = alloc_tg_bf16_gmem(&ctx, bk_ctx, dst_shape, ifmt);

    p2.src = p->ofmap;
    p2.dst = cvk_tg_tmp;

    // store for later reshape
    bk_ctx->ops->tdma_l2g_bf16_tensor_copy(bk_ctx, &p2);
    test_submit_comp(&ctx, bk_ctx);

    // free useless for later reallocate
    free_depthwise_param(bk_ctx, p);

    p->ofmap = bk_ctx->ops->lmem_alloc_tensor(bk_ctx, r_ofmap_shape, ifmt,
                                              /*eu_align*/ 0);
    assert(p->ofmap);

    cvk_tg_tmp->shape = r_tg_shape;
    cvk_tg_tmp->stride = r_tg_stride;

    p1.src = cvk_tg_tmp;
    p1.dst = p->ofmap;
    bk_ctx->ops->tdma_g2l_bf16_tensor_copy(bk_ctx, &p1);
    free_tg_gmem(&ctx, cvk_tg_tmp);
  }

  cvk_fmt_t ofmap_fmt = ifmt == CVK_FMT_BF16 ? CVK_FMT_BF16 : CVK_FMT_I8;
  output = (uint16_t *)get_bf16_tensor_l2g(&ctx, bk_ctx, p1.dst, ofmap_fmt);
  compare_results(p, input, weight, bias, output, output_ref, org_o_shape_size, is_valid_pack,
                  org_oc, org_oh, org_ow);

  // free resource
  if (is_valid_pack) {
    free_tl(bk_ctx, p->ofmap);
  } else {
    free_depthwise_param(bk_ctx, p);
  }

  delete[] input;
  free(weight);
  free(bias);
  free(output);

  return 1;
}

static void init_input(param_t *p, int *ic, int *ih, int *iw, int *kh, int *kw, int *pad_right,
                       int *pad_left) {
  *ic = p->ifmap->shape.c;
  *ih = p->ifmap->shape.h;
  *iw = p->ifmap->shape.w;
  *kh = p->weight->shape.h;
  *kw = p->weight->shape.w;
  *pad_right = p->pad_right;
  *pad_left = p->pad_left;
}

static int test_depthwise_pooling(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx) {
  int loop = 1;
  int test_finished_num = 0;
  int ihs[] = {24, 96, 120, 480, 0};
  int iws[] = {16, 17, 19, 23, 128, 256, 0};
  int stride_hs[] = {3, 4, 0};
  cvk_fmt_t formats[] = {CVK_FMT_I8, CVK_FMT_U8, CVK_FMT_BF16, CVK_FMT_F32};
  int ic, ih, iw, kh, kw, pad_right, pad_left;
  cvk_fmt_t ifmt;
  param_t param;
  assert(print_pooling_param);

  ifmt = CVK_FMT_U8;
  param = random_depthwise_param(bk_ctx, 210, 640, 1, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);
  print_pooling_param(&param);
  free_depthwise_struct(&param);

#if 1
  param = random_depthwise_param(bk_ctx, 36, 11, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);
  print_pooling_param(&param);
  free_depthwise_struct(&param);

  ifmt = CVK_FMT_U8;
  param = random_depthwise_param(bk_ctx, 24, 29, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = CVK_FMT_BF16;
  param = random_depthwise_param(bk_ctx, 480, 53, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = CVK_FMT_I8;
  param = random_depthwise_param(bk_ctx, 480, 61, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = CVK_FMT_U8;
  param = random_depthwise_param(bk_ctx, 24, 17, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = CVK_FMT_BF16;
  param = random_depthwise_param(bk_ctx, 48, 65, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);

  ifmt = CVK_FMT_I8;
  param = random_depthwise_param(bk_ctx, 48, 63, 3, ifmt);
  init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
  free_depthwise_struct(&param);
  test_finished_num += test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                      param.stride_h, param.stride_w, param.bias, ifmt);
#endif

  for (int i = 0; i < loop; i++) {
    for (int i = 0; ihs[i] != 0; i++) {
      for (int j = 0; iws[j] != 0; j++) {
        for (int k = 0; stride_hs[k] != 0; k++) {
          for (int l = 0; formats[l] != 0; l++) {
            continue;
            if (ihs[i] >= 480 && formats[l] == CVK_FMT_BF16) {
              continue;
            }
            param = random_depthwise_param(bk_ctx, ihs[i], iws[j], stride_hs[k], formats[l]);
            ifmt = formats[l];
            printf("test[%d] ih/iw/sh/fmt is {%d, %d, %d, %d}\n", test_finished_num, ihs[i], iws[j],
                   stride_hs[k], formats[l]);

            init_input(&param, &ic, &ih, &iw, &kh, &kw, &pad_right, &pad_left);
            free_depthwise_struct(&param);
            int r = test_depthwise(*ctx, bk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left,
                                   param.stride_h, param.stride_w, param.bias, ifmt);
            test_finished_num += r;
          }
        }
      }
    }
  }
  printf("Test finished %u\n", test_finished_num);

  return test_finished_num;
}

int main() {
  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  test_init(&ctx, &bk_ctx);

  int round_mode;
  round_mode = set_store_feround();
  int ret = test_depthwise_pooling(&ctx, bk_ctx);
  assert(ret >= 0);
  (void)ret;
  printf("pass\n");

  test_exit(&ctx, bk_ctx);
  restore_feround(round_mode);
  return 0;
}
