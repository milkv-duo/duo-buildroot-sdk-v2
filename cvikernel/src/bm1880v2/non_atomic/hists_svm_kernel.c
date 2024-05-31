#include "../kernel_1880v2.h"
#define LLVM_DEBUG(...)
#define SPLIT_FAILED 0xFFFF

// only fill base_reg_index/int8_rnd_mode
static void init_tgmem(bmk1880v2_tensor_tgmem_t* t) {
  t->base_reg_index = 0;
  t->int8_rnd_mode = 0;
}

static void copy_tg_tl_tensor_shape(bmk1880v2_tensor_lmem_shape_t* dst,
                                    const bmk1880v2_tensor_tgmem_shape_t* src) {
  dst->n = src->n;
  dst->c = src->c;
  dst->h = src->h;
  dst->w = src->w;
}

static void copy_tl_tg_tensor_shape(bmk1880v2_tensor_tgmem_shape_t* dst,
                                    const bmk1880v2_tensor_lmem_shape_t* src) {
  dst->n = src->n;
  dst->c = src->c;
  dst->h = src->h;
  dst->w = src->w;
}

static int conv_out(int conv_in_ext, int conv_kernel_ext, int stride) {
  return conv_in_ext - conv_kernel_ext / stride + 1;
}

static void conv_output(int on, bmk1880v2_tensor_lmem_shape_t* out_shape,
                        const bmk1880v2_tensor_tgmem_shape_t* image_shape,
                        const bmk1880v2_tensor_tgmem_shape_t* svm_shape) {
  int ins_h = 0, ins_h_last = 0, pad_top = 0, pad_bot = 0, dh = 1;
  int conv_ih_ext = (image_shape->h - 1) * (ins_h + 1) + ins_h_last + 1 + pad_top + pad_bot;
  int conv_kh_ext = (svm_shape->h - 1) * dh + 1;
  int stride_h = 1;

  int ins_w = 0, ins_w_last = 0, pad_left = 0, pad_right = 0, dw = 1;
  int conv_kw_ext = (svm_shape->w - 1) * dw + 1;
  int conv_iw_ext = (image_shape->w - 1) * (ins_w + 1) + ins_w_last + 1 + pad_left + pad_right;
  int stride_w = 1;

  int oh = conv_out(conv_ih_ext, conv_kh_ext, stride_h);
  int ow = conv_out(conv_iw_ext, conv_kw_ext, stride_w);

  out_shape->n = on;
  out_shape->c = svm_shape->n;
  out_shape->h = oh;
  out_shape->w = ow;
}

typedef struct {
  int n;
  int oc;
  int ic;
  int h;
  int w;
  int ic_step;
  int oc_step;
  int oh_step;
  int ow_step;
  int ih_step;
  int iw_step;
} SLICES;

SLICES slices;

static int is_split_ic() { return 0; }
static int is_split_oc() { return 1; }
static int is_reuse_weight() { return 1; }

static bmk1880v2_tensor_lmem_shape_t _shape_t4(int n, int c, int h, int w) {
  bmk1880v2_tensor_lmem_shape_t s;
  s.n = n;
  s.c = c;
  s.h = h;
  s.w = w;
  return s;
}

static bmk1880v2_tensor_tgmem_shape_t _tg_shape_t4(int n, int c, int h, int w) {
  bmk1880v2_tensor_tgmem_shape_t s;
  s.n = n;
  s.c = c;
  s.h = h;
  s.w = w;
  return s;
}

