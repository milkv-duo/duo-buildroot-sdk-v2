#include <cassert>
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <memory.h>

#include <runtime/kernel_function.hpp>

namespace cvi {
namespace runtime {

static void load_and_convert_to_bf16(cvk_context_t *cvk_ctx, cvk_tl_t *tl_mem,
                                     cvk_tl_shape_t &shape,
                                     cvk_tg_stride_t &stride, int x_base_ga_idx,
                                     uint64_t x_ga) {
  assert(tl_mem);
  cvk_tdma_g2l_tensor_copy_param_t p1 = {0};
  cvk_tg_t tg_x;
  tg_x.start_address = x_ga;
  tg_x.base_reg_index = x_base_ga_idx;
  tg_x.int8_rnd_mode = 0;
  tg_x.fmt = CVK_FMT_U8;
  tg_x.shape = {shape.n, shape.c, shape.h, shape.w};
  tg_x.stride = stride;
  p1.src = &tg_x;
  p1.dst = tl_mem;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p1);

  return;
}

static void convert_ps32_to_fp32(cvk_context_t *cvk_ctx, cvk_tl_t *output) {
  assert(output->shape.n == 2); // Exclude lower part
  assert((output->shape.h == 1) && (output->shape.w == 1) && "Only support h=1, w=1");

  uint32_t la_high = output->start_address;
  cvk_tl_t tl_src;
  tl_src.start_address = la_high;
  tl_src.fmt = CVK_FMT_BF16;
  tl_src.shape = output->shape;
  tl_src.shape.n = 1;
  tl_src.stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_src.shape, tl_src.fmt, 1);
  tl_src.stride.n = output->stride.n;

  uint32_t la_low = output->start_address + tl_src.stride.n;
  cvk_tl_t tl_dst;
  tl_dst.start_address = la_low + sizeof(uint16_t); // concat higher part
  tl_dst.fmt = CVK_FMT_BF16;
  tl_dst.shape = output->shape;
  tl_dst.shape.n = 1;
  tl_dst.stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_dst.shape, tl_dst.fmt, 1);
  tl_dst.stride.n = output->stride.n;

  cvk_tiu_copy_param_t param = {0};
  param.src = &tl_src;
  param.dst = &tl_dst;
  param.layer_id = 0;
  cvk_ctx->ops->tiu_copy(cvk_ctx, &param);
}

static void store_fp32(cvk_context_t *cvk_ctx, int base_ga_idx, uint64_t ga_dst, cvk_tl_t *output) {
                       
  assert(output->shape.n == 2); // Exclude lower part
  assert(output->shape.h == 1 && output->shape.w == 1);

  cvk_tl_t src;
  src.fmt = CVK_FMT_BF16;
  src.shape = output->shape;
  src.shape.n = 1;
  src.shape.w = 2;
  src.stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, src.shape, src.fmt, 1);
  src.stride.n = output->stride.n;
  src.start_address = output->start_address + src.stride.n;
  src.eu_align = 1;

  cvk_tg_t dst;
  dst.fmt = CVK_FMT_BF16;
  dst.shape.n = 1;
  dst.shape.c = output->shape.c;
  dst.shape.h = output->shape.h;
  dst.shape.w = 2;
  dst.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, dst.shape, dst.fmt);
  dst.base_reg_index = base_ga_idx;
  dst.start_address = ga_dst;

  cvk_tdma_l2g_tensor_copy_param_t param = {0};
  param.src = &src;
  param.dst = &dst;
  param.layer_id = 0;
  param.intra_cmd_paral = 0;
  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &param);
}

