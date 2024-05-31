#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <cmath>
#include "test_cvikernel_util.h"
#include "test_tf_quant_util.h"
#include "test_native_ref.h"

// #define DUMP_MSG

#define TEST_CASE_NAME    "test_cv1880v2_pool3d_bf16"

// input (n, c, id, ih, iw)
// weight (kd, kh, kw), stride (sd, sh, sw)
// output (n, c, od, oh, ow)
void pool3d_float_ref(float *input, float *output,
    int input_n, int input_c, int input_d, int input_h, int input_w,
    int output_d, int output_h, int output_w,
    int kernel_d, int kernel_h, int kernel_w,
    int stride_d, int stride_h, int stride_w,
    int pad_d0, int pad_d1,
    int pad_top, int pad_bot, int pad_left, int pad_right)
{
  (void)pad_d1;
  (void)pad_bot;
  (void)pad_right;

#ifdef DUMP_MSG
  printf("  %s =>\n", __FUNCTION__);
#endif

  int input_shapes[5] = {input_n, input_c, input_d, input_h, input_w};
  int input_strides[5];

  int output_shapes[5] = {input_n, input_c, output_d, output_h, output_w};
  int output_strides[5];

  // logical stride, in unit of float
  get_strides_from_shapes5d(input_strides, input_shapes, 1);
  get_strides_from_shapes5d(output_strides, output_shapes, 1);

  for (int i = 0; i < input_n; i++) {
    for (int c = 0; c < input_c; c++) {
      for (int oz = 0; oz < output_d; oz++) {
        for (int oy = 0; oy < output_h; oy++) {
          for (int ox = 0; ox < output_w; ox++) {
            float max_value = -std::numeric_limits<float>::infinity();

            for (int pd = 0; pd < kernel_d; pd++) {
              int iz = oz * stride_d + pd - pad_d0;
              for (int py = 0; py < kernel_h; py++) {
                int iy = oy * stride_h + py - pad_top;
                for (int px = 0; px < kernel_w; px++) {
                  int ix = ox * stride_w + px - pad_left;
                  if (iz < input_d && iy < input_h && ix < input_w) {
                    int poss[5] = {i, c, iz, iy, ix};
                    int input_offset = get_tensor5d_offset(poss, input_strides);
                    max_value = std::fmax(max_value, input[input_offset]);

#ifdef DUMP_MSG
                    printf("  [i=%d][c=%d][oz=%d][oy=%d][ox=%d]" \
                           "[pd=%d][py=%d][px=%d] input(%d, %d, %d, %d, %d)"\
                           "(offset=%d)=%f, max=%f\n",
                           i, c, oz, oy, ox,
                           pd, py, px,
                           poss[0], poss[1], poss[2], poss[3], poss[4],
                           input_offset,
                           input[input_offset],
                           max_value);
#endif

                  }
                }
              }
            }

            int output_poss[5] = {i, c, oz, oy, ox};
            int output_offset = get_tensor5d_offset(output_poss, output_strides);
            output[output_offset] = max_value;

#ifdef DUMP_MSG
            printf("    output(%d, %d, %d, %d, %d)(offset=%d)=%f\n",
                  output_poss[0], output_poss[1], output_poss[2],
                  output_poss[3], output_poss[4],
                  output_offset, max_value);
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

// Input
// global memory (n, ic, id, ih, iw)
// local memory  (n*id, ic, ih, iw), align eu
static void loadInput(
    cvk_context_t *cvk_ctx,
    int n, int ic, int id, int ih, int iw,
    uint64_t ga_input, cvk_tl_t *tl_input_al)
{
  assert(n == 1 && "Only handle batch 1");
  int eu_num = (int)cvk_ctx->info.eu_num;

  // local memory layout (id, ic, ih, iw)
  cvk_fmt_t fmt = tl_input_al->fmt;
  cvk_tl_shape_t tl_shape = {1, (uint32_t)ic, (uint32_t)id, (uint32_t)ih*iw};
  cvk_tl_t tl_input;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_shape, fmt,
                                 tl_input_al->eu_align);
  tl_input.start_address = tl_input_al->start_address;
  tl_input.stride.h = ((tl_input.stride.h + eu_num - 1) / eu_num) * eu_num;
  tl_input.stride.n = tl_input.stride.h * id;

  // global memory (1, ic, id, ih*iw)
  cvk_tg_shape_t tg_shape = {1, (uint32_t)ic, (uint32_t)id, (uint32_t)ih*iw};
  cvk_tg_t gm_input;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &gm_input, tg_shape, fmt);
  gm_input.start_address = ga_input;

  cvk_tdma_g2l_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &gm_input;
  param.dst = &tl_input;

#ifdef DUMP_MSG
  printf("  loadInput:\n" \
         "    src %" PRIu64 ", (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 "), stride(%" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 ", %" PRIu32 ")\n" \
         "    dst %" PRIu32 ", (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 "), stride(%" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 ", %" PRIu32 ")\n",
         gm_input.start_address,
         gm_input.shape.n, gm_input.shape.c, gm_input.shape.h, gm_input.shape.w,
         gm_input.stride.n, gm_input.stride.c, gm_input.stride.h,
         gm_input.stride.w,
         tl_input.start_address,
         tl_input.shape.n, tl_input.shape.c, tl_input.shape.h, tl_input.shape.w,
         tl_input.stride.n, tl_input.stride.c, tl_input.stride.h,
         tl_input.stride.w);
#endif

  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy(cvk_ctx, &param);
}

static void compute(
    cvk_context_t *cvk_ctx,
    int n, int ic, int id, int ih, int iw,
    int od, int oh, int ow,
    int pad_d0, int pad_d1,
    int pad_top, int pad_bot, int pad_left, int pad_right,
    int kd, int kh, int kw,
    int stride_d, int stride_h, int stride_w,
    cvk_tl_t *tl_input_al, cvk_tl_t *tl_work, cvk_tl_t *tl_output_al)
{
  (void)pad_d1;
  (void)pad_bot;
  (void)pad_right;
  assert(n == 1 && "Only support batch 1");

  cvk_fmt_t fmt = tl_input_al->fmt;

  // Apply 2d max pool to each input depth
  {
    cvk_tl_shape_t input_shape = {
        (uint32_t)id, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw};
    cvk_tl_t tl_input;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, input_shape, fmt,
                                  /*eu_align=*/1);
    tl_input.start_address = tl_input_al->start_address;

    cvk_tl_shape_t output_shape = {
      (uint32_t)id, (uint32_t)ic, (uint32_t)oh, (uint32_t)ow};
    cvk_tl_t tl_output;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, output_shape, fmt,
                                   /*eu_align=*/1);
    tl_output.start_address = tl_output_al->start_address;

    cvk_tiu_max_pooling_param_t param;
    memset(&param, 0, sizeof(param));
    param.ofmap = &tl_output;
    param.ifmap = &tl_input;
    param.kh = kh;
    param.kw = kw;
    param.pad_top = (uint8_t)pad_top;
    param.pad_bottom = (uint8_t)pad_bot;
    param.pad_left = (uint8_t)pad_left;
    param.pad_right = (uint8_t)pad_right;
    param.stride_h = (uint8_t)stride_h;
    param.stride_w = (uint8_t)stride_w;

#ifdef DUMP_MSG
    printf("  compute, max pool:\n" \
           "    ifmap %" PRIu32 ", shape(%" PRIu32 ", %" PRIu32 ", "\
           "%" PRIu32 ", %" PRIu32 "), stride (%" PRIu32 ", %" PRIu32 ", "\
           "%" PRIu32 ", %" PRIu32 ")\n" \
           "    ofmap %" PRIu32 ", shape(%" PRIu32 ", %" PRIu32 ", "\
           "%" PRIu32 ", %" PRIu32 "), stride (%" PRIu32 ", %" PRIu32 ", "\
           "%" PRIu32 ", %" PRIu32 ")\n",
           tl_input.start_address,
           tl_input.shape.n, tl_input.shape.c, tl_input.shape.h,
           tl_input.shape.w,
           tl_input.stride.n, tl_input.stride.c, tl_input.stride.h,
           tl_input.stride.w,
           tl_output.start_address,
           tl_output.shape.n, tl_output.shape.c, tl_output.shape.h,
           tl_output.shape.w,
           tl_output.stride.n, tl_output.stride.c, tl_output.stride.h,
           tl_output.stride.w);
#endif

    cvk_ctx->ops->tiu_max_pooling(cvk_ctx, &param);
  }

  // TIU copy (od, ic, oh, ow) -> (1, ic, od, oh*ow)
  // from eu-aligned to contiguous
  {
    cvk_tl_shape_t tl_input_shape = {
        1, (uint32_t)ic, (uint32_t)oh, (uint32_t)ow};

    cvk_tl_stride_t aligned_stride =
        cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_input_shape, fmt, 1);
    cvk_tl_stride_t unaligned_stride =
        cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_input_shape, fmt, 0);

    for (int oz = 0; oz < od; ++oz) {
      for (int pd = 0; pd < kd; pd++) {
        int iz = oz * stride_d + pd - pad_d0;

        cvk_tl_t tl_input;
        cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_input_shape, fmt,
                                       /*eu_align=*/1);
        tl_input.start_address = tl_output_al->start_address +
                                 aligned_stride.n * iz;

        cvk_tl_shape_t tl_output_shape = {
            1, (uint32_t)ic, (uint32_t)oh, (uint32_t)ow};
        cvk_tl_t tl_output;
        cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, tl_output_shape,
                                       fmt, /*eu_align=*/0);
        tl_output.start_address = tl_work->start_address +
                                  aligned_stride.n * oz * kd +
                                  unaligned_stride.n * pd;

        cvk_tiu_copy_param_t param;
        memset(&param, 0, sizeof(param));
        param.src = &tl_input;
        param.dst = &tl_output;

#ifdef DUMP_MSG
        printf("    [oz=%d][pd=%d] tiu_copy:\n" \
              "       src %" PRIu32 ", (%" PRIu32 ", %" PRIu32 ", "\
              "%" PRIu32 ", %" PRIu32 "), stride(%" PRIu32 ", "\
              "%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n" \
              "       dst %" PRIu32 ", (%" PRIu32 ", %" PRIu32 ", "\
              "%" PRIu32 ", %" PRIu32 "), stride(%" PRIu32 ", "\
              "%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
              oz, pd,
              tl_input.start_address,
              tl_input.shape.n, tl_input.shape.c, tl_input.shape.h,
              tl_input.shape.w,
              tl_input.stride.n, tl_input.stride.c, tl_input.stride.h,
              tl_input.stride.w,
              tl_output.start_address,
              tl_output.shape.n, tl_output.shape.c, tl_output.shape.h,
              tl_output.shape.w,
              tl_output.stride.n, tl_output.stride.c, tl_output.stride.h,
              tl_output.stride.w);
#endif

        cvk_ctx->ops->tiu_copy(cvk_ctx, &param);
      }
    }
  }

  // Apply 2d max pool to input depth
  // input (od, ic, kd, oh*ow)
  // kernel (id, 1)
  // output (od, ic, 1, oh*ow)
  {
    cvk_tl_shape_t tiu_copy_input_shape = {
        1, (uint32_t)ic, (uint32_t)oh, (uint32_t)ow};

    cvk_tl_stride_t tiu_copy_aligned_stride =
        cvk_ctx->ops->tl_default_stride(cvk_ctx, tiu_copy_input_shape, fmt, 1);

    for (int oz = 0; oz < od; ++oz) {
      cvk_tl_shape_t tl_input_shape = {
          1, (uint32_t)ic, (uint32_t)kd, (uint32_t)oh*ow};
      cvk_tl_t tl_input;
      cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_input, tl_input_shape, fmt,
                                     /*eu_align=*/1);
      tl_input.start_address = tl_work->start_address +
                               tiu_copy_aligned_stride.n * oz * kd;

      cvk_tl_shape_t tl_output_shape = {
          1, (uint32_t)ic, 1, (uint32_t)oh*ow};
      cvk_tl_t tl_output;
      cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, tl_output_shape, fmt,
                                     /*eu_align=*/1);
      tl_output.start_address = tl_output_al->start_address +
                                tl_output.stride.n * oz;

      cvk_tiu_max_pooling_param_t param;
      memset(&param, 0, sizeof(param));
      param.ofmap = &tl_output;
      param.ifmap = &tl_input;
      param.kh = kd;
      param.kw = 1;
      param.pad_top = 0;
      param.pad_bottom = 0;
      param.pad_left = 0;
      param.pad_right = 0;
      param.stride_h = 1;
      param.stride_w = 1;

