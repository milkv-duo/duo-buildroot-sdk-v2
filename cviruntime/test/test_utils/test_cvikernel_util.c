#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "test_cvikernel_util.h"

// Fail:
//   test_1810_tdma_bf16_matrix_vlc_decompress_compress
//   test_1810_tdma_bf16_tensor_vlc_decompress_compress
//   test_1810_tdma_g2l_bf16_matrix_vlc_copy_decompressed
//   test_1810_tdma_g2l_bf16_tensor_vlc_copy_decompressed
//   test_1810_tdma_l2g_bf16_matrix_vlc_copy_compressed
//   test_1810_tdma_l2g_bf16_tensor_vlc_copy_compressed
//#define ENABEL_GAUSSIANRANDOM_VLC_TEST

#define RAND_SEED_MOD 10

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct {
  cvk_tg_t tg;
  CVI_RT_MEM mem;
} test_tg_wrapper_t;

typedef struct {
  cvk_cmpr_tg_t cmpr_tg;
  CVI_RT_MEM mem;
} test_cmpr_tg_wrapper_t;

typedef struct {
  cvk_mg_t mg;
  CVI_RT_MEM mem;
} test_mg_wrapper_t;

typedef struct {
  cvk_cmpr_mg_t cmpr_mg;
  CVI_RT_MEM mem;
} test_cmpr_mg_wrapper_t;

#define CHECK(_cond)           assert((_cond))
#define CHECK_LE(a, b)         CHECK((a) <= (b))
#define CHECK_GT(a, b)         CHECK((a) > (b))

cvk_tg_t *alloc_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt)
{
  test_tg_wrapper_t *w = malloc(sizeof(*w));
  if (!w)
    return NULL;

  w->mem = CVI_RT_MemAlloc(rt_handle, tg_shape_size(&shape, fmt));
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &w->tg, shape, fmt);
  w->tg.start_address = CVI_RT_MemGetPAddr(w->mem);

  return &w->tg;
}

void free_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_tg_t *tg)
{
  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  CVI_RT_MemFree(rt_handle, w->mem);

  free(w);
}

cvk_cmpr_tg_t *alloc_cmpr_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt,
    CommandInfo *cmd_info)
{
  if (fmt != CVK_FMT_I8 && fmt != CVK_FMT_U8 && fmt != CVK_FMT_BF16)
    return NULL;

  test_cmpr_tg_wrapper_t *w = malloc(sizeof(*w));
  if (!w)
    return NULL;

  size_t bs_buf_size = cmpr_tg_shape_size(&shape, fmt);
  w->mem = CVI_RT_MemAlloc(rt_handle, bs_buf_size);

  memset(&w->cmpr_tg, 0, sizeof(w->cmpr_tg));
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &w->cmpr_tg.t, shape, fmt);
  w->cmpr_tg.t.start_address = CVI_RT_MemGetPAddr(w->mem);
  w->cmpr_tg.reserved_size = bs_buf_size;

  if (cmd_info) {
    w->cmpr_tg.bias0 = cmd_info->bias0;
    w->cmpr_tg.bias1 = cmd_info->bias1;
    w->cmpr_tg.zero_guard_en = cmd_info->zero_guard_en;
  }
  else {
    if (fmt == CVK_FMT_BF16) {
      w->cmpr_tg.bias0 = 127;
    }
    else if (fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8) {
      w->cmpr_tg.bias0 = 0;
    }

    w->cmpr_tg.bias1 = 0;
    w->cmpr_tg.zero_guard_en = 0;
  }

  return &w->cmpr_tg;
}

void free_cmpr_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg)
{
  test_cmpr_tg_wrapper_t *w = container_of(cmpr_tg, test_cmpr_tg_wrapper_t, cmpr_tg);
  CVI_RT_MemFree(rt_handle, w->mem);

  free(w);
}

CVI_RC tensor_copy_s2d(
  CVI_RT_HANDLE rt_handle,
  const cvk_tg_t *tg,
  uint8_t *data)
{
  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  return CVI_RT_MemCopyS2D(rt_handle, w->mem, data);
}

CVI_RC cmpr_tensor_copy_s2d(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg,
    uint8_t *data)
{
  test_cmpr_tg_wrapper_t *w = container_of(cmpr_tg, test_cmpr_tg_wrapper_t, cmpr_tg);
  return CVI_RT_MemCopyS2D(rt_handle, w->mem, data);
}

