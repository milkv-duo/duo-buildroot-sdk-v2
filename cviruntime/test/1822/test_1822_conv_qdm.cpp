#include <limits.h>
#include "1822_test_util.h"
#include "test_tf_quant_util.h"

// #define ENABLE_DEBUG_MSG
// #define ENABLE_FULL_REGRESSION
// #define ENABLE_TV_GEN_PATTERN

#define MIN_EXEC_TESTS  20

typedef struct {
  int input_n;
  int input_c;
  int input_h;
  int input_w;
  int kw;
  int kh;
  int dh;
  int dw;
  int pad_top;
  int pad_bot;
  int pad_left;
  int pad_right;
  int ins_h;
  int ins_h_last;
  int ins_w;
  int ins_w_last;
  int stride_h;
  int stride_w;
  int output_c;
  int output_h;
  int output_w;
  int has_bias;
  int relu_enable;
  s8 *input_data;
  s8 *filter_data;
  s8 *output_data;
  s32 *bias_data;
  u32 *multiplier_data;
  s8 *shift_data;
  float float_multiplier;
  int retry_cnt;
} conv_test_param_t;

inline int Offset(tl_shape_t shape, int n, int c, int h, int w)
{
  return n * (shape.c * shape.h * shape.w) + c * (shape.h * shape.w) +
         h * shape.w + w;
}

void conv_per_channel_ref(conv_test_param_t *p_param)
{
  const int stride_width = p_param->stride_w;
  const int stride_height = p_param->stride_h;
  const int dilation_width_factor = 1;
  const int dilation_height_factor = 1;
  const int pad_width = p_param->pad_left;
  const int pad_height = p_param->pad_top;

  const s32 output_activation_min = -128;
  const s32 output_activation_max = 127;

  const int batches = p_param->input_n;
  const int input_depth = p_param->input_c;
  const int output_depth = p_param->output_c;

  const int input_height = p_param->input_h;
  const int input_width = p_param->input_w;
  const int filter_height = p_param->kh;
  const int filter_width = p_param->kw;
  const int output_height = p_param->output_h;
  const int output_width = p_param->output_w;
  s8 *input_data = p_param->input_data;
  s8 *filter_data = p_param->filter_data;
  s8 *output_data = p_param->output_data;
  s32 *bias_data = p_param->has_bias ? p_param->bias_data : nullptr;
  u32 *output_multiplier = p_param->multiplier_data;
  s8 *output_rshift = p_param->shift_data;

  tl_shape_t input_shape = {
      static_cast<u32>(batches), static_cast<u32>(input_depth),
      static_cast<u32>(input_height), static_cast<u32>(input_width)};
  tl_shape_t filter_shape = {
      static_cast<u32>(output_depth), static_cast<u32>(filter_height),
      static_cast<u32>(filter_width), static_cast<u32>(input_depth)};
  tl_shape_t output_shape = {
      static_cast<u32>(batches), static_cast<u32>(output_depth),
      static_cast<u32>(output_height), static_cast<u32>(output_width)};

#ifdef ENABLE_DEBUG_MSG
  printf("conv_per_channel_ref: \n"
         "  input (n=%d, ic=%d, h=%d, w=%d)\n"
         "  kernel (oc=%d, kh=%d, kw=%d, ic=%d)\n",
         batches, input_depth, input_height, input_width, output_depth,
         filter_height, filter_width, input_depth);
#endif

  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      for (int out_x = 0; out_x < output_width; ++out_x) {
        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          const int in_x_origin = (out_x * stride_width) - pad_width;
          const int in_y_origin = (out_y * stride_height) - pad_height;
          s32 acc = 0;
          for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
            for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
              for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
                const int in_x = in_x_origin + dilation_width_factor * filter_x;
                const int in_y =
                    in_y_origin + dilation_height_factor * filter_y;
                const bool is_point_inside_image =
                    (in_x >= 0) && (in_x < input_width) && (in_y >= 0) &&
                    (in_y < input_height);
                if (is_point_inside_image) {
                  s32 input_val = input_data[Offset(input_shape, batch,
                                                    in_channel, in_y, in_x)];
                  // s32 filter_val = filter_data[Offset(filter_shape,
                  // out_channel, in_channel,
                  //                                    filter_y, filter_x)];
                  s32 filter_val =
                      filter_data[Offset(filter_shape, out_channel, filter_y,
                                         filter_x, in_channel)];
                  acc += filter_val * input_val;

#ifdef ENABLE_DEBUG_MSG
                  printf("  [batch=%d][out_channel=%d][out_y=%d][out_x=%d]"
                         "[filter_y=%d][filter_x=%d][in_channel=%d] acc(%d) += "
                         "%d * %d = %d\n",
                         batch, out_channel, out_y, out_x, filter_y, filter_x,
                         in_channel, acc - filter_val * input_val, filter_val,
                         input_val, acc);
#endif
                }
              }
            }
          }

          if (bias_data) {
            acc += bias_data[out_channel];
          }

#ifdef ENABLE_DEBUG_MSG
          printf("  [batch=%d][out_channel=%d][out_y=%d][out_x=%d] acc = %d, "
                 "bias %d\n",
                 batch, out_channel, out_y, out_x, acc,
                 bias_data ? bias_data[out_channel] : 0);
#endif

          acc = MultiplyByQuantizedMultiplier(
              acc, output_multiplier[out_channel], output_rshift[out_channel]);

#ifdef ENABLE_DEBUG_MSG
          printf("  [batch=%d][out_channel=%d][out_y=%d][out_x=%d] acc = %d, "
                 "multiplier %d, shift %d\n",
                 batch, out_channel, out_y, out_x, acc,
                 output_multiplier[out_channel], output_rshift[out_channel]);
#endif

          acc = MAX(acc, output_activation_min);
          acc = MIN(acc, output_activation_max);

#ifdef ENABLE_DEBUG_MSG
          printf("  [batch=%d][out_channel=%d][out_y=%d][out_x=%d] acc = %d\n",
                 batch, out_channel, out_y, out_x, acc);
#endif

          output_data[Offset(output_shape, batch, out_channel, out_y, out_x)] =
              static_cast<s8>(acc);
        }
      }
    }
  }
}

