#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include "test_cvikernel_util.h"
#include "test_tf_quant_util.h"
#include "test_native_ref.h"

//#define DUMP_MSG

#define TEST_CASE_NAME    "test_cv1880v2_conv"

typedef enum {
  NCDHW_N = 0,
  NCDHW_C = 1,
  NCDHW_D = 2,
  NCDHW_H = 3,
  NCDHW_W = 4,
  NCDHW_MAX_DIMS
} NCDHW_DIMS;

typedef enum {
  SPATIAL_D = 0,
  SPATIAL_H = 1,
  SPATIAL_W = 2,
  SPATIAL_MAX_DIMS
} SPATIAL_DIMS;

typedef struct {
  int input_shapes[5];
  int output_shapes[5];
  int weight_shapes[5];
  int bias_shapes[5];
  int weight_strides[SPATIAL_MAX_DIMS];
  int dilations[SPATIAL_MAX_DIMS];
  int paddings[6]; // depth[2], top, bottom, left, right
  void *input_data;
  void *output_data;
  void *weight_data;
  void *bias_data;
  void *ref_output_data;
  cvk_fmt_t data_format;
} conv3d_test_param_t;

static void permute5d(float *dst, float *src, int src_shapes[5], int orders[5])
{
  assert((orders[0] < 5) && (orders[1] < 5) && (orders[2] < 5) &&
         (orders[3] < 5) && (orders[4] < 5) && "Expect 5d permute");

  int dst_shapes[5] = {
      src_shapes[orders[0]], src_shapes[orders[1]], src_shapes[orders[2]],
      src_shapes[orders[3]], src_shapes[orders[4]]};

  // logical stride, in unit of float
  int src_strides[5], dst_strides[5];
  get_strides_from_shapes5d(src_strides, src_shapes, 1);
  get_strides_from_shapes5d(dst_strides, dst_shapes, 1);

  for (int i = 0; i < src_shapes[0]; i++) {
    for (int j = 0; j < src_shapes[1]; j++) {
      for (int z = 0; z < src_shapes[2]; z++) {
        for (int y = 0; y < src_shapes[3]; y++) {
          for (int x = 0; x < src_shapes[4]; x++) {
            int src_poss[5] = {i, j, z, y, x};
            int dst_poss[5] = {
                src_poss[orders[0]], src_poss[orders[1]], src_poss[orders[2]],
                src_poss[orders[3]], src_poss[orders[4]]};
            int src_offset = get_tensor5d_offset(src_poss, src_strides);
            int dst_offset = get_tensor5d_offset(dst_poss, dst_strides);
            dst[dst_offset] = src[src_offset];
          }
        }
      }
    }
  }
}

static void convert_ncdhw_to_ndchw(float *dst, float *src, int src_shapes[5])
{
  // Permute
  //    0  1  2  3  4      0  2  1  3  4
  //   (n, c, d, h, w) -> (n, d, c, h, w)
  int orders[5] = {0, 2, 1, 3, 4};
  permute5d(dst, src, src_shapes, orders);
}

static void convert_tpu_weight_for_ncdhw(
    float *tpu_weight, float cpu_weight[5],
    int cpu_shapes[5])
{
  //           0   1   2   3   4       2   0   3   4   1
  //           N   C   D   H   W       D   N   H   W   C
  // Permute (oc, ic, kd, kh, kw) -> (kd, oc, kh, kw, ic)
  int orders[5] = {2, 0, 3, 4, 1};
  permute5d(tpu_weight, cpu_weight, cpu_shapes, orders);
}

void dumpFloatData(float *data, int shapes[5])
{
  int strides[5];

  // logical stride, in unit of float
  get_strides_from_shapes5d(strides, shapes, 1);

  printf("%s (%d, %d, %d, %d, %d)=>\n",
        __FUNCTION__, shapes[0], shapes[1], shapes[2], shapes[3], shapes[4]);

  for (int i = 0; i < shapes[NCDHW_N]; i++) {
    for (int j = 0; j < shapes[NCDHW_C]; j++) {
      for (int z = 0; z < shapes[NCDHW_D]; z++) {
        for (int y = 0; y < shapes[NCDHW_H]; y++) {
          printf("  [n=%d][c=%d][d=%d][h=%d] ", i, j, z, y);
          for (int x = 0; x < shapes[NCDHW_W]; x++) {
            int poss[5] = {i, j, z, y, x};
            int offset = get_tensor5d_offset(poss, strides);
            printf("%f ", data[offset]);
          }
          printf("\n");
        }
      }
    }
  }

  printf("<= %s\n", __FUNCTION__);

}

static uint32_t addr_after_right_shift(
    cvk_context_t *cvk_ctx, int addr, uint32_t step, int c_str)
{
  uint32_t npu_num = cvk_ctx->info.npu_num;
  uint32_t lmem_size = cvk_ctx->info.lmem_size; 

  uint32_t lmem_i = (addr / lmem_size + step) % npu_num;
  uint32_t offset = addr % lmem_size + (lmem_i + step) / npu_num * c_str;
  return lmem_i * lmem_size + offset;
}

