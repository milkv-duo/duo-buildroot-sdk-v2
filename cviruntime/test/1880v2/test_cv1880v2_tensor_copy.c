#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef enum {
  NCHW_N = 0,
  NCHW_C = 1,
  NCHW_H = 2,
  NCHW_W = 3,
  NCHW_MAX_DIMS
} NCHW_DIMS;


void gmem_init_tensor(
    struct cvikernel_context *cvk_ctx,
    cvk_tg_t *tg,
    cvk_tg_shape_t shape,
    cvk_fmt_t fmt) {

  memset(tg, 0, sizeof(*tg));
  tg->fmt = fmt;
  tg->shape = shape;
  tg->stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, tg->shape, tg->fmt);
}

static void tl_copy_ref(int8_t *a, int8_t *res, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    res[i] = a[i];
}

static int test_tl_copy(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;

  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h * w;
  int8_t *a_data = (int8_t *)malloc(size);
  assert(a_data && "Expect allocated a_data");
  for (uint32_t i = 0; i < size; i++)
    a_data[i] = (int8_t)(i % 256);

  int8_t *ref_data = (int8_t *)malloc(size);
  assert(ref_data && "Expect allocated ref_data");
  tl_copy_ref(a_data, ref_data, size);

  cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);

  cvk_tiu_copy_param_t p10;
  p10.dst = tl_res;
  p10.src = tl_a;
  cvk_ctx->ops->tiu_copy(cvk_ctx, &p10);
  uint8_t *res_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res);

  for (uint64_t i = 0; i < size; i++) {
    if ((int8_t)res_data[i] != ref_data[i]) {
      printf("    comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      ret = -1;
    }
  }

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

  free(a_data);
  free(ref_data);
  free(res_data);

  return ret;
}

static int test_hw_tp(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  int8_t data[6] = {1, 2, 3, 4, 5, 6};
  int8_t ref_data_hw_tp[6] = {1, 4, 2, 5, 3, 6};

  cvk_tl_shape_t src_shape = {1, 1, 2, 3};
  cvk_tl_shape_t dst_shape = {1, 1, 3, 2};

  int eu_align = 0; // contiguous memory layout
  uint32_t offset = 0;
  cvk_tl_t tl_src;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_src, src_shape, CVK_FMT_I8,
                                 eu_align);
  offset += cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, src_shape, CVK_FMT_I8,
                                              eu_align);

  // HW transpose, still use source shape for data transfer
  cvk_tl_t tl_dst;
  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_dst, src_shape, CVK_FMT_I8,
                                 eu_align);
  tl_dst.start_address = offset;
  tl_dst.stride.h = tl_dst.stride.w; // unit of data type size (int8/bf16)
  tl_dst.stride.w = dst_shape.w * tl_dst.stride.w;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, &tl_src, (uint8_t *)data);

  cvk_tiu_copy_param_t param;
  param.src = &tl_src;
  param.dst = &tl_dst;
  cvk_ctx->ops->tiu_copy(cvk_ctx, &param);

  CVI_RT_Submit(cvk_ctx);

  cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_dst, dst_shape, CVK_FMT_I8,
                                 eu_align);
  tl_dst.start_address = offset;

  uint8_t *res_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, &tl_dst);

  printf("  test_hw_tp: compare\n");
  for (uint64_t i = 0; i < sizeof(ref_data_hw_tp); i++) {
    if (res_data[i] != ref_data_hw_tp[i]) {
      printf("    res_data[%" PRIu64 "]  %d != %d\n",
             i, res_data[i], ref_data_hw_tp[i]);
      ret = -1;
    }
  }

  free(res_data);

  return ret;
}

