#include "../kernel_1880v2.h"

// only fill base_reg_index/int8_rnd_mode
static void init_tgmem(bmk1880v2_tensor_tgmem_t* t) {
  t->base_reg_index = 0;
  t->int8_rnd_mode = 0;
}

int bf16_s2s_fp32_bf16(bmk1880v2_context_t* ctx, uint64_t gaddr_fp32,
                       bmk1880v2_tensor_tgmem_shape_t fp32_shape, uint64_t gaddr_bf16,
                       bmk1880v2_tensor_tgmem_shape_t bf16_shape, fmt_t fmt) {
  int ret = 0;
  ASSERT(fmt == FMT_BF16 && "only support FMT_BF16");
  ASSERT(fp32_shape.w % 2 == 0 && "fp32's w MUST align with 2");

  bmk1880v2_tdma_tg2tg_tensor_copy_param_t p;

  bmk1880v2_tensor_tgmem_t src, dst;

  init_tgmem(&src);
  init_tgmem(&dst);

  int fp32_w = 2;
  src.fmt = fmt;
  src.start_address = gaddr_fp32 + fp32_w;  // copy from high part
  src.shape = fp32_shape;
  src.shape.h = fp32_shape.w * fp32_shape.h / fp32_w;
  src.shape.w = 1;

  int fmt_sz = ceiling_bytesize_of(bitsize_of_fmt(fmt));
  src.stride.n = fp32_shape.w * fp32_shape.h * fp32_shape.c * fmt_sz;
  src.stride.c = fp32_shape.w * fp32_shape.h * fmt_sz;
  src.stride.h = fp32_w * fmt_sz;

  dst.fmt = fmt;
  dst.start_address = gaddr_bf16;
  dst.shape = bf16_shape;
  dst.shape.h = bf16_shape.w * bf16_shape.h / fp32_w;
  dst.shape.w = 1;
  dst.stride = bmk1880v2_tensor_tgmem_default_stride(dst.shape, fmt);

  memset(&p, 0, sizeof(p));
  p.src = &src;
  p.dst = &dst;

  bmk1880v2_tdma_tg2tg_bf16_tensor_copy(ctx, &p);

  return ret;
}
