#ifndef __BMKERNEL_1880v2_H__
#define __BMKERNEL_1880v2_H__

#include <bmkernel/bm_kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMK1880v2_TIU        0  // Tensor Instruction Unit
#define BMK1880v2_CPU        1  // CPU, Reserved for common cpu op
#define BMK1880v2_TDMA       2  // TPU DMA
#define BMK1880v2_ENGINE_NUM 3  // Number of Engines

typedef struct bmk_context bmk1880v2_context_t;
typedef struct bmk_context bmk_context_t;
typedef cvk_chip_info_t bmk1880v2_chip_info_t;

typedef struct ec_desc bmk1880v2_op_t;

bmk1880v2_context_t * bmk1880v2_register(bmk_info_t *info);
void bmk1880v2_cleanup(bmk1880v2_context_t *ctx);
void bmk1880v2_reset(bmk1880v2_context_t *ctx);
uint8_t *bmk1880v2_acquire_cmdbuf(bmk1880v2_context_t *ctx, uint32_t *size);
void bmk1880v2_dmabuf_size(uint8_t *cmdbuf, uint32_t sz, uint32_t *psize, uint32_t *pmu_size);
void bmk1880v2_dmabuf_relocate(
    uint8_t *dmabuf, uint64_t dmabuf_devaddr,
    uint32_t original_size, uint32_t pmubuf_size);
void bmk1880v2_dmabuf_convert(uint8_t *cmdbuf, uint32_t sz, uint8_t *dmabuf);
void bmk1880v2_dmabuf_dump(uint8_t * dmabuf);
void bmk1880v2_arraybase_set(
    uint8_t *dmabuf, uint32_t arraybase0L, uint32_t arraybase1L,
    uint32_t arraybase0H, uint32_t arraybase1H);

void bmk1880v2_parallel_enable(bmk1880v2_context_t *ctx);
void bmk1880v2_set_op(bmk1880v2_context_t *ctx, void* op);
void* bmk1880v2_get_op(bmk1880v2_context_t *ctx);
void bmk1880v2_parallel_disable(bmk1880v2_context_t *ctx);
void bmk1880v2_set_layer_id(bmk1880v2_context_t *ctx, uint16_t layer_id);
uint16_t bmk1880v2_layer_id(bmk1880v2_context_t *ctx);

void bmk1880v2_create_streams(bmk1880v2_context_t *ctx, int nr_streams);
void bmk1880v2_destroy_streams(bmk1880v2_context_t *ctx);
void bmk1880v2_set_stream(bmk1880v2_context_t *ctx, int i);

void bmk1880v2_add_dependency(
    bmk1880v2_context_t *ctx,
    bmk1880v2_op_t *before,
    bmk1880v2_op_t *after);

void bmk1880v2_cpu_op(
    bmk1880v2_context_t *ctx,
    const char* op_name, char *params, int size);

/*
 * Fundamental structures for tensor and matrix
 */

typedef struct {
  uint32_t n, c, w, col;
} bmk1880v2_matrix_lmem_shape_t;

typedef struct {
  uint32_t row, col;
} bmk1880v2_matrix_tgmem_shape_t;

typedef struct {
  uint32_t n, c, h;
} bmk1880v2_matrix_lmem_stride_t;

typedef struct {
  uint32_t row;
} bmk1880v2_matrix_tgmem_stride_t;

typedef struct {
  uint32_t n, c, h, w;
} bmk1880v2_tensor_lmem_shape_t;

typedef struct {
  uint32_t n, c, h, w;
} bmk1880v2_tensor_tgmem_shape_t;

typedef struct {
  uint32_t n, c, h, w;
} bmk1880v2_tensor_lmem_stride_t;

typedef struct {
  uint32_t n, c, h;
} bmk1880v2_tensor_tgmem_stride_t;

typedef struct {
  uint32_t start_address;
  fmt_t fmt;
  fmt_t cmprs_fmt;
  bmk1880v2_tensor_lmem_shape_t shape;
  bmk1880v2_tensor_lmem_stride_t stride;
  uint8_t int8_rnd_mode; // <! 0 is round to nearset even, 1 is toward zero, currently used by lut
  uint8_t eu_align;
} bmk1880v2_tensor_lmem_t;

