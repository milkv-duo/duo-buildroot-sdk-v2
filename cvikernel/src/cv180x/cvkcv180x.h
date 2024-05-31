#ifndef CVKCV180X_H
#define CVKCV180X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine_conductor.h"
#include "engine_state.h"
#include "mode_manager.h"
#include <cvikernel/cvikernel.h>
#include <cvikernel/cvk_fp_convert.h>
#include "../../include/cvikernel/cv180x/cv180x_tiu_reg.h"
#include "../../include/cvikernel/cv180x/cv180x_tdma_reg.h"
#include "../../include/cvikernel/cv180x/cv180x_tpu_cfg.h"

#define CV180X_TIU        0  // Tensor Instruction Unit
#define CV180X_CPU        1  // CPU, Reserved for common cpu op
#define CV180X_TDMA       2  // TPU DMA
#define CV180X_ENGINE_NUM 3  // Number of Engines

typedef struct __cmd_hdr_s {
  uint8_t magic;              // 0xA5
  uint8_t len;                // lens in bytes
  uint8_t engine_id: 4;       // TPU, TDMA
  uint8_t __deprecated: 4;
  uint8_t flags;              // CMD_ID, sync flags, etc. TBD
  uint32_t mask;              // bit mask for which register need to write
  uint8_t cmd[0];
} __attribute__((packed)) cmd_hdr_t;

typedef struct {
  cmd_hdr_t *cmd_hdr;
  ec_desc_t *ec_desc;
} desc_pair_t;

typedef struct cvk_prv_data {
  ec_t ec;
  mode_manager_t mode_manager;

  uint32_t cmdbuf_ptr;
  uint32_t max_nr_desc;
  uint32_t cur_nr_desc;
  desc_pair_t *desc_pairs;

  uint32_t lmem_ptr;
  uint16_t layer_id;

  uint32_t cmdbuf_size;
  uint8_t *cmdbuf;
} cvk_prv_data_t;

desc_pair_t *cvkcv180x_get_desc_pair(cvk_context_t *ctx, uint8_t eng_id);

#define CHECK(_status, _cond)       \
  do {                              \
    (_status) |= (_cond) ? 0 : -1;  \
  } while (0)

static inline int ceiling_func(int numerator, int denominator)
{
  return (numerator + denominator - 1) / denominator;
}

static inline int ceiling_func_shift(int numerator, int shift)
{
  return (numerator + (1 << shift) - 1) >> shift;
}

static inline uint64_t align_up(uint64_t x, uint64_t n)
{
  return (x + n - 1) / n * n;
}

static inline int8_t check_same_stride(const cvk_tl_t *a, const cvk_tl_t *b)
{
  int8_t status = 0;

  CHECK(status, a->stride.n == b->stride.n);
  CHECK(status, a->stride.c == b->stride.c);
  CHECK(status, a->stride.h == b->stride.h);
  CHECK(status, a->stride.w == b->stride.w);

  return status;
}

static inline int8_t check_same_shape(const cvk_tl_t *a, const cvk_tl_t *b)
{
  int8_t status = 0;

  CHECK(status, a->shape.n == b->shape.n);
  CHECK(status, a->shape.c == b->shape.c);
  CHECK(status, a->shape.h == b->shape.h);
  CHECK(status, a->shape.w == b->shape.w);

  return status;
}

static inline int8_t check_same_shape_3(
    const cvk_tl_t *a,
    const cvk_tl_t *b,
    const cvk_tl_t *c)
{
  int8_t status = 0;
  status |= check_same_shape(a, b);
  status |= check_same_shape(a, c);

  return status;
}

static inline int8_t check_same_shape_4(
    const cvk_tl_t *a,
    const cvk_tl_t *b,
    const cvk_tl_t *c,
    const cvk_tl_t *d)
{
  int8_t status = 0;
  status |= check_same_shape_3(a, b, c);
  status |= check_same_shape(a, d);

  return status;
}

