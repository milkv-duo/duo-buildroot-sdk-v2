#include <cvimath_internal.h>
#include "gen_lut.h"  // NOLINT

#define DEBUG_TYPE "bmnet_bf16_fc_kernel"

#define RELU 0
#define PRELU 1

#define ENABLE_DBG
#ifdef ENABLE_DBG
#define LLVM_DEBUG(msg) msg
#else
#define LLVM_DEBUG(msg)
#endif

#define NEURON_MEMORY (0)
#define WEIGHT_MEMORY (1)

// declare in DTCM for preventing xmalloc/free
static cvk_ml_t matrix_lmem[5];
#define DEBUG (0)
#define DBG(fmt, ...)                             \
  do {                                            \
    if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

// gemm used
uint32_t lmem_ptr = 0;  // FIXME: move to kernel
cvk_ml_t *bmk1880v2_matrix_lmem_prealloc_align(cvk_context_t *ctx, cvk_ml_t *pre, uint32_t la,
                                               cvk_ml_shape_t s, cvk_fmt_t fmt, int eu_align) {
  uint32_t lmem_size = ctx->info.lmem_size;
  uint32_t npu_num = ctx->info.npu_num;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t val = (fmt == CVK_FMT_BF16) ? 2 : 1;

  cvk_ml_t *t;
  if (pre) {
    t = pre;
  } else {
    t = xmalloc(sizeof(*t));
  }

  t->start_address = la;
  t->fmt = fmt;
  t->shape = s;
  t->stride.h = s.w * val;
  if (eu_align)
    t->stride.c = align_up(s.w * val, eu_num);
  else
    t->stride.c = s.w * val;
  t->stride.n = t->stride.c * ceiling_func(s.c, npu_num);

  uint32_t needed = align_up(t->shape.n * t->stride.n, eu_num);

  if (lmem_size - lmem_ptr < needed) {
    if (!pre) {
      free(t);
    }
    ASSERT(0 && "not enough local memory alloc");
    return NULL;
  }
  // ctx->lmem_ptr += needed;
  lmem_ptr = la + 1;
  return t;
}

void bmk1880v2_lmem_free_prealloc_bf16_matrix(cvk_context_t *ctx, bool is_pre_alloc,
                                              const cvk_ml_t *t) {
  // printf("free from %d, lmem_ptr is %d\n", t->start_address, ctx->lmem_ptr);
  (void)ctx;
  ASSERT(t->start_address < lmem_ptr);
  lmem_ptr = t->start_address;
  if (!is_pre_alloc) {
    free((void *)t);
  }
}

static void tdma_store_stride_bf16(cvk_context_t *ctx, cvk_ml_t *tlp, uint64_t ga_dst,
                                   cvk_mg_stride_t ts_stride, ctrl_t ctrl) {
  bool DoTranspose = (ctrl & CTRL_TP) ? true : false;
  bool isNeuron = (ctrl & CTRL_NEURON) ? true : false;

  ASSERT(DoTranspose == false);
  (void)DoTranspose;

  // tensor in system memory
  // Global shape use local shape
  // Global shape used for stride calculation
  cvk_mg_t ts_data;
  ts_data.base_reg_index = isNeuron ? NEURON_MEMORY : WEIGHT_MEMORY;
  ts_data.start_address = ga_dst;
  ts_data.fmt = tlp->fmt;
  ts_data.shape.row = tlp->shape.n;
  ts_data.shape.col = tlp->shape.col;
  ts_data.stride = ts_stride;

  cvk_tdma_l2g_matrix_copy_param_t p1;
  p1.src = tlp;
  p1.dst = &ts_data;
  ctx->ops->tdma_l2g_bf16_matrix_copy(ctx, &p1);
}

static void tdma_load_stride_bf16(cvk_context_t *ctx, cvk_ml_t *tlp, uint64_t ga_src,
                                  cvk_mg_stride_t ts_stride, ctrl_t ctrl) {
  ASSERT(tlp != NULL);

  bool DoTranspose = (ctrl & CTRL_TP) ? true : false;
  bool isNeuron = (ctrl & CTRL_NEURON) ? true : false;
  (void)DoTranspose;

  // Global memory from reshaped local memory
  cvk_mg_t ts_data;
  ts_data.base_reg_index = isNeuron ? NEURON_MEMORY : WEIGHT_MEMORY;
  ts_data.start_address = ga_src;
  ts_data.fmt = tlp->fmt;
  ts_data.shape.row = tlp->shape.n;
  ts_data.shape.col = tlp->shape.col;
  ts_data.stride = ts_stride;

  // BM1880v2 tdma does not support transposed matrix load
  ASSERT(!DoTranspose);

  cvk_tdma_g2l_matrix_copy_param_t p1;
  p1.src = &ts_data;
  p1.dst = tlp;
  ctx->ops->tdma_g2l_bf16_matrix_copy(ctx, &p1);
}
//
// Shape/stride used in TDMA may not the same as in TIU.
// Adjust shape/stride for TIU.
//
// E.g.
//   Y(0, 4) = L(1, 256) * R(256, 4) + B(1, 4)
//
//   TDMA:
//      L(0, 16, 1, 16)
//      R(255, 1, 1, 4)
//      B(0, 1, 1, 4)
//
//   TIU:
//       Y res0(1, 1, 1, 16)
//       L opd0(1, 16, 1, 16)
//       R opd1(256, 1, 1, 16)
//       B opd2(1, 1, 1, 16)
//
static void matrix_multiplication(cvk_context_t *ctx, cvk_tiu_matrix_multiplication_param_t *p) {
  // No need to adjust shape/stride
  if (p->res->shape.w >= ctx->info.eu_num) {
    // LLVM_DEBUG(printf("    L(%d, %d), R(%d, %d)\n", p->left->shape.n,
    //                                       p->left->shape.col, p->right->shape.n,
    //                                       p->right->shape.col););
    ctx->ops->tiu_matrix_multiplication(ctx, p);

    return;
  }

  //
  // New shape/stride to align ctx->info.eu_num
  // adjust w as ctx->info.eu_num
  //
  cvk_ml_t tl_res;
  tl_res.start_address = p->res->start_address;
  tl_res.fmt = p->res->fmt;
  tl_res.shape.n = p->res->shape.n;
  tl_res.shape.c = p->res->shape.c;
  tl_res.shape.w = (uint32_t)(ctx->info.eu_num);
  tl_res.shape.col = p->res->shape.col;
  tl_res.stride = ctx->ops->ml_default_stride(ctx, tl_res.shape, CVK_FMT_BF16, /*eu_align=*/1);

  cvk_ml_t tl_right;
  tl_right.start_address = p->right->start_address;
  tl_right.fmt = p->right->fmt;
  tl_right.shape.n = p->right->shape.n;
  tl_right.shape.c = p->right->shape.c;
  tl_right.shape.w = (uint32_t)(ctx->info.eu_num);
  tl_right.shape.col = p->right->shape.col;
  tl_right.stride = ctx->ops->ml_default_stride(ctx, tl_right.shape, CVK_FMT_BF16, /*eu_align=*/1);

  cvk_ml_t tl_bias = {0};
  if (p->bias) {
    tl_bias.start_address = p->bias->start_address;
    tl_bias.fmt = p->bias->fmt;
    tl_bias.shape.n = p->bias->shape.n;
    tl_bias.shape.c = p->bias->shape.c;
    tl_bias.shape.w = (uint32_t)(ctx->info.eu_num);
    tl_bias.shape.col = p->bias->shape.col;
    tl_bias.stride = ctx->ops->ml_default_stride(ctx, tl_bias.shape, CVK_FMT_BF16, /*eu_align=*/1);
  }

  cvk_tiu_matrix_multiplication_param_t p2;
  // copy p to p2
  p2.res = p->res;
  p2.left = p->left;
  p2.right = p->right;
  p2.bias = p->bias;
  p2.lshift_bits = p->lshift_bits;
  p2.rshift_bits = p->rshift_bits;
  p2.res_is_int8 = p->res_is_int8;
  p2.add_result = p->add_result;
  p2.relu_enable = p->relu_enable;
  p2.ps32_mode = p->ps32_mode;
  p2.res_is_int8 = p->res_is_int8;

  p2.layer_id = p->layer_id;
  // p2.sw_op_info = p->sw_op_info;

  p2.res = &tl_res;
  p2.left = p->left;
  p2.right = &tl_right;
  p2.bias = p->bias ? &tl_bias : NULL;

  LLVM_DEBUG(printf("    Modified L(%d, %d), R(%d, %d)\n", p2.left->shape.n, p2.left->shape.col,
                    p2.right->shape.n, p2.right->shape.col););

  ctx->ops->tiu_matrix_multiplication(ctx, &p2);
}

static void fc_slicing_multi_dimention(cvk_context_t *ctx, uint32_t layer_id,
                                       gaddr_t global_offset_bottom_data,
                                       gaddr_t global_offset_weight_data,
                                       gaddr_t global_offset_bias_data,
                                       gaddr_t global_offset_top_data, int input_row_num,
                                       int input_col_num, int weight_col_num, int have_bias,
                                       int do_activation, int activation_method) {
  // Y(M, K) = L(M, K) * R(K, N) + B(1, N)
  uint32_t M = (uint32_t)(input_row_num);
  uint32_t K = (uint32_t)(input_col_num);
  uint32_t N = (uint32_t)(weight_col_num);

  LLVM_DEBUG(printf("fc_slicing_multi_dimension\n"
                    "  Y(%d, %d) = L(%d, %d) * R(%d, %d) + B(%d, %d)\n",
                    M, N, M, K, K, N, 1, N););

  // Split N <= max total eu number
  uint32_t total_eu = ctx->info.npu_num * ctx->info.eu_num;
  uint32_t tiled_N = (N >= total_eu) ? total_eu : N;

  // Split K based on lane size
  uint32_t lane_size = ctx->info.lmem_size;
  uint32_t max_k = (1 << 12) - 1;  // 1880v2: 12 bit
  uint32_t tiled_K = (K >= max_k) ? max_k : K;

  // Tiled Y
  cvk_ml_t tl_tiled_Y = {0};
  tl_tiled_Y.fmt = CVK_FMT_BF16;

  // Tiled L
  cvk_ml_t tl_tiled_L = {0};
  tl_tiled_L.fmt = CVK_FMT_BF16;

  // Tiled R
  cvk_ml_t tl_tiled_R = {0};
  tl_tiled_R.fmt = CVK_FMT_BF16;

  // Tiled B
  cvk_ml_t tl_tiled_B = {0};
  if (have_bias) {
    // ctx->ops->tiu_matrix_multiplication will change shape.n from 2 to 1
    // So we use the shape for both dma load and local memory allocation.

    // Upper16 [31:16] then Lower16 [15:0] separated by b_stride
    tl_tiled_B.fmt = CVK_FMT_BF16;
    tl_tiled_B.shape = ctx->ops->ml_default_shape(ctx, sizeof(uint32_t) / sizeof(uint16_t), tiled_N,
                                                  CVK_FMT_BF16);  // 2 x 16bit
    tl_tiled_B.stride =
        ctx->ops->ml_default_stride(ctx, tl_tiled_B.shape, CVK_FMT_BF16, /*eu_align=*/1);
  }

  // Tiled local memory layout:
  //   Y at fixed position since last tiled ones may be smaller
  //
  //   tiled Y, [7:0]
  //   tiled Y, [15:8]
  //   tiled Y, [23:16]
  //   tiled Y, [31:24]
  //   tiled L  [15:0]
  //   tiled R  [15:0]
  //   tiled B, [31:16], if existed
  //   tiled B, [15:0], if existed

  // Find max tiled K
  uint32_t required_size = 0;
  do {
    required_size = 0;  // Start of LMEM

    // Not split M since we don't want to reload L(weight)
    // or reload partial result of different M.
    //
    // Y(M, N) = L(M, K) * R(K, N) + B(1, N)
    // tiled_Y(M, tiled_N) = tiled_L(M, tiled_K) * tiled_R(tiled_K, tiled_N) + tiled_B(1, tiled_N)

    // tiled Y, 2 * 16bit
    tl_tiled_Y.start_address = required_size;
    tl_tiled_Y.shape = ctx->ops->ml_default_shape(ctx, M, tiled_N, CVK_FMT_BF16);
    tl_tiled_Y.stride =
        ctx->ops->ml_default_stride(ctx, tl_tiled_Y.shape, CVK_FMT_BF16, /*eu_align=*/1);
    required_size += ctx->ops->lmem_ps32_matrix_to_size(ctx, tl_tiled_Y.shape, CVK_FMT_BF16,
                                                        /*eu_align=*/1);

    // tiled L, 16bit
    tl_tiled_L.start_address = required_size;
    tl_tiled_L.shape = ctx->ops->ml_default_shape(ctx, M, tiled_K, CVK_FMT_BF16);
    tl_tiled_L.stride =
        ctx->ops->ml_default_stride(ctx, tl_tiled_L.shape, CVK_FMT_BF16, /*eu_align=*/1);
    required_size +=
        ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_L.shape, CVK_FMT_BF16, /*eu_align=*/1);

    // tiled R, 16bit
    tl_tiled_R.start_address = required_size;
    tl_tiled_R.shape = ctx->ops->ml_default_shape(ctx, tiled_K, tiled_N, CVK_FMT_BF16);
    tl_tiled_R.stride =
        ctx->ops->ml_default_stride(ctx, tl_tiled_R.shape, CVK_FMT_BF16, /*eu_align=*/1);
    required_size +=
        ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_R.shape, CVK_FMT_BF16, /*eu_align=*/1);

    // tiled B, 2 * 16bit
    if (have_bias) {
      tl_tiled_B.start_address = required_size;
      required_size +=
          ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_B.shape, CVK_FMT_BF16, /*eu_align=*/1);
    }

    if (required_size <= lane_size) {
      // LLVM_DEBUG(printf("  tiled_Y %d, tiled_L %d, tiled_R %d, tiled_B %d, required_size %d\n",
      //                              ctx->ops->lmem_ps32_matrix_to_size(ctx, tl_tiled_Y.shape,
      //                              CVK_FMT_BF16, /*eu_align=*/1),
      //                              ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_L.shape,
      //                              CVK_FMT_BF16, /*eu_align=*/1),
      //                              ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_R.shape,
      //                              CVK_FMT_BF16, /*eu_align=*/1),
      //                              ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_B.shape,
      //                              CVK_FMT_BF16, /*eu_align=*/1), required_size););

      break;
    }

  } while (--tiled_K);

  LLVM_DEBUG(printf("  tiled_Y(%d, %d) = tiled_L(%d, %d) * tiled_R(%d, %d) + tiled_B(%d, %d),"
                    " required_size %d kB\n",
                    M, tiled_N, M, tiled_K, tiled_K, tiled_N, 1, tiled_N, required_size / 1024););

  LLVM_DEBUG(
      printf("  tiled_Y shape (n=%d, c=%d, w=%d, col=%d), stride(n=%d, c=%d, h=%d)\n"
             "  tiled_L shape (n=%d, c=%d, w=%d, col=%d), stride(n=%d, c=%d, h=%d)\n"
             "  tiled_R shape (n=%d, c=%d, w=%d, col=%d), stride(n=%d, c=%d, h=%d)\n"
             "  tiled_B shape (n=%d, c=%d, w=%d, col=%d), stride(n=%d, c=%d, h=%d)\n",
             tl_tiled_Y.shape.n, tl_tiled_Y.shape.c, tl_tiled_Y.shape.w, tl_tiled_Y.shape.col,
             tl_tiled_Y.stride.n, tl_tiled_Y.stride.c, tl_tiled_Y.stride.h, tl_tiled_L.shape.n,
             tl_tiled_L.shape.c, tl_tiled_L.shape.w, tl_tiled_L.shape.col, tl_tiled_L.stride.n,
             tl_tiled_L.stride.c, tl_tiled_L.stride.h, tl_tiled_R.shape.n, tl_tiled_R.shape.c,
             tl_tiled_R.shape.w, tl_tiled_R.shape.col, tl_tiled_R.stride.n, tl_tiled_R.stride.c,
             tl_tiled_R.stride.h, tl_tiled_B.shape.n, tl_tiled_B.shape.c, tl_tiled_B.shape.w,
             tl_tiled_B.shape.col, tl_tiled_B.stride.n, tl_tiled_B.stride.c, tl_tiled_B.stride.h););

  ASSERT(tiled_K);
  if (!tiled_K) {
    return;
  }

  // Each tiled_R(weight) is only loaded once.
  // tiled_L(input) reload is reload once tiled_weight moves right.
  //
  // for each tiled N
  for (uint32_t offset_N = 0; offset_N < N; offset_N += tiled_N) {
    // Y = [Y0, Y1, ... Yn-1]

    // Actual width
    uint32_t width_N = ((offset_N + tiled_N) <= N) ? tiled_N : (N - offset_N);

    // for each tiled K
    for (uint32_t offset_K = 0; offset_K < K; offset_K += tiled_K) {
      // Y(M, K) = L(M, K) * R(K, N) + B(1, N)
      // tiled_Y(M, tiled_K) = tiled_L(M, tiled_K) * tiled_R(tiled_K, tiled_N) + tiled_B(1, tiled_N)
      //
      // L = [L0, L1, ... Lk-1]
      // R = [R0,0,   R0,1,   ..., R0,n-1
      //      R1,0,
      //
      //      Rk-1,0, Rk-1,1, ..., Rk-1,n-1]
      // B = [B0, B1, ... Bn-1]
      //
      // tiled_y,i += L0 * R0,i + L1 * R1,i + ... + Ln-1 * Rk-1,i + Bi

      // Actual width
      uint32_t width_K = ((offset_K + tiled_K) <= K) ? tiled_K : (K - offset_K);

      required_size = 0;  // Start of LMEM

      // tiled Y, 32bit
      tl_tiled_Y.start_address = required_size;
      tl_tiled_Y.shape = ctx->ops->ml_default_shape(ctx, M, width_N, CVK_FMT_BF16);  // actual width
      required_size += ctx->ops->lmem_ps32_matrix_to_size(ctx, tl_tiled_Y.shape, CVK_FMT_BF16,
                                                          /*eu_align=*/1);

      // Load tiled L from global memory, input
      tl_tiled_L.start_address = required_size;
      tl_tiled_L.shape = ctx->ops->ml_default_shape(ctx, M, width_K, CVK_FMT_BF16);  // actual width
      tl_tiled_L.stride = ctx->ops->ml_default_stride(ctx, tl_tiled_L.shape, CVK_FMT_BF16,
                                                      /*eu_align=*/1);
      required_size +=
          ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_L.shape, CVK_FMT_BF16, /*eu_align=*/1);
      cvk_mg_stride_t ts_stride;
      ts_stride.row = K * sizeof(uint16_t);
      tdma_load_stride_bf16(ctx, &tl_tiled_L,
                            global_offset_bottom_data + offset_K * sizeof(uint16_t),
                            ts_stride,  // original column width
                            CTRL_NEURON);

      // Load tiled R from global memory, weight
      tl_tiled_R.start_address = required_size;
      tl_tiled_R.shape =
          ctx->ops->ml_default_shape(ctx, width_K, width_N, CVK_FMT_BF16);  // actual width
      tl_tiled_R.stride = ctx->ops->ml_default_stride(ctx, tl_tiled_R.shape, CVK_FMT_BF16,
                                                      /*eu_align=*/1);
      required_size +=
          ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_R.shape, CVK_FMT_BF16, /*eu_aligned=*/1);

      ts_stride.row = N * sizeof(uint16_t);
      tdma_load_stride_bf16(
          ctx, &tl_tiled_R,
          global_offset_weight_data + (offset_K * N + offset_N) * sizeof(uint16_t),
          ts_stride,  // original column width
          CTRL_NEURON);

      // Load tiled B(bias) from gobale memory at last time as H/W does
      // we need temporary shape to load uppper 16bit and lower 16bit
      bool is_last_tile = ((offset_K + tiled_K) >= K) ? true : false;
      bool B_needed = (is_last_tile && have_bias) ? true : false;
      if (B_needed) {
        tl_tiled_B.start_address = required_size;

        tl_tiled_B.shape =
            ctx->ops->ml_default_shape(ctx, sizeof(uint32_t) / sizeof(uint16_t), width_N,
                                       CVK_FMT_BF16);  // 2 x 16bit, actual width
        tl_tiled_B.stride =
            ctx->ops->ml_default_stride(ctx, tl_tiled_B.shape, CVK_FMT_BF16, /*eu_align=*/1);
        required_size += ctx->ops->lmem_matrix_to_size(ctx, tl_tiled_B.shape, CVK_FMT_BF16,
                                                       /*eu_aligned=*/1);
        ASSERT(required_size <= lane_size);

        ts_stride.row = N * sizeof(uint16_t);
        tdma_load_stride_bf16(ctx, &tl_tiled_B,
                              global_offset_bias_data + offset_N * sizeof(uint16_t),
                              ts_stride,  // original column width
                              CTRL_NEURON);
      }

      uint32_t ps32_mode = 0;    // normal mode
      uint32_t relu_enable = 0;  // 1880v2 relu can be used in ps32_mode
      if (tiled_K < K) {
        if (offset_K == 0) {        // first tile
          ps32_mode = 2;            // write 32b result at the first time
        } else if (is_last_tile) {  // last tile
          ps32_mode = 1;            // load previous 32-bit result
        } else {
          ps32_mode = 3;  // init & write 32bits partial sum
        }
      }

      // No tiling or last tile
      if ((ps32_mode == 0 || ps32_mode == 1) && do_activation && activation_method == RELU) {
        relu_enable = 1;
      }

      {
        cvk_tiu_matrix_multiplication_param_t p;
        p.res = &tl_tiled_Y;
        p.left = &tl_tiled_L;
        p.right = &tl_tiled_R;
        p.bias = B_needed ? &tl_tiled_B : NULL;
        p.lshift_bits = 0;  // deprecated
        p.rshift_bits = 0;
        p.res_is_int8 = 1;  // H/W constraint
        p.add_result = 0;   // H/W constraint
        p.relu_enable = relu_enable;
        p.ps32_mode = ps32_mode;
        p.res_is_int8 = 1;

        p.layer_id = layer_id;
        // p.sw_op_info = offset_N;

        LLVM_DEBUG(printf("  [offset_N=%d][offset_K=%d] L(%d, %d), R(%d, %d)\n", offset_N, offset_K,
                          p.left->shape.n, p.left->shape.col, p.right->shape.n,
                          p.right->shape.col););

        matrix_multiplication(ctx, &p);
      }

      // Store tiled_Y to global memory
      if (is_last_tile) {
        ts_stride.row = N * sizeof(uint16_t);
        tdma_store_stride_bf16(ctx, &tl_tiled_Y,
                               global_offset_top_data + offset_N * sizeof(uint16_t),
                               ts_stride,  // original column width
                               CTRL_NEURON);
      }

    }  // for (uint32_t offset_K = 0; offset_K < K; offset_K += tiled_K)

  }  // for (uint32_t offset_N = 0; offset_N < N; offset_N += tiled_N)
}