CVI_RT_MEM runtimeJitEuclideanDistance(CVI_RT_HANDLE ctx, void *cvk_ctx, uint32_t records,
                                 uint32_t feature_size) {
  // tile
  auto cvk = (cvk_context_t *)cvk_ctx;
  cvk->ops->set_layer_id(cvk, 0);
  uint32_t lane_num = cvk->info.npu_num;
  uint32_t c_step = 0;
  cvk_tl_shape_t input_x_shape = {1, lane_num, 1, feature_size};
  uint32_t in_x_size =
      cvk->ops->lmem_tensor_to_size(cvk, input_x_shape, CVK_FMT_BF16, 1);
  for (c_step = records; c_step > 0; --c_step) {
    uint32_t total_size = 0;
    cvk_tl_shape_t in_y_shape = {1, c_step, 1, feature_size};
    uint32_t in_y_size = cvk->ops->lmem_tensor_to_size(cvk, in_y_shape, CVK_FMT_BF16, 1);
    cvk_tl_shape_t output_shape = {2, c_step, 1, 2};
    uint32_t output_size =
        cvk->ops->lmem_tensor_to_size(cvk, output_shape, CVK_FMT_BF16, 1);
    total_size += in_x_size;
    total_size += in_y_size;
    total_size += output_size;
    if (total_size < cvk->info.lmem_size) {
      break;
    }
  }
  assert(c_step);

  int x_ga_base_reg_idx = 2;
  int y_ga_base_reg_idx = 3;
  int o_ga_base_reg_idx = 4;

  uint64_t x_ga = 0;
  uint64_t y_ga = 0;
  uint64_t o_ga = 0;

  // alloc lmem
  cvk_tl_t *input_x = cvk->ops->lmem_alloc_tensor(cvk, input_x_shape, CVK_FMT_BF16, 1);
  cvk_tl_shape_t input_y_shape = {1, c_step, 1, feature_size};
  cvk_tl_t *input_y = cvk->ops->lmem_alloc_tensor(cvk, input_y_shape, CVK_FMT_BF16, 1);
  cvk_tl_shape_t output_shape = {2, c_step, 1, 1};
  cvk_tl_t *output =
      cvk->ops->lmem_alloc_tensor(cvk, output_shape, CVK_FMT_BF16, 1);
  assert(input_x);
  assert(input_y);
  assert(output);

  // load input_x
  cvk_tg_shape_t tg_input_x_shape = {1, lane_num, 1, feature_size};
  cvk_tg_stride_t tg_input_x_stride = cvk->ops->tg_default_stride(cvk, tg_input_x_shape, CVK_FMT_U8);
  tg_input_x_stride.c = 0;
  tg_input_x_stride.n = 0;
  load_and_convert_to_bf16(cvk, input_x, input_x_shape, tg_input_x_stride, x_ga_base_reg_idx, x_ga);

  for (uint32_t c_pos = 0; c_pos < records;) {
    uint32_t tile_c = std::min(c_step, records - c_pos);

    //load input_y
    cvk_tl_shape_t input_y_shape = {1, tile_c, 1, feature_size};
    cvk_tg_shape_t tg_input_y_shape = {input_y_shape.n, input_y_shape.c, input_y_shape.h, input_y_shape.w};
    cvk_tg_stride_t tg_input_y_stride = cvk->ops->tg_default_stride(cvk, tg_input_y_shape, CVK_FMT_U8);
    input_y->shape.c = tile_c;
    load_and_convert_to_bf16(cvk, input_y, input_y_shape, tg_input_y_stride, y_ga_base_reg_idx, y_ga + c_pos * feature_size);

    cvk_tl_t b;
    b.start_address = input_x->start_address;
    b.shape = input_y->shape;
    b.stride = input_y->stride;
    b.stride.c = 0;
    b.stride.n = 0;
    b.fmt = input_x->fmt;

    cvk_tiu_sub_param_t p1 = {0};
    p1.res_high = 0;
    p1.res_low = input_y;
    p1.a_high = 0;
    p1.a_low = input_y;
    p1.b_high = 0;
    p1.b_low = &b;
    p1.rshift_bits = 0;
    p1.layer_id = 0;
    cvk->ops->tiu_sub(cvk, &p1);

    output->shape.n = 1;
    output->shape.c = tile_c;

    cvk_tiu_depthwise_pt_convolution_param_t p2 = {0};
    p2.ofmap = output;
    p2.ifmap = input_y;
    p2.weight = input_y;
    p2.bias = nullptr;
    p2.ins_h = 0;
    p2.ins_w = 0;
    p2.ins_last_h = 0;
    p2.ins_last_w = 0;
    p2.pad_top = 0;
    p2.pad_bottom = 0;
    p2.pad_left = 0;
    p2.pad_right = 0;
    p2.stride_h = 1;
    p2.stride_w = 1;
    p2.dilation_h = 1;
    p2.dilation_w = 1;
    p2.relu_enable = false;
    p2.rshift_bits = 0;
    p2.ps32_mode = 2;
    p2.layer_id = 0;
    cvk->ops->tiu_pt_depthwise_convolution(cvk, &p2);

    output->shape.n = 2;
    convert_ps32_to_fp32(cvk, output);

    store_fp32(cvk, o_ga_base_reg_idx, o_ga + c_pos * sizeof(float), output);

    c_pos += tile_c;
  }

  cvk->ops->lmem_free_tensor(cvk, output);
  cvk->ops->lmem_free_tensor(cvk, input_y);
  cvk->ops->lmem_free_tensor(cvk, input_x);

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