static inline int8_t check_same_shape_5(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2,
    const cvk_tl_t *t3,
    const cvk_tl_t *t4)
{
  int8_t status = 0;
  status |= check_same_shape_3(t0, t1, t2);
  status |= check_same_shape_3(t0, t3, t4);

  return status;
}

static inline int8_t check_same_shape_6(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2,
    const cvk_tl_t *t3,
    const cvk_tl_t *t4,
    const cvk_tl_t *t5)
{
  int8_t status = 0;
  status |= check_same_shape_5(t0, t1, t2, t3, t4);
  status |=check_same_shape(t0, t5);

  return status;
}


static inline int8_t check_tiu_tensor_shape(const cvk_tl_t *t)
{
  int8_t status = 0;
  CHECK(status, t->shape.n > 0);
  CHECK(status, t->shape.c > 0);
  CHECK(status, t->shape.h > 0);
  CHECK(status, t->shape.w > 0);

  CHECK(status, t->shape.n < 0x1000);
  CHECK(status, t->shape.c < 0x1000);
  CHECK(status, t->shape.h <= (4095-32)); // 12bit, max 4095-32(lanes)
  CHECK(status, t->shape.w <= (4095-32)); // 12bit, max 4095-32(lanes)

  return status;
}

static inline int8_t check_tiu_tensor(const cvk_tl_t *t)
{
  int8_t status = 0;

  if (!t)
    return -1;

  status |= check_tiu_tensor_shape(t);
  CHECK(status, t->fmt == CVK_FMT_I8 || t->fmt == CVK_FMT_U8 || t->fmt == CVK_FMT_BF16);

  return status;
}

static inline int8_t check_tiu_tensor_2(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1)
{
  int8_t status = 0;
  status |= check_tiu_tensor(t0);
  status |= check_tiu_tensor(t1);

  return status;
}

static inline int8_t check_tiu_tensor_3(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2)
{
  int8_t status = 0;
  status |= check_tiu_tensor(t0);
  status |= check_tiu_tensor_2(t1, t2);

  return status;
}

static inline int8_t check_tiu_tensor_4(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2,
    const cvk_tl_t *t3)
{
  int8_t status = 0;
  status |= check_tiu_tensor_3(t0, t1, t2);
  status |= check_tiu_tensor(t3);

  return status;
}

static inline int8_t check_tiu_tensor_5(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2,
    const cvk_tl_t *t3,
    const cvk_tl_t *t4)
{
  int8_t status = 0;
  status |= check_tiu_tensor_3(t0, t1, t2);
  status |= check_tiu_tensor_2(t3, t4);

  return status;
}

static inline int8_t check_tiu_tensor_6(
    const cvk_tl_t *t0,
    const cvk_tl_t *t1,
    const cvk_tl_t *t2,
    const cvk_tl_t *t3,
    const cvk_tl_t *t4,
    const cvk_tl_t *t5)
{
  int8_t status = 0;
  status |= check_tiu_tensor_3(t0, t1, t2);
  status |= check_tiu_tensor_3(t3, t4, t5);

  return status;
}

static inline int8_t check_16bit_tiu_tensor(const cvk_tl_t *low, const cvk_tl_t *high)
{
  int8_t status = 0;

  status |= check_tiu_tensor_2(low, high);
  status |= check_same_shape(low, high);
  status |= check_same_stride(low, high);
  CHECK(status, low->fmt == high->fmt);
  CHECK(status, low->start_address < high->start_address);

  return status;
}

static inline int8_t check_stride_type_0(cvk_context_t *ctx, const cvk_tl_t *t)
{
  int8_t status = 0;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t fmt = (t->fmt == CVK_FMT_BF16) ? 2 : 1;

  uint32_t h = t->shape.h;
  uint32_t w = t->shape.w * fmt;
  uint32_t c_stride = align_up(h * w, eu_num);

  CHECK(status, t->stride.c == c_stride);
  CHECK(status, t->stride.h == w);
  CHECK(status, t->stride.w == fmt);

  return status;
}