void cvm_fc_forward_kernel(cvk_context_t *ctx, uint32_t layer_id, gaddr_t bottom_data_gaddr,
                           gaddr_t weight_data_gaddr, gaddr_t bias_data_gaddr,
                           gaddr_t top_data_gaddr, int in_row, int in_col, int out_col,
                           int have_bias, int do_activation, int activation_method) {
  // LLVM_DEBUG(
  //    printf("bf16_fc_forward_kernel\n"
  //           "    bottom_gaddr 0x%lx, weight_gaddr 0x%lx, bias_gaddr 0x%lx, top_gaddr 0x%lx\n"
  //           "    in (%d, %d), out (%d)\n"
  //           "    has_bias %d, do_activation %d, activation_method %d\n",
  //           bottom_data_gaddr, weight_data_gaddr, bias_data_gaddr, top_data_gaddr, in_row,
  //           in_col, out_col, have_bias, do_activation, activation_method););

  fc_slicing_multi_dimention(ctx, layer_id, bottom_data_gaddr, weight_data_gaddr, bias_data_gaddr,
                             top_data_gaddr, in_row, in_col, out_col, have_bias, do_activation,
                             activation_method);
}

// gemm
inline static size_t get_neuron_csize_local(cvk_context_t *ctx, size_t h, size_t w, cvk_fmt_t fmt) {
  size_t size = h * w * bitsize_of_fmt(fmt) / 8;
  // ctx->info.eu_num neurons align
  return ALIGN(size, ctx->info.eu_num);
}