#define NPU_SHIFT (get_num_shift(ctx->chip_info.npu_num))
static int _split(bmk1880v2_context_t* ctx, int input_n, int input_c, int input_h, int input_w,
                  int groups, int output_c, uint16_t kh, uint16_t kw, uint16_t dilation_h, uint16_t dilation_w,
                  uint8_t pad_top, uint8_t pad_bottom, uint8_t pad_left, uint8_t pad_right, uint8_t stride_h, uint8_t stride_w) {
  int do_bias = 0;
  int duplicate_weights = 2;  // force duplicate weight to speed up

  int ic = input_c / groups;
  int oc = output_c / groups;
  int kh_extent = dilation_h * (kh - 1) + 1;
  int kw_extent = dilation_w * (kw - 1) + 1;
  int oh = (input_h + pad_top + pad_bottom - kh_extent) / stride_h + 1;
  int ow = (input_w + pad_left + pad_right - kw_extent) / stride_w + 1;
  int ih = input_h;
  int iw = input_w;
  int n = input_n;

  // Depthwise
  uint8_t isDepthWise = (input_c == groups && output_c == groups && 1 != groups) ? true : false;
  if (isDepthWise) {
    ic = input_c;
    oc = output_c;
  }

  LLVM_DEBUG(llvm::errs() << llvm::format(
                 "BM1880v2ConvBF16::split =>\n"
                 "  groups %d, ifmap (%d, %d, %d, %d), ofmap(%d, %d, %d, %d)\n"
                 "  kernel (%d, %d), pad (top=%d, bot=%d, left=%d, right=%d)\n"
                 "  stride (%d, %d), dilation (%d, %d)\n",
                 groups, input_n, input_c, input_h, input_w, input_n, oc, oh, ow, kh, kw, pad_top,
                 pad_bottom, pad_left, pad_right, stride_h, stride_w, dilation_h, dilation_w));

  slices.n = 1;
  slices.oc = oc / ctx->chip_info.npu_num;  // lane parallelism
  // slices.ic = isDepthWise ? ic : 1;
  slices.ic = 1;
  slices.h = (ih + (4095 - 32 - 1)) / (4095 - 32);  // 12bit, max 4095-32(lanes)
  slices.w = (iw + (4095 - 32 - 1)) / (4095 - 32);  // 12bit, max 4095-32(lanes)

  // int oc_step = (oc >= (int)ctx->chip_info.npu_num) ? (int)ctx->chip_info.npu_num : oc;  // use
  // all lanes int ic_step = isDepthWise ? 1 : ic;
  int ic_step = ic;
  int num_oc_step = 1;

  //
  // Slices may not be a good way to find size
  // We may try to increase or decrease width in aligned with 4, 8, 16 ...
  // or specific height/width (8, 8), (16, 16) ...
  //
  // Split ow
  if (is_split_ic()) {
    LLVM_DEBUG(llvm::errs() << "<= slice ic(" << ic << ")\n";);
    ASSERT(0);
    // return split_ic(ctx);
  }

  if (is_split_oc()) {
    LLVM_DEBUG(llvm::errs() << "<= slice oc\n";);
    num_oc_step = (oc + ctx->chip_info.npu_num - 1) / ctx->chip_info.npu_num;
  }

  // TODO: suppot slice kernel
  // 'iw / slices.w >= kw_extent' means we CANT slice kernel
  for (slices.w = 1; slices.w <= ow && iw / slices.w >= kw_extent; ++slices.w) {
    int ow_step = ceiling_func(ow, slices.w);
    int iw_step = math_min((ow_step - 1) * stride_w + kw_extent, iw);

    if ((slices.w == 1) && (stride_w > 1)) {
      // For better DMA transfer efficiency, use whole width.
      //   E.g.
      //     ifmap (1, 512, 28, 28), kernel (1, 1), stride 2
      //
      //     input (27, 27) needed, but (27, 28) is better
      iw_step = math_min(iw_step + stride_w - 1, iw);
      slices.iw_step = iw_step;
    }

    // Split oh
    // TODO: support slice kernel
    for (slices.h = 1; slices.h <= oh && ih / slices.h >= kh_extent; ++slices.h) {
      // Split oc
      // TODO: config not split it
      for (int slice_oc = 0; slice_oc < num_oc_step; ++slice_oc) {
        // Downward, align lanes
        //   E.g. oc = 48, oc_step: 48, 32
        int oc_step = math_min((num_oc_step - slice_oc) * (int)ctx->chip_info.npu_num, oc);
        if (num_oc_step == 1) {
          // FIXME: not check every loop
          oc_step = oc;
          slices.oc = 1;
        }

        uint32_t coeff_oc_step_size = 0;

        if (do_bias) {
          // 2x 16bit
          coeff_oc_step_size += bmk1880v2_lmem_tensor_to_size(ctx, _shape_t4(2, oc_step, 1, 1),
                                                              FMT_BF16, /*eu_align=*/0);
        }

        // TODO: handle prelu

        // Add weight size
        coeff_oc_step_size += bmk1880v2_lmem_tensor_to_size(
            ctx, _shape_t4(ic_step, oc_step, kh, kw), FMT_BF16, /*eu_align=*/0);

        // split n
        for (slices.n = 1; slices.n <= n; ++slices.n) {
          int n_step = ceiling_func(n, slices.n);

          int oh_step = ceiling_func(oh, slices.h);
          int ih_step = math_min((oh_step - 1) * stride_h + kh_extent, ih);

          uint32_t total_needed = 0;

          uint32_t ofmap_size = bmk1880v2_lmem_tensor_to_size(
              ctx, _shape_t4(n_step, oc_step, oh_step, ow_step), FMT_BF16, /*eu_align=*/1);

          total_needed += ofmap_size;

          uint32_t ifmap_size = bmk1880v2_lmem_tensor_to_size(
              ctx, _shape_t4(n_step, ic_step, ih_step, iw_step), FMT_BF16, /*eu_align=*/1);
          total_needed += ifmap_size;

          total_needed += coeff_oc_step_size;

          // Double buffers so that TDMA load and store can run during TIU executes.
          total_needed *= duplicate_weights;

          // TODO: handle prelu, leaky relu
          // Both prelu and leaky relu need tl_neg, tl_relu.
          // tl_relu, tl_neg are not from tmda and not final output.
          // One copy is enough.
          // if (do_activation && ((activation_method == PRELU) ||
          //                       (activation_method == RELU && activation_arg && activation_arg[0]
          //                       != 0.0f))) {
          //   total_needed += 2 * ofmap_size;  // tl_relu + tl_neg
          // }

          if (total_needed < BM1880V2_HW_LMEM_SIZE) {
            slices.ic_step = ic_step;
            slices.oc_step = oc_step;
            slices.oh_step = oh_step;
            slices.ow_step = ow_step;
            slices.ih_step = ih_step;
            slices.iw_step = iw_step;

            LLVM_DEBUG(
                llvm::errs() << llvm::format(
                    "  Slices(n=%d, oc=%d, ic=%d, h=%d, w=%d), n_step %d, oh_step %d, ih_step %d"
                    ", coeff_oc_step_size %d, total_needed %d\n",
                    slices.n, slices.oc, slices.ic, slices.h, slices.w, n_step, oh_step, ih_step,
                    coeff_oc_step_size, total_needed););
            LLVM_DEBUG(llvm::errs() << "<= BM1880v2ConvFixedParallelv2_qdm::split succeed"
                                    << "/n");
            return total_needed;
          }

        }  // for (slices.n = 1; slices.n < n; ++slices.n)

      }  // for (int slice_oc = 0; slice_oc < num_oc_step; ++slice_oc)

    }  // for (slices.h = 1; slices.h <= oh; ++slices.h)

  }  // for (slices.w = 1; slices.w <= ow; ++slices.ow)

  LLVM_DEBUG(llvm::errs() << "<= BM1880v2ConvBF16::split fail"
                          << "\n");

  return SPLIT_FAILED;
}