static inline int8_t check_bf16_stride_type_0(cvk_context_t *ctx, const cvk_tl_t *t)
{
  int8_t status = 0;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t fmt = (t->fmt == CVK_FMT_BF16) ? 2 : 1;

  CHECK(status, t->stride.c % eu_num == 0);
  CHECK(status, t->stride.w == fmt);

  return status;
}

static inline int8_t check_stride_type_2(cvk_context_t *ctx, const cvk_tl_t *t)
{
  int8_t status = 0;

  CHECK(status, t->shape.h == 1);
  CHECK(status, t->shape.w == 1);

  uint32_t fmt = (t->fmt == CVK_FMT_BF16) ? 2 : 1;
  uint32_t c = t->shape.c;
  uint32_t npu_num = ctx->info.npu_num;

  CHECK(status, t->stride.n == fmt * align_up(c, npu_num) / npu_num);
  CHECK(status, t->stride.c == 1 * fmt);
  CHECK(status, t->stride.h == 1 * fmt);
  CHECK(status, t->stride.w == 1 * fmt);

  return status;
}

static inline int8_t check_bf16_stride_type_2(cvk_context_t *ctx, const cvk_tl_t *t)
{
  int8_t status = 0;
  CHECK(status, t->shape.h == 1);
  CHECK(status, t->shape.w == 1);

  uint32_t fmt = (t->fmt == CVK_FMT_BF16) ? 2 : 1;
  uint32_t c = t->shape.c;
  uint32_t npu_num = ctx->info.npu_num;

  CHECK(status, t->stride.n == fmt * align_up(c, npu_num) / npu_num);
  CHECK(status, t->stride.c == 1 * fmt);
  CHECK(status, t->stride.h == 1 * fmt);
  CHECK(status, t->stride.w == 1 * fmt);

  return status;
}

static inline int tensor_is_signed(const cvk_tl_t *t)
{
  switch (t->fmt) {
    case CVK_FMT_I8:
      return 1;
    case CVK_FMT_U8:
    case CVK_FMT_BF16: //does not matter, so set to default 0
      return 0;
    default:
      break;
  }

  return 1;
}

static inline int matrix_is_signed(const cvk_ml_t *t)
{
  switch (t->fmt) {
    case CVK_FMT_I8:
      return 1;
    case CVK_FMT_U8:
    case CVK_FMT_BF16: //does not matter, so set to default 0
      return 0;
    default:
      break;
  }

  return 1;
}

static inline void fill_same_tensor_shape(tiu_reg_t *r, cvk_tl_shape_t s)
{
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = s.w;

  r->opd0_n = n;
  r->opd0_c = c;
  r->opd0_h = h;
  r->opd0_w = w;

  r->opd1_n = n;
  r->opd1_c = c;
  r->opd1_h = h;
  r->opd1_w = w;

  r->opd2_n = n;
  r->opd2_c = c;
  r->opd2_h = h;
  r->opd2_w = w;

  r->res0_n = n;
  r->res0_c = c;
  r->res0_h = h;
  r->res0_w = w;
}

static inline int8_t check_stride_range(cvk_tl_stride_t s)
{
  int8_t status = 0;

  CHECK(status, s.n < 0x10000);
  CHECK(status, s.c < 0x10000);
  CHECK(status, s.h < 0x10000);

  return status;
}

static inline void fill_same_tensor_stride(tiu_reg_t *r, cvk_tl_stride_t s)
{
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = 1;

  r->opd0_n_str = n;
  r->opd0_c_str = c;
  r->opd0_h_str = h;
  r->opd0_w_str = w;

  r->opd1_n_str = n;
  r->opd1_c_str = c;
  r->opd1_h_str = h;
  r->opd1_w_str = w;

  r->opd2_n_str = n;
  r->opd2_c_str = c;
  r->opd2_h_str = h;
  r->opd2_w_str = w;

  r->res0_n_str = n;
  r->res0_c_str = c;
  r->res0_h_str = h;
  r->res0_w_str = w;
}