static int get_fmt_byte_sz(cvk_fmt_t fmt) { return bitsize_of_fmt(fmt) / 8; }

static uint64_t get_slice_global_offset(uint64_t global_offset, size_t row_slice_idx,
                                        size_t col_slice_idx, size_t row_num, size_t col_num,
                                        size_t row_slice_num, size_t col_slice_num, cvk_fmt_t fmt) {
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
  uint64_t slice_offset_row = 0;
  if (row_slice_idx < (row_num % row_slice_num)) {
    slice_offset_row = row_slice_idx * (row_num / row_slice_num + 1);
  } else {
    slice_offset_row = (row_num % row_slice_num) * (row_num / row_slice_num + 1) +
                       (row_slice_idx - (row_num % row_slice_num)) * (row_num / row_slice_num);
  }

  uint64_t slice_offset_col = 0;
  if (col_slice_idx < (col_num % col_slice_num)) {
    slice_offset_col = col_slice_idx * (col_num / col_slice_num + 1);
  } else {
    slice_offset_col = (col_num % col_slice_num) * (col_num / col_slice_num + 1) +
                       (col_slice_idx - (col_num % col_slice_num)) * (col_num / col_slice_num);
  }

  uint64_t slice_offset;
  slice_offset = (slice_offset_col + slice_offset_row * col_num) * fmt_byte_sz;
  return (global_offset + slice_offset);
}