static int test_tp_0213(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  const uint32_t input_n = 1;
  const uint32_t input_c = 3;
  const uint32_t input_h = 2;
  const uint32_t input_w = 4;
  // const uint32_t output_n = 1;
  // const uint32_t output_c = 3;
  // const uint32_t output_h = 2;
  // const uint32_t output_w = 4;
  uint32_t order[4] = {0, 2, 1, 3};

  uint32_t src_shape[4] = {input_n, input_c, input_h, input_w};
  uint32_t dst_shape[4] = {src_shape[order[0]], src_shape[order[1]],
                           src_shape[order[2]],  src_shape[order[3]]};

  // Shape (1, 3, 2, 4) -> (1, 2, 3, 4)
  int8_t data[] = {
      //    H0     |        H1
      1,  2,  3 , 4,  5,  6,  7,  8,   // C0
      9, 10, 11, 12, 13, 14, 15, 16,   // C1
     17, 18, 19, 20, 21, 22, 23, 24    // C2
  };
  int8_t ref_dst_data[] = {
      //    H0     |        H1     |       H2
      1,  2,  3,  4,  9, 10, 11, 12, 17, 18, 19, 20,  // C0
      5,  6,  7,  8, 13, 14, 15, 16, 21, 22, 23, 24   // C1
  };

  int eu_align = 0; // contiguous memory layout
  cvk_fmt_t fmt = CVK_FMT_I8;

  // Alloc global memory
  cvk_tg_shape_t tg_src_shape = {
      src_shape[0], src_shape[1], src_shape[2], src_shape[3]};
  cvk_tg_t *tg_mem_1 =
      alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_src_shape, fmt);
  if (!tg_mem_1)
    return -1;

  uint64_t ga_ifmap = tg_mem_1->start_address;
  uint32_t tg_src_stride[3] = {
      tg_mem_1->stride.n, tg_mem_1->stride.c, tg_mem_1->stride.h};

  cvk_tg_shape_t tg_dst_shape = {
      dst_shape[0], dst_shape[1], dst_shape[2], dst_shape[3]};
  cvk_tg_t *tg_mem_2 =
      alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_dst_shape, fmt);
  uint32_t tg_dst_strides[3] = {
      tg_mem_2->stride.n, tg_mem_2->stride.c, tg_mem_2->stride.h};
  uint64_t ga_ofmap = tg_mem_2->start_address;

  // test stride
  {
    int8_t test_dst_data[1 * 3 * 2 * 4];
    uint32_t dst_strides[3] = {
        tg_dst_strides[order[0]], tg_dst_strides[order[1]],
        tg_dst_strides[order[2]]};

    for (uint32_t i = 0; i < input_n; i++) {
      for (uint32_t j = 0; j < input_c; j++) {
        for (uint32_t k = 0; k < input_h; k++) {
          for (uint32_t l = 0; l < input_w; l++) {
              uint32_t src_offset = i * tg_src_stride[0] + j * tg_src_stride[1]
                                  + k * tg_src_stride[2] + l;
              uint32_t dst_offset = i * dst_strides[0] + j * dst_strides[1] +
                                    k * dst_strides[2] + l;
            test_dst_data[dst_offset] = data[src_offset];
          }
        }
      }
    }

    printf("  test_tp_0213: compare test\n");
    for (uint32_t i = 0; i < sizeof(ref_dst_data); i++) {
      if (test_dst_data[i] != ref_dst_data[i])
        printf("    [%d] test_dst_data %d != %d\n",
               i, test_dst_data[i], ref_dst_data[i]);
    }

  }

  // Fill data in global memory
  tensor_copy_s2d(rt_handle, tg_mem_1, (uint8_t *)data);

  // 1. tensor load
  {
    cvk_tg_t tg_src;
    memset(&tg_src, 0, sizeof(tg_src));
    tg_src.base_reg_index = 0;
    tg_src.start_address = ga_ifmap;
    tg_src.fmt = fmt;
    tg_src.shape.n = src_shape[0];
    tg_src.shape.c = src_shape[1];
    tg_src.shape.h = src_shape[2];
    tg_src.shape.w = src_shape[3];
    tg_src.stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, tg_src.shape, fmt);

    cvk_tl_shape_t tl_dst_shape = {
        src_shape[0], src_shape[1], src_shape[2], src_shape[3]};
    cvk_tl_t tl_dst;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_dst, tl_dst_shape, fmt,
                                   eu_align);

    cvk_tdma_g2l_tensor_copy_param_t param;
    param.src = &tg_src;
    param.dst = &tl_dst;
    cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &param);
  }

  // 2. tensor store w/ (1, 3, 2, 4) transpose
  {
    cvk_tl_shape_t tl_src_shape = {
        src_shape[0], src_shape[1], src_shape[2], src_shape[3]};

    cvk_tl_t tl_src;
    cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_src, tl_src_shape, fmt,
                                   eu_align);

    cvk_tg_t tg_dst;
    memset(&tg_dst, 0, sizeof(tg_dst));
    tg_dst.base_reg_index = 0;
    tg_dst.start_address = ga_ofmap;
    tg_dst.fmt = fmt;
    tg_dst.shape.n = src_shape[0];
    tg_dst.shape.c = src_shape[1];
    tg_dst.shape.h = src_shape[2];
    tg_dst.shape.w = src_shape[3];
    tg_dst.stride.n = tg_dst_strides[order[0]];
    tg_dst.stride.c = tg_dst_strides[order[1]];
    tg_dst.stride.h = tg_dst_strides[order[2]];

    cvk_tdma_l2g_tensor_copy_param_t param;
    param.src = &tl_src;
    param.dst = &tg_dst;
    cvk_ctx->ops->tdma_l2g_tensor_copy(cvk_ctx, &param);
  }

  CVI_RT_Submit(cvk_ctx);

  int8_t *dst_data = (int8_t *)tensor_copy_d2s(rt_handle, tg_mem_2);

  printf("  test_tp_0213: compare\n");
  for (uint64_t i = 0; i < sizeof(ref_dst_data); i++) {
    if (dst_data[i] != ref_dst_data[i]) {
      printf("    dst_data[%" PRIu64 "]  %d != %d\n",
             i, dst_data[i], ref_dst_data[i]);
      ret = -1;
    }
  }

  // Free global memory
  free(dst_data);
  free_tensor_dev_mem(rt_handle, tg_mem_1);
  free_tensor_dev_mem(rt_handle, tg_mem_2);

  return ret;
}

