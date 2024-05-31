#ifndef CVIRUNTIME_TEST_UTIL_H
#define CVIRUNTIME_TEST_UTIL_H

#include <stdint.h>
#include "cvikernel/cvikernel.h"
#include "cvikernel/cvk_fp_convert.h"
#include "cvikernel/cvk_vlc_compress.h"
#include "cviruntime_context.h"
#include "cvitpu_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMDBUF_SIZE   (512*1024)  // Adjust based on test case

#define __FILENAME__ (strrchr(__FILE__, '/') ? \
                     strrchr(__FILE__, '/') + 1 : __FILE__)

#define math_min(x, y)          ((x) < (y) ? (x) : (y))
#define math_max(x, y)          ((x) > (y) ? (x) : (y))

typedef enum {
  VLC_CMP_MODE_HW = 0, // <! vlc compress mode - hw, ONLY support bias0/bias1
  VLC_CMP_MODE_COMPILER, // <! vlc compress mode - sw, compiler, it could call bm_vlc_est_weight_bias
  VLC_CMP_MODE_MAX,
} vlc_cmp_mode_e;

static inline uint32_t fmt_size(cvk_fmt_t fmt)
{
  if (fmt == CVK_FMT_I8)
    return 1;
  else if (fmt == CVK_FMT_BF16)
    return 2;
  else if (fmt == CVK_FMT_F32)
    return 4;
  else if (fmt == CVK_FMT_I32)
    return 4;
  else if (fmt == CVK_FMT_U32)
    return 4;
  else
    return 1;
}

static inline uint32_t tl_shape_size(const cvk_tl_shape_t *s, cvk_fmt_t fmt)
{
  return s->n * s->c * s->h * s->w * fmt_size(fmt);
}

static inline uint32_t tg_shape_size(const cvk_tg_shape_t *s, cvk_fmt_t fmt)
{
  return s->n * s->c * s->h * s->w * fmt_size(fmt);
}

static inline uint32_t cmpr_tg_shape_size(const cvk_tg_shape_t *s, cvk_fmt_t fmt)
{
  uint8_t data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;
  uint64_t size = tg_shape_size(s, fmt);
  return get_out_bs_buf_size(size, data_type);
}

static inline uint32_t mg_shape_size(const cvk_mg_shape_t *s, cvk_fmt_t fmt)
{
  return s->row * s->col * fmt_size(fmt);
}

static inline uint32_t cmpr_mg_shape_size(const cvk_mg_shape_t *s, cvk_fmt_t fmt)
{
  uint8_t data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;
  uint64_t size = mg_shape_size(s, fmt);
  return get_out_bs_buf_size(size, data_type);
}

static inline uint32_t ml_shape_size(const cvk_ml_shape_t *s, cvk_fmt_t fmt)
{
  return s->n * s->col * fmt_size(fmt);
}

static inline cvk_tl_shape_t tl_shape_t4(uint32_t n, uint32_t c, uint32_t h, uint32_t w)
{
    cvk_tl_shape_t shape = {.n = n, .c = c, .h = h, .w = w};
    return shape;
}

static inline cvk_tg_shape_t tg_shape_t4(uint32_t n, uint32_t c, uint32_t h, uint32_t w)
{
    cvk_tg_shape_t shape = {.n = n, .c = c, .h = h, .w = w};
    return shape;
}

static inline uint64_t align_up(uint64_t x, uint64_t n)
{
  return (x + n - 1) / n * n;
}

cvk_tg_t *alloc_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt);

void free_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_tg_t *tg);

cvk_cmpr_tg_t *alloc_cmpr_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt,
    CommandInfo *cmd_info);

void free_cmpr_tensor_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg);

CVI_RC tensor_copy_s2d(
    CVI_RT_HANDLE rt_handle,
    const cvk_tg_t *tg,
    uint8_t *data);

CVI_RC cmpr_tensor_copy_s2d(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg,
    uint8_t *data);

uint8_t *tensor_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_tg_t *tg);

uint8_t *cmpr_tensor_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_tg_t *cmpr_tg);

uint8_t *tensor_copy_l2g_d2s_stride(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_tl_t *tl,
  cvk_tg_stride_t tg_stride);

uint8_t *tensor_copy_l2g_d2s(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_tl_t *tl);

void tensor_copy_s2d_g2l_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    cvk_tg_stride_t tg_stride,
    uint8_t *data);

void tensor_copy_s2d_g2l(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint8_t *data);

void tensor_copy_s2d_g2l_tp_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    cvk_tg_stride_t tg_stride,
    uint8_t *data);

void tensor_copy_s2d_g2l_tp(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint8_t *data);

cvk_mg_t *alloc_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_mg_shape_t shape,
    cvk_fmt_t fmt);

void free_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_mg_t *mg);

cvk_cmpr_mg_t *alloc_cmpr_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    cvk_mg_shape_t shape,
    cvk_fmt_t fmt,
    CommandInfo* cmd_info);

void free_cmpr_matrix_dev_mem(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_mg_t *cmpr_mg);

CVI_RC matrix_copy_s2d(
  CVI_RT_HANDLE rt_handle,
  const cvk_mg_t *mg,
  uint8_t *data);

void matrix_copy_s2d_g2l_stride(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    cvk_mg_stride_t mg_stride,
    uint8_t *data);

void matrix_copy_s2d_g2l(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint8_t *data);

CVI_RC cmpr_matrix_copy_s2d(
  CVI_RT_HANDLE rt_handle,
  const cvk_cmpr_mg_t *cmpr_mg,
  uint8_t *data);

uint8_t *matrix_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_mg_t *mg);

uint8_t *matrix_copy_l2g_d2s_stride(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_ml_t *ml,
  cvk_mg_stride_t mg_stride);

uint8_t *matrix_copy_l2g_d2s(
  CVI_RT_HANDLE rt_handle,
  cvk_context_t *cvk_ctx,
  const cvk_ml_t *ml);

uint8_t *cmpr_matrix_copy_d2s(
    CVI_RT_HANDLE rt_handle,
    const cvk_cmpr_mg_t *cmpr_mg);

void convert_fp32_to_bf16_data(
    cvk_context_t *cvk_ctx, uint16_t *bf16_data, float *fp32_data,
    int length);

void get_strides_from_shapes5d(int strides[5], const int shapes[5], int ws);
int get_tensor5d_offset(int poss[5], const int strides[5]);

void arith_right_shift(
    int32_t *buf, uint64_t size, int shift_bits, int round_up);
void logic_right_shift(
    int32_t *buf, uint64_t size, int shift_bits, int round_up);

void saturate_to_int8(int32_t *buf, uint64_t size, int res_sign);
void saturate_to_int16(int32_t *buf, uint64_t size, int res_sign);

uint8_t *test_vlc_compress(
    uint8_t *src_data,
    uint64_t size,
    int is_signed,
    int data_type,
    size_t* bs_size,
    const CommandInfo* cmd_info_est_in,
    CommandInfo* cmd_info_est_out);

void test_vlc_init_testdata(
    uint8_t *src_data,
    uint64_t shape_size,
    int8_t signedness,
    int8_t data_type);

uint16_t test_generate_bf16_corner_val(float val);

#ifdef __cplusplus
}
#endif

#endif // TEST_CVIKERNEL_UTIL_H