#define LOCAL_MEM_BANKS (ctx->info.lmem_banks)
#define NPU_SHIFT (get_num_shift(ctx->info.npu_num))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
//#define SMALL_TEST (1)
static size_t get_slice_num(cvk_context_t *ctx, size_t M, size_t N, size_t K, size_t *slice_num,
                            cvk_fmt_t fmt) {
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
#ifdef SMALL_TEST
  size_t bank_size = (2048 / LOCAL_MEM_BANKS / fmt_byte_sz);
#else  /* ! ifdef SMALL_TEST */
  size_t bank_size = (ctx->info.lmem_size / LOCAL_MEM_BANKS / fmt_byte_sz);
#endif /* SMALL_TEST */
  slice_num[0] = slice_num[1] = slice_num[2] = 1;

  size_t W_param = ctx->info.eu_num;
  size_t csize_local = get_neuron_csize_local(ctx, 1, W_param, fmt);
  size_t C_param = (K + W_param - 1) / W_param;
  size_t size_A = M * ceiling_func_shift(C_param, NPU_SHIFT) * csize_local;
  C_param = (N + W_param - 1) / W_param;
  size_t size_B = K * ceiling_func_shift(C_param, NPU_SHIFT) * csize_local;
  int res_byte_sz = 1;
  if (fmt != CVK_FMT_BF16) {
    // partial sum for 32bit output
    res_byte_sz = sizeof(int);
  }

  size_t size_C = res_byte_sz * M * ceiling_func_shift(C_param, NPU_SHIFT) * csize_local;

  DBG(" A size: %zu, B size: %zu, C size: %zu, bank: %zu\n", size_A, size_B, size_C, bank_size);

  if (size_A <= bank_size && size_B <= bank_size && size_C <= bank_size) {
    return 0;
  } else if (size_B <= bank_size) {
    slice_num[0] = MAX(ceiling_func(size_A, bank_size), ceiling_func(size_C, bank_size));
    // split C local memory size
    size_t slice_size = ceiling_func(M, slice_num[0]);
    C_param = (N + ctx->info.eu_num - 1) / ctx->info.eu_num;
    size_t slice_mem_C = slice_size * ceiling_func_shift(C_param, NPU_SHIFT) * csize_local;
    if (slice_mem_C > bank_size) return 3;
    return 1;
  } else if (size_A <= bank_size) {
    slice_num[1] = MAX(ceiling_func(size_B, bank_size), ceiling_func(size_C, bank_size));
    // Split more times in N, use load_stride maybe override previous data.
    if (slice_num[1] > 1) {
      int N_silce = N / slice_num[1];
      int N_floor = (N_silce / ctx->info.eu_num) * ctx->info.eu_num;
      if (N_floor == 0) return 3;
      slice_num[1] = ceiling_func(N, N_floor);
    }
    //
    if (ceiling_func(ceiling_func(N, ctx->info.eu_num), NPU_SHIFT) < (int)slice_num[1]) return 3;
    return 2;
  } else {
    return 3;
  }
}

static inline size_t get_max(size_t a, size_t b) { return a > b ? a : b; }

static int load_matrix(cvk_context_t *ctx, cvk_ml_t *tlp, uint64_t gaddr_in, cvk_mg_shape_t shape,
                       cvk_mg_stride_t stride, cvk_fmt_t fmt) {
  // load matrix data
  cvk_mg_t ts_data;
  ts_data.base_reg_index = 0;
  ts_data.start_address = gaddr_in;
  ts_data.stride = stride;
  ts_data.fmt = fmt;
  ts_data.shape = shape;
  cvk_tdma_g2l_matrix_copy_param_t p1;
  p1.src = &ts_data;
  p1.dst = tlp;
  if (fmt == CVK_FMT_BF16) {
    ctx->ops->tdma_g2l_bf16_matrix_copy(ctx, &p1);
  } else {
    ctx->ops->tdma_g2l_matrix_copy(ctx, &p1);
  }

  return 0;
}

static int store_matrix(cvk_context_t *ctx, cvk_ml_t *tlp, uint64_t gaddr_in, cvk_mg_shape_t shape,
                        cvk_mg_stride_t stride, cvk_fmt_t fmt) {
  cvk_mg_t ts_data;
  ts_data.base_reg_index = 0;
  ts_data.start_address = gaddr_in;
  ts_data.fmt = fmt;
  ts_data.shape = shape;
  ts_data.stride = stride;
  cvk_tdma_l2g_matrix_copy_param_t p3;
  p3.src = tlp;
  p3.dst = &ts_data;
  if (fmt == CVK_FMT_BF16) {
    ctx->ops->tdma_l2g_bf16_matrix_copy(ctx, &p3);
  } else {
    ctx->ops->tdma_l2g_matrix_copy(ctx, &p3);
  }

  return 0;
}

static int layerid = 0;
static void _matrix_multiplication(cvk_context_t *ctx, cvk_ml_t *tlp_a, cvk_ml_t *tlp_b,
                                   cvk_ml_t *tlp_c, int ps32_mode) {
  // mac
  cvk_tiu_matrix_multiplication_param_t p2;
  p2.bias = NULL;
  p2.left = tlp_a;
  p2.right = tlp_b;
  p2.res = tlp_c;
  p2.lshift_bits = 0;
  p2.rshift_bits = 0;
  if (tlp_c->fmt == CVK_FMT_BF16) {
    p2.res_is_int8 = true;
  } else {
    // for int
    p2.res_is_int8 = false;
  }
  p2.relu_enable = 0;
  p2.add_result = 0; /*bf16 HW does not support add_result*/
  p2.ps32_mode = ps32_mode;

  p2.layer_id = layerid;
  layerid++;
  ctx->ops->tiu_matrix_multiplication(ctx, &p2);
}

static void strategy_no_slice(cvk_context_t *ctx, size_t M, size_t N, size_t K, uint64_t gaddr_a,
                              uint64_t gaddr_b, uint64_t gaddr_c, cvk_fmt_t fmt) {
  cvk_ml_t *tlp_a;
  cvk_ml_t *tlp_b;
  cvk_ml_t *tlp_c;
  // size_t bank_size = ctx->info.lmem_size / LOCAL_MEM_BANKS * 2;
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
  // size_t bank_size = ctx->info.lmem_size / LOCAL_MEM_BANKS;
  int psmode = 0;  // default for bf16

  cvk_ml_shape_t shape_a = ctx->ops->ml_default_shape(ctx, M, K, fmt);
  cvk_ml_shape_t shape_b = ctx->ops->ml_default_shape(ctx, K, N, fmt);
  cvk_ml_shape_t shape_c = ctx->ops->ml_default_shape(ctx, M, N, fmt);

  tlp_a = ctx->ops->lmem_alloc_matrix(ctx, shape_a, fmt, CTRL_AL);
  tlp_b = ctx->ops->lmem_alloc_matrix(ctx, shape_b, fmt, CTRL_AL);
  if (fmt == CVK_FMT_BF16) {
    tlp_c = ctx->ops->lmem_alloc_matrix(ctx, shape_c, fmt, CTRL_AL);
  } else {
    shape_c.n = shape_a.n;
    shape_c.c = shape_b.c;
    shape_c.w = shape_b.w;
    shape_c.col = shape_b.col;

    tlp_c = ctx->ops->lmem_alloc_ps32_matrix(ctx, shape_c, fmt, CTRL_AL);
    psmode = 2;
  }

  cvk_mg_shape_t shape;
  shape.row = tlp_a->shape.n;
  shape.col = tlp_a->shape.col;
  cvk_mg_stride_t stride;
  stride.row = (uint32_t)K * fmt_byte_sz;
  load_matrix(ctx, tlp_a, gaddr_a, shape, stride, fmt);

  shape.row = tlp_b->shape.n;
  shape.col = tlp_b->shape.col;
  stride.row = (uint32_t)N * fmt_byte_sz;
  load_matrix(ctx, tlp_b, gaddr_b, shape, stride, fmt);

  // mac
  _matrix_multiplication(ctx, tlp_a, tlp_b, tlp_c, psmode);

  if (fmt != CVK_FMT_BF16) {
    tlp_c->shape.n *= sizeof(int);  // partial sum for 32bit output
  }
  shape.row = tlp_c->shape.n;
  shape.col = tlp_c->shape.col;
  stride.row = (uint32_t)N * fmt_byte_sz;
  store_matrix(ctx, tlp_c, gaddr_c, shape, stride, fmt);

  ctx->ops->lmem_free_matrix(ctx, tlp_c);
  ctx->ops->lmem_free_matrix(ctx, tlp_b);
  ctx->ops->lmem_free_matrix(ctx, tlp_a);
}