typedef struct {
  uint32_t start_address;
  fmt_t fmt;
  bmk1880v2_matrix_lmem_shape_t shape;
  bmk1880v2_matrix_lmem_stride_t stride;
  uint8_t int8_rnd_mode; // <! 0 is round to nearset even, 1 is toward zero
  uint8_t eu_align;
} bmk1880v2_matrix_lmem_t;

typedef struct {
  uint8_t  base_reg_index;
  uint64_t start_address;
  fmt_t fmt;
  bmk1880v2_tensor_tgmem_shape_t shape;
  bmk1880v2_tensor_tgmem_stride_t stride;
  uint8_t int8_rnd_mode; // <! 0 is round to nearset even, 1 is toward zero
} bmk1880v2_tensor_tgmem_t;

typedef struct {
  bmk1880v2_tensor_tgmem_t t;
  uint64_t reserved_size;
  uint8_t bit_length; //<! deprecated for zero compress
  uint8_t bias0;
  uint8_t bias1;
  uint8_t zero_guard_en;
} bmk1880v2_compressed_tensor_tgmem_t;

typedef struct {
  uint8_t  base_reg_index;
  uint64_t start_address;
  fmt_t fmt;
  bmk1880v2_matrix_tgmem_shape_t shape;
  bmk1880v2_matrix_tgmem_stride_t stride;
  uint8_t int8_rnd_mode; // <! 0 is round to nearset even, 1 is toward zero
} bmk1880v2_matrix_tgmem_t;

typedef struct {
  bmk1880v2_matrix_tgmem_t m;
  uint8_t bias0;
  uint8_t bias1;
  uint8_t zero_guard_en;
} bmk1880v2_compressed_matrix_tgmem_t;

typedef struct {
  uint8_t  base_reg_index;
  uint64_t start_address;
  fmt_t fmt;
  bmk1880v2_matrix_tgmem_shape_t shape;
  bmk1880v2_matrix_tgmem_stride_t stride;
} bmk1880v2_matrix_bf16_tgmem_t;

/*
 * TDMA Engine APIs: LMEM to LMEM (L2L)
 */

typedef struct {
  uint8_t mv_lut_idx;
  uint8_t mv_lut_base;
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
  uint8_t outstanding;
} bmk1880v2_tdma_l2l_tensor_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2l_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2l_bf16_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_copy_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
  int right_shift;
  uint32_t lrn_step;
} bmk1880v2_tdma_l2l_tensor_lrn_shift_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2l_tensor_lrn_shift(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2l_tensor_lrn_shift_param_t *p);

/*
 * TDMA Engine APIs: LMEM to GMEM (L2G)
 */

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_tensor_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_nc_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy_nc_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_nc_transposed_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_cw_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_tensor_copy_cw_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_cw_transposed_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_compressed_tensor_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_tensor_copy_compressed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_copy_compressed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_copy_compressed_param_t *p);

typedef struct {
  uint16_t constant;
  const bmk1880v2_tensor_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_tensor_fill_constant_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_tensor_fill_constant(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_tensor_fill_constant_param_t *p);

typedef struct {
  const bmk1880v2_matrix_lmem_t *src;
  const bmk1880v2_matrix_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_matrix_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_matrix_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_matrix_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_param_t *p);

typedef struct {
  uint32_t src_address;
  uint8_t dst_base_reg_index;
  uint64_t dst_address;
  uint32_t bytes;
} bmk1880v2_tdma_l2tg_general_copy_param_t;

typedef struct {
  uint32_t src_address;
  uint8_t dst_base_reg_index;
  uint64_t dst_address;
  uint32_t src_bytes;
  fmt_t src_fmt;
  fmt_t dst_fmt;
} bmk1880v2_tdma_l2tg_bf16_general_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_general_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_l2g_bf16_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_bf16_general_copy_param_t *p);

/*
 * TDMA Engine APIs: GMEM to LMEM (TG2L)
 */

typedef struct {
  const bmk1880v2_tensor_tgmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
} bmk1880v2_tdma_tg2l_tensor_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_param_t *p);

