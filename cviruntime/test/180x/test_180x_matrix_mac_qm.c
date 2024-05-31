#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_tf_quant_util.h"

// #define ENABLE_DEBUG_MSG

#define MIN_EXEC_TESTS  20

typedef cvk_tiu_matrix_multiplication_qm_param_t param_t;

typedef struct {
  int left_row;
  int left_col;
  int right_col;
  int has_bias;
  int relu_enable;
  int8_t *input_data;
  int8_t *filter_data;
  int8_t *output_data;
  int32_t *bias_data;
  uint32_t multiplier;
  int8_t right_shift;
  float float_multiplier;
  int retry_cnt;
} fc_test_param_t;

void fully_connected_ref(fc_test_param_t *p_param)
{
  const int32_t input_offset = 0;
  const int32_t filter_offset = 0;
  const int32_t output_offset = 0;
  const int32_t output_multiplier = p_param->multiplier;
  const int output_rshift = p_param->right_shift;
  const int batches = p_param->left_row;
  const int output_depth = p_param->right_col;
  const int accum_depth = p_param->left_col;
  int8_t *input_data = p_param->input_data;
  int8_t *filter_data = p_param->filter_data;
  int8_t *output_data = p_param->output_data;
  int32_t *bias_data = p_param->has_bias ? p_param->bias_data : NULL;

  const int32_t output_activation_min = -128;
  const int32_t output_activation_max = 127;

#ifdef ENABLE_DEBUG_MSG
  printf("fully_connected_ref:\n");
  printf("  batches %d, output_depth %d, accum_depth %d, filter_offset %d, "
         "input_offset %d\n",
         batches, output_depth, accum_depth, filter_offset, input_offset);
  printf("  output_multiplier %d, output_rshift %d\n", output_multiplier,
         output_rshift);
#endif

  for (int b = 0; b < batches; ++b) {
    for (int out_c = 0; out_c < output_depth; ++out_c) {
      int32_t acc = 0;
      for (int d = 0; d < accum_depth; ++d) {
        int32_t input_val = input_data[b * accum_depth + d];
        // int32_t filter_val = filter_data[out_c * accum_depth + d];
        int32_t filter_val = filter_data[output_depth * d + out_c];
        acc += (filter_val + filter_offset) * (input_val + input_offset);

#ifdef ENABLE_DEBUG_MSG
        printf("  [%d][%d][%d] acc(%d) += (%d + %d) * (%d + %d) = %d\n", b,
               out_c, d,
               acc - (filter_val + filter_offset) * (input_val + input_offset),
               filter_val, filter_offset, input_val, input_offset, acc);
#endif
      }
      if (bias_data) {
        acc += bias_data[out_c];

#ifdef ENABLE_DEBUG_MSG
        printf("  [%d][%d] acc %d, bias %d\n", b, out_c, acc,
               bias_data ? bias_data[out_c] : 0);
#endif
      }
      acc = MultiplyByQuantizedMultiplier(acc, output_multiplier, output_rshift);

#ifdef ENABLE_DEBUG_MSG
      printf("  [%d][%d] acc %d, output_multiplier %d, output_rshift %d\n", b,
             out_c, acc, output_multiplier, output_rshift);
#endif

      acc += output_offset;

#ifdef ENABLE_DEBUG_MSG
      printf("  [%d][%d] acc %d, output_offset %d\n", b, out_c, acc,
             output_offset);
#endif

      acc = MAX(acc, output_activation_min);
      acc = MIN(acc, output_activation_max);

#ifdef ENABLE_DEBUG_MSG
      printf("  [%d][%d] acc %d, output_activation_min %d, "
             "output_activation_max %d\n",
             b, out_c, acc, output_activation_min, output_activation_max);
#endif

      output_data[out_c + output_depth * b] = acc;
    }
  }
}