void tdma_load_stride_bf16(bmk1880v2_context_t* ctx, bmk1880v2_tensor_lmem_t* tlp, uint64_t ga_src,
                           bmk1880v2_tensor_tgmem_stride_t ts_stride, ctrl_t ctrl) {
  ASSERT(tlp != NULL);

  uint8_t DoTranspose = (ctrl & CTRL_TP) ? true : false;

  // tensor in system memory
  // Global shape use local shape
  bmk1880v2_tensor_tgmem_t ts_data;
  ts_data.base_reg_index = 0;
  ts_data.fmt = tlp->fmt;
  ts_data.start_address = ga_src;
  ts_data.shape = _tg_shape_t4(tlp->shape.n, tlp->shape.c, tlp->shape.h, tlp->shape.w);
  ts_data.stride = ts_stride;

  if (DoTranspose) {
    bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t p1;
    memset(&p1, 0, sizeof(p1));
    p1.src = &ts_data;
    p1.dst = tlp;
    bmk1880v2_tdma_g2l_bf16_tensor_copy_nc_transposed(ctx, &p1);
  } else {
    bmk1880v2_tdma_tg2l_tensor_copy_param_t p1;
    memset(&p1, 0, sizeof(p1));
    p1.src = &ts_data;
    p1.dst = tlp;
    bmk1880v2_tdma_g2l_bf16_tensor_copy(ctx, &p1);
  }
}

void tdma_store_stride_bf16(bmk1880v2_context_t* ctx, bmk1880v2_tensor_lmem_t* tlp, uint64_t ga_dst,
                            bmk1880v2_tensor_tgmem_stride_t ts_stride, ctrl_t ctrl) {
  ASSERT(tlp != NULL);

  uint8_t DoTranspose = (ctrl & CTRL_TP) ? true : false;

  // tensor in system memory
  // Global shape use local shape
  // Global shape used for stride calculation
  bmk1880v2_tensor_tgmem_t ts_data;
  ts_data.base_reg_index = 0;
  ts_data.fmt = tlp->fmt;
  ts_data.start_address = ga_dst;
  ts_data.shape = _tg_shape_t4(tlp->shape.n, tlp->shape.c, tlp->shape.h, tlp->shape.w);
  ts_data.stride = ts_stride;

  if (DoTranspose) {
    bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t p1;
    memset(&p1, 0, sizeof(p1));
    p1.src = tlp;
    p1.dst = &ts_data;
    bmk1880v2_tdma_l2g_bf16_tensor_copy_nc_transposed(ctx, &p1);
  } else {
    bmk1880v2_tdma_l2tg_tensor_copy_param_t p1;
    memset(&p1, 0, sizeof(p1));
    p1.src = tlp;
    p1.dst = &ts_data;
    bmk1880v2_tdma_l2g_bf16_tensor_copy(ctx, &p1);
  }
}