uint8_t *tensor_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_tg_t *tg)
{
  uint32_t size = tg_shape_size(&tg->shape, tg->fmt);
  uint8_t *data = (uint8_t *)malloc(size);
  if (!data)
    return NULL;

  test_tg_wrapper_t *w = container_of(tg, test_tg_wrapper_t, tg);
  CVI_RC ret = CVI_RT_MemCopyD2S(rt_handle, data, w->mem);
  if (ret != CVI_SUCCESS) {
    free(data);
    data = NULL;
  }

  return data;
}

uint8_t *cmpr_tensor_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg)
{
  size_t bs_buf_size = cmpr_tg_shape_size(&cmpr_tg->t.shape, cmpr_tg->t.fmt);
  uint8_t *data = (uint8_t *)malloc(bs_buf_size);
  if (!data)
    return NULL;

  test_cmpr_tg_wrapper_t *w = container_of(cmpr_tg, test_cmpr_tg_wrapper_t, cmpr_tg);
  CVI_RC ret = CVI_RT_MemCopyD2S(rt_handle, data, w->mem);
  if (ret != CVI_SUCCESS) {
    free(data);
    data = NULL;
  }

  return data;
}

uint8_t *tensor_copy_l2g_d2s_stride(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_tl_t *tl,
  cvk_tg_stride_t tg_stride)
{
  cvk_tg_shape_t s;
  s.n = tl->shape.n;
  s.c = tl->shape.h;
  s.h = tl->shape.w;
  s.w = tl->shape.c;
  cvk_tg_t *tg = alloc_tensor_dev_mem(rt_handle, cvk_ctx, s, tl->fmt);
  if (!tg)
    return NULL;

  tg->stride = tg_stride;

  cvk_tdma_l2g_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = tg;
  cvk_ctx->ops->tdma_l2g_tensor_copy(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  uint8_t *data = tensor_copy_d2s(rt_handle, tg);
  free_tensor_dev_mem(rt_handle, tg);

  return data;
}

uint8_t *tensor_copy_l2g_d2s(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_tl_t *tl)
{
  cvk_tg_shape_t s;
  s.n = tl->shape.n;
  s.c = tl->shape.h;
  s.h = tl->shape.w;
  s.w = tl->shape.c;
  cvk_tg_stride_t tg_stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, s, tl->fmt);

  uint8_t *data = tensor_copy_l2g_d2s_stride(rt_handle, cvk_ctx, tl, tg_stride);

  return data;
}

void tensor_copy_s2d_g2l_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    cvk_tg_stride_t tg_stride,
    uint8_t *data)
{
  cvk_tg_shape_t tg_shape;
  tg_shape.n = tl->shape.n;
  tg_shape.c = tl->shape.c;
  tg_shape.h = tl->shape.h;
  tg_shape.w = tl->shape.w;

  cvk_tg_t *tg = alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, tl->fmt);
  if (!tg)
    return;

  tg->stride = tg_stride;
  tensor_copy_s2d(rt_handle, tg, data);

  cvk_tdma_g2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tg;
  p.dst = tl;
  cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  free_tensor_dev_mem(rt_handle, tg);
}

void tensor_copy_s2d_g2l(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint8_t *data)
{
  cvk_tg_shape_t tg_shape;
  tg_shape.n = tl->shape.n;
  tg_shape.c = tl->shape.c;
  tg_shape.h = tl->shape.h;
  tg_shape.w = tl->shape.w;

  cvk_tg_stride_t tg_stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, tg_shape, tl->fmt);

  tensor_copy_s2d_g2l_stride(rt_handle, cvk_ctx, tl, tg_stride, data);
}

void tensor_copy_s2d_g2l_tp_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    cvk_tg_stride_t tg_stride,
    uint8_t *data)
{
  cvk_tg_shape_t tg_shape =
      tg_shape_t4(tl->shape.n, tl->shape.c, tl->shape.h, tl->shape.w);

  cvk_tg_t *tg = alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, tl->fmt);
  if (!tg)
    return;

  tg->stride = tg_stride;
  tensor_copy_s2d(rt_handle, tg, data);

  cvk_tdma_g2l_tensor_copy_nc_transposed_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tg;
  p.dst = tl;
  cvk_ctx->ops->tdma_g2l_tensor_copy_nc_transposed(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  free_tensor_dev_mem(rt_handle, tg);
}

