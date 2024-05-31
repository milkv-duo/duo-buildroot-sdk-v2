#include "cvkcv180x.h"
#include <stdlib.h>
#include <string.h>

static inline int bitsize_of_fmt(cvk_fmt_t fmt)
{
  switch (fmt) {
    case CVK_FMT_F32:
    case CVK_FMT_I32:
      return 32;
    case CVK_FMT_F16:
    case CVK_FMT_I16:
    case CVK_FMT_U16:
    case CVK_FMT_BF16:
      return 16;
    case CVK_FMT_I8:
    case CVK_FMT_U8:
      return 8;
    default:
      return 32;
  }
}

static void cvkcv180x_replace_cmd_id(uint32_t *desc, uint32_t eng_id, uint16_t ids[])
{
  if (eng_id == CV180X_TIU) {
    tiu_reg_t reg;
    parse_tiu_reg(&reg, desc);
    reg.cmd_id_en = 1;
    reg.cmd_id_tpu = ids[eng_id];
    reg.cmd_id_gdma = ids[CV180X_TDMA];
    emit_tiu_reg(&reg, desc);
  } else if (eng_id == CV180X_TDMA) {
    tdma_reg_t tdma_reg;
    parse_tdma_reg(&tdma_reg, desc);
    tdma_reg.cmd_id = ids[eng_id];
    tdma_reg.wait_id_tpu = ids[CV180X_TIU];
    tdma_reg.bar_en = 1;
    emit_tdma_reg(&tdma_reg, desc);
  }
}

static int cvkcv180x_get_engine_desc_length(uint32_t engine_id)
{
  switch (engine_id) {
    case CV180X_TIU:
      return TIU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    case CV180X_TDMA:
      return TDMA_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    //case CV180X_CPU:
    //  return CPU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    default:
      //ASSERT(0);
      break;
  }

  return 0;
}

// Estimate the number of command descriptor based on buffer size provided
// by the user.
static uint32_t cvkcv180x_estimate_nr_desc(uint32_t cmdbuf_size)
{
  uint32_t tiu_desc_len = cvkcv180x_get_engine_desc_length(CV180X_TIU);
  uint32_t tdma_desc_len = cvkcv180x_get_engine_desc_length(CV180X_TDMA);
  uint32_t hdr_len = sizeof(cmd_hdr_t);

  uint32_t desc_len =
      (tiu_desc_len > tdma_desc_len) ? tiu_desc_len : tdma_desc_len;

  return cmdbuf_size / (desc_len + hdr_len);
}

static cmd_hdr_t *kernel_alloc_cmd_hdr(
    cvk_context_t *ctx, uint8_t eng_id, uint32_t desc_len)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;
  uint32_t free_len = prv_data->cmdbuf_size - prv_data->cmdbuf_ptr;
  uint32_t hdr_len = sizeof(cmd_hdr_t);
  uint32_t total_len = hdr_len + desc_len;

  if (total_len > free_len)
    return NULL;

  cmd_hdr_t *hdr = (cmd_hdr_t *)&prv_data->cmdbuf[prv_data->cmdbuf_ptr];
  hdr->magic = 0xA8; // CMDBUF_HDR_MAGIC_180X
  hdr->len = desc_len;
  hdr->engine_id = eng_id;
  hdr->__deprecated = 0;  // for valgrind
  hdr->flags = 0;
  hdr->mask = 0;

  prv_data->cmdbuf_ptr += total_len;
  return hdr;
}

static desc_pair_t *kernel_alloc_desc_pair(cvk_context_t *ctx, uint8_t eng_id)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  if (eng_id >= CV180X_ENGINE_NUM || prv_data->cur_nr_desc >= prv_data->max_nr_desc)
    return NULL;

  uint32_t desc_len = cvkcv180x_get_engine_desc_length(eng_id);
  desc_pair_t *dp = &prv_data->desc_pairs[prv_data->cur_nr_desc++];
  dp->cmd_hdr = kernel_alloc_cmd_hdr(ctx, eng_id, desc_len);
  dp->ec_desc = ec_alloc_desc(&prv_data->ec, eng_id);

  mode_manager_record_ec_desc(&prv_data->mode_manager, dp->ec_desc);
  return dp;
}