//
//  Permute 0231, (N, C, H, W) -> (N, H, W, C)
//    tensor load
//    tensor move, hw transpose
//    tensor store, cw transpose
//
//  0  1  2  3
// (N, C, H, W) -> (N, H, W, C)
// (1, 2, 4, 4) -> (1, 4, 4, 2)
//
// Source (1, 2, 4, 4)
//
//             Tile 1           ||          Tile 0
//         H3           H2      ||      H1           H0
// || 16 15 14 13 | 12 11 10  9 ||  8  7  6  5 |  4  3  2  1 ||   C0
// || 32 31 30 29 | 28 27 26 25 || 24 23 22 21 | 20 19 18 17 ||   C1
//
//
// Destination (1, 4, 4, 2)
//
//  20  4 | 19  3 | 18  2 | 17  1    C0    Tile 0
//  24  8 | 23  7 | 22  6 | 21  5    C1
//  ==============================================
//  28 12 | 27 11 | 26 10 | 25  9    C2    Tile 1
//  32 16 | 31 15 | 30 14 | 29 13    C3
//
// 1. Tile 0
// 1.1. Tensor load
//    src shape (1, 2, 2, 4), stride (32, 16, 4), offset 0
//    dst shape (1, 2, 2, 4), stride (8, 8, 4)
//
//         H1            H0
//     8  7  6  5 |  4  3  2  1    C0
//    24 23 22 21 | 20 19 18 17    C1
//
// 1.2. Tensor move, HW transpose
//    src shape (1, 2, 2, 4), stride (8, 8, 4, 1)
//    dst shape (1, 2, 2, 4), stride (8, 8, 1, 2)
//
//      H3     H2      H1      H0
//     8  4 | 7  3 |  6  2 |  5  1    C0
//    24 20 |23 19 | 22 18 | 21 17    C1
//
// 1.3. Tensor store, CW transpose
//    src shape (1, 2, 4, 2), stride (8, 8, 2)
//    dst shape (1, 2, 4, 2), stride (16, 8, 2), offset 0
//
//    H3      H2      H1      H0
//  20  4 | 19  3 | 18  2 | 17  1    C0
//  24  8 | 23  7 | 22  6 | 21  5    C1
//
//
// 2. Tile 1
// 2.1. Tensor load
//    src shape (1, 2, 2, 4), stride (32, 16, 4), offset 8
//    dst shape (1, 2, 2, 4), stride (8, 8, 4)
//
//         H1            H0
//    16 15 14 13 | 12 11 10  9    C0
//    32 31 30 29 | 28 27 26 25    C1
//
// 2.2. Tensor move, HW transpose
//    src shape (1, 2, 2, 4), stride (8, 8, 4, 1)
//    dst shape (1, 2, 2, 4), stride (8, 8, 1, 2)
//
//      H3      H2      H1      H0
//    16 12 | 15 11 | 14 10 | 13  9    C0
//    32 28 | 31 27 | 30 26 | 29 25    C1
//
// 2.3. Tensor store, CW transpose
//    src shape (1, 2, 4, 2), stride (8, 8, 2)
//    dst shape (1, 2, 4, 2), stride (16, 8, 2), offset 16
//
//      H3      H2      H1      H0
//    28 12 | 27 11 | 26 10 | 25  9    C0
//    32 16 | 31 15 | 30 14 | 29 13    C1
//
//    destination in global memory
//    shape (1, 4, 4, 2), stride (32, 8, 2)
//    gm_permuted_strides[order_n 0] = dst_gm_stride.n 32
//    gm_permuted_strides[order_c 2] = dst_gm_stride.c 8
//    gm_permuted_strides[order_h 3] = dst_gm_stride.h 2
//
//    tile1 1
//    source in global memory, offset [0][0][2][0], 9
//      src_gm_offset = 2 * h_stride = 2 * 4 = 8, used in first load
//
//    destination in global memory, offset [0][2][0][0], 9
//    dst_gm_offset = 2 * c_stride = 2 * 8 = 16
//
//    src[i][j][k][l] = dst[i][k][l][j]
//    src[0][0][2][0] = dst[0][2][0][0]
//
static int test_tp_0231(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  cvk_fmt_t fmt = CVK_FMT_I8;
  uint8_t eu_align = 0; // No need to align eu
  uint32_t orders[NCHW_MAX_DIMS] = {0, 2, 3, 1}; // NCHW -> NHWC

  uint32_t src_shapes[NCHW_MAX_DIMS] = {1, 2, 4, 4};
  uint32_t src_strides[NCHW_MAX_DIMS];
  src_strides[NCHW_W] = 1; // int8
  src_strides[NCHW_H] = src_shapes[NCHW_W] * src_strides[NCHW_W];
  src_strides[NCHW_C] = src_shapes[NCHW_H] * src_strides[NCHW_H];
  src_strides[NCHW_N] = src_shapes[NCHW_C] * src_strides[NCHW_C];

  uint32_t dst_shapes[NCHW_MAX_DIMS] = {
      src_shapes[orders[NCHW_N]], src_shapes[orders[NCHW_C]],
      src_shapes[orders[NCHW_H]], src_shapes[orders[NCHW_W]]};
  uint32_t dst_strides[NCHW_MAX_DIMS];
  dst_strides[NCHW_W] = 1; // int8
  dst_strides[NCHW_H] = dst_shapes[NCHW_W] * dst_strides[NCHW_W];
  dst_strides[NCHW_C] = dst_shapes[NCHW_H] * dst_strides[NCHW_H];
  dst_strides[NCHW_N] = dst_shapes[NCHW_C] * dst_strides[NCHW_C];

  // Source shape (1, 2, 4, 4)
  const int8_t src_data[] = {
      //     H0     |      H1       |        H2     |       H3
       1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, // C0
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32  // C1
  };

  // Destination shape (1, 4, 4, 2)
  const int8_t ref_dst_data[] = {
      // H0 |   H1  |   H2  |   H3
       1, 17,  2, 18,  3, 19,  4, 20, // C0
       5, 21,  6, 22,  7, 23,  8, 24, // C1
       9, 25, 10, 26, 11, 27, 12, 28, // C2
      13, 29, 14, 30, 15, 31, 16, 32  // C3
  };

  int8_t dst_data_cpu[sizeof(ref_dst_data)];

  // Derive destination offset from source position
  uint32_t dst_index[NCHW_MAX_DIMS];
  dst_index[orders[NCHW_N]] = 0;
  dst_index[orders[NCHW_C]] = 1;
  dst_index[orders[NCHW_H]] = 2;
  dst_index[orders[NCHW_W]] = 3;

  // test element-wise copy
  {
    // source is contiguous
    for (uint32_t i = 0; i < src_shapes[NCHW_N]; ++i) {
      for (uint32_t j = 0; j < src_shapes[NCHW_C]; ++j) {
        for (uint32_t k = 0; k < src_shapes[NCHW_H]; ++k) {
          for (uint32_t l = 0; l < src_shapes[NCHW_W]; ++l) {
            uint32_t src_offset =
                i * src_strides[NCHW_N] + j * src_strides[NCHW_C] +
                k * src_strides[NCHW_H] + l * src_strides[NCHW_W];
            uint32_t dst_offset = i * dst_strides[dst_index[NCHW_N]] +
                                  j * dst_strides[dst_index[NCHW_C]] +
                                  k * dst_strides[dst_index[NCHW_H]] +
                                  l * dst_strides[dst_index[NCHW_W]];
            dst_data_cpu[dst_offset] = src_data[src_offset];
          }
        }
      }
    }

    printf("  test_tp_0231: elt copy, compare test\n");
    for (uint32_t i = 0; i < sizeof(dst_data_cpu); ++i) {
      if (dst_data_cpu[i] != ref_dst_data[i]) {
        printf("    [%d] dst_data %d != %d\n",
               i, dst_data_cpu[i], ref_dst_data[i]);
        ret = -1;
      }
    }
  }

  //
  // Data initialization in runtime.
  //
  cvk_tg_shape_t tg_src_shape = {
      src_shapes[NCHW_N], src_shapes[NCHW_C], src_shapes[NCHW_H],
      src_shapes[NCHW_W]};
  cvk_tg_t *tg_src =
      alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_src_shape, fmt);

  // Fill data in global memory
  tensor_copy_s2d(rt_handle, tg_src, (uint8_t *)src_data);

  cvk_tg_shape_t tg_dst_shape = {
      dst_shapes[NCHW_N], dst_shapes[NCHW_C], dst_shapes[NCHW_H],
      dst_shapes[NCHW_W]};
  cvk_tg_t *tg_dst =
      alloc_tensor_dev_mem(rt_handle, cvk_ctx, tg_dst_shape, fmt);


  //
  // Main tiled transpose routine
  //
  uint32_t src_h_step = src_shapes[NCHW_H] / 2; // 2 tiles
  uint32_t src_poss[NCHW_MAX_DIMS] = {0, 0, 0, 0};
  for (src_poss[NCHW_H] = 0; src_poss[NCHW_H] < src_shapes[NCHW_H];
       src_poss[NCHW_H] += src_h_step) {
    uint32_t src_tiled_shapes[NCHW_MAX_DIMS] = {
        src_shapes[NCHW_N], src_shapes[NCHW_C], src_shapes[NCHW_H],
        src_shapes[NCHW_W]};
    src_tiled_shapes[NCHW_H] =
        ((src_poss[NCHW_H] + src_h_step) > src_shapes[NCHW_H]) ?
            (src_shapes[NCHW_H] - src_poss[NCHW_H]) : src_h_step;

    uint32_t src_offset =
        src_poss[NCHW_N] * src_strides[NCHW_N] +
        src_poss[NCHW_C] * src_strides[NCHW_C] +
        src_poss[NCHW_H] * src_strides[NCHW_H] +
        src_poss[NCHW_W] * src_strides[NCHW_W];
    uint32_t dst_offset = src_poss[NCHW_N] * dst_strides[dst_index[NCHW_N]] +
                          src_poss[NCHW_C] * dst_strides[dst_index[NCHW_C]] +
                          src_poss[NCHW_H] * dst_strides[dst_index[NCHW_H]] +
                          src_poss[NCHW_W] * dst_strides[dst_index[NCHW_W]];

    // 1. Tensor load, tiled shape, global stride
    cvk_tl_t *tl_load_dst_tiled = NULL;
    {
      cvk_tg_t tg_src_tiled;
      memset(&tg_src_tiled, 0, sizeof(tg_src_tiled));
      tg_src_tiled.base_reg_index = 0;
      tg_src_tiled.start_address = tg_src->start_address + src_offset;
      tg_src_tiled.fmt = fmt;
      tg_src_tiled.shape.n = src_tiled_shapes[NCHW_N];
      tg_src_tiled.shape.c = src_tiled_shapes[NCHW_C];
      tg_src_tiled.shape.h = src_tiled_shapes[NCHW_H];
      tg_src_tiled.shape.w = src_tiled_shapes[NCHW_W];
      tg_src_tiled.stride.n = tg_src->stride.n;
      tg_src_tiled.stride.c = tg_src->stride.c;
      tg_src_tiled.stride.h = tg_src->stride.h;

      cvk_tl_shape_t tl_dst_tiled_shape = {
          src_tiled_shapes[NCHW_N], src_tiled_shapes[NCHW_C],
          src_tiled_shapes[NCHW_H], src_tiled_shapes[NCHW_W]};
      tl_load_dst_tiled =
          cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_dst_tiled_shape, fmt,
                                          eu_align);

      cvk_tdma_g2l_tensor_copy_param_t param;
      param.src = &tg_src_tiled;
      param.dst = tl_load_dst_tiled;
      cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &param);
    }

    // 2. Tensor move, HW transpose
    cvk_tl_t *tl_move_dst = NULL;
    {
      cvk_tl_shape_t tl_move_dst_shape = {
          src_tiled_shapes[NCHW_N], src_tiled_shapes[NCHW_C],
          src_tiled_shapes[NCHW_W], src_tiled_shapes[NCHW_H]};
      tl_move_dst =
          cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_move_dst_shape, fmt,
                                          eu_align);

      // HW transpose, still use source shape for data transfer
      cvk_tl_t tl_dst_hw_tp;
      cvk_ctx->ops->lmem_init_tensor(
          cvk_ctx, &tl_dst_hw_tp, tl_load_dst_tiled->shape, fmt, eu_align);
      tl_dst_hw_tp.start_address = tl_move_dst->start_address;
      tl_dst_hw_tp.stride.h = tl_move_dst->stride.w;
      tl_dst_hw_tp.stride.w = tl_move_dst->stride.h;

      cvk_tiu_copy_param_t param;
      param.src = tl_load_dst_tiled;
      param.dst = &tl_dst_hw_tp;
      cvk_ctx->ops->tiu_copy(cvk_ctx, &param);
    }

    // 3. Tensor store, CW transpose
    {
      cvk_tg_t tg_dst_tiled;
      memset(&tg_dst_tiled, 0, sizeof(tg_dst_tiled));
      tg_dst_tiled.base_reg_index = 0;
      tg_dst_tiled.start_address = tg_dst->start_address + dst_offset;
      tg_dst_tiled.fmt = fmt;
      tg_dst_tiled.shape.n = tl_move_dst->shape.n;
      tg_dst_tiled.shape.c = tl_move_dst->shape.w; // CW transpose
      tg_dst_tiled.shape.h = tl_move_dst->shape.h;
      tg_dst_tiled.shape.w = tl_move_dst->shape.c; // CW transpose
      tg_dst_tiled.stride =
          cvk_ctx->ops->tg_default_stride(cvk_ctx, tg_dst_tiled.shape, fmt);

      cvk_tdma_l2g_tensor_copy_cw_transposed_param_t param;
      param.src = tl_move_dst;
      param.dst = &tg_dst_tiled;
      cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &param);
    }

    // Free local memory
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_move_dst);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_load_dst_tiled);
  }

  CVI_RT_Submit(cvk_ctx);

  int8_t *dst_data = (int8_t *)tensor_copy_d2s(rt_handle, tg_dst);

  printf("  test_tp_0231: compare\n");
  for (uint64_t i = 0; i < sizeof(ref_dst_data); i++) {
    if (dst_data[i] != ref_dst_data[i]) {
      printf("    dst_data[%" PRIu64 "]  %d != %d\n",
             i, dst_data[i], ref_dst_data[i]);
      ret = -1;
    }
  }

  // Free global memory
  free(dst_data);
  free_tensor_dev_mem(rt_handle, tg_src);
  free_tensor_dev_mem(rt_handle, tg_dst);

  return ret;
}