void tensor_copy_s2d_g2l_tp(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint8_t *data)
{
  cvk_tg_shape_t tg_shape =
      tg_shape_t4(tl->shape.n, tl->shape.c, tl->shape.h, tl->shape.w);

  cvk_tg_stride_t tg_stride =
      cvk_ctx->ops->tg_default_stride(cvk_ctx, tg_shape, tl->fmt);

  tensor_copy_s2d_g2l_tp_stride(rt_handle, cvk_ctx, tl, tg_stride, data);
}

cvk_mg_t *alloc_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_mg_shape_t shape,
    cvk_fmt_t fmt)
{
  uint32_t w_stride = fmt_size(fmt);
  test_mg_wrapper_t *w = malloc(sizeof(*w));
  if (!w)
    return NULL;

  memset(&w->mg, 0, sizeof(w->mg));

  w->mem = CVI_RT_MemAlloc(rt_handle, mg_shape_size(&shape, fmt));
  w->mg.base_reg_index = 0;
  w->mg.start_address = CVI_RT_MemGetPAddr(w->mem);
  w->mg.fmt = fmt;
  w->mg.shape = shape;
  w->mg.stride.row = shape.col * w_stride;

  return &w->mg;
}

void free_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_mg_t *mg)
{
  test_mg_wrapper_t *w = container_of(mg, test_mg_wrapper_t, mg);
  CVI_RT_MemFree(rt_handle, w->mem);

  free(w);
}

cvk_cmpr_mg_t *alloc_cmpr_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_mg_shape_t shape,
    cvk_fmt_t fmt,
    CommandInfo *cmd_info)
{
  if (fmt != CVK_FMT_I8 && fmt != CVK_FMT_U8 && fmt != CVK_FMT_BF16)
    return NULL;

  uint32_t w_stride = fmt_size(fmt);
  test_cmpr_mg_wrapper_t *w = malloc(sizeof(*w));
  if (!w)
    return NULL;

  size_t bs_buf_size = cmpr_mg_shape_size(&shape, fmt);
  w->mem = CVI_RT_MemAlloc(rt_handle, bs_buf_size);

  memset(&w->cmpr_mg, 0, sizeof(w->cmpr_mg));
  w->cmpr_mg.m.base_reg_index = 0;
  w->cmpr_mg.m.start_address = CVI_RT_MemGetPAddr(w->mem);
  w->cmpr_mg.m.fmt = fmt;
  w->cmpr_mg.m.shape = shape;
  w->cmpr_mg.m.stride.row = shape.col * w_stride;

  if (cmd_info) {
    w->cmpr_mg.bias0 = cmd_info->bias0;
    w->cmpr_mg.bias1 = cmd_info->bias1;
    w->cmpr_mg.zero_guard_en = cmd_info->zero_guard_en;
  }
  else {
    w->cmpr_mg.bias0 = 0;

    if (fmt == CVK_FMT_BF16) {
      w->cmpr_mg.bias0 = 127;
    }
    else if (fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8) {
      w->cmpr_mg.bias0 = 0;
    }

    w->cmpr_mg.bias1 = 0;
    w->cmpr_mg.zero_guard_en = 0;
  }

  return &w->cmpr_mg;
}

void free_cmpr_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_mg_t *cmpr_mg)
{
  test_cmpr_mg_wrapper_t *w = container_of(cmpr_mg, test_cmpr_mg_wrapper_t, cmpr_mg);
  CVI_RT_MemFree(rt_handle, w->mem);

  free(w);
}

CVI_RC matrix_copy_s2d(
  CVI_RT_HANDLE rt_handle,
  const cvk_mg_t *mg,
  uint8_t *data)
{
  test_mg_wrapper_t *w = container_of(mg, test_mg_wrapper_t, mg);
  return CVI_RT_MemCopyS2D(rt_handle, w->mem, data);
}