#ifdef DUMP_MSG
      printf("    [%d] max pool:\n" \
            "      ifmap %" PRIu32 ", shape(%" PRIu32 ", %" PRIu32 ", "\
            "%" PRIu32 ", %" PRIu32 "), stride (%" PRIu32 ", %" PRIu32 ", "\
            "%" PRIu32 ", %" PRIu32 ")\n" \
            "      ofmap %" PRIu32 ", shape(%" PRIu32 ", %" PRIu32 ", "\
            "%" PRIu32 ", %" PRIu32 "), stride (%" PRIu32 ", %" PRIu32 ", "\
            "%" PRIu32 ", %" PRIu32 ")\n",
            oz,
            tl_input.start_address,
            tl_input.shape.n, tl_input.shape.c, tl_input.shape.h,
            tl_input.shape.w,
            tl_input.stride.n, tl_input.stride.c, tl_input.stride.h,
            tl_input.stride.w,
            tl_output.start_address,
            tl_output.shape.n, tl_output.shape.c, tl_output.shape.h,
            tl_output.shape.w,
            tl_output.stride.n, tl_output.stride.c, tl_output.stride.h,
            tl_output.stride.w);
#endif

      cvk_ctx->ops->tiu_max_pooling(cvk_ctx, &param);
    }

  }
}