// input (n, ic, id, ih, iw)
// output (n, oc, od, oh, ow)
// weight (oc, kd, kh, kw, ic)
void conv3d_float_ref_for_ncdhw(
  float *input, float *weight, float *bias, float *output,
  int batch, int input_c, int input_d, int input_h, int input_w,
  int output_c, int output_d, int output_h, int output_w,
  int kernel_d, int kernel_h, int kernel_w,
  int stride_d, int stride_h, int stride_w,
  int dilation_d, int dilation_h, int dilation_w,
  int pad_d0, int pad_d1,
  int pad_top, int pad_bottom, int pad_left, int pad_right) {
  (void)pad_d1;
  (void)pad_bottom;
  (void)pad_right;

  int input_shapes[5] = {batch, input_c, input_d, input_h, input_w};
  int output_shapes[5] = {batch, output_c, output_d, output_h, output_w};
  int kernel_shapes[5] = {output_c, kernel_d, kernel_h, kernel_w, input_c};
  int input_strides[5];
  int output_strides[5];
  int kernel_strides[5];

  // input/output shape (n, c, d, h, w)
  get_strides_from_shapes5d(input_strides, input_shapes, sizeof(float));
  get_strides_from_shapes5d(output_strides, output_shapes, sizeof(float));

  // kernel shape (oc, kd, kh, kw, kc)
  get_strides_from_shapes5d(kernel_strides, kernel_shapes, sizeof(float));

#ifdef DUMP_MSG
  printf("  %s =>\n", __FUNCTION__);
#endif

  for (int i = 0; i < batch; ++i) {
    for (int oc = 0; oc < output_c; oc++) {
      for (int oz = 0; oz < output_d; oz++) {
        for (int oy = 0; oy < output_h; ++oy) {
          for (int ox = 0; ox < output_w; ++ox) {
            for (int ic = 0; ic < input_c; ++ic) {
              for (int kz = 0; kz < kernel_d; ++kz) {
                const int iz = oz * stride_d + kz * dilation_d - pad_d0;

#ifdef DUMP_MSG
                printf("    [i=%d][oc=%d][oz=%d][oy=%d][ox=%d][ic=%d][kz=%d]" \
                       "iz= %d = %d(oz) * %d(stride_depth) + "\
                       "%d(kz) * %d(dilation_depth) - %d(padding_d_start)\n",
                      i, oc, oz, oy, ox, ic, kz,
                      iz, oz, stride_d, kz, dilation_d,
                      pad_d0);
#endif

                if (iz >= 0 && iz < input_d) {
                  for (int ky = 0; ky < kernel_h; ++ky) {
                    const int iy = oy * stride_h + ky * dilation_h - pad_top;
                    if (iy >= 0 && iy < input_h) {
                      for (int kx = 0; kx < kernel_w; ++kx) {
                        const int ix = ox * stride_w + kx * dilation_w - pad_left;
                        if (ix >= 0 && ix < input_w) {
                          int input_poss[5] = {i, ic, iz, iy, ix};
                          int input_offset =
                              get_tensor5d_offset(input_poss, input_strides)
                                  / input_strides[5 - 1];

                          // pytorch (Oc=1, Id=1, Ic=1, kh=3, kw=3)
                          int kernel_poss[5] = {oc, ic, kz, ky, kx};

                          int kernel_offset =
                            get_tensor5d_offset(kernel_poss, kernel_strides)
                                / kernel_strides[5 - 1];

                          int output_poss[5] = {i, oc, oz, oy, ox};
                          int output_offset =
                            get_tensor5d_offset(output_poss, output_strides)
                                / output_strides[5 - 1];

                          output[output_offset] +=
                            input[input_offset] * weight[kernel_offset];

#ifdef DUMP_MSG
                          printf("      [n=%d][oc=%d][oz=%d][oh=%d][ow=%d]" \
                                 "[ic=%d][kz=%d[ky=%d][kx=%d] output[%d](%f) "\
                                 "+= input[n=%d][ic=%d][iz=%d][iy=%d][ix=%d]"\
                                 "[%d](%f) * weight[oc=%d][ic=%d][kz=%d]"\
                                 "[ky=%d][kx=%d][%d](%f) = %f\n",
                              i, oc, oz, oy, ox, ic, kz, ky, kx, output_offset,
                              output[output_offset] -
                              input[input_offset] * weight[kernel_offset],
                              i, ic, iz, iy, ix,
                              input_offset, input[input_offset],
                              oc, ic, kz, ky, kx,
                              kernel_offset, weight[kernel_offset],
                              output[output_offset]);
#endif

                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (int i = 0; i < batch; ++i) {
    for (int oy = 0; oy < output_h; ++oy) {
      for (int ox = 0; ox < output_w; ++ox) {
        for (int oc = 0; oc < output_c; ++oc) {
          for (int od = 0; od < output_d; ++od) {
            int output_poss[5] = {i, oc, od, oy, ox};
            int output_offset =
                get_tensor5d_offset(output_poss, output_strides)
                    / output_strides[5 - 1];
            output[output_offset] += bias[oc];

#ifdef DUMP_MSG
            printf("    [n=%d][oy=%d][ox=%d][oc=%d][od=%d] output[%d](%f)" \
                   " += bias(%f) = %f\n",
                   i, oy, ox, oc, od, output_offset,
                   output[output_offset] - bias[oc], bias[oc],
                   output[output_offset]);
#endif

          }
        }
      }
    }
  }

#ifdef DUMP_MSG
  printf("  <= %s\n", __FUNCTION__);
#endif

}

void conv3d_float_ref(conv3d_test_param_t *test_param)
{
  // input
  int batch = test_param->input_shapes[NCDHW_N];
  int input_channel = test_param->input_shapes[NCDHW_C];
  int input_depth = test_param->input_shapes[NCDHW_D];
  int input_height = test_param->input_shapes[NCDHW_H];
  int input_width = test_param->input_shapes[NCDHW_W];

  int padding_d_start = test_param->paddings[0];
  // int padding_d_end = test_param->paddings[1];
  int padding_h_start = test_param->paddings[2];
  // int padding_h_end = test_param->paddings[3];
  int padding_w_start = test_param->paddings[4];
  // int padding_w_end = test_param->paddings[5];

  // output
  int output_depth = test_param->output_shapes[NCDHW_D];
  int output_channel = test_param->output_shapes[NCDHW_C];
  int output_height = test_param->output_shapes[NCDHW_H];
  int output_width = test_param->output_shapes[NCDHW_W];

#if 1
  // pytorch weight (oc, ic, kd, kh, kw)
  int weight_depth = test_param->weight_shapes[NCDHW_D];
  int weight_height = test_param->weight_shapes[NCDHW_H];
  int weight_width = test_param->weight_shapes[NCDHW_W];
#else
  // weight
  // weight (oc=1, id=1, kh=3, kw=3, ic=1)
  int weight_height = test_param->weight_shapes[2];
  int weight_width = test_param->weight_shapes[3];
#endif

  int stride_depth = test_param->weight_strides[SPATIAL_D];
  int stride_height = test_param->weight_strides[SPATIAL_H];
  int stride_width = test_param->weight_strides[SPATIAL_W];
  int dilation_depth = test_param->dilations[SPATIAL_D];
  int dilation_height = test_param->dilations[SPATIAL_H];
  int dilation_width = test_param->dilations[SPATIAL_W];

  int input_strides[5];
  int output_strides[5];
  int weight_strides[5];

  float *input_data = (float *)test_param->input_data;
  float *output_data = (float *)test_param->output_data;
  float *weight_data = (float *)test_param->weight_data;
  float *bias_data = (float *)test_param->bias_data;

  // input/output shape (n, c, d, h, w)
  get_strides_from_shapes5d(input_strides, test_param->input_shapes,
                           sizeof(float));
  get_strides_from_shapes5d(output_strides, test_param->output_shapes,
                            sizeof(float));

  // weight shape (oc, kd, kh, kw, ic)
  get_strides_from_shapes5d(weight_strides, test_param->weight_shapes,
                            sizeof(float));

  memset(output_data, 0,
         sizeof(float) * batch * output_channel * output_depth *
         output_height * output_width);

#ifdef DUMP_MSG
  printf("  %s =>\n", __FUNCTION__);
#endif

  for (int i = 0; i < batch; i++) {
    for (int oc = 0; oc < output_channel; oc++) {
      for (int oz = 0; oz < output_depth; oz++) {
        for (int oy = 0; oy < output_height; oy++) {
          for (int ox = 0; ox < output_width; ox++) {
            for (int ic = 0; ic < input_channel; ic++) {
              for (int kz = 0; kz < weight_depth; kz++) {
                const int iz =
                  oz * stride_depth + kz * dilation_depth - padding_d_start;

#ifdef DUMP_MSG
                printf("    [i=%d][oc=%d][oz=%d][oy=%d][ox=%d][ic=%d][kz=%d]" \
                       "iz= %d = %d(oz) * %d(stride_depth) + "\
                       "%d(kz) * %d(dilation_depth) - %d(padding_d_start)\n",
                      i, oc, oz, oy, ox, ic, kz,
                      iz, oz, stride_depth, kz, dilation_depth,
                      padding_d_start);
#endif

                if (iz < input_depth) {
                  for (int ky = 0; ky < weight_height; ky++) {
                    const int iy =
                        oy * stride_height + ky * dilation_height - padding_h_start;
                    if (iy < input_height) {
                      for (int kx = 0; kx < weight_width; kx++) {
                        const int ix =
                            ox * stride_width + kx * dilation_width - padding_w_start;
                        if (ix < input_width) {
                          int input_poss[5] = {i, ic, iz, iy, ix};
                          int input_offset =
                            get_tensor5d_offset(input_poss, input_strides)
                                / input_strides[5 - 1];

                          // pytorch (Oc=1, Id=1, Ic=1, kh=3, kw=3)
                          int weight_poss[5] = {
                              oc, ic, kz, ky, kx};

                          int weight_offset =
                            get_tensor5d_offset(weight_poss, weight_strides)
                                / weight_strides[5 - 1];

                          int output_poss[5] = {i, oc, oz, oy, ox};
                          int output_offset =
                            get_tensor5d_offset(output_poss, output_strides)
                                / output_strides[5 - 1];

                          output_data[output_offset] +=
                            input_data[input_offset] * weight_data[weight_offset];

#ifdef DUMP_MSG
                          printf("      [n=%d][oc=%d][oz=%d][oh=%d][ow=%d]" \
                                 "[ic=%d][kz=%d[ky=%d][kx=%d] output[%d](%f) "\
                                 "+= input[n=%d][ic=%d][iz=%d][iy=%d][ix=%d]"\
                                 "[%d](%f) * weight[oc=%d][ic=%d][kz=%d]"\
                                 "[ky=%d][kx=%d][%d](%f) = %f\n",
                              i, oc, oz, oy, ox, ic, kz, ky, kx, output_offset,
                              output_data[output_offset] -
                              input_data[input_offset] * weight_data[weight_offset],
                              i, ic, iz, iy, ix,
                              input_offset, input_data[input_offset],
                              oc, ic, kz, ky, kx,
                              weight_offset, weight_data[weight_offset],
                              output_data[output_offset]);
#endif

                        } // if (ix < input_width
                      } // for (int kx = 0; kx < weight_width; kx++)
                    } // if (iy < input_height)
                  } // for (int ky = 0; ky < weight_height; ky++)
                } // if (iz < input_depth)

              } // for (int ow = 0; ow < output_width; ow++) {
            } // for (int ic = 0; ic < input_channel; ic++)
          } // for (int oh = 0; oh < output_height; oh++) {
        } // for (int kz = 0; kz < weight_depth; kz++)
      } // for (int od = 0; od < output_depth; od++)
    } // for (int oc = 0; oc < output_channel; oc++)
  } // for (int i = 0; i < batch; ++i) {

  for (int i = 0; i < batch; ++i) {
    for (int oy = 0; oy < output_height; oy++) {
      for (int ox = 0; ox < output_width; ox++) {
        for (int oc = 0; oc < output_channel; oc++) {
          for (int od = 0; od < output_depth; od++) {
            int output_poss[5] = {i, oc, od, oy, ox};
            int output_offset =
                get_tensor5d_offset(output_poss, output_strides)
                    / output_strides[5 - 1];
            output_data[output_offset] += bias_data[oc];

#ifdef DUMP_MSG
            printf("    [n=%d][oy=%d][ox=%d][oc=%d][od=%d] output[%d](%f)" \
                   " += bias(%f) = %f\n",
                   i, oy, ox, oc, od, output_offset,
                   output_data[output_offset] - bias_data[oc], bias_data[oc],
                   output_data[output_offset]);
#endif

          } // for (int od = 0; od < output_depth; od++)
        } // for (int oc = 0; oc < output_channel; oc++)
      } // for (int ox = 0; ox < output_width; ox++)
    } // for (int oy = 0; oy < output_height; oy++)
  } // for (int i = 0; i < batch; ++i)

#ifdef DUMP_MSG
  printf("  <= %s\n", __FUNCTION__);
#endif

}

static void load_bias(cvk_context_t *cvk_ctx,
                      uint64_t ga_bias,
                      cvk_tl_t *tl_bias_al)
{
  cvk_fmt_t fmt = tl_bias_al->fmt;
  cvk_tg_shape_t gm_bias_shape = {
      tl_bias_al->shape.n, tl_bias_al->shape.c, tl_bias_al->shape.h,
      tl_bias_al->shape.w};
  cvk_tg_t gm_bias;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_bias, gm_bias_shape, fmt);
  gm_bias.start_address = ga_bias;

  cvk_tdma_g2l_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &gm_bias;
  param.dst = tl_bias_al;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);
}

// Input (n, ic, id, ih, iw)
static void load_input(cvk_context_t *cvk_ctx,
                       int n, int ic, int id, int ih, int iw,
                       int idi,
                       uint64_t ga_input,
                       cvk_tl_t *tl_input_al)
{
  // reshape (n, ic, id, ih, iw) => (n, ic, id, ih*iw)
  cvk_fmt_t fmt = tl_input_al->fmt;
  cvk_tl_shape_t tl_shape = {(uint32_t)n, (uint32_t)ic, 1, (uint32_t)(ih*iw)};
  cvk_tl_t tl_input;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_shape, fmt,
                                 tl_input_al->eu_align);
  tl_input.start_address = tl_input_al->start_address;

  uint32_t ds = (fmt == CVK_FMT_BF16) ? 2 : 1;
  cvk_tg_shape_t gm_input_shape = {
      (uint32_t)n, (uint32_t)ic, 1, (uint32_t)(ih*iw)};
  cvk_tg_stride_t gm_input_stride = {ic*id*ih*iw*ds, id*ih*iw*ds, ih*iw*ds, ds};

  if (idi >= 0 && idi < id) {
    cvk_tg_t gm_input;
    cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_input, gm_input_shape, fmt);
    gm_input.start_address = ga_input + gm_input_stride.h * idi;
    gm_input.stride = gm_input_stride;

    cvk_tdma_g2l_tensor_copy_param_t param;
    memset(&param, 0, sizeof(param));
    param.src = &gm_input;
    param.dst = &tl_input;
    cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);
  } else {
    uint32_t elt_size = fmt == CVK_FMT_BF16 ? 2 : 1;

    cvk_tl_shape_t tl_pad_shape = {
        (uint32_t)n, (uint32_t)ic, 1, (uint32_t)(ih*iw*elt_size)};
    cvk_tl_t tl_pad;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_pad, tl_pad_shape,
                                   CVK_FMT_I8, /*eu_align=*/1);
    tl_pad.start_address = tl_input_al->start_address;

    cvk_tiu_xor_int8_param_t param;
    memset(&param, 0, sizeof(param));
    param.res = &tl_pad;
    param.a = &tl_pad;
    param.b = &tl_pad;
    cvk_ctx->ops->tiu_xor_int8(cvk_ctx, &param);
  }
}

// TPU weight (kd, oc, kh*kw, ic)
static void load_weight(cvk_context_t *cvk_ctx,
                        int oc, int ic,
                        int kh, int kw, int kdi,
                        uint64_t ga_weight,
                        cvk_tl_t *tl_weight_al)
{
  cvk_fmt_t fmt = tl_weight_al->fmt;
  uint32_t ds = (fmt == CVK_FMT_BF16) ? 2 : 1;
  cvk_tg_shape_t gm_weight_shape = {
      1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)ic};
  cvk_tg_stride_t gm_weight_stride = {oc*kh*kw*ic*ds, kh*kw*ic*ds, ic*ds, ds};
  cvk_tg_t gm_weight;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_weight, gm_weight_shape, fmt);
  gm_weight.start_address = ga_weight + gm_weight_stride.n * kdi;

  cvk_tdma_g2l_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &gm_weight;
  param.dst = tl_weight_al;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);
}

static int get_ps32_mode(int kdi, int kd) {
  if (kd == 1)
    return 0;

  if (kdi == 0)
    return 2; // [1]: write
  else if (kdi == (kd - 1))
    return 1; // [0]: read

  return 3; // [1]: write, [0]: read
}

static void compute(cvk_context_t *cvk_ctx,
                    int n, int ic,
                    int kh, int kw,
                    int pad_top, int pad_bot,
                    int pad_left, int pad_right,
                    int oc, int oh, int ow,
                    int ps32_mode,
                    cvk_tl_t *tl_input_al,
                    cvk_tl_t *tl_weight_al,
                    cvk_tl_t *tl_bias_al,
                    cvk_tl_t *tl_output_al)
{
  cvk_fmt_t fmt = tl_weight_al->fmt;
  cvk_tl_shape_t tl_output_shape = {
      (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow};
  cvk_tl_t tl_output;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, tl_output_shape, fmt,
                                 /*eu_align=*/1);
  tl_output.start_address = tl_output_al->start_address;
  cvk_tl_t *tl_input = tl_input_al;
  cvk_tl_shape_t tl_weight_shape = {
      (uint32_t)ic, (uint32_t)oc, (uint32_t)kh, (uint32_t)kw};
  cvk_tl_t tl_weight;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_weight, tl_weight_shape, fmt,
                                 /*eu_align=*/0);
  tl_weight.start_address = tl_weight_al->start_address;

  cvk_tl_shape_t tl_bias_shape = {2, (uint32_t)oc, 1, 1};
  cvk_tl_t tl_bias;
  if (tl_bias_al) {
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_bias, tl_bias_shape, fmt,
                                  /*eu_align=*/0);
    tl_bias.start_address = tl_bias_al->start_address;
  }

  cvk_tiu_pt_convolution_param_t param;
  memset(&param, 0, sizeof(param));
  param.ifmap = tl_input;
  param.ofmap = &tl_output;
  param.weight = &tl_weight;
  param.bias = (tl_bias_al && ps32_mode == 1) ? &tl_bias : NULL;
  param.pad_top = (uint8_t)pad_top;
  param.pad_bottom = (uint8_t)pad_bot;
  param.pad_left = (uint8_t)pad_left;
  param.pad_right = (uint8_t)pad_right;
  param.stride_h = 1;
  param.stride_w = 1;
  param.dilation_h = 1;
  param.dilation_w = 1;
  param.ps32_mode = ps32_mode;
  cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &param);
}

static void store_output(cvk_context_t *cvk_ctx,
                         int oc, int od, int oh, int ow,
                         int odi,
                         uint64_t ga_res,
                         cvk_tl_t *tl_res)
{
  cvk_fmt_t fmt = tl_res->fmt;
  uint32_t ds = (fmt == CVK_FMT_BF16) ? 2 : 1;

  // Global memory shape (n, oc, od, oh, ow)
  cvk_tg_shape_t tg_res_shape = {
      tl_res->shape.n, tl_res->shape.c, tl_res->shape.h, tl_res->shape.w};
  cvk_tg_stride_t tg_stride = {
      oc * od * oh * ow * ds, od * oh * ow * ds, ow * ds, ds};
  uint32_t od_stride = oh * ow * ds;

  cvk_tg_t gm_res;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_res, tg_res_shape, fmt);
  gm_res.start_address = ga_res + od_stride * odi;
  gm_res.stride = tg_stride;

  cvk_tdma_l2g_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = tl_res;
  param.dst = &gm_res;
  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &param);
}