static void strategy_slice_on_M(cvk_context_t *ctx, size_t M, size_t N, size_t K, uint64_t gaddr_a,
                                uint64_t gaddr_b, uint64_t gaddr_c, size_t slice_num,
                                cvk_fmt_t fmt) {
  cvk_ml_t *tlp_b;
  int fmt_byte_sz = get_fmt_byte_sz(fmt);

  cvk_ml_shape_t s_B = ctx->ops->ml_default_shape(ctx, K, N, fmt);

  tlp_b = ctx->ops->lmem_alloc_matrix(ctx, s_B, fmt, CTRL_AL);

  cvk_mg_shape_t shape;
  shape.row = tlp_b->shape.n;
  shape.col = tlp_b->shape.col;
  cvk_mg_stride_t stride;
  stride.row = (uint32_t)N * fmt_byte_sz;
  load_matrix(ctx, tlp_b, gaddr_b, shape, stride, fmt);

  int pack_shift = 0;
  int psmode = 0;  // default for bf16
  for (size_t slice_idx = 0; slice_idx < slice_num; slice_idx++) {
    cvk_ml_t *tlp_a;
    size_t M_slice = M / slice_num + (slice_idx < M % slice_num);
    cvk_ml_shape_t s_A = ctx->ops->ml_default_shape(ctx, M_slice, K, fmt);
    tlp_a = ctx->ops->lmem_alloc_matrix(ctx, s_A, fmt, CTRL_AL);

    uint64_t A_slice_global_offset =
        get_slice_global_offset(gaddr_a, slice_idx, 0, M, K, slice_num, 1, fmt);

    cvk_mg_shape_t shape;
    shape.row = tlp_a->shape.n;
    shape.col = tlp_a->shape.col;
    cvk_mg_stride_t st_A;
    st_A.row = (uint32_t)K * fmt_byte_sz;
    load_matrix(ctx, tlp_a, A_slice_global_offset, shape, st_A, fmt);

    tlp_a->shape.n = M_slice;
    tlp_a->shape.col = K;

    cvk_ml_shape_t s_C = ctx->ops->ml_default_shape(ctx, M_slice, N, fmt);
    cvk_ml_t *tlp_c;

    if (fmt == CVK_FMT_BF16) {
      tlp_c = ctx->ops->lmem_alloc_matrix(ctx, s_C, fmt, CTRL_AL);
    } else {
      s_C.n = tlp_a->shape.n;
      s_C.c = tlp_b->shape.c;
      s_C.w = tlp_b->shape.w;
      s_C.col = tlp_b->shape.col;

      tlp_c = ctx->ops->lmem_alloc_ps32_matrix(ctx, s_C, fmt, CTRL_AL);
      psmode = 2;
    }

    uint64_t C_slice_global_offset =
        get_slice_global_offset(gaddr_c, slice_idx, 0, M, N, slice_num, 1, fmt);

    if (fmt != CVK_FMT_BF16) {
      // int32 pack
      C_slice_global_offset = get_slice_global_offset(gaddr_c, 0, 0, M, N, slice_num, 1, fmt);
      C_slice_global_offset += pack_shift;
      pack_shift += (tlp_c->shape.n * tlp_c->shape.col * sizeof(int));
      // C_slice_global_offset =
      //  get_slice_global_offset(gaddr_c, slice_idx, 0, M * 2, N * 2, slice_num, 1, fmt);
    }

    _matrix_multiplication(ctx, tlp_a, tlp_b, tlp_c, psmode);

    if (fmt != CVK_FMT_BF16) {
      tlp_c->shape.n *= sizeof(int);  // partial sum for 32bit output
    }
    shape.row = tlp_c->shape.n;
    shape.col = tlp_c->shape.col;
    stride.row = (uint32_t)N * fmt_byte_sz;  // place with no tiling
    store_matrix(ctx, tlp_c, C_slice_global_offset, shape, stride, fmt);

    ctx->ops->lmem_free_matrix(ctx, tlp_c);
    ctx->ops->lmem_free_matrix(ctx, tlp_a);
  }
  ctx->ops->lmem_free_matrix(ctx, tlp_b);
}

static void strategy_slice_on_N(cvk_context_t *ctx, size_t M, size_t N, size_t K, uint64_t gaddr_a,
                                uint64_t gaddr_b, uint64_t gaddr_c, size_t slice_num,
                                cvk_fmt_t fmt) {
  cvk_ml_t *tlp_a;
  int fmt_byte_sz = get_fmt_byte_sz(fmt);

  cvk_ml_shape_t s_a = ctx->ops->ml_default_shape(ctx, M, K, fmt);
  tlp_a = ctx->ops->lmem_alloc_matrix(ctx, s_a, fmt, CTRL_AL);

  cvk_mg_stride_t stride;
  stride.row = (uint32_t)K * fmt_byte_sz;
  cvk_mg_shape_t shape;
  shape.row = tlp_a->shape.n;
  shape.col = tlp_a->shape.col;
  load_matrix(ctx, tlp_a, gaddr_a, shape, stride, fmt);

  int pack_shift = 0;
  int psmode = 0;  // default for bf16
  for (size_t slice_idx = 0; slice_idx < slice_num; slice_idx++) {
    size_t N_slice = N / slice_num + (slice_idx < N % slice_num);

    cvk_ml_shape_t s_b = ctx->ops->ml_default_shape(ctx, K, N_slice, fmt);
    cvk_ml_t *tlp_b;
    tlp_b = ctx->ops->lmem_alloc_matrix(ctx, s_b, fmt, CTRL_AL);

    uint64_t B_slice_global_offset =
        get_slice_global_offset(gaddr_b, 0, slice_idx, K, N, 1, slice_num, fmt);

    // load b
    stride.row = (uint32_t)N * fmt_byte_sz;
    shape.row = tlp_b->shape.n;
    shape.col = tlp_b->shape.col;
    load_matrix(ctx, tlp_b, B_slice_global_offset, shape, stride, fmt);

    // c for answer
    cvk_ml_shape_t s_c = ctx->ops->ml_default_shape(ctx, M, N_slice, fmt);
    cvk_ml_t *tlp_c;
    if (fmt == CVK_FMT_BF16) {
      tlp_c = ctx->ops->lmem_alloc_matrix(ctx, s_c, fmt, CTRL_AL);
    } else {
      s_c.n = tlp_a->shape.n;
      s_c.c = tlp_b->shape.c;
      s_c.w = tlp_b->shape.w;
      s_c.col = tlp_b->shape.col;

      tlp_c = ctx->ops->lmem_alloc_ps32_matrix(ctx, s_c, fmt, CTRL_AL);
      psmode = 2;
    }

    uint64_t C_slice_global_offset =
        get_slice_global_offset(gaddr_c, 0, slice_idx, M, N, 1, slice_num, fmt);
    if (fmt != CVK_FMT_BF16) {
      // int32 pack
      C_slice_global_offset = get_slice_global_offset(gaddr_c, 0, 0, M, N, 1, slice_num, fmt);
      C_slice_global_offset += pack_shift;
      pack_shift += tlp_c->shape.col;
      // C_slice_global_offset =
      //  get_slice_global_offset(gaddr_c, 0, slice_idx, M*2, N*2, 1, slice_num, fmt);
    }

    _matrix_multiplication(ctx, tlp_a, tlp_b, tlp_c, psmode);

    if (fmt != CVK_FMT_BF16) {
      tlp_c->shape.n *= sizeof(int);  // partial sum for 32bit output
    }
    shape.row = tlp_c->shape.n;
    shape.col = tlp_c->shape.col;
    stride.row = (uint32_t)N * fmt_byte_sz;
    store_matrix(ctx, tlp_c, C_slice_global_offset, shape, stride, fmt);

    ctx->ops->lmem_free_matrix(ctx, tlp_c);
    ctx->ops->lmem_free_matrix(ctx, tlp_b);
  }

  ctx->ops->lmem_free_matrix(ctx, tlp_a);
}