void calc_conv_float_multiplier(conv_test_param_t *p_param)
{
  const int stride_width = p_param->stride_w;
  const int stride_height = p_param->stride_h;
  const int dilation_width_factor = 1;
  const int dilation_height_factor = 1;
  const int pad_width = p_param->pad_left;
  const int pad_height = p_param->pad_top;

  const int batches = p_param->input_n;
  const int input_depth = p_param->input_c;
  const int output_depth = p_param->output_c;

  const int input_height = p_param->input_h;
  const int input_width = p_param->input_w;
  const int filter_height = p_param->kh;
  const int filter_width = p_param->kw;
  const int output_height = p_param->output_h;
  const int output_width = p_param->output_w;
  s8 *input_data = p_param->input_data;
  s8 *filter_data = p_param->filter_data;
  s32 *bias_data = p_param->has_bias ? p_param->bias_data : nullptr;

  tl_shape_t input_shape = {
      static_cast<u32>(batches), static_cast<u32>(input_depth),
      static_cast<u32>(input_height), static_cast<u32>(input_width)};
  tl_shape_t filter_shape = {
      static_cast<u32>(output_depth), static_cast<u32>(filter_height),
      static_cast<u32>(filter_width), static_cast<u32>(input_depth)};

  int output_accu_min = INT_MAX;
  int output_accu_max = INT_MIN;

#ifdef ENABLE_DEBUG_MSG
  printf("calc_conv_float_multiplier =>\n");
#endif
  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      for (int out_x = 0; out_x < output_width; ++out_x) {
        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          const int in_x_origin = (out_x * stride_width) - pad_width;
          const int in_y_origin = (out_y * stride_height) - pad_height;
          s32 acc = 0;
          for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
            for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
              for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
                const int in_x = in_x_origin + dilation_width_factor * filter_x;
                const int in_y =
                    in_y_origin + dilation_height_factor * filter_y;
                const bool is_point_inside_image =
                    (in_x >= 0) && (in_x < input_width) && (in_y >= 0) &&
                    (in_y < input_height);
                if (is_point_inside_image) {
                  s32 input_val = input_data[Offset(input_shape, batch,
                                                    in_channel, in_y, in_x)];
                  // s32 filter_val = filter_data[Offset(filter_shape,
                  // out_channel, in_channel,
                  //                                    filter_y, filter_x)];
                  s32 filter_val =
                      filter_data[Offset(filter_shape, out_channel, filter_y,
                                         filter_x, in_channel)];
                  acc += filter_val * input_val;

                  // printf("  [batch=%d][out_channel=%d][out_y=%d][out_x=%d]"
                  //        "[filter_y=%d][filter_x=%d][in_channel=%d] acc(%d)
                  //        += %d * %d = %d\n", batch, out_channel, out_y,
                  //        out_x, filter_y, filter_x, in_channel, acc -
                  //        filter_val * input_val, filter_val, input_val, acc);
                }
              }
            }
          }

          if (bias_data) {
            acc += bias_data[out_channel];
          }

          output_accu_max = MAX(acc, output_accu_max);
          output_accu_min = MIN(acc, output_accu_min);
        }
      }
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
  printf("<= calc_dw_conv_float_multiplier\n");
#endif
}

int simple_test(bmctx_t *ctx, bmk_ctx_t *bmk)
{
  int ret = 0;

  const int batches = 1;
  const int input_depth = 2;
  const int input_height = 2;
  const int input_width = 3;
  tl_shape_t input_shape = {batches, input_depth, input_height, input_width};
  s8 input_data[12] = {
      9,  1,   -11,  // ic = 0, h = 0
      13, 5,   -15,  // ic = 0, h = 1
      5,  -7,  -15,  // ic = 1, h = 0
      9,  -11, -19   // ic = 1, h = 1
  };

  const int output_depth = 2;
  const int kernel_height = 2;
  const int kernel_width = 2;
  tl_shape_t filter_shape = {output_depth, input_depth, kernel_height,
                             kernel_width};

  // TIU weight layout (1, oc, hw*kc, ic)
  tl_shape_t filter_shape_for_dma = {1, output_depth,
                                     kernel_height * kernel_width, input_depth};
  s8 filter_data_for_dma[16] = {
      2,  4,  6,  8,  6,  8,  10, 12,  // oc = 0
      28, 32, 20, 24, 12, 16, 4,  8    // oc = 1
  };

  s32 bias_data[2] = {12, -16};

  const int output_height = 1;
  const int output_width = 2;
  tl_shape_t output_shape = {1, output_depth, output_height, output_width};
  // zero_point = 0
  s8 ref_output_data[4] = {
      17, -128,  // oc = 0
      60, -128,  // oc = 1
  };

  u32 output_multiplier[] = {1073741824, 1073741824};
  s8 output_rshift[2] = {1, 2};  // changed to right shift

  s8 output_data[4];

  conv_test_param_t params;
  memset(&params, 0, sizeof(params));

  params.input_n = batches;
  params.input_c = input_depth;
  params.input_h = input_height;
  params.input_w = input_width;
  params.kh = kernel_height;
  params.kw = kernel_width;
  params.output_c = output_depth;
  params.output_h = output_height;
  params.output_w = output_width;
  params.stride_w = 1;
  params.stride_h = 1;
  params.input_data = input_data;
  params.filter_data = filter_data_for_dma;
  params.output_data = output_data;
  params.has_bias = 1;
  params.bias_data = bias_data;
  params.multiplier_data = output_multiplier;
  params.shift_data = output_rshift;
  conv_per_channel_ref(&params);

  printf("Compare ref and golden\n");
  for (int i = 0; i < 4; i++) {
    if (output_data[i] != ref_output_data[i]) {
      printf("Error ! output[%d]=%d != ref_output_data[%d]=%d\n", i,
             output_data[i], i, ref_output_data[i]);
      ret = -1;
    }
  }

  // tl_shape_t per_channel_cal_shape = {1, /*oc=*/2, 1, 9};
  u8 per_channel_cal_data[18];
  pack_chl_quan_param(2, /*has_bias=*/true, bias_data, output_multiplier,
                      output_rshift, per_channel_cal_data);

  bmk1822_tensor_lmem_t *tl_per_channel_cal =
      bmk1822_lmem_alloc_tensor(bmk, {1, 2, 1, 9}, FMT_U8,
                                  /*eu_align*/ 0);

  bmk1822_tensor_lmem_t *tl_input =
      bmk1822_lmem_alloc_tensor(bmk, input_shape, FMT_I8, /*eu_aign=*/1);

  bmk1822_tensor_lmem_t *tl_filter = bmk1822_lmem_alloc_tensor(
      bmk, filter_shape_for_dma, FMT_I8, /*eu_align=*/1);

  bmk1822_tensor_lmem_t *tl_output =
      bmk1822_lmem_alloc_tensor(bmk, output_shape, FMT_I8, /*eu_align=*/1);

  put_tensor_g2l(ctx, bmk, tl_per_channel_cal, per_channel_cal_data);
  put_tensor_g2l(ctx, bmk, tl_input, reinterpret_cast<u8 *>(input_data));
  put_tensor_g2l(ctx, bmk, tl_filter,
                 reinterpret_cast<u8 *>(filter_data_for_dma));

  // Restore filter shape for tiu operation
  tl_filter->shape = filter_shape;
  tl_filter->stride = bmk1822_tensor_lmem_default_stride(
      bmk, tl_filter->shape, FMT_I8, /*eu_align=*/1);

  {
    // Reshape per channel quantization data
    tl_per_channel_cal->shape = {1, 2, 1, 1};
    tl_per_channel_cal->stride = bmk1822_tensor_lmem_default_stride(
        bmk, tl_per_channel_cal->shape, FMT_I8, /*eu_align=*/0);

    bmk1822_tiu_convolution_qdm_param_t param;
    memset(&param, 0, sizeof(param));
    param.ofmap = tl_output;
    param.ifmap = tl_input;
    param.weight = tl_filter;
    param.chl_quan_param = tl_per_channel_cal;
    param.stride_h = 1;
    param.stride_w = 1;
    param.dilation_h = 1;
    param.dilation_w = 1;
    param.has_bias = 1;
    bmk1822_tiu_convolution_qdm(bmk, &param);
  }

  test_submit(ctx);

  printf("Compare tiu and golden\n");
  s8 *conv_output_data =
      reinterpret_cast<s8 *>(get_tensor_l2g(ctx, bmk, tl_output));
  for (int i = 0; i < static_cast<int>(sizeof(ref_output_data)); i++) {
    if (conv_output_data[i] != ref_output_data[i]) {
      printf("output_data[%d] %d != %d\n", i, conv_output_data[i],
             ref_output_data[i]);
      ret = -1;
    }
  }

  free(conv_output_data);

  // Reverse order
  bmk1822_lmem_free_tensor(bmk, tl_output);
  bmk1822_lmem_free_tensor(bmk, tl_filter);
  bmk1822_lmem_free_tensor(bmk, tl_input);
  bmk1822_lmem_free_tensor(bmk, tl_per_channel_cal);

  return ret;
}