static void cvkcv180x_update_sync_id(cvk_context_t *ctx)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;
  ec_compute_sync_ids(&prv_data->ec);

  for (uint32_t di = 0; di < prv_data->cur_nr_desc; di++) {
    desc_pair_t *dp = &prv_data->desc_pairs[di];
    uint8_t eng_id = dp->ec_desc->engine_id;
    uint32_t *desc = (uint32_t *)dp->cmd_hdr->cmd;
    cvkcv180x_replace_cmd_id(desc, eng_id, dp->ec_desc->sync_ids);
  }
}

desc_pair_t *cvkcv180x_get_desc_pair(cvk_context_t *ctx, uint8_t eng_id)
{
#if 0
  if (eng_id == BMK1822_CPU) {
    kernel_update_sync_id(k);
    k->cur_nr_desc = 0;

    ec_reset(&k->ec);
    mode_manager_restart_sync_id(&k->mode_manager);
  }
#endif

  return kernel_alloc_desc_pair(ctx, eng_id);
}

void cvkcv180x_cleanup(cvk_context_t *ctx)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  free(prv_data->desc_pairs);
  ec_destroy(&prv_data->ec);
  mode_manager_destroy(&prv_data->mode_manager);
}

void cvkcv180x_reset(cvk_context_t *ctx)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  prv_data->cur_nr_desc = 0;
  prv_data->cmdbuf_ptr = 0;

  ec_reset(&prv_data->ec);
  mode_manager_reset(&prv_data->mode_manager);
}

static uint8_t *cvkcv180x_acquire_cmdbuf(cvk_context_t *ctx, uint32_t *size)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  *size = prv_data->cmdbuf_ptr;
  cvkcv180x_update_sync_id(ctx);
  return prv_data->cmdbuf;
}

void cvkcv180x_set_layer_id(
    struct cvikernel_context *ctx,
    uint16_t layer_id)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  prv_data->layer_id = layer_id;
}

void cvkcv180x_parallel_enable(struct cvikernel_context *ctx)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  mode_manager_enable_parallel(&prv_data->mode_manager);
}

void cvkcv180x_parallel_disable(struct cvikernel_context *ctx)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;

  mode_manager_disable_parallel(&prv_data->mode_manager);
}

cvk_tl_stride_t cvkcv180x_tl_default_stride(
    cvk_context_t *ctx,
    cvk_tl_shape_t s,
    cvk_fmt_t fmt_type,
    int eu_align)
{
  cvk_tl_stride_t stride;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t fmt = (fmt_type == CVK_FMT_BF16) ? 2 : 1;
  stride.w = fmt;
  stride.h = s.w * fmt;
  if (eu_align)
    stride.c = align_up(s.h * s.w * fmt, eu_num);
  else
    stride.c = s.h * s.w * fmt;

  stride.n = stride.c * ceiling_func(s.c, npu_num);

  return stride;
}

void cvkcv180x_lmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tl_t *tl,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  memset(tl, 0, sizeof(*tl));
  tl->fmt = fmt;
  tl->shape = shape;
  tl->eu_align = eu_align;
  tl->stride = cvkcv180x_tl_default_stride(ctx, shape, fmt, eu_align);
}

uint32_t cvkcv180x_lmem_tensor_to_size(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  uint32_t eu_num = ctx->info.eu_num;

  cvk_tl_stride_t stride;
  stride = cvkcv180x_tl_default_stride(ctx, shape, fmt, eu_align);

  uint32_t needed = align_up(shape.n * stride.n, eu_num);

  return needed;
}

cvk_tl_t *cvkcv180x_lmem_alloc_tensor(
    cvk_context_t *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;
  uint32_t lmem_size = ctx->info.lmem_size;
  uint32_t eu_num = ctx->info.eu_num;

  cvk_tl_t *t = malloc(sizeof(*t));
  if (!t)
    return NULL;

  memset(t, 0, sizeof(*t));
  t->start_address = prv_data->lmem_ptr;
  t->fmt = fmt;
  t->cmprs_fmt = fmt;
  t->shape = shape;
  t->eu_align = eu_align;
  t->stride = cvkcv180x_tl_default_stride(ctx, shape, fmt, eu_align);

  uint32_t needed = align_up(t->shape.n * t->stride.n, eu_num);
  if ((lmem_size - prv_data->lmem_ptr < needed) || !needed) {
    free(t);
    return NULL;
  }

  prv_data->lmem_ptr += needed;
  return t;
}