void calc_fc_float_multiplier(fc_test_param_t *p_param)
{
  const int32_t input_offset = 0;
  const int32_t filter_offset = 0;
  const int batches = p_param->left_row;
  const int output_depth = p_param->right_col;
  const int accum_depth = p_param->left_col;
  int8_t *input_data = p_param->input_data;
  int8_t *filter_data = p_param->filter_data;
  int32_t *bias_data = p_param->has_bias ? p_param->bias_data : NULL;

  int output_accu_min = INT_MIN;
  int output_accu_max = INT_MAX;

#ifdef ENABLE_DEBUG_MSG
  printf("calc_fc_float_multiplier:\n");
#endif

  for (int b = 0; b < batches; ++b) {
    for (int out_c = 0; out_c < output_depth; ++out_c) {
      int32_t acc = 0;
      for (int d = 0; d < accum_depth; ++d) {
        int32_t input_val = input_data[b * accum_depth + d];
        // int32_t filter_val = filter_data[out_c * accum_depth + d];
        int32_t filter_val = filter_data[output_depth * d + out_c];
        acc += (filter_val + filter_offset) * (input_val + input_offset);
      }
      if (bias_data) {
        acc += bias_data[out_c];
      }

      output_accu_max = MAX(acc, output_accu_max);
      output_accu_min = MIN(acc, output_accu_min);

    }
  }

  // Since int8 ranges from -128 to 127, we need to squeeze the accumulator
  // MIN/MAX fit in those ranges correspondingly as much as possible.
  if (abs(output_accu_max) > abs(output_accu_min)) {
    p_param->float_multiplier = 127.0f / abs(output_accu_max);
  } else {
    p_param->float_multiplier = 128.0f / abs(output_accu_min);
  }

#ifdef ENABLE_DEBUG_MSG
  printf("  output_accu_min %d, output_accu_max %d, output_multiplier %f\n",
         output_accu_min, output_accu_max, p_param->float_multiplier);
#endif

#ifdef ENABLE_DEBUG_MSG
  printf("<= calc_fc_float_multiplier\n");
#endif
}

static void put_bias32(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const cvk_ml_t *ml,
                       int32_t data[])
{
  uint64_t size = ml->shape.col;

  uint8_t *tmp = (uint8_t *)malloc(size * 4);
  if (!tmp)
    return;

  for (uint64_t i = 0; i < size; i++) {
    uint32_t val = data[i];
    tmp[i] = val & 0xff;
    tmp[i + size] = (val >> 8) & 0xff;
    tmp[i + 2 * size] = (val >> 16) & 0xff;
    tmp[i + 3 * size] = (val >> 24) & 0xff;
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, ml, tmp);

  free(tmp);
}

#if 0
typedef struct {
  int32_t input_offset;
  int32_t weights_offset;
  int32_t output_offset;
  int32_t output_multiplier;
  int output_rshift;
} FullyConnectedParams;

int tfl_original_test()
{
  int ret = 0;

  // 2x10
  int8_t input_data[20] = {
    1, 3, 5, 7,  9, 11, 13,  15, -19, -21,
    1, 3, 5, 7,  9, 11, 13, -17,  17, -21};

  // 3x10
  int8_t filter_data[30] = {
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19};

  // 1x3
  int32_t bias_data[3] = {4, 8, 12};

  // 2x3
  int8_t ref_output_data[6] = {
    23, 24, 25,
    57, 58, 59};

  int8_t output_rshift = 1; // change to right shift
  uint32_t output_multiplier = 1073741824;

  int32_t input_offset = 1;
  int32_t filter_offset = 1;
  int32_t output_offset = 1;  // change to right shift

  FullyConnectedParams params;
  params.input_offset = input_offset;
  params.weights_offset = filter_offset;
  params.output_offset = output_offset;
  params.output_multiplier = output_multiplier;
  params.output_rshift = output_rshift;

  cvk_tl_shape_t input_shape = {2, 10, 1, 1};
  cvk_tl_shape_t filter_shape = {3, 10, 1, 1};
  cvk_tl_shape_t bias_shape = {1, 3, 1, 1};
  cvk_tl_shape_t output_shape = {2, 3, 1, 1};

  int8_t output_data[6];
  fully_connected_ref(params, input_shape,
                      input_data, filter_shape,
                      filter_data, bias_shape,
                      bias_data, output_shape,
                      output_data);
  for (int i = 0; i < 6; i++) {
    if (output_data[i] != ref_output_data[i]) {
      printf("  output_data[%d] %d != %d\n",
             i, output_data[i], ref_output_data[i]);
      ret = -1;
    }
  }

  return ret;
}
#endif