static void storeOutput(
    cvk_context_t *cvk_ctx,
    int n, int ic, int od, int oh, int ow,
    uint64_t ga_output, cvk_tl_t *tl_output_al) {

  assert(n == 1 && "Only support batch 1");
  int eu_num = (int)cvk_ctx->info.eu_num;

  cvk_fmt_t fmt = tl_output_al->fmt;
  cvk_tl_shape_t tl_output_shape = {
      1, (uint32_t)ic, (uint32_t)od, (uint32_t)oh*ow};
  cvk_tl_t tl_output;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_output, tl_output_shape, fmt,
                                 /*eu_align=*/1);
  tl_output.start_address = tl_output_al->start_address;
  tl_output.stride.h = ((tl_output.stride.h + eu_num - 1) / eu_num) * eu_num;

  cvk_tg_shape_t tg_output_shape = {
      1, (uint32_t)ic, (uint32_t)od, (uint32_t)oh*ow};
  cvk_tg_t tg_output;
  cvk_ctx->ops->gmem_init_tensor(cvk_ctx, &tg_output, tg_output_shape, fmt);
  tg_output.start_address = ga_output;

  cvk_tdma_l2g_tensor_copy_param_t param;
  memset(&param, 0, sizeof(param));
  param.src = &tl_output;
  param.dst = &tg_output;