typedef struct {
  const bmk1880v2_tensor_tgmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
} bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_nc_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_tensor_copy_nc_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t *p);

typedef struct {
  const bmk1880v2_tensor_tgmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
} bmk1880v2_tdma_tg2l_tensor_copy_chw_rotated_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_chw_rotated(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_chw_rotated_param_t *p);

typedef struct {
  const bmk1880v2_compressed_tensor_tgmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
} bmk1880v2_tdma_tg2l_tensor_copy_decompressed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_tensor_copy_decompressed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_copy_decompressed_param_t *p);

typedef struct {
  uint16_t constant;
  const bmk1880v2_tensor_lmem_t *dst;
} bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_tg2l_tensor_fill_constant(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_tg2l_bf16_tensor_fill_constant(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_tensor_fill_constant_param_t *p);

typedef struct {
  const bmk1880v2_compressed_matrix_tgmem_t *src;
  const bmk1880v2_matrix_lmem_t *dst;
} bmk1880v2_tdma_tg2l_matrix_copy_decompressed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy_decompressed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_decompressed_param_t *p);

typedef struct {
  const bmk1880v2_matrix_lmem_t *src;
  const bmk1880v2_compressed_matrix_tgmem_t *dst;
} bmk1880v2_tdma_l2tg_matrix_copy_compressed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_l2g_matrix_copy_compressed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_l2tg_matrix_copy_compressed_param_t *p);

typedef struct {
  const bmk1880v2_matrix_tgmem_t *src;
  const bmk1880v2_matrix_lmem_t *dst;
} bmk1880v2_tdma_tg2l_matrix_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_matrix_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_param_t *p);

typedef struct {
  const bmk1880v2_matrix_tgmem_t *src;
  const bmk1880v2_matrix_lmem_t *dst;
} bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_matrix_copy_row_col_transposed(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t *p);

typedef struct {
  uint8_t src_base_reg_index;
  uint64_t src_address;
  uint32_t dst_address;
  uint32_t bytes;
} bmk1880v2_tdma_tg2l_general_copy_param_t;

typedef struct {
  uint8_t src_base_reg_index;
  uint64_t src_address;
  uint32_t dst_address;
  uint32_t src_bytes;
  fmt_t src_fmt;
  fmt_t dst_fmt;
} bmk1880v2_tdma_tg2l_bf16_general_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_g2l_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_general_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_g2l_bf16_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2l_bf16_general_copy_param_t *p);

/*
 * TDMA Engine APIs: GEM to GEM (G2G)
 */
typedef struct {
  const bmk1880v2_tensor_tgmem_t *src;
  const bmk1880v2_tensor_tgmem_t *dst;
} bmk1880v2_tdma_tg2tg_tensor_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_bf16_general_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p);

bmk1880v2_op_t * bmk1880v2_tdma_tg2tg_bf16_tensor_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tdma_tg2tg_tensor_copy_param_t *p);


/*
 * TIU Engine APIs
 *
 * General rules for tensor arithmetic APIs:
 *
 * 1, All tensors can be either signed or unsigned
 *    if not mentioned otherwise.
 * 2, A tensor @x with both @x_high and @x_low as
 *    parameters can optionally be 8-bit (when @x_high
 *    is NULL) or 16-bit (otherwise).
 */

typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a;
  int b_is_const;
  union {
    const bmk1880v2_tensor_lmem_t *b;
    struct {
      int16_t val;
      int is_signed;
    } b_const;
  };
  uint8_t rshift_bits;
  int relu_enable;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_mul_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_mul(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_mul_param_t *p);

// Multiplier in quantization down
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a;
  int b_is_const;
  union {
    const bmk1880v2_tensor_lmem_t *b;
    struct {
      int8_t val;
      int is_signed;
    } b_const;
  };
  uint8_t rshift_bits;
  int relu_enable;
  uint32_t multiplier;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_mul_qdm_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_mul_qdm(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_mul_qdm_param_t *p);