int simple_test(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;

  // 2x10
  int8_t input_data[20] = {1, 3, 5, 7, 9, 11, 13, 15,  -19, -21,
                       1, 3, 5, 7, 9, 11, 13, -17, 17,  -21};

#if 0
  // 3x10
  // tfl use transposed filter
  int8_t filter_data_tp[30] = {
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
    1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
#endif

  // 10x3
  int8_t filter_data[30] = {1,  1,  1,  3,  3,  3,  5,  5,  5,  7,
                        7,  7,  9,  9,  9,  11, 11, 11, 13, 13,
                        13, 15, 15, 15, 17, 17, 17, 19, 19, 19};

  // 1x3
  int32_t bias_data[3] = {4, 8, 12};

  // 2x3, input/kernel/output zero_point = 0
  int8_t ref_output_data[6] = {-10, -9, -8, 24, 25, 26};
  int8_t output_data[6];

  int8_t output_rshift = 1;  // change to right shift
  uint32_t output_multiplier = 1073741824;

  int left_row = 2;
  int left_col = 10;
  int right_col = 3;

  fc_test_param_t params;
  memset(&params, 0, sizeof(params));
  params.left_row = left_row;
  params.left_col = left_col;
  params.right_col = right_col;
  params.has_bias = 1;
  params.relu_enable = 0;
  params.input_data = input_data;
  params.filter_data = filter_data;
  params.output_data = output_data;
  params.bias_data = bias_data;
  params.multiplier = output_multiplier;
  params.right_shift = output_rshift;
  fully_connected_ref(&params);

#ifdef ENABLE_DEBUG_MSG
  printf("Compare ref and golden\n");
#endif
  for (int i = 0; i < 6; i++) {
    if (output_data[i] != ref_output_data[i]) {
      printf("  output_data[%d] %d(ref) != %d(golden)\n", i, output_data[i],
             ref_output_data[i]);
      ret = -1;
    }
  }

  cvk_ml_shape_t left_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, left_col, CVK_FMT_I8);

  cvk_ml_shape_t right_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_col, right_col, CVK_FMT_I8);

  cvk_ml_shape_t b_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, 4, right_col, CVK_FMT_I8);  // 32bit

  cvk_ml_shape_t y_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, right_col, CVK_FMT_I8);

  cvk_ml_t *tl_left =
      cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  cvk_ml_t *tl_right =
      cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  cvk_ml_t *tl_b =
      cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, b_shape, CVK_FMT_I8, 1);
  cvk_ml_t *tl_y =
      cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, y_shape, CVK_FMT_I8, 1);

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, tl_left, (uint8_t *)input_data);
  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, tl_right, (uint8_t *)filter_data);
  put_bias32(rt_handle, cvk_ctx, tl_b, bias_data);

  {
    param_t p;
    memset(&p, 0, sizeof(p));
    p.left = tl_left;
    p.right = tl_right;
    p.bias = tl_b;
    p.res = tl_y;
    p.rshift_bits = output_rshift;
    p.res_is_int8 = 1;
    p.ps32_mode = 0;
    p.quan_m = output_multiplier;
    cvk_ctx->ops->tiu_matrix_multiplication_qm(cvk_ctx, &p);
  }

  int8_t *tiu_output_data =
      (int8_t *)matrix_copy_l2g_d2s(rt_handle, cvk_ctx, tl_y);
#ifdef ENABLE_DEBUG_MSG
  printf("Compare tiu and ref\n");
#endif
  for (int i = 0; i < 6; i++) {
    if (tiu_output_data[i] != ref_output_data[i]) {
      printf("  output_data[%d] %d(tiu) != %d(ref)\n", i, tiu_output_data[i],
             ref_output_data[i]);
      ret = -1;
    }
  }

  free(tiu_output_data);

  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_y);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_b);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_right);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_left);

  return ret;
}

int choose_from_range(int table[], int size, int index)
{
  if (index >= size) {
    return 0;
  }

  int val = table[index];
  if (index < (size - 1)) {
    int range = MAX(table[index + 1] - table[index] - 1, 1);
    val += rand() % range;
  }

  return val;
}