#define fill_stride_code(r, op, str)            \
  do {                                          \
    r->op##_n_str = str->n;                     \
    r->op##_c_str = str->c;                     \
    r->op##_h_str = str->h;                     \
    r->op##_w_str = str->w;                     \
  } while (0)

static inline void fill_opd0_stride(tiu_reg_t *r, const cvk_tl_stride_t *str)
{
  fill_stride_code(r, opd0, str);
}

static inline void fill_opd1_stride(tiu_reg_t *r, const cvk_tl_stride_t *str)
{
  fill_stride_code(r, opd1, str);
}

static inline void fill_opd2_stride(tiu_reg_t *r, const cvk_tl_stride_t *str)
{
  fill_stride_code(r, opd2, str);
}

static inline void fill_res0_stride(tiu_reg_t *r, const cvk_tl_stride_t *str)
{
  fill_stride_code(r, res0, str);
}

static inline void fill_same_tensor_stride_type(tiu_reg_t *r, int type)
{
  r->short_opd0_str = type & 0b11;
  r->short_opd1_str = type & 0b11;
  r->short_opd2_str = type & 0b11;
  r->short_res0_str = type & 0b11;
}

static inline ec_desc_t * emit_tiu_cmdbuf(cvk_context_t *ctx, tiu_reg_t *r)
{
  int engine_id = CV180X_TIU;

  desc_pair_t *dp = cvkcv180x_get_desc_pair(ctx, engine_id);
  uint32_t *cmdbuf = (uint32_t *)dp->cmd_hdr->cmd;
  emit_tiu_reg(r, cmdbuf);

  return dp->ec_desc;
}

void cvkcv180x_cleanup(struct cvikernel_context *ctx);
void cvkcv180x_reset(struct cvikernel_context *ctx);

void cvkcv180x_parallel_enable(struct cvikernel_context *ctx);
void cvkcv180x_parallel_disable(struct cvikernel_context *ctx);
void cvkcv180x_set_layer_id(
    struct cvikernel_context *ctx,
    uint16_t layer_id);
cvk_tl_t *cvkcv180x_lmem_alloc_tensor(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
cvk_ml_t *cvkcv180x_lmem_alloc_matrix(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
cvk_ml_t *cvkcv180x_lmem_alloc_ps32_matrix(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
void cvkcv180x_lmem_free_tensor(
    struct cvikernel_context *ctx,
    const cvk_tl_t *tl);
void cvkcv180x_lmem_free_matrix(
    struct cvikernel_context *ctx,
    const cvk_ml_t *ml);
void cvkcv180x_lmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tl_t *tl,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
void cvkcv180x_lmem_init_matrix(
    struct cvikernel_context *ctx,
    cvk_ml_t *ml,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
cvk_tl_stride_t cvkcv180x_tl_default_stride(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
cvk_tg_stride_t cvkcv180x_tg_default_stride(
    struct cvikernel_context *ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt);
cvk_ml_shape_t cvkcv180x_ml_default_shape(
    struct cvikernel_context *ctx,
    uint32_t row,
    uint32_t col,
    cvk_fmt_t fmt);
cvk_ml_stride_t cvkcv180x_ml_default_stride(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
cvk_ml_shape_t cvkcv180x_ml_shape_t1(
    struct cvikernel_context *ctx,
    uint32_t len,
    cvk_fmt_t fmt);
uint32_t cvkcv180x_lmem_tensor_to_size(
    struct cvikernel_context *ctx,
    cvk_tl_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
uint32_t cvkcv180x_lmem_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
uint32_t cvkcv180x_lmem_ps32_matrix_to_size(
    struct cvikernel_context *ctx,
    cvk_ml_shape_t shape,
    cvk_fmt_t fmt,
    int eu_align);
void cvkcv180x_gmem_init_tensor(
    struct cvikernel_context *ctx,
    cvk_tg_t *tg,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt);

/* Local to Local DMA API */
void cvkcv180x_tdma_l2l_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2l_tensor_copy_param_t *param);
void cvkcv180x_tdma_l2l_bf16_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2l_tensor_copy_param_t *param);
void cvkcv180x_tdma_l2l_tensor_lrn_shift(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2l_tensor_lrn_shift_param_t *param);

/* Local to Global DMA API */
void cvkcv180x_tdma_l2g_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_param_t *param);
void cvkcv180x_tdma_l2g_bf16_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_param_t *param);
void cvkcv180x_tdma_l2g_tensor_copy_nc_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_nc_transposed_param_t *param);
void cvkcv180x_tdma_l2g_bf16_tensor_copy_nc_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_nc_transposed_param_t *param);
void cvkcv180x_tdma_l2g_tensor_copy_compressed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_compressed_param_t *param);
void cvkcv180x_tdma_l2g_tensor_fill_constant(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_fill_constant_param_t *param);
void cvkcv180x_tdma_l2g_tensor_copy_cw_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_cw_transposed_param_t *param);
void cvkcv180x_tdma_l2g_bf16_tensor_copy_cw_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_tensor_copy_cw_transposed_param_t *param);
void cvkcv180x_tdma_l2g_matrix_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_matrix_copy_param_t *param);
void cvkcv180x_tdma_l2g_bf16_matrix_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_matrix_copy_param_t *param);
void cvkcv180x_tdma_l2g_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_general_copy_param_t *param);
void cvkcv180x_tdma_l2g_bf16_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_bf16_general_copy_param_t *param);