static void slice_split_strategy(cvk_context_t *ctx, size_t M, size_t N, size_t K,
                                 size_t *slice_num, cvk_fmt_t fmt) {
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
  size_t W_param = ctx->info.eu_num;
  size_t channel_size_local = get_neuron_csize_local(ctx, 1, W_param, fmt);
#ifdef SMALL_TEST
  size_t bank_size = (2048 / LOCAL_MEM_BANKS / fmt_byte_sz);
#else
  size_t bank_size = (ctx->info.lmem_size / LOCAL_MEM_BANKS / fmt_byte_sz);
#endif
  size_t bank_size_half = bank_size >> 1;
  slice_num[0] = slice_num[1] = slice_num[2] = 1;

  // input blob
  size_t C_param = (K + W_param - 1) / W_param;
  size_t local_size_A = M * (ceiling_func_shift(C_param, NPU_SHIFT)) * channel_size_local;
  size_t slice_num_A = (local_size_A + bank_size_half - 1) / (bank_size_half);
  size_t col_slice_time_A = ceiling_func_shift(C_param, NPU_SHIFT);
  size_t row_slice_time_A = (slice_num_A < M) ? slice_num_A : M;

  // weight blob
  C_param = (N + W_param - 1) / W_param;
  size_t local_size_B = K * (ceiling_func_shift(C_param, NPU_SHIFT)) * channel_size_local;
  size_t slice_num_B = (local_size_B + bank_size_half - 1) / bank_size_half;
  size_t row_slice_time_B = (slice_num_B < K) ? slice_num_B : K;

  // output blob
  C_param = (N + W_param - 1) / W_param;
  // multi 2 for simulating result add
  int outputs_nr = 2;
  if (fmt != CVK_FMT_BF16) {
    // int8 output 32bit result
    // outputs_nr = 4;
  }
  size_t local_size_C = (M + 1) * (ceiling_func_shift(C_param, NPU_SHIFT)) * channel_size_local;
  size_t slice_num_C = (local_size_C + bank_size * outputs_nr - 1) / (bank_size * outputs_nr);
  size_t col_slice_time_C = ceiling_func_shift(C_param, NPU_SHIFT);

  // A
  if (col_slice_time_A == 0) {
    slice_num[0] = row_slice_time_A;
  } else {
    if (col_slice_time_A < slice_num_A) {
      slice_num[0] = (slice_num_A + col_slice_time_A - 1) / col_slice_time_A;
    } else {
      slice_num[0] = 1;
      slice_num[2] = slice_num_A;
    }
  }

  // C
  if ((slice_num_C > slice_num[0]) && col_slice_time_C) {
    size_t tmp = (slice_num_C + slice_num[0] - 1) / slice_num[0];
    slice_num[1] = (col_slice_time_C > tmp) ? tmp : col_slice_time_C;
  }

  // B
  if (slice_num_B > slice_num[1]) {
    size_t tmp = (slice_num_B + slice_num[1] - 1) / slice_num[1];
    slice_num[2] = get_max(slice_num[2], (row_slice_time_B > tmp) ? tmp : row_slice_time_B);
  }
  // fine-tuning
  size_t matrix_shape[3] = {1, 1, 1};
  while (true) {
    matrix_shape[0] = (M + slice_num[0] - 1) / slice_num[0];
    matrix_shape[2] = (N + slice_num[1] - 1) / slice_num[1];
    matrix_shape[1] = (K + slice_num[2] - 1) / slice_num[2];
    size_t C_param_input_col = (matrix_shape[1] + W_param - 1) / W_param;
    size_t C_param_weight_col = (matrix_shape[2] + W_param - 1) / W_param;

    size_t local_size_B =
        matrix_shape[1] * (ceiling_func_shift(C_param_weight_col, NPU_SHIFT)) * channel_size_local;
    size_t local_size_C =
        matrix_shape[0] * (ceiling_func_shift(C_param_weight_col, NPU_SHIFT)) * channel_size_local;
    size_t local_size_A =
        matrix_shape[0] * (ceiling_func_shift(C_param_input_col, NPU_SHIFT)) * channel_size_local;
    bool slicing_success = (local_size_A <= bank_size_half) &&
                           (local_size_C <= bank_size * outputs_nr) &&
                           (local_size_B <= bank_size_half);

    if (slicing_success) {
      if (slice_num[1] > 1) {
        int N_silce = N / slice_num[1];
        int N_floor = (N_silce / ctx->info.eu_num) * ctx->info.eu_num;
        ASSERT(N_floor);
        slice_num[1] = ceiling_func(N, N_floor);
      }
#if 0  // def DEBUG_LOCAL
      size_t bias_local_size =
          (ceiling_func_shift(C_param_weight_col, NPU_SHIFT)) * channel_size_local;
      // DBG("multi-dim slicing:\n");
      DBG("local_size_B = %lu\n", local_size_B);
      DBG("local_size_C = %lu\n", local_size_C);
      DBG("local_size_A =  %lu\n", local_size_A);
      DBG("bias_local_size =   %lu\n", bias_local_size);
#endif
      return;
    } else if (local_size_A > bank_size_half) {
      slice_num[2]++;
    } else if (local_size_B > bank_size_half) {
      slice_num[2]++;
    } else if (local_size_C > 2 * bank_size) {
      slice_num[0]++;
    }
  }
}

static void strategy_slice_on_multidim_init(cvk_context_t *ctx, gaddr_t *slice_global_offset,
                                            size_t *matrix_shape, size_t *slice_row_stride,
                                            cvk_fmt_t fmt) {
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
  gaddr_t global_offset_A = slice_global_offset[0];
  gaddr_t global_offset_B = slice_global_offset[1];
  size_t row_num_A = matrix_shape[0];
  size_t col_num_A = matrix_shape[1];
  size_t col_num_B = matrix_shape[2];

  cvk_ml_shape_t s_A, s_B;
  s_A = ctx->ops->ml_default_shape(ctx, row_num_A, col_num_A, fmt);
  s_B = ctx->ops->ml_default_shape(ctx, col_num_A, col_num_B, fmt);

  cvk_mg_stride_t st_A, st_B;
  st_A.row = (uint32_t)slice_row_stride[0] * fmt_byte_sz;
  st_B.row = (uint32_t)slice_row_stride[1] * fmt_byte_sz;

#ifdef SMALL_TEST
  size_t bank_size = 2048 / LOCAL_MEM_BANKS;
#else
  size_t bank_size = ctx->info.lmem_size / LOCAL_MEM_BANKS * 2;
#endif
  cvk_ml_t *tl_A = bmk1880v2_matrix_lmem_prealloc_align(ctx, &matrix_lmem[0], 0, s_A, fmt, CTRL_AL);
  cvk_ml_t *tl_B =
      bmk1880v2_matrix_lmem_prealloc_align(ctx, &matrix_lmem[1], bank_size, s_B, fmt, CTRL_AL);
  cvk_mg_shape_t shape;
  shape.row = tl_A->shape.n;
  shape.col = tl_A->shape.col;
  load_matrix(ctx, tl_A, global_offset_A, shape, st_A, fmt);

  shape.row = tl_B->shape.n;
  shape.col = tl_B->shape.col;
  load_matrix(ctx, tl_B, global_offset_B, shape, st_B, fmt);
  // DBG("0->load from %u/%u,off %lu/%lu\n", tl_A->start_address, tl_B->start_address,
  // global_offset_A,
  //    global_offset_B);

  bool is_alloc_from_stack = true;
  bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_B);
  bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_A);
}

