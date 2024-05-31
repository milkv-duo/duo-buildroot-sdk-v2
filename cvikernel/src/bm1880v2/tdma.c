#include "kernel_1880v2.h"
#include "bmkernel/bm1880v2/1880v2_fp_convert.h"

//<! sync define with cmodel
#define FMT_BF16_TYP     2
#define FMT_FIX8B_TYP    1

static void check_tdma_tl_shape(const tl_shape_t *s)
{
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->h < 0x10000);
  ASSERT(s->w < 0x10000);

  ASSERT(s->n > 0x0);
  ASSERT(s->c > 0x0);
  ASSERT(s->h > 0x0);
  ASSERT(s->w > 0x0);
}

static void check_tdma_tl_bf16_shape(const tl_shape_t *s, fmt_t fmt)
{
  uint8_t fmt_type = (fmt == FMT_BF16 ? 2 : 1);
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->h < 0x10000);
  ASSERT(s->w < 0x10000 / fmt_type);

  ASSERT(s->n > 0x0);
  ASSERT(s->c > 0x0);
  ASSERT(s->h > 0x0);
  ASSERT(s->w > 0x0);
}

static void check_tdma_tg_shape(const tg_shape_t *s)
{
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->h < 0x10000);
  ASSERT(s->w < 0x10000);

  ASSERT(s->n > 0x0);
  ASSERT(s->c > 0x0);
  ASSERT(s->h > 0x0);
  ASSERT(s->w > 0x0);
}

static void check_tdma_tg_bf16_shape(const tg_shape_t *s, fmt_t fmt)
{
  uint8_t fmt_type = (fmt == FMT_BF16 ? 2 : 1);
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->h < 0x10000);
  ASSERT(s->w < 0x10000 / fmt_type);

  ASSERT(s->n > 0x0);
  ASSERT(s->c > 0x0);
  ASSERT(s->h > 0x0);
  ASSERT(s->w > 0x0);
}


static void check_tdma_ml_shape(const ml_shape_t *s)
{
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->w < 0x10000);
  ASSERT(s->col < 0x10000);

  ASSERT(s->n > 0);
  ASSERT(s->c > 0);
  ASSERT(s->w > 0);
  ASSERT(s->col > 0);
}

static void check_tdma_ml_bf16_shape(const ml_shape_t *s, fmt_t fmt)
{
  uint8_t fmt_type = (fmt == FMT_BF16 ? 2 : 1);
  ASSERT(s->n < 0x10000);
  ASSERT(s->c < 0x10000);
  ASSERT(s->w < 0x10000 / fmt_type);
  ASSERT(s->col < 0x10000);

  ASSERT(s->n > 0);
  ASSERT(s->c > 0);
  ASSERT(s->w > 0);
  ASSERT(s->col > 0);
}

static void check_tdma_mg_shape(const mg_shape_t *s)
{
  ASSERT(s->row < 0x10000);
  ASSERT(s->col < 0x10000);

  ASSERT(s->row > 0x0);
  ASSERT(s->col > 0x0);
}

static void check_tdma_mg_bf16_shape(const mg_shape_t *s, fmt_t fmt)
{
  uint8_t fmt_type = (fmt == FMT_BF16 ? 2 : 1);
  ASSERT(s->row < 0x10000);
  ASSERT(s->col < 0x10000 / fmt_type);

  ASSERT(s->row > 0x0);
  ASSERT(s->col > 0x0);
}

static void check_tdma_tl(const tl_t *t)
{
  ASSERT(t);
  ASSERT(t->fmt == FMT_I8 || t->fmt == FMT_U8 || t->fmt == FMT_BF16);
  check_tdma_tl_shape(&t->shape);
}

static void check_tdma_tl_bf16(const tl_t *t)
{
  ASSERT(t);
  ASSERT(t->fmt == FMT_I8 || t->fmt == FMT_U8 || t->fmt == FMT_BF16);
  check_tdma_tl_bf16_shape(&t->shape, t->fmt);
}

static void check_tdma_tg(const tg_t *t)
{
  ASSERT(t);
  ASSERT(t->base_reg_index < TDMA_NUM_BASE_REGS);
  ASSERT(t->fmt == FMT_I8 || t->fmt == FMT_U8 || t->fmt == FMT_BF16);
  check_tdma_tg_shape(&t->shape);
}

static void check_tdma_tg_bf16(const tg_t *t)
{
  ASSERT(t);
  ASSERT(t->base_reg_index < TDMA_NUM_BASE_REGS);
  ASSERT(t->fmt == FMT_I8 || t->fmt == FMT_U8 || t->fmt == FMT_BF16);
  check_tdma_tg_bf16_shape(&t->shape, t->fmt);
}

static void check_tdma_compressed_tg(const compressed_tg_t *t)
{
  uint32_t stride_w = t->t.fmt == FMT_BF16 ? 2 : 1;

  ASSERT(t);
  ASSERT(t->t.base_reg_index < TDMA_NUM_BASE_REGS);
  check_tdma_tg_shape(&t->t.shape);
  ASSERT(!(t->t.start_address%0x10));

  // Enable after backend fix
  //ASSERT(t->t.stride.n ==
  //       (t->t.shape.w * t->t.shape.h * t->t.shape.c * stride_w));

  ASSERT(t->t.stride.c == (t->t.shape.w * t->t.shape.h * stride_w));
  ASSERT(t->t.stride.h == (t->t.shape.w * stride_w));
  // <! sram dont care stride
}