/*
 * @res = @a * @b + @res
 *
 * 1, @res_high must not be NULL since input @res must be 16-bit.
 * 2, If output @res is 8-bit (@res_is_int8 == 1), only @res_low
 *    is used as output tensor.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a;
  int b_is_const;
  union {
    const bmk1880v2_tensor_lmem_t *b;
    struct {
      int16_t val;
      int is_signed;
    } b_const;
  };
  int res_is_int8;
  int relu_enable;
  uint8_t lshift_bits;
  uint8_t rshift_bits;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_mac_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_mac(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_mac_param_t *p);

/*
 * @a and @b must all be 16-bit.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  int b_is_const;
  union {
    struct {
      const bmk1880v2_tensor_lmem_t *b_high;
      const bmk1880v2_tensor_lmem_t *b_low;
    };
    struct {
      int16_t val;
      int is_signed;
    } b_const;
  };
  uint8_t rshift_bits;
  int relu_enable;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_add_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_add(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_add_param_t *p);

/*
 * 1, @a and @b must all be 16-bit.
 * 2, @res must be signed.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  const bmk1880v2_tensor_lmem_t *b_high;
  const bmk1880v2_tensor_lmem_t *b_low;
  uint8_t rshift_bits;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_sub_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_sub(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_sub_param_t *p);

/*
 * @a and @b must both be signed or unsigned.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *max;
  const bmk1880v2_tensor_lmem_t *a;
  int b_is_const;
  union {
    const bmk1880v2_tensor_lmem_t *b;
    struct {
      int16_t val;
      int is_signed;
    } b_const;
  };
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_max_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_max(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_max_param_t *p);

/*
 * @a and @b must both be signed or unsigned.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *min;
  const bmk1880v2_tensor_lmem_t *a;
  int b_is_const;
  union {
    const bmk1880v2_tensor_lmem_t *b;
    struct {
      int16_t val;
      int is_signed;
    } b_const;
  };
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_min_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_min(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_min_param_t *p);

/*
 * 1, @a must be 16-bit and signed.
 * 2, @res must be 16-bit.
 * 3, @bits must be signed and must range in [-16, 16].
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  const bmk1880v2_tensor_lmem_t *bits;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_arith_shift_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_arith_shift(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_arith_shift_param_t *p);

/*
 * TODO: bmk1880v2_tl_logic_shift()
 */

