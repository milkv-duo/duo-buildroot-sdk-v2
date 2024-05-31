#include <cvimath_internal.h>
#include "gen_lut.h"

// only fill base_reg_index/int8_rnd_mode
static void init_tgmem(cvk_tg_t* t) {
  t->base_reg_index = 0;
  t->int8_rnd_mode = 0;
}

int cvm_s2s_fp32_bf16(cvk_context_t* ctx, uint64_t gaddr_fp32, cvk_tg_shape_t fp32_shape,
                      uint64_t gaddr_bf16, cvk_tg_shape_t cvm_shape, cvk_fmt_t fmt) {
  int ret = 0;
  ASSERT(fmt == CVK_FMT_BF16 && "only support CVK_FMT_BF16");
  ASSERT(fp32_shape.w % 2 == 0 && "fp32's w MUST align with 2");

  cvk_tdma_g2g_tensor_copy_param_t p;

  cvk_tg_t src, dst;

  init_tgmem(&src);
  init_tgmem(&dst);

  int fp32_w = 2;
  src.fmt = fmt;
  src.start_address = gaddr_fp32 + fp32_w;  // copy from high part
  src.shape = fp32_shape;
  src.shape.h = fp32_shape.w * fp32_shape.h / fp32_w;
  src.shape.w = 1;

  int fmt_sz = bytesize_of_fmt(fmt);
  src.stride.n = fp32_shape.w * fp32_shape.h * fp32_shape.c * fmt_sz;
  src.stride.c = fp32_shape.w * fp32_shape.h * fmt_sz;
  src.stride.h = fp32_w * fmt_sz;

  dst.fmt = fmt;
  dst.start_address = gaddr_bf16;
  dst.shape = cvm_shape;
  dst.shape.h = cvm_shape.w * cvm_shape.h / fp32_w;
  dst.shape.w = 1;
  dst.stride = ctx->ops->tg_default_stride(ctx, dst.shape, dst.fmt);

  p.src = &src;
  p.dst = &dst;

  ctx->ops->tdma_g2g_bf16_tensor_copy(ctx, &p);

  return ret;
}

// default implement by s->s
void cvm_bf16_fp32(cvk_context_t* cvk_ctx, cvk_tg_t* tg_bf16, cvk_tg_t* tg_fp32) {
#if 0
    // sys->local->sys implement
  cvk_fmt_t fmt = tg_bf16->fmt;
  cvk_tl_shape_t tl_shape;
  int ctrl = CTRL_AL; // eu align

  tl_shape.n = tg_fp32->shape.n;
  tl_shape.c = tg_fp32->shape.c;
  tl_shape.h = tg_fp32->shape.h;
  tl_shape.w = tg_fp32->shape.w;

  // 1. fill local memory to 0 for mantissa
  cvk_tl_t *tl_ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, tg_bf16->fmt, ctrl);
  cvk_tiu_mul_param_t p0;
  p0.res_high = NULL;
  p0.res_low = tl_ofmap;
  p0.a = tl_ofmap;
  p0.b_is_const = 1;
  p0.b_const.val = 0;
  p0.b_const.is_signed = 0;
  p0.rshift_bits = 0;
  p0.relu_enable = 0;
  p0.layer_id = 0;

  cvk_ctx->ops->tiu_mul(cvk_ctx, &p0);


  // pretend the same shape, reshape h, w to h * w, 1
  int fmt_bytesize = cvm_bytesize_of_fmt(tl_ofmap->fmt);
  tl_ofmap->shape.w = 1;
  tl_ofmap->shape.h = tg_bf16->shape.h * tg_bf16->shape.w;
  tl_ofmap->stride.h = 4;
  tl_ofmap->stride.c = align_up(tg_fp32->shape.w * tg_fp32->shape.h * fmt_bytesize,
      cvk_ctx->info.eu_num);
  tl_ofmap->stride.n = tl_ofmap->stride.c * ceiling_func(tg_fp32->shape.c,
      cvk_ctx->info.npu_num);


  // 2. load from tg with reshaped w
  // FIXME: check overwrite
  tl_ofmap->start_address = tl_ofmap->start_address + 2;// 2 means shift fp32 high 16 part
  cvk_tdma_g2l_tensor_copy_param_t p;
  p.src = tg_bf16;
  p.dst = tl_ofmap;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &p);

  // 3. store back to tg
  tl_ofmap->start_address = tl_ofmap->start_address - 2; //revert
  tl_ofmap->shape = tl_shape;
  tl_ofmap->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_ofmap->shape, fmt, ctrl);

  cvk_tdma_l2g_tensor_copy_param_t p1;
  p1.src = tl_ofmap;
  p1.dst = tg_fp32;
  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &p1);

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_ofmap);
#else
  // sys->sys implement
  // 1. fill tg with low 16but part as 0
  cvk_tdma_l2g_tensor_fill_constant_param_t p0;
  p0.constant = 0;
  p0.dst = tg_fp32;
  p0.layer_id = 0;
  cvk_ctx->ops->tdma_l2g_tensor_fill_constant(cvk_ctx, &p0);

  // 2. sys->sys
  cvk_tdma_g2g_tensor_copy_param_t p1;
  cvk_tg_shape_t shape = tg_fp32->shape;  // backup
  cvk_tg_stride_t stride = tg_fp32->stride;

  tg_fp32->shape.w = 1;
  tg_fp32->shape.h = tg_bf16->shape.h * tg_bf16->shape.w;
  tg_fp32->stride.h = 4;

  tg_fp32->start_address = tg_fp32->start_address + 2;  // +2 means shift from high part
  p1.src = tg_bf16;
  p1.dst = tg_fp32;
  p1.layer_id = 0;
  cvk_ctx->ops->tdma_g2g_bf16_tensor_copy(cvk_ctx, &p1);

  // restore
  tg_fp32->start_address = tg_fp32->start_address - 2;
  tg_fp32->shape = shape;
  tg_fp32->stride = stride;
#endif
}