static void check_tdma_vlc_matrix_compressed_mg(const bmk1880v2_compressed_matrix_tgmem_t *t)
{
  ASSERT(t);
  ASSERT(t->m.base_reg_index < TDMA_NUM_BASE_REGS);
  ASSERT(!(t->m.start_address%0x10));

  // the data should be continuous
  if (t->m.fmt == FMT_BF16) {
    ASSERT(t->m.stride.row == t->m.shape.col * 2);
  }
  else if (t->m.fmt == FMT_I8 || t->m.fmt == FMT_U8) {
    ASSERT(t->m.stride.row == t->m.shape.col);
  }
  else {
    ASSERT(0); //<! not support shape
  }
}

static void check_tdma_ml(const ml_t *m)
{
  ASSERT(m);
  ASSERT(m->fmt == FMT_I8 || m->fmt == FMT_U8 || m->fmt == FMT_BF16);
  check_tdma_ml_shape(&m->shape);
}

static void check_tdma_ml_bf16(const ml_t *m)
{
  ASSERT(m);
  ASSERT(m->fmt == FMT_I8 || m->fmt == FMT_U8 || m->fmt == FMT_BF16);
  check_tdma_ml_bf16_shape(&m->shape, m->fmt);
}

static void check_tdma_mg(const mg_t *m)
{
  ASSERT(m);
  ASSERT(m->base_reg_index < TDMA_NUM_BASE_REGS);
  check_tdma_mg_shape(&m->shape);
}

static void check_tdma_mg_bf16(const mg_t *m)
{
  ASSERT(m);
  ASSERT(m->base_reg_index < TDMA_NUM_BASE_REGS);
  check_tdma_mg_bf16_shape(&m->shape, m->fmt);
}

static void check_tdma_compress_mg(const compressed_mg_t *m)
{
  ASSERT(m);
  ASSERT(m->m.base_reg_index < TDMA_NUM_BASE_REGS);
  check_tdma_mg_shape(&m->m.shape);
}

static void assert_tl_same_size(const tl_t *a, const tl_t *b)
{
  uint32_t a_size = a->shape.n * a->shape.c * a->shape.h * a->shape.w;
  uint32_t b_size = b->shape.n * b->shape.c * b->shape.h * b->shape.w;

  ASSERT(a_size == b_size);
}

static void assert_tl_tg_same_size(const tl_t *tl, const tg_t *tg)
{
  uint32_t tl_size = tl->shape.n * tl->shape.c * tl->shape.h * tl->shape.w;
  uint32_t tg_size = tg->shape.n * tg->shape.c * tg->shape.h * tg->shape.w;
  ASSERT(tl_size == tg_size);
}

static void assert_ml_mg_same_size(const ml_t *ml, const mg_t *mg)
{
  uint32_t ml_size = ml->shape.n * ml->shape.col;
  uint32_t mg_size = mg->shape.row * mg->shape.col;

  ASSERT(ml_size == mg_size);
}

static uint64_t absolute_gmem_addr(uint64_t addr)
{
  return (addr & 0x0FFFFFFFFFF) + BM1880V2_GLOBAL_MEM_START_ADDR;
}

static ec_desc_t * emit_tdma_cmdbuf(ctx_t *ctx, tdma_reg_t *reg)
{
  desc_pair_t *dp = bm1880v2_get_desc_pair(ctx, BMK1880v2_TDMA);

  reg->layer_ID = ctx->layer_id;
  //ASSERT(reg->rsv5 != 0x0);// "this is debug use, it's fine for skip";

  uint32_t *cmdbuf = (uint32_t *)dp->cmd_hdr->cmd;
  emit_tdma_reg(reg, cmdbuf);

  return dp->ec_desc;
}

static void fill_l2tg_fmt(tdma_reg_t *reg, fmt_t src_fmt, fmt_t dst_fmt)
{
  reg->dst_fmt = (dst_fmt == FMT_BF16) ? 2 : 1;
  reg->src_fmt = (src_fmt == FMT_BF16) ? 2 : 1;
  // check and decide bf16->int8 or bf16->uint8_t
  reg->int8_sign = (dst_fmt == FMT_I8 ? 1 : 0);// | (dst_fmt == FMT_U8 ? 1 : 0);
}

static void fill_tg2l_fmt(tdma_reg_t *reg, fmt_t src_fmt, fmt_t dst_fmt)
{
  reg->dst_fmt = (dst_fmt == FMT_BF16) ? 2 : 1;
  reg->src_fmt = (src_fmt == FMT_BF16) ? 2 : 1;
  // check and decide int8->bf16 or uint8_t->bf16
  reg->int8_sign = (src_fmt == FMT_I8 ? 1 : 0) ;//| (src_fmt == FMT_U8 ? 1 : 0);
}