void matrix_copy_s2d_g2l_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    cvk_mg_stride_t mg_stride,
    uint8_t *data)
{
  cvk_mg_shape_t mg_shape = {
      .row = ml->shape.n,
      .col = ml->shape.col
  };

  cvk_mg_t *mg = alloc_matrix_dev_mem(rt_handle, mg_shape, ml->fmt);
  mg->stride = mg_stride;
  matrix_copy_s2d(rt_handle, mg, data);

  cvk_tdma_g2l_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = mg;
  p.dst = ml;
  cvk_ctx->ops->tdma_g2l_matrix_copy(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  free_matrix_dev_mem(rt_handle, mg);
}

void matrix_copy_s2d_g2l(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint8_t *data)
{
  cvk_mg_shape_t mg_shape = {
      .row = ml->shape.n,
      .col = ml->shape.col
  };

  cvk_mg_stride_t mg_stride = { .row = mg_shape.col * fmt_size(ml->fmt) };
  matrix_copy_s2d_g2l_stride(rt_handle, cvk_ctx, ml, mg_stride, data);
}

CVI_RC cmpr_matrix_copy_s2d(
  CVI_RT_HANDLE rt_handle,
  const cvk_cmpr_mg_t *cmpr_mg,
  uint8_t *data)
{
  test_cmpr_mg_wrapper_t *w = container_of(cmpr_mg, test_cmpr_mg_wrapper_t, cmpr_mg);
  return CVI_RT_MemCopyS2D(rt_handle, w->mem, data);
}

uint8_t *matrix_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_mg_t *mg)
{
  uint32_t size = mg_shape_size(&mg->shape, mg->fmt);
  uint8_t *data = (uint8_t *)malloc(size);
  if (!data)
    return NULL;

  test_mg_wrapper_t *w = container_of(mg, test_mg_wrapper_t, mg);
  CVI_RC ret = CVI_RT_MemCopyD2S(rt_handle, data, w->mem);
  if (ret != CVI_SUCCESS) {
    free(data);
    data = NULL;
  }

  return data;
}

uint8_t *matrix_copy_l2g_d2s_stride(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_ml_t *ml,
  cvk_mg_stride_t mg_stride)
{
  cvk_mg_shape_t s;
  s.row = ml->shape.n;
  s.col = ml->shape.col;

  cvk_mg_t *mg = alloc_matrix_dev_mem(rt_handle, s, ml->fmt);
  mg->stride = mg_stride;

  cvk_tdma_l2g_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = ml;
  p.dst = mg;
  cvk_ctx->ops->tdma_l2g_matrix_copy(cvk_ctx, &p);
  CVI_RT_Submit(cvk_ctx);

  uint8_t *data = matrix_copy_d2s(rt_handle, mg);
  free_matrix_dev_mem(rt_handle, mg);

  return data;
}

uint8_t *matrix_copy_l2g_d2s(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_ml_t *ml)
{
  cvk_mg_stride_t mg_stride = { ml->shape.col * fmt_size(ml->fmt)};
  return matrix_copy_l2g_d2s_stride(rt_handle, cvk_ctx, ml, mg_stride);
}

uint8_t *cmpr_matrix_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_mg_t *cmpr_mg)
{
  size_t bs_buf_size = cmpr_mg_shape_size(&cmpr_mg->m.shape, cmpr_mg->m.fmt);
  uint8_t *data = (uint8_t *)malloc(bs_buf_size);
  if (!data)
    return NULL;

  test_cmpr_mg_wrapper_t *w = container_of(cmpr_mg, test_cmpr_mg_wrapper_t, cmpr_mg);
  CVI_RC ret = CVI_RT_MemCopyD2S(rt_handle, data, w->mem);
  if (ret != CVI_SUCCESS) {
    free(data);
    data = NULL;
  }

  return data;
}

void convert_fp32_to_bf16_data(
    cvk_context_t *cvk_ctx, uint16_t *bf16_data, float *fp32_data,
    int length)
{
  // printf("  convert_fp32_to_bf16_data, len %d\n", length);
  for (int i = 0; i < length; i++) {
    bf16_data[i] = cvk_ctx->misc_ops->float_to_bfloat16(cvk_ctx, fp32_data[i]);
    // printf("    [%d] %f -> 0x%x\n", i, fp32_data[i], bf16_data[i]);
  }
}

void get_strides_from_shapes5d(int strides[5], const int shapes[5], int ws)
{
  strides[5 - 1] = ws;
  for (int i = 5 - 2; i >= 0; i--)
    strides[i] = shapes[i + 1] * strides[i + 1];
}

int get_tensor5d_offset(int poss[5], const int strides[5])
{
  int offset = 0;
  for (int i = 0; i < 5; i++)
    offset += poss[i] * strides[i];

  return offset;
}