#ifdef DUMP_MSG
  printf("  storeOutput:\n" \
         "    src %" PRIu32 ", (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 "), stride(%" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 ", %" PRIu32 ")\n" \
         "    dst %" PRIu64 ", (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 "), stride(%" PRIu32 ", %" PRIu32 ", "\
         "%" PRIu32 ", %" PRIu32 ")\n",
         tl_output.start_address,
         tl_output.shape.n, tl_output.shape.c, tl_output.shape.h,
         tl_output.shape.w,
         tl_output.stride.n, tl_output.stride.c, tl_output.stride.h,
         tl_output.stride.w,
         tg_output.start_address,
         tg_output.shape.n, tg_output.shape.c, tg_output.shape.h,
         tg_output.shape.w,
         tg_output.stride.n, tg_output.stride.c, tg_output.stride.h,
         tg_output.stride.w);
#endif

  cvk_ctx->ops->tdma_l2g_bf16_tensor_copy(cvk_ctx, &param);
}

//
//        N  IC  ID  IH  IW
// input (1,  2,  4,  6,  6)
//
//        KD  KH  KW
// kernel (2,  2,  2), stride (2, 2, 2)
//
//         N  OC  OD  OH  OW
// output (1,  2,  2,  3,  3)
//
// pytorch:
//   import torch
//   import torch.nn as nn
//   m = nn.MaxPool3d(kernel_size=(2, 2, 2), stride=(2, 2, 2))
//   input = torch.rand(1, 2, 4, 6, 6)
//   output = m(input)
//
static int pool3d_test(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
#ifdef DUMP_MSG
  printf("===================================\n\n");
  printf("%s =>\n", __FUNCTION__);
#endif

  (void)rt_handle;
  (void)cvk_ctx;

  int ret = 0;

  // input (n=1, c=2, d=4, h=6, w=6)
  int n = 1, ic = 2, id = 4, ih = 6, iw = 6;
  //int input_shapes[5] = {n, ic, id, ih, iw};
  float input_data[] = {
        // IC=0
        0.6907, 0.9408, 0.5792, 0.0155, 0.3657, 0.3555, // ic=0, id=0, ih=0
        0.0933, 0.1023, 0.1212, 0.6805, 0.7696, 0.9585,
        0.9876, 0.8971, 0.9734, 0.7682, 0.1532, 0.7841,
        0.5079, 0.0060, 0.6073, 0.7310, 0.2100, 0.6355,
        0.6528, 0.4982, 0.6783, 0.9851, 0.5486, 0.4885,
        0.9682, 0.8595, 0.0752, 0.1718, 0.4142, 0.3268,

        0.5709, 0.1998, 0.0288, 0.7744, 0.8626, 0.9731, // ic=0, id=1, ih=0
        0.7898, 0.9141, 0.5941, 0.2131, 0.4342, 0.4791,
        0.5463, 0.5508, 0.7898, 0.6795, 0.2314, 0.4887,
        0.2611, 0.0303, 0.1486, 0.1386, 0.7260, 0.7107,
        0.2266, 0.4536, 0.0832, 0.7075, 0.4564, 0.3716,
        0.1076, 0.3894, 0.2450, 0.7306, 0.5887, 0.9756,

        0.1618, 0.7542, 0.9378, 0.3864, 0.5607, 0.7753, // ic=0, id=2, ih=0
        0.1143, 0.7555, 0.9824, 0.4149, 0.4235, 0.4298,
        0.8252, 0.7352, 0.7124, 0.3989, 0.1546, 0.8114,
        0.9839, 0.5569, 0.5464, 0.3193, 0.7710, 0.5774,
        0.9209, 0.2261, 0.9562, 0.0164, 0.4955, 0.6177,
        0.5790, 0.1055, 0.2765, 0.5313, 0.9878, 0.4712,

        0.1046, 0.4042, 0.8381, 0.2987, 0.8166, 0.0809, // ic=0, id=3, ih=0
        0.1088, 0.7503, 0.1620, 0.0757, 0.0527, 0.4848,
        0.6198, 0.3235, 0.7783, 0.1561, 0.4574, 0.3427,
        0.3435, 0.2180, 0.8079, 0.2090, 0.8582, 0.0381,
        0.6562, 0.7126, 0.5397, 0.5042, 0.0537, 0.7438,
        0.4731, 0.8265, 0.6840, 0.2583, 0.2385, 0.9646,

        // IC=1
        0.4381, 0.0173, 0.8579, 0.0612, 0.1047, 0.3874, // ic=1, id=0, ih=0
        0.6478, 0.1154, 0.2030, 0.8241, 0.8321, 0.3291,
        0.1108, 0.0253, 0.1421, 0.8958, 0.4012, 0.7565,
        0.7778, 0.5838, 0.3576, 0.3304, 0.8876, 0.8442,
        0.6297, 0.9765, 0.5933, 0.0829, 0.3601, 0.1416,
        0.6709, 0.5021, 0.3242, 0.0080, 0.6258, 0.3447,

        0.8346, 0.7683, 0.5717, 0.1758, 0.7923, 0.1612, // ic=1, id=1, ih=0
        0.2289, 0.5588, 0.2911, 0.7938, 0.9405, 0.6662,
        0.0654, 0.1096, 0.9990, 0.6394, 0.6518, 0.0435,
        0.8786, 0.6171, 0.6420, 0.8809, 0.8736, 0.6752,
        0.2551, 0.6027, 0.6452, 0.9325, 0.4888, 0.3601,
        0.3799, 0.9947, 0.1809, 0.9057, 0.5961, 0.3890,

        0.1345, 0.0214, 0.9583, 0.3609, 0.8325, 0.9346, // ic=1, id=2, ih=0
        0.5194, 0.0346, 0.4887, 0.3495, 0.6093, 0.1503,
        0.9276, 0.0147, 0.7693, 0.2042, 0.0840, 0.0885,
        0.5881, 0.0846, 0.4413, 0.1325, 0.2781, 0.6833,
        0.1373, 0.1428, 0.4761, 0.3221, 0.6378, 0.2035,
        0.1556, 0.6974, 0.0709, 0.1990, 0.5579, 0.5456,

        0.4670, 0.7720, 0.8733, 0.6276, 0.4938, 0.8457, // ic=1, id=3, ih=0
        0.2115, 0.0147, 0.0665, 0.3245, 0.9537, 0.6126,
        0.9876, 0.5670, 0.8435, 0.2570, 0.8289, 0.4260,
        0.2659, 0.3500, 0.6909, 0.2132, 0.8072, 0.6591,
        0.0285, 0.8939, 0.1296, 0.2531, 0.1313, 0.8860,
        0.5761, 0.5247, 0.6818, 0.0679, 0.9456, 0.3174
  };

  // weight (2, 2, 2), stride (2, 2, 2)
  int kd = 2, kh = 2, kw = 2;
  int stride_d = 2, stride_h = 2, stride_w = 2;

  // output (n=1, oc=2, od=2, oh=3, ow=3)
  int pad_d0=0, pad_d1=0, pad_top=0, pad_bot=0, pad_left=0, pad_right=0;
  int od = 2, oh = 3, ow = 3;
  float ref_output_data[] = {
        // oc=0
        0.9408, 0.7744, 0.9731, // oc=0, od=0, oh=0
        0.9876, 0.9734, 0.7841,
        0.9682, 0.9851, 0.9756,

        0.7555, 0.9824, 0.8166, // oc=0, od=1, oh=0
        0.9839, 0.8079, 0.8582,
        0.9209, 0.9562, 0.9878,

        // oc=1
        0.8346, 0.8579, 0.9405, // oc=1, od=0, oh=0
        0.8786, 0.9990, 0.8876,
        0.9947, 0.9325, 0.6258,

        0.7720, 0.9583, 0.9537, // oc=1, od=1, oh=0
        0.9876, 0.8435, 0.8289,
        0.8939, 0.6818, 0.9456
  };

  float output_data_cpu[sizeof(ref_output_data)/sizeof(float)];
  pool3d_float_ref(input_data, output_data_cpu,
                   n, ic, id, ih, iw,
                   od, oh, ow,
                   kd, kh, kw,
                   stride_d, stride_h, stride_w,
                   pad_d0, pad_d1,
                   pad_top, pad_bot, pad_left, pad_right);

  printf("  pool3d_test: compare ref\n");
  const float precision = 0.0002;
  for (size_t i = 0; i < sizeof(output_data_cpu)/sizeof(float); i++)
  {
    if (fabs(output_data_cpu[i] - ref_output_data[i]) > precision) {
      printf("    [%d] Error ! val %f, expected %f\n",
             (int)i, output_data_cpu[i], ref_output_data[i]);
      ret = -1;
    }
  }
  printf("  pool3d_test: compare ref %s\n", ret ? "fail" : "pass");

  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tl_t *tl_input_al = NULL;
  cvk_tl_t *tl_output_al = NULL;
  cvk_tl_t *tl_work_al = NULL;

  // Allocate input (n*id, ic, ih, iw)
  // treat input depth as batch, do 2d max pool for each input depth
  // align EU for tiu max pool.
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n*id, (uint32_t)ic, (uint32_t)ih, (uint32_t)iw};
    tl_input_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                  /*eu_align=*/1);
  }

  // Allocate work (n=od*kd, ic, oh, ow)
  {
    cvk_tl_shape_t shape = {
        (uint32_t)od*kd, (uint32_t)ic, (uint32_t)oh, (uint32_t)ow};
    tl_work_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                 /*eu_align=*/1);
  }

  // Allocate output (n=1, ic, od, oh*ow)
  {
    cvk_tl_shape_t shape = {
        (uint32_t)n, (uint32_t)ic, (uint32_t)od, (uint32_t)oh*ow};
    tl_output_al = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, shape, fmt,
                                                   /*eu_align=*/0);
  }

  CVI_RT_MEM gm_input_dev_mem = NULL;
  CVI_RT_MEM gm_output_dev_mem = NULL;
  uint64_t ga_input = 0;
  uint64_t ga_output = 0;

  // Allocate device memory of input
  {
    // shape (1, ic=2, id=4, ih=6, iw=6)
    int total_len = 1 * ic * id * ih * iw;
    uint16_t input_bf16_data[total_len];
    convert_fp32_to_bf16_data(cvk_ctx, input_bf16_data, input_data, total_len);

    gm_input_dev_mem = CVI_RT_MemAlloc(rt_handle, total_len * sizeof(uint16_t));

    // Copy from system memory to device memory
    CVI_RT_MemCopyS2D(rt_handle, gm_input_dev_mem, (uint8_t *)input_bf16_data);

    ga_input = CVI_RT_MemGetPAddr(gm_input_dev_mem);
  }

  // Allocate device memory of output
  {
    int len = n * ic * od * oh * ow;
    gm_output_dev_mem = CVI_RT_MemAlloc(rt_handle, len * sizeof(uint16_t));
    ga_output = CVI_RT_MemGetPAddr(gm_output_dev_mem);
  }

  assert(gm_input_dev_mem && gm_output_dev_mem && "Expect valid gm dev mem");
  assert(ga_input && ga_output && "Expect valid gaddr");

  // load input
  loadInput(cvk_ctx,
            n, ic, id, ih, iw,
            ga_input, tl_input_al);

  // 3d max pool
  compute(cvk_ctx,
          n, ic, id, ih, iw,
          od, oh, ow,
          pad_d0, pad_d1,
          pad_top, pad_bot, pad_left, pad_right,
          kd, kh, kw,
          stride_d, stride_h, stride_w,
          tl_input_al, tl_work_al, tl_output_al);

  // store output
  storeOutput(cvk_ctx,
              n, ic, od, oh, ow,
              ga_output, tl_output_al);

  CVI_RT_Submit(cvk_ctx);

  // copy from device memory to system memory
  int output_len = n * ic * od * oh * ow;

  uint16_t ref_output_bf16_data[output_len];
  convert_fp32_to_bf16_data(cvk_ctx, ref_output_bf16_data, ref_output_data,
                            output_len);

  uint16_t output_bf16_data_tpu[output_len];
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) output_bf16_data_tpu,
                    gm_output_dev_mem);

  printf("  pool3d_test: compare tpu\n");
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
  printf("  pool3d_test: compare tpu %s\n", ret ? "fail" : "pass");

  // Reverse order
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_output_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_work_al);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_input_al);

  CVI_RT_MemFree(rt_handle, gm_input_dev_mem);
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

  ret = pool3d_test(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  if (!ret)
    printf("%s pass\n", __FILENAME__);
  else
    printf("%s fail\n", __FILENAME__);

  return ret;
}