bool check_valid_test_param(cvk_context_t *cvk_ctx, fc_test_param_t *p_param)
{
  int left_row = p_param->left_row;
  int left_col = p_param->left_col;
  int right_col = p_param->right_col;
  int has_bias = p_param->has_bias;

  cvk_ml_shape_t tl_input_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, left_col, CVK_FMT_I8);
  cvk_ml_stride_t tl_input_stride =
      cvk_ctx->ops->ml_default_stride(cvk_ctx, tl_input_shape, CVK_FMT_I8, 1);
      
  cvk_ml_shape_t tl_filter_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_col, right_col, CVK_FMT_I8);
  cvk_ml_stride_t tl_filter_stride =
      cvk_ctx->ops->ml_default_stride(cvk_ctx, tl_filter_shape, CVK_FMT_I8, 1);

  cvk_ml_shape_t tl_output_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, right_col, CVK_FMT_I8);
  cvk_ml_stride_t tl_output_stride =
      cvk_ctx->ops->ml_default_stride(cvk_ctx, tl_output_shape, CVK_FMT_I8, 1);

  uint32_t bias_size = 0;
  if (has_bias) {
    cvk_ml_shape_t tl_bias_shape =
        cvk_ctx->ops->ml_default_shape(cvk_ctx, 4, right_col, CVK_FMT_I8);  // 32bit
    cvk_ml_stride_t tl_bias_stride =
        cvk_ctx->ops->ml_default_stride(cvk_ctx, tl_bias_shape, CVK_FMT_I8, 1);
    bias_size = tl_bias_shape.n * tl_bias_stride.n;
  }

  uint32_t lmem_size_per_lane = cvk_ctx->info.lmem_size;
  // uint32_t total_lmem_size = cvk_ctx->info.lmem_size * cvk_ctx->info.npu_num;

  uint32_t needed_size = tl_input_shape.n * tl_input_stride.n +
                    tl_filter_shape.n * tl_filter_stride.n +
                    tl_output_shape.n * tl_output_stride.n + bias_size;

  if (needed_size > lmem_size_per_lane) {
    return false;
  }

  return true;
}

void fill_random_data_s8(int8_t *input_data, int size)
{
  for (int i = 0; i < size; ++i) {
    int is_satured = ((rand() % 1000) == 1) ? 1 : 0;
    int is_sign = rand() % 2 ? 1 : -1;

    if (is_satured && is_sign) {
      input_data[i] = -128;
    } else if (is_satured) {
      input_data[i] = 127;
    } else {
      input_data[i] = is_sign * rand() % 128;
    }
  }
}

void fill_random_data_s32(int32_t *input_data, int size)
{
  for (int i = 0; i < size; ++i) {
    int is_satured = ((rand() % 1000) == 1) ? 1 : 0;
    int is_sign = rand() % 2 ? 1 : -1;

    if (is_satured && is_sign) {
      input_data[i] = INT_MIN;
    } else if (is_satured) {
      input_data[i] = INT_MAX;
    } else {
      input_data[i] = is_sign * rand() % 128;
    }
  }
}

void dump_test_param(fc_test_param_t *p_param, bool dump_content)
{
  printf("Dump test paramter:\n");
  printf("  left_row %d\n", p_param->left_col);
  printf("  left_col %d\n", p_param->left_col);
  printf("  right_col %d\n", p_param->right_col);
  printf("  has_bias %d\n", p_param->has_bias);
  printf("  multiplier %d\n", p_param->multiplier);
  printf("  right_shift %d\n", p_param->right_shift);

  if (dump_content) {
    printf("input_data(%d, %d)\n", p_param->left_row, p_param->left_col);
    int left_row = p_param->left_row;
    int left_col = p_param->left_col;
    for (int i = 0; i < left_row; ++i) {
      for (int j = 0; j < left_col; ++j) {
        int offset = i * left_col + j;
        printf("%d, ", p_param->input_data[offset]);
      }
      printf("\n");
    }
    printf("\n\n");

    int right_col = p_param->right_col;
    printf("kernel_data (%d, %d)\n", left_col, right_col);
    for (int i = 0; i < left_col; ++i) {
      for (int j = 0; j < right_col; ++j) {
        int offset = i * right_col + j;
        printf("%d, ", p_param->filter_data[offset]);
      }
      printf("\n");
    }
    printf("\n\n");

    if (p_param->has_bias) {
      for (int i = 0; i < right_col; ++i) {
        printf("%d, ", p_param->bias_data[i]);
      }
      printf("\n\n");
    }
  }
}