void cvkcv180x_lmem_free_tensor(
    struct cvikernel_context *ctx,
    const cvk_tl_t *tl)
{
  cvk_prv_data_t *prv_data;

  if (!ctx || !tl)
    return;

  prv_data = (cvk_prv_data_t *)ctx->priv_data;

  if (tl->start_address >= prv_data->lmem_ptr)
    printf("cvkcv180x lm free tensor: ptr out of range\n");

  prv_data->lmem_ptr = tl->start_address;

  free((void *)tl);
}

static void try_optimize_matrix_shape(cvk_context_t *ctx, cvk_ml_shape_t *s,
                                      cvk_fmt_t fmt_type) {
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t col = s->col;
  uint8_t isBf16 = (fmt_type == CVK_FMT_BF16);
  uint32_t workingNumber = isBf16 ? eu_num / 2 : eu_num;

  if (col >= workingNumber) {
    int num_eu = ceiling_func(col, workingNumber * npu_num);
    s->w = workingNumber * num_eu;
    s->c = ceiling_func(col, s->w);
  } else {
    // col < EU_NUM
    // Only transfer needed data
    // We still change tensor shape in TIU mac op
    s->w = col;
    s->c = 1;
  }
}

cvk_ml_shape_t cvkcv180x_ml_default_shape(
    struct cvikernel_context *ctx,
    uint32_t row,
    uint32_t col,
    cvk_fmt_t fmt_type)
{
  cvk_ml_shape_t shape = {0};
  shape.n = row;
  shape.col = col;

  try_optimize_matrix_shape(ctx, &shape, fmt_type);

  return shape;
}

cvk_ml_stride_t cvkcv180x_ml_default_stride(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t val = (fmt == CVK_FMT_BF16) ? 2 : 1;

  cvk_ml_stride_t stride;
  stride.h = shape.w * val;
  if (eu_align)
    stride.c = align_up(shape.w * val, eu_num);
  else
    stride.c = shape.w * val;
  stride.n = stride.c * ceiling_func(shape.c, npu_num);

  return stride;
}

cvk_ml_shape_t cvkcv180x_ml_shape_t1(
    struct cvikernel_context *ctx,
    uint32_t len,
    cvk_fmt_t fmt_type)
{
  uint32_t lmem_size = ctx->info.lmem_size;
  cvk_ml_shape_t shape = {0};

  uint32_t row = 1;
  uint32_t col = len;

  while (col >= lmem_size) {
    if (col % 2)
      return shape;

    col /= 2;
    row *= 2;
  }

  shape.n = row;
  shape.col = col;

  try_optimize_matrix_shape(ctx, &shape, fmt_type);
  return shape;
}

void cvkcv180x_lmem_init_matrix(
    struct cvikernel_context *ctx,
    cvk_ml_t *ml,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  memset(ml, 0, sizeof(*ml));
  ml->fmt = fmt;
  ml->shape = shape;
  ml->stride = cvkcv180x_ml_default_stride(ctx, shape, fmt, eu_align);
  ml->eu_align = eu_align;
}


uint32_t cvkcv180x_lmem_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t val = (fmt == CVK_FMT_BF16) ? 2 : 1;

  cvk_ml_t t;
  t.fmt = fmt;
  t.shape = shape;
  t.stride.h = shape.w * val;
  if (eu_align)
    t.stride.c = align_up(shape.w * val, eu_num);
  else
    t.stride.c = shape.w * val;
  t.stride.n = t.stride.c * ceiling_func(shape.c, npu_num);

  uint32_t needed = align_up(t.shape.n * t.stride.n, eu_num);

  return needed;
}

uint32_t cvkcv180x_lmem_ps32_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  /* Partial sum is located in lmem in 32-bit format, so we times n to 4 to
   * spare a sapce for it.
   */

  shape.n = shape.n * (bitsize_of_fmt(CVK_FMT_I32) / bitsize_of_fmt(fmt));

  return cvkcv180x_lmem_matrix_to_size(ctx, shape, fmt, eu_align);

}

