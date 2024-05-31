#include "kernel_internal.h"
#include <assert.h>
#include "cvikernel/cvikernel.h"
#include "../bm1822/kernel_1822.h"
#include "bmkernel/bm1822/1822_fp_convert.h"

#define SET_TG_SHAPE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).h = (_src).h; \
  (_dst).w = (_src).w; \
} while(0)

#define SET_TG_STRIDE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).h = (_src).h; \
} while(0)

#define SET_TL_SHAPE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).h = (_src).h; \
  (_dst).w = (_src).w; \
} while(0)

#define SET_TL_STRIDE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).h = (_src).h; \
  (_dst).w = (_src).w; \
} while(0)

#define SET_ML_SHAPE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).w = (_src).w; \
  (_dst).col = (_src).col; \
} while(0)

#define SET_ML_STRIDE(_dst, _src) \
do { \
  (_dst).n = (_src).n; \
  (_dst).c = (_src).c; \
  (_dst).h = (_src).h; \
} while(0)

typedef struct cvk_prv_data {
  bmk1822_context_t *bmk_ctx;
  uint32_t cmdbuf_size;
  uint8_t *cmdbuf;
} cvk_prv_data_t;

extern uint32_t bmk1822_estimate_nr_desc(bmk_context_t *k);

static void convert_lmem_tensor(
    bmk1822_tensor_lmem_t *dst,
    const cvk_tl_t *src)
{
  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  dst->cmprs_fmt = src->cmprs_fmt;
  SET_TL_SHAPE(dst->shape, src->shape);
  SET_TL_STRIDE(dst->stride, src->stride);
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static void convert_gmem_tensor(
    bmk1822_tensor_tgmem_t *dst,
    const cvk_tg_t *src)
{
  dst->base_reg_index = src->base_reg_index;
  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  SET_TG_SHAPE(dst->shape, src->shape);
  SET_TG_STRIDE(dst->stride, src->stride);
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static void convert_gmem_compressed_tensor(
    bmk1822_compressed_tensor_tgmem_t *dst,
    const cvk_cmpr_tg_t *src)
{
  dst->t.base_reg_index = src->t.base_reg_index;
  dst->t.start_address = src->t.start_address;
  dst->t.fmt = src->t.fmt;
  SET_TG_SHAPE(dst->t.shape, src->t.shape);
  SET_TG_STRIDE(dst->t.stride, src->t.stride);
  dst->t.int8_rnd_mode = src->t.int8_rnd_mode;
  dst->reserved_size = src->reserved_size;
  dst->bit_length = src->bit_length;
  dst->bias0 = src->bias0;
  dst->bias1 = src->bias1;
  dst->zero_guard_en = src->zero_guard_en;
}

static void convert_lmem_matrix(
    bmk1822_matrix_lmem_t *dst,
    const cvk_ml_t *src)
{
  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  SET_ML_SHAPE(dst->shape, src->shape);
  SET_ML_STRIDE(dst->stride, src->stride);
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static void convert_gmem_matrix(
    bmk1822_matrix_tgmem_t *dst,
    const cvk_mg_t *src)
{
  dst->base_reg_index = src->base_reg_index;
  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  dst->shape.row = src->shape.row;
  dst->shape.col = src->shape.col;
  dst->stride.row = src->stride.row;
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static void convert_gmem_compressed_matrix(
    bmk1822_compressed_matrix_tgmem_t *dst,
    const cvk_cmpr_mg_t *src)
{
  dst->m.base_reg_index = src->m.base_reg_index;
  dst->m.start_address = src->m.start_address;
  dst->m.fmt = src->m.fmt;
  dst->m.shape.row = src->m.shape.row;
  dst->m.shape.col = src->m.shape.col;
  dst->m.stride.row = src->m.stride.row;
  dst->m.int8_rnd_mode = src->m.int8_rnd_mode;
  dst->bias0 = src->bias0;
  dst->bias1 = src->bias1;
  dst->zero_guard_en = src->zero_guard_en;
}

void cvk1822_cleanup(struct cvikernel_context *ctx)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_cleanup(bmk_ctx);

  // command buffer is freed in cviruntime_cvikernel_destroy().
  //free(((cvk_prv_data_t *)ctx->priv_data)->cmdbuf);

  free(ctx->priv_data);
  ctx->priv_data = NULL;
}

void cvk1822_reset(struct cvikernel_context *ctx)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_reset(bmk_ctx);
}

uint8_t *cvk1822_acquire_cmdbuf(
    struct cvikernel_context *ctx,
    uint32_t *size)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  return bmk1822_acquire_cmdbuf(bmk_ctx, size);
}

void cvk1822_set_layer_id(
    struct cvikernel_context *ctx,
    uint16_t layer_id)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_set_layer_id(bmk_ctx, layer_id);
}

void cvk1822_parallel_enable(struct cvikernel_context *ctx)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_parallel_enable(bmk_ctx);
}

void cvk1822_parallel_disable(struct cvikernel_context *ctx)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_parallel_disable(bmk_ctx);
}

cvk_tl_t *cvk1822_lmem_alloc_tensor(
    cvk_context_t *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tensor_lmem_shape_t bmk_shape;
  SET_TL_SHAPE(bmk_shape, shape);

  fmt_t bmk_fmt = fmt;

  bmk1822_tensor_lmem_t *bmk_tl =
      bmk1822_lmem_alloc_tensor(bmk_ctx, bmk_shape, bmk_fmt, eu_align);

  return (cvk_tl_t *)bmk_tl;
}

cvk_ml_t *cvk1822_lmem_alloc_matrix(
    cvk_context_t *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  bmk1822_matrix_lmem_t *bmk_ml =
      bmk1822_lmem_alloc_matrix(bmk_ctx, bmk_shape, fmt, eu_align);

  return (cvk_ml_t *)bmk_ml;
}

cvk_ml_t *cvk1822_lmem_alloc_ps32_matrix(
    cvk_context_t *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  bmk1822_matrix_lmem_t *bmk_ml =
      bmk1822_lmem_alloc_ps32_matrix(bmk_ctx, bmk_shape, fmt, eu_align);

  return (cvk_ml_t *)bmk_ml;
}

void cvk1822_lmem_free_tensor(
    struct cvikernel_context *ctx,
    const cvk_tl_t *tl)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_lmem_free_tensor(bmk_ctx, (bmk1822_tensor_lmem_t *)tl);
}

void cvk1822_lmem_free_matrix(
    struct cvikernel_context *ctx,
    const cvk_ml_t *ml)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_lmem_free_matrix(bmk_ctx, (bmk1822_matrix_lmem_t *)ml);
}

void cvk1822_lmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tl_t *tl,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  // memset(tl, 0, sizeof(*tl));
  // tl->shape = shape;
  // tl->eu_align = eu_align;
  // tl->stride = cvk1822_tl_default_stride(ctx, shape, fmt, eu_align);
  bmk1822_tensor_lmem_shape_t bmk_shape;
  SET_TL_SHAPE(bmk_shape, shape);

  fmt_t bmk_fmt = fmt;

  bmk1822_lmem_init_tensor(bmk_ctx, (bmk1822_tensor_lmem_t *)tl, bmk_shape,
                           bmk_fmt, eu_align);
}