//
//        N  IC  ID  IH  IW
// input (1,  2,  4,  3,  3)
//
//        OC  IC  KD  KH  KW
// kernel (4,  2,  2,  2,  2)
//
//         N  OC  OD  OH  OW
// output (1,  4,  3,  2,  2)
//
// pytorch:
//   import torch
//   import torch.nn as nn
//   m = nn.Conv3d(2, 4, [2, 2, 2], stride=1)
//   input = torch.rand(1, 2, 4, 3, 3)
//   output = m(input)
//
static int conv3d_test(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  int ret = 0;

  // input (N=1, IC=2, ID=4, IH=3, IW=3)
  int n = 1, ic = 2, id = 4, ih = 3, iw = 3;
  float input_data[] = {
      // IC=0
      0.6762, 0.9451, 0.9486,   // ic = 0, id = 0, ih = 0
      0.1077, 0.6062, 0.1011,   // ic = 0, id = 0, ih = 1
      0.1065, 0.9864, 0.8988,   // ic = 0, id = 0, ih = 2

      0.1986, 0.6289, 0.9028,
      0.6754, 0.3942, 0.3231,
      0.4473, 0.9430, 0.1674,

      0.8915, 0.2300, 0.2834,
      0.8005, 0.9905, 0.5067,
      0.5892, 0.3737, 0.1197,

      0.2946, 0.8567, 0.7306,
      0.6123, 0.9854, 0.4904,
      0.9217, 0.8343, 0.0686,

      // IC=1
      0.7664, 0.0755, 0.4231,   // ic = 1, id = 0, ih = 0
      0.4695, 0.5165, 0.9785,   // ic = 1, id = 0, ih = 1
      0.6668, 0.4878, 0.5354,   // ic = 1, id = 0, ih = 2

      0.1907, 0.7196, 0.7503,
      0.9623, 0.4420, 0.1084,
      0.5654, 0.9658, 0.8150,

      0.3203, 0.6839, 0.4136,
      0.3514, 0.4005, 0.4281,
      0.1185, 0.0036, 0.1968,

      0.8295, 0.1635, 0.6517,
      0.0113, 0.9510, 0.4708,
      0.0686, 0.1143, 0.6780
  };

  // pytorch weight (Oc=4, Ic=2, kd=2, kh=2, kw=2)
  int oc = 4, kd = 2, kh = 2, kw = 2;
  int weight_shapes[5] = {oc, ic, kd, kh, kw};
  float weight_data[] = {
       // OC=0
       0.1715,  0.1906,   // ic = 0, kd = 0, kh = 0
       0.0437, -0.0401,   // ic = 0, kd = 0, kh = 1

      -0.2442,  0.1911,   // ic = 0, kd = 1, kh = 0
      -0.0082, -0.0663,   // ic = 0, kd = 1, kh = 1

      -0.1137, -0.0246,   // ic = 1, kd = 0, kh = 0
       0.2495, -0.0684,   // ic = 1, kd = 0, kh = 1

      -0.0456,  0.0776,   // ic = 1, kd = 1, kh = 0
       0.1798,  0.1516,   // ic = 1, kd = 1, kh = 1

      // OC=1
       0.0527,  0.2034,
      -0.1434, -0.1642,

      -0.0797,  0.0839,
      -0.0746,  0.1446,

       0.1706,  0.1556,
       0.0149,  0.1610,

       0.0890,  0.0433,
       0.0363,  0.2293,

      // OC=2
       0.2052,  0.0489,
      -0.1775,  0.0486,

       0.1524,  0.0386,
       0.1624,  0.0692,

       0.1914,  0.0774,
      -0.1583,  0.1109,

       0.2034, -0.1709,
       0.1521, -0.1975,

      // OC=3
       0.1881,  0.1785,
       0.0584, -0.0217,

       0.1191,  0.2206,
       0.1310, -0.0952,

      -0.1424,  0.1071,
       0.0292, -0.1104,

       0.1335,  0.1561,
      -0.1034, -0.2354
  };

  // tpu weight shape (kd=2, oc=4, kh(2)*kw(2), ic=2)
  // int weight_shapes_tpu[5] = {kd, oc, kh, kw, ic};
  float weight_data_tpu[kd * oc * kh * kw * ic];
  convert_tpu_weight_for_ncdhw(weight_data_tpu, weight_data, weight_shapes);

#if 0
  float weight_data_tpu_ref[] = {
       0.171500, -0.113700,   // kd = 0, oc = 0, kh = 0, kw = 0
       0.190600, -0.024600,   // kd = 0, oc = 0, kh = 0, kw = 1
       0.043700,  0.249500,   // kd = 0, oc = 0, kh = 1, kw = 0
      -0.040100, -0.068400,   // kd = 0, oc = 0, kh = 1, kw = 1

       0.052700,  0.170600,   // kd = 0, oc = 1
       0.203400,  0.155600,
      -0.143400,  0.014900,
      -0.164200,  0.161000,

       0.205200,  0.191400,   // kd = 0, oc = 2
       0.048900,  0.077400,
      -0.177500, -0.158300,
       0.048600,  0.110900,

       0.188100, -0.142400,   // kd = 0, oc = 3
       0.178500,  0.107100,
       0.058400,  0.029200,
      -0.021700, -0.110400,

      -0.244200, -0.045600,   // kd = 1, oc = 0
       0.191100,  0.077600,
      -0.008200,  0.179800,
      -0.066300,  0.151600,

      -0.079700,  0.089000,   // kd = 1, oc = 1
       0.083900,  0.043300,
      -0.074600,  0.036300,
       0.144600,  0.229300,

       0.152400,  0.203400,   // kd = 1, oc = 2
       0.038600, -0.170900,
       0.162400,  0.152100,
       0.069200, -0.197500,

       0.119100,  0.133500,   // kd = 1, oc = 3
       0.220600,  0.156100,
       0.131000, -0.103400,
      -0.095200, -0.235400,
  };
#endif

  // dumpFloatData(weight_data, weight_shapes);
  // dumpFloatData(weight_data_tpu, weight_shapes_tpu);

  // bias (4)
  float bias_data[] = {
    0.1204, -0.1286, -0.0339, -0.1120
  };

  // output (N=1, Oc=4, Od=3, Oh=2, Ow=2)
  int od = 3, oh = 2, ow = 2;
  float ref_output_data[] = {
      // OC=0
      0.7170, 0.6444,
      0.3692, 0.4852,

      0.3749, 0.5013,
      0.2489, 0.3058,

      0.4620, 0.3949,
      0.5157, 0.2879,

      // OC=1
      0.4449, 0.4349,
      0.5010, 0.1843,

      0.2726, 0.4384,
      0.2482, 0.0854,

      0.3631, 0.1475,
      0.2504, 0.2950,

      // OC=2
      0.4633, 0.4587,
      0.3968, 0.4154,

      0.1917, 0.5096,
      0.6285, 0.1435,

      0.3697, 0.3493,
      0.3388, 0.5705,

      // OC=3
      0.1802, 0.6468,
      0.0031, 0.0546,

      0.2840, 0.3474,
      0.3630, 0.2990,

      0.2374, 0.2000,
      0.6851, 0.5085
  };

  // dilation = (depth=1, height=1, width=1)
  int dilation_d = 1, dilation_h = 1, dilation_w = 1;

  // stride = (depth=1, height=1, width=1)
  int stride_d = 1, stride_h = 1, stride_w = 1;

  // zero padding
  int pad_top = 0, pad_bot = 0, pad_left = 0, pad_right = 0;

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)] = {0.0};
  conv3d_float_ref_for_ncdhw(input_data, weight_data, bias_data, output_data_cpu,
                             n, ic, id, ih, iw,
                             oc, od, oh, ow,
                             kd, kh, kw,
                             stride_d, stride_h, stride_w,
                             dilation_d, dilation_h, dilation_w,
                             0, 0, 0, 0, 0, 0
                             );

  printf("  %s: compare ref\n", __FUNCTION__);
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare ref %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Partial sum
  //   oc[0]od[0]  = id[0]ic[0] * oc[0]kd[0]ic[0] + id[0]ic[1] * oc[0]kd[0]ic[1]
  //   oc[0]od[0] += id[1]ic[0] * oc[0]kd[1]ic[0] + id[1]ic[1] * oc[0]kd[1]ic[1]
  //
  //   oc[0]od[1]  = id[1]ic[0] * oc[0]kd[0]ic[0] + id[1]ic[1] * oc[0]kd[0]ic[1]
  //   oc[0]od[1] += ic[2]ic[0] * oc[0]kd[1]ic[0] + id[2]ic[1] * oc[0]kd[1]ic[1]
  //
  //   oc[0]od[2]  = id[2]ic[0] * oc[0]kd[0]ic[0] + id[2]ic[1] * oc[0]kd[0]ic[1]
  //   oc[0]od[2] += ic[3]ic[0] * oc[0]kd[1]ic[0] + id[3]ic[1] * oc[0]kd[1]ic[1]
  //
  //   ...
  //
  //   oc[3]od[0]  = id[0]ic[0] * oc[3]kd[0]ic[0] + id[0]ic[1] * oc[3]kd[0]ic[1]
  //   oc[3]od[0] += id[1]ic[0] * oc[3]kd[1]ic[0] + id[1]ic[1] * oc[3]kd[1]ic[1]
  //
  //   oc[3]od[1]  = id[1]ic[0] * oc[3]kd[0]ic[0] + id[1]ic[1] * oc[3]kd[0]ic[1]
  //   oc[3]od[1] += ic[2]ic[0] * oc[3]kd[1]ic[0] + id[2]ic[1] * oc[3]kd[1]ic[1]
  //
  //   oc[3]od[2]  = id[2]ic[0] * oc[3]kd[0]ic[0] + id[2]ic[1] * oc[3]kd[0]ic[1]
  //   oc[3]od[2] += ic[3]ic[0] * oc[3]kd[1]ic[0] + id[3]ic[1] * oc[3]kd[1]ic[1]

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_weight_al = NULL;
  cvk_tl_t *tl_bias_al = NULL;

  // Allocate ps32 output
  {
    cvk_tl_shape_t shape = {
        4 * (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow}; // 4x
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/1);
  }

  // Allocate input
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate weight
  {
    cvk_tl_shape_t shape = {
        1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)ic};
    tl_weight_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  // Allocate bias
  // bias
  {
    cvk_tl_shape_t shape = {2, (uint32_t)oc, 1, 1};
    tl_bias_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/0);
  }

  assert(tl_output_al && tl_input_al && tl_weight_al && tl_bias_al &&
         "Expect all allocated");

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_weight_dev_mem = NULL;
  CVI_RT_MEM gm_bias_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_weight = 0;
  uint64_t ga_bias = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, ic=2, id=4, ih=3, iw=3)
    // reshape (1, ic=2, id=4, ih=3, iw=3) -> (1, 2, 4, 3x3)
    int total_len = 1 * ic * id * ih * iw;
    uint16_t input_bf16_data[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data, input_data, total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem, (uint8_t *)input_bf16_data);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of weight
  {
    int len = kd * oc * kh * kw * ic;
    uint16_t weight_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, weight_bf16_data_tpu, weight_data_tpu,
                              len);

    gm_weight_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_weight_dev_mem,
                      (uint8_t *)weight_bf16_data_tpu);

    ga_weight = CVI_RT_MemGetPAddr(gm_weight_dev_mem);
  }

  // Allocate device memory of bias
  {
    int len = oc;
    uint16_t bias_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, bias_bf16_data_tpu, bias_data, len);

    gm_bias_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_bias_dev_mem,
                      (uint8_t *)bias_bf16_data_tpu);

    ga_bias = CVI_RT_MemGetPAddr(gm_bias_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * oc * od * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && gm_weight_dev_mem &&
         gm_bias_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && ga_weight && ga_bias && "Expect valid gaddr");

  load_bias(cvk_ctx, ga_bias, tl_bias_al);

  for (int odi = 0; odi < od; odi++) {
    int id_start = odi; // not support padding

    for (int kdi = 0; kdi < kd; kdi++) {
      int idi = id_start + kdi;
      int ps32_mode = get_ps32_mode(kdi, kd);

      load_input(cvk_ctx, n, ic, id, ih, iw, idi, ga_input, tl_input_al);
      load_weight(cvk_ctx, oc, ic, kh, kw, kdi, ga_weight, tl_weight_al);
      compute(cvk_ctx, n, ic, kh, kw,
              pad_top, pad_bot, pad_left, pad_right,
              oc, oh, ow, ps32_mode, tl_input_al,
              tl_weight_al, tl_bias_al, tl_output_al);
    }
    store_output(cvk_ctx, oc, od, oh, ow, odi, ga_output, tl_output_al);
  }

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * oc * od * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  %s: compare tpu\n", __FUNCTION__);
  const float tpu_precision = 0.01;
  for (int i = 0; i < output_len; i++) {
    float tpu_data = cvk_convert_bf16_fp32(output_bf16_data_tpu[i]);
    if (fabs(tpu_data - ref_output_data[i]) > tpu_precision) {
      printf("    [%d] Error ! val %f(0x%x), expected %f(0x%x)\n",
             (int)i, tpu_data, output_bf16_data_tpu[i], ref_output_data[i],
             ref_output_bf16_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare tpu %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bias_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_weight_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_weight_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_bias_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_output_dev_mem);

#ifdef DUMP_MSG
  printf("<= %s\n", __FUNCTION__);
  printf("===================================\n\n");
#endif

  return ret;
}

// # pytorch
// #
// #        N  IC  ID  IH  IW
// # input (1,  2,  4,  3,  3)
// #
// #        OC  IC  KD  KH  KW
// # kernel (4,  2,  2,  2,  2)
// #
// #         N  OC  OD  OH  OW
// # output (1,  4,  5,  4,  4)
// #
// #            IC OC  KD KH KW
// m = nn.Conv3d(2, 4, [2, 2, 2], stride=(1, 1, 1), padding=(1, 1, 1))
// input = torch.rand(1, 2, 4, 3, 3)
// output = m(input)
//
static int conv3d_test_ncdhw_pad_dhw(CVI_RT_HANDLE rt_handle,
                                     cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  int ret = 0;

  // input (N=1, IC=2, ID=4, IH=3, IW=3)
  int n = 1, ic = 2, id = 4, ih = 3, iw = 3;
  float input_data[] = {
      // IC=0
      0.3307, 0.6577, 0.3520,   // ic = 0, id = 0, ih = 0
      0.5691, 0.1531, 0.6240,   // ic = 0, id = 0, ih = 1
      0.4324, 0.9731, 0.4587,   // ic = 0, id = 0, ih = 2

      0.6121, 0.5937, 0.8512,
      0.7932, 0.3473, 0.4032,
      0.0156, 0.6799, 0.8587,

      0.9278, 0.1046, 0.2478,
      0.4399, 0.2543, 0.8906,
      0.0275, 0.0450, 0.1212,

      0.5655, 0.6741, 0.3396,
      0.6126, 0.6385, 0.5160,
      0.9062, 0.5286, 0.7064,

      // IC=1
      0.0512, 0.9951, 0.8289,   // ic = 1, id = 0, ih = 0
      0.9011, 0.0602, 0.5583,   // ic = 1, id = 0, ih = 1
      0.5176, 0.9857, 0.8772,   // ic = 1, id = 0, ih = 2

      0.8971, 0.5207, 0.1500,
      0.8408, 0.2034, 0.7618,
      0.7618, 0.0702, 0.9254,

      0.2110, 0.1366, 0.5222,
      0.0626, 0.9902, 0.2842,
      0.0101, 0.6390, 0.0038,

      0.7045, 0.3892, 0.7232,
      0.7224, 0.8458, 0.6474,
      0.0602, 0.9074, 0.4171
  };


  // pytorch weight (Oc=4, Ic=2, kd=2, kh=2, kw=2)
  int oc = 4, kd = 2, kh = 2, kw = 2;
  int weight_shapes[5] = {oc, ic, kd, kh, kw};
  float weight_data[] = {
      // OC=0
      -0.2046, -0.2492,   // ic = 0, kd = 0, kh = 0
      -0.0783,  0.1082,   // ic = 0, kd = 0, kh = 1

       0.1393, -0.1803,   // ic = 0, kd = 1, kh = 0
      -0.0110, -0.1141,   // ic = 0, kd = 1, kh = 1

       0.0606,  0.1902,   // ic = 1, kd = 0, kh = 0
       0.1254,  0.1572,   // ic = 1, kd = 0, kh = 1

       0.0887, -0.0336,   // ic = 1, kd = 1, kh = 0
       0.0918, -0.1099,   // ic = 1, kd = 1, kh = 1

      // OC=1
      -0.0181, -0.2228,
      -0.0575, -0.2464,

      -0.0757, -0.0122,
      -0.1896,  0.1301,

      -0.0215,  0.0568,
      -0.1381, -0.1621,

      -0.1247, -0.0738,
      -0.0146,  0.0719,

      // OC=2
       0.0960, -0.1865,
      -0.2124, -0.0125,

       0.0159,  0.1148,
       0.1430,  0.1978,

       0.0292, -0.2130,
       0.2055,  0.1678,

       0.2236, -0.0215,
      -0.2171,  0.1709,

      // OC=3
       0.2186,  0.1488,
       0.1558,  0.0359,

       0.1822, -0.0433,
       0.0960,  0.1791,

      -0.0060,  0.0006,
       0.0400,  0.1488,

       0.1811, -0.1055,
       0.1138, -0.0898
  };

  // tpu weight shape (kd=2, oc=4, kh(2)*kw(2), ic=2)
  // int weight_shapes_tpu[5] = {kd, oc, kh, kw, ic};
  float weight_data_tpu[kd * oc * kh * kw * ic];
  convert_tpu_weight_for_ncdhw(weight_data_tpu, weight_data, weight_shapes);

  // bias (4)
  float bias_data[] = {
      -0.2107, -0.1894, -0.0108,  0.1728
  };

  // output (N=1, Oc=4, Od=5, Oh=4, Ow=4)
  int od = 5, oh = 4, ow = 4;
  float ref_output_data[] = {
      // OC=0
      -2.5403e-01, -3.9400e-01, -2.5784e-01, -1.3846e-01,
      -4.3596e-01, -2.5972e-01, -2.5080e-01, -4.3702e-02,
      -4.4977e-01, -2.5769e-01, -3.8422e-01,  1.2387e-03,
      -3.0602e-01, -3.1306e-01, -9.9820e-02, -6.8943e-02,

      -3.3526e-01, -5.1864e-02, -4.1363e-02, -1.2992e-01,
      -4.0344e-01, -1.0866e-01, -2.0857e-01, -1.3983e-02,
      -3.0966e-01,  9.2221e-02, -2.8528e-01, -3.1210e-02,
      -2.4841e-01, -3.7795e-01, -3.8244e-01, -4.9618e-02,

      -1.3243e-01, -1.7816e-02, -1.5046e-01, -2.1334e-01,
      -2.0596e-01, -2.3001e-01, -4.0274e-01, -2.1468e-01,
      -2.1257e-01, -2.7799e-01, -3.3916e-02, -4.9950e-02,
      -7.4998e-02, -3.4861e-01, -3.4250e-01, -3.1303e-01,

      -2.1902e-01, -2.8536e-01, -1.8272e-01, -1.0197e-01,
      -6.1921e-01, -3.3074e-01,  4.2541e-02, -9.8628e-02,
      -5.4856e-01, -2.2603e-01, -2.8005e-01, -2.2485e-01,
      -3.8100e-01, -9.9595e-02, -1.9782e-01, -9.9829e-02,

      -3.8680e-02, -3.2488e-02, -6.4191e-02, -1.4660e-01,
      -3.7711e-02, -1.3294e-01, -5.8401e-02, -1.9556e-01,
      -1.1838e-01, -1.5398e-01, -8.1088e-02, -2.8004e-01,
      -4.2503e-01, -3.5157e-01, -3.6049e-01, -3.2990e-01,

      // OC=1
      -1.4269e-01, -9.5723e-02, -2.2320e-01, -2.6823e-01,
      -5.8374e-02, -3.9911e-01, -3.3735e-01, -4.4587e-01,
      -1.6939e-01, -2.4322e-01, -3.3348e-01, -4.0600e-01,
      -2.3289e-01, -3.7133e-01, -4.5634e-01, -3.3350e-01,

      -1.3504e-01, -5.5332e-01, -5.8443e-01, -4.8773e-01,
      -4.5654e-01, -7.9793e-01, -6.0842e-01, -4.9727e-01,
      -4.7046e-01, -8.5047e-01, -8.1262e-01, -6.6208e-01,
      -3.1279e-01, -4.7892e-01, -4.1965e-01, -3.9698e-01,

      -3.4979e-01, -7.3478e-01, -4.8156e-01, -3.1361e-01,
      -5.7180e-01, -6.9073e-01, -6.5631e-01, -5.9334e-01,
      -4.5140e-01, -6.4365e-01, -8.3343e-01, -5.1614e-01,
      -1.5070e-01, -4.0468e-01, -4.2686e-01, -2.3451e-01,

      -3.2802e-01, -3.2160e-01, -3.9730e-01, -3.5070e-01,
      -4.2998e-01, -6.3385e-01, -8.1355e-01, -5.1874e-01,
      -2.3089e-01, -5.6220e-01, -7.1846e-01, -4.7895e-01,
      -2.1047e-01, -3.1337e-01, -4.2336e-01, -2.9714e-01,

      -4.4296e-01, -5.4842e-01, -4.8282e-01, -3.0881e-01,
      -5.4348e-01, -7.7235e-01, -6.3022e-01, -3.3020e-01,
      -5.1796e-01, -6.4802e-01, -6.9482e-01, -3.1089e-01,
      -3.8791e-01, -2.7335e-01, -3.5223e-01, -2.1117e-01,

      // OC=2
       6.3343e-02,  3.2551e-01,  7.8462e-02, -1.4047e-01,
       2.9263e-01, -1.3682e-02,  4.7238e-01,  1.4810e-01,
       2.0917e-01,  5.2640e-01,  2.3049e-01, -9.6724e-04,
       2.7707e-02,  2.0233e-01,  2.5884e-01,  1.9259e-01,

       2.6804e-01,  1.8736e-01,  3.5448e-01,  1.7387e-01,
       4.1227e-01,  6.1802e-02,  3.4067e-01, -3.1375e-02,
      -2.1211e-02,  4.1589e-01,  3.9848e-01,  2.4676e-01,
      -2.1633e-01, -9.8574e-02, -5.5862e-02,  2.7933e-01,

       3.5165e-01,  2.5434e-01,  1.0813e-01, -2.3880e-01,
       1.4803e-02,  2.2636e-01,  5.6942e-02,  3.3249e-01,
      -1.5394e-01,  2.8699e-01,  1.9381e-02,  1.5203e-01,
      -1.7307e-01, -1.3476e-01, -1.4338e-01,  1.0136e-01,

       2.4526e-01, -1.5181e-02,  2.8220e-01, -6.4634e-02,
       7.0619e-02,  5.5526e-01,  2.7332e-01, -2.2993e-03,
       1.3952e-01,  4.8027e-01,  2.7088e-01,  2.2137e-01,
       8.4666e-02, -8.3372e-02,  2.7218e-01,  1.0539e-01,

       1.0030e-01,  7.0672e-02,  4.3040e-02,  6.5646e-02,
      -1.5283e-01,  7.5928e-03, -1.1840e-02,  6.6282e-02,
      -2.8021e-01, -2.6473e-01, -2.3691e-02, -6.7624e-03,
      -1.9269e-01, -2.1400e-01, -1.5423e-01,  6.9144e-02,

      // OC=3
       2.2745e-01,  2.3889e-01,  3.3790e-01,  3.0098e-01,
       1.7417e-01,  2.8818e-01,  4.5343e-01,  5.1056e-01,
       8.4155e-02,  6.1303e-01,  3.3481e-01,  5.3153e-01,
       9.9508e-02,  1.9928e-01,  4.1631e-01,  4.1526e-01,

       2.2143e-01,  6.1859e-01,  7.0634e-01,  3.5959e-01,
       3.2209e-01,  8.9161e-01,  7.0540e-01,  6.7204e-01,
       1.6198e-01,  1.0484e+00,  7.8346e-01,  8.1161e-01,
       1.5643e-01,  5.1360e-01,  4.5026e-01,  5.9188e-01,

       4.7556e-01,  5.2241e-01,  3.6209e-01,  3.9465e-01,
       4.2880e-01,  7.8418e-01,  8.6553e-01,  7.0884e-01,
       3.8365e-01,  3.9124e-01,  8.4084e-01,  6.5296e-01,
       1.7330e-01,  2.1039e-01,  5.6763e-01,  3.7774e-01,

       2.7558e-01,  5.7020e-01,  3.8613e-01,  3.4723e-01,
       2.8224e-01,  9.5750e-01,  6.7976e-01,  6.9007e-01,
       2.9504e-01,  6.4116e-01,  8.1472e-01,  7.1143e-01,
       1.3136e-01,  2.4329e-01,  3.8296e-01,  4.0355e-01,

       2.9795e-01,  3.7119e-01,  4.1321e-01,  2.5462e-01,
       3.8687e-01,  6.6586e-01,  6.1692e-01,  3.4898e-01,
       3.0586e-01,  6.9549e-01,  5.9051e-01,  4.0845e-01,
       3.0771e-01,  4.4973e-01,  3.8828e-01,  3.2473e-01,
  };

  // dilation = (depth=1, height=1, width=1)
  int dilation_d = 1, dilation_h = 1, dilation_w = 1;

  // stride = (depth=1, height=1, width=1)
  int stride_d = 1, stride_h = 1, stride_w = 1;

  // padding = (1, 1, 1)
  int pad_d0 = 1, pad_d1 = 1;
  int pad_top = 1, pad_bot = 1, pad_left = 1, pad_right = 1;

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)] = {0.0};
  conv3d_float_ref_for_ncdhw(
      input_data, weight_data, bias_data, output_data_cpu,
      n, ic, id, ih, iw,
      oc, od, oh, ow,
      kd, kh, kw,
      stride_d, stride_h, stride_w,
      dilation_d, dilation_h, dilation_w,
      pad_d0, pad_d1,
      pad_top, pad_bot, pad_left, pad_right);

  printf("  %s: compare ref\n", __FUNCTION__);
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare ref %s\n", __FUNCTION__, ret ? "fail" : "pass");

  if (ret)
    return ret;

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_weight_al = NULL;
  cvk_tl_t *tl_bias_al = NULL;

  // Allocate ps32 output
  {
    cvk_tl_shape_t shape = {
        4 * (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow}; // 4x
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/1);
  }

  // Allocate input
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate weight
  {
    cvk_tl_shape_t shape = {
        1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)ic};
    tl_weight_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  // Allocate bias
  // bias
  {
    cvk_tl_shape_t shape = {2, (uint32_t)oc, 1, 1};
    tl_bias_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/0);
  }

  assert(tl_output_al && tl_input_al && tl_weight_al && tl_bias_al &&
         "Expect all allocated");

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_weight_dev_mem = NULL;
  CVI_RT_MEM gm_bias_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_weight = 0;
  uint64_t ga_bias = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, ic=2, id=4, ih=3, iw=3)
    // reshape (1, ic=2, id=4, ih=3, iw=3) -> (1, 2, 4, 3x3)
    int total_len = 1 * ic * id * ih * iw;
    uint16_t input_bf16_data[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data, input_data, total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem, (uint8_t *)input_bf16_data);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of weight
  {
    int len = kd * oc * kh * kw * ic;
    uint16_t weight_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, weight_bf16_data_tpu, weight_data_tpu,
                              len);

    gm_weight_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_weight_dev_mem,
                      (uint8_t *)weight_bf16_data_tpu);

    ga_weight = CVI_RT_MemGetPAddr(gm_weight_dev_mem);
  }

  // Allocate device memory of bias
  {
    int len = oc;
    uint16_t bias_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, bias_bf16_data_tpu, bias_data, len);

    gm_bias_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_bias_dev_mem,
                      (uint8_t *)bias_bf16_data_tpu);

    ga_bias = CVI_RT_MemGetPAddr(gm_bias_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * oc * od * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && gm_weight_dev_mem &&
         gm_bias_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && ga_weight && ga_bias && "Expect valid gaddr");

  load_bias(cvk_ctx, ga_bias, tl_bias_al);

  for (int odi = 0; odi < od; odi++) {
    for (int kdi = 0; kdi < kd; kdi++) {
      int idi = odi * stride_d + kdi * dilation_d - pad_d0;
      int ps32_mode = get_ps32_mode(kdi, kd);

      load_input(cvk_ctx, n, ic, id, ih, iw, idi, ga_input, tl_input_al);
      load_weight(cvk_ctx, oc, ic, kh, kw, kdi, ga_weight, tl_weight_al);
      compute(cvk_ctx, n, ic, kh, kw,
              pad_top, pad_bot, pad_left, pad_right,
              oc, oh, ow,
              ps32_mode, tl_input_al,
              tl_weight_al, tl_bias_al, tl_output_al);
    }
    store_output(cvk_ctx, oc, od, oh, ow, odi, ga_output, tl_output_al);
  }

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * oc * od * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  %s: compare tpu\n", __FUNCTION__);
  const float tpu_precision = 0.01;
  for (int i = 0; i < output_len; i++) {
    float tpu_data = cvk_convert_bf16_fp32(output_bf16_data_tpu[i]);
    if (fabs(tpu_data - ref_output_data[i]) > tpu_precision) {
      printf("    [%d] Error ! val %f(0x%x), expected %f(0x%x)\n",
             (int)i, tpu_data, output_bf16_data_tpu[i], ref_output_data[i],
             ref_output_bf16_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare tpu %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bias_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_weight_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_weight_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_bias_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_output_dev_mem);

#ifdef DUMP_MSG
  printf("<= %s\n", __FUNCTION__);
  printf("===================================\n\n");
#endif

  return ret;
}

// Input (n, id, ic, ih, iw)
static void load_input_for_ndchw(cvk_context_t *cvk_ctx,
                                 int n, int ic, int id, int ih, int iw,
                                 int pad_d0, int pad_d1,
                                 uint64_t ga_input,
                                 cvk_tl_t *tl_input_al)
{
  cvk_fmt_t fmt = tl_input_al->fmt;
  uint32_t start_addr = tl_input_al->start_address;

  // Fill (n, pad0, ic, ih, iw)
  if (pad_d0) {
    uint32_t elt_size = fmt == CVK_FMT_BF16 ? 2 : 1;

    cvk_tl_shape_t tl_shape = {
        (uint32_t)n, (uint32_t)(pad_d0 * ic), (uint32_t)ih,
        (uint32_t)iw * elt_size};
    cvk_tl_t tl_pad;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_pad, tl_shape, CVK_FMT_I8,
                                   /*eu_align=*/1);
    tl_pad.start_address = start_addr;

    cvk_tiu_xor_int8_param_t param;
    memset(&param, 0, sizeof(param));
    param.res = &tl_pad;
    param.a = &tl_pad;
    param.b = &tl_pad;
    cvk_ctx->ops->tiu_xor_int8(cvk_ctx, &param);

    start_addr = addr_after_right_shift(cvk_ctx, start_addr,
                                        (uint32_t)(pad_d0 * ic),
                                        tl_pad.stride.c);
  }

  // reshape (n, id, ic, ih, iw) -> (n, id*ic, ih, iw)
  cvk_tl_shape_t tl_shape = {
      (uint32_t)n, (uint32_t)id*ic, (uint32_t)ih, (uint32_t)iw};
  cvk_tl_t tl_input;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_shape, fmt,
                                 tl_input_al->eu_align);
  tl_input.start_address = start_addr;

  cvk_tg_shape_t gm_input_shape = {
      (uint32_t)n, (uint32_t)id*ic, (uint32_t)ih, (uint32_t)iw};

  cvk_tg_t gm_input;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_input, gm_input_shape, fmt);
  gm_input.start_address = ga_input;

  cvk_tdma_g2l_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &gm_input;
  param.dst = &tl_input;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);

  if (pad_d1) {
    start_addr = addr_after_right_shift(cvk_ctx, start_addr,
                                        (uint32_t)(id * ic),
                                        tl_input.stride.c);
    cvk_tl_shape_t tl_shape = {
        (uint32_t)n, (uint32_t)(pad_d1 * ic), (uint32_t)ih, (uint32_t)iw};
    cvk_tl_t tl_pad;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_pad, tl_shape, fmt,
                                   /*eu_align=*/1);
    tl_pad.start_address = start_addr;

    cvk_tdma_g2l_tensor_fill_constant_param_t param;
    memset(&param, 0, sizeof(param));
    param.constant = cvk_ctx->misc_ops->float_to_bfloat16(cvk_ctx, 0.0);
    param.dst = &tl_pad;
    cvk_ctx->ops->tdma_g2l_bf16_tensor_fill_constant(cvk_ctx, &param);
  }
}