cvk_ml_t *cvkcv180x_lmem_alloc_matrix(
    cvk_context_t *ctx,
    cvk_ml_shape_t s,
    cvk_fmt_t fmt,
    int eu_align)
{
  cvk_prv_data_t *prv_data = (cvk_prv_data_t *)ctx->priv_data;
  uint32_t lmem_size = ctx->info.lmem_size;
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t val = (fmt == CVK_FMT_BF16) ? 2 : 1;

  cvk_ml_t *t = malloc(sizeof(*t));
  if (!t)
    return NULL;

  memset(t, 0, sizeof(*t));
  t->start_address = prv_data->lmem_ptr;
  t->fmt = fmt;
  t->shape = s;
  t->stride.h = s.w * val;
  if (eu_align)
    t->stride.c = align_up(s.w * val, eu_num);
  else
    t->stride.c = s.w * val;
  t->stride.n = t->stride.c * ceiling_func(s.c, npu_num);
  t->eu_align = eu_align;

  uint32_t needed = align_up(t->shape.n * t->stride.n, eu_num);
  if (lmem_size - prv_data->lmem_ptr < needed) {
    free(t);
    return NULL;
  }
  prv_data->lmem_ptr += needed;

  return t;
}

void cvkcv180x_lmem_free_matrix(
    struct cvikernel_context *ctx,
    const cvk_ml_t *ml)
{
  cvk_prv_data_t *prv_data;

  if (!ctx || !ml)
    return;

  prv_data = (cvk_prv_data_t *)ctx->priv_data;

  if (ml->start_address >= prv_data->lmem_ptr)
    printf("cvkcv180x lm free matrix: ptr out of range\n");

  prv_data->lmem_ptr = ml->start_address;
  free((void *)ml);
}

cvk_ml_t *cvkcv180x_lmem_alloc_ps32_matrix(
    cvk_context_t *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align)
{
  /* Partial sum is located in lmem in 32-bit format, so we times n to 4 to
   * spare a space for it.
   */

  uint32_t prev_n;

  prev_n = shape.n;
  shape.n = shape.n * (bitsize_of_fmt(CVK_FMT_I32) / bitsize_of_fmt(fmt));
  cvk_ml_t *res = cvkcv180x_lmem_alloc_matrix(ctx, shape, fmt, eu_align);

  if(res == NULL) {
    printf("cvkcv180x: alloc ps32 matrix fail\n");
    return NULL;
  }

  res->shape.n = prev_n;
  return res;
}

cvk_tg_stride_t cvkcv180x_tg_default_stride(
    struct cvikernel_context *ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt)
{
  uint32_t data_type_size = (fmt == CVK_FMT_BF16) ? 2 : 1;
  cvk_tg_stride_t stride;
  stride.h = shape.w * data_type_size;
  stride.c = shape.h * stride.h;
  stride.n = shape.c * stride.c;
  stride.w = (fmt == CVK_FMT_BF16) ? 2 : 1;

  (void)ctx;

  return stride;
}