/* Global to Local DMA API */
void cvkcv180x_tdma_g2l_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_param_t *param);
void cvkcv180x_tdma_g2l_bf16_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_param_t *param);
void cvkcv180x_tdma_g2l_tensor_copy_nc_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_nc_transposed_param_t *param);
void cvkcv180x_tdma_g2l_bf16_tensor_copy_nc_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_nc_transposed_param_t *param);
void cvkcv180x_tdma_g2l_tensor_copy_chw_rotated(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_chw_rotated_param_t *param);
void cvkcv180x_tdma_g2l_tensor_copy_decompressed(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_copy_decompressed_param_t *param);
void cvkcv180x_tdma_g2l_tensor_fill_constant(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_fill_constant_param_t *param);
void cvkcv180x_tdma_g2l_bf16_tensor_fill_constant(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_tensor_fill_constant_param_t *param);
void cvkcv180x_tdma_g2l_matrix_copy_decompressed(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_matrix_copy_decompressed_param_t *param);
void cvkcv180x_tdma_l2g_matrix_copy_compressed(
    struct cvikernel_context *ctx,
    const cvk_tdma_l2g_matrix_copy_compressed_param_t *param);
void cvkcv180x_tdma_g2l_matrix_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_matrix_copy_param_t *param);
void cvkcv180x_tdma_g2l_bf16_matrix_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_matrix_copy_param_t *param);
void cvkcv180x_tdma_g2l_matrix_copy_row_col_transposed(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_matrix_copy_row_col_transposed_param_t *param);
void cvkcv180x_tdma_g2l_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_general_copy_param_t *param);
void cvkcv180x_tdma_g2l_bf16_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2l_bf16_general_copy_param_t *param);

/* Global to Global DMA API */
void cvkcv180x_tdma_g2g_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param);
void cvkcv180x_tdma_g2g_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param);
void cvkcv180x_tdma_g2g_bf16_general_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param);
void cvkcv180x_tdma_g2g_bf16_tensor_copy(
    struct cvikernel_context *ctx,
    const cvk_tdma_g2g_tensor_copy_param_t *param);

/* TIU API */
void cvkcv180x_tiu_mul(
    struct cvikernel_context *ctx,
    const cvk_tiu_mul_param_t *param);
