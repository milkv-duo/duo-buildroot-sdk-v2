#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>
#include <runtime/kernel_function.hpp>
#include <runtime/debug.h>

namespace cvi {
namespace runtime {

static inline int ceiling_func(int numerator, int denominator) {
  return (numerator + denominator - 1) / denominator;
}

static inline uint64_t align_up(uint64_t x, uint64_t n) {
  return (x + n - 1) / n * n;
}

static void tdma_load_stride(cvk_context_t* cvk, cvk_tl_t* tensor,
    uint64_t ga_src, cvk_tg_stride_t stride, int32_t reg_idx) {
  cvk_tg_t src;
  src.base_reg_index = reg_idx;
  src.fmt = CVK_FMT_U8;
  src.shape = {tensor->shape.n, tensor->shape.c,
               tensor->shape.h, tensor->shape.w};
  src.start_address = ga_src;
  src.stride = stride;

  cvk_tdma_g2l_tensor_copy_param_t p = {0};
  p.src = &src;
  p.dst = tensor;
  p.layer_id = 0;

  cvk->ops->tdma_g2l_tensor_copy(cvk, &p);
}

static void tdma_store_stride(cvk_context_t *cvk, cvk_tl_t *tensor,
    uint64_t ga_dst, cvk_tg_stride_t stride) {
  cvk_tg_t dst;
  dst.base_reg_index = 3;
  dst.fmt = CVK_FMT_U8;
  dst.start_address = ga_dst;
  dst.shape = {tensor->shape.n, tensor->shape.c,
               tensor->shape.h, tensor->shape.w};
  dst.stride = stride;

  cvk_tdma_l2g_tensor_copy_param_t p;
  p.src = tensor;
  p.dst = &dst;
  p.layer_id = 0;
  cvk->ops->tdma_l2g_tensor_copy(cvk, &p);
}

static void calc_background(cvk_context_t *cvk,
    int32_t ih, int32_t iw, int32_t oh, int32_t ow,
    int32_t k, int32_t s, int32_t pad) {
  uint64_t x_ga = 0;
  uint64_t y_ga = 0;
  int32_t kh = k;
  int32_t kw = k;
  int32_t sh = s;
  int32_t sw = s;

  int32_t step_oh = 0, step_ow = 0;
  int32_t x_tl_sz = 0, y_tl_sz = 0;

  // determin the shape of tile.
  for (step_ow = ow; step_ow > 0; step_ow--) {
    for (step_oh = oh; step_oh > 0; step_oh--) {
      auto step_ih = (step_oh - 1) * sh + kh;
      auto step_iw = (step_ow - 1) * sw + kw;
      if (step_ih > ih) {
        step_ih = ih;
      }
      if (step_iw > iw) {
        step_iw = iw;
      }
      cvk_tl_shape_t input_shape = {1, 1, (uint32_t)step_ih, (uint32_t)step_iw};
      cvk_tl_shape_t output_shape = {1, 1, (uint32_t)step_oh, (uint32_t)step_ow};
      x_tl_sz = cvk->ops->lmem_tensor_to_size(cvk, input_shape, CVK_FMT_U8, 1);
      y_tl_sz = cvk->ops->lmem_tensor_to_size(cvk, output_shape, CVK_FMT_U8, 1);
      auto total_lmem = x_tl_sz + y_tl_sz;
      if (total_lmem <= (int32_t)cvk->info.lmem_size) {
        goto do_tile;
      }
    }
  }
do_tile:
  cvk_tg_stride_t ga_stride = cvk->ops->tg_default_stride(
      cvk, {1, 1, (uint32_t)ih, (uint32_t)iw}, CVK_FMT_U8);
  for (int oh_pos = 0; oh_pos < oh; oh_pos += step_oh) {
    int32_t cur_oh = std::min(oh - oh_pos, step_oh);
    int32_t oh_top = oh_pos;
    int32_t oh_bot = oh_pos + cur_oh;
    int32_t ih_top = std::max(oh_top * sh - pad, 0);
    int32_t ih_bot = std::min((oh_bot - 1) * sh + kh - pad, ih);
    int32_t cur_ih = ih_bot - ih_top;
    int32_t cur_pad_t = (ih_top == 0) ? (pad - oh_top * sh) : 0;
    int32_t cur_pad_b = (ih_bot == ih) ? ((oh_bot - 1) * sh + kh - pad - ih) : 0;

    for (int ow_pos = 0; ow_pos < ow; ow_pos += step_ow) {
      int32_t cur_ow = std::min(ow - ow_pos, step_ow);
      int32_t ow_left = ow_pos;
      int32_t ow_right = ow_pos + cur_ow;
      int32_t iw_left = std::max(ow_left * sw - pad, 0);
      int32_t iw_right = std::min((ow_right - 1) * sw + kw - pad, iw);
      int32_t cur_iw = iw_right - iw_left;
      int32_t cur_pad_l = (iw_left == 0) ? (pad - ow_left * sw) : 0;
      int32_t cur_pad_r = (iw_right == iw) ? ((ow_right - 1) * sw + kw - pad - iw) : 0;
      cvk_tl_t x, bkg;
      x.fmt = CVK_FMT_U8;
      x.start_address = 0;
      x.shape = {1, 1, (uint32_t)cur_ih, (uint32_t)cur_iw};
      x.stride = cvk->ops->tl_default_stride(cvk, x.shape, x.fmt, 1);
      tdma_load_stride(cvk, &x, x_ga + ih_top * iw + iw_left, ga_stride, 2);

      bkg = x;
      bkg.fmt = CVK_FMT_U8;
      bkg.start_address = x_tl_sz;
      bkg.shape = {1, 1, (uint32_t)cur_oh, (uint32_t)cur_ow};
      bkg.stride = cvk->ops->tl_default_stride(cvk, bkg.shape, bkg.fmt, 1);

      cvk_tiu_max_pooling_param_t p0 = {0};
      p0.ofmap = &bkg;
      p0.ifmap = &x;
      p0.kh = kh;
      p0.kw = kw;
      p0.pad_top = cur_pad_t;
      p0.pad_bottom = cur_pad_b;
      p0.pad_left = cur_pad_l;
      p0.pad_right = cur_pad_r;
      p0.stride_h = 1;
      p0.stride_w = 1;
      cvk->ops->tiu_max_pooling(cvk, &p0);
      tdma_store_stride(cvk, &bkg, y_ga + oh_pos * ow + ow_pos, ga_stride);
    }
  }
}

static void compute(cvk_context_t *cvk, int32_t offset, int32_t ic, int32_t ih, int32_t iw) {
  uint64_t x_ga = 0;
  uint64_t y_ga = 0;

  cvk_fmt_t fmt = CVK_FMT_U8;
  cvk_tl_t x, bkg, mask, diff, mask_high, diff_high;
  cvk_tl_t y, tmp_high;

  uint32_t lmem_address = 0;
  x.start_address = lmem_address;
  x.fmt = fmt;
  x.shape = {1, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw};
  x.stride = cvk->ops->tl_default_stride(cvk, x.shape, fmt, 1);
  cvk_tg_stride_t ga_stride = cvk->ops->tg_default_stride(
          cvk, {1, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw}, fmt);
  uint32_t tl_size = cvk->ops->lmem_tensor_to_size(cvk, x.shape, fmt, 1);
  lmem_address += tl_size;

  bkg = x;
  bkg.start_address = lmem_address;
  lmem_address += tl_size;

  mask = x;
  mask.start_address = lmem_address;
  lmem_address += tl_size;

  diff = x;
  diff.fmt = CVK_FMT_I8;
  diff.start_address = lmem_address;
  lmem_address += tl_size;

  mask_high = x;
  mask_high.start_address = lmem_address;
  lmem_address += tl_size;

  diff_high = diff;
  diff_high.start_address = lmem_address;
  lmem_address += tl_size;
  assert(lmem_address < cvk->info.lmem_size);
  y = mask;
  tmp_high = mask_high;

  tdma_load_stride(cvk, &x, x_ga + offset, ga_stride, 2);
  tdma_load_stride(cvk, &bkg, y_ga + offset, ga_stride, 3);

  cvk_tiu_ge_param_t p1 = {0};
  p1.ge = &mask;
  p1.a = &x;
  p1.b = &bkg;
  cvk->ops->tiu_ge(cvk, &p1);

  cvk_tiu_mul_param_t p2 = {0};
  mask.fmt = CVK_FMT_I8;
  mask_high.fmt = CVK_FMT_I8;
  p2.res_high = &mask_high;
  p2.res_low = &mask;
  p2.a = &mask;
  p2.b_is_const = 1;
  p2.b_const.val = 255;
  p2.b_const.is_signed = 1;
  cvk->ops->tiu_mul(cvk, &p2);

  cvk_tiu_xor_int8_param_t p3 = {0};
  p3.res = &tmp_high;
  p3.a = &tmp_high;
  p3.b = &tmp_high;
  cvk->ops->tiu_xor_int8(cvk, &p3);

  cvk_tiu_sub_param_t p4 = {0};
  p4.res_high = &diff_high;
  p4.res_low = &diff;
  p4.a_high = &tmp_high;
  p4.a_low = &x;
  p4.b_high = &tmp_high;
  p4.b_low = &bkg;
  cvk->ops->tiu_sub(cvk, &p4);

  cvk_tiu_add_param_t p5 = {0};
  p5.res_high = &diff_high;
  p5.res_low = &diff;
  p5.a_high = &diff_high;
  p5.a_low = &diff;
  p5.b_is_const = 1;
  p5.b_const.val = 255;
  p5.b_const.is_signed = 1;
  cvk->ops->tiu_add(cvk, &p5);

  cvk_tiu_add_param_t p6 = {0};
  p6.res_low = &y;
  p6.a_high = &diff_high;
  p6.a_low = &diff;
  p6.b.high = &mask_high;
  p6.b.low = &mask;
  cvk->ops->tiu_add(cvk, &p6);

  tdma_store_stride(cvk, &y, y_ga + offset, ga_stride);
}

CVI_RT_MEM runtimeJitGrayImageLight(
    CVI_RT_HANDLE ctx, void* cvk_ctx,
    int32_t ih, int32_t iw, int32_t kernel_sz) {

  auto cvk = (cvk_context_t*)cvk_ctx;
  int32_t pad = (kernel_sz - 1) / 2;
  calc_background(cvk, ih, iw, ih, iw, kernel_sz, 1, pad);

  int32_t block_num = 6;
  int32_t eu_num = cvk->info.eu_num;
  int32_t npu_num = cvk->info.npu_num;
  int32_t total_sz = align_up(ih * iw, eu_num);
  uint32_t remain = total_sz;
  uint32_t offset = 0;
  int32_t max_h = cvk->info.lmem_size / (eu_num * block_num);
  max_h = std::min(max_h, 4096 - 32);
  assert(max_h);
  int32_t cur_h = max_h;

  int32_t loop = remain / (npu_num * eu_num * max_h);
  remain %= npu_num * eu_num * max_h;
  for (int32_t i = 0; i < loop; i++) {
    compute(cvk, offset, npu_num, cur_h, eu_num);
    offset += 1 * npu_num * cur_h * eu_num;
  }
  if (remain) {
    int32_t cur_c = remain / (eu_num * cur_h);
    if (cur_c) {
      compute(cvk, offset, cur_c, cur_h, eu_num);
      offset += 1 * cur_c * cur_h * eu_num;
    }

    remain %= eu_num * cur_h;
    if (remain) {
      compute(cvk, offset, 1, ceiling_func(remain, eu_num), eu_num);
    }
  }

  CVI_RT_MEM cmdbuf_mem;
  uint32_t size;
  auto cmdbuf = cvk->ops->acquire_cmdbuf(cvk, &size);
  int ret = CVI_RT_LoadCmdbuf(ctx, cmdbuf, size, 0, 0, false, &cmdbuf_mem);
  assert(ret == 0);
  cvk->ops->reset(cvk);
  return cmdbuf_mem;
}


}
}