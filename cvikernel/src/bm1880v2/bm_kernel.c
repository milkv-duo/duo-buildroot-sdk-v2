#include "kernel_1880v2.h"
#include <bmkernel/bm1880v2/bm1880v2_tpu_cfg.h>

static void replace_cmd_id(uint32_t *desc, uint32_t eng_id, uint16_t ids[])
{
  if (eng_id == BMK1880v2_TIU) {
    tiu_reg_t reg;
    parse_tiu_reg(&reg, desc);
    reg.cmd_id_en = 1;
    reg.cmd_id_tpu = ids[eng_id];
    reg.cmd_id_gdma = ids[BMK1880v2_TDMA];
    emit_tiu_reg(&reg, desc);

    // printf("    %s: TIU eng_id %d, [wait_tdma_id=%d|tiu_id=%d] dst shape(%d, %d, %d, %d)\n",
    //        __FUNCTION__, eng_id, reg.cmd_id_gdma, reg.cmd_id_tpu,
    //        reg.res0_n, reg.res0_c, reg.res0_h, reg.res0_w);

  } else if (eng_id == BMK1880v2_TDMA) {
    tdma_reg_t tdma_reg;
    parse_tdma_reg(&tdma_reg, desc);
    tdma_reg.cmd_id = ids[eng_id];
    tdma_reg.wait_id_tpu = ids[BMK1880v2_TIU];
    tdma_reg.bar_en = 1;

    // printf("    %s: TDMA eng_id %d, [tdma_id=%d|wait_tiu_id=%d], dst shape(%d, %d, %d, %d)\n",
    //        __FUNCTION__, eng_id, tdma_reg.cmd_id, tdma_reg.wait_id_tpu,
    //        tdma_reg.src_n, tdma_reg.dst_c, tdma_reg.dst_h, tdma_reg.dst_w);

    emit_tdma_reg(&tdma_reg, desc);
  }
}

static int bm1880v2_get_engine_desc_length(uint32_t engine_id)
{
  switch (engine_id) {
    case BMK1880v2_TIU:
      return TIU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    case BMK1880v2_TDMA:
      return TDMA_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    case BMK1880v2_CPU:
      return CPU_ENGINE_DESCRIPTOR_NUM * sizeof(uint32_t);
    default:
      ASSERT(0);
  }
}

// Estimate the number of command descriptor based on buffer size provided
// by the user.
uint32_t bmk1880v2_estimate_nr_desc(ctx_t *k)
{
  uint32_t tiu_desc_len = bm1880v2_get_engine_desc_length(BMK1880v2_TIU);
  uint32_t tdma_desc_len = bm1880v2_get_engine_desc_length(BMK1880v2_TDMA);
  uint32_t hdr_len = sizeof(cmd_hdr_t);

  uint32_t desc_len =
      (tiu_desc_len > tdma_desc_len) ? tiu_desc_len : tdma_desc_len;

  return k->info.cmdbuf_size / (desc_len + hdr_len);
}

static void kernel_init(ctx_t *k, bmk_info_t *info)
{
  k->info = *info;
  //1880v2->18802
  ASSERT(info->chip_version == BM1880V2_VER);
  k->chip_info = bmk1880v2_chip_info();

  uint32_t max_nr_desc = bmk1880v2_estimate_nr_desc(k);
  ec_init(&k->ec, BMK1880v2_ENGINE_NUM, max_nr_desc);
  mode_manager_init(&k->mode_manager, &k->ec, BMK1880v2_ENGINE_NUM);

  k->cmdbuf_ptr = 0;
  k->max_nr_desc = max_nr_desc;
  k->cur_nr_desc = 0;
  k->desc_pairs = xmalloc(max_nr_desc * sizeof(k->desc_pairs[0]));

  k->lmem_ptr = 0;
}

static void kernel_destroy(ctx_t *k)
{
  free(k->desc_pairs);
  ec_destroy(&k->ec);
  mode_manager_destroy(&k->mode_manager);
}