void cvkcv180x_tiu_mul_qm(
    struct cvikernel_context *ctx,
    const cvk_tiu_mul_qm_param_t *param);
void cvkcv180x_tiu_mac(
    struct cvikernel_context *ctx,
    const cvk_tiu_mac_param_t *param);
void cvkcv180x_tiu_add(
    struct cvikernel_context *ctx,
    const cvk_tiu_add_param_t *param);
void cvkcv180x_tiu_sub(
    struct cvikernel_context *ctx,
    const cvk_tiu_sub_param_t *param);
void cvkcv180x_tiu_max(
    struct cvikernel_context *ctx,
    const cvk_tiu_max_param_t *param);
void cvkcv180x_tiu_min(
    struct cvikernel_context *ctx,
    const cvk_tiu_min_param_t *param);
void cvkcv180x_tiu_and_int8(
    struct cvikernel_context *ctx,
    const cvk_tiu_and_int8_param_t *param);
void cvkcv180x_tiu_arith_shift(
    struct cvikernel_context *ctx,
    const cvk_tiu_arith_shift_param_t *param);
void cvkcv180x_tiu_and_int16(
    struct cvikernel_context *ctx,
    const cvk_tiu_and_int16_param_t *param);
void cvkcv180x_tiu_or_int8(
    struct cvikernel_context *ctx,
    const cvk_tiu_or_int8_param_t *param);
void cvkcv180x_tiu_or_int16(
    struct cvikernel_context *ctx,
    const cvk_tiu_or_int16_param_t *param);
void cvkcv180x_tiu_xor_int8(
    struct cvikernel_context *ctx,
    const cvk_tiu_xor_int8_param_t *param);
void cvkcv180x_tiu_xor_int16(
    struct cvikernel_context *ctx,
    const cvk_tiu_xor_int16_param_t *param);
void cvkcv180x_tiu_copy(
    struct cvikernel_context *ctx,
    const cvk_tiu_copy_param_t *param);
void cvkcv180x_tiu_lookup_table(
    struct cvikernel_context *ctx,
    const cvk_tiu_lookup_table_param_t *param);
void cvkcv180x_tiu_bf16_lookup_interp_table(
    struct cvikernel_context *ctx,
    const cvk_tiu_bf16_lookup_interp_table_param_t *param);
void cvkcv180x_tiu_pt_convolution(
    struct cvikernel_context *ctx,
    const cvk_tiu_pt_convolution_param_t *param);
void cvkcv180x_tiu_convolution(
    struct cvikernel_context *ctx,
    const cvk_tiu_convolution_param_t *param);
void cvkcv180x_tiu_max_pooling(
    struct cvikernel_context *ctx,
    const cvk_tiu_max_pooling_param_t *param);
void cvkcv180x_tiu_average_pooling(
    struct cvikernel_context *ctx,
    const cvk_tiu_average_pooling_param_t *param);
void cvkcv180x_tiu_pt_depthwise_convolution(
    struct cvikernel_context *ctx,
    const cvk_tiu_depthwise_pt_convolution_param_t *param);
void cvkcv180x_tiu_depthwise_convolution(
    struct cvikernel_context *ctx,
    const cvk_tiu_depthwise_convolution_param_t *param);
void cvkcv180x_tiu_matrix_multiplication(
    struct cvikernel_context *ctx,
    const cvk_tiu_matrix_multiplication_param_t *param);
void cvkcv180x_tiu_matrix_multiplication_qm(
    struct cvikernel_context *ctx,
    const cvk_tiu_matrix_multiplication_qm_param_t *param);
void cvkcv180x_tiu_ge(
    cvk_context_t *ctx,
    const cvk_tiu_ge_param_t *p);
void cvkcv180x_tiu_min_pooling(
    cvk_context_t *ctx,
    const cvk_tiu_min_pooling_param_t *p);

#ifdef __cplusplus
}
#endif

#endif /* CVKCV180X_H */