static int strategy_slice_on_multi_dimension_internal(
    cvk_context_t *ctx, size_t *slice_idx, size_t *slice_num, gaddr_t *slice_global_offset,
    size_t *matrix_shape, gaddr_t *slice_global_offset_next, size_t *matrix_shape_next,
    size_t *slice_row_stride, cvk_fmt_t fmt) {
  int fmt_byte_sz = get_fmt_byte_sz(fmt);
  size_t row_num_A = matrix_shape[0];
  size_t col_num_A = matrix_shape[1];
  size_t col_num_B = matrix_shape[2];

  gaddr_t global_offset_next_A = slice_global_offset_next[0];
  gaddr_t global_offset_next_B = slice_global_offset_next[1];

  size_t row_num_next_A = matrix_shape_next[0];
  size_t col_num_next_A = matrix_shape_next[1];
  size_t col_num_next_B = matrix_shape_next[2];

  cvk_ml_shape_t s_next_A = ctx->ops->ml_default_shape(ctx, row_num_next_A, col_num_next_A, fmt);
  cvk_ml_shape_t s_next_B = ctx->ops->ml_default_shape(ctx, col_num_next_A, col_num_next_B, fmt);
  cvk_ml_shape_t s_A, s_B, s_C;

  s_A = ctx->ops->ml_default_shape(ctx, row_num_A, col_num_A, fmt);
  s_B = ctx->ops->ml_default_shape(ctx, col_num_A, col_num_B, fmt);
  s_C = ctx->ops->ml_default_shape(ctx, row_num_A, col_num_B, fmt);

  // int partition = 2; // 2 means one for A/B, another for C with double output
  // if (fmt != CVK_FMT_BF16) {
  //  partition = 6;
  //}
  // size_t bank_size = ctx->info.lmem_size / LOCAL_MEM_BANKS * 2;
#ifdef SMALL_TEST
  size_t bank_size = 2048 / LOCAL_MEM_BANKS;
#else
  size_t bank_size = ctx->info.lmem_size / LOCAL_MEM_BANKS * 2;
#endif
  size_t hf_bsize = bank_size / 2;
  size_t cur = slice_idx[1] % 2;
  size_t next = (slice_idx[1] + 1) % 2;

  int output_nr = 2;  // 2 means output with low part and high part
  int psmode = 1;     // default for bf16
  if (fmt != CVK_FMT_BF16) {
    // output_nr = 4; // 4 for 32bit output with 4 * 1 byte output
    psmode = 3;
  }

  cvk_mg_stride_t st_A, st_C, st_B;
  st_A.row = (uint32_t)slice_row_stride[0] * fmt_byte_sz;
  st_C.row = (uint32_t)slice_row_stride[1] * fmt_byte_sz;
  st_B.row = (uint32_t)slice_row_stride[1] * fmt_byte_sz;

  cvk_ml_t *tl_A =
      bmk1880v2_matrix_lmem_prealloc_align(ctx, &matrix_lmem[0], hf_bsize * cur, s_A, fmt, CTRL_AL);
  cvk_ml_t *tl_next_A = bmk1880v2_matrix_lmem_prealloc_align(ctx, &matrix_lmem[1], hf_bsize * next,
                                                             s_next_A, fmt, CTRL_AL);
  cvk_ml_t *tl_B = bmk1880v2_matrix_lmem_prealloc_align(
      ctx, &matrix_lmem[2], bank_size + hf_bsize * cur, s_B, fmt, CTRL_AL);
  cvk_ml_t *tl_next_B = bmk1880v2_matrix_lmem_prealloc_align(
      ctx, &matrix_lmem[3], bank_size + hf_bsize * next, s_next_B, fmt, CTRL_AL);
  cvk_ml_t *tl_C = bmk1880v2_matrix_lmem_prealloc_align(ctx, &matrix_lmem[4], output_nr * bank_size,
                                                        s_C, fmt, CTRL_AL);

  ctx->ops->parallel_enable(ctx);

  if (slice_num[2] - 1 > slice_idx[1]) {
    st_A.row = (uint32_t)slice_row_stride[0] * fmt_byte_sz;
    st_B.row = (uint32_t)slice_row_stride[1] * fmt_byte_sz;

    cvk_mg_shape_t shape;
    shape.row = tl_next_A->shape.n;
    shape.col = tl_next_A->shape.col;
    load_matrix(ctx, tl_next_A, global_offset_next_A, shape, st_A, fmt);

    shape.row = tl_next_B->shape.n;
    shape.col = tl_next_B->shape.col;
    load_matrix(ctx, tl_next_B, global_offset_next_B, shape, st_B, fmt);

    // DBG("do %u/%u ", tl_A->start_address, tl_B->start_address);
#define PS32_CTRL_RA (3)   /* normal case */
#define PS32_CTRL_NULL (2) /* first one */
    int pint32_t_status = slice_idx[1] > 0 ? PS32_CTRL_RA : PS32_CTRL_NULL;
    _matrix_multiplication(ctx, tl_A, tl_B, tl_C, pint32_t_status);
    // DBG(">load from %u/%u off %lu/%lu s(%d)\n", tl_next_A->start_address,
    // tl_next_B->start_address,
    //    global_offset_next_A, global_offset_next_B, pint32_t_status);
  }

  if (slice_idx[1] == slice_num[2] - 1) {
    // last one
    // not using ps mode 1 cuz it could saturate from 32bit to 16 bit
    _matrix_multiplication(ctx, tl_A, tl_B, tl_C, psmode);
  }

  ctx->ops->parallel_disable(ctx);

  if (slice_idx[1] == slice_num[2] - 1) {
    // last one
    cvk_mg_shape_t shape;
    if (fmt != CVK_FMT_BF16) {
      tl_C->shape.n *= sizeof(int);  // partial sum for 32bit output
    }
    shape.row = tl_C->shape.n;
    shape.col = tl_C->shape.col;
    st_C.row = (uint32_t)slice_row_stride[1] * fmt_byte_sz;

    store_matrix(ctx, tl_C, slice_global_offset[2], shape, st_C, fmt);
    // DBG("local memory a/b/c is %u/%u/%u, store to slice_global_offset[2] %lu\n",
    //    tl_A->start_address, tl_B->start_address, tl_C->start_address, slice_global_offset[2]);
  }

  bool is_alloc_from_stack = true;

  if (cur == 0) {
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_next_B);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_B);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_next_A);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_A);
  } else {
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_B);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_next_B);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_A);
    bmk1880v2_lmem_free_prealloc_bf16_matrix(ctx, is_alloc_from_stack, tl_next_A);
  }

  return 0;
}