void arith_right_shift(
    int32_t *buf, uint64_t size, int shift_bits, int round_up)
{
  if (shift_bits == 0)
    return;

  for (uint64_t i = 0; i < size; i++) {
    buf[i] >>= shift_bits - 1;
    if (round_up)
      buf[i] += 1;
    buf[i] >>= 1;
  }
}

void logic_right_shift(
    int32_t *buf, uint64_t size, int shift_bits, int round_up)
{
  if (shift_bits == 0)
    return;

  for (uint64_t i = 0; i < size; i++) {
    buf[i] = (uint32_t)buf[i] >> (shift_bits - 1);
    if (round_up)
      buf[i] += 1;
    buf[i] = (uint32_t)buf[i] >> 1;
  }
}

void saturate_to_int8(int32_t *buf, uint64_t size, int res_sign)
{
  int32_t max, min;
  if (res_sign) {
    max = 127;
    min = -128;
  } else {
    max = 255;
    min = 0;
  }

  for (uint64_t i = 0; i < size; i++) {
    if (buf[i] > max)
      buf[i] = max;
    else if (buf[i] < min)
      buf[i] = min;
  }
}

void saturate_to_int16(int32_t *buf, uint64_t size, int res_sign)
{
  int32_t max, min;
  if (res_sign) {
    max = 32767;
    min = -32768;
  } else {
    max = 65535;
    min = 0;
  }

  for (uint64_t i = 0; i < size; i++) {
    if (buf[i] > max)
      buf[i] = max;
    else if (buf[i] < min)
      buf[i] = min;
  }
}


/**
 * \cmd_info_est_in that manual set compress parameters, the possible input as below
    1. NULL, it could call \bm_vlc_est_weight_bias
    2. not NULL that directly send to \bm_vlc_enc_int8
 * \cmd_info_est_out output est result, the passble value as following
    1. \cmd_info_est_out = \cmd_info_est_in once cmd_info_est_in != NULL
    2. \cmd_info_est_out = est result once cmd_info_est_in == NULL
    3. NULL if you dont care
 */
uint8_t *test_vlc_compress (
    uint8_t *src_data, uint64_t size, int is_signed, int data_type, size_t* bs_size, const CommandInfo* cmd_info_est_in, CommandInfo* cmd_info_est_out)
{
  CommandInfo cmd_info;
  size_t bs_buf_size = get_out_bs_buf_size(size, data_type);

  uint8_t *bsbuf = (uint8_t *)malloc(bs_buf_size);
  if (!bsbuf)
    return NULL;

  memset(&cmd_info, 0x00, sizeof(CommandInfo));

  /* generate comparess data (bsbuf)*/
  if (cmd_info_est_in) {
    memcpy(&cmd_info, cmd_info_est_in, sizeof(CommandInfo));
  }
  else {
    cvk_vlc_est_weight_bias(src_data, size, (int8_t)is_signed, (int8_t)data_type, &cmd_info);
  }

  if (cmd_info_est_out) {
    memcpy(cmd_info_est_out, &cmd_info, sizeof(CommandInfo));
  }

  if (data_type) {
    cvk_vlc_enc_bf16((uint16_t *)src_data, size, bsbuf, bs_size, &cmd_info);
  }
  else {
    cvk_vlc_enc_int8(src_data, size, bsbuf, bs_size, &cmd_info);
  }

  return bsbuf;
}


#ifdef ENABEL_GAUSSIANRANDOM_VLC_TEST
// --- contrain random test ---
static double getGaussianRandomVar(double mean, double std)
{
  double PI = 3.1415926;
  double u0 = (double)rand() / RAND_MAX;
  double u1 = (double)rand() / RAND_MAX;
  double n = sqrt(-2 * log(u0)) * cos(2 * PI * u1);
  return n * std + mean;
}

static double getExpRandomVar(double lambda)
{
  double x = (double)rand() / RAND_MAX;
  return log(1 - x) / (-lambda);
}