static void convert_tpu_weight_for_ndchw(
    float *tpu_weight, float cpu_weight[5], int cpu_shapes[5])
{
  // Permute
  //     0   1   2   3   4       0   3   4   2   1
  //   (oc, ic, kd, kh, kw) -> (oc, kh, kw, kd, ic)
  int orders[5] = {0, 3, 4, 2, 1};
  permute5d(tpu_weight, cpu_weight, cpu_shapes, orders);
}

// TPU weight (1, oc, kh*kw, kd*ic)
static void load_weight_for_ndchw(cvk_context_t *cvk_ctx,
                                  int oc, int ic, int kd,
                                  int kh, int kw,
                                  uint64_t ga_weight,
                                  cvk_tl_t *tl_weight_al)
{
  cvk_fmt_t fmt = tl_weight_al->fmt;
  cvk_tg_shape_t gm_weight_shape = {
      1, (uint32_t)oc, (uint32_t)kh*kw, (uint32_t)kd*ic};
  cvk_tg_t gm_weight;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_weight, gm_weight_shape, fmt);
  gm_weight.start_address = ga_weight;

  cvk_tdma_g2l_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &gm_weight;
  param.dst = tl_weight_al;
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);
}

static void store_output_for_ndchw(cvk_context_t *cvk_ctx,
                                   int oc, int od, int oh, int ow,
                                   int odi,
                                   uint64_t ga_output,
                                   cvk_tl_t *tl_output)
{
  assert(odi < od);

  cvk_fmt_t fmt = tl_output->fmt;
  uint32_t ds = (fmt == CVK_FMT_BF16) ? 2 : 1;

  // Global memory shape (n, od, oc, oh, ow)
  cvk_tg_shape_t tg_output_shape = {
      tl_output->shape.n, tl_output->shape.c, tl_output->shape.h,
      tl_output->shape.w};
  cvk_tg_stride_t tg_stride = {
      oc * oh * ow * ds, oh * ow * ds, ow * ds, ds};

  cvk_tg_t gm_output;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_output, tg_output_shape, fmt);
  gm_output.start_address = ga_output + tg_stride.n * odi;

  cvk_tdma_l2g_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = tl_output;
  param.dst = &gm_output;
  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &param);
}