static void ConvReuseWeight(bmk1880v2_context_t* ctx, gaddr_t ga_ifmap, gaddr_t ga_ofmap,
                            gaddr_t ga_weight, int input_n, int input_c, int input_h, int input_w,
                            int groups, int output_c, uint16_t kh, uint16_t kw, uint16_t dilation_h,
                            uint16_t dilation_w, uint8_t pad_top, uint8_t pad_bottom, uint8_t pad_left, uint8_t pad_right,
                            uint8_t stride_h, uint8_t stride_w) {
#define RELU (0)
  int do_scale = 0;
  int do_bn = 0;
  int do_activation = 0;
  int activation_method = 0;
  int* activation_arg = NULL;
  int do_bias = 0;
  int ga_bias = -1;  // not support
  int layer_id = 2;  // debug

  int ic = input_c / groups;
  int oc = output_c / groups;
  int kh_ext = dilation_h * (kh - 1) + 1;
  int kw_ext = dilation_w * (kw - 1) + 1;
  int oh = (input_h + pad_top + pad_bottom - kh_ext) / stride_h + 1;
  int ow = (input_w + pad_left + pad_right - kw_ext) / stride_w + 1;

  int n_step = ceiling_func(input_n, slices.n);
  // int ic_step = ceiling_func(ic, slices.ic);
  // ic_step = slices.ic_step;
  int oh_step = slices.oh_step;
  int ow_step = slices.ow_step;
  int ih_step = slices.ih_step;
  int iw_step = slices.iw_step;
  int oc_step = slices.oc_step;

  // Always use all lanes.
  // Not divided by slices.oc.
  // E.g. mtcnn_det2_cic oc = 48, slices.oc = 2
  // It is better to store step.
  if (slices.oc > 1) {
    ASSERT(oc > (int)ctx->chip_info.npu_num);
    oc_step = ctx->chip_info.npu_num;
  }

  if (slices.h > 1) {
    // max input height inside feature map
    ih_step = (oh_step - 1) * stride_h + kh_ext;
  }
  if (slices.w > 1) {
    // max input width inside feature map
    iw_step = (ow_step - 1) * stride_w + kw_ext;
  }

  LLVM_DEBUG(llvm::errs() << llvm::format(
                 "ConvReuseWeight =>\n"
                 "  groups %d, ifmap (%d, %d, %d, %d), ofmap(%d, %d, %d, %d)\n"
                 "  kernel (%d, %d), pad (top=%d, bot=%d, left=%d, right=%d)\n"
                 "  stride (%d, %d), dilation (%d, %d)\n"
                 "  Slices (n=%d, oc=%d, ic=%d, h=%d, w=%d)\n",
                 groups, input_n, input_c, input_h, input_w, input_n, oc, oh, ow, kh, kw, pad_top,
                 pad_bottom, pad_left, pad_right, stride_h, stride_w, dilation_h, dilation_w,
                 slices.n, slices.oc, slices.ic, slices.h, slices.w));

  uint8_t fused_conv_relu = (!do_scale && !do_bn &&
                          (do_activation && activation_method == RELU &&
                           (!activation_arg || (activation_arg[0] == 0.0f))))
                             ? true
                             : false;

  // uint8_t fused_conv_bn_relu =
  //    (!do_scale && do_bn &&
  //     (do_activation && activation_method == RELU && (!activation_arg || (activation_arg[0] ==
  //     0.0f))))
  //        ? true
  //        : false;

  // bmk1880v2_tensor_lmem_shape_t oc_shape_ = _shape_t4(1, oc_step, 1, 1);
  // bmk1880v2_tensor_lmem_shape_t ifmap_shape_ = _shape_t4(n_step, ic_step, ih_step, input_w);
  // bmk1880v2_tensor_lmem_shape_t ofmap_shape_ = _shape_t4(n_step, oc_step, oh_step, ow);

  bmk1880v2_tensor_lmem_t *tl_weight[2] = {NULL, NULL}, *tl_bias[2] = {NULL, NULL};
  bmk1880v2_tensor_lmem_t* tl_ifmap[2] = {NULL};
  bmk1880v2_tensor_lmem_t* tl_ofmap[2] = {NULL};

  // Global memory stride from global memory shape
  // input_c, output_c, not ic, oc
  // bmk1880v2_tensor_tgmem_stride_t ofmap_gstride = {static_cast<uint32_t>(output_c) * oh * ow,
  //                                                  static_cast<uint32_t>(oh) * ow,
  //                                                  static_cast<uint32_t>(ow)};
  // bmk1880v2_tensor_tgmem_stride_t ifmap_gstride = {static_cast<uint32_t>(input_c) * input_h * input_w,
  //                                                  static_cast<uint32_t>(input_h) * input_w,
  //                                                  static_cast<uint32_t>(input_w)};
  // bmk1880v2_tensor_tgmem_stride_t bias_gstride = {static_cast<uint32_t>(output_c), 1, 1};
  // bmk1880v2_tensor_tgmem_stride_t weight_gstride = {
  //     static_cast<uint32_t>(oc) * kh * kw * ic, static_cast<uint32_t>(kh) * kw * ic, static_cast<uint32_t>(ic)};
  bmk1880v2_tensor_tgmem_stride_t ofmap_gstride =
      bmk1880v2_tensor_tgmem_default_stride(_tg_shape_t4(1, output_c, oh, ow), FMT_BF16);
  bmk1880v2_tensor_tgmem_stride_t ifmap_gstride = bmk1880v2_tensor_tgmem_default_stride(
      _tg_shape_t4(1, input_c, input_h, input_w), FMT_BF16);
  bmk1880v2_tensor_tgmem_stride_t bias_gstride =
      bmk1880v2_tensor_tgmem_default_stride(_tg_shape_t4(1, output_c, 1, 1), FMT_BF16);
  bmk1880v2_tensor_tgmem_stride_t weight_gstride =
      bmk1880v2_tensor_tgmem_default_stride(_tg_shape_t4(1, oc, kh * kw, ic), FMT_BF16);

  //
  // Pre-alloc maximum one-step size
  //
  // Need vector to track the order of local memory.
  // The local memory release must be in reverse order.
  //
  tl_weight[0] =
      bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(ic, oc_step, kh, kw), FMT_BF16, CTRL_NULL);
  if (is_reuse_weight()) {
    tl_weight[1] =
        bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(ic, oc_step, kh, kw), FMT_BF16, CTRL_NULL);
  } else {
    // tl_weight[1] = tl_weight[0];
  }

  tl_ifmap[0] = bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(n_step, ic, ih_step, iw_step),
                                                 FMT_BF16, CTRL_AL);

  if (is_reuse_weight()) {
    tl_ifmap[1] = bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(n_step, ic, ih_step, iw_step),
                                                   FMT_BF16, CTRL_AL);
  } else {
    // tl_ifmap[1] = tl_ifmap[0];
  }

  tl_ofmap[0] = bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(n_step, oc_step, oh_step, ow_step),
                                                 FMT_BF16, CTRL_AL);

  if (is_reuse_weight()) {
    tl_ofmap[1] = bmk1880v2_lmem_alloc_tensor(
        ctx, _shape_t4(n_step, oc_step, oh_step, ow_step), FMT_BF16, CTRL_AL);
  } else {
    // tl_ofmap[1] = tl_ofmap[0];
  }

  ASSERT(tl_weight[0] && tl_ifmap[0] && tl_ofmap[0]);

  if (is_reuse_weight()) {
    ASSERT(tl_weight[1] && tl_ifmap[1] && tl_ofmap[1]);
  }

  if (do_bias) {
    // 16 bit
    tl_bias[0] = bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(2, oc_step, 1, 1), FMT_BF16,
                                                  /*eu_align=*/0);
    if (is_reuse_weight()) {
      tl_bias[1] = bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(2, oc_step, 1, 1), FMT_BF16,
                                                    /*eu_align=*/0);
    } else {
      // tl_bias[1] = tl_bias[0];
    }
    ASSERT(tl_bias[0]);
    if (is_reuse_weight()) {
      ASSERT(tl_bias[1]);
    }
  }

  // split groups
  for (int ig = 0; ig < groups; ++ig) {
    int first = 1;
    int flip = 0;
    int coeff_flip = 0;
    gaddr_t ga_ofmap_cur[2] = {0};

    bmk1880v2_parallel_disable(ctx);

    // split oc
    for (int oc_pos = 0; oc_pos < oc; oc_pos += oc_step) {
      int cur_oc = math_min(oc - oc_pos, oc_step);

      uint64_t coeff_offset = (ig * oc + oc_pos) * sizeof(uint16_t);

      if (do_bias) {
        // 2x 16 bit
        // bmk does not keep eu-align info, user need to update stride if shape changed
        tl_bias[coeff_flip]->shape = _shape_t4(2, cur_oc, 1, 1);
        tl_bias[coeff_flip]->stride = bmk1880v2_tensor_lmem_default_stride(
            ctx, tl_bias[coeff_flip]->shape, FMT_BF16, /*eu_align=*/0);

        LLVM_DEBUG(llvm::errs() << llvm::format(
                       "  [ig=%d][oc_pos=%d] tdma_load_stride_bf16:\n"
                       "    tl_bias gaddr 0x%lx, laddr 0x%x, shape (%d, %d, "
                       "%d, %d), stride (%d, %d, %d)\n",
                       ig, oc_pos, ga_bias + coeff_offset, tl_bias[coeff_flip]->start_address,
                       tl_bias[coeff_flip]->shape.n, tl_bias[coeff_flip]->shape.c,
                       tl_bias[coeff_flip]->shape.h, tl_bias[coeff_flip]->shape.w, bias_gstride.n,
                       bias_gstride.c, bias_gstride.h));
        tdma_load_stride_bf16(ctx, tl_bias[coeff_flip], ga_bias + coeff_offset, bias_gstride,
                              CTRL_WEIGHT);
      }

      // Weight shape for load != shape for tiu
      // bmk does not keep eu-align info, user need to update stride if shape changed
      tl_weight[coeff_flip]->shape = _shape_t4(ic, cur_oc, kh, kw);
      tl_weight[coeff_flip]->stride = bmk1880v2_tensor_lmem_default_stride(
          ctx, tl_weight[coeff_flip]->shape, FMT_BF16, /*eu_align*/ 0);

      uint64_t weight_offset = (ig * oc * ic * kh * kw + oc_pos * ic * kh * kw) * sizeof(uint16_t);
      {
        // Same local address, different shape, stride
        bmk1880v2_tensor_lmem_t tl_tmp;
        tl_tmp.start_address = tl_weight[coeff_flip]->start_address;
        tl_tmp.fmt = FMT_BF16;
        tl_tmp.shape = _shape_t4(1, cur_oc, kh * kw, ic);
        tl_tmp.stride =
            bmk1880v2_tensor_lmem_default_stride(ctx, tl_tmp.shape, FMT_BF16, /*eu_align=*/0);

        LLVM_DEBUG(llvm::errs() << llvm::format(
                       "  [ig=%d][oc_pos=%d] tdma_load_stride_bf16:\n"
                       "    tl_weight gaddr 0x%lx, laddr 0x%x, shape (%d, %d, "
                       "%d, %d), stride (%d, %d, %d)\n",
                       ig, oc_pos, weight_offset, tl_tmp.start_address, tl_tmp.shape.n,
                       tl_tmp.shape.c, tl_tmp.shape.h, tl_tmp.shape.w, tl_tmp.stride.n,
                       tl_tmp.stride.c, tl_tmp.stride.h, tl_tmp.stride.w));
        tdma_load_stride_bf16(ctx, &tl_tmp, ga_weight + weight_offset, weight_gstride, CTRL_WEIGHT);
      }

      // bmk1880v2_tensor_lmem_shape_t ifmap_shape[2] = {0};
      // bmk1880v2_tensor_lmem_shape_t ofmap_shape[2] = {0};
      // gaddr_t ga_ifmap_cur[2] = {0};

      // split n
      for (int n_pos = 0; n_pos < input_n; n_pos += n_step) {
        int cur_n = math_min(input_n - n_pos, n_step);

        // split h
        for (int oh_pos = 0; oh_pos < oh; oh_pos += oh_step) {
          int cur_oh = math_min(oh - oh_pos, oh_step);

          int oh_top = oh_pos;
          int oh_bot = oh_top + cur_oh;
          int ih_top = math_max(oh_top * stride_h - pad_top, 0);
          int ih_bot = math_min((oh_bot - 1) * stride_h + kh_ext - pad_top, input_h);
          int cur_ih = ih_bot - ih_top;

          int ph_top = 0;
          if (ih_top == 0) {
            ph_top = pad_top - oh_top * stride_h;
          }

          int ph_bot = 0;
          if (ih_bot == input_h) {
            ph_bot = (oh_bot - 1) * stride_h + kh_ext - pad_top - input_h;
          }

          // split w
          for (int ow_pos = 0; ow_pos < ow; ow_pos += ow_step) {
            int cur_ow = math_min(ow - ow_pos, ow_step);

            int ow_left = ow_pos;
            int ow_right = ow_left + cur_ow;
            int iw_left = math_max(ow_left * stride_w - pad_left, 0);
            int iw_right = math_min((ow_right - 1) * stride_w + kw_ext - pad_left, input_w);
            int cur_iw = iw_right - iw_left;

            int pw_left = 0;
            if (iw_left == 0) {
              pw_left = pad_left - ow_left * stride_w;
            }

            int pw_right = 0;
            if (iw_right == input_w) {
              pw_right = (ow_right - 1) * stride_w + kw_ext - pad_left - input_w;
            }

            LLVM_DEBUG(llvm::errs()
                       << llvm::format("  [ig=%d][oc_pos=%d][n_pos=%d][oh_pos=%d][ow_pos=%d]"
                                       " cur_oh %d, cur_ih %d, ih_top %d, ih_bot %d"
                                       ", cur_ow %d, cur_iw %d, iw_left %d, iw_right %d\n",
                                       ig, oc_pos, n_pos, oh_pos, ow_pos, cur_oh, cur_ih, ih_top,
                                       ih_bot, cur_ow, cur_iw, iw_left, iw_right));

            // Adjust current shape and stride
            // bmk does not keep eu-align info, user need to update stride if shape changed
            tl_ofmap[flip]->shape = _shape_t4(cur_n, cur_oc, cur_oh, cur_ow);
            tl_ofmap[flip]->stride = bmk1880v2_tensor_lmem_default_stride(
                ctx, tl_ofmap[flip]->shape, FMT_BF16, /*eu_align=*/1);

            // bmk does not keep eu-align info, user need to update stride if shape changed
            tl_ifmap[flip]->shape = _shape_t4(cur_n, ic, cur_ih, cur_iw);
            tl_ifmap[flip]->stride = bmk1880v2_tensor_lmem_default_stride(
                ctx, tl_ifmap[flip]->shape, FMT_BF16, /*eu_align=*/1);

            uint64_t ifmap_offset = (ig * ic * input_h * input_w + n_pos * input_c * input_h * input_w +
                                ih_top * input_w + iw_left) *
                               sizeof(uint16_t);

            LLVM_DEBUG(
                llvm::errs() << llvm::format(
                    "  [ig=%d][oc_pos=%d][n_pos=%d][oh_pos=%d][ow_pos=%d] tdma_load_stride_bf16:\n"
                    "    tl_ifmap gaddr 0x%lx, laddr 0x%x, shape (%d, %d, "
                    "%d, %d), stride (%d, %d, %d)\n",
                    ig, oc_pos, n_pos, oh_pos, ow_pos, ifmap_offset, tl_ifmap[flip]->start_address,
                    tl_ifmap[flip]->shape.n, tl_ifmap[flip]->shape.c, tl_ifmap[flip]->shape.h,
                    tl_ifmap[flip]->shape.w, tl_ifmap[flip]->stride.n, tl_ifmap[flip]->stride.c,
                    tl_ifmap[flip]->stride.h, tl_ifmap[flip]->stride.w));

            tdma_load_stride_bf16(ctx, tl_ifmap[flip], ga_ifmap + ifmap_offset, ifmap_gstride,
                                  CTRL_NEURON);

            bmk1880v2_parallel_disable(ctx);
            bmk1880v2_parallel_enable(ctx);

            {
              bmk1880v2_tiu_convolution_param_t param;
              memset(&param, 0, sizeof(param));
              param.ofmap = tl_ofmap[flip];
              param.ifmap = tl_ifmap[flip];
              param.weight = tl_weight[coeff_flip];
              param.bias = tl_bias[coeff_flip];
              param.ins_h = param.ins_last_h = 0;
              param.ins_w = param.ins_last_w = 0;
              param.pad_top = ph_top;
              param.pad_bottom = ph_bot;
              param.pad_left = pw_left;
              param.pad_right = pw_right;
              param.stride_h = stride_h;
              param.stride_w = stride_w;
              param.dilation_h = dilation_h;
              param.dilation_w = dilation_w;
              param.relu_enable = fused_conv_relu;
              param.ps32_mode = 0;
              param.w_is_const = 0;
              param.layer_id = layer_id;

              LLVM_DEBUG(llvm::errs() << llvm::format(
                             "  [ig=%d][oc_pos=%d][n_pos=%d][oh_pos=%d][ow_pos=%d] conv:\n"
                             "    ifmap la_addr 0x%x, shape (%d, %d, %d, %d)\n"
                             "    weight la_addr 0x%x, shape (%d, %d, %d, %d)\n"
                             "    ofmap la_addr 0x%x, shape (%d, %d, %d, %d)\n",
                             ig, oc_pos, n_pos, oh_pos, ow_pos, param.ifmap->start_address,
                             param.ifmap->shape.n, param.ifmap->shape.c, param.ifmap->shape.h,
                             param.ifmap->shape.w, param.weight->start_address,
                             param.weight->shape.n, param.weight->shape.c, param.weight->shape.h,
                             param.weight->shape.w, param.ofmap->start_address,
                             param.ofmap->shape.n, param.ofmap->shape.c, param.ofmap->shape.h,
                             param.ofmap->shape.w));

              bmk1880v2_tiu_convolution(ctx, &param);
            }

            ga_ofmap_cur[flip] = ga_ofmap + (ig * oc * oh * ow + n_pos * output_c * oh * ow +
                                             oc_pos * oh * ow + oh_top * ow + ow_left) *
                                                sizeof(uint16_t);

            if (!is_reuse_weight()) {
              flip = 1;
              first = 0;
            }

            if (first) {
              // postpone first result to next loop
              // loop0: LD0 TIU0
              // loop1: LD1 TIU1 SD0
              // loop2: LD2 TIU2 SD1
              first = 0;
            } else {
              int flip_back = 1 - flip;

              // Store back to global memory
              LLVM_DEBUG(llvm::errs() << llvm::format(
                             "  [ig=%d][oc_pos=%d][n_pos=%d][oh_pos=%d][ow_pos=%d] "
                             "tdma_store_stride_bf16:\n"
                             "    tl_ofmap gaddr 0x%lx, laddr 0x%x, shape (%d, %d, "
                             "%d, %d), stride (%d, %d, %d)\n",
                             ig, oc_pos, n_pos, oh_pos, ow_pos, ga_ofmap_cur[flip_back],
                             tl_ofmap[flip_back]->start_address, tl_ofmap[flip_back]->shape.n,
                             tl_ofmap[flip_back]->shape.c, tl_ofmap[flip_back]->shape.h,
                             tl_ofmap[flip_back]->shape.w, tl_ofmap[flip_back]->stride.n,
                             tl_ofmap[flip_back]->stride.c, tl_ofmap[flip_back]->stride.h,
                             tl_ofmap[flip_back]->stride.w));

              tdma_store_stride_bf16(ctx, tl_ofmap[flip_back], ga_ofmap_cur[flip_back],
                                     ofmap_gstride, CTRL_NEURON);
            }

            flip = 1 - flip;

          }  // for (int ow_pos = 0; ow_pos < ow; ow_pos += ow_step)

        }  // for (int oh_i = 0; oh_i < oh; oh_i += oh_step)

      }  // for (int n_i = 0; n_i < n; ni += n_step)

      if (!is_reuse_weight()) {
        coeff_flip = 1;
      }

      coeff_flip = 1 - coeff_flip;

    }  // for (int oc_i = 0; oc_i < oc; oc_i += oc_step

    bmk1880v2_parallel_disable(ctx);

    // the last iteration stored the other side, leave the last side not stored
    if (!is_reuse_weight()) {
      // TODO: no need to store last one cuz we store every loop
      flip = 1;
    } else {
      int flip_back = 1 - flip;

      // Store back to global memory
      LLVM_DEBUG(llvm::errs() << llvm::format(
                     "  [ig=%d] tdma_store_stride_bf16:\n"
                     "    tl_ofmap gaddr 0x%lx, laddr 0x%x, shape (%d, %d, "
                     "%d, %d), stride (%d, %d, %d)\n",
                     ig, ga_ofmap_cur[flip_back], tl_ofmap[flip_back]->start_address,
                     tl_ofmap[flip_back]->shape.n, tl_ofmap[flip_back]->shape.c,
                     tl_ofmap[flip_back]->shape.h, tl_ofmap[flip_back]->shape.w,
                     tl_ofmap[flip_back]->stride.n, tl_ofmap[flip_back]->stride.c,
                     tl_ofmap[flip_back]->stride.h, tl_ofmap[flip_back]->stride.w));

      tdma_store_stride_bf16(ctx, tl_ofmap[flip_back], ga_ofmap_cur[flip_back], ofmap_gstride,
                             CTRL_NEURON);
    }

  }  // for (int group_i = 0; group_i < groups; ++groups)

  //
  // Release resource in reverse order
  //
  if (do_bias) {
    if (is_reuse_weight()) bmk1880v2_lmem_free_tensor(ctx, tl_bias[1]);

    bmk1880v2_lmem_free_tensor(ctx, tl_bias[0]);
  }
  if (is_reuse_weight()) bmk1880v2_lmem_free_tensor(ctx, tl_ofmap[1]);

  bmk1880v2_lmem_free_tensor(ctx, tl_ofmap[0]);

  if (is_reuse_weight()) bmk1880v2_lmem_free_tensor(ctx, tl_ifmap[1]);

  bmk1880v2_lmem_free_tensor(ctx, tl_ifmap[0]);

  if (is_reuse_weight()) bmk1880v2_lmem_free_tensor(ctx, tl_weight[1]);

  bmk1880v2_lmem_free_tensor(ctx, tl_weight[0]);

  LLVM_DEBUG(llvm::errs() << "<=ConvReuseWeight"
                          << "/n");
}