static void kernel_reset(ctx_t *k)
{
  k->cur_nr_desc = 0;
  k->cmdbuf_ptr = 0;

  ec_reset(&k->ec);
  mode_manager_reset(&k->mode_manager);
}

static cmd_hdr_t * kernel_alloc_cmd_hdr(
    ctx_t *k, uint8_t eng_id, uint32_t desc_len)
{
  uint32_t free_len = k->info.cmdbuf_size - k->cmdbuf_ptr;
  uint32_t hdr_len = sizeof(cmd_hdr_t);
  uint32_t total_len = hdr_len + desc_len;
  ASSERT(total_len <= free_len);

  cmd_hdr_t *hdr = (cmd_hdr_t *)&k->info.cmdbuf[k->cmdbuf_ptr];
  hdr->magic = CMDBUF_HDR_MAGIC_1880v2;
  hdr->len = desc_len;
  hdr->engine_id = eng_id;
  hdr->__deprecated = 0;  // for valgrind
  hdr->flags = 0;
  hdr->mask = 0;

  k->cmdbuf_ptr += total_len;
  return hdr;
}

static desc_pair_t * kernel_alloc_desc_pair(ctx_t *k, uint8_t eng_id)
{
  ASSERT(eng_id < BMK1880v2_ENGINE_NUM);
  ASSERT(k->cur_nr_desc < k->max_nr_desc);

  uint32_t desc_len = bm1880v2_get_engine_desc_length(eng_id);
  desc_pair_t *dp = &k->desc_pairs[k->cur_nr_desc++];
  dp->cmd_hdr = kernel_alloc_cmd_hdr(k, eng_id, desc_len);
  dp->ec_desc = ec_alloc_desc(&k->ec, eng_id);

  mode_manager_record_ec_desc(&k->mode_manager, dp->ec_desc);
  return dp;
}

static void kernel_update_sync_id(ctx_t *k)
{
  ec_compute_sync_ids(&k->ec);

  for (uint32_t di = 0; di < k->cur_nr_desc; di++) {
    desc_pair_t *dp = &k->desc_pairs[di];
    uint8_t eng_id = dp->ec_desc->engine_id;
    uint32_t *desc = (uint32_t *)dp->cmd_hdr->cmd;
    replace_cmd_id(desc, eng_id, dp->ec_desc->sync_ids);
  }
}

void bmk1880v2_add_dependency(
    ctx_t *ctx,
    bmk1880v2_op_t *before,
    bmk1880v2_op_t *after)
{
  ec_add_dependency(&ctx->ec, before, after);
}

desc_pair_t * bm1880v2_get_desc_pair(ctx_t *k, uint8_t eng_id)
{
  if (eng_id == BMK1880v2_CPU) {
    kernel_update_sync_id(k);
    k->cur_nr_desc = 0;

    ec_reset(&k->ec);
    mode_manager_restart_sync_id(&k->mode_manager);
  }

  return kernel_alloc_desc_pair(k, eng_id);
}

ctx_t * bmk1880v2_register(bmk_info_t *info)
{
  ASSERT(info);
  ASSERT(info->cmdbuf);
  ASSERT(info->cmdbuf_size > 0);
  ctx_t *k = xmalloc(sizeof(*k));
  kernel_init(k, info);
  return k;
}

void bmk1880v2_cleanup(ctx_t *ctx)
{
  ASSERT(ctx);

  ctx_t *k = (typeof(k))ctx;

  kernel_destroy(k);
  free(k);
}

void bmk1880v2_reset(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  kernel_reset(k);
}

uint8_t *bmk1880v2_acquire_cmdbuf(ctx_t *ctx, uint32_t *size)
{
  ctx_t *k = (typeof(k))ctx;

  *size = k->cmdbuf_ptr;
  kernel_update_sync_id(k);
  return k->info.cmdbuf;
}

void bmk1880v2_parallel_enable(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  mode_manager_enable_parallel(&k->mode_manager);
}