static void compute_for_ndchw(cvk_context_t *cvk_ctx,
                              int ic, int id, int ih, int iw,
                              int kd, int kh, int kw,
                              int sd, int sh, int sw,
                              int pad_top, int pad_bot,
                              int pad_left, int pad_right,
                              int oc, int oh, int ow,
                              int odi,
                              cvk_tl_t *tl_input_al,
                              cvk_tl_t *tl_weight_al,
                              cvk_tl_t *tl_bias_al,
                              cvk_tl_t *tl_output_al)
{
  (void)id;

  cvk_fmt_t fmt = tl_weight_al->fmt;
  cvk_tl_shape_t tl_input_shape = {
      1, (uint32_t)kd*ic, (uint32_t)ih, (uint32_t)iw};
  cvk_tl_t tl_input;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_input_shape, fmt,
                                 /*eu_align=*/1);
  tl_input.start_address = addr_after_right_shift(cvk_ctx,
                                                  tl_input_al->start_address,
                                                  (uint32_t)(odi * sd * kd),
                                                  tl_input.stride.c);

  cvk_tl_shape_t tl_output_shape = {
      1, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow};
  cvk_tl_t tl_output;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, tl_output_shape, fmt,
                                 /*eu_align=*/1);
  tl_output.start_address = tl_output_al->start_address;

  cvk_tl_shape_t tl_weight_shape = {
      (uint32_t)kd*ic, (uint32_t)oc, (uint32_t)kh, (uint32_t)kw};
  cvk_tl_t tl_weight;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_weight, tl_weight_shape, fmt,
                                 /*eu_align=*/0);
  tl_weight.start_address = tl_weight_al->start_address;

  cvk_tl_t tl_bias;
  if (tl_bias_al) {
    cvk_tl_shape_t tl_bias_shape = {2, (uint32_t)oc, 1, 1};
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_bias, tl_bias_shape, fmt,
                                   /*eu_align=*/0);
    tl_bias.start_address = tl_bias_al->start_address;
  }

  cvk_tiu_pt_convolution_param_t param;
  memset(&param, 0, sizeof(param));
  param.ifmap = &tl_input;
  param.ofmap = &tl_output;
  param.weight = &tl_weight;
  param.bias = tl_bias_al ? &tl_bias : NULL;
  param.pad_top = (uint8_t)pad_top;
  param.pad_bottom = (uint8_t)pad_bot;
  param.pad_left = (uint8_t)pad_left;
  param.pad_right = (uint8_t)pad_right;
  param.stride_h = sh;
  param.stride_w = sw;
  param.dilation_h = 1;
  param.dilation_w = 1;
  cvk_ctx->ops->tiu_pt_convolution(cvk_ctx, &param);
}