int bf16_hists_svm(bmk1880v2_context_t* ctx, uint64_t gaddr_image, uint64_t gaddr_nc_image,
                   bmk1880v2_tensor_tgmem_shape_t image_shape, uint64_t re_order_gaddr_svm,
                   bmk1880v2_tensor_tgmem_shape_t svm_shape,  // (oc, ic, kh, kw)
                   uint64_t gaddr_output, int unit_size, fmt_t fmt) {
  int ret = 0;
  ASSERT(image_shape.n == 1 && image_shape.c == 1 && "image_shape should 2 dims");
  // ASSERT(svm_shape.n == unit_size && "svm_shape channel MUST eq unit_size");
  ASSERT(fmt == FMT_BF16 && "only support FMT_BF16");
  // 1. nc load transpose, for split unit
  // 2. store back for load to channel
  // 3. load c by unit
  // 4. weight MUST re-order for step 2
  // 5. conv

  bmk1880v2_tensor_tgmem_t src;
  init_tgmem(&src);

  // 1. nc load transpose, for split unit
  bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t p1;
  bmk1880v2_tensor_tgmem_shape_t image_shape_expend_unit;
  image_shape_expend_unit.n = image_shape.h * image_shape.w;
  image_shape_expend_unit.c = unit_size;
  image_shape_expend_unit.h = 1;
  image_shape_expend_unit.w = 1;

  src.fmt = fmt;
  src.start_address = gaddr_image;
  src.shape = image_shape_expend_unit;
  src.stride = bmk1880v2_tensor_tgmem_default_stride(src.shape, src.fmt);

  bmk1880v2_tensor_lmem_shape_t l_image_shape_expend_unit;
  l_image_shape_expend_unit.n = image_shape_expend_unit.c;
  l_image_shape_expend_unit.c = image_shape_expend_unit.n;
  l_image_shape_expend_unit.h = 1;
  l_image_shape_expend_unit.w = 1;

  bmk1880v2_tensor_lmem_t* dst =
      bmk1880v2_lmem_alloc_tensor(ctx, l_image_shape_expend_unit, fmt, CTRL_NULL);

  memset(&p1, 0, sizeof(p1));
  p1.src = &src;
  p1.dst = dst;
  bmk1880v2_tdma_g2l_bf16_tensor_copy_nc_transposed(ctx, &p1);

  // 2. store back for load to channel
  bmk1880v2_tdma_l2tg_tensor_copy_param_t p2;
  memset(&p2, 0, sizeof(p2));
  copy_tl_tg_tensor_shape(&image_shape_expend_unit, &l_image_shape_expend_unit);

  src.start_address = gaddr_nc_image;
  src.shape = image_shape_expend_unit;
  src.stride = bmk1880v2_tensor_tgmem_default_stride(src.shape, src.fmt);

  p2.src = dst;
  p2.dst = &src;
  bmk1880v2_tdma_l2g_bf16_tensor_copy(ctx, &p2);

  bmk1880v2_lmem_free_tensor(ctx, dst);

  // tiling conv, copy from backend
  if (1) {
    int input_n = 1;
    int groups = 1;
    uint16_t dilation_h = 1, dilation_w = 1;
    uint8_t pad_top = 0;
    uint8_t pad_bottom = 0, pad_left = 0, pad_right = 0, stride_h = 1, stride_w = 1;

    _split(ctx, input_n, unit_size, image_shape.h, image_shape.w, groups, svm_shape.n, svm_shape.h,
           svm_shape.w, dilation_h, dilation_w, pad_top, pad_bottom, pad_left, pad_right, stride_h,
           stride_w);

    ConvReuseWeight(ctx, gaddr_nc_image, gaddr_output, re_order_gaddr_svm, input_n, unit_size,
                    image_shape.h, image_shape.w, groups, svm_shape.n, svm_shape.h, svm_shape.w,
                    dilation_h, dilation_w, pad_top, pad_bottom, pad_left, pad_right, stride_h,
                    stride_w);
  } else {
    // 3. load c by unit
    bmk1880v2_tdma_tg2l_tensor_copy_param_t p3;
    memset(&p3, 0, sizeof(p3));
    image_shape_expend_unit.n = 1;
    image_shape_expend_unit.c = unit_size;
    image_shape_expend_unit.h = image_shape.h;
    image_shape_expend_unit.w = image_shape.w;

    copy_tg_tl_tensor_shape(&l_image_shape_expend_unit, &image_shape_expend_unit);

    bmk1880v2_tensor_lmem_t* tl_ifmap =
        bmk1880v2_lmem_alloc_tensor(ctx, l_image_shape_expend_unit, fmt, CTRL_AL);

    p3.src = &src;
    p3.dst = tl_ifmap;
    bmk1880v2_tdma_g2l_bf16_tensor_copy(ctx, &p3);

    // 4. weight MUST re-order for step 2
    // bmk1880v2_tensor_lmem_t bmk1880v2_lmem_alloc_tensor(ctx, _shape_t4(ic, oc_step, kh, kw),
    // FMT_BF16, CTRL_NULL);
    // weight from origin layout (oc, ic, kh, kw) transform to (1, oc, kh*kw, ic)
    bmk1880v2_tensor_tgmem_shape_t transpose_svm_shape;
    transpose_svm_shape.n = 1;
    transpose_svm_shape.c = svm_shape.n;
    transpose_svm_shape.h = svm_shape.h * svm_shape.w;
    transpose_svm_shape.w = svm_shape.c;

    src.start_address = re_order_gaddr_svm;
    src.shape = image_shape_expend_unit;
    src.base_reg_index = 1;

    bmk1880v2_tensor_lmem_shape_t l_transpose_svm_shape;
    copy_tg_tl_tensor_shape(&l_transpose_svm_shape, &transpose_svm_shape);
    bmk1880v2_tensor_lmem_t* tl_weight =
        bmk1880v2_lmem_alloc_tensor(ctx, l_transpose_svm_shape, fmt, CTRL_NULL);

    p3.src = &src;
    p3.dst = tl_weight;
    bmk1880v2_tdma_g2l_bf16_tensor_copy(ctx, &p3);

    // 5. conv
    // alloc output
    bmk1880v2_tensor_lmem_shape_t l_out_shape;
    conv_output(1, &l_out_shape, &image_shape, &svm_shape);
    bmk1880v2_tensor_lmem_t* tl_ofmap =
        bmk1880v2_lmem_alloc_tensor(ctx, l_out_shape, fmt, CTRL_AL);

    bmk1880v2_tiu_convolution_param_t param;
    memset(&param, 0, sizeof(param));
    param.ofmap = tl_ofmap;
    param.ifmap = tl_ifmap;
    param.weight = tl_weight;
    param.bias = NULL;
    param.ins_h = param.ins_last_h = 0;
    param.ins_w = param.ins_last_w = 0;
    param.pad_top = 0;
    param.pad_bottom = 0;
    param.pad_left = 0;
    param.pad_right = 0;
    param.stride_h = 1;
    param.stride_w = 1;
    param.dilation_h = 1;
    param.dilation_w = 1;
    param.relu_enable = 0;
    param.ps32_mode = 0;
    param.w_is_const = 0;
    param.layer_id = 0;

    bmk1880v2_tiu_convolution(ctx, &param);

    bmk1880v2_tensor_tgmem_shape_t out_shape;
    copy_tl_tg_tensor_shape(&out_shape, &l_out_shape);

    src.start_address = gaddr_output;
    src.shape = out_shape;
    src.base_reg_index = 0;

    p2.src = tl_ofmap;
    p2.dst = &src;
    bmk1880v2_tdma_l2g_bf16_tensor_copy(ctx, &p2);

    bmk1880v2_lmem_free_tensor(ctx, tl_ofmap);
    bmk1880v2_lmem_free_tensor(ctx, tl_weight);
    bmk1880v2_lmem_free_tensor(ctx, tl_ifmap);
  }

  return ret;
}