void bmk1880v2_set_op(ctx_t *ctx, void* op)
{
  ctx_t *k = (typeof(k))ctx;
  k->op = op;
}

void* bmk1880v2_get_op(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  return k->op;
}

void bmk1880v2_parallel_disable(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  mode_manager_disable_parallel(&k->mode_manager);
}

void bmk1880v2_create_streams(ctx_t *ctx, int nr_streams)
{
  ctx_t *k = (typeof(k))ctx;
  mode_manager_create_streams(&k->mode_manager, nr_streams);
}

void bmk1880v2_set_layer_id(ctx_t *ctx, uint16_t layer_id)
{
  ctx_t *k = (typeof(k))ctx;
  k->layer_id = layer_id;
}

uint16_t bmk1880v2_layer_id(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  return k->layer_id;
}

void bmk1880v2_destroy_streams(ctx_t *ctx)
{
  ctx_t *k = (typeof(k))ctx;
  mode_manager_destroy_streams(&k->mode_manager);
}

void bmk1880v2_set_stream(ctx_t *ctx, int i)
{
  ctx_t *k = (typeof(k))ctx;
  mode_manager_set_stream(&k->mode_manager, i);
}

static bmk1880v2_chip_info_t bm1880v2_chip_info = {
  .version = BM1880V2_VER,
  .npu_num = BM1880V2_HW_NPU_NUM,
  .eu_num = BM1880V2_HW_EU_NUM,
  .lmem_size = BM1880V2_HW_LMEM_SIZE,
  .lmem_banks = BM1880V2_HW_LMEM_BANKS,
  .lmem_bank_size = BM1880V2_HW_LMEM_BANK_SIZE,
  .gmem_start  = BM1880V2_GLOBAL_MEM_START_ADDR,
  .gmem_size   = BM1880V2_GLOBAL_MEM_SIZE,
};

bmk1880v2_chip_info_t bmk1880v2_chip_info(void)
{
  return bm1880v2_chip_info;
}

bmk1880v2_tensor_lmem_t * bmk1880v2_lmem_alloc_tensor(
    ctx_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt, int eu_align)
{
  ctx_t *k = (typeof(k))ctx;
  uint32_t lmem_size = k->chip_info.lmem_size;
  uint32_t eu_num = k->chip_info.eu_num;

  bmk1880v2_tensor_lmem_t *t = xmalloc(sizeof(*t));
  memset(t, 0, sizeof(*t));
  t->start_address = k->lmem_ptr;
  t->fmt = fmt;
  t->cmprs_fmt = fmt;
  t->shape = s;
  t->eu_align = eu_align;
  t->stride = bmk1880v2_tensor_lmem_default_stride(ctx, s, fmt, eu_align);

  uint32_t needed = align_up(t->shape.n * t->stride.n, eu_num);
  if ((lmem_size - k->lmem_ptr < needed) || !needed) {
    free(t);
    return NULL;
  }

  k->lmem_ptr += needed;
  return t;
}

void bmk1880v2_lmem_init_tensor(
    ctx_t *ctx,
    bmk1880v2_tensor_lmem_t *tl,
    bmk1880v2_tensor_lmem_shape_t shape,
    fmt_t fmt,
    int eu_align)
{
  memset(tl, 0, sizeof(*tl));
  tl->fmt = fmt;
  tl->shape = shape;
  tl->stride = bmk1880v2_tensor_lmem_default_stride(ctx, shape, fmt, eu_align);
  tl->eu_align = eu_align;
}

// Provide the unified api for tensor size calculation.
//   Must have the same logic as bmk1880v2_lmem_bf16_alloc_tensor.
//   The backed does not need to duplicate the related code.
uint32_t bmk1880v2_lmem_tensor_to_size(
    ctx_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt, int eu_align)
{
  ctx_t *k = (typeof(k))ctx;
  uint32_t eu_num = k->chip_info.eu_num;

  bmk1880v2_tensor_lmem_stride_t stride;
  stride = bmk1880v2_tensor_lmem_default_stride(ctx, s, fmt, eu_align);

  uint32_t needed = align_up(s.n * stride.n, eu_num);

  return needed;
}