static int conv3d_test_ndchw(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  int ret = 0;

  // input (N=1, IC=2, ID=4, IH=3, IW=3)
  int n = 1, ic = 2, id = 4, ih = 3, iw = 3;
  int input_shapes[5] = {n, ic, id, ih, iw};
  float input_data[] = {
      // IC=0
      0.6762, 0.9451, 0.9486,   // ic = 0, id = 0, ih = 0
      0.1077, 0.6062, 0.1011,   // ic = 0, id = 0, ih = 1
      0.1065, 0.9864, 0.8988,   // ic = 0, id = 0, ih = 2

      0.1986, 0.6289, 0.9028,
      0.6754, 0.3942, 0.3231,
      0.4473, 0.9430, 0.1674,

      0.8915, 0.2300, 0.2834,
      0.8005, 0.9905, 0.5067,
      0.5892, 0.3737, 0.1197,

      0.2946, 0.8567, 0.7306,
      0.6123, 0.9854, 0.4904,
      0.9217, 0.8343, 0.0686,

      // IC=1
      0.7664, 0.0755, 0.4231,   // ic = 1, id = 0, ih = 0
      0.4695, 0.5165, 0.9785,   // ic = 1, id = 0, ih = 1
      0.6668, 0.4878, 0.5354,   // ic = 1, id = 0, ih = 2

      0.1907, 0.7196, 0.7503,
      0.9623, 0.4420, 0.1084,
      0.5654, 0.9658, 0.8150,

      0.3203, 0.6839, 0.4136,
      0.3514, 0.4005, 0.4281,
      0.1185, 0.0036, 0.1968,

      0.8295, 0.1635, 0.6517,
      0.0113, 0.9510, 0.4708,
      0.0686, 0.1143, 0.6780
  };

  // tpu input shape (n=1, id=4, ic=2, ih=3, iw=3)
  float input_data_tpu[n * id * ic * ih * iw];
  convert_ncdhw_to_ndchw(input_data_tpu, input_data, input_shapes);

#if 0
  int input_shapes_tpu[5] = {n, id, ic, ih, iw};
  dumpFloatData(input_data_tpu, input_shapes_tpu);

  float input_data_tpu_ref[] = {
      0.676200, 0.945100, 0.948600, // id=0, ic=0, ih=0, iw=0
      0.107700, 0.606200, 0.101100,
      0.106500, 0.986400, 0.898800,

      0.766400, 0.075500, 0.423100, // id=0, ic=1, ih=0, iw=0
      0.469500, 0.516500, 0.978500,
      0.666800, 0.487800, 0.535400,

      0.198600, 0.628900, 0.902800, // id=1, ic=0, ih=0, iw=0
      0.675400, 0.394200, 0.323100,
      0.447300, 0.943000, 0.167400,

      0.190700, 0.719600, 0.750300, // id=1, ic=1, ih=0, iw=0
      0.962300, 0.442000, 0.108400,
      0.565400, 0.965800, 0.815000,

      0.891500, 0.230000, 0.283400, // id=2, ic=0, ih=0, iw=0
      0.800500, 0.990500, 0.506700,
      0.589200, 0.373700, 0.119700,

      0.320300, 0.683900, 0.413600, // id=2, ic=1, ih=0, iw=0
      0.351400, 0.400500, 0.428100,
      0.118500, 0.003600, 0.196800,

      0.294600, 0.856700, 0.730600, // id=3, ic=0, ih=0, iw=0
      0.612300, 0.985400, 0.490400,
      0.921700, 0.834300, 0.068600,

      0.829500, 0.163500, 0.651700, // id=3, ic=1, ih=0, iw=0
      0.011300, 0.951000, 0.470800,
      0.068600, 0.114300, 0.678000,
  };
#endif

  // pytorch weight (Oc=4, Ic=2, kd=2, kh=2, kw=2)
  int oc = 4, kd = 2, kh = 2, kw = 2;
  int weight_shapes[5] = {oc, ic, kd, kh, kw};
  float weight_data[] = {
       // OC=0
       0.1715,  0.1906,   // ic = 0, kd = 0, kh = 0
       0.0437, -0.0401,   // ic = 0, kd = 0, kh = 1

      -0.2442,  0.1911,   // ic = 0, kd = 1, kh = 0
      -0.0082, -0.0663,   // ic = 0, kd = 1, kh = 1

      -0.1137, -0.0246,   // ic = 1, kd = 0, kh = 0
       0.2495, -0.0684,   // ic = 1, kd = 0, kh = 1

      -0.0456,  0.0776,   // ic = 1, kd = 1, kh = 0
       0.1798,  0.1516,   // ic = 1, kd = 1, kh = 1

      // OC=1
       0.0527,  0.2034,
      -0.1434, -0.1642,

      -0.0797,  0.0839,
      -0.0746,  0.1446,

       0.1706,  0.1556,
       0.0149,  0.1610,

       0.0890,  0.0433,
       0.0363,  0.2293,

      // OC=2
       0.2052,  0.0489,
      -0.1775,  0.0486,

       0.1524,  0.0386,
       0.1624,  0.0692,

       0.1914,  0.0774,
      -0.1583,  0.1109,

       0.2034, -0.1709,
       0.1521, -0.1975,

      // OC=3
       0.1881,  0.1785,
       0.0584, -0.0217,

       0.1191,  0.2206,
       0.1310, -0.0952,

      -0.1424,  0.1071,
       0.0292, -0.1104,

       0.1335,  0.1561,
      -0.1034, -0.2354
  };

  // tpu weight shape (1, oc=4, kh(2)*kw(2), kd(2)*ic(2))
  float weight_data_tpu[oc * kh * kw * kd * ic];
  convert_tpu_weight_for_ndchw(weight_data_tpu, weight_data, weight_shapes);

#if 0
  int weight_shapes_tpu[5] = {oc, kh, kw, kd, ic};
  dumpFloatData(weight_data_tpu, weight_shapes_tpu);

  flaot weight_data_tpu_ref[] = {
       0.171500, -0.113700, // oc=0, kh=0, kw=0, kd=0, ic=0
      -0.244200, -0.045600,

       0.190600, -0.024600, // oc=0, kh=0, kw=1, kd=0, ic=0
       0.191100,  0.077600,

       0.043700,  0.249500, // oc=0, kh=1, kw=0, kd=0, ic=0
      -0.008200,  0.179800,

      -0.040100, -0.068400, // oc=0, kh=1, kw=1, kd=0, ic=0
      -0.066300,  0.151600,

       0.052700,  0.170600, // oc=1, kh=0, kw=0, kd=0, ic=0
      -0.079700,  0.089000,

       0.203400,  0.155600,
       0.083900,  0.043300,

      -0.143400,  0.014900,
      -0.074600,  0.036300,

      -0.164200,  0.161000,
       0.144600,  0.229300,

       0.205200,  0.191400,
       0.152400,  0.203400,

       0.048900,  0.077400,
       0.038600, -0.170900,

      -0.177500, -0.158300,
       0.162400,  0.152100,

       0.048600,  0.110900,
       0.069200, -0.197500,

       0.188100, -0.142400,
       0.119100,  0.133500,

       0.178500,  0.107100,
       0.220600,  0.156100,

       0.058400,  0.029200,
       0.131000, -0.103400,

      -0.021700, -0.110400,
      -0.095200, -0.235400,
  };
#endif

  // bias (4)
  float bias_data[] = {
    0.1204, -0.1286, -0.0339, -0.1120
  };

  // output (N=1, Oc=4, Od=3, Oh=2, Ow=2)
  int od = 3, oh = 2, ow = 2;
  int output_shapes[5] = {n, oc, od, oh, ow};
  float ref_output_data[] = {
      // OC=0
      0.7170, 0.6444,
      0.3692, 0.4852,

      0.3749, 0.5013,
      0.2489, 0.3058,

      0.4620, 0.3949,
      0.5157, 0.2879,

      // OC=1
      0.4449, 0.4349,
      0.5010, 0.1843,

      0.2726, 0.4384,
      0.2482, 0.0854,

      0.3631, 0.1475,
      0.2504, 0.2950,

      // OC=2
      0.4633, 0.4587,
      0.3968, 0.4154,

      0.1917, 0.5096,
      0.6285, 0.1435,

      0.3697, 0.3493,
      0.3388, 0.5705,

      // OC=3
      0.1802, 0.6468,
      0.0031, 0.0546,

      0.2840, 0.3474,
      0.3630, 0.2990,

      0.2374, 0.2000,
      0.6851, 0.5085
  };

  // tpu output (n, od, oc, oh, ow)
  float ref_output_data_ndchw[n * od * oc * oh * ow];
  convert_ncdhw_to_ndchw(ref_output_data_ndchw, ref_output_data, output_shapes);

  // dilation = (depth=1, height=1, width=1)
  int dilation_d = 1, dilation_h = 1, dilation_w = 1;

  // stride = (depth=1, height=1, width=1)
  int stride_d = 1, stride_h = 1, stride_w = 1;

  // zero padding
  int pad_d0 = 0, pad_d1 = 0;
  int pad_top = 0, pad_bot = 0, pad_left = 0, pad_right = 0;

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)] = {0.0};
  conv3d_float_ref_for_ncdhw(
      input_data, weight_data, bias_data, output_data_cpu,
      n, ic, id, ih, iw,
      oc, od, oh, ow,
      kd, kh, kw,
      stride_d, stride_h, stride_w,
      dilation_d, dilation_h, dilation_w,
      pad_d0, pad_d1,
      pad_top, pad_bot, pad_left, pad_right);

  printf("  %s: compare ref\n", __FUNCTION__);
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare ref %s\n", __FUNCTION__, ret ? "fail" : "pass");

  if (ret)
    return ret;

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_weight_al = NULL;
  cvk_tl_t *tl_bias_al = NULL;

  // Allocate output
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow};
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/1);
  }

  // Allocate input
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)id*ic, (uint32_t)ih, (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate weight
  {
    cvk_tl_shape_t shape = {
        1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)kd*ic};
    tl_weight_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  // Allocate bias
  // bias
  {
    cvk_tl_shape_t shape = {2, (uint32_t)oc, 1, 1};
    tl_bias_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/0);
  }

  assert(tl_output_al && tl_input_al && tl_weight_al && tl_bias_al &&
         "Expect all allocated");

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_weight_dev_mem = NULL;
  CVI_RT_MEM gm_bias_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_weight = 0;
  uint64_t ga_bias = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, id=4, ic=2, ih=3, iw=3)
    // reshape (1, id=4, ic=2, ih=3, iw=3) -> (1, 4, 2, 3x3)
    int total_len = 1 * id * ic * ih * iw;
    uint16_t input_bf16_data_tpu[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data_tpu, input_data_tpu,
                              total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem,
                      (uint8_t *)input_bf16_data_tpu);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of weight
  {
    int len = oc * kh * kw * kd * ic;
    uint16_t weight_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, weight_bf16_data_tpu, weight_data_tpu,
                              len);

    gm_weight_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_weight_dev_mem,
                      (uint8_t *)weight_bf16_data_tpu);

    ga_weight = CVI_RT_MemGetPAddr(gm_weight_dev_mem);
  }

  // Allocate device memory of bias
  {
    int len = oc;
    uint16_t bias_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, bias_bf16_data_tpu, bias_data, len);

    gm_bias_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_bias_dev_mem,
                      (uint8_t *)bias_bf16_data_tpu);

    ga_bias = CVI_RT_MemGetPAddr(gm_bias_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * od * oc * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && gm_weight_dev_mem &&
         gm_bias_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && ga_weight && ga_bias && "Expect valid gaddr");

  load_bias(cvk_ctx, ga_bias, tl_bias_al);
  load_weight_for_ndchw(cvk_ctx, oc, ic, kd, kh, kw, ga_weight, tl_weight_al);
  load_input_for_ndchw(cvk_ctx,
                       n, ic, id, ih, iw,
                       pad_d0, pad_d1,
                       ga_input, tl_input_al);

  for (int odi = 0; odi < od; odi++) {
    compute_for_ndchw(cvk_ctx,
                      ic, id, ih, iw,
                      kd, kh, kw,
                      stride_d, stride_h, stride_w,
                      pad_top, pad_bot, pad_left, pad_right,
                      oc, oh, ow,
                      odi,
                      tl_input_al,
                      tl_weight_al,
                      tl_bias_al,
                      tl_output_al);
    store_output_for_ndchw(cvk_ctx,
                           oc, od, oh, ow,
                           odi,
                           ga_output,
                           tl_output_al);
  }

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * od * oc * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  %s: compare tpu\n", __FUNCTION__);
  const float tpu_precision = 0.01;
  for (int i = 0; i < output_len; i++) {
    float tpu_data = cvk_convert_bf16_fp32(output_bf16_data_tpu[i]);
    if (fabs(tpu_data - ref_output_data_ndchw[i]) > tpu_precision) {
      printf("    [%d] Error ! val %f(0x%x), expected %f(0x%x)\n",
             (int)i, tpu_data, output_bf16_data_tpu[i],
             ref_output_data_ndchw[i], ref_output_bf16_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare tpu %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bias_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_weight_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_weight_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_bias_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_output_dev_mem);

#ifdef DUMP_MSG
  printf("<= %s\n", __FUNCTION__);
  printf("===================================\n\n");
#endif

  return ret;
}

// # pytorch
// #
// #        N  IC  ID  IH  IW
// # input (1,  2,  4,  3,  3)
// #
// #        OC  IC  KD  KH  KW
// # kernel (4,  2,  2,  2,  2)
// #
// #         N  OC  OD  OH  OW
// # output (1,  4,  3,  4,  4)
// #
// #            IC OC  KD KH KW
// m = nn.Conv3d(2, 4, [2, 2, 2], stride=(1, 1, 1), padding=(0, 1, 1))
// input = torch.rand(1, 2, 4, 3, 3)
// output = m(input)
//
static int conv3d_test_ndchw_pad_hw(CVI_RT_HANDLE rt_handle,
                                    cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  int ret = 0;

  // input (N=1, IC=2, ID=4, IH=3, IW=3)
  int n = 1, ic = 2, id = 4, ih = 3, iw = 3;
  int input_shapes[5] = {n, ic, id, ih, iw};
  float input_data[] = {
      // IC=0
      0.3519, 0.4651, 0.9173,   // ic = 0, id = 0, ih = 0
      0.5175, 0.1068, 0.8725,   // ic = 0, id = 0, ih = 1
      0.3365, 0.7358, 0.0661,   // ic = 0, id = 0, ih = 2

      0.7301, 0.5292, 0.8247,
      0.3301, 0.4827, 0.9843,
      0.8852, 0.3967, 0.9667,

      0.2098, 0.7486, 0.9042,
      0.6317, 0.7464, 0.0149,
      0.0671, 0.2331, 0.2570,

      0.0870, 0.1124, 0.6606,
      0.3854, 0.9084, 0.2225,
      0.6355, 0.1670, 0.8032,

      // IC=1
      0.8309, 0.8918, 0.3241,   // ic = 1, id = 0, ih = 0
      0.1311, 0.1492, 0.8990,   // ic = 1, id = 0, ih = 1
      0.4111, 0.4590, 0.4945,   // ic = 1, id = 0, ih = 2

      0.4529, 0.5284, 0.3906,
      0.4197, 0.4409, 0.8277,
      0.4973, 0.2427, 0.5823,

      0.3362, 0.3004, 0.5242,
      0.9657, 0.9594, 0.0147,
      0.6477, 0.6943, 0.2552,

      0.8387, 0.4657, 0.5361,
      0.4703, 0.6478, 0.8114,
      0.6255, 0.6939, 0.2478
  };

  // tpu input shape (n=1, id=4, ic=2, ih=3, iw=3)
  float input_data_tpu[n * id * ic * ih * iw];
  convert_ncdhw_to_ndchw(input_data_tpu, input_data, input_shapes);

  // pytorch weight (Oc=4, Ic=2, kd=2, kh=2, kw=2)
  int oc = 4, kd = 2, kh = 2, kw = 2;
  int weight_shapes[5] = {oc, ic, kd, kh, kw};
  float weight_data[] = {
      // OC=0
       0.0405, -0.1043,   // ic = 0, kd = 0, kh = 0
       0.1978,  0.0733,   // ic = 0, kd = 0, kh = 1

      -0.1659, -0.1932,   // ic = 0, kd = 1, kh = 0
       0.0010, -0.2304,   // ic = 0, kd = 1, kh = 1

      -0.2116, -0.0438,   // ic = 1, kd = 0, kh = 0
      -0.1418, -0.1537,   // ic = 1, kd = 0, kh = 1

       0.0897,  0.0856,   // ic = 1, kd = 1, kh = 0
      -0.0758,  0.1184,   // ic = 1, kd = 1, kh = 1

      // OC=1
      -0.1565, -0.0361,
       0.0632,  0.1512,

      -0.2068,  0.0470,
       0.1306,  0.1910,

      -0.2418, -0.0457,
       0.1569,  0.0661,

       0.0540,  0.0379,
       0.2167, -0.0688,

      // OC=2
      -0.0279, -0.1743,
      -0.2304, -0.0677,

       0.0108, -0.0891,
      -0.0099, -0.0792,

       0.1598,  0.0055,
      -0.2395, -0.0643,

      -0.0918,  0.0547,
       0.0517, -0.0688,

      // OC=3
       0.0506,  0.1193,
      -0.0683, -0.1807,

      -0.1150, -0.0912,
      -0.0225, -0.1393,

       0.0520,  0.1461,
       0.1875, -0.2178,

       0.0830,  0.1741,
      -0.0341,  0.0611
  };

  // tpu weight shape (1, oc=4, kh(2)*kw(2), kd(2)*ic(2))
  float weight_data_tpu[oc * kh * kw * kd * ic];
  convert_tpu_weight_for_ndchw(weight_data_tpu, weight_data, weight_shapes);

  // bias (4)
  float bias_data[] = {
      0.1053, -0.0710,  0.0016, -0.1625
  };

  // output (N=1, Oc=4, Od=3, Oh=4, Ow=4)
  int od = 3, oh = 4, ow = 4;
  int output_shapes[5] = {n, oc, od, oh, ow};
  float ref_output_data[] = {
      // OC=0
      -1.1119e-01, -1.3884e-01, -9.5057e-02,  2.1200e-01,
      -7.8709e-02, -3.0321e-01, -5.7672e-01, -4.4566e-02,
      -1.6587e-01, -9.9638e-02, -3.7471e-01, -2.3886e-01,
      -7.6292e-02, -2.2308e-01, -1.7159e-01, -1.0482e-01,

       8.0675e-02, -1.9141e-02, -3.2850e-02,  1.7421e-01,
      -7.3950e-02, -3.2045e-01, -4.1114e-01,  2.9242e-02,
       6.2759e-02, -4.4301e-02, -2.0282e-01,  5.8408e-02,
       3.3655e-02,  4.5276e-02, -6.0553e-02,  1.4932e-03,

       1.4830e-01,  7.3535e-02,  7.2533e-02,  1.6984e-01,
      -1.1557e-02, -2.4226e-01, -9.6795e-02, -9.0924e-02,
      -2.0409e-01, -5.0648e-01, -4.1673e-01,  1.3535e-01,
       6.8198e-04, -1.0596e-01, -1.6961e-01, -4.9334e-02,

      // OC=1
       1.4539e-01,  4.6912e-01,  5.7270e-01,  2.3017e-01,
       5.0830e-02, -1.9102e-01,  7.6440e-02,  6.1754e-02,
       1.4862e-01,  3.0930e-01,  2.1541e-01, -2.4954e-01,
      -4.1520e-02, -3.9902e-01, -3.2361e-01, -3.6940e-01,

       8.6251e-02,  3.8368e-01,  4.9539e-01,  2.7409e-01,
       3.6369e-02,  2.4048e-01,  2.0503e-01, -2.5604e-01,
       9.9149e-02,  8.7076e-02,  3.2400e-02, -1.8619e-01,
      -9.8027e-02, -2.9687e-01, -2.4229e-01, -4.0250e-01,

      -5.8173e-02,  3.1059e-01,  3.9969e-01,  2.7085e-01,
       1.4254e-01,  4.7329e-01,  1.8244e-01, -2.3881e-01,
       2.9225e-02, -7.1563e-02, -4.4758e-02,  1.3586e-01,
      -4.9485e-02, -3.4177e-01, -2.4624e-01, -3.2567e-01,

      // OC=2
      -1.6466e-01, -4.2935e-01, -4.7211e-01, -2.7528e-01,
      -1.9397e-01, -2.2121e-01, -4.1602e-01, -3.8243e-01,
      -2.4789e-01, -3.5001e-01, -6.2618e-01, -5.7601e-02,
      -1.0650e-01, -1.2604e-01, -2.6756e-02,  3.5703e-02,

      -1.1669e-01, -4.0951e-01, -4.2731e-01, -2.6376e-01,
      -2.8931e-01, -4.3372e-01, -4.3838e-01, -4.2177e-01,
      -1.9895e-01, -5.1715e-01, -4.4574e-01, -2.4636e-01,
      -1.2053e-01, -5.3043e-02, -2.0617e-01,  4.6975e-02,

      -9.8851e-02, -1.9570e-01, -4.0398e-01, -3.1105e-01,
      -1.6280e-01, -7.2503e-01, -6.4973e-01,  5.0791e-02,
      -2.5145e-01, -3.3708e-01, -1.9385e-01, -1.8400e-01,
      -2.8993e-02,  3.8838e-02, -5.7331e-02,  2.1099e-02,

      // OC=3
      -4.8111e-01, -3.8231e-01, -3.8434e-01, -1.9630e-01,
      -1.2935e-01, -4.2991e-02, -4.0529e-01, -1.0304e-01,
      -2.8196e-01, -3.2151e-01, -7.8678e-02, -6.9456e-02,
      -5.6521e-02, -2.3751e-02, -3.3585e-02, -1.9628e-01,

      -4.0179e-01, -4.4029e-01, -4.5469e-01, -1.8383e-01,
      -1.4998e-01, -1.9349e-01, -3.6435e-01, -7.3818e-02,
      -1.8945e-01, -9.8586e-04, -2.1411e-01, -4.1524e-02,
       1.2230e-01,  1.3655e-01,  1.2234e-01, -9.1697e-02,

      -2.3454e-01, -3.3230e-01, -5.1257e-01, -1.5916e-01,
      -2.9983e-01, -1.8840e-01,  2.3329e-01, -1.5189e-01,
      -1.0291e-01,  8.0501e-02, -1.1257e-01, -1.1544e-01,
      -9.0000e-03,  8.8157e-02, -3.8429e-02, -2.0804e-01,
  };

  // tpu output (n, od, oc, oh, ow)
  float ref_output_data_ndchw[n * od * oc * oh * ow];
  convert_ncdhw_to_ndchw(ref_output_data_ndchw, ref_output_data, output_shapes);

  // dilation = (depth=1, height=1, width=1)
  int dilation_d = 1, dilation_h = 1, dilation_w = 1;

  // stride = (depth=1, height=1, width=1)
  int stride_d = 1, stride_h = 1, stride_w = 1;

  // padding = (1, 1, 1)
  int pad_d0 = 0, pad_d1 = 0;
  int pad_top = 1, pad_bot = 1, pad_left = 1, pad_right = 1;

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)] = {0.0};
  conv3d_float_ref_for_ncdhw(
      input_data, weight_data, bias_data, output_data_cpu,
      n, ic, id, ih, iw,
      oc, od, oh, ow,
      kd, kh, kw,
      stride_d, stride_h, stride_w,
      dilation_d, dilation_h, dilation_w,
      pad_d0, pad_d1,
      pad_top, pad_bot, pad_left, pad_right);

  printf("  %s: compare ref\n", __FUNCTION__);
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare ref %s\n", __FUNCTION__, ret ? "fail" : "pass");
  if (ret)
    return ret;

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_weight_al = NULL;
  cvk_tl_t *tl_bias_al = NULL;

  // Allocate output
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow};
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/1);
  }

  // Allocate input
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)id*ic, (uint32_t)ih, (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate weight
  {
    cvk_tl_shape_t shape = {
        1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)kd*ic};
    tl_weight_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  // Allocate bias
  // bias
  {
    cvk_tl_shape_t shape = {2, (uint32_t)oc, 1, 1};
    tl_bias_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/0);
  }

  assert(tl_output_al && tl_input_al && tl_weight_al && tl_bias_al &&
         "Expect all allocated");

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_weight_dev_mem = NULL;
  CVI_RT_MEM gm_bias_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_weight = 0;
  uint64_t ga_bias = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, id=4, ic=2, ih=3, iw=3)
    // reshape (1, id=4, ic=2, ih=3, iw=3) -> (1, 4, 2, 3x3)
    int total_len = 1 * id * ic * ih * iw;
    uint16_t input_bf16_data_tpu[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data_tpu, input_data_tpu,
                              total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem,
                      (uint8_t *)input_bf16_data_tpu);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of weight
  {
    int len = oc * kh * kw * kd * ic;
    uint16_t weight_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, weight_bf16_data_tpu, weight_data_tpu,
                              len);

    gm_weight_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_weight_dev_mem,
                      (uint8_t *)weight_bf16_data_tpu);

    ga_weight = CVI_RT_MemGetPAddr(gm_weight_dev_mem);
  }

  // Allocate device memory of bias
  {
    int len = oc;
    uint16_t bias_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, bias_bf16_data_tpu, bias_data, len);

    gm_bias_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_bias_dev_mem,
                      (uint8_t *)bias_bf16_data_tpu);

    ga_bias = CVI_RT_MemGetPAddr(gm_bias_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * od * oc * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && gm_weight_dev_mem &&
         gm_bias_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && ga_weight && ga_bias && "Expect valid gaddr");

  load_bias(cvk_ctx, ga_bias, tl_bias_al);
  load_weight_for_ndchw(cvk_ctx, oc, ic, kd, kh, kw, ga_weight, tl_weight_al);
  load_input_for_ndchw(cvk_ctx,
                       n, ic, id, ih, iw,
                       pad_d0, pad_d1,
                       ga_input, tl_input_al);

  for (int odi = 0; odi < od; odi++) {
    compute_for_ndchw(cvk_ctx,
                      ic, id, ih, iw,
                      kd, kh, kw,
                      stride_d, stride_h, stride_w,
                      pad_top, pad_bot, pad_left, pad_right,
                      oc, oh, ow,
                      odi,
                      tl_input_al,
                      tl_weight_al,
                      tl_bias_al,
                      tl_output_al);
    store_output_for_ndchw(cvk_ctx,
                           oc, od, oh, ow,
                           odi,
                           ga_output,
                           tl_output_al);
  }

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * od * oc * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  %s: compare tpu\n", __FUNCTION__);
  const float tpu_precision = 0.01;
  for (int i = 0; i < output_len; i++) {
    float tpu_data = cvk_convert_bf16_fp32(output_bf16_data_tpu[i]);
    if (fabs(tpu_data - ref_output_data_ndchw[i]) > tpu_precision) {
      printf("    [%d] Error ! val %f(0x%x), expected %f(0x%x)\n",
             (int)i, tpu_data, output_bf16_data_tpu[i],
             ref_output_data_ndchw[i], ref_output_bf16_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare tpu %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bias_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_weight_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_weight_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_bias_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_output_dev_mem);

#ifdef DUMP_MSG
  printf("<= %s\n", __FUNCTION__);
  printf("===================================\n\n");
#endif

  return ret;
}

// # pytorch
// #
// #        N  IC  ID  IH  IW
// # input (1,  2,  4,  3,  3)
// #
// #        OC  IC  KD  KH  KW
// # kernel (4,  2,  2,  2,  2)
// #
// #         N  OC  OD  OH  OW
// # output (1,  4,  5,  4,  4)
// #
// #            IC OC  KD KH KW
// m = nn.Conv3d(2, 4, [2, 2, 2], stride=(1, 1, 1), padding=(1, 1, 1))
// input = torch.rand(1, 2, 4, 3, 3)
// output = m(input)
//
static int conv3d_test_ndchw_pad_dhw(CVI_RT_HANDLE rt_handle,
                                     cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  int ret = 0;

  // input (N=1, IC=2, ID=4, IH=3, IW=3)
  int n = 1, ic = 2, id = 4, ih = 3, iw = 3;
  int input_shapes[5] = {n, ic, id, ih, iw};
  float input_data[] = {
      // IC=0
      0.3307, 0.6577, 0.3520,   // ic = 0, id = 0, ih = 0
      0.5691, 0.1531, 0.6240,   // ic = 0, id = 0, ih = 1
      0.4324, 0.9731, 0.4587,   // ic = 0, id = 0, ih = 2

      0.6121, 0.5937, 0.8512,
      0.7932, 0.3473, 0.4032,
      0.0156, 0.6799, 0.8587,

      0.9278, 0.1046, 0.2478,
      0.4399, 0.2543, 0.8906,
      0.0275, 0.0450, 0.1212,

      0.5655, 0.6741, 0.3396,
      0.6126, 0.6385, 0.5160,
      0.9062, 0.5286, 0.7064,

      // IC=1
      0.0512, 0.9951, 0.8289,   // ic = 1, id = 0, ih = 0
      0.9011, 0.0602, 0.5583,   // ic = 1, id = 0, ih = 1
      0.5176, 0.9857, 0.8772,   // ic = 1, id = 0, ih = 2

      0.8971, 0.5207, 0.1500,
      0.8408, 0.2034, 0.7618,
      0.7618, 0.0702, 0.9254,

      0.2110, 0.1366, 0.5222,
      0.0626, 0.9902, 0.2842,
      0.0101, 0.6390, 0.0038,

      0.7045, 0.3892, 0.7232,
      0.7224, 0.8458, 0.6474,
      0.0602, 0.9074, 0.4171
  };

  // tpu input shape (n=1, id=4, ic=2, ih=3, iw=3)
  float input_data_tpu[n * id * ic * ih * iw];
  convert_ncdhw_to_ndchw(input_data_tpu, input_data, input_shapes);

  // pytorch weight (Oc=4, Ic=2, kd=2, kh=2, kw=2)
  int oc = 4, kd = 2, kh = 2, kw = 2;
  int weight_shapes[5] = {oc, ic, kd, kh, kw};
  float weight_data[] = {
      // OC=0
      -0.2046, -0.2492,   // ic = 0, kd = 0, kh = 0
      -0.0783,  0.1082,   // ic = 0, kd = 0, kh = 1

       0.1393, -0.1803,   // ic = 0, kd = 1, kh = 0
      -0.0110, -0.1141,   // ic = 0, kd = 1, kh = 1

       0.0606,  0.1902,   // ic = 1, kd = 0, kh = 0
       0.1254,  0.1572,   // ic = 1, kd = 0, kh = 1

       0.0887, -0.0336,   // ic = 1, kd = 1, kh = 0
       0.0918, -0.1099,   // ic = 1, kd = 1, kh = 1

      // OC=1
      -0.0181, -0.2228,
      -0.0575, -0.2464,

      -0.0757, -0.0122,
      -0.1896,  0.1301,

      -0.0215,  0.0568,
      -0.1381, -0.1621,

      -0.1247, -0.0738,
      -0.0146,  0.0719,

      // OC=2
       0.0960, -0.1865,
      -0.2124, -0.0125,

       0.0159,  0.1148,
       0.1430,  0.1978,

       0.0292, -0.2130,
       0.2055,  0.1678,

       0.2236, -0.0215,
      -0.2171,  0.1709,

      // OC=3
       0.2186,  0.1488,
       0.1558,  0.0359,

       0.1822, -0.0433,
       0.0960,  0.1791,

      -0.0060,  0.0006,
       0.0400,  0.1488,

       0.1811, -0.1055,
       0.1138, -0.0898
  };

  // tpu weight shape (1, oc=4, kh(2)*kw(2), kd(2)*ic(2))
  float weight_data_tpu[oc * kh * kw * kd * ic];
  convert_tpu_weight_for_ndchw(weight_data_tpu, weight_data, weight_shapes);

  // bias (4)
  float bias_data[] = {
      -0.2107, -0.1894, -0.0108,  0.1728
  };

  // output (N=1, Oc=4, Od=5, Oh=4, Ow=4)
  int od = 5, oh = 4, ow = 4;
  int output_shapes[5] = {n, oc, od, oh, ow};
  float ref_output_data[] = {
      // OC=0
      -2.5403e-01, -3.9400e-01, -2.5784e-01, -1.3846e-01,
      -4.3596e-01, -2.5972e-01, -2.5080e-01, -4.3702e-02,
      -4.4977e-01, -2.5769e-01, -3.8422e-01,  1.2387e-03,
      -3.0602e-01, -3.1306e-01, -9.9820e-02, -6.8943e-02,

      -3.3526e-01, -5.1864e-02, -4.1363e-02, -1.2992e-01,
      -4.0344e-01, -1.0866e-01, -2.0857e-01, -1.3983e-02,
      -3.0966e-01,  9.2221e-02, -2.8528e-01, -3.1210e-02,
      -2.4841e-01, -3.7795e-01, -3.8244e-01, -4.9618e-02,

      -1.3243e-01, -1.7816e-02, -1.5046e-01, -2.1334e-01,
      -2.0596e-01, -2.3001e-01, -4.0274e-01, -2.1468e-01,
      -2.1257e-01, -2.7799e-01, -3.3916e-02, -4.9950e-02,
      -7.4998e-02, -3.4861e-01, -3.4250e-01, -3.1303e-01,

      -2.1902e-01, -2.8536e-01, -1.8272e-01, -1.0197e-01,
      -6.1921e-01, -3.3074e-01,  4.2541e-02, -9.8628e-02,
      -5.4856e-01, -2.2603e-01, -2.8005e-01, -2.2485e-01,
      -3.8100e-01, -9.9595e-02, -1.9782e-01, -9.9829e-02,

      -3.8680e-02, -3.2488e-02, -6.4191e-02, -1.4660e-01,
      -3.7711e-02, -1.3294e-01, -5.8401e-02, -1.9556e-01,
      -1.1838e-01, -1.5398e-01, -8.1088e-02, -2.8004e-01,
      -4.2503e-01, -3.5157e-01, -3.6049e-01, -3.2990e-01,

      // OC=1
      -1.4269e-01, -9.5723e-02, -2.2320e-01, -2.6823e-01,
      -5.8374e-02, -3.9911e-01, -3.3735e-01, -4.4587e-01,
      -1.6939e-01, -2.4322e-01, -3.3348e-01, -4.0600e-01,
      -2.3289e-01, -3.7133e-01, -4.5634e-01, -3.3350e-01,

      -1.3504e-01, -5.5332e-01, -5.8443e-01, -4.8773e-01,
      -4.5654e-01, -7.9793e-01, -6.0842e-01, -4.9727e-01,
      -4.7046e-01, -8.5047e-01, -8.1262e-01, -6.6208e-01,
      -3.1279e-01, -4.7892e-01, -4.1965e-01, -3.9698e-01,

      -3.4979e-01, -7.3478e-01, -4.8156e-01, -3.1361e-01,
      -5.7180e-01, -6.9073e-01, -6.5631e-01, -5.9334e-01,
      -4.5140e-01, -6.4365e-01, -8.3343e-01, -5.1614e-01,
      -1.5070e-01, -4.0468e-01, -4.2686e-01, -2.3451e-01,

      -3.2802e-01, -3.2160e-01, -3.9730e-01, -3.5070e-01,
      -4.2998e-01, -6.3385e-01, -8.1355e-01, -5.1874e-01,
      -2.3089e-01, -5.6220e-01, -7.1846e-01, -4.7895e-01,
      -2.1047e-01, -3.1337e-01, -4.2336e-01, -2.9714e-01,

      -4.4296e-01, -5.4842e-01, -4.8282e-01, -3.0881e-01,
      -5.4348e-01, -7.7235e-01, -6.3022e-01, -3.3020e-01,
      -5.1796e-01, -6.4802e-01, -6.9482e-01, -3.1089e-01,
      -3.8791e-01, -2.7335e-01, -3.5223e-01, -2.1117e-01,

      // OC=2
       6.3343e-02,  3.2551e-01,  7.8462e-02, -1.4047e-01,
       2.9263e-01, -1.3682e-02,  4.7238e-01,  1.4810e-01,
       2.0917e-01,  5.2640e-01,  2.3049e-01, -9.6724e-04,
       2.7707e-02,  2.0233e-01,  2.5884e-01,  1.9259e-01,

       2.6804e-01,  1.8736e-01,  3.5448e-01,  1.7387e-01,
       4.1227e-01,  6.1802e-02,  3.4067e-01, -3.1375e-02,
      -2.1211e-02,  4.1589e-01,  3.9848e-01,  2.4676e-01,
      -2.1633e-01, -9.8574e-02, -5.5862e-02,  2.7933e-01,

       3.5165e-01,  2.5434e-01,  1.0813e-01, -2.3880e-01,
       1.4803e-02,  2.2636e-01,  5.6942e-02,  3.3249e-01,
      -1.5394e-01,  2.8699e-01,  1.9381e-02,  1.5203e-01,
      -1.7307e-01, -1.3476e-01, -1.4338e-01,  1.0136e-01,

       2.4526e-01, -1.5181e-02,  2.8220e-01, -6.4634e-02,
       7.0619e-02,  5.5526e-01,  2.7332e-01, -2.2993e-03,
       1.3952e-01,  4.8027e-01,  2.7088e-01,  2.2137e-01,
       8.4666e-02, -8.3372e-02,  2.7218e-01,  1.0539e-01,

       1.0030e-01,  7.0672e-02,  4.3040e-02,  6.5646e-02,
      -1.5283e-01,  7.5928e-03, -1.1840e-02,  6.6282e-02,
      -2.8021e-01, -2.6473e-01, -2.3691e-02, -6.7624e-03,
      -1.9269e-01, -2.1400e-01, -1.5423e-01,  6.9144e-02,

      // OC=3
       2.2745e-01,  2.3889e-01,  3.3790e-01,  3.0098e-01,
       1.7417e-01,  2.8818e-01,  4.5343e-01,  5.1056e-01,
       8.4155e-02,  6.1303e-01,  3.3481e-01,  5.3153e-01,
       9.9508e-02,  1.9928e-01,  4.1631e-01,  4.1526e-01,

       2.2143e-01,  6.1859e-01,  7.0634e-01,  3.5959e-01,
       3.2209e-01,  8.9161e-01,  7.0540e-01,  6.7204e-01,
       1.6198e-01,  1.0484e+00,  7.8346e-01,  8.1161e-01,
       1.5643e-01,  5.1360e-01,  4.5026e-01,  5.9188e-01,

       4.7556e-01,  5.2241e-01,  3.6209e-01,  3.9465e-01,
       4.2880e-01,  7.8418e-01,  8.6553e-01,  7.0884e-01,
       3.8365e-01,  3.9124e-01,  8.4084e-01,  6.5296e-01,
       1.7330e-01,  2.1039e-01,  5.6763e-01,  3.7774e-01,

       2.7558e-01,  5.7020e-01,  3.8613e-01,  3.4723e-01,
       2.8224e-01,  9.5750e-01,  6.7976e-01,  6.9007e-01,
       2.9504e-01,  6.4116e-01,  8.1472e-01,  7.1143e-01,
       1.3136e-01,  2.4329e-01,  3.8296e-01,  4.0355e-01,

       2.9795e-01,  3.7119e-01,  4.1321e-01,  2.5462e-01,
       3.8687e-01,  6.6586e-01,  6.1692e-01,  3.4898e-01,
       3.0586e-01,  6.9549e-01,  5.9051e-01,  4.0845e-01,
       3.0771e-01,  4.4973e-01,  3.8828e-01,  3.2473e-01,
  };

  // tpu output (n, od, oc, oh, ow)
  float ref_output_data_ndchw[n * od * oc * oh * ow];
  convert_ncdhw_to_ndchw(ref_output_data_ndchw, ref_output_data, output_shapes);

  // dilation = (depth=1, height=1, width=1)
  int dilation_d = 1, dilation_h = 1, dilation_w = 1;

  // stride = (depth=1, height=1, width=1)
  int stride_d = 1, stride_h = 1, stride_w = 1;

  // padding = (1, 1, 1)
  int pad_d0 = 1, pad_d1 = 1;
  int pad_top = 1, pad_bot = 1, pad_left = 1, pad_right = 1;

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)] = {0.0};
  conv3d_float_ref_for_ncdhw(
      input_data, weight_data, bias_data, output_data_cpu,
      n, ic, id, ih, iw,
      oc, od, oh, ow,
      kd, kh, kw,
      stride_d, stride_h, stride_w,
      dilation_d, dilation_h, dilation_w,
      pad_d0, pad_d1,
      pad_top, pad_bot, pad_left, pad_right);

  printf("  %s: compare ref\n", __FUNCTION__);
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare ref %s\n", __FUNCTION__, ret ? "fail" : "pass");

  if (ret)
    return ret;

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_weight_al = NULL;
  cvk_tl_t *tl_bias_al = NULL;

  // Allocate output
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)oc, (uint32_t)oh, (uint32_t)ow};
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/1);
  }

  // Allocate input
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)(id + pad_d0 + pad_d1) * ic, (uint32_t)ih,
        (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate weight
  {
    cvk_tl_shape_t shape = {
        1, (uint32_t)oc, (uint32_t)(kh*kw), (uint32_t)kd*ic};
    tl_weight_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  // Allocate bias
  // bias
  {
    cvk_tl_shape_t shape = {2, (uint32_t)oc, 1, 1};
    tl_bias_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/0);
  }

  assert(tl_output_al && tl_input_al && tl_weight_al && tl_bias_al &&
         "Expect all allocated");

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_weight_dev_mem = NULL;
  CVI_RT_MEM gm_bias_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_weight = 0;
  uint64_t ga_bias = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, id=4, ic=2, ih=3, iw=3)
    // reshape (1, id=4, ic=2, ih=3, iw=3) -> (1, 4, 2, 3x3)
    int total_len = 1 * id * ic * ih * iw;
    uint16_t input_bf16_data_tpu[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data_tpu, input_data_tpu,
                              total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem,
                      (uint8_t *)input_bf16_data_tpu);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of weight
  {
    int len = oc * kh * kw * kd * ic;
    uint16_t weight_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, weight_bf16_data_tpu, weight_data_tpu,
                              len);

    gm_weight_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_weight_dev_mem,
                      (uint8_t *)weight_bf16_data_tpu);

    ga_weight = CVI_RT_MemGetPAddr(gm_weight_dev_mem);
  }

  // Allocate device memory of bias
  {
    int len = oc;
    uint16_t bias_bf16_data_tpu[len];
    convert_fp32_to_bf16_data(cvk_ctx, bias_bf16_data_tpu, bias_data, len);

    gm_bias_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_bias_dev_mem,
                      (uint8_t *)bias_bf16_data_tpu);

    ga_bias = CVI_RT_MemGetPAddr(gm_bias_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * od * oc * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && gm_weight_dev_mem &&
         gm_bias_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && ga_weight && ga_bias && "Expect valid gaddr");

  load_bias(cvk_ctx, ga_bias, tl_bias_al);
  load_weight_for_ndchw(cvk_ctx, oc, ic, kd, kh, kw, ga_weight, tl_weight_al);
  load_input_for_ndchw(cvk_ctx,
                       n, ic, id, ih, iw,
                       pad_d0, pad_d1,
                       ga_input, tl_input_al);

  for (int odi = 0; odi < od; odi++) {
    compute_for_ndchw(cvk_ctx,
                      ic, id, ih, iw,
                      kd, kh, kw,
                      stride_d, stride_h, stride_w,
                      pad_top, pad_bot, pad_left, pad_right,
                      oc, oh, ow,
                      odi,
                      tl_input_al,
                      tl_weight_al,
                      tl_bias_al,
                      tl_output_al);
    store_output_for_ndchw(cvk_ctx,
                           oc, od, oh, ow,
                           odi,
                           ga_output,
                           tl_output_al);
  }

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * od * oc * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  %s: compare tpu\n", __FUNCTION__);
  const float tpu_precision = 0.01;
  for (int i = 0; i < output_len; i++) {
    float tpu_data = cvk_convert_bf16_fp32(output_bf16_data_tpu[i]);
    if (fabs(tpu_data - ref_output_data_ndchw[i]) > tpu_precision) {
      printf("    [%d] Error ! val %f(0x%x), expected %f(0x%x)\n",
             (int)i, tpu_data, output_bf16_data_tpu[i],
             ref_output_data_ndchw[i], ref_output_bf16_data[i]);
      ret = -1;
    }
  }
  printf("  %s: compare tpu %s\n", __FUNCTION__, ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bias_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_weight_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_weight_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_bias_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_output_dev_mem);

#ifdef DUMP_MSG
  printf("<= %s\n", __FUNCTION__);
  printf("===================================\n\n");
#endif

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;

  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_HANDLE rt_handle;
  cvk_context_t *cvk_ctx = NULL;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = (cvk_context_t *)CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);

  ret |= conv3d_test_ncdhw_pad_dhw(rt_handle, cvk_ctx);
  if (ret)
    goto end;

  ret |= conv3d_test_ndchw_pad_dhw(rt_handle, cvk_ctx);
  if (ret)
    goto end;

  ret |= conv3d_test_ndchw_pad_hw(rt_handle, cvk_ctx);
  if (ret)
    goto end;

  ret |= conv3d_test_ndchw(rt_handle, cvk_ctx);
  if (ret)
    goto end;

  ret |= conv3d_test(rt_handle, cvk_ctx);

end:
  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  if (!ret)
    printf("%s pass\n", __FILENAME__);
  else
    printf("%s fail\n", __FILENAME__);

  return ret;
}