//
// Permute 0321, (N, C, H, W) -> (N, H, W, C)
//   tensor load
//   tensor store, cw transpose
//
//  0  1  2  3
// (N, C, H, W) -> (N, W, H, C)
// (1, 4, 2, 2) -> (1, 2, 2, 4)
//
//
// Source (1, 4, 2, 2)
//
// Tile 1   Tile 0
//   H1  ||   H0
//  3  2 ||  1  0     C0
//  7  6 ||  5  4     C1
// 11 10 ||  9  8     C2
// 15 14 || 13 12     C3
//
//
// Destination (1, 2, 2, 4)
//
//   Tile 1          Tile 0
//      H1     ||      H0
// 14 10  6  2 || 12  8  4  0   C0
// 15 11  7  3 || 13  9  5  1   C1
//
// 1. Tile 0
// 1.1. Tensor load
//     src shape (1, 4, 1, 2), stride (16, 4, 2), offset 0
//     dst shape (1, 4, 1, 2), stride (2, 2, 2)
//
//   H0
//  1  0     C0
//  5  4     C1
//  9  8     C2
// 13 12     C3
//
// 1.2. Tensor store, CW transpose
//     src shape (1, 4, 1, 2), stride (2, 2, 2)
//     dst shape (1, 2, 1, 4), stride (8, 2, 4), offset 0
//
//      H0
// 12  8  4  0   C0
// 13  9  5  1   C1
//
//
// 2. Tile 1
// 2.1. Tensor load
//    src shape (1, 4, 1, 2), stride (16, 4, 2), offset 2
//    dst shape (1, 4, 1, 2), stride (2, 1, 2)
//
//   H0
//  3  2     C0
//  7  6     C1
// 11 10     C2
// 15 14     C3
//
// 2.2. Tensor store, CW transpose
//     src shape (1, 4, 1, 2), stride (1, 2, 1, 2)
//     dst shape (1, 2, 1, 4), stride (8, 2, 4), offset 4
//
//       H1
//  14 10  6  2    C0
//  15 11  7  3    C1
//
static int test_tp_0321(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  cvk_fmt_t fmt = CVK_FMT_I8;
  int64_t data_type_size = (fmt == CVK_FMT_BF16) ? 2 : 1;
  uint8_t eu_align = 0; // No need to align eu
  uint32_t orders[NCHW_MAX_DIMS] = {0, 3, 2, 1}; // NCHW -> NWHC

  uint32_t src_shapes[NCHW_MAX_DIMS] = {1, 4, 2, 2};
  uint32_t src_strides[NCHW_MAX_DIMS];
  src_strides[NCHW_W] = data_type_size;
  src_strides[NCHW_H] = src_shapes[NCHW_W] * src_strides[NCHW_W];
  src_strides[NCHW_C] = src_shapes[NCHW_H] * src_strides[NCHW_H];
  src_strides[NCHW_N] = src_shapes[NCHW_C] * src_strides[NCHW_C];

  uint32_t dst_shapes[NCHW_MAX_DIMS] = {
      src_shapes[orders[NCHW_N]], src_shapes[orders[NCHW_C]],
      src_shapes[orders[NCHW_H]], src_shapes[orders[NCHW_W]]};
  uint32_t dst_strides[NCHW_MAX_DIMS];
  dst_strides[NCHW_W] = 1; // int8
  dst_strides[NCHW_H] = dst_shapes[NCHW_W] * dst_strides[NCHW_W];
  dst_strides[NCHW_C] = dst_shapes[NCHW_H] * dst_strides[NCHW_H];
  dst_strides[NCHW_N] = dst_shapes[NCHW_C] * dst_strides[NCHW_C];

  // Source shape (1, 4, 2, 2)
  const int8_t src_data[] = {
      //  H0 |   H1
        0,  1,  2,  3,  // C0
        4,  5,  6,  7,  // C1
        8,  9, 10, 11,  // C2
       12, 13, 14, 15   // C3
  };

  // Destination shape (1, 2, 2, 4)
  const int8_t ref_dst_data[] = {
      //    H0      |       H1
       0,  4,  8, 12,  2,  6, 10, 14,   // C0
       1,  5,  9, 13,  3,  7, 11, 15    // C1
  };

  int8_t dst_data_cpu[sizeof(ref_dst_data)];

  // Derive destination offset from source position
  uint32_t dst_index[NCHW_MAX_DIMS];
  dst_index[orders[NCHW_N]] = 0;
  dst_index[orders[NCHW_C]] = 1;
  dst_index[orders[NCHW_H]] = 2;
  dst_index[orders[NCHW_W]] = 3;

  // test element-wise copy
  {
    // source is contiguous
    for (uint32_t i = 0; i < src_shapes[NCHW_N]; ++i) {
      for (uint32_t j = 0; j < src_shapes[NCHW_C]; ++j) {
        for (uint32_t k = 0; k < src_shapes[NCHW_H]; ++k) {
          for (uint32_t l = 0; l < src_shapes[NCHW_W]; ++l) {
            uint32_t src_offset =
                i * src_strides[NCHW_N] + j * src_strides[NCHW_C] +
                k * src_strides[NCHW_H] + l * src_strides[NCHW_W];
            uint32_t dst_offset = i * dst_strides[dst_index[NCHW_N]] +
                                  j * dst_strides[dst_index[NCHW_C]] +
                                  k * dst_strides[dst_index[NCHW_H]] +
                                  l * dst_strides[dst_index[NCHW_W]];
            dst_data_cpu[dst_offset] = src_data[src_offset];
          }
        }
      }
    }

    printf("  test_tp_0321: elt copy, compare test\n");
    for (uint32_t i = 0; i < sizeof(dst_data_cpu); ++i) {
      if (dst_data_cpu[i] != ref_dst_data[i]) {
        printf("    [%d] dst_data %d != %d\n",
               i, dst_data_cpu[i], ref_dst_data[i]);
        ret = -1;
      }
    }
  }

  //
  // Data initialization in runtime.
  //
  cvk_tg_shape_t tg_src_shape = {
      src_shapes[NCHW_N], src_shapes[NCHW_C], src_shapes[NCHW_H],
      src_shapes[NCHW_W]};
  int64_t src_length =
      src_shapes[NCHW_N] * src_shapes[NCHW_C] * src_shapes[NCHW_H] *
      src_shapes[NCHW_W];

  CVI_RT_MEM gm_src_dev_mem =
      CVI_RT_MemAlloc(rt_handle, src_length * data_type_size);

  cvk_tg_t tg_src;
  gmem_init_tensor(cvk_ctx, &tg_src, tg_src_shape, fmt);
  tg_src.start_address = CVI_RT_MemGetPAddr(gm_src_dev_mem);

  // Copy from system memory to device memory
  CVI_RT_MemCopyS2D(rt_handle, gm_src_dev_mem, (uint8_t *)src_data);

  cvk_tg_shape_t tg_dst_shape = {
      dst_shapes[NCHW_N], dst_shapes[NCHW_C], dst_shapes[NCHW_H],
      dst_shapes[NCHW_W]};
  int64_t dst_length =
      dst_shapes[NCHW_N] * dst_shapes[NCHW_C] * dst_shapes[NCHW_H] *
      dst_shapes[NCHW_W];

  CVI_RT_MEM gm_dst_dev_mem =
      CVI_RT_MemAlloc(rt_handle, dst_length * data_type_size);

  cvk_tg_t tg_dst;
  gmem_init_tensor(cvk_ctx, &tg_dst, tg_dst_shape, fmt);
  tg_dst.start_address = CVI_RT_MemGetPAddr(gm_dst_dev_mem);

  //
  // Main tiled transpose routine
  //
  uint32_t src_h_step = src_shapes[NCHW_H] / 2; // 2 tiles
  uint32_t src_poss[NCHW_MAX_DIMS] = {0, 0, 0, 0};
  for (src_poss[NCHW_H] = 0; src_poss[NCHW_H] < src_shapes[NCHW_H];
       src_poss[NCHW_H] += src_h_step) {
    uint32_t src_tiled_shapes[NCHW_MAX_DIMS] = {
        src_shapes[NCHW_N], src_shapes[NCHW_C], src_shapes[NCHW_H],
        src_shapes[NCHW_W]};
    src_tiled_shapes[NCHW_H] =
        ((src_poss[NCHW_H] + src_h_step) > src_shapes[NCHW_H]) ?
            (src_shapes[NCHW_H] - src_poss[NCHW_H]) : src_h_step;

    uint32_t src_offset =
        src_poss[NCHW_N] * src_strides[NCHW_N] +
        src_poss[NCHW_C] * src_strides[NCHW_C] +
        src_poss[NCHW_H] * src_strides[NCHW_H] +
        src_poss[NCHW_W] * src_strides[NCHW_W];
    uint32_t dst_offset = src_poss[NCHW_N] * dst_strides[dst_index[NCHW_N]] +
                          src_poss[NCHW_C] * dst_strides[dst_index[NCHW_C]] +
                          src_poss[NCHW_H] * dst_strides[dst_index[NCHW_H]] +
                          src_poss[NCHW_W] * dst_strides[dst_index[NCHW_W]];

    // 1. Tensor load, tiled shape, global stride
    cvk_tl_t *tl_load_dst_tiled = NULL;
    {
      cvk_tg_t tg_src_tiled;
      memset(&tg_src_tiled, 0, sizeof(tg_src_tiled));
      tg_src_tiled.base_reg_index = 0;
      tg_src_tiled.start_address = tg_src.start_address + src_offset;
      tg_src_tiled.fmt = fmt;
      tg_src_tiled.shape.n = src_tiled_shapes[NCHW_N];
      tg_src_tiled.shape.c = src_tiled_shapes[NCHW_C];
      tg_src_tiled.shape.h = src_tiled_shapes[NCHW_H];
      tg_src_tiled.shape.w = src_tiled_shapes[NCHW_W];
      tg_src_tiled.stride.n = tg_src.stride.n;
      tg_src_tiled.stride.c = tg_src.stride.c;
      tg_src_tiled.stride.h = tg_src.stride.h;

      cvk_tl_shape_t tl_dst_tiled_shape = {
          src_tiled_shapes[NCHW_N], src_tiled_shapes[NCHW_C],
          src_tiled_shapes[NCHW_H], src_tiled_shapes[NCHW_W]};
      tl_load_dst_tiled =
          cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_dst_tiled_shape, fmt,
                                          eu_align);

      cvk_tdma_g2l_tensor_copy_param_t param;
      param.src = &tg_src_tiled;
      param.dst = tl_load_dst_tiled;
      cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &param);
    }

    // 2. Tensor store, CW transpose
    {
      cvk_tg_t tg_dst_tiled;
      memset(&tg_dst_tiled, 0, sizeof(tg_dst_tiled));
      tg_dst_tiled.base_reg_index = 0;
      tg_dst_tiled.start_address = tg_dst.start_address + dst_offset;
      tg_dst_tiled.fmt = fmt;
      tg_dst_tiled.shape.n = src_tiled_shapes[NCHW_N];
      tg_dst_tiled.shape.c = src_tiled_shapes[NCHW_W]; // CW transpose
      tg_dst_tiled.shape.h = src_tiled_shapes[NCHW_H];
      tg_dst_tiled.shape.w = src_tiled_shapes[NCHW_C]; // CW transpose
      tg_dst_tiled.stride.n = tg_dst.stride.n;
      tg_dst_tiled.stride.c = tg_dst.stride.c;
      tg_dst_tiled.stride.h = tg_dst.stride.h;

      cvk_tdma_l2g_tensor_copy_cw_transposed_param_t param;
      param.src = tl_load_dst_tiled;
      param.dst = &tg_dst_tiled;
      cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &param);
    }

    // Free local memory
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_load_dst_tiled);
  }

  CVI_RT_Submit(cvk_ctx);

  int8_t dst_data[src_length];

  // copy from device memory to system memory
  CVI_RT_MemCopyD2S(rt_handle, (uint8_t *) dst_data, gm_dst_dev_mem);

  printf("  test_tp_0321: compare\n");
  for (uint64_t i = 0; i < sizeof(ref_dst_data); i++) {
    if (dst_data[i] != ref_dst_data[i]) {
      printf("    dst_data[%" PRIu64 "]  %d != %d\n",
             i, dst_data[i], ref_dst_data[i]);
      ret = -1;
    }
  }

  CVI_RT_MemFree(rt_handle, gm_dst_dev_mem);
  CVI_RT_MemFree(rt_handle, gm_src_dev_mem);

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
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);

  ret = test_tl_copy(rt_handle, cvk_ctx, 0);
  ret |= test_tl_copy(rt_handle, cvk_ctx, 1);
  ret |= test_hw_tp(rt_handle, cvk_ctx);
  ret |= test_tp_0213(rt_handle, cvk_ctx);
  ret |= test_tp_0231(rt_handle, cvk_ctx);
  ret |= test_tp_0321(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  if (!ret)
    printf("%s pass\n", __FILENAME__);
  else
    printf("%s fail\n", __FILENAME__);

  return ret;
}