bmk1880v2_tensor_lmem_t * bmk1880v2_lmem_alloc_ps32_tensor(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt,
    int eu_align)
{
  /* Partial sum is located in lmem in 32-bit format, so we times n to 2 to
   * spare a sapce for it.
   */

  uint32_t prev_n;

  prev_n = s.n;
  s.n = s.n * (bitsize_of_fmt(FMT_I32) / bitsize_of_fmt(fmt));
  bmk1880v2_tensor_lmem_t *res = bmk1880v2_lmem_alloc_tensor(ctx, s, fmt, eu_align);
  if(res == NULL)
    ASSERT(0);
  res->shape.n = prev_n;
  return res;
}

void bmk1880v2_lmem_free_tensor(
    ctx_t *ctx, const bmk1880v2_tensor_lmem_t *t)
{
  ASSERT(t->start_address < ctx->lmem_ptr);
  ctx->lmem_ptr = t->start_address;

  free((void *)t);
}

bmk1880v2_matrix_lmem_t * bmk1880v2_lmem_alloc_matrix(
    ctx_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align)
{
  uint32_t lmem_size = ctx->chip_info.lmem_size;
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t val = (fmt == FMT_BF16) ? 2 : 1;

  bmk1880v2_matrix_lmem_t *t = xmalloc(sizeof(*t));
  memset(t, 0, sizeof(*t));
  t->start_address = ctx->lmem_ptr;
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
  if (lmem_size - ctx->lmem_ptr < needed) {
    free(t);
    return NULL;
  }
  ctx->lmem_ptr += needed;
  return t;
}

void bmk1880v2_lmem_init_matrix(
    ctx_t *ctx,
    bmk1880v2_matrix_lmem_t *ml,
    bmk1880v2_matrix_lmem_shape_t shape,
    fmt_t fmt,
    int eu_align)
{
  memset(ml, 0, sizeof(*ml));
  ml->fmt = fmt;
  ml->shape = shape;
  ml->stride = bmk1880v2_matrix_lmem_default_stride(ctx, shape, fmt, eu_align);
  ml->eu_align = eu_align;
}

// Provide the unified api for matrix size calculation.
//   Must have the same logic as bmk1880v2_lmem_alloc_matrix.
//   The backed does not need to duplicate the related code.
uint32_t bmk1880v2_lmem_matrix_to_size(
    ctx_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align) {
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t val = (fmt == FMT_BF16) ? 2 : 1;

  bmk1880v2_matrix_lmem_t t;
  t.fmt = fmt;
  t.shape = s;
  t.stride.h = s.w * val;
  if (eu_align)
    t.stride.c = align_up(s.w * val, eu_num);
  else
    t.stride.c = s.w * val;
  t.stride.n = t.stride.c * ceiling_func(s.c, npu_num);

  uint32_t needed = align_up(t.shape.n * t.stride.n, eu_num);

  return needed;
}

bmk1880v2_matrix_lmem_t * bmk1880v2_lmem_alloc_ps32_matrix(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align)
{
  /* Partial sum is located in lmem in 32-bit format, so we times n to 4 to
   * spare a sapce for it.
   */

  uint32_t prev_n;

  prev_n = s.n;
  s.n = s.n * (bitsize_of_fmt(FMT_I32) / bitsize_of_fmt(fmt));
  bmk1880v2_matrix_lmem_t *res = bmk1880v2_lmem_alloc_matrix(ctx, s, fmt, eu_align);
  if(res == NULL)
    ASSERT(0);
  res->shape.n = prev_n;
  return res;
}