void fill_random_data_s8(s8 *input_data, int size)
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

void fill_random_data_s32(s32 *input_data, int size)
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

bool check_valid_test_param(bmk_ctx_t *bk_ctx, conv_test_param_t *p_param)
{
  int in = p_param->input_n;
  int ic = p_param->input_c;
  int ih = p_param->input_h;
  int iw = p_param->input_w;
  int oc = p_param->output_c;
  int oh = p_param->output_h;
  int ow = p_param->output_w;
  int kh = p_param->kh;
  int kw = p_param->kw;
  int stride_h = p_param->stride_h;
  int stride_w = p_param->stride_w;
  int chl_quan_per_lane_data_size =
      p_param->has_bias ? 9 : 5;  // bias(4) + multiplier(4) + shift(1)

  // Skip invalid shape
  if ((kh > ih) || (kw > iw) || (stride_h > ih) || (stride_w > iw)) {
    return false;
  }

  // muliply random-choosen value may exceeded than s32
  u32 input_size = in * ic * ih * iw;
  u32 kernel_size = oc * ic * kh * kw;
  u32 output_size = in * oc * oh * ow;

  bmk1822_chip_info_t chip_info = bmk1822_chip_info();
  u32 lmem_size_per_lane = chip_info.lmem_size;
  u32 total_lmem_size = chip_info.lmem_size * chip_info.npu_num;

  u32 total_needed_size = input_size + kernel_size + output_size +
                          chl_quan_per_lane_data_size * chip_info.npu_num;
  if (total_needed_size > total_lmem_size) {
    return false;
  }

  tl_shape_t input_shape = {static_cast<u32>(in), static_cast<u32>(ic),
                            static_cast<u32>(ih), static_cast<u32>(iw)};
  tl_shape_t filter_shape = {1, static_cast<u32>(oc),
                             static_cast<u32>(kh) * static_cast<u32>(kw),
                             static_cast<u32>(ic)};
  tl_shape_t output_shape = {static_cast<u32>(in), static_cast<u32>(oc),
                             static_cast<u32>(oh), static_cast<u32>(ow)};
  tl_shape_t cal_shape = {1, static_cast<u32>(oc), 1,
                          static_cast<u32>(chl_quan_per_lane_data_size)};

  u32 needed_size =
      bmk1822_lmem_tensor_to_size(bk_ctx, input_shape, FMT_I8, /*eu_align=*/1) +
      bmk1822_lmem_tensor_to_size(bk_ctx, filter_shape, FMT_I8, /*eu_align=*/0) +
      bmk1822_lmem_tensor_to_size(bk_ctx, output_shape, FMT_I8, /*eu_align=*/1) +
      bmk1822_lmem_tensor_to_size(bk_ctx, cal_shape, FMT_I8, /*eu_align=*/0);

  // Skip invalid shape
  if (needed_size > lmem_size_per_lane) {
    return false;
  }

  return true;
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

void dump_test_param(conv_test_param_t *p_param, bool dump_content)
{
  printf("Dump test parameter:\n");
  printf("  input_n %d\n", p_param->input_n);
  printf("  input_c %d\n", p_param->input_c);
  printf("  input_h %d\n", p_param->input_h);
  printf("  input_w %d\n", p_param->input_w);
  printf("  kw %d\n", p_param->kw);
  printf("  kh %d\n", p_param->kh);
  printf("  dh %d\n", p_param->dh);
  printf("  dw %d\n", p_param->dw);
  printf("  pad_top %d\n", p_param->pad_top);
  printf("  pad_bot %d\n", p_param->pad_bot);
  printf("  pad_left %d\n", p_param->pad_left);
  printf("  pad_right %d\n", p_param->pad_right);
  printf("  ins_h %d\n", p_param->ins_h);
  printf("  ins_h_last %d\n", p_param->ins_h_last);
  printf("  ins_w %d\n", p_param->ins_w);
  printf("  ins_w_last %d\n", p_param->ins_w_last);
  printf("  stride_h %d\n", p_param->stride_h);
  printf("  stride_w %d\n", p_param->stride_w);
  printf("  output_c %d\n", p_param->output_c);
  printf("  output_h %d\n", p_param->output_h);
  printf("  output_w %d\n", p_param->output_w);
  printf("  has_bias %d\n", p_param->has_bias);
  printf("  relu_enable %d\n", p_param->relu_enable);

  if (dump_content) {
    printf("input_data(%d, %d, %d, %d) :\n", p_param->input_n, p_param->input_c,
           p_param->input_h, p_param->input_w);
    int in = p_param->input_n;
    int ic = p_param->input_c;
    int ih = p_param->input_h;
    int iw = p_param->input_w;
    for (int i = 0; i < in; ++i) {
      for (int j = 0; j < ic; ++j) {
        for (int k = 0; k < ih; ++k) {
          for (int l = 0; l < iw; ++l) {
            int offset = i * (ic * ih * iw) + j * (ih * iw) + k * iw + l;
            printf("%d, ", p_param->input_data[offset]);
          }
          printf("\n");
        }
      }
    }
    printf("\n\n");

    printf("kener_data (oc=%d, kh=%d, kw=%d, ic=%d)\n", p_param->output_c,
           p_param->kh, p_param->kw, p_param->input_c);
    int oc = p_param->output_c;
    int kh = p_param->kh;
    int kw = p_param->kw;
    for (int i = 0; i < oc; ++i) {
      for (int j = 0; j < kh; ++j) {
        for (int k = 0; k < kw; ++k) {
          for (int l = 0; l < ic; ++l) {
            int offset = i * (kh * kw * ic) + j * (kw * ic) + k * ic + l;
            printf("%d, ", p_param->filter_data[offset]);
          }
          printf("\n");
        }
      }
    }
    printf("\n\n");

    if (p_param->has_bias) {
      printf("bias_data:\n");
      for (int i = 0; i < oc; ++i) {
        printf("%d, ", p_param->bias_data[i]);
      }
      printf("\n\n");
    }

    printf("multiplier_data:\n");
    for (int i = 0; i < oc; ++i) {
      printf("%d, ", p_param->multiplier_data[i]);
    }
    printf("\n\n");

    printf("shift_data:\n");
    for (int i = 0; i < oc; ++i) {
      printf("%d, ", p_param->shift_data[i]);
    }
    printf("\n\n");
  }
}



static conv_test_param_t keepFailParam;;
static s8 *keep_input_data = NULL;

static int keep_kernel_size = 0;
static s8 *keep_kernel_data = NULL;

static int keep_output_size = 0;
static s8 *keep_output_data = NULL;

static s32 *keep_bias_data = NULL;
static u32 *keep_multiplier_data = NULL;
static s8 *keep_shift_data = NULL;


int keep_fail_param(conv_test_param_t *p_param)
{
	int in = p_param->input_n;
	int ic = p_param->input_c;
	int ih = p_param->input_h;
	int iw = p_param->input_w;
	int oc = p_param->output_c;
	int oh = p_param->output_h;
	int ow = p_param->output_w;
	int kh = p_param->kh;
	int kw = p_param->kw;
	//int dh = p_param->dh;
	//int dw = p_param->dw;
	//int pad_top = p_param->pad_top;
	//int pad_bot = p_param->pad_bot;
	//int pad_left = p_param->pad_left;
	//int pad_right = p_param->pad_right;
	//int ins_h = p_param->ins_h;
	//int ins_last_h = p_param->ins_h_last;
	//int ins_w = p_param->ins_w;
	//int ins_last_w = p_param->ins_w_last;
	//int stride_h = p_param->stride_h;
	//int stride_w = p_param->stride_w;
	int has_bias = p_param->has_bias;
	//int relu_enable = p_param->relu_enable;


	memcpy(&keepFailParam, p_param, sizeof(conv_test_param_t));

	int input_size = in * ic * iw * ih;
	keep_input_data = (s8 *)malloc(input_size);
	memcpy(keep_input_data, p_param->input_data, input_size);

	
	keep_kernel_size = oc * ic * kh * kw;
	keep_kernel_data = (s8 *)malloc(keep_kernel_size);
	memcpy(keep_kernel_data, p_param->filter_data, keep_kernel_size);
	
	keep_output_size = in * oc * oh * ow;
	keep_output_data = (s8 *)malloc(keep_output_size);
	memcpy(keep_output_data, p_param->output_data, keep_output_size);

	keep_bias_data = (s32 *) malloc(sizeof(s32) * oc);
	memcpy(keep_bias_data, p_param->bias_data, sizeof(s32) * oc);

	keep_multiplier_data = (u32 *) malloc(sizeof(u32) * oc);
	memcpy(keep_multiplier_data, p_param->multiplier_data, sizeof(s32) * oc);

	keep_shift_data = (s8 *)malloc(oc);
	memcpy(keep_shift_data, p_param->shift_data, oc);
	


	keepFailParam.input_data = keep_input_data;
	keepFailParam.filter_data = keep_kernel_data;
	keepFailParam.output_data = keep_output_data;
	keepFailParam.has_bias = has_bias;
	keepFailParam.bias_data = keep_bias_data;
	keepFailParam.multiplier_data = keep_multiplier_data;
	keepFailParam.shift_data = keep_shift_data;

	return 0;
}


void dump2_test_param(conv_test_param_t *p_param)
{
	printf("dump2_test_param:\n");
	printf("  input_n %d\n", p_param->input_n);
	printf("  input_c %d\n", p_param->input_c);
	printf("  input_h %d\n", p_param->input_h);
	printf("  input_w %d\n", p_param->input_w);
	printf("  kw %d\n", p_param->kw);
	printf("  kh %d\n", p_param->kh);
	printf("  dh %d\n", p_param->dh);
	printf("  dw %d\n", p_param->dw);
	printf("  pad_top %d\n", p_param->pad_top);
	printf("  pad_bot %d\n", p_param->pad_bot);
	printf("  pad_left %d\n", p_param->pad_left);
	printf("  pad_right %d\n", p_param->pad_right);
	printf("  ins_h %d\n", p_param->ins_h);
	printf("  ins_h_last %d\n", p_param->ins_h_last);
	printf("  ins_w %d\n", p_param->ins_w);
	printf("  ins_w_last %d\n", p_param->ins_w_last);
	printf("  stride_h %d\n", p_param->stride_h);
	printf("  stride_w %d\n", p_param->stride_w);
	printf("  output_c %d\n", p_param->output_c);
	printf("  output_h %d\n", p_param->output_h);
	printf("  output_w %d\n", p_param->output_w);
	printf("  has_bias %d\n", p_param->has_bias);
	printf("  relu_enable %d\n", p_param->relu_enable);

	keep_fail_param(p_param);
	printf("dump2_test_param\n\n");
	assert(0);
}

int run_compare_conv(bmctx_t *ctx, bmk_ctx_t *bk_ctx,
                     conv_test_param_t *p_param)
{
  int ret = 0;

  if (ctx == nullptr || bk_ctx == nullptr) {
    return -1;
  }

  int in = p_param->input_n;
  int ic = p_param->input_c;
  int ih = p_param->input_h;
  int iw = p_param->input_w;
  int oc = p_param->output_c;
  int oh = p_param->output_h;
  int ow = p_param->output_w;
  int kh = p_param->kh;
  int kw = p_param->kw;
  int dh = p_param->dh;
  int dw = p_param->dw;
  int pad_top = p_param->pad_top;
  int pad_bot = p_param->pad_bot;
  int pad_left = p_param->pad_left;
  int pad_right = p_param->pad_right;
  int ins_h = p_param->ins_h;
  int ins_last_h = p_param->ins_h_last;
  int ins_w = p_param->ins_w;
  int ins_last_w = p_param->ins_w_last;
  int stride_h = p_param->stride_h;
  int stride_w = p_param->stride_w;
  int has_bias = p_param->has_bias;
  int relu_enable = p_param->relu_enable;

  int input_size = in * ic * iw * ih;
  s8 *input_data = (s8 *)malloc(input_size);

  int kernel_size = oc * ic * kh * kw;
  s8 *kernel_data = (s8 *)malloc(kernel_size);

  int output_size = in * oc * oh * ow;
  s8 *output_data = (s8 *)malloc(output_size);
  if (!input_data || !kernel_data || !output_data) {
    free(input_data);
    free(kernel_data);
    free(output_data);
    return -1;
  }

  memset(output_data, 0, output_size);

  s32 *bias_data = (s32 *) malloc(sizeof(s32) * oc);
  u32 *multiplier_data = (u32 *) malloc(sizeof(u32) * oc);
  s8 *shift_data = (s8 *)malloc(oc);

  p_param->input_data = input_data;
  p_param->filter_data = kernel_data;
  p_param->output_data = output_data;
  p_param->has_bias = has_bias;
  p_param->bias_data = bias_data;
  p_param->multiplier_data = multiplier_data;
  p_param->shift_data = shift_data;

#ifdef ENABLE_DEBUG_MSG
  printf("    run_compare_conv =>\n");
  printf("      input (n=%d, ic=%d, h=%d, w=%d), kernel (oc=%d, ic=%d, h=%d, "
         "w=%d), output (, c=%d, h=%d, w=%d), has_bias %d\n",
         in, ic, ih, iw, oc, ic, kh, kw, oc, oh, ow, has_bias);
#endif

  int retry_cnt = p_param->retry_cnt;
  do {
    fill_random_data_s8(input_data, input_size);
    fill_random_data_s8(kernel_data, kernel_size);
    if (has_bias) {
      fill_random_data_s32(bias_data, oc);
    }

    p_param->float_multiplier = 100.0;  // should be < 1.0
    calc_conv_float_multiplier(p_param);

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
    free(multiplier_data);
    free(shift_data);

    return -1;
  }

  u32 base_multiplier = 0;
  int base_shift = 0;
  QuantizeMultiplierSmallerThanOne(p_param->float_multiplier, &base_multiplier,
                                   &base_shift);

  for (int i = 0; i < oc; ++i) {
    // multipliers typically range in [2^30 ; 2^31 - 1].
    // Values in [0, 2^30 - 1] are normally unused, but harmless.
    // Thus a good way to randomize multipliers is to subtract from them
    // a random value smaller than 2^30 but still significant compared to it.
    p_param->multiplier_data[i] = base_multiplier - (rand() % (1 << 26));

    int right_shift = base_shift - 1 + (rand() % 4);
    p_param->shift_data[i] =
        truncate_rshift((s8)right_shift, /*allow_lshift*/1);

#ifdef ENABLE_DEBUG_MSG
    printf("      [oc=%d] multiplier_data %d, shift_data %d\n", i,
           p_param->multiplier_data[i], p_param->shift_data[i]);
#endif
  }

  conv_per_channel_ref(p_param);

  const int chl_quan_per_lane_data_size =
      p_param->has_bias ? 9 : 5;  // bias(4) + multiplier(4) + shift(1)
  const int cal_data_size = oc * chl_quan_per_lane_data_size;
  u8 *chl_quan_data = (u8 *) malloc(cal_data_size);
  pack_chl_quan_param(oc, p_param->has_bias, p_param->bias_data,
                      p_param->multiplier_data, p_param->shift_data,
                      chl_quan_data);

  tl_shape_t input_shape = {static_cast<u32>(in), static_cast<u32>(ic),
                            static_cast<u32>(ih), static_cast<u32>(iw)};
  tl_shape_t filter_shape = {1, static_cast<u32>(oc),
                             static_cast<u32>(kh) * static_cast<u32>(kw),
                             static_cast<u32>(ic)};
  tl_shape_t output_shape = {static_cast<u32>(in), static_cast<u32>(oc),
                             static_cast<u32>(oh), static_cast<u32>(ow)};
  tl_shape_t cal_shape = {1, static_cast<u32>(oc), 1,
                          static_cast<u32>(chl_quan_per_lane_data_size)};

  bmk1822_tensor_lmem_t *tl_input =
      bmk1822_lmem_alloc_tensor(bk_ctx, input_shape, FMT_I8, /*eu_aign=*/1);

  bmk1822_tensor_lmem_t *tl_filter =
      bmk1822_lmem_alloc_tensor(bk_ctx, filter_shape, FMT_I8, /*eu_align=*/0);

  bmk1822_tensor_lmem_t *tl_output =
      bmk1822_lmem_alloc_tensor(bk_ctx, output_shape, FMT_I8, /*eu_align=*/1);

  // Shape for TDMA load
  bmk1822_tensor_lmem_t *tl_cal_data =
      bmk1822_lmem_alloc_tensor(bk_ctx, cal_shape, FMT_U8, /*eu_align*/ 0);

  if (!tl_input || !tl_filter || !tl_output || !tl_cal_data) {
    if (tl_input == nullptr) {
      printf("      fail to alloc tl_input (%d, %d, %d, %d)\n", input_shape.n,
            input_shape.c, input_shape.h, input_shape.w);
    }
    if (tl_filter == nullptr) {
      printf("     fail to alloc tl_filter (%d, %d, %d, %d)\n", filter_shape.n,
            filter_shape.c, filter_shape.h, filter_shape.w);
    }
    if (tl_output == nullptr) {
      printf("    fail to alloc tl_output (%d, %d, %d, %d)\n", output_shape.n,
            output_shape.c, output_shape.h, output_shape.w);
    }
    if (tl_cal_data == nullptr) {
      printf("    fail to alloc tl_cal_data (%d, %d ,%d, %d)\n", cal_shape.n,
            cal_shape.c, cal_shape.h, cal_shape.w);
    }

    // Reverse order
    if (tl_cal_data)
      bmk1822_lmem_free_tensor(bk_ctx, tl_cal_data);
    if (tl_output)
      bmk1822_lmem_free_tensor(bk_ctx, tl_output);
    if (tl_filter)
      bmk1822_lmem_free_tensor(bk_ctx, tl_filter);
    if (tl_input)
      bmk1822_lmem_free_tensor(bk_ctx, tl_input);

    return -1;
  }

  put_tensor_g2l(ctx, bk_ctx, tl_cal_data, chl_quan_data);
  put_tensor_g2l(ctx, bk_ctx, tl_input, reinterpret_cast<u8 *>(input_data));
  put_tensor_g2l(ctx, bk_ctx, tl_filter, reinterpret_cast<u8 *>(kernel_data));

  {
    // Reshape per channel quantization data for TIU
    tl_cal_data->shape = {1, static_cast<u32>(oc), 1, 1};
    tl_cal_data->stride = bmk1822_tensor_lmem_default_stride(
        bk_ctx, tl_cal_data->shape, FMT_I8, /*eu_align=*/0);

    // Reshape weight for TIU
    tl_filter->shape = {static_cast<u32>(ic), static_cast<u32>(oc),
                        static_cast<u32>(kh), static_cast<u32>(kw)};

    bmk1822_tiu_convolution_qdm_param_t param;
    memset(&param, 0, sizeof(param));
    param.ofmap = tl_output;
    param.ifmap = tl_input;
    param.weight = tl_filter;
    param.chl_quan_param = tl_cal_data;
    param.ins_h = ins_h;
    param.ins_last_h = ins_last_h;
    param.ins_w = ins_w;
    param.ins_last_w = ins_last_w;
    param.stride_h = stride_h;
    param.stride_w = stride_w;
    param.dilation_h = dh;
    param.dilation_w = dw;
    param.pad_top = pad_top;
    param.pad_bottom = pad_bot;
    param.pad_left = pad_left;
    param.pad_right = pad_right;
    param.has_bias = has_bias;
    param.relu_enable = relu_enable;

#ifdef ENABLE_DEBUG_MSG
    printf("      tiu_conv_qdm:\n");
    printf("        ifmap shape (%d, %d, %d, %d)\n", param.ifmap->shape.n,
           param.ifmap->shape.c, param.ifmap->shape.h, param.ifmap->shape.w);
    printf("        weight shape (%d, %d, %d, %d)\n", param.weight->shape.n,
           param.weight->shape.c, param.weight->shape.h, param.weight->shape.w);
    printf("        ofmap shape (%d, %d, %d, %d)\n", param.ofmap->shape.n,
           param.ofmap->shape.c, param.ofmap->shape.h, param.ofmap->shape.w);
#endif

    bmk1822_tiu_convolution_qdm(bk_ctx, &param);
  }

  test_submit(ctx);

#ifdef ENABLE_DEBUG_MSG
  printf("      compare result:\n");
#endif
  s8 *conv_output_data =
      reinterpret_cast<s8 *>(get_tensor_l2g(ctx, bk_ctx, tl_output));
  for (int i = 0; i < in; ++i) {
    for (int j = 0; j < oc; ++j) {
      for (int k = 0; k < oh; ++k) {
        for (int l = 0; l < ow; ++l) {
          int offset = i * (oc * oh * ow) + j * (oh * ow) + k * ow + l;
          if (conv_output_data[offset] != output_data[offset]) {
            printf("        [ni=%d][oci=%d][ohi=%d][owi=%d] output %d(tiu) != "
                   "%d(ref)\n",
                   i, j, k, l, conv_output_data[offset], output_data[offset]);
            ret = -1;
            break;
          }
        }
      }
    }
  }

  if (ret) {
    //dump_test_param(p_param, /*dump_content=*/true);
	dump2_test_param(p_param);
  }

  // Reverse order
  bmk1822_lmem_free_tensor(bk_ctx, tl_cal_data);
  bmk1822_lmem_free_tensor(bk_ctx, tl_output);
  bmk1822_lmem_free_tensor(bk_ctx, tl_filter);
  bmk1822_lmem_free_tensor(bk_ctx, tl_input);

  free(conv_output_data);

  free(input_data);
  free(kernel_data);
  free(output_data);
  free(bias_data);
  free(multiplier_data);
  free(shift_data);
  free(chl_quan_data);

#ifdef ENABLE_DEBUG_MSG
  printf("    <= run_compare_conv\n");
#endif

  return ret;
}




int run2_compare_conv(bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int ret = 0;

  if (ctx == nullptr || bk_ctx == nullptr) {
    return -1;
  }

  conv_test_param_t *p_param = &keepFailParam; 

  int in = p_param->input_n;
  int ic = p_param->input_c;
  int ih = p_param->input_h;
  int iw = p_param->input_w;
  int oc = p_param->output_c;
  int oh = p_param->output_h;
  int ow = p_param->output_w;
  int kh = p_param->kh;
  int kw = p_param->kw;
  int dh = p_param->dh;
  int dw = p_param->dw;
  int pad_top = p_param->pad_top;
  int pad_bot = p_param->pad_bot;
  int pad_left = p_param->pad_left;
  int pad_right = p_param->pad_right;
  int ins_h = p_param->ins_h;
  int ins_last_h = p_param->ins_h_last;
  int ins_w = p_param->ins_w;
  int ins_last_w = p_param->ins_w_last;
  int stride_h = p_param->stride_h;
  int stride_w = p_param->stride_w;
  int has_bias = p_param->has_bias;
  int relu_enable = p_param->relu_enable;

  int input_size = in * ic * iw * ih;
  s8 *input_data = (s8 *)malloc(input_size);

  int kernel_size = oc * ic * kh * kw;
  s8 *kernel_data = (s8 *)malloc(kernel_size);

  int output_size = in * oc * oh * ow;
  s8 *output_data = (s8 *)malloc(output_size);
  if (!input_data || !kernel_data || !output_data) {
    free(input_data);
    free(kernel_data);
    free(output_data);
    return -1;
  }

  memset(output_data, 0, output_size);

  s32 *bias_data = (s32 *) malloc(sizeof(s32) * oc);
  u32 *multiplier_data = (u32 *) malloc(sizeof(u32) * oc);
  s8 *shift_data = (s8 *)malloc(oc);

  //p_param->input_data = input_data;
  //p_param->filter_data = kernel_data;
  //p_param->output_data = output_data;
  //p_param->has_bias = has_bias;
  //p_param->bias_data = bias_data;
  //p_param->multiplier_data = multiplier_data;
  //p_param->shift_data = shift_data;

#ifdef ENABLE_DEBUG_MSG
  printf("    run_compare_conv =>\n");
  printf("      input (n=%d, ic=%d, h=%d, w=%d), kernel (oc=%d, ic=%d, h=%d, "
         "w=%d), output (, c=%d, h=%d, w=%d), has_bias %d\n",
         in, ic, ih, iw, oc, ic, kh, kw, oc, oh, ow, has_bias);
#endif

  int retry_cnt = p_param->retry_cnt;
  do {
    fill_random_data_s8(input_data, input_size);
    fill_random_data_s8(kernel_data, kernel_size);
    if (has_bias) {
      fill_random_data_s32(bias_data, oc);
    }

    p_param->float_multiplier = 100.0;  // should be < 1.0
    calc_conv_float_multiplier(p_param);

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
    free(multiplier_data);
    free(shift_data);

    return -1;
  }

  u32 base_multiplier = 0;
  int base_shift = 0;
  QuantizeMultiplierSmallerThanOne(p_param->float_multiplier, &base_multiplier,
                                   &base_shift);

  for (int i = 0; i < oc; ++i) {
    // multipliers typically range in [2^30 ; 2^31 - 1].
    // Values in [0, 2^30 - 1] are normally unused, but harmless.
    // Thus a good way to randomize multipliers is to subtract from them
    // a random value smaller than 2^30 but still significant compared to it.
    p_param->multiplier_data[i] = base_multiplier - (rand() % (1 << 26));

    // Our H/W only supports right shift
    int right_shift = base_shift - 1 + (rand() % 4);
    p_param->shift_data[i] = right_shift > 0 ? right_shift : 0;

#ifdef ENABLE_DEBUG_MSG
    printf("      [oc=%d] multiplier_data %d, shift_data %d\n", i,
           p_param->multiplier_data[i], p_param->shift_data[i]);
#endif
  }

  conv_per_channel_ref(p_param);

  const int chl_quan_per_lane_data_size =
      p_param->has_bias ? 9 : 5;  // bias(4) + multiplier(4) + shift(1)
  const int cal_data_size = oc * chl_quan_per_lane_data_size;
  u8 *chl_quan_data = (u8 *) malloc(cal_data_size);
  pack_chl_quan_param(oc, p_param->has_bias, p_param->bias_data,
                      p_param->multiplier_data, p_param->shift_data,
                      chl_quan_data);

  tl_shape_t input_shape = {static_cast<u32>(in), static_cast<u32>(ic),
                            static_cast<u32>(ih), static_cast<u32>(iw)};
  tl_shape_t filter_shape = {1, static_cast<u32>(oc),
                             static_cast<u32>(kh) * static_cast<u32>(kw),
                             static_cast<u32>(ic)};
  tl_shape_t output_shape = {static_cast<u32>(in), static_cast<u32>(oc),
                             static_cast<u32>(oh), static_cast<u32>(ow)};
  tl_shape_t cal_shape = {1, static_cast<u32>(oc), 1,
                          static_cast<u32>(chl_quan_per_lane_data_size)};

  bmk1822_tensor_lmem_t *tl_input =
      bmk1822_lmem_alloc_tensor(bk_ctx, input_shape, FMT_I8, /*eu_aign=*/1);

  bmk1822_tensor_lmem_t *tl_filter =
      bmk1822_lmem_alloc_tensor(bk_ctx, filter_shape, FMT_I8, /*eu_align=*/0);

  bmk1822_tensor_lmem_t *tl_output =
      bmk1822_lmem_alloc_tensor(bk_ctx, output_shape, FMT_I8, /*eu_align=*/1);

  // Shape for TDMA load
  bmk1822_tensor_lmem_t *tl_cal_data =
      bmk1822_lmem_alloc_tensor(bk_ctx, cal_shape, FMT_U8, /*eu_align*/ 0);

  if (!tl_input || !tl_filter || !tl_output || !tl_cal_data) {
    if (tl_input == nullptr) {
      printf("      fail to alloc tl_input (%d, %d, %d, %d)\n", input_shape.n,
            input_shape.c, input_shape.h, input_shape.w);
    }
    if (tl_filter == nullptr) {
      printf("     fail to alloc tl_filter (%d, %d, %d, %d)\n", filter_shape.n,
            filter_shape.c, filter_shape.h, filter_shape.w);
    }
    if (tl_output == nullptr) {
      printf("    fail to alloc tl_output (%d, %d, %d, %d)\n", output_shape.n,
            output_shape.c, output_shape.h, output_shape.w);
    }
    if (tl_cal_data == nullptr) {
      printf("    fail to alloc tl_cal_data (%d, %d ,%d, %d)\n", cal_shape.n,
            cal_shape.c, cal_shape.h, cal_shape.w);
    }

    // Reverse order
    if (tl_cal_data)
      bmk1822_lmem_free_tensor(bk_ctx, tl_cal_data);
    if (tl_output)
      bmk1822_lmem_free_tensor(bk_ctx, tl_output);
    if (tl_filter)
      bmk1822_lmem_free_tensor(bk_ctx, tl_filter);
    if (tl_input)
      bmk1822_lmem_free_tensor(bk_ctx, tl_input);

    return -1;
  }

  put_tensor_g2l(ctx, bk_ctx, tl_cal_data, chl_quan_data);
  put_tensor_g2l(ctx, bk_ctx, tl_input, reinterpret_cast<u8 *>(input_data));
  put_tensor_g2l(ctx, bk_ctx, tl_filter, reinterpret_cast<u8 *>(kernel_data));

  {
    // Reshape per channel quantization data for TIU
    tl_cal_data->shape = {1, static_cast<u32>(oc), 1, 1};
    tl_cal_data->stride = bmk1822_tensor_lmem_default_stride(
        bk_ctx, tl_cal_data->shape, FMT_I8, /*eu_align=*/0);

    // Reshape weight for TIU
    tl_filter->shape = {static_cast<u32>(ic), static_cast<u32>(oc),
                        static_cast<u32>(kh), static_cast<u32>(kw)};

    bmk1822_tiu_convolution_qdm_param_t param;
    memset(&param, 0, sizeof(param));
    param.ofmap = tl_output;
    param.ifmap = tl_input;
    param.weight = tl_filter;
    param.chl_quan_param = tl_cal_data;
    param.ins_h = ins_h;
    param.ins_last_h = ins_last_h;
    param.ins_w = ins_w;
    param.ins_last_w = ins_last_w;
    param.stride_h = stride_h;
    param.stride_w = stride_w;
    param.dilation_h = dh;
    param.dilation_w = dw;
    param.pad_top = pad_top;
    param.pad_bottom = pad_bot;
    param.pad_left = pad_left;
    param.pad_right = pad_right;
    param.has_bias = has_bias;
    param.relu_enable = relu_enable;

#ifdef ENABLE_DEBUG_MSG
    printf("      tiu_conv_qdm:\n");
    printf("        ifmap shape (%d, %d, %d, %d)\n", param.ifmap->shape.n,
           param.ifmap->shape.c, param.ifmap->shape.h, param.ifmap->shape.w);
    printf("        weight shape (%d, %d, %d, %d)\n", param.weight->shape.n,
           param.weight->shape.c, param.weight->shape.h, param.weight->shape.w);
    printf("        ofmap shape (%d, %d, %d, %d)\n", param.ofmap->shape.n,
           param.ofmap->shape.c, param.ofmap->shape.h, param.ofmap->shape.w);
#endif

    bmk1822_tiu_convolution_qdm(bk_ctx, &param);
  }

  test_submit(ctx);

#ifdef ENABLE_DEBUG_MSG
  printf("      compare result:\n");
#endif
  s8 *conv_output_data =
      reinterpret_cast<s8 *>(get_tensor_l2g(ctx, bk_ctx, tl_output));
  for (int i = 0; i < in; ++i) {
    for (int j = 0; j < oc; ++j) {
      for (int k = 0; k < oh; ++k) {
        for (int l = 0; l < ow; ++l) {
          int offset = i * (oc * oh * ow) + j * (oh * ow) + k * ow + l;
          if (conv_output_data[offset] != output_data[offset]) {
            printf("        [ni=%d][oci=%d][ohi=%d][owi=%d] output %d(tiu) != "
                   "%d(ref)\n",
                   i, j, k, l, conv_output_data[offset], output_data[offset]);
            ret = -1;
            break;
          }
        }
      }
    }
  }

  if (ret) {
    //dump_test_param(p_param, /*dump_content=*/true);
    dump2_test_param(p_param);
  }

  // Reverse order
  bmk1822_lmem_free_tensor(bk_ctx, tl_cal_data);
  bmk1822_lmem_free_tensor(bk_ctx, tl_output);
  bmk1822_lmem_free_tensor(bk_ctx, tl_filter);
  bmk1822_lmem_free_tensor(bk_ctx, tl_input);

  free(conv_output_data);

  free(input_data);
  free(kernel_data);
  free(output_data);
  free(bias_data);
  free(multiplier_data);
  free(shift_data);
  free(chl_quan_data);

#ifdef ENABLE_DEBUG_MSG
  printf("    <= run_compare_conv\n");
#endif

  return ret;
}

int random_test(bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int ret = 0;

#ifndef ENABLE_FULL_REGRESSION
#ifndef ENABLE_TV_GEN_PATTERN
  // n: 12b, c: 12b, h: 12b(4095-32), wb: 12b(4095-32)
  int batch_range[] = {1, 1, 2, 4095 - 32};
  int input_height_range[] = {1, 512, 1024, 4095 - 32};
  int input_width_range[] = {1, 512, 1024, 4095 - 32};
  int input_depth_range[] = {1, 16, 32, 64, 102, 4095};
  int output_depth_range[] = {1, 16, 32, 64, 128, 4095};

  // h: 12b, w: 12b
  // stride_h: 4b, strid_w: 4b
  int kernel_height_range[] = {1, 11, 2048, 4095};
  int kernel_width_range[] = {1, 11, 2048, 4095};
  int kernel_stride_height_range[] = {1, 5, 16, 31};
  int kernel_stride_width_range[] = {1, 5, 16, 31};
#else
  // TV_GEN pattern
  // Random Test, total 19683, skipped 118066, executed 32, failed 0, ret 0

  // n: 12b, c: 12b, h: 12b(4095-32), wb: 12b(4095-32)
  int batch_range[] = {1, 1, 32};
  int input_height_range[] = {1, 512, 4095 - 32};
  int input_width_range[] = {1, 512, 4095 - 32};
  int input_depth_range[] = {1, 16, 32, 64, 102, 4095};
  int output_depth_range[] = {1, 16, 32, 64, 1024, 4095};

  // h: 12b, w: 12b
  // stride_h: 4b, strid_w: 4b
  int kernel_height_range[] = {1, 11, 2048, 4095};
  int kernel_width_range[] = {1, 11, 2048, 4095};
  int kernel_stride_height_range[] = {1, 5, 16, 31};
  int kernel_stride_width_range[] = {1, 5, 16, 31};

#endif //ENABLE_TV_GEN_PATTERN
#else
#if 0
  // Input with same range size
  int batch_range[] = {1};
  int input_height_range[] = {1};
  int input_width_range[] = {1};
  int input_depth_range[] = {1};
  const int input_range_size = sizeof(input_height_range)/sizeof(input_height_range[0]);

  // Kernel with same range size
  int kernel_height_range[] = {1};
  int kernel_width_range[] = {1};
  int kernel_stride_height_range[] = {1};
  int kernel_stride_width_range[] = {1};
  int output_depth_range[] = {1};
  const int kernel_range_size = sizeof(kernel_height_range)/sizeof(kernel_height_range[0]);
#else
  // 10/21/2019 overnight
  // total 20480000, skipped 20301713, executed 178287, failed 0

  // n: 12b, c: 12b, h: 12b(4095-32), wb: 12b(4095-32)
  int batch_range[] = {1, 2, 4, 8, 16, 32, 64, 4095 - 32};
  int input_height_range[] = {1, 3, 11, 128, 512, 1024, 2048, 4095 - 32};
  int input_width_range[] = {1, 3, 11, 128, 512, 1024, 2048, 4095 - 32};
  int input_depth_range[] = {1, 3, 11, 32, 64, 1024, 2048, 4095};
  int output_depth_range[] = {1, 3, 11, 32, 64, 1024, 2048, 4095};

  // h: 12b, w: 12b
  // stride_h: 4b, strid_w: 4b
  int kernel_height_range[] = {1, 3, 11, 511, 4095};
  int kernel_width_range[] = {1, 3, 11, 511, 4095};
  int kernel_stride_height_range[] = {1, 3, 5, 7, 15, 16, 31};
  int kernel_stride_width_range[] = {1, 3, 5, 7, 15, 16, 31};
#endif
#endif /* ENABLE_FULL_REGRESSION */

  const int batch_range_size = sizeof(batch_range) / sizeof(batch_range[0]);
  const int input_height_range_size =
      sizeof(input_height_range) / sizeof(input_height_range[0]);
  const int input_width_range_size =
      sizeof(input_width_range) / sizeof(input_width_range[0]);
  const int input_depth_range_size =
      sizeof(input_depth_range) / sizeof(input_depth_range[0]);
  const int output_depth_range_size =
      sizeof(output_depth_range) / sizeof(output_depth_range[0]);

  const int kernel_height_range_size =
      sizeof(kernel_height_range) / sizeof(kernel_height_range[0]);
  const int kernel_width_range_size =
      sizeof(kernel_width_range) / sizeof(kernel_width_range[0]);
  const int kernel_stride_height_range_size =
      sizeof(kernel_stride_height_range) /
      sizeof(kernel_stride_height_range[0]);
  const int kernel_stride_width_range_size =
      sizeof(kernel_stride_width_range) / sizeof(kernel_stride_width_range[0]);

  int random_seed = clock();
  srand(random_seed);

  const int retry_test_count = 100;

  bool stop_at_first_error = true;

  int total_tests = batch_range_size * input_depth_range_size *
                    input_height_range_size * input_width_range_size *
                    output_depth_range_size * kernel_height_range_size *
                    kernel_width_range_size * kernel_stride_height_range_size *
                    kernel_stride_width_range_size;
  int skipped_tests = 0;
  int executed_tests = 0;
  int failed_tests = 0;
  int current_test = 0;

  printf("Random Test =>\n");
  for (int m = 0; m < retry_test_count; ++m) {
    for (int i = 0; i < batch_range_size; ++i) {
      // random choosed from [range[i] : range[i+1]]
      int batch = choose_from_range(batch_range, batch_range_size, i);

      for (int j = 0; j < input_height_range_size; ++j) {
        int input_height =
            choose_from_range(input_height_range, input_height_range_size, j);

        for (int k = 0; k < input_width_range_size; ++k) {
          int input_width =
              choose_from_range(input_width_range, input_width_range_size, k);

          for (int l = 0; l < input_depth_range_size; ++l) {
            int input_depth =
                choose_from_range(input_depth_range, input_depth_range_size, k);

            for (int m = 0; m < kernel_height_range_size; ++m) {
              int kernel_height = choose_from_range(
                  kernel_height_range, kernel_height_range_size, m);

              for (int n = 0; n < kernel_width_range_size; ++n) {
                int kernel_width = choose_from_range(
                    kernel_width_range, kernel_width_range_size, n);

                for (int x = 0; x < kernel_stride_height_range_size; ++x) {
                  int kernel_stride_height =
                      choose_from_range(kernel_stride_height_range,
                                        kernel_stride_height_range_size, x);

                  for (int y = 0; y < kernel_stride_width_range_size; ++y) {
                    int kernel_stride_width =
                        choose_from_range(kernel_stride_width_range,
                                          kernel_stride_width_range_size, y);

                    for (int z = 0; z < output_depth_range_size; ++z) {
                      int output_depth = choose_from_range(
                          output_depth_range, output_depth_range_size, y);

                      current_test++;

                      int has_bias = rand() % 2;
                      int dh = 1;
                      int dw = 1;
                      int ins_h = 0;
                      int ins_h_last = 0;
                      int ins_w = 0;
                      int ins_w_last = 0;
                      int pad_top = 0;
                      int pad_bot = 0;
                      int pad_left = 0;
                      int pad_right = 0;

                      int ih_ext = calc_dilute_hw(input_height, ins_h,
                                                  ins_h_last, pad_top, pad_bot);
                      int iw_ext = calc_dilute_hw(
                          input_width, ins_w, ins_w_last, pad_left, pad_right);
                      int kh_ext =
                          calc_dilute_hw(kernel_height, dh - 1, 0, 0, 0);
                      int kw_ext =
                          calc_dilute_hw(kernel_width, dw - 1, 0, 0, 0);

                      int oh =
                          calc_output_hw(ih_ext, kh_ext, kernel_stride_height);
                      int ow =
                          calc_output_hw(iw_ext, kw_ext, kernel_stride_width);

                      conv_test_param_t test_param;
                      memset(&test_param, 0, sizeof(test_param));
                      test_param.input_n = batch;
                      test_param.input_c = input_depth;
                      test_param.input_h = input_height;
                      test_param.input_w = input_width;
                      test_param.kh = kernel_height;
                      test_param.kw = kernel_width;
                      test_param.dh = dh;
                      test_param.dw = dw;
                      test_param.pad_top = pad_top;
                      test_param.pad_bot = pad_bot;
                      test_param.pad_left = pad_left;
                      test_param.pad_right = pad_right;
                      test_param.ins_h = ins_h;
                      test_param.ins_h_last = ins_h_last;
                      test_param.ins_w = ins_w;
                      test_param.ins_w_last = ins_w_last;
                      test_param.stride_h = kernel_stride_height;
                      test_param.stride_w = kernel_stride_width;
                      test_param.output_c = output_depth;
                      test_param.output_h = oh;
                      test_param.output_w = ow;
                      test_param.has_bias = has_bias;
                      test_param.retry_cnt = 5;

                      bool is_valid_param =
                          check_valid_test_param(bk_ctx, &test_param);
                      if (is_valid_param == false) {
                        skipped_tests++;
                        continue;
                      }

                      int ret2 = run_compare_conv(ctx, bk_ctx, &test_param);
                      failed_tests = ret2 ? failed_tests + 1 : failed_tests;
                      ret |= ret2;
                      executed_tests++;

#ifdef ENABLE_DEBUG_MSG
                      printf(
                          "  [%d/%d] random test: input shape(%d, %d, %d, %d)",
                          current_test, total_tests, batch, input_depth,
                          input_height, input_width);
                      printf(", kernel shape (%d, %d, %d, %d), result %d\n",
                             output_depth, input_depth, kernel_height,
                             kernel_width, ret2);
#endif

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
    }
    // Stop at first error
    if (ret && stop_at_first_error) {
      break;
    }

    if (executed_tests >= MIN_EXEC_TESTS) {
      break;
    }
  }

  printf(
      "<= Random Test, total %d, skipped %d, executed %d, failed %d, ret %d\n",
      total_tests, skipped_tests, executed_tests, failed_tests, ret);

  return ret;
}

int main()
{
  int ret = 0;
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  ret |= simple_test(&ctx, bk_ctx);
  ret |= random_test(&ctx, bk_ctx);

  test_exit(&ctx);

  return ret;
}