void cvk1822_lmem_init_matrix(
    struct cvikernel_context *ctx,
    cvk_ml_t *ml,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  bmk1822_lmem_init_matrix(bmk_ctx, (bmk1822_matrix_lmem_t *)ml, bmk_shape, fmt,
                           eu_align);
}

cvk_tl_stride_t cvk1822_tl_default_stride(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tensor_lmem_shape_t bmk_shape;
  SET_TL_SHAPE(bmk_shape, shape);

  bmk1822_tensor_lmem_stride_t bmk_stride =
      bmk1822_tensor_lmem_default_stride(bmk_ctx, bmk_shape, fmt,
                                         eu_align);

  cvk_tl_stride_t stride;
  SET_TL_STRIDE(stride, bmk_stride);

  return stride;
}

cvk_tg_stride_t cvk1822_tg_default_stride(
    struct cvikernel_context *ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt)
{
  (void)ctx;

  bmk1822_tensor_tgmem_shape_t bmk_shape;
  SET_TG_SHAPE(bmk_shape, shape);

  bmk1822_tensor_tgmem_stride_t bmk_stride =
      bmk1822_tensor_tgmem_default_stride(bmk_shape, fmt);

  cvk_tg_stride_t stride;
  SET_TG_STRIDE(stride, bmk_stride);
  stride.w = (fmt == CVK_FMT_BF16) ? 2 : 1;

  return stride;
}

cvk_ml_shape_t cvk1822_ml_default_shape(
    struct cvikernel_context *ctx,
    uint32_t row,
    uint32_t col,
    cvk_fmt_t fmt_type) {
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bm_ml_shape =
      bmk1822_matrix_lmem_default_shape(bmk_ctx, row, col, fmt_type);

  cvk_ml_shape_t ml_shape;
  SET_ML_SHAPE(ml_shape, bm_ml_shape);

  return ml_shape;
}

cvk_ml_stride_t cvk1822_ml_default_stride(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align) {
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  bmk1822_matrix_lmem_stride_t bmk_stride =
      bmk1822_matrix_lmem_default_stride(bmk_ctx, bmk_shape, fmt, eu_align);

  cvk_ml_stride_t stride;
  SET_ML_STRIDE(stride, bmk_stride);

  return stride;
}

cvk_ml_shape_t cvk1822_ml_shape_t1(
    struct cvikernel_context *ctx,
    uint32_t len,
    cvk_fmt_t fmt_type)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape =
      bmk1822_matrix_lmem_shape_t1(bmk_ctx, len, fmt_type);

  cvk_ml_shape_t shape;
  SET_ML_SHAPE(shape, bmk_shape);

  return shape;
}

uint32_t cvk1822_lmem_tensor_to_size(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tensor_lmem_shape_t bmk_shape;
  SET_TL_SHAPE(bmk_shape, shape);

  return bmk1822_lmem_tensor_to_size(bmk_ctx, bmk_shape, fmt, eu_align);
}

uint32_t cvk1822_lmem_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  return bmk1822_lmem_matrix_to_size(bmk_ctx, bmk_shape, fmt,
                                       eu_align);
}

uint32_t cvk1822_lmem_ps32_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_matrix_lmem_shape_t bmk_shape;
  SET_ML_SHAPE(bmk_shape, shape);

  return bmk1822_lmem_ps32_matrix_to_size(bmk_ctx, bmk_shape, fmt,
                                          eu_align);
}

void cvk1822_gmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tg_t *tg,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt) {
  memset(tg, 0, sizeof(*tg));
  tg->fmt = fmt;
  tg->shape = shape;
  tg->stride = ctx->ops->tg_default_stride(ctx, tg->shape, tg->fmt);
}