int run_compare_fc(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, fc_test_param_t *p_param)
{
  int ret = 0;

  int left_row = p_param->left_row;
  int left_col = p_param->left_col;
  int right_col = p_param->right_col;
  int has_bias = p_param->has_bias;

  int input_size = left_row * left_col;
  int8_t *input_data = (int8_t *)malloc(input_size);

  int kernel_size = left_col * right_col;
  int8_t *kernel_data = (int8_t *)malloc(kernel_size);

  int output_size = left_row * right_col;
  int8_t *output_data = (int8_t *)malloc(output_size);

  int32_t *bias_data = (int32_t *) malloc(sizeof(int32_t) * right_col);

  p_param->input_data = input_data;
  p_param->filter_data = kernel_data;
  p_param->output_data = output_data;
  p_param->bias_data = bias_data;

#ifdef ENABLE_DEBUG_MSG
  printf("    run_compare_conv =>\n");
  printf("      left (%d, %d), right (%d, %d), has_bias %d\n", left_row,
         left_col, left_col, right_col, has_bias);
#endif

  int retry_cnt = p_param->retry_cnt;
  do {
    fill_random_data_s8(input_data, input_size);
    fill_random_data_s8(kernel_data, kernel_size);
    if (has_bias) {
      fill_random_data_s32(bias_data, right_col);
    }

    p_param->float_multiplier = 100.0;  // should be < 1.0
    calc_fc_float_multiplier(p_param);

    if (p_param->float_multiplier > 0.f && p_param->float_multiplier < 1.0) {
      break;
    }

  } while (--retry_cnt);

  if (p_param->float_multiplier >= 1.0) {
    printf("    run_compare_dw_conv: unable to find valid multiplier\n");
    free(input_data);
    free(kernel_data);
    free(output_data);
    free(bias_data);
    return -1;
  }

  uint32_t base_multiplier = 0;
  int base_shift = 0;
  QuantizeMultiplierSmallerThanOne(p_param->float_multiplier, &base_multiplier,
                                   &base_shift);

  // multipliers typically range in [2^30 ; 2^31 - 1].
  // Values in [0, 2^30 - 1] are normally unused, but harmless.
  // Thus a good way to randomize multipliers is to subtract from them
  // a random value smaller than 2^30 but still significant compared to it.
  uint32_t output_multiplier = base_multiplier - (rand() % (1 << 26));

  int right_shift = base_shift - 1 + (rand() % 4);
  int8_t output_rshift = truncate_rshift((int8_t)right_shift, /*allow_lshift*/1);

#ifdef ENABLE_DEBUG_MSG
  printf("      multiplier_data %d, shift_data %d\n", output_multiplier,
         output_rshift);
#endif

  p_param->multiplier = output_multiplier;
  p_param->right_shift = output_rshift;
  fully_connected_ref(p_param);

  cvk_ml_shape_t tl_input_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, left_col, CVK_FMT_I8);

  cvk_ml_shape_t tl_filter_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_col, right_col, CVK_FMT_I8);

  cvk_ml_shape_t tl_output_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, left_row, right_col, CVK_FMT_I8);

  cvk_ml_shape_t tl_bias_shape =
      cvk_ctx->ops->ml_default_shape(cvk_ctx, 4, right_col, CVK_FMT_I8);  // 32bit

  cvk_ml_t *tl_input = cvk_ctx->ops->lmem_alloc_matrix(
      cvk_ctx, tl_input_shape, CVK_FMT_I8, /*eu_align=*/1);
  cvk_ml_t *tl_filter = cvk_ctx->ops->lmem_alloc_matrix(
      cvk_ctx, tl_filter_shape, CVK_FMT_I8, /*eu_align=*/1);
  cvk_ml_t *tl_output = cvk_ctx->ops->lmem_alloc_matrix(
      cvk_ctx, tl_output_shape, CVK_FMT_I8, /*eu_align=*/1);

  cvk_ml_t *tl_bias = NULL;
  if (has_bias) {
    tl_bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, tl_bias_shape, CVK_FMT_I8,
                                          /*eu_align=*/1);
  }

  if (tl_input == NULL) {
    printf("   fail to alloc tl_input (%d, %d)\n", left_row, left_col);
    return -1;
  }
  if (tl_filter == NULL) {
    printf("    fail to alloc tl_filter (%d, %d)\n", left_col, right_col);
    return -1;
  }
  if (tl_output == NULL) {
    printf("    fail to alloc tl_output (%d, %d)\n", left_row, right_col);
    return -1;
  }
  if (has_bias && (tl_bias == NULL)) {
    printf("  fail to alloc bias (%d, %d)\n", 4, right_col);
    return -1;
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, tl_input, (uint8_t *)input_data);
  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, tl_filter, (uint8_t *)kernel_data);
  if (tl_bias) {
    put_bias32(rt_handle, cvk_ctx, tl_bias, bias_data);
  }

  {
    param_t p;
    memset(&p, 0, sizeof(p));
    p.left = tl_input;
    p.right = tl_filter;
    p.bias = tl_bias;
    p.res = tl_output;
    p.rshift_bits = (uint8_t)output_rshift;
    p.res_is_int8 = 1;
    p.ps32_mode = 0;
    p.quan_m = output_multiplier;
    cvk_ctx->ops->tiu_matrix_multiplication_qm(cvk_ctx, &p);
  }

  int8_t *tiu_output_data =
      (int8_t *)matrix_copy_l2g_d2s(rt_handle, cvk_ctx, tl_output);