static void fill_l2l_fmt(tdma_reg_t *reg, fmt_t src_fmt, fmt_t dst_fmt)
{
  reg->dst_fmt = (dst_fmt == FMT_BF16) ? 2 : 1;
  reg->src_fmt = (src_fmt == FMT_BF16) ? 2 : 1;
  // check and decide bf16->int8 or bf16->uint8_t or int8->bf16 or uint8_t->bf16
  reg->int8_sign = (dst_fmt == FMT_I8 ? 1 : 0) | (src_fmt == FMT_I8 ? 1 : 0);
}

static void fill_src_addr(tdma_reg_t *r, uint64_t addr)
{
  r->src_base_addr_low = (uint32_t)addr;
  r->src_base_addr_high = (addr >> 32);
}

static void fill_dst_addr(tdma_reg_t *r, uint64_t addr)
{
  r->dst_base_addr_low = (uint32_t)addr;
  r->dst_base_addr_high = (addr >> 32);
}

static void fill_src_c_stride(tdma_reg_t *r, uint32_t str)
{
  r->src_c_stride_low = (uint16_t)str;
  r->src_c_stride_high = (str >> 16);
}

static void fill_dst_c_stride(tdma_reg_t *r, uint32_t str)
{
  r->dst_c_stride_low = (uint16_t)str;
  r->dst_c_stride_high = (str >> 16);
}

static void set_int8_rnd_mode(tdma_reg_t *r, uint32_t int8_rnd_mode)
{
  if (int8_rnd_mode == 1) {
    // <! enable ONLY bf16->int8
    if (r->src_fmt == FMT_BF16_TYP && r->dst_fmt == FMT_FIX8B_TYP) {
      r->int8_rnd_mode = int8_rnd_mode;
    }
  }
}


/*
 * Direction: L2L
 */