typedef struct {
  const bmk1880v2_tensor_lmem_t *res;
  const bmk1880v2_tensor_lmem_t *a;
  const bmk1880v2_tensor_lmem_t *b;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_and_int8_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_and_int8(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_and_int8_param_t *p);

/*
 * All parameters must be 16-bit.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  const bmk1880v2_tensor_lmem_t *b_high;
  const bmk1880v2_tensor_lmem_t *b_low;
} bmk1880v2_tiu_element_wise_and_int16_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_and_int16(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_and_int16_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *res;
  const bmk1880v2_tensor_lmem_t *a;
  const bmk1880v2_tensor_lmem_t *b;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_or_int8_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_or_int8(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_or_int8_param_t *p);

/*
 * All parameters must be 16-bit.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  const bmk1880v2_tensor_lmem_t *b_high;
  const bmk1880v2_tensor_lmem_t *b_low;
} bmk1880v2_tiu_element_wise_or_int16_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_or_int16(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_or_int16_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *res;
  const bmk1880v2_tensor_lmem_t *a;
  const bmk1880v2_tensor_lmem_t *b;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_xor_int8_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_xor_int8(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_xor_int8_param_t *p);

/*
 * All parameters must be 16-bit.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *res_high;
  const bmk1880v2_tensor_lmem_t *res_low;
  const bmk1880v2_tensor_lmem_t *a_high;
  const bmk1880v2_tensor_lmem_t *a_low;
  const bmk1880v2_tensor_lmem_t *b_high;
  const bmk1880v2_tensor_lmem_t *b_low;
} bmk1880v2_tiu_element_wise_xor_int16_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_xor_int16(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_xor_int16_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *src;
  const bmk1880v2_tensor_lmem_t *dst;
  uint16_t layer_id;
} bmk1880v2_tiu_element_wise_copy_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_copy(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_element_wise_copy_param_t *p);

/*
 * @res and @a must be both signed or unsigned.
 * TODO: explain the shape and layout of @res when it's 16-bit
 */
typedef struct {
  int res_is_int8;
  const bmk1880v2_tensor_lmem_t *res;
  const bmk1880v2_tensor_lmem_t *input;
  uint8_t rshift_bits;
  uint16_t layer_id;
} bmk1880v2_tiu_mdsum_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_mdsum(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_mdsum_param_t *p);

/*
 * NOTE:
 *   @table is treated logically as a linear list of
 *   length @table_n, where @table_n is a multiple of
 *   16 and is smaller than or equal to 256.
 *   When stored in local memory, @table is a tensor
 *   of shape (1, npu_num, 1, @table_n), that is, the
 *   data of the linear list should be copied across
 *   each NPU's local memory by user. The behavior when
 *   these copies differ is undefined.
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  const bmk1880v2_tensor_lmem_t *table;
  uint16_t layer_id;
} bmk1880v2_tiu_lookup_table_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_lookup_table(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_lookup_table_param_t *p);

/*
 * Convolution weight shape:
 *   Calibration output (oc, ic, kh, kw)
 *   bm_build transforms (oc, ic, kh, kw) -> (1, oc, kh*kw, ic)
 *   TDMA load global (1, oc, kh*w, ic) -> local (1, oc, kh*kw, ic)
 *   TIU conv opd1 (ic, oc, kh, kw)
 */
typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  const bmk1880v2_tensor_lmem_t *weight;
  const bmk1880v2_tensor_lmem_t *bias;
  uint8_t ins_h, ins_last_h;
  uint8_t ins_w, ins_last_w;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  uint8_t dilation_h, dilation_w;
  int relu_enable;
  uint8_t rshift_bits;
  uint8_t ps32_mode;
  uint8_t w_is_const;
  uint16_t layer_id;
  uint8_t fp_round_typ;
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
} bmk1880v2_tiu_convolution_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_convolution(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_convolution_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  const bmk1880v2_tensor_lmem_t *weight;
  const bmk1880v2_tensor_lmem_t *chl_quan_param;
  uint8_t ins_h, ins_last_h;
  uint8_t ins_w, ins_last_w;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  uint8_t dilation_h, dilation_w;
  uint8_t has_bias;
  uint8_t relu_enable;
  uint8_t ps32_mode;
  uint8_t w_is_const;
  uint16_t layer_id;
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
} bmk1880v2_tiu_convolution_qdm_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_convolution_qdm(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_convolution_qdm_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  uint16_t kh, kw;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
  uint16_t layer_id;
} bmk1880v2_tiu_max_pooling_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_max_pooling(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_max_pooling_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  uint16_t kh, kw;
  uint8_t ins_h, ins_last_h;
  uint8_t ins_w, ins_last_w;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  uint16_t avg_pooling_const;
  uint8_t rshift_bits;
  uint16_t layer_id;
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
} bmk1880v2_tiu_average_pooling_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_average_pooling(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_average_pooling_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  const bmk1880v2_tensor_lmem_t *weight;
  const bmk1880v2_tensor_lmem_t *bias;
  int weight_is_const;
  struct {
    int16_t val;
    int is_signed;
  } weight_const;
  uint8_t ins_h, ins_last_h;
  uint8_t ins_w, ins_last_w;
  uint8_t dilation_h, dilation_w;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  uint8_t rshift_bits;
  int relu_enable;
  uint16_t layer_id;
  uint8_t ps32_mode;  //output fp32 result if ps32_mode==2
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
} bmk1880v2_tiu_depthwise_convolution_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_depthwise_convolution(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_depthwise_convolution_param_t *p);

typedef struct {
  const bmk1880v2_tensor_lmem_t *ofmap;
  const bmk1880v2_tensor_lmem_t *ifmap;
  const bmk1880v2_tensor_lmem_t *weight;
  const bmk1880v2_tensor_lmem_t *chl_quan_param;
  int weight_is_const;
  struct {
    int16_t val;
    int is_signed;
  } weight_const;
  uint8_t ins_h, ins_last_h;
  uint8_t ins_w, ins_last_w;
  uint8_t dilation_h, dilation_w;
  uint8_t pad_top, pad_bottom;
  uint8_t pad_left, pad_right;
  uint8_t stride_h, stride_w;
  uint8_t has_bias;
  uint8_t relu_enable;
  uint16_t layer_id;
  int8_t ins_val;   // padding value for int8
  uint16_t ins_fp;  // padding value for bf16
} bmk1880v2_tiu_depthwise_convolution_qdm_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_depthwise_convolution_qdm(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_depthwise_convolution_qdm_param_t *p);