static void random_gen_nn_data(uint8_t *ibuf, size_t in_num, int8_t signedness, int8_t data_type, double zero_ratio)
{
  float *random_buf = (float *)malloc(in_num * sizeof(float));
  int zero_thr = (int)(100 * zero_ratio);
  double lambda = getGaussianRandomVar(0, 0.5);
  double mean = getGaussianRandomVar(0, 8);
  int8_t pdf_sel = ((rand() % 10) < 9); // 9 over 10 choose exponential pdf
  double max_v = 0;
  double eps = 0.0001;
  lambda += (lambda > 0) ? eps : -eps;
  for (size_t i = 0; i < in_num; i++)
  {
    double val = (pdf_sel) ? getExpRandomVar(lambda) : getGaussianRandomVar(mean, lambda);
    val = ((signedness || data_type) && rand() % 2) ? -val : val;
    random_buf[i] = ((rand() % 100) < zero_thr) ? 0 : val;
    max_v = (fabs(random_buf[i]) > max_v) ? fabs(random_buf[i]) : max_v;
  }

  if (data_type == 0) // INT8
  {
    double cali_decay = (signedness) ? (rand() / (double)RAND_MAX) + 1 : 1; // weight dacay by calibration
    uint8_t pruned_thr = (signedness && !data_type && (rand() % 2)) ? rand() % 12 : 0;
    for (size_t i = 0; i < in_num; i++)
    {
      int val = (int)((random_buf[i] * 127) / (max_v * cali_decay));
      ibuf[i] = (abs(val) < pruned_thr)
                    ? 0
                    : (val > 127)
                          ? 127
                          : (val < (-128))
                                ? -128
                                : val;
    }
  }
  else // BFloat16
  {
    uint16_t *bf16_buf = (uint16_t *)random_buf;
    for (size_t i = 0; i < in_num; i++)
    {
      short bf16_val = bf16_buf[(i << 1) + 1];
      // WARNING: set subnormal value to zero since HW do NOT support
      int exp = ((bf16_val >> 7) & 0xFF);
      bf16_val = (exp) ? bf16_val : 0;

      ibuf[i << 1] = (uint8_t)(bf16_val & 0xFF);
      ibuf[(i << 1) + 1] = (uint8_t)(bf16_val >> 8);
    }
  }
  free(random_buf);
}
#endif /* ENABEL_GAUSSIANRANDOM_VLC_TEST */

void test_vlc_init_testdata(
    uint8_t *src_data,
    uint64_t shape_size,
    int8_t signedness,
    int8_t data_type)
{
#ifdef ENABEL_GAUSSIANRANDOM_VLC_TEST
  float zero_ratio = 0;
  assert(data_type == 0); //<! bf16 only set to 1
  random_gen_nn_data(src_data, shape_size, signedness, data_type, zero_ratio);
#else
  (void)signedness;
  (void)data_type;

  printf ("random size %d signedness %d data_type %d\n", (int)shape_size, signedness, data_type);

  if (data_type == 1) {
    // bf16
    uint16_t *src_data_16  = (uint16_t *)src_data;
    memset(src_data, 0x00, shape_size * sizeof(uint16_t));
    for (uint64_t i = 0; i < shape_size; i++)
      src_data_16[i] = 200 + i;

    uint64_t zero_range = 20; //<! friendly enhance compress ratio
    if (shape_size > zero_range) {
      for (uint64_t i = 0; i < shape_size - zero_range; i++) {
        src_data_16[i] = 0;
      }
    }
  } else {
    // int8
    memset(src_data, 0x00, shape_size);
    for (uint64_t i = 0; i < shape_size; i++)
      src_data[i] = 200 + i;

    uint64_t zero_range = 20; //<! friendly enhance compress ratio
    if (shape_size > zero_range) {
      for (uint64_t i = 0; i < shape_size - zero_range; i++) {
        src_data[i] = 0;
      }
    }
  }
#endif /* ENABEL_GAUSSIANRANDOM_VLC_TEST */
}

uint16_t corner_val[] = {
  0x0000, // 0 00000000 0000000 = zero
  0x8000, // 1 00000000 0000000 = −zero
  0x7f80, // 0 11111111 0000000 = infinity
  0xff80, // 1 11111111 0000000 = −infinity
  0x4049, // 0 10000000 1001001 = 3.140625 ≈ π ( pi )
  0x3eab, // 0 01111101 0101011 = 0.333984375 ≈ 1/3
  0xffc1, // x 11111111 1000001 => qNaN
  0xff81, // x 11111111 0000001 => sNaN
  0x00ff, // x 00000000 1111111 => denormal
};

uint16_t test_generate_bf16_corner_val(float val)
{
  if( rand()%RAND_SEED_MOD == 0 ) {
    return corner_val[ rand() % (sizeof(corner_val)/sizeof(uint16_t)) ];
  } else {
    return cvk_convert_fp32_bf16(val);
  }
}