void cvk1822_tdma_l2l_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2l_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2l_tensor_copy_param_t bmk_param = {0};
  bmk_param.mv_lut_idx = param->mv_lut_idx;
  bmk_param.mv_lut_base = param->mv_lut_base;
  bmk_param.outstanding = param->outstanding;

  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_l2l_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2l_bf16_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2l_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2l_tensor_copy_param_t bmk_param = {0};
  bmk_param.mv_lut_idx = param->mv_lut_idx;
  bmk_param.mv_lut_base = param->mv_lut_base;
  bmk_param.outstanding = param->outstanding;

  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_l2l_bf16_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2l_tensor_lrn_shift(
    cvk_context_t *ctx,
    const cvk_tdma_l2l_tensor_lrn_shift_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2l_tensor_lrn_shift_param_t bmk_param;
  bmk_param.right_shift = param->right_shift;
  bmk_param.lrn_step = param->lrn_step;

  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_l2l_tensor_lrn_shift(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_l2g_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_bf16_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_l2g_bf16_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_tensor_copy_nc_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_nc_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_nc_transposed_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_tensor_copy_nc_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_bf16_tensor_copy_nc_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_nc_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_nc_transposed_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_bf16_tensor_copy_nc_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_tensor_copy_compressed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_compressed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_compressed_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_compressed_tensor_tgmem_t tg_dst;
  convert_gmem_compressed_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_l2g_tensor_copy_compressed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_tensor_fill_constant(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_fill_constant_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_fill_constant_param_t bmk_param;
  bmk_param.constant = param->constant;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_tensor_fill_constant(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_tensor_copy_cw_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_cw_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_cw_transposed_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_tensor_copy_cw_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_bf16_tensor_copy_cw_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_tensor_copy_cw_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_tensor_copy_cw_transposed_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_bf16_tensor_copy_cw_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_matrix_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_matrix_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_matrix_copy_param_t bmk_param;
  bmk1822_matrix_lmem_t tl_src;
  convert_lmem_matrix(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_matrix_tgmem_t tg_dst;
  convert_gmem_matrix(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_matrix_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_bf16_matrix_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_matrix_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_matrix_copy_param_t bmk_param;
  bmk1822_matrix_lmem_t tl_src;
  convert_lmem_matrix(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_matrix_tgmem_t tg_dst;
  convert_gmem_matrix(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_l2g_bf16_matrix_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_matrix_copy_compressed(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_matrix_copy_compressed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_matrix_copy_compressed_param_t bmk_param;
  bmk1822_matrix_lmem_t tl_src;
  convert_lmem_matrix(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_compressed_matrix_tgmem_t mg_dst;
  convert_gmem_compressed_matrix(&mg_dst, param->dst);
  bmk_param.dst = &mg_dst;

  bmk1822_tdma_l2g_matrix_copy_compressed(bmk_ctx, &bmk_param);
}


void cvk1822_tdma_l2g_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_general_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_general_copy_param_t bmk_param;
  bmk_param.src_address = param->src_address;
  bmk_param.dst_base_reg_index = param->dst_base_reg_index;
  bmk_param.dst_address = param->dst_address;
  bmk_param.bytes = param->bytes;

  bmk1822_tdma_l2g_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_l2g_bf16_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_l2g_bf16_general_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_l2tg_bf16_general_copy_param_t bmk_param;
  bmk_param.src_address = param->src_address;
  bmk_param.dst_base_reg_index = param->dst_base_reg_index;
  bmk_param.dst_address = param->dst_address;
  bmk_param.src_bytes = param->src_bytes;

  bmk1822_tdma_l2g_bf16_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_g2l_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_bf16_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_g2l_bf16_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_tensor_copy_nc_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_nc_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_nc_transposed_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_g2l_tensor_copy_nc_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_bf16_tensor_copy_nc_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_nc_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_nc_transposed_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_g2l_bf16_tensor_copy_nc_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_tensor_copy_chw_rotated(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_chw_rotated_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_chw_rotated_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_g2l_tensor_copy_chw_rotated(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_tensor_copy_decompressed(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_copy_decompressed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_copy_decompressed_param_t bmk_param;
  bmk1822_compressed_tensor_tgmem_t tg_src;
  convert_gmem_compressed_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk_param.intra_cmd_paral = param->intra_cmd_paral;

  bmk1822_tdma_g2l_tensor_copy_decompressed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_tensor_fill_constant(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_fill_constant_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_fill_constant_param_t bmk_param;
  bmk_param.constant = param->constant;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_tg2l_tensor_fill_constant(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_bf16_tensor_fill_constant(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_tensor_fill_constant_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_tensor_fill_constant_param_t bmk_param;
  bmk_param.constant = param->constant;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk1822_tdma_tg2l_bf16_tensor_fill_constant(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_matrix_copy_decompressed(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_matrix_copy_decompressed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_matrix_copy_decompressed_param_t bmk_param;
  bmk1822_compressed_matrix_tgmem_t mg_src;
  convert_gmem_compressed_matrix(&mg_src, param->src);
  bmk_param.src = &mg_src;

  bmk1822_matrix_lmem_t ml_dst;
  convert_lmem_matrix(&ml_dst, param->dst);
  bmk_param.dst = &ml_dst;

  bmk1822_tdma_g2l_matrix_copy_decompressed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_matrix_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_matrix_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_matrix_copy_param_t bmk_param;
  bmk1822_matrix_tgmem_t mg_src;
  convert_gmem_matrix(&mg_src, param->src);
  bmk_param.src = &mg_src;

  bmk1822_matrix_lmem_t ml_dst;
  convert_lmem_matrix(&ml_dst, param->dst);
  bmk_param.dst = &ml_dst;

  bmk1822_tdma_g2l_matrix_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_bf16_matrix_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_matrix_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_matrix_copy_param_t bmk_param;
  bmk1822_matrix_tgmem_t mg_src;
  convert_gmem_matrix(&mg_src, param->src);
  bmk_param.src = &mg_src;

  bmk1822_matrix_lmem_t ml_dst;
  convert_lmem_matrix(&ml_dst, param->dst);
  bmk_param.dst = &ml_dst;

  bmk1822_tdma_g2l_bf16_matrix_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_matrix_copy_row_col_transposed(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_matrix_copy_row_col_transposed_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_matrix_copy_row_col_transposed_param_t bmk_param;
  bmk1822_matrix_tgmem_t mg_src;
  convert_gmem_matrix(&mg_src, param->src);
  bmk_param.src = &mg_src;

  bmk1822_matrix_lmem_t ml_dst;
  convert_lmem_matrix(&ml_dst, param->dst);
  bmk_param.dst = &ml_dst;

  bmk1822_tdma_g2l_matrix_copy_row_col_transposed(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_general_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_general_copy_param_t bmk_param;
  bmk_param.src_base_reg_index = param->src_base_reg_index;
  bmk_param.src_address = param->src_address;
  bmk_param.dst_address = param->dst_address;
  bmk_param.bytes = param->src_base_reg_index;

  bmk1822_tdma_g2l_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2l_bf16_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2l_bf16_general_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2l_bf16_general_copy_param_t bmk_param;
  bmk_param.src_base_reg_index = param->src_base_reg_index;
  bmk_param.src_address = param->src_address;
  bmk_param.dst_address = param->dst_address;
  bmk_param.src_bytes = param->src_base_reg_index;
  bmk_param.src_fmt = param->src_fmt;
  bmk_param.dst_fmt = param->dst_fmt;

  bmk1822_tdma_g2l_bf16_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2g_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_tg2tg_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2g_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_tg2tg_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2g_bf16_general_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_tg2tg_bf16_general_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tdma_g2g_bf16_tensor_copy(
    cvk_context_t *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tdma_tg2tg_tensor_copy_param_t bmk_param;
  bmk1822_tensor_tgmem_t tg_src;
  convert_gmem_tensor(&tg_src, param->src);
  bmk_param.src = &tg_src;

  bmk1822_tensor_tgmem_t tg_dst;
  convert_gmem_tensor(&tg_dst, param->dst);
  bmk_param.dst = &tg_dst;

  bmk1822_tdma_tg2tg_bf16_tensor_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_mul(
    cvk_context_t *ctx,
    const cvk_tiu_mul_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_mul_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;

  bmk1822_tensor_lmem_t tl_b;
  if (!param->b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_mul(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_mul_qm(
    cvk_context_t *ctx,
    const cvk_tiu_mul_qm_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_mul_qdm_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;

  bmk1822_tensor_lmem_t tl_b;
  if (!param->b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.multiplier = param->multiplier;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_mul_qdm(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_mac(
    cvk_context_t *ctx,
    const cvk_tiu_mac_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_mac_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;

  bmk1822_tensor_lmem_t tl_b;
  if (!param->b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.res_is_int8 = param->res_is_int8;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.lshift_bits = param->lshift_bits;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_mac(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_add(
    cvk_context_t *ctx,
    const cvk_tiu_add_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_add_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk_param.b_is_const = param->b_is_const;

  bmk1822_tensor_lmem_t tl_b_high;
  bmk1822_tensor_lmem_t tl_b_low;
  if (!param->b_is_const) {
    if (param->b.high) {
      convert_lmem_tensor(&tl_b_high, param->b.high);
      bmk_param.b_high = &tl_b_high;
    } else {
      bmk_param.b_high = NULL;
    }

    convert_lmem_tensor(&tl_b_low, param->b.low);
    bmk_param.b_low = &tl_b_low;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_add(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_sub(
    cvk_context_t *ctx,
    const cvk_tiu_sub_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_sub_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk1822_tensor_lmem_t tl_b_high;
  if (param->b_high) {
    convert_lmem_tensor(&tl_b_high, param->b_high);
    bmk_param.b_high = &tl_b_high;
  } else {
    bmk_param.b_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_b_low;
  convert_lmem_tensor(&tl_b_low, param->b_low);
  bmk_param.b_low = &tl_b_low;

  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_sub(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_max(
    cvk_context_t *ctx,
    const cvk_tiu_max_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_max_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_max;
  convert_lmem_tensor(&tl_max, param->max);
  bmk_param.max = &tl_max;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;
  bmk1822_tensor_lmem_t tl_b;
  if (!param->b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.layer_id = param->layer_id;
  bmk1822_tiu_element_wise_max(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_min(
    cvk_context_t *ctx,
    const cvk_tiu_min_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_min_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_min;
  convert_lmem_tensor(&tl_min, param->min);
  bmk_param.min = &tl_min;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;
  bmk1822_tensor_lmem_t tl_b;
  if (!param->b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_min(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_and_int8(
    cvk_context_t *ctx,
    const cvk_tiu_and_int8_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_and_int8_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res;
  convert_lmem_tensor(&tl_res, param->res);
  bmk_param.res = &tl_res;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk1822_tensor_lmem_t tl_b;
  convert_lmem_tensor(&tl_b, param->b);
  bmk_param.b = &tl_b;

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_and_int8(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_arith_shift(
    cvk_context_t *ctx,
    const cvk_tiu_arith_shift_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_arith_shift_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk1822_tensor_lmem_t tl_bits;
  convert_lmem_tensor(&tl_bits, param->bits);
  bmk_param.bits = &tl_bits;

  bmk1822_tiu_element_wise_arith_shift(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_and_int16(
    cvk_context_t *ctx,
    const cvk_tiu_and_int16_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_and_int16_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk1822_tensor_lmem_t tl_b_high;
  if (param->b_high) {
    convert_lmem_tensor(&tl_b_high, param->b_high);
    bmk_param.b_high = &tl_b_high;
  } else {
    bmk_param.b_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_b_low;
  convert_lmem_tensor(&tl_b_low, param->b_low);
  bmk_param.b_low = &tl_b_low;

  bmk1822_tiu_element_wise_and_int16(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_or_int8(
    cvk_context_t *ctx,
    const cvk_tiu_or_int8_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_or_int8_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res;
  convert_lmem_tensor(&tl_res, param->res);
  bmk_param.res = &tl_res;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk1822_tensor_lmem_t tl_b;
  convert_lmem_tensor(&tl_b, param->b);
  bmk_param.b = &tl_b;

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_or_int8(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_or_int16(
    cvk_context_t *ctx,
    const cvk_tiu_or_int16_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_or_int16_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk1822_tensor_lmem_t tl_b_high;
  if (param->b_high) {
    convert_lmem_tensor(&tl_b_high, param->b_high);
    bmk_param.b_high = &tl_b_high;
  } else {
    bmk_param.b_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_b_low;
  convert_lmem_tensor(&tl_b_low, param->b_low);
  bmk_param.b_low = &tl_b_low;

  bmk1822_tiu_element_wise_or_int16(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_xor_int8(
    cvk_context_t *ctx,
    const cvk_tiu_xor_int8_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_xor_int8_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res;
  convert_lmem_tensor(&tl_res, param->res);
  bmk_param.res = &tl_res;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk1822_tensor_lmem_t tl_b;
  convert_lmem_tensor(&tl_b, param->b);
  bmk_param.b = &tl_b;

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_xor_int8(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_xor_int16(
    cvk_context_t *ctx,
    const cvk_tiu_xor_int16_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_xor_int16_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_res_high;
  if (param->res_high) {
    convert_lmem_tensor(&tl_res_high, param->res_high);
    bmk_param.res_high = &tl_res_high;
  } else {
    bmk_param.res_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_res_low;
  convert_lmem_tensor(&tl_res_low, param->res_low);
  bmk_param.res_low = &tl_res_low;

  bmk1822_tensor_lmem_t tl_a_high;
  if (param->a_high) {
    convert_lmem_tensor(&tl_a_high, param->a_high);
    bmk_param.a_high = &tl_a_high;
  } else {
    bmk_param.a_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_a_low;
  convert_lmem_tensor(&tl_a_low, param->a_low);
  bmk_param.a_low = &tl_a_low;

  bmk1822_tensor_lmem_t tl_b_high;
  if (param->b_high) {
    convert_lmem_tensor(&tl_b_high, param->b_high);
    bmk_param.b_high = &tl_b_high;
  } else {
    bmk_param.b_high = NULL;
  }

  bmk1822_tensor_lmem_t tl_b_low;
  convert_lmem_tensor(&tl_b_low, param->b_low);
  bmk_param.b_low = &tl_b_low;

  bmk1822_tiu_element_wise_xor_int16(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_copy(
    cvk_context_t *ctx,
    const cvk_tiu_copy_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_copy_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_src;
  convert_lmem_tensor(&tl_src, param->src);
  bmk_param.src = &tl_src;

  bmk1822_tensor_lmem_t tl_dst;
  convert_lmem_tensor(&tl_dst, param->dst);
  bmk_param.dst = &tl_dst;

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_copy(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_lookup_table(
    cvk_context_t *ctx,
    const cvk_tiu_lookup_table_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_lookup_table_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk1822_tensor_lmem_t tl_table;
  convert_lmem_tensor(&tl_table, param->table);
  bmk_param.table = &tl_table;

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_lookup_table(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_bf16_lookup_interp_table(
    cvk_context_t *ctx,
    const cvk_tiu_bf16_lookup_interp_table_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tensor_lmem_t ifmap;
  convert_lmem_tensor(&ifmap, param->ifmap);

  bmk1822_tensor_lmem_t buf;
  convert_lmem_tensor(&buf, param->buf);

  bmk1822_tensor_lmem_t tbl_answer;
  convert_lmem_tensor(&tbl_answer, param->tbl_answer);

  bmk1822_tensor_lmem_t tbl_answer_mantissa;
  convert_lmem_tensor(&tbl_answer_mantissa, param->tbl_answer_mantissa);

  bmk1822_tensor_lmem_t ofmap;
  convert_lmem_tensor(&ofmap, param->ofmap);

  if (param->is_scientific) {
    // issue lut cmd
    bmk1822_tdma_l2l_tensor_copy_param_t p10;
    // remove low 8 bits by int8 copy with stride
    // get index(pow)
    memset(&p10, 0x00, sizeof(bmk1822_tdma_l2l_tensor_copy_param_t));
    p10.dst = &ofmap;
    p10.src = &ifmap;
    p10.mv_lut_base = false; // MUST init by ifself in soc
    p10.mv_lut_idx = true;
    bmk1822_tdma_l2l_bf16_tensor_copy(bmk_ctx, &p10);
    p10.mv_lut_idx = false;

    // get f(x0) = 2^(x0*-0.5)
    bmk1822_tiu_lookup_table_param_t p12;
    p12.ofmap = &ofmap;
    p12.ifmap = &ofmap;
    p12.table = &tbl_answer;
    p12.layer_id = param->layer_id;
    bmk1822_tiu_lookup_table(bmk_ctx, &p12);

    // get mantissa value
    p12.ofmap = &buf;
    p12.ifmap = &ifmap;
    p12.table = &tbl_answer_mantissa;
    bmk1822_tiu_lookup_table(bmk_ctx, &p12);

    // (2^exp) * mantissa
    bmk1822_tiu_element_wise_mul_param_t p1;
    p1.res_high = NULL;
    p1.res_low = &ofmap;
    p1.a = &ofmap;
    p1.b_is_const = 0;
    p1.b = &buf;
    p1.rshift_bits = 0;
    p1.relu_enable = 0;
    p1.layer_id = param->layer_id;
    bmk1822_tiu_element_wise_mul(bmk_ctx, &p1);
  }
  else {
    // duplicate from cvikernel_1880v2.c
    const cvk_tl_t *tl_ifmap = param->ifmap;
    const cvk_tl_t *tl_ofmap_slope = param->buf;
    const cvk_tl_t *tl_table_answer = param->tbl_answer;
    const cvk_tl_t *tl_table_answer_slope = param->tbl_answer_mantissa;
    const cvk_tl_t *tl_ofmap_y0 = param->ofmap;
    float min = param->min;
    float max = param->max;
    float scale = 256 / (max - min); // 256 means hw support lut index size
    uint8_t eu_align = param->eu_align;
    cvk_fmt_t fmt = CVK_FMT_BF16;

    cvk_tl_shape_t tl_ofmap_x0_int8_shape = {
      1, tl_ifmap->shape.c, tl_ifmap->shape.h * tl_ifmap->shape.w, 1};

    // filter y = max(range_min, x)
    cvk_tiu_max_param_t p1 = {0};
    p1.max = tl_ifmap;
    p1.a = tl_ifmap;
    p1.b_is_const = 1;
    p1.b_const.is_signed = 1;
    p1.b_const.val = ctx->misc_ops->float_to_bfloat16(ctx, min);
    p1.layer_id = param->layer_id;
    ctx->ops->tiu_max(ctx, &p1);

    // filter y = min(8, x)
    cvk_tiu_min_param_t p2 = {0};
    p2.min = tl_ifmap;
    p2.a = tl_ifmap;
    p2.b_is_const = 1;
    p2.b_const.val = ctx->misc_ops->float_to_bfloat16(ctx, max - 1 / scale); // corner
    p2.b_const.is_signed = 1;
    p2.layer_id = param->layer_id;
    ctx->ops->tiu_min(ctx, &p2);

    cvk_tdma_l2l_tensor_copy_param_t p3 = {0};
    // scale input for remap its idx(-x~x) to (-127~127), dirty tl_ifmap
    cvk_tiu_mul_param_t p4 = {0};
    p4.res_high = NULL;
    p4.res_low = tl_ifmap;
    p4.a = tl_ifmap;
    p4.b_is_const = 1;
    p4.b_const.val = ctx->misc_ops->float_to_bfloat16(ctx, scale);
    p4.rshift_bits = 0;
    p4.relu_enable = 0;
    p4.layer_id = param->layer_id;
    ctx->ops->tiu_mul(ctx, &p4);

    // <! get idx from bf16->int8
    memset(&p3, 0x00, sizeof(cvk_tdma_l2l_tensor_copy_param_t));
    cvk_tl_t dst;
    memcpy(&dst, tl_ofmap_y0, sizeof(cvk_tl_t));

    dst.shape = tl_ofmap_x0_int8_shape;
    dst.fmt = CVK_FMT_I8;
    dst.stride =
      ctx->ops->tl_default_stride(ctx, tl_ofmap_x0_int8_shape, CVK_FMT_I8, eu_align);
    dst.stride.h = dst.stride.h * 2;
    dst.int8_rnd_mode = 1;
    p3.dst = &dst;
    p3.src = tl_ifmap;
    ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p3);
    dst.int8_rnd_mode = 0; // reset

    // <! int8 to bf16 format cus for sub use, sub MUST in the same format
    memset(&p3, 0x00, sizeof(cvk_tdma_l2l_tensor_copy_param_t));
    p3.dst = tl_ofmap_slope; //<! bf16
    p3.src = &dst;
    ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p3);

    // <! sub, diff base , a - b
    // (x - x0)
    cvk_tiu_sub_param_t p5 = {0};
    p5.res_high = 0;
    p5.res_low = tl_ifmap;
    p5.a_high = 0;
    p5.a_low = tl_ifmap;
    p5.b_high = 0;
    p5.b_low = tl_ofmap_slope;
    p5.rshift_bits = 0;
    ctx->ops->tiu_sub(ctx, &p5);

    // get f(x0) and slope(x)
    // reshape, 16->16
    dst.fmt = fmt;
    dst.shape = tl_ofmap_slope->shape;
    dst.stride = tl_ofmap_slope->stride;

    // <! get slope by index
    cvk_tiu_lookup_table_param_t p6 = {0};
    memset(&p6, 0x0, sizeof(cvk_tiu_lookup_table_param_t));
    p6.ofmap = tl_ofmap_slope;
    p6.ifmap = &dst;
    p6.table = tl_table_answer_slope;
    p6.layer_id = param->layer_id;
    ctx->ops->tiu_lookup_table(ctx, &p6);

    // base f(x0)
    memset(&p6, 0x0, sizeof(cvk_tiu_lookup_table_param_t));
    p6.ofmap = tl_ofmap_y0;
    p6.ifmap = &dst;
    p6.table = tl_table_answer;
    p6.layer_id = param->layer_id;
    ctx->ops->tiu_lookup_table(ctx, &p6);

    // <! mac
    // <! part A + part B, a * b + res = res
    cvk_tiu_mac_param_t p7 = {0};
    p7.res_high = 0;
    p7.res_low = tl_ofmap_y0;
    p7.res_is_int8 = 0;
    p7.a = tl_ifmap;
    p7.b_is_const = 0;
    p7.b = tl_ofmap_slope;
    p7.lshift_bits = 0; // lshift_bits;
    p7.rshift_bits = 0; // rshift_bits;
    p7.relu_enable = 0;
    p7.layer_id = param->layer_id;
    ctx->ops->tiu_mac(ctx, &p7);

  }
}

void cvk1822_tiu_pt_convolution(
    cvk_context_t *ctx,
    const cvk_tiu_pt_convolution_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_convolution_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk1822_tensor_lmem_t tl_weight;
  convert_lmem_tensor(&tl_weight, param->weight);
  bmk_param.weight = &tl_weight;

  bmk1822_tensor_lmem_t tl_bias;
  if (param->bias) {
    convert_lmem_tensor(&tl_bias, param->bias);
    bmk_param.bias = &tl_bias;
  } else {
    bmk_param.bias = NULL;
  }

  bmk_param.ins_h = param->ins_h;
  bmk_param.ins_last_h = param->ins_last_h;
  bmk_param.ins_w = param->ins_w;
  bmk_param.ins_last_w = param->ins_last_w;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.dilation_h = param->dilation_h;
  bmk_param.dilation_w = param->dilation_w;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.ps32_mode = param->ps32_mode;
  bmk_param.w_is_const = param->w_is_const;
  bmk_param.layer_id = param->layer_id;
  bmk_param.fp_round_typ = param->fp_round_typ;
  bmk_param.cmd_pre_exe_typ = param->cmd_pre_exe_typ;
  bmk_param.cmd_pre_exe = param->cmd_pre_exe;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;

  bmk1822_tiu_convolution(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_convolution(
    cvk_context_t *ctx,
    const cvk_tiu_convolution_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_convolution_qdm_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk1822_tensor_lmem_t tl_weight;
  convert_lmem_tensor(&tl_weight, param->weight);
  bmk_param.weight = &tl_weight;

  bmk1822_tensor_lmem_t tl_chl_quan_param;
  if (param->chl_quan_param) {
    convert_lmem_tensor(&tl_chl_quan_param, param->chl_quan_param);
    bmk_param.chl_quan_param = &tl_chl_quan_param;
  } else {
    bmk_param.chl_quan_param = NULL;
  }

  bmk_param.ins_h = param->ins_h;
  bmk_param.ins_last_h = param->ins_last_h;
  bmk_param.ins_w = param->ins_w;
  bmk_param.ins_last_w = param->ins_last_w;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.dilation_h = param->dilation_h;
  bmk_param.dilation_w = param->dilation_w;
  bmk_param.has_bias = param->has_bias;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.ps32_mode = param->ps32_mode;
  bmk_param.w_is_const = param->w_is_const;
  bmk_param.layer_id = param->layer_id;
  bmk_param.cmd_pre_exe_typ = param->cmd_pre_exe_typ;
  bmk_param.cmd_pre_exe = param->cmd_pre_exe;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;

  bmk1822_tiu_convolution_qdm(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_max_pooling(
    cvk_context_t *ctx,
    const cvk_tiu_max_pooling_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_max_pooling_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk_param.kh = param->kh;
  bmk_param.kw = param->kw;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_max_pooling(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_average_pooling(
    cvk_context_t *ctx,
    const cvk_tiu_average_pooling_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_average_pooling_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk_param.kh = param->kh;
  bmk_param.kw = param->kw;
  bmk_param.ins_h = param->ins_h;
  bmk_param.ins_last_h = param->ins_last_h;
  bmk_param.ins_w = param->ins_w;
  bmk_param.ins_last_w = param->ins_last_w;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.avg_pooling_const = param->avg_pooling_const;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.layer_id = param->layer_id;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;

  bmk1822_tiu_average_pooling(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_pt_depthwise_convolution(
    cvk_context_t *ctx,
    const cvk_tiu_depthwise_pt_convolution_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_depthwise_convolution_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk1822_tensor_lmem_t tl_weight;
  convert_lmem_tensor(&tl_weight, param->weight);
  bmk_param.weight = &tl_weight;

  bmk1822_tensor_lmem_t tl_bias;
  if (param->bias) {
    convert_lmem_tensor(&tl_bias, param->bias);
    bmk_param.bias = &tl_bias;
  } else {
    bmk_param.bias = NULL;
  }

  bmk_param.ins_h = param->ins_h;
  bmk_param.ins_last_h = param->ins_last_h;
  bmk_param.ins_w = param->ins_w;
  bmk_param.ins_last_w = param->ins_last_w;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.dilation_h = param->dilation_h;
  bmk_param.dilation_w = param->dilation_w;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.layer_id = param->layer_id;
  bmk_param.cmd_pre_exe_typ = param->cmd_pre_exe_typ;
  bmk_param.cmd_pre_exe = param->cmd_pre_exe;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;
  bmk_param.weight_is_const = param->weight_is_const;
  bmk_param.weight_const.is_signed = param->weight_const.is_signed;
  bmk_param.weight_const.val = param->weight_const.val;

  bmk1822_tiu_depthwise_convolution(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_depthwise_convolution(
    cvk_context_t *ctx,
    const cvk_tiu_depthwise_convolution_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_depthwise_convolution_qdm_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk1822_tensor_lmem_t tl_weight;
  convert_lmem_tensor(&tl_weight, param->weight);
  bmk_param.weight = &tl_weight;

  bmk1822_tensor_lmem_t tl_chl_quan_param;
  convert_lmem_tensor(&tl_chl_quan_param, param->chl_quan_param);
  bmk_param.chl_quan_param = &tl_chl_quan_param;

  bmk_param.ins_h = param->ins_h;
  bmk_param.ins_last_h = param->ins_last_h;
  bmk_param.ins_w = param->ins_w;
  bmk_param.ins_last_w = param->ins_last_w;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.dilation_h = param->dilation_h;
  bmk_param.dilation_w = param->dilation_w;
  bmk_param.has_bias = param->has_bias;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.layer_id = param->layer_id;
  bmk_param.cmd_pre_exe_typ = param->cmd_pre_exe_typ;
  bmk_param.cmd_pre_exe = param->cmd_pre_exe;
  bmk_param.ins_val = param->ins_val;
  bmk_param.ins_fp = param->ins_fp;
  bmk_param.weight_is_const = param->weight_is_const;
  bmk_param.weight_const.is_signed = param->weight_const.is_signed;
  bmk_param.weight_const.val = param->weight_const.val;

  bmk1822_tiu_depthwise_convolution_qdm(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_matrix_multiplication(
    cvk_context_t *ctx,
    const cvk_tiu_matrix_multiplication_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_matrix_multiplication_param_t bmk_param;
  bmk1822_matrix_lmem_t ml_res;
  convert_lmem_matrix(&ml_res, param->res);
  bmk_param.res = &ml_res;

  bmk1822_matrix_lmem_t ml_left;
  convert_lmem_matrix(&ml_left, param->left);
  bmk_param.left = &ml_left;

  bmk1822_matrix_lmem_t ml_right;
  convert_lmem_matrix(&ml_right, param->right);
  bmk_param.right = &ml_right;

  bmk1822_matrix_lmem_t ml_bias;
  if (param->bias) {
    convert_lmem_matrix(&ml_bias, param->bias);
    bmk_param.bias = &ml_bias;
  } else {
    bmk_param.bias = NULL;
  }

  bmk_param.lshift_bits = param->lshift_bits;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.res_is_int8 = param->res_is_int8;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.add_result = param->add_result;
  bmk_param.ps32_mode = param->ps32_mode;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_matrix_multiplication(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_matrix_multiplication_qm(
    cvk_context_t *ctx,
    const cvk_tiu_matrix_multiplication_qm_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_matrix_multiplication_qdm_param_t bmk_param;
  bmk1822_matrix_lmem_t ml_res;
  convert_lmem_matrix(&ml_res, param->res);
  bmk_param.res = &ml_res;

  bmk1822_matrix_lmem_t ml_left;
  convert_lmem_matrix(&ml_left, param->left);
  bmk_param.left = &ml_left;

  bmk1822_matrix_lmem_t ml_right;
  convert_lmem_matrix(&ml_right, param->right);
  bmk_param.right = &ml_right;

  bmk1822_matrix_lmem_t ml_bias;
  if (param->bias) {
    convert_lmem_matrix(&ml_bias, param->bias);
    bmk_param.bias = &ml_bias;
  } else {
    bmk_param.bias = NULL;
  }

  bmk_param.lshift_bits = param->lshift_bits;
  bmk_param.rshift_bits = param->rshift_bits;
  bmk_param.res_is_int8 = param->res_is_int8;
  bmk_param.relu_enable = param->relu_enable;
  bmk_param.add_result = param->add_result;
  bmk_param.ps32_mode = param->ps32_mode;
  bmk_param.quan_m = param->quan_m;
  bmk_param.layer_id = param->layer_id;
  bmk1822_tiu_matrix_multiplication_qdm(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_ge(
    cvk_context_t *ctx,
    const cvk_tiu_ge_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_element_wise_ge_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ge;
  convert_lmem_tensor(&tl_ge, param->ge);
  bmk_param.ge = &tl_ge;

  bmk1822_tensor_lmem_t tl_a;
  convert_lmem_tensor(&tl_a, param->a);
  bmk_param.a = &tl_a;

  bmk_param.b_is_const = param->b_is_const;
  bmk1822_tensor_lmem_t tl_b;
  if (!bmk_param.b_is_const) {
    convert_lmem_tensor(&tl_b, param->b);
    bmk_param.b = &tl_b;
  } else {
    bmk_param.b_const.val = param->b_const.val;
    bmk_param.b_const.is_signed = param->b_const.is_signed;
  }

  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_element_wise_ge(bmk_ctx, &bmk_param);
}

void cvk1822_tiu_min_pooling(
    struct cvikernel_context *ctx,
    const cvk_tiu_min_pooling_param_t *param)
{
  bmk1822_context_t *bmk_ctx =
      ((cvk_prv_data_t *)ctx->priv_data)->bmk_ctx;

  bmk1822_tiu_min_pooling_param_t bmk_param;
  bmk1822_tensor_lmem_t tl_ofmap;
  convert_lmem_tensor(&tl_ofmap, param->ofmap);
  bmk_param.ofmap = &tl_ofmap;

  bmk1822_tensor_lmem_t tl_ifmap;
  convert_lmem_tensor(&tl_ifmap, param->ifmap);
  bmk_param.ifmap = &tl_ifmap;

  bmk_param.kh = param->kh;
  bmk_param.kw = param->kw;
  bmk_param.pad_top = param->pad_top;
  bmk_param.pad_bottom = param->pad_bottom;
  bmk_param.pad_left = param->pad_left;
  bmk_param.pad_right = param->pad_right;
  bmk_param.stride_h = param->stride_h;
  bmk_param.stride_w = param->stride_w;
  bmk_param.ins_fp = param->ins_fp;
  bmk_param.layer_id = param->layer_id;

  bmk1822_tiu_min_pooling(bmk_ctx, &bmk_param);
}

uint16_t cvk1822_float_to_bfloat16(
    cvk_context_t *ctx,
    float data)
{
  (void)ctx;

  return convert_fp32_bf16(data);
}

void cvk1822_bf16_table_shape(
    cvk_context_t *ctx,
    cvk_tl_shape_t *shape)
{
  if (!ctx || !shape)
    return;

  shape->n = 1;
  shape->c = ctx->info.npu_num;
  shape->h = 32;  // hard-coded in 1880v2
  shape->w = 8; // hard-coded in 1822
}

static cvk_operations_t cvikernel_1822_ops = {
  .cleanup = cvk1822_cleanup,
  .reset = cvk1822_reset,
  .acquire_cmdbuf = cvk1822_acquire_cmdbuf,
  .dmabuf_size = bmk1822_dmabuf_size,
  .dmabuf_convert = bmk1822_dmabuf_convert,
  .set_layer_id = cvk1822_set_layer_id,
  .parallel_enable = cvk1822_parallel_enable,
  .parallel_disable = cvk1822_parallel_disable,
  .lmem_alloc_tensor = cvk1822_lmem_alloc_tensor,
  .lmem_alloc_matrix = cvk1822_lmem_alloc_matrix,
  .lmem_alloc_ps32_matrix = cvk1822_lmem_alloc_ps32_matrix,
  .lmem_free_tensor = cvk1822_lmem_free_tensor,
  .lmem_free_matrix = cvk1822_lmem_free_matrix,
  .lmem_init_tensor = cvk1822_lmem_init_tensor,
  .lmem_init_matrix = cvk1822_lmem_init_matrix,
  .tl_default_stride = cvk1822_tl_default_stride,
  .tg_default_stride = cvk1822_tg_default_stride,
  .ml_default_shape = cvk1822_ml_default_shape,
  .ml_default_stride = cvk1822_ml_default_stride,
  .ml_shape_t1 = cvk1822_ml_shape_t1,
  .lmem_tensor_to_size = cvk1822_lmem_tensor_to_size,
  .lmem_matrix_to_size = cvk1822_lmem_matrix_to_size,
  .lmem_ps32_matrix_to_size = cvk1822_lmem_ps32_matrix_to_size,
  .gmem_init_tensor = cvk1822_gmem_init_tensor,
  .tdma_l2l_tensor_copy = cvk1822_tdma_l2l_tensor_copy,
  .tdma_l2l_bf16_tensor_copy = cvk1822_tdma_l2l_bf16_tensor_copy,
  .tdma_l2l_tensor_lrn_shift = cvk1822_tdma_l2l_tensor_lrn_shift,
  .tdma_l2g_tensor_copy = cvk1822_tdma_l2g_tensor_copy,
  .tdma_l2g_bf16_tensor_copy = cvk1822_tdma_l2g_bf16_tensor_copy,
  .tdma_l2g_tensor_copy_nc_transposed = cvk1822_tdma_l2g_tensor_copy_nc_transposed,
  .tdma_l2g_bf16_tensor_copy_nc_transposed = cvk1822_tdma_l2g_bf16_tensor_copy_nc_transposed,
  .tdma_l2g_tensor_copy_compressed = cvk1822_tdma_l2g_tensor_copy_compressed,
  .tdma_l2g_tensor_fill_constant = cvk1822_tdma_l2g_tensor_fill_constant,
  .tdma_l2g_tensor_copy_cw_transposed = cvk1822_tdma_l2g_tensor_copy_cw_transposed,
  .tdma_l2g_bf16_tensor_copy_cw_transposed = cvk1822_tdma_l2g_bf16_tensor_copy_cw_transposed,
  .tdma_l2g_matrix_copy = cvk1822_tdma_l2g_matrix_copy,
  .tdma_l2g_bf16_matrix_copy = cvk1822_tdma_l2g_bf16_matrix_copy,
  .tdma_l2g_matrix_copy_compressed = cvk1822_tdma_l2g_matrix_copy_compressed,
  .tdma_l2g_general_copy = cvk1822_tdma_l2g_general_copy,
  .tdma_l2g_bf16_general_copy = cvk1822_tdma_l2g_bf16_general_copy,
  .tdma_g2l_tensor_copy = cvk1822_tdma_g2l_tensor_copy,
  .tdma_g2l_bf16_tensor_copy = cvk1822_tdma_g2l_bf16_tensor_copy,
  .tdma_g2l_tensor_copy_nc_transposed = cvk1822_tdma_g2l_tensor_copy_nc_transposed,
  .tdma_g2l_bf16_tensor_copy_nc_transposed = cvk1822_tdma_g2l_bf16_tensor_copy_nc_transposed,
  .tdma_g2l_tensor_copy_chw_rotated = cvk1822_tdma_g2l_tensor_copy_chw_rotated,
  .tdma_g2l_tensor_copy_decompressed = cvk1822_tdma_g2l_tensor_copy_decompressed,
  .tdma_g2l_tensor_fill_constant = cvk1822_tdma_g2l_tensor_fill_constant,
  .tdma_g2l_bf16_tensor_fill_constant = cvk1822_tdma_g2l_bf16_tensor_fill_constant,
  .tdma_g2l_matrix_copy_decompressed = cvk1822_tdma_g2l_matrix_copy_decompressed,
  .tdma_g2l_matrix_copy = cvk1822_tdma_g2l_matrix_copy,
  .tdma_g2l_bf16_matrix_copy = cvk1822_tdma_g2l_bf16_matrix_copy,
  .tdma_g2l_matrix_copy_row_col_transposed = cvk1822_tdma_g2l_matrix_copy_row_col_transposed,
  .tdma_g2l_general_copy = cvk1822_tdma_g2l_general_copy,
  .tdma_g2l_bf16_general_copy = cvk1822_tdma_g2l_bf16_general_copy,
  .tdma_g2g_tensor_copy = cvk1822_tdma_g2g_tensor_copy,
  .tdma_g2g_general_copy = cvk1822_tdma_g2g_general_copy,
  .tdma_g2g_bf16_general_copy = cvk1822_tdma_g2g_bf16_general_copy,
  .tdma_g2g_bf16_tensor_copy = cvk1822_tdma_g2g_bf16_tensor_copy,
  .tiu_mul = cvk1822_tiu_mul,
  .tiu_mul_qm = cvk1822_tiu_mul_qm,
  .tiu_mac = cvk1822_tiu_mac,
  .tiu_add = cvk1822_tiu_add,
  .tiu_sub = cvk1822_tiu_sub,
  .tiu_max = cvk1822_tiu_max,
  .tiu_min = cvk1822_tiu_min,
  .tiu_and_int8 = cvk1822_tiu_and_int8,
  .tiu_arith_shift = cvk1822_tiu_arith_shift,
  .tiu_and_int16 = cvk1822_tiu_and_int16,
  .tiu_or_int8 = cvk1822_tiu_or_int8,
  .tiu_or_int16 = cvk1822_tiu_or_int16,
  .tiu_xor_int8 = cvk1822_tiu_xor_int8,
  .tiu_xor_int16 = cvk1822_tiu_xor_int16,
  .tiu_copy = cvk1822_tiu_copy,
  .tiu_lookup_table = cvk1822_tiu_lookup_table,
  .tiu_bf16_lookup_interp_table = cvk1822_tiu_bf16_lookup_interp_table,
  .tiu_pt_convolution = cvk1822_tiu_pt_convolution,
  .tiu_convolution = cvk1822_tiu_convolution,
  .tiu_max_pooling = cvk1822_tiu_max_pooling,
  .tiu_average_pooling = cvk1822_tiu_average_pooling,
  .tiu_pt_depthwise_convolution = cvk1822_tiu_pt_depthwise_convolution,
  .tiu_depthwise_convolution = cvk1822_tiu_depthwise_convolution,
  .tiu_matrix_multiplication = cvk1822_tiu_matrix_multiplication,
  .tiu_matrix_multiplication_qm = cvk1822_tiu_matrix_multiplication_qm,
  .tiu_ge = cvk1822_tiu_ge,
  .tiu_min_pooling = cvk1822_tiu_min_pooling,
};

static cvk_misc_operations_t cvikernel_1822_misc_ops = {
  .float_to_bfloat16 = cvk1822_float_to_bfloat16,
  .bf16_table_shape = cvk1822_bf16_table_shape,
};

char *cvikernel_get_chip_info_1822(void)
{
  return CVI_TPU_VERSION_182X;
}

void cvikernel_init_1822(
    cvk_reg_info_t *req_info,
    cvk_context_t *ctx)
{
  ctx->info.version = BM1822_VER;
  ctx->info.node_num = BM1822_HW_NODE_CHIP_NUM;
  ctx->info.node_shift = BM1822_HW_NODE_CHIP_SHIFT;
  ctx->info.npu_num = BM1822_HW_NPU_NUM;
  ctx->info.npu_shift = BM1822_HW_NPU_SHIFT;
  ctx->info.eu_num = BM1822_HW_EU_NUM;
  ctx->info.eu_shift = BM1822_HW_EU_SHIFT;
  ctx->info.lmem_size = BM1822_HW_LMEM_SIZE;
  ctx->info.lmem_shift = BM1822_HW_LMEM_SHIFT;
  ctx->info.lmem_banks = BM1822_HW_LMEM_BANKS;
  ctx->info.lmem_bank_size = BM1822_HW_LMEM_BANK_SIZE;
  ctx->info.gmem_start = BM1822_GLOBAL_MEM_START_ADDR;
  ctx->info.features = CVK_HWF_FC_OP1_CONST | CVK_HWF_8B_ADD_SUB |
                       CVK_HWF_MIN_POOL | CVK_HWF_M_BRADCAST |
                       CVK_HWF_QM_LSHIFT | CVK_HWF_GE | CVK_HWF_CMD_PRE_EXE;
  ctx->info.gmem_size = BM1822_GLOBAL_MEM_SIZE;

  ctx->ops = &cvikernel_1822_ops;
  ctx->misc_ops = &cvikernel_1822_misc_ops;

  // kernel_init() in bmkernel.c
  bmk1822_context_t *bmk_ctx = xmalloc(sizeof(bmk1822_context_t));
  bmk_ctx->info.chip_version = BM1822_VER;
  bmk_ctx->info.cmdbuf_size = req_info->cmdbuf_size;
  bmk_ctx->info.cmdbuf = req_info->cmdbuf;

  bmk_ctx->chip_info.version = BM1822_VER;
  bmk_ctx->chip_info.node_num = BM1822_HW_NODE_CHIP_NUM;
  bmk_ctx->chip_info.node_shift = BM1822_HW_NODE_CHIP_SHIFT;
  bmk_ctx->chip_info.npu_num = BM1822_HW_NPU_NUM;
  bmk_ctx->chip_info.npu_shift = BM1822_HW_NPU_SHIFT;
  bmk_ctx->chip_info.eu_num = BM1822_HW_EU_NUM;
  bmk_ctx->chip_info.eu_shift = BM1822_HW_EU_SHIFT;
  bmk_ctx->chip_info.lmem_size = BM1822_HW_LMEM_SIZE;
  bmk_ctx->chip_info.lmem_shift = BM1822_HW_LMEM_SHIFT;
  bmk_ctx->chip_info.lmem_banks = BM1822_HW_LMEM_BANKS;
  bmk_ctx->chip_info.lmem_bank_size = BM1822_HW_LMEM_BANK_SIZE;
  bmk_ctx->chip_info.gmem_start = BM1822_GLOBAL_MEM_START_ADDR;
  bmk_ctx->chip_info.gmem_size = BM1822_GLOBAL_MEM_SIZE;

  uint32_t max_nr_desc = bmk1822_estimate_nr_desc(bmk_ctx);

  bmk_ctx->cmdbuf_ptr = 0;
  bmk_ctx->max_nr_desc = max_nr_desc;
  bmk_ctx->cur_nr_desc = 0;
  bmk_ctx->desc_pairs = xmalloc(max_nr_desc * sizeof(bmk_ctx->desc_pairs[0]));
  bmk_ctx->lmem_ptr = 0;

  ec_init(&bmk_ctx->ec, BMK1822_ENGINE_NUM, max_nr_desc);
  mode_manager_init(&bmk_ctx->mode_manager, &bmk_ctx->ec, BMK1822_ENGINE_NUM);

  cvk_prv_data_t *prv_data = malloc(sizeof(cvk_prv_data_t));
  prv_data->bmk_ctx = bmk_ctx;
  prv_data->cmdbuf = req_info->cmdbuf;
  prv_data->cmdbuf_size = req_info->cmdbuf_size;

  ctx->priv_data = prv_data;
}