bmk1880v2_op_t * bmk1880v2_tdma_l2l_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_copy_param_t *p)
{
  check_tdma_tl(p->src);
  check_tdma_tl(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 3;
  reg.spec_func = 0;
  reg.sys_dtype = 0;

  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  reg.outstanding_en = p->outstanding;

  return emit_tdma_cmdbuf(ctx, &reg);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2l_bf16_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_copy_param_t *p)
{
  check_tdma_tl_bf16(p->src);
  check_tdma_tl_bf16(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 3;
  reg.spec_func = 0;
  reg.sys_dtype = 0;
  fill_l2l_fmt(&reg, p->src->fmt, p->dst->fmt);
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  // does not allow open `mv_lut_idx and `mv_lut_basemv_lut_base at same time
  if (p->mv_lut_idx == 1) {
    reg.mv_lut_idx = p->mv_lut_idx;
  }

  if (p->mv_lut_base == 1) {
    reg.mv_lut_base = p->mv_lut_base;
  }

  if (reg.mv_lut_idx == 1 && reg.mv_lut_base == 1) {
    ASSERT(0);
  }

  set_int8_rnd_mode(&reg, p->dst->int8_rnd_mode);

  reg.outstanding_en = p->outstanding;

  //trace_tdma_reg(&reg, __func__);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static uint32_t addr_after_right_shift(
    ctx_t *ctx, int addr, uint32_t step, int c_str)
{
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t lmem_size = ctx->chip_info.lmem_size;;

  uint32_t lmem_i = (addr / lmem_size + step) % npu_num;
  uint32_t offset = addr % lmem_size + (addr / lmem_size + step) / npu_num * c_str;
  return lmem_i * lmem_size + offset;
}

bmk1880v2_op_t * bmk1880v2_tdma_l2l_tensor_lrn_shift(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_lrn_shift_param_t *p)
{
  check_tdma_tl(p->src);
  check_tdma_tl(p->dst);
  assert_tl_same_size(p->src, p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(p->src->shape.c == p->dst->shape.c);
  ASSERT(p->src->shape.c > p->lrn_step);
  ASSERT(p->src->shape.h * p->src->shape.w ==
         p->dst->shape.h * p->dst->shape.w);
  ASSERT(p->lrn_step < 16);

  ASSERT(p->src->fmt == p->dst->fmt);

  int is_bf16 = (p->src->fmt == FMT_BF16) ? 1 : 0;
  if (is_bf16) {
    check_tdma_tl_bf16(p->src);
    check_tdma_tl_bf16(p->dst);
  }

  /* L2L lrn copy */
  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 3;
  reg.spec_func = 0;
  reg.sys_dtype = 0;
  fill_l2l_fmt(&reg, p->src->fmt, p->dst->fmt);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c - p->lrn_step;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c - p->lrn_step;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  if (p->right_shift) {
    uint32_t dst_addr = addr_after_right_shift(
        ctx, p->dst->start_address, p->lrn_step, p->dst->stride.c);

    fill_src_addr(&reg, p->src->start_address);
    fill_dst_addr(&reg, dst_addr);
  } else {
    uint32_t src_addr = addr_after_right_shift(
        ctx, p->src->start_address, p->lrn_step, p->src->stride.c);

    fill_src_addr(&reg, src_addr);
    fill_dst_addr(&reg, p->dst->start_address);
  }

  if (is_bf16)
    set_int8_rnd_mode(&reg, p->dst->int8_rnd_mode);

  emit_tdma_cmdbuf(ctx, &reg);

  /* Constant fill with zero */
  reg.trans_dir = 0;
  reg.spec_func = 4;
  reg.const_val = is_bf16 ? convert_fp32_bf16(0.0):  0;

  reg.dst_c = p->lrn_step;
  if (p->right_shift) {
    uint32_t dst_addr = addr_after_right_shift(
        ctx, p->dst->start_address, p->lrn_step, p->dst->stride.c);

    uint32_t lmem_size = ctx->chip_info.lmem_size;;
    uint32_t npu_num = ctx->chip_info.npu_num;
    uint32_t sht_num = p->lrn_step;

    uint32_t lmem_i = (dst_addr / lmem_size - sht_num) % npu_num;
    uint32_t offset = (lmem_i + sht_num) / npu_num * p->dst->stride.c;
    uint32_t zero_addr = lmem_i * lmem_size + dst_addr % lmem_size - offset;

    // printf("  lmem_i 0x%x, offset 0x%x, zero_addr 0x%x\n",
    //        lmem_i, offset, zero_addr);

    fill_dst_addr(&reg, zero_addr);

  } else {
    uint32_t start_mem = p->dst->start_address / ctx->chip_info.lmem_size;
    uint32_t cur_mem = (start_mem + (p->dst->shape.c - p->lrn_step)) % ctx->chip_info.npu_num;
    uint32_t offset =
        (p->dst->start_address % ctx->chip_info.lmem_size) +
        ((start_mem + (p->dst->shape.c - p->lrn_step)) / ctx->chip_info.npu_num) * p->dst->stride.c;
    uint32_t zero_addr = cur_mem * ctx->chip_info.lmem_size + offset;

    // printf("  start_mem 0x%x, cur_mem 0x%x, offset 0x%x, zero_addr 0x%x\n",
    //        start_mem, cur_mem, offset, zero_addr);

    fill_dst_addr(&reg, zero_addr);
  }

  return emit_tdma_cmdbuf(ctx, &reg);
}

/*
 * Direction: L2TG
 */

static bmk1880v2_op_t * tdma_l2tg_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl(p->src);
  check_tdma_tg(p->dst);
  assert_tl_tg_same_size(p->src, p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}


static bmk1880v2_op_t * tdma_l2tg_bf16_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl_bf16(p->src);
  check_tdma_tg_bf16(p->dst);
  assert_tl_tg_same_size(p->src, p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(!(p->src->fmt == FMT_I8 && p->dst->fmt == FMT_BF16)); // not support tl(int8)->tg(bf16)

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  fill_l2tg_fmt(&reg, p->src->fmt, p->dst->fmt);
  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  set_int8_rnd_mode(&reg, p->dst->int8_rnd_mode);
  //trace_tdma_reg(&reg, __func__);
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl(p->src);
  check_tdma_tg(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.c);
  ASSERT(p->dst->shape.c == p->src->shape.n);
  ASSERT(p->dst->shape.h * p->dst->shape.w ==
         p->src->shape.h * p->src->shape.w);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 1;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_bf16_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl_bf16(p->src);
  check_tdma_tg_bf16(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.c);
  ASSERT(p->dst->shape.c == p->src->shape.n);
  ASSERT(p->dst->shape.h * p->dst->shape.w ==
         p->src->shape.h * p->src->shape.w);
  ASSERT(!(p->src->fmt == FMT_I8 && p->dst->fmt == FMT_BF16)); // not support tl(int8)->tg(bf16)

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 1;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);
  fill_l2tg_fmt(&reg, p->src->fmt, p->dst->fmt);
  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;
  set_int8_rnd_mode(&reg, p->dst->int8_rnd_mode);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_tensor_copy_cw_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl(p->src);
  check_tdma_tg(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(p->src->shape.c == p->dst->shape.w);
  ASSERT(p->src->shape.h == p->dst->shape.h);
  ASSERT(p->src->shape.w == p->dst->shape.c);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.spec_func = 1;
  reg.transpose_md = 3;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_bf16_tensor_copy_cw_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl_bf16(p->src);
  check_tdma_tg_bf16(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(p->src->shape.c == p->dst->shape.w);
  ASSERT(p->src->shape.h == p->dst->shape.h);
  ASSERT(p->src->shape.w == p->dst->shape.c);

  /*not support bf16 mode*/
  ASSERT(!(p->src->fmt == FMT_BF16 || p->dst->fmt == FMT_BF16));

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.spec_func = 1;
  reg.transpose_md = 3;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  fill_l2tg_fmt(&reg, p->src->fmt, p->dst->fmt);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_tensor_copy_compressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_compressed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tl(p->src);
  check_tdma_compressed_tg(p->dst);
  assert_tl_tg_same_size(p->src, &p->dst->t);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  //<! only support int8/uint8/bf16
  ASSERT(p->src->fmt == FMT_BF16 || p->src->fmt == FMT_I8 || p->src->fmt == FMT_U8);

  ASSERT(p->dst->bias1 == 0);
  if (p->src->fmt == FMT_BF16) {
    ASSERT(p->dst->bias0 == 127);
  }
  else {
    //p->src->fmt == FMT_I8 || p->src->fmt == FMT_U8);
    ASSERT(p->dst->bias0 == 0);
    ASSERT(p->dst->zero_guard_en == 0);
  }

  reg.src_fmt = (p->src->fmt == FMT_BF16) ? FMT_BF16_TYP : FMT_FIX8B_TYP;
  reg.dst_fmt = reg.src_fmt;

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.compress_en = 1;

  // VLC constraint under hw compress
  //1. in int8/uint8, bias0/bias should be 0/0
  //2. in bf16, signed should be 0 and bias0 set to 127, bias1 set to 0
  reg.cmprs_fmt = (p->src->fmt == FMT_I8);

  // NOTICE: it recommand set to 1 once data contain '0' under bf16
  reg.compress_zero_guard = p->dst->zero_guard_en ? 1 : 0;
  reg.compress_bias0 = p->dst->bias0;
  reg.compress_bias1 = p->dst->bias1;

  reg.dst_base_reg_sel = p->dst->t.base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->t.shape.c;
  reg.dst_h = p->dst->t.shape.h;
  reg.dst_w = p->dst->t.shape.w;
  reg.dst_n_stride = p->dst->t.stride.n;
  fill_dst_c_stride(&reg, p->dst->t.stride.c);
  reg.dst_h_stride = p->dst->t.stride.h;
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_fill_constant_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_tg_bf16(p->dst);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.spec_func = 4;
  reg.const_val = p->constant;

  // only support tl(bf16)->tg(bf16) or tl(fix8b)->tg(fix8b)
  fill_l2tg_fmt(&reg, p->dst->fmt, p->dst->fmt);

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->dst->shape.n;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_ml(p->src);
  check_tdma_mg(p->dst);
  assert_ml_mg_same_size(p->src, p->dst);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 1;
  reg.spec_func = 0;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = 1;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.row;
  reg.dst_w = p->dst->shape.col;
  fill_dst_c_stride(&reg, p->dst->stride.row);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_matrix_copy_compressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_compressed_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_ml(p->src);
  check_tdma_compress_mg(p->dst);
  check_tdma_vlc_matrix_compressed_mg(p->dst);
  assert_ml_mg_same_size(p->src, &p->dst->m);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.compress_en = 1;
  reg.sys_dtype = 1;
  reg.spec_func = 0;

  // vlc setting
  reg.cmprs_fmt = (p->src->fmt == FMT_I8);

  ASSERT(p->dst->bias1 == 0);
  if (p->src->fmt == FMT_BF16) {
    ASSERT(p->dst->bias0 == 127);
  }
  else {
    //p->src->fmt == FMT_I8 || p->src->fmt == FMT_U8);
    ASSERT(p->dst->bias0 == 0);
    ASSERT(p->dst->zero_guard_en == 0);
  }

  // NOTICE: it should be 1 once data contain '0' under bf16
  reg.compress_zero_guard = p->dst->zero_guard_en ? 1 : 0;
  reg.compress_bias0 = p->dst->bias0;
  reg.compress_bias1 = p->dst->bias1;

  reg.dst_base_reg_sel = p->dst->m.base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  fill_l2tg_fmt(&reg, p->src->fmt, p->dst->m.fmt);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = 1;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->m.shape.row;
  reg.dst_w = p->dst->m.shape.col;
  fill_dst_c_stride(&reg, p->dst->m.stride.row);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_bf16_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p,
    uint64_t dst_addr)
{
  check_tdma_ml_bf16(p->src);
  check_tdma_mg_bf16(p->dst);
  assert_ml_mg_same_size(p->src, p->dst);
  ASSERT(!((p->src->fmt == FMT_I8 || p->src->fmt == FMT_U8) && p->dst->fmt == FMT_BF16)); // not support tl(i8/uint8_t)->tg(bf16)
  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.sys_dtype = 1;
  reg.spec_func = 0;

  reg.dst_base_reg_sel = p->dst->base_reg_index;
  fill_src_addr(&reg, p->src->start_address);
  fill_dst_addr(&reg, dst_addr);

  fill_l2tg_fmt(&reg, p->src->fmt, p->dst->fmt);
  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = 1;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.row;
  reg.dst_w = p->dst->shape.col;
  fill_dst_c_stride(&reg, p->dst->stride.row);
  set_int8_rnd_mode(&reg, p->dst->int8_rnd_mode);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_general_copy_param_t *p,
    uint64_t dst_addr)
{
  ASSERT(p->dst_base_reg_index < TDMA_NUM_BASE_REGS);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.trans_fmt = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  reg.dst_base_reg_sel = p->dst_base_reg_index;
  fill_src_addr(&reg, p->src_address);
  fill_dst_addr(&reg, dst_addr);
  reg.src_n_stride = p->bytes;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_l2tg_bf16_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_bf16_general_copy_param_t *p,
    uint64_t dst_addr)
{
  ASSERT(p->dst_base_reg_index < TDMA_NUM_BASE_REGS);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);
  // only support fix8b->fix8b or bf16->bf16
  ASSERT(p->src_fmt == p->dst_fmt);

  reg.vld = 1;
  reg.trans_dir = 1;
  reg.trans_fmt = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;
  fill_l2tg_fmt(&reg, p->src_fmt, p->dst_fmt);

  reg.dst_base_reg_sel = p->dst_base_reg_index;
  fill_src_addr(&reg, p->src_address);
  fill_dst_addr(&reg, dst_addr);
  reg.src_n_stride = p->src_bytes;
  return emit_tdma_cmdbuf(ctx, &reg);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_tensor_copy(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_bf16_tensor_copy(ctx, p, dst_addr);
}
bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_tensor_copy_nc_transposed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_bf16_tensor_copy_nc_transposed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_cw_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_tensor_copy_cw_transposed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy_cw_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_bf16_tensor_copy_cw_transposed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_compressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_compressed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->t.start_address);
  return tdma_l2tg_tensor_copy_compressed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_fill_constant_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_tensor_fill_constant(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_matrix_copy(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->start_address);
  return tdma_l2tg_bf16_matrix_copy(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_matrix_copy_compressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_compressed_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst->m.start_address);
  return tdma_l2tg_matrix_copy_compressed(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_general_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst_address);
  return tdma_l2tg_general_copy(ctx, p, dst_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_l2tg_bf16_general_copy_param_t *p)
{
  uint64_t dst_addr = absolute_gmem_addr(p->dst_address);
  return tdma_l2tg_bf16_general_copy(ctx, p, dst_addr);
}
/*
 * Direction: TG2L
 */

static bmk1880v2_op_t * tdma_tg2l_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p,
    uint64_t src_addr)
{
  check_tdma_tg(p->src);
  check_tdma_tl(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  assert_tl_tg_same_size(p->dst, p->src);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_bf16_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p,
    uint64_t src_addr)
{
  check_tdma_tg_bf16(p->src);
  check_tdma_tl_bf16(p->dst);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(!(p->src->fmt == FMT_BF16 && p->dst->fmt == FMT_I8)); // not support tg(bf16)->tl(int8)
  assert_tl_tg_same_size(p->dst, p->src);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  reg.src_base_reg_sel = p->src->base_reg_index;

  fill_tg2l_fmt(&reg, p->src->fmt, p->dst->fmt);
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  //trace_tdma_reg(&reg, __func__);
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p,
    uint64_t src_addr)
{
  check_tdma_tg(p->src);
  check_tdma_tl(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.c);
  ASSERT(p->dst->shape.c == p->src->shape.n);
  ASSERT(p->dst->shape.h * p->dst->shape.w ==
         p->src->shape.h * p->src->shape.w);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 0;
  reg.spec_func = 1;

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_bf16_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p,
    uint64_t src_addr)
{
  check_tdma_tg_bf16(p->src);
  check_tdma_tl_bf16(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.c);
  ASSERT(p->dst->shape.c == p->src->shape.n);
  ASSERT(p->dst->shape.h * p->dst->shape.w ==
         p->src->shape.h * p->src->shape.w);

  ASSERT(!(p->src->fmt == FMT_BF16 && p->dst->fmt == FMT_I8)); // not support tg(bf16)->tl(int8)

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 0;
  reg.spec_func = 1;

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);
  fill_tg2l_fmt(&reg, p->src->fmt, p->dst->fmt);
  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_tensor_copy_chw_rotated(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_chw_rotated_param_t *p,
    uint64_t src_addr)
{
  check_tdma_tg(p->src);
  check_tdma_tl(p->dst);

  ASSERT(p->src->shape.c == 3 || p->src->shape.c == 4);
  ASSERT(p->src->shape.n == p->dst->shape.n);
  ASSERT(p->src->shape.c == p->dst->shape.c);
  ASSERT(p->src->shape.h == p->dst->shape.h);
  ASSERT(p->src->shape.w == p->dst->shape.w);

  ASSERT(p->dst->start_address % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.n % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.c % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.h == p->dst->shape.w);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 0;
  reg.spec_func = 1;

  if (p->dst->shape.c == 3)
    reg.transpose_md = 1;
  else if(p->dst->shape.c == 4)
    reg.transpose_md = 2;
  else
    ASSERT(0);

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;
  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, 1);
  reg.src_h_stride = p->src->shape.c * p->src->shape.w;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_tensor_copy_decompressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_decompressed_param_t *p,
    uint64_t src_addr)
{
  check_tdma_compressed_tg(p->src);
  check_tdma_tl(p->dst);
  assert_tl_tg_same_size(p->dst, &p->src->t);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  //<! only support int8/uint8/bf16
  ASSERT(p->dst->fmt == FMT_BF16 || p->dst->fmt == FMT_I8 || p->dst->fmt == FMT_U8);
  fill_tg2l_fmt(&reg, p->src->t.fmt, p->dst->fmt);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.compress_en = 1;
  reg.cmprs_fmt = (p->src->t.fmt == FMT_I8);

  reg.src_base_reg_sel = p->src->t.base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->t.shape.n;
  reg.src_c = p->src->t.shape.c;
  reg.src_h = p->src->t.shape.h;
  reg.src_w = p->src->t.shape.w;
  reg.src_n_stride = p->src->t.stride.n;
  fill_src_c_stride(&reg, p->src->t.stride.c);
  reg.src_h_stride = p->src->t.stride.h;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  // trace_tdma_reg(&reg, __FUNCTION__);

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p)
{
  check_tdma_tl(p->dst);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.spec_func = 4;
  reg.const_val = p->constant;

  reg.dst_fmt = (p->dst->fmt == FMT_BF16) ? 2 : 1;

  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->dst->shape.n;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_bf16_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p)
{
  check_tdma_tl_bf16(p->dst);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.spec_func = 4;
  reg.const_val = p->constant;

  /*only suppoert fix8b->fix8b or bf16->bf16*/
  fill_tg2l_fmt(&reg, p->dst->fmt, p->dst->fmt);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->dst->shape.n;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p,
    uint64_t src_addr)
{
  check_tdma_mg(p->src);
  check_tdma_ml(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.row);
  assert_ml_mg_same_size(p->dst, p->src);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 1;
  reg.spec_func = 0;

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.row;
  reg.src_c = p->src->shape.row;
  reg.src_w = p->src->shape.col;
  fill_src_c_stride(&reg, p->src->stride.row);

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = 1;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_matrix_copy_decompressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_decompressed_param_t *p,
    uint64_t src_addr)
{
  check_tdma_vlc_matrix_compressed_mg(p->src);
  check_tdma_mg(&p->src->m);
  check_tdma_ml(p->dst);
  ASSERT(p->dst->shape.n == p->src->m.shape.row);
  assert_ml_mg_same_size(p->dst, &p->src->m);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 1;
  reg.spec_func = 0;
  reg.compress_en = 1;
  reg.cmprs_fmt = (p->src->m.fmt == FMT_I8);

  fill_tg2l_fmt(&reg, p->src->m.fmt, p->dst->fmt);
  reg.src_base_reg_sel = p->src->m.base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->m.shape.row;
  reg.src_c = p->src->m.shape.row;
  reg.src_w = p->src->m.shape.col;
  fill_src_c_stride(&reg, p->src->m.stride.row);

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = 1;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_bf16_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p,
    uint64_t src_addr)
{
  check_tdma_mg_bf16(p->src);
  check_tdma_ml_bf16(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.row);
  assert_ml_mg_same_size(p->dst, p->src);
  ASSERT(!(p->src->fmt == FMT_BF16 && p->dst->fmt == FMT_I8)); // not support tg(bf16)->tl(int8)
  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 1;
  reg.spec_func = 0;

  fill_tg2l_fmt(&reg, p->src->fmt, p->dst->fmt);
  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.row;
  reg.src_c = p->src->shape.row;
  reg.src_w = p->src->shape.col;
  fill_src_c_stride(&reg, p->src->stride.row);

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = 1;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;
  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_matrix_copy_row_col_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t *p,
    uint64_t src_addr)
{
  check_tdma_mg(p->src);
  check_tdma_ml(p->dst);
  ASSERT(p->dst->shape.n == p->src->shape.col);
  ASSERT(p->dst->shape.col == p->src->shape.row);
  assert_ml_mg_same_size(p->dst, p->src);

  ASSERT(p->src->shape.row >= p->dst->shape.w);
  ASSERT(p->dst->shape.c ==
      (uint32_t) ceiling_func(p->src->shape.row, p->dst->shape.w));

  ASSERT(p->dst->start_address % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.n % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.c % ctx->chip_info.eu_num == 0);
  ASSERT(p->dst->stride.h == p->dst->shape.w);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.sys_dtype = 1;
  reg.spec_func = 1;

  reg.src_base_reg_sel = p->src->base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst->start_address);

  reg.src_n = p->src->shape.row;
  reg.src_c = p->src->shape.row;
  reg.src_w = p->src->shape.col;
  fill_src_c_stride(&reg, p->src->stride.row);

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = 1;
  reg.dst_w = p->dst->shape.w;
  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p->dst->stride.h;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_general_copy_param_t *p,
    uint64_t src_addr)
{
  ASSERT(p->src_base_reg_index < TDMA_NUM_BASE_REGS);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.trans_fmt = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  reg.src_base_reg_sel = p->src_base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst_address);
  reg.src_n_stride = p->bytes;

  return emit_tdma_cmdbuf(ctx, &reg);
}

static bmk1880v2_op_t * tdma_tg2l_bf16_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_bf16_general_copy_param_t *p,
    uint64_t src_addr)
{
  ASSERT(p->src_base_reg_index < TDMA_NUM_BASE_REGS);
  // only support fix8b->fix8b or bf16->bf16
  ASSERT(p->dst_fmt == p->src_fmt);

  tdma_reg_t reg;
  reset_tdma_reg(&reg);

  reg.vld = 1;
  reg.trans_dir = 0;
  reg.trans_fmt = 1;
  reg.sys_dtype = 0;
  reg.spec_func = 0;

  fill_tg2l_fmt(&reg, p->src_fmt, p->dst_fmt);

  reg.src_base_reg_sel = p->src_base_reg_index;
  fill_src_addr(&reg, src_addr);
  fill_dst_addr(&reg, p->dst_address);
  reg.src_n_stride = p->src_bytes;

  return emit_tdma_cmdbuf(ctx, &reg);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_tensor_copy(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_tensor_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_bf16_tensor_copy(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_tensor_copy_nc_transposed(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_tensor_copy_nc_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_bf16_tensor_copy_nc_transposed(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_chw_rotated(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_chw_rotated_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_tensor_copy_chw_rotated(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_decompressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_decompressed_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->t.start_address);
  return tdma_tg2l_tensor_copy_decompressed(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_tg2l_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p)
{
  return tdma_tg2l_tensor_fill_constant(ctx, p);
}

bmk1880v2_op_t * bmk1880v2_tdma_tg2l_bf16_tensor_fill_constant(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p)
{
  return tdma_tg2l_bf16_tensor_fill_constant(ctx, p);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_matrix_copy(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy_decompressed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_decompressed_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->m.start_address);
  return tdma_tg2l_matrix_copy_decompressed(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_matrix_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_bf16_matrix_copy(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy_row_col_transposed(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src->start_address);
  return tdma_tg2l_matrix_copy_row_col_transposed(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_general_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src_address);
  return tdma_tg2l_general_copy(ctx, p, src_addr);
}

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_general_copy(
    ctx_t *ctx,
    const bmk1880v2_tdma_tg2l_bf16_general_copy_param_t *p)
{
  uint64_t src_addr = absolute_gmem_addr(p->src_address);
  return tdma_tg2l_bf16_general_copy(ctx, p, src_addr);
}

/*
 * Direction: TG2TG
 */

static bmk1880v2_op_t * bmk1880v2_gdma_copy_gmem(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p,
    uint8_t u8_trans_fmt)
{
  tdma_reg_t reg;

  reset_tdma_reg(&reg);

  uint64_t u64_src_addr;
  uint64_t u64_dst_addr;

  reg.vld = 1;
  reg.trans_dir = 2; // 0:tg2l, 1:l2tg, 2:g2g, 3:l2l
  reg.trans_fmt = u8_trans_fmt; // 1:general copy, 2:tensor copy
  reg.sys_dtype = 0; //
  reg.spec_func = 0; //

  u64_src_addr = absolute_gmem_addr(p->src->start_address);
  u64_dst_addr = absolute_gmem_addr(p->dst->start_address);
  fill_src_addr(&reg, u64_src_addr);
  fill_dst_addr(&reg, u64_dst_addr);

  reg.src_base_reg_sel = p->src->base_reg_index;
  reg.dst_base_reg_sel = p->dst->base_reg_index;

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;

  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p-> dst->stride.h;

  return emit_tdma_cmdbuf( ctx, &reg);
}

static bmk1880v2_op_t * bmk1880v2_gdma_bf16_copy_gmem(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p,
    uint8_t u8_trans_fmt)
{
  tdma_reg_t reg;

  reset_tdma_reg(&reg);

  uint64_t u64_src_addr;
  uint64_t u64_dst_addr;

  reg.vld = 1;
  reg.trans_dir = 2; // 0:tg2l, 1:l2tg, 2:g2g, 3:l2l
  reg.trans_fmt = u8_trans_fmt; // 1:general copy, 2:tensor copy
  reg.sys_dtype = 0; //
  reg.spec_func = 0; //
  ASSERT(p->src->fmt == p->dst->fmt);

  reg.dst_fmt = (p->dst->fmt == FMT_BF16) ? 2 : 1;
  reg.src_fmt = (p->src->fmt == FMT_BF16) ? 2 : 1;

  u64_src_addr = absolute_gmem_addr(p->src->start_address);
  u64_dst_addr = absolute_gmem_addr(p->dst->start_address);
  fill_src_addr(&reg, u64_src_addr);
  fill_dst_addr(&reg, u64_dst_addr);

  reg.src_base_reg_sel = p->src->base_reg_index;
  reg.dst_base_reg_sel = p->dst->base_reg_index;

  reg.src_n = p->src->shape.n;
  reg.src_c = p->src->shape.c;
  reg.src_h = p->src->shape.h;
  reg.src_w = p->src->shape.w;

  reg.dst_c = p->dst->shape.c;
  reg.dst_h = p->dst->shape.h;
  reg.dst_w = p->dst->shape.w;

  reg.src_n_stride = p->src->stride.n;
  fill_src_c_stride(&reg, p->src->stride.c);
  reg.src_h_stride = p->src->stride.h;

  reg.dst_n_stride = p->dst->stride.n;
  fill_dst_c_stride(&reg, p->dst->stride.c);
  reg.dst_h_stride = p-> dst->stride.h;

  return emit_tdma_cmdbuf( ctx, &reg);
}

/*
 * Direction: G2G
 */
bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p)
{
  bmk1880v2_gdma_copy_gmem(ctx, p, 2);
  return NULL;
}

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_bf16_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p)
{
  bmk1880v2_gdma_bf16_copy_gmem(ctx, p, 2);
  return NULL;
}

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p)
{
  bmk1880v2_gdma_copy_gmem(ctx, p, 1);
  return NULL;
}

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_bf16_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p)
{
  bmk1880v2_gdma_bf16_copy_gmem(ctx, p, 1);
  return NULL;
}