typedef struct {
  const bmk1880v2_matrix_lmem_t *res;
  const bmk1880v2_matrix_lmem_t *left;
  const bmk1880v2_matrix_lmem_t *right;
  const bmk1880v2_matrix_lmem_t *bias;
  uint8_t lshift_bits;
  uint8_t rshift_bits;
  int res_is_int8;
  int relu_enable;
  int add_result;
  uint8_t  ps32_mode;
  uint16_t layer_id;
} bmk1880v2_tiu_matrix_multiplication_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_matrix_multiplication(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_matrix_multiplication_param_t *p);

typedef struct {
  const bmk1880v2_matrix_lmem_t *res;
  const bmk1880v2_matrix_lmem_t *left;
  const bmk1880v2_matrix_lmem_t *right;
  const bmk1880v2_matrix_lmem_t *bias;
  uint8_t lshift_bits;
  uint8_t rshift_bits;
  int res_is_int8;
  int relu_enable;
  int add_result;
  uint8_t  ps32_mode;
  int32_t quan_m;
  uint16_t layer_id;
} bmk1880v2_tiu_matrix_multiplication_qdm_param_t;

bmk1880v2_op_t * bmk1880v2_tiu_matrix_multiplication_qdm(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tiu_matrix_multiplication_qdm_param_t *p);

/*
 * Helpers
 */

bmk1880v2_tensor_lmem_stride_t bmk1880v2_tensor_lmem_default_stride(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt_type,
    int eu_align);

bmk1880v2_tensor_tgmem_stride_t bmk1880v2_tensor_tgmem_default_stride(
    bmk1880v2_tensor_tgmem_shape_t s,
    fmt_t fmt_type);

bmk1880v2_matrix_lmem_shape_t bmk1880v2_matrix_lmem_default_shape(
    bmk1880v2_context_t *ctx,
    uint32_t row,
    uint32_t col,
    fmt_t fmt_type);

bmk1880v2_matrix_lmem_shape_t bmk1880v2_matrix_lmem_shape_t1(
    bmk1880v2_context_t *ctx,
    uint32_t len,
    fmt_t fmt_type);

bmk1880v2_matrix_lmem_stride_t bmk1880v2_matrix_lmem_default_stride(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

bmk1880v2_tensor_lmem_t * bmk1880v2_lmem_alloc_tensor(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

void bmk1880v2_lmem_init_tensor(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_t *tl,
    bmk1880v2_tensor_lmem_shape_t shape,
    fmt_t fmt,
    int eu_align);

bmk1880v2_tensor_lmem_t * bmk1880v2_lmem_alloc_ps32_tensor(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

void bmk1880v2_lmem_free_tensor(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_tensor_lmem_t *t);

bmk1880v2_matrix_lmem_t * bmk1880v2_lmem_alloc_matrix(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

void bmk1880v2_lmem_init_matrix(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_t *ml,
    bmk1880v2_matrix_lmem_shape_t shape,
    fmt_t fmt,
    int eu_align);

bmk1880v2_matrix_lmem_t * bmk1880v2_lmem_alloc_ps32_matrix(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

void bmk1880v2_lmem_free_matrix(
    bmk1880v2_context_t *ctx,
    const bmk1880v2_matrix_lmem_t *t);

uint32_t bmk1880v2_lmem_tensor_to_size(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

uint32_t bmk1880v2_lmem_matrix_to_size(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

uint32_t bmk1880v2_lmem_ps32_matrix_to_size(
    bmk1880v2_context_t *ctx,
    bmk1880v2_matrix_lmem_shape_t s,
    fmt_t fmt,
    int eu_align);

#include "non_atomic.h"

#ifdef __cplusplus
}
#endif

#endif /* __BMKERNEL_1880V2_H__ */