// Provide the unified api for matrix size calculation.
//   Must have the same logic as bmk1880v2_lmem_alloc_ps32_bf16_matrix.
//   The backed does not need to duplicate the related code.
uint32_t bmk1880v2_lmem_ps32_matrix_to_size(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align)
{
  /* Partial sum is located in lmem in 32-bit format, so we times n to 4 to
   * spare a sapce for it.
   */

  s.n = s.n * (bitsize_of_fmt(FMT_I32) / bitsize_of_fmt(fmt));

  return bmk1880v2_lmem_matrix_to_size(ctx, s, fmt, eu_align);
}

void bmk1880v2_lmem_free_matrix(
    ctx_t *ctx, const bmk1880v2_matrix_lmem_t *t)
{
  ASSERT(t->start_address < ctx->lmem_ptr);
  ctx->lmem_ptr = t->start_address;
  free((void *)t);
}

bmk1880v2_tensor_lmem_stride_t bmk1880v2_tensor_lmem_default_stride(
    ctx_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt_type,
    int eu_align)
{
  bmk1880v2_tensor_lmem_stride_t stride;
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t fmt = (fmt_type == FMT_BF16) ? 2 : 1;
  stride.w = fmt;
  stride.h = s.w * fmt;
  if (eu_align)
    stride.c = align_up(s.h * s.w * fmt, eu_num);
  else
    stride.c = s.h * s.w * fmt;

  stride.n = stride.c * ceiling_func(s.c, npu_num);
//  printf("bmk1880v2_tensor_lmem_default_stride stride n=%x c=%x h=%x w=%x\n", stride.n , stride.c , stride.h, stride.w);
  return stride;
}

bmk1880v2_tensor_tgmem_stride_t bmk1880v2_tensor_tgmem_default_stride(
    bmk1880v2_tensor_tgmem_shape_t s, fmt_t fmt)
{
  uint32_t data_type_size = (fmt == FMT_BF16) ? 2 : 1;
  bmk1880v2_tensor_tgmem_stride_t stride;
  stride.h = s.w * data_type_size;
  stride.c = s.h * stride.h;
  stride.n = s.c * stride.c;
  return stride;
}

static void try_optimize_matrix_shape(ctx_t *ctx,
                                      bmk1880v2_matrix_lmem_shape_t *s,
                                      fmt_t fmt_type) {
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t col = s->col;
  bool isBf16 = (fmt_type == FMT_BF16);
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

bmk1880v2_matrix_lmem_shape_t bmk1880v2_matrix_lmem_default_shape(
    ctx_t *ctx,
    uint32_t row,
    uint32_t col,
    fmt_t fmt_type)
{
  bmk1880v2_matrix_lmem_shape_t s = {0};
  s.n = row;
  s.col = col;

  try_optimize_matrix_shape(ctx, &s, fmt_type);

  return s;
}

bmk1880v2_matrix_lmem_shape_t bmk1880v2_matrix_lmem_shape_t1(
    ctx_t *ctx,
    uint32_t len,
    fmt_t fmt_type)
{
  uint32_t lmem_size = ctx->chip_info.lmem_size;
  bmk1880v2_matrix_lmem_shape_t s = {0};

  uint32_t row = 1;
  uint32_t col = len;

  while (col >= lmem_size) {
    ASSERT(col % 2 == 0);
    col /= 2;
    row *= 2;
  }

  s.n = row;
  s.col = col;

  try_optimize_matrix_shape(ctx, &s, fmt_type);
  return s;
}

// This should be inside bmk1880v2_lmem_alloc_matrix
bmk1880v2_matrix_lmem_stride_t bmk1880v2_matrix_lmem_default_stride(
    ctx_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align)
{
  uint32_t npu_num = ctx->chip_info.npu_num;
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t val = (fmt == FMT_BF16) ? 2 : 1;

  bmk1880v2_matrix_lmem_stride_t stride;
  stride.h = s.w * val;
  if (eu_align)
    stride.c = align_up(s.w * val, eu_num);
  else
    stride.c = s.w * val;
  stride.n = stride.c * ceiling_func(s.c, npu_num);

  return stride;
}