#ifdef ENABLE_DEBUG_MSG
  printf("Compare tiu and ref\n");
#endif
  for (int i = 0; i < left_row; ++i) {
    for (int j = 0; j < right_col; ++j) {
      int offset = i * right_col + j;
      if (tiu_output_data[offset] != output_data[offset]) {
        printf("  output_data[%d][%d] %d(tiu) != %d(ref)\n", i, j,
               tiu_output_data[offset], output_data[offset]);
        ret = -1;
      }
    }
  }

  if (ret) {
    dump_test_param(p_param, /*dump_content=*/true);
  }

  // Reverse order
  if (tl_bias) {
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_bias);
  }

  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_output);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_filter);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, tl_input);

  free(tiu_output_data);

  free(input_data);
  free(kernel_data);
  free(output_data);
  free(bias_data);

#ifdef ENABLE_DEBUG_MSG
  printf("    <= run_compare_conv, ret %d\n", ret);
#endif

  return ret;
}

int random_test(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;

#if 0
  int left_row_range[] = {1};
  int left_col_range[] = {1};
  int right_col_range[] = {1};
#else
  int left_row_range[] =  {1, 16, 4095};
  int left_col_range[] =  {1, 16, 4095};
  int right_col_range[] = {1, 16, 4095};
#endif

  const int left_row_range_size =
      sizeof(left_row_range) / sizeof(left_row_range[0]);
  const int left_col_range_size =
      sizeof(left_col_range) / sizeof(left_col_range[0]);
  const int right_col_range_size =
      sizeof(right_col_range) / sizeof(right_col_range[0]);

  int random_seed = clock();
  srand(random_seed);

  const int retry_test_count = 100;
  bool stop_at_first_error = true;

  int executed_tests = 0;
  int failed_tests = 0;

  printf("1822-fc-qm: random test =>\n");
  for (int m = 0; m < retry_test_count; ++m) {
    for (int i = 0; i < left_row_range_size; ++i) {
      // random choosed from [range[i] : range[i+1]]
      int left_row = choose_from_range(left_row_range, left_row_range_size, i);

      for (int j = 0; j < left_col_range_size; ++j) {
        int left_col =
            choose_from_range(left_col_range, left_col_range_size, j);

        for (int k = 0; k < right_col_range_size; ++k) {
          int right_col =
              choose_from_range(right_col_range, right_col_range_size, k);

          int has_bias = rand() % 2;

          fc_test_param_t test_param;
          memset(&test_param, 0, sizeof(test_param));
          test_param.left_row = left_row;
          test_param.left_col = left_col;
          test_param.right_col = right_col;
          test_param.has_bias = has_bias;
          test_param.retry_cnt = 5;

          bool is_valid_param = check_valid_test_param(cvk_ctx, &test_param);
          if (is_valid_param == false) {
            continue;
          }

          int ret2 = run_compare_fc(rt_handle, cvk_ctx, &test_param);
          failed_tests = ret2 ? failed_tests + 1 : failed_tests;
          ret |= ret2;
          executed_tests++;

#ifdef ENABLE_DEBUG_MSG
          printf("  [%d] random test: left(%d, %d), right (%d, %d), result "
                 "%d\n",
                 executed_tests, left_row, left_col, left_col,
                 right_col, ret2);
#endif
        }

        // Stop at first error
        if (ret && stop_at_first_error) {
          break;
        }
      }

      // Stop at first error
      if (ret && stop_at_first_error) {
        break;
      }
    }

    // Stop at first error
    if (ret && stop_at_first_error) {
      break;
    }

    if (executed_tests >= MIN_EXEC_TESTS) {
      break;
    }
  }

  printf("<= 1822-fc-qm: random test, total %d, failed %d, ret %d\n",
         executed_tests, failed_tests, ret);

  return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  // ret |= tfl_original_test();
  ret |= simple_test(rt_handle, cvk_ctx);
  ret |= random_test(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