static void strategy_slice_on_multi_dimension(cvk_context_t *ctx, gaddr_t global_offset_A,
                                              gaddr_t global_offset_B, gaddr_t global_offset_C,
                                              size_t M, size_t N, size_t K, size_t *slice_num,
                                              cvk_fmt_t fmt) {
  size_t slice_row_stride[4] = {0, 0, 0, 0};
  slice_row_stride[0] = K;
  slice_row_stride[1] = N;

  gaddr_t slice_global_offset[3] = {0, 0, 0};
  gaddr_t slice_global_offset_next[4] = {0, 0, 0, 0};
  size_t slice_idx[3] = {0, 0, 0};
  size_t matrix_shape[3] = {0, 0, 0};
  size_t matrix_shape_next[3] = {0, 0, 0};
  int slice_idx_0 = slice_idx[0];
  int slice_idx_2 = slice_idx[2];
  int pack_shift = 0;
  for (slice_idx[0] = 0; slice_idx[0] < slice_num[0]; slice_idx[0]++) {
    matrix_shape[0] = M / slice_num[0] + (slice_idx[0] < M % slice_num[0]);
    matrix_shape_next[0] = M / slice_num[0] + (0 + slice_idx[0] < M % slice_num[0]);
    for (slice_idx[2] = 0; slice_idx[2] < slice_num[1]; slice_idx[2]++) {
      matrix_shape[2] = N / slice_num[1] + (slice_idx[2] < N % slice_num[1]);
      matrix_shape_next[2] = N / slice_num[1] + (0 + slice_idx[1] < N % slice_num[1]);
      for (slice_idx[1] = 0; slice_idx[1] < slice_num[2]; slice_idx[1]++) {
        matrix_shape[1] = K / slice_num[2] + (slice_idx[1] < K % slice_num[2]);
        matrix_shape_next[1] = K / slice_num[2] + (1 + slice_idx[1] < K % slice_num[2]);
        slice_global_offset[0] = get_slice_global_offset(
            global_offset_A, slice_idx[0], slice_idx[1], M, K, slice_num[0], slice_num[2], fmt);
        slice_global_offset[1] = get_slice_global_offset(
            global_offset_B, slice_idx[1], slice_idx[2], K, N, slice_num[2], slice_num[1], fmt);
        // the low 8-bits of C
        if (fmt == CVK_FMT_BF16) {
          slice_global_offset[2] = get_slice_global_offset(
              global_offset_C, slice_idx[0], slice_idx[2], M, N, slice_num[0], slice_num[1], fmt);
        } else {
          slice_global_offset[2] = get_slice_global_offset(
              global_offset_C, slice_idx_0, slice_idx_2, M, N, slice_num[0], slice_num[1], fmt);
          if (slice_idx[1] == slice_num[2] - 1) {
            // only shift in real store
            slice_global_offset[2] += pack_shift;
            // FIXME: slice N, currently ONLY slice M and K
            size_t row_num_A = matrix_shape[0];
            size_t col_num_B = matrix_shape[2];
            pack_shift += (row_num_A * col_num_B * sizeof(int));
          }
        }

        slice_global_offset_next[0] = get_slice_global_offset(
            global_offset_A, slice_idx[0], slice_idx[1] + 1, M, K, slice_num[0], slice_num[2], fmt);
        slice_global_offset_next[1] = get_slice_global_offset(
            global_offset_B, slice_idx[1] + 1, slice_idx[2], K, N, slice_num[2], slice_num[1], fmt);
        // DBG("=>(%s)slice_global_offset[0](%lu)/slice_global_offset[1](%lu) for slice_idx[1](%lu)
        // "
        //    "== 0\n"
        //    ", (%s)slice_global_offset[2](%lu) for slice_idx[1](%lu) == slice_num[2](%lu) - 1\n"
        //    ", (%s)(next)slice_global_offset_next[0](%lu)/slice_global_offset_next[1](%lu) for "
        //    "slice_num[2](%lu) - 1 > slice_idx[1](%lu)\n"
        //    "next ctrl:%s, store ctrl:%s\n",
        //    slice_idx[1] == 0 ? "en" : " ", slice_global_offset[0], slice_global_offset[1],
        //    slice_idx[1], slice_idx[1] == slice_num[2] - 1 ? "en" : " ", slice_global_offset[2],
        //    slice_idx[1], slice_num[2], slice_num[2] - 1 > slice_idx[1] ? "en" : " ",
        //    slice_global_offset_next[0], slice_global_offset_next[1], slice_num[2], slice_idx[1],
        //    (slice_idx[1] > 0) ? "CTRL_RA" : "CTRL_NULL",
        //    (slice_num[2] > 1) ? "CTRL_RA" : "CTRL_NULL");

        if (slice_idx[1] == 0) {
          strategy_slice_on_multidim_init(ctx, slice_global_offset, matrix_shape, slice_row_stride,
                                          fmt);
        }

        strategy_slice_on_multi_dimension_internal(ctx, slice_idx, slice_num, slice_global_offset,
                                                   matrix_shape, slice_global_offset_next,
                                                   matrix_shape_next, slice_row_stride, fmt);
      }
    }
  }
}

size_t *bmblas_gemm(cvk_context_t *ctx, size_t M, size_t N, size_t K, uint64_t gaddr_a,
                    uint64_t gaddr_b, uint64_t gaddr_c, cvk_fmt_t fmt) {
  size_t slice_num[3] = {1, 1, 1};
  ASSERT(slice_num[0] <= M && slice_num[0] >= 1);
  ASSERT(slice_num[1] <= N && slice_num[1] >= 1);
  ASSERT(slice_num[2] <= K && slice_num[2] >= 1);

  size_t strategy = get_slice_num(ctx, M, N, K, slice_num, fmt);
  // printf("strategy: %lu\n slice %lu %lu %lu\n", strategy, slice_num[0], slice_num[1],
  // slice_num[2]);

  switch (strategy) {
    case 0: {
      strategy_no_slice(ctx, M, N, K, gaddr_a, gaddr_b, gaddr_c, fmt);
    } break;
    case 1: {
      strategy_slice_on_M(ctx, M, N, K, gaddr_a, gaddr_b, gaddr_c, slice_num[0], fmt);
    } break;
    case 2: {
      strategy_slice_on_N(ctx, M, N, K, gaddr_a, gaddr_b, gaddr_c, slice_num[1], fmt);
    } break;
    case 3: {
      slice_split_strategy(ctx, M, N, K, slice_num, fmt);
      // printf("slice all, %lu %lu %lu\n", slice_num[0], slice_num[1], slice_num[2]);

      strategy_slice_on_multi_dimension(ctx, gaddr_a, gaddr_b, gaddr_c, M, N, K, slice_num, fmt);
    }
    default:
      break;
  }
  // 3 indicate M N K
  int slice_num_len = 4 * sizeof(size_t);
  size_t *_slice_num = (size_t *)malloc(slice_num_len);
  memcpy(_slice_num, slice_num, 3 * sizeof(size_t));
  _slice_num[3] = strategy;
  return _slice_num;
}

size_t *cvm_gemm(cvk_context_t *ctx, gaddr_t bottom_data_gaddr, gaddr_t weight_data_gaddr,
                 gaddr_t top_data_gaddr, int in_row, int in_col, int out_col, cvk_fmt_t fmt) {
  size_t *slice_num = NULL;
  if (0) {
    // backend impelement
    cvm_fc_forward_kernel(ctx, 0, bottom_data_gaddr, weight_data_gaddr, GADDR_INVALID,
                          top_data_gaddr, in_row, in_col, out_col, 0, 0, 0);
  } else {
    slice_num = bmblas_gemm(ctx, in_row, out_col, in_col, bottom_data_gaddr, weight_data_gaddr,
                            top_data_gaddr, fmt);
  }
  return slice_num;
}

int cvm_combin_gemm_i8(size_t *slice_num, uint8_t *i8_C, uint32_t *i32_C, int M, int N) {
  int bstride = M * N;
  int size = bstride;

  int strategy = slice_num[3];
  int chunks = slice_num[0] * slice_num[1] * slice_num[2];
  int chunk_size = M * N / chunks;
  size = chunk_size;
  bstride = chunk_size;
  if (strategy == 0 || strategy == 2) {
    // slice N
    int pack_shift = 0;
    int N_slice_cnt = 0;
    for (int tiling = 0; tiling < chunks; tiling++) {
      size_t N_slice = N / slice_num[1] + (tiling < (int)(N % slice_num[1]));
      chunk_size = N_slice * M;
      size = chunk_size;
      bstride = M * N;
      for (int m = 0; m < (int)M; m++) {
        for (int n = 0; n < (int)N_slice; n++) {
          int shift = N_slice_cnt + m * N + n;
          i32_C[shift] = (i8_C[shift + bstride * 0]) | (i8_C[shift + bstride * 1] << 8) |
                         (i8_C[shift + bstride * 2] << 16) | (i8_C[shift + bstride * 3] << 24);
        }
      }
      pack_shift += size;
      N_slice_cnt += N_slice;
    }
  } else if (strategy == 1) {
    int pack_shift = 0;
    for (int tiling = 0; tiling < chunks; tiling++) {
      size_t M_slice = M / slice_num[0] + (tiling < (int)(M % slice_num[0]));
      chunk_size = M_slice * N;
      size = chunk_size;
      bstride = chunk_size;
      for (int i = 0; i < size; i++) {
        i32_C[pack_shift + i] = (i8_C[pack_shift * sizeof(int) + i + bstride * 0]) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 1] << 8) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 2] << 16) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 3] << 24);
      }
      pack_shift += size;
    }
  } else if (strategy == 3) {
    // tiling all, it MUST tiling M/K ONLY
    // FIXME: tiling N
    int pack_shift = 0;
    for (int tiling = 0; tiling < (int)slice_num[0]; tiling++) {
      size_t M_slice = M / slice_num[0] + (tiling < (int)(M % slice_num[0]));
      int size = M_slice * N;
      int bstride = size;
      for (int i = 0; i < size; i++) {
        i32_C[pack_shift + i] = (i8_C[pack_shift * sizeof(int) + i + bstride * 0]) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 1] << 8) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 2] << 16) |
                                (i8_C[pack_shift * sizeof(int) + i + bstride * 3] << 24);
      }
      pack_shift += size;
    }
  }
  return 0;
}