void cvkcv180x_tiu_bf16_lookup_interp_table(
    cvk_context_t *ctx,
    const cvk_tiu_bf16_lookup_interp_table_param_t *param)
{
  if (param->is_scientific) {
    // issue lut cmd
    cvk_tdma_l2l_tensor_copy_param_t p10;
    // remove low 8 bits by int8 copy with stride
    // get index(pow)
    memset(&p10, 0x00, sizeof(cvk_tdma_l2l_tensor_copy_param_t));
    p10.dst = param->ofmap;
    p10.src = param->ifmap;
    p10.mv_lut_base = 0; // MUST init by ifself in soc
    p10.mv_lut_idx = 1;
    p10.layer_id = param->layer_id;
    cvkcv180x_tdma_l2l_bf16_tensor_copy(ctx, &p10);
    p10.mv_lut_idx = 0;

    // get f(x0) = 2^(x0*-0.5)
    cvk_tiu_lookup_table_param_t p12;
    p12.ofmap = param->ofmap;
    p12.ifmap = param->ofmap;
    p12.table = param->tbl_answer;
    p12.layer_id = param->layer_id;
    cvkcv180x_tiu_lookup_table(ctx, &p12);

    // get mantissa value
    p12.ofmap = param->buf;
    p12.ifmap = param->ifmap;
    p12.table = param->tbl_answer_mantissa;
    cvkcv180x_tiu_lookup_table(ctx, &p12);

    // (2^exp) * mantissa
    cvk_tiu_mul_param_t p1;
    p1.res_high = NULL;
    p1.res_low = param->ofmap;
    p1.a = param->ofmap;
    p1.b_is_const = 0;
    p1.b = param->buf;
    p1.rshift_bits = 0;
    p1.relu_enable = 0;
    p1.layer_id = param->layer_id;
    cvkcv180x_tiu_mul(ctx, &p1);
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
    p1.b_const.val = cvk_convert_fp32_bf16(min);
    p1.layer_id = param->layer_id;
    ctx->ops->tiu_max(ctx, &p1);

    // filter y = min(8, x)
    cvk_tiu_min_param_t p2 = {0};
    p2.min = tl_ifmap;
    p2.a = tl_ifmap;
    p2.b_is_const = 1;
    p2.b_const.val = cvk_convert_fp32_bf16(max - 1 / scale); // corner
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
    p4.b_const.val = cvk_convert_fp32_bf16(scale);
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

void cvkcv180x_gmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tg_t *tg,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt) {
  memset(tg, 0, sizeof(*tg));
  tg->fmt = fmt;
  tg->shape = shape;
  tg->stride = cvkcv180x_tg_default_stride(ctx, tg->shape, tg->fmt);
}

static uint16_t cvkcv180x_float_to_bfloat16(
    cvk_context_t *ctx,
    float data)
{
  (void)ctx;

  return cvk_convert_fp32_bf16(data);
}

static void cvkcv180x_bf16_table_shape(
    cvk_context_t *ctx,
    cvk_tl_shape_t *shape)
{
  if (!ctx || !shape)
    return;

  shape->n = 1;
  shape->c = ctx->info.npu_num;
  shape->h = 32;  // hard-coded in cv180x
  shape->w = 8; // hard-coded in cv180x
}

static cvk_operations_t cvk_cv180x_ops = {
  .cleanup = cvkcv180x_cleanup,
  .reset = cvkcv180x_reset,
  .acquire_cmdbuf = cvkcv180x_acquire_cmdbuf,
  .set_layer_id = cvkcv180x_set_layer_id,
  .parallel_enable = cvkcv180x_parallel_enable,
  .parallel_disable = cvkcv180x_parallel_disable,
  .lmem_alloc_tensor = cvkcv180x_lmem_alloc_tensor,
  .lmem_alloc_matrix = cvkcv180x_lmem_alloc_matrix,
  .lmem_alloc_ps32_matrix = cvkcv180x_lmem_alloc_ps32_matrix,
  .lmem_free_tensor = cvkcv180x_lmem_free_tensor,
  .lmem_free_matrix = cvkcv180x_lmem_free_matrix,
  .lmem_init_tensor = cvkcv180x_lmem_init_tensor,
  .lmem_init_matrix = cvkcv180x_lmem_init_matrix,
  .tl_default_stride = cvkcv180x_tl_default_stride,
  .tg_default_stride = cvkcv180x_tg_default_stride,
  .ml_default_shape = cvkcv180x_ml_default_shape,
  .ml_default_stride = cvkcv180x_ml_default_stride,
  .ml_shape_t1 = cvkcv180x_ml_shape_t1,
  .lmem_tensor_to_size = cvkcv180x_lmem_tensor_to_size,
  .lmem_matrix_to_size = cvkcv180x_lmem_matrix_to_size,
  .lmem_ps32_matrix_to_size = cvkcv180x_lmem_ps32_matrix_to_size,
  .gmem_init_tensor = cvkcv180x_gmem_init_tensor,
  .tdma_l2l_tensor_copy = cvkcv180x_tdma_l2l_bf16_tensor_copy,
  .tdma_l2l_bf16_tensor_copy = cvkcv180x_tdma_l2l_bf16_tensor_copy,
  .tdma_l2l_tensor_lrn_shift = cvkcv180x_tdma_l2l_tensor_lrn_shift,
  .tdma_l2g_tensor_copy = cvkcv180x_tdma_l2g_bf16_tensor_copy,
  .tdma_l2g_bf16_tensor_copy = cvkcv180x_tdma_l2g_bf16_tensor_copy,
  .tdma_l2g_tensor_copy_nc_transposed = cvkcv180x_tdma_l2g_bf16_tensor_copy_nc_transposed,
  .tdma_l2g_bf16_tensor_copy_nc_transposed = cvkcv180x_tdma_l2g_bf16_tensor_copy_nc_transposed,
  .tdma_l2g_tensor_copy_compressed = cvkcv180x_tdma_l2g_tensor_copy_compressed,
  .tdma_l2g_tensor_fill_constant = cvkcv180x_tdma_l2g_tensor_fill_constant,
  .tdma_l2g_tensor_copy_cw_transposed = cvkcv180x_tdma_l2g_bf16_tensor_copy_cw_transposed,
  .tdma_l2g_bf16_tensor_copy_cw_transposed = cvkcv180x_tdma_l2g_bf16_tensor_copy_cw_transposed,
  .tdma_l2g_matrix_copy = cvkcv180x_tdma_l2g_bf16_matrix_copy,
  .tdma_l2g_bf16_matrix_copy = cvkcv180x_tdma_l2g_bf16_matrix_copy,
  .tdma_l2g_matrix_copy_compressed = cvkcv180x_tdma_l2g_matrix_copy_compressed,
  .tdma_l2g_general_copy = cvkcv180x_tdma_l2g_general_copy,
  .tdma_l2g_bf16_general_copy = cvkcv180x_tdma_l2g_bf16_general_copy,
  .tdma_g2l_tensor_copy = cvkcv180x_tdma_g2l_bf16_tensor_copy,
  .tdma_g2l_bf16_tensor_copy = cvkcv180x_tdma_g2l_bf16_tensor_copy,
  .tdma_g2l_tensor_copy_nc_transposed = cvkcv180x_tdma_g2l_bf16_tensor_copy_nc_transposed,
  .tdma_g2l_bf16_tensor_copy_nc_transposed = cvkcv180x_tdma_g2l_bf16_tensor_copy_nc_transposed,
  .tdma_g2l_tensor_copy_chw_rotated = cvkcv180x_tdma_g2l_tensor_copy_chw_rotated,
  .tdma_g2l_tensor_copy_decompressed = cvkcv180x_tdma_g2l_tensor_copy_decompressed,
  .tdma_g2l_tensor_fill_constant = cvkcv180x_tdma_g2l_bf16_tensor_fill_constant,
  .tdma_g2l_bf16_tensor_fill_constant = cvkcv180x_tdma_g2l_bf16_tensor_fill_constant,
  .tdma_g2l_matrix_copy_decompressed = cvkcv180x_tdma_g2l_matrix_copy_decompressed,
  .tdma_g2l_matrix_copy = cvkcv180x_tdma_g2l_bf16_matrix_copy,
  .tdma_g2l_bf16_matrix_copy = cvkcv180x_tdma_g2l_bf16_matrix_copy,
  .tdma_g2l_matrix_copy_row_col_transposed = cvkcv180x_tdma_g2l_matrix_copy_row_col_transposed,
  .tdma_g2l_general_copy = cvkcv180x_tdma_g2l_general_copy,
  .tdma_g2l_bf16_general_copy = cvkcv180x_tdma_g2l_bf16_general_copy,
  .tdma_g2g_tensor_copy = cvkcv180x_tdma_g2g_tensor_copy,
  .tdma_g2g_general_copy = cvkcv180x_tdma_g2g_general_copy,
  .tdma_g2g_bf16_general_copy = cvkcv180x_tdma_g2g_bf16_general_copy,
  .tdma_g2g_bf16_tensor_copy = cvkcv180x_tdma_g2g_bf16_tensor_copy,
  .tiu_mul = cvkcv180x_tiu_mul,
  .tiu_mul_qm = cvkcv180x_tiu_mul_qm,
  .tiu_mac = cvkcv180x_tiu_mac,
  .tiu_add = cvkcv180x_tiu_add,
  .tiu_sub = cvkcv180x_tiu_sub,
  .tiu_max = cvkcv180x_tiu_max,
  .tiu_min = cvkcv180x_tiu_min,
  .tiu_and_int8 = cvkcv180x_tiu_and_int8,
  .tiu_arith_shift = cvkcv180x_tiu_arith_shift,
  .tiu_and_int16 = cvkcv180x_tiu_and_int16,
  .tiu_or_int8 = cvkcv180x_tiu_or_int8,
  .tiu_or_int16 = cvkcv180x_tiu_or_int16,
  .tiu_xor_int8 = cvkcv180x_tiu_xor_int8,
  .tiu_xor_int16 = cvkcv180x_tiu_xor_int16,
  .tiu_copy = cvkcv180x_tiu_copy,
  .tiu_lookup_table = cvkcv180x_tiu_lookup_table,
  .tiu_bf16_lookup_interp_table = cvkcv180x_tiu_bf16_lookup_interp_table,
  .tiu_pt_convolution = cvkcv180x_tiu_pt_convolution,
  .tiu_convolution = cvkcv180x_tiu_convolution,
  .tiu_max_pooling = cvkcv180x_tiu_max_pooling,
  .tiu_average_pooling = cvkcv180x_tiu_average_pooling,
  .tiu_pt_depthwise_convolution = cvkcv180x_tiu_pt_depthwise_convolution,
  .tiu_depthwise_convolution = cvkcv180x_tiu_depthwise_convolution,
  .tiu_matrix_multiplication = cvkcv180x_tiu_matrix_multiplication,
  .tiu_matrix_multiplication_qm = cvkcv180x_tiu_matrix_multiplication_qm,
  .tiu_ge = cvkcv180x_tiu_ge,
  .tiu_min_pooling = cvkcv180x_tiu_min_pooling,
};

static cvk_misc_operations_t cvk_cv180x_misc_ops = {
  .float_to_bfloat16 = cvkcv180x_float_to_bfloat16,
  .bf16_table_shape = cvkcv180x_bf16_table_shape,
};

char *cvikernel_get_chip_info_cv180x(void)
{
  return CVI_TPU_VERSION_180X;
}

void cvikernel_init_cv180x(
    cvk_reg_info_t *req_info,
    cvk_context_t *ctx)
{
  uint32_t max_nr_desc = cvkcv180x_estimate_nr_desc(req_info->cmdbuf_size);
  cvk_prv_data_t *prv_data;
  desc_pair_t *desc_pairs;

  prv_data = malloc(sizeof(cvk_prv_data_t));
  desc_pairs = malloc(max_nr_desc * sizeof(desc_pair_t));
  if (!req_info || !ctx || !prv_data || !desc_pairs) {
    if (prv_data)
      free(prv_data);
    if (desc_pairs)
      free(desc_pairs);
    return;
  }

  ctx->info.version = CV180X_VER;
  ctx->info.node_num = CV180X_HW_NODE_CHIP_NUM;
  ctx->info.node_shift = CV180X_HW_NODE_CHIP_SHIFT;
  ctx->info.npu_num = CV180X_HW_NPU_NUM;
  ctx->info.npu_shift = CV180X_HW_NPU_SHIFT;
  ctx->info.eu_num = CV180X_HW_EU_NUM;
  ctx->info.eu_shift = CV180X_HW_EU_SHIFT;
  ctx->info.lmem_size = CV180X_HW_LMEM_SIZE;
  ctx->info.lmem_shift = CV180X_HW_LMEM_SHIFT;
  ctx->info.lmem_banks = CV180X_HW_LMEM_BANKS;
  ctx->info.lmem_bank_size = CV180X_HW_LMEM_BANK_SIZE;
  ctx->info.gmem_start = CV180X_GLOBAL_MEM_START_ADDR;
  ctx->info.features = CVK_HWF_FC_OP1_CONST | CVK_HWF_8B_ADD_SUB |
                       CVK_HWF_MIN_POOL | CVK_HWF_M_BRADCAST |
                       CVK_HWF_QM_LSHIFT | CVK_HWF_GE | CVK_HWF_CMD_PRE_EXE;
  ctx->info.gmem_size = CV180X_GLOBAL_MEM_SIZE;

  ctx->ops = &cvk_cv180x_ops;
  ctx->misc_ops = &cvk_cv180x_misc_ops;

  prv_data->cmdbuf_ptr = 0;
  prv_data->max_nr_desc = max_nr_desc;
  prv_data->cur_nr_desc = 0;
  prv_data->desc_pairs = desc_pairs;
  prv_data->lmem_ptr = 0;

  if (!prv_data->desc_pairs) {
    printf("cvkcv180x init: fail to allocate internal data\n");
    free(prv_data);
    return;
  }

  ec_init(&prv_data->ec, CV180X_ENGINE_NUM, max_nr_desc);
  mode_manager_init(&prv_data->mode_manager, &prv_data->ec, CV180X_ENGINE_NUM);

  prv_data->cmdbuf = req_info->cmdbuf;
  prv_data->cmdbuf_size = req_info->cmdbuf_size;
  ctx->priv_data = prv_data;
}
