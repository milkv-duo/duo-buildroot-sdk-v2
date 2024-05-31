// \file sample for set value by mask, plz refer \cvimath_internal.h for more details

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

#include <sys/time.h>  // int gettimeofday
#include <time.h>      /* clock_t, clock, CLOCKS_PER_SEC */

#define DEBUG 1  // < 0 is disable debug
#define debug_print(fmt, ...)                     \
  do {                                            \
    if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

int flip = 0;
struct testbench {
  char *name;
  int (*cvm_run)(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2, cvk_tl_t *tl_buf,
                 cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl, cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias,
                 uint8_t threshold, uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap);
  void (*ref)(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask, uint8_t *pNewY, uint8_t *pY,
              uint8_t *g_update_tbl, uint8_t threshold, uint8_t w1, uint8_t w2);
  uint8_t threshold;
  uint8_t w1;
  uint8_t w2;
};

static void init_kernel(uint8_t *kernel_data, uint64_t kernel_size, int8_t val) {
  int8_t *kernel_data_i8 = (int8_t *)kernel_data;
  for (uint64_t i = 0; i < kernel_size; i++) {
    kernel_data_i8[i] = val;
  }
}

static void init_bias(uint8_t *bias_data, uint64_t bias_size, int16_t val) {
  int c = bias_size / 2;

  for (int i = 0; i < c; i++) {
    bias_data[i] = val & 0xff;
    bias_data[i + c] = (val >> 8) & 0xff;
  }
}

static void init_input_2(uint8_t *input_data, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    input_data[i] = i * 2 * (i % 3 ? -1 : 1);
  }
}

static void init_input_3(uint8_t *input_data, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    input_data[i] = i * 3;
  }
}

static void init_mask(uint8_t *mask, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    mask[i] = i % 2;
  }
}

static void init_update_tbl(uint8_t *update_tbl, uint64_t ifmap_size) {
  int8_t *update_tbl_i8 = (int8_t *)update_tbl;
  for (uint64_t i = 0; i < ifmap_size; i++) {
    update_tbl_i8[i] = i * (i % 2 ? -1 : 1);
  }
}

static void init_ref(uint8_t *ref_data, uint64_t ofmap_size) {
  for (uint64_t i = 0; i < ofmap_size; i++) {
    ref_data[i] = -1 * i;
    // ref_data[i] = 3 * i;
  }
}

static void set_image_by_u8mask(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask,
                                uint8_t *pNewY, uint8_t *pY, uint8_t *g_update_tbl,
                                uint8_t threshold, uint8_t w1, uint8_t w2) {
  (void)pY;
  (void)g_update_tbl;
  (void)threshold;
  (void)w1;
  (void)w2;

  for (size_t i = 0; i < ifmap_size; i++) {
    if (mask[i]) {
      ref_data[i] = pNewY[i];
    }
  }
}

static void set_image_by_two_info_i8(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask,
                                     uint8_t *pNewY, uint8_t *pY, uint8_t *g_update_tbl,
                                     uint8_t threshold, uint8_t w1, uint8_t w2) {
  (void)pY;
  (void)w1;
  (void)w2;
  int8_t *g_update_tbl_i8 = (int8_t *)g_update_tbl;

  for (size_t i = 0; i < ifmap_size; i++) {
    if (mask[i] && (g_update_tbl_i8[i] < threshold)) {
      ref_data[i] = pNewY[i];
    }
  }
}

static void gen_image_diff(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask, uint8_t *pNewY,
                           uint8_t *pY, uint8_t *g_update_tbl, uint8_t threshold, uint8_t w1,
                           uint8_t w2) {
  (void)mask;
  (void)w1;
  (void)w2;
  (void)g_update_tbl;
  (void)threshold;

  for (size_t i = 0; i < ifmap_size; i++) {
    ref_data[i] = abs(pNewY[i] - pY[i]);
  }
}

static void update_tbl_by_threshold(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask,
                                    uint8_t *pNewY, uint8_t *pY, uint8_t *g_update_tbl,
                                    uint8_t threshold, uint8_t w1, uint8_t w2) {
  (void)pNewY;
  (void)pY;
  (void)g_update_tbl;
  (void)mask;
  (void)w2;
  int8_t *ref_data_i8 = (int8_t *)ref_data;  // output is i8

  for (size_t i = 0; i < ifmap_size; i++) {
    mask[i] = 0;
  }

  for (size_t i = 0; i < ifmap_size; i++) {
    int8_t old = ref_data_i8[i];
    if (g_update_tbl[i] < threshold) {
      ref_data_i8[i] = (ref_data_i8[i] < w1) ? 0 : (ref_data_i8[i] - 1);
    } else {
      if (old != 127) {
        // saturate it
        ref_data_i8[i]++;
      }
      mask[i] = 1;
    }
  }
}

static void set_image_by_two_info_u8(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask,
                                     uint8_t *pNewY, uint8_t *pY, uint8_t *g_update_tbl,
                                     uint8_t threshold, uint8_t w1, uint8_t w2) {
  (void)pY;
  (void)mask;
  (void)w1;
  (void)w2;
  // int8_t* g_update_tbl_i8 = (int8_t*)g_update_tbl;

  for (size_t i = 0; i < ifmap_size; i++) {
    if (g_update_tbl[i] >= threshold) {
      ref_data[i] = pNewY[i];
    }
  }
}

static void blend_image_by_tbl(uint8_t *ref_data, uint64_t ifmap_size, uint8_t *mask,
                               uint8_t *pNewY, uint8_t *pY, uint8_t *g_update_tbl,
                               uint8_t threshold, uint8_t w1, uint8_t w2) {
  (void)mask;
  (void)pY;
  int8_t *g_update_tbl_i8 = (int8_t *)g_update_tbl;
  for (size_t i = 0; i < ifmap_size; i++) {
    if (g_update_tbl_i8[i] > threshold) {
      ref_data[i] = (w1 * ref_data[i] + w2 * pNewY[i]) >> 8;
    }
  }
}

static int _cvm_set_image_by_u8mask(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                                    cvk_tl_t *tl_buf, cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                                    cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias, uint8_t threshold,
                                    uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)tl_update_tbl;
  (void)threshold;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;
  (void)tl_buf;

  return cvm_set_image_by_u8mask(ctx, tl_ifmap, tl_buf, tl_mask, tl_ofmap);
}

static int _cvm_set_image_by_u8mask_dp(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                                       cvk_tl_t *tl_buf, cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                                       cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias, uint8_t threshold,
                                       uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)tl_update_tbl;
  (void)threshold;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;
  (void)tl_buf;

  return cvm_set_image_by_u8mask_dp(ctx, tl_ifmap, tl_mask, tl_kernel, tl_bias, tl_ofmap);
}

static int _cvm_set_image_by_two_info_i8(cvk_context_t *ctx, cvk_tl_t *tl_ifmap,
                                         cvk_tl_t *tl_ifmap2, cvk_tl_t *tl_buf, cvk_tl_t *tl_mask,
                                         cvk_tl_t *tl_update_tbl, cvk_tl_t *tl_kernel,
                                         cvk_tl_t *tl_bias, uint8_t threshold, uint8_t w1,
                                         uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)threshold;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;

  // tl_ifmap2 as buf
  return cvm_set_image_by_two_info_i8(ctx, tl_ifmap, tl_buf, tl_mask, tl_update_tbl, threshold,
                                      tl_ofmap);
}

static int _cvm_set_image_by_two_info_i8_dp(cvk_context_t *ctx, cvk_tl_t *tl_ifmap,
                                            cvk_tl_t *tl_ifmap2, cvk_tl_t *tl_buf,
                                            cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                                            cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias,
                                            uint8_t threshold, uint8_t w1, uint8_t w2,
                                            cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)threshold;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)threshold;
  (void)tl_buf;

  return cvm_set_image_by_two_info_i8_dp(ctx, tl_ifmap, tl_kernel, tl_mask, tl_update_tbl, tl_bias,
                                         tl_ofmap);
}

static int _cvm_gen_image_diff(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                               cvk_tl_t *tl_buf, cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                               cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias, uint8_t threshold,
                               uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_mask;
  (void)tl_buf;
  (void)tl_update_tbl;
  (void)threshold;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;

  // tl_mask as buffer
  return cvm_gen_image_diff(ctx, tl_ifmap, tl_ifmap2, tl_mask, tl_buf, tl_ofmap);
}

static int _cvm_update_tbl_by_threshold(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                                        cvk_tl_t *tl_buf, cvk_tl_t *tl_mask,
                                        cvk_tl_t *tl_update_tbl, cvk_tl_t *tl_kernel,
                                        cvk_tl_t *tl_bias, uint8_t threshold, uint8_t w1,
                                        uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;

  // w1 as threshold_b, tl_ifmap/tl_ifmap2 as buf
  return cvm_update_tbl_by_threshold(ctx, tl_mask, tl_ifmap, tl_ifmap2, tl_buf, tl_update_tbl,
                                     threshold, w1, tl_ofmap);
}

static int _cvm_set_image_by_two_info_u8(cvk_context_t *ctx, cvk_tl_t *tl_ifmap,
                                         cvk_tl_t *tl_ifmap2, cvk_tl_t *tl_buf, cvk_tl_t *tl_mask,
                                         cvk_tl_t *tl_update_tbl, cvk_tl_t *tl_kernel,
                                         cvk_tl_t *tl_bias, uint8_t threshold, uint8_t w1,
                                         uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)tl_mask;
  (void)w1;
  (void)w2;
  (void)tl_kernel;
  (void)tl_bias;

  // tl_ifmap2 as buf
  return cvm_set_image_by_two_info_u8(ctx, tl_ifmap, tl_ifmap2, tl_buf, tl_update_tbl, threshold,
                                      tl_ofmap);
}

static int _cvm_blend_image_by_tbl(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                                   cvk_tl_t *tl_buf, cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                                   cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias, uint8_t threshold,
                                   uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap) {
  (void)tl_ifmap2;
  (void)tl_kernel;
  (void)tl_bias;
  // tl_mask as buf
  return cvm_blend_image_by_tbl(ctx, tl_ifmap, tl_mask, tl_buf, tl_update_tbl, threshold, w1, w2,
                                tl_ofmap);
}

struct testbench testbenchs[] = {
    {(char *)"set_image_by_two_info_i8_dp", _cvm_set_image_by_two_info_i8_dp,
     set_image_by_two_info_i8, 2, 2, 3},
    {(char *)"set_image_by_u8mask_dp", _cvm_set_image_by_u8mask_dp, set_image_by_u8mask, 10, 2, 3},

    {(char *)"set_image_by_u8mask", _cvm_set_image_by_u8mask, set_image_by_u8mask, 10, 2, 3},
    {(char *)"set_image_by_two_info_i8", _cvm_set_image_by_two_info_i8, set_image_by_two_info_i8, 2,
     2, 3},
    {(char *)"update_tbl_by_threshold", _cvm_update_tbl_by_threshold, update_tbl_by_threshold, 15,
     12, 3},
    {(char *)"gen_image_diff", _cvm_gen_image_diff, gen_image_diff, 10, 2, 3},
    {(char *)"set_image_by_two_info_u8", _cvm_set_image_by_two_info_u8, set_image_by_two_info_u8,
     40, 2, 3},
    {(char *)"blend_image_by_tbl", _cvm_blend_image_by_tbl, blend_image_by_tbl, 6, 2, 3},
};

static void load(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap2,
                 uint8_t *input_ifmap2, cvk_tl_t *tl_ifmap3, uint8_t *input_ifmap3,
                 cvk_tl_t *tl_ofmap, uint8_t *input_ofmap, cvk_tl_t *tl_mask, uint8_t *input_mask,
                 cvk_tl_t *tl_update_tbl, uint8_t *input_update_tbl) {
  // send device memory to sram
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_ifmap2, input_ifmap2);
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_ifmap3, input_ifmap3);
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_mask, input_mask);
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_update_tbl, input_update_tbl);
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_ofmap, input_ofmap);
}

static void store(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, char *name, cvk_tl_t *tl_ofmap,
                  uint8_t *output_ofmap, cvk_tl_t *tl_mask, uint8_t *output_mask, int sz) {
  uint8_t *ofmap_data = (uint8_t *)test_get_tensor_l2g_comp(rt_ctx, cvk_ctx, tl_ofmap);

  // NOTICE: heavy copy
  memcpy(output_ofmap, ofmap_data, sz);

  free(ofmap_data);

  if (!strcmp(name, "update_tbl_by_threshold")) {
    uint8_t *mask_data = (uint8_t *)test_get_tensor_l2g_comp(rt_ctx, cvk_ctx, tl_mask);
    memcpy(output_mask, mask_data, sz);
    free(mask_data);
  }
}

static void testbench(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, cvk_tg_shape_t *tg_shape,
                      int testcase_idx, int is_pingpong = false) {
  // for calculate size we need in host
  cvk_tl_shape_t ifmap_shape = {tg_shape->n, tg_shape->c, tg_shape->h, tg_shape->w};
  cvk_tl_shape_t ofmap_shape = ifmap_shape;

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  // unit size is 1 bytes
  int data_type_size = 1;

  // get input/output size
  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;

  // alloc on ddr
  // uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *input_data2 = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *input_data3 = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *mask = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *update_tbl = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *_ref_data = (uint8_t *)xmalloc(ofmap_bytesize);
  uint8_t *ref_data = (uint8_t *)xmalloc(ofmap_bytesize);
  uint8_t *tpu_output_data = (uint8_t *)xmalloc(ofmap_bytesize);
  uint8_t *tpu_output_mask = (uint8_t *)xmalloc(ofmap_bytesize);

  // init input / output data in ddr
  uint8_t threshold, w1, w2;
  threshold = testbenchs[testcase_idx].threshold;
  w1 = testbenchs[testcase_idx].w1;
  w2 = testbenchs[testcase_idx].w2;
  init_input_2(input_data2, ifmap_size);
  init_input_3(input_data3, ifmap_size);
  // init_input(input_data2, ifmap_size);
  // init_input(input_data3, ifmap_size);
  init_mask(mask, ifmap_size);
  init_update_tbl(update_tbl, ifmap_size);
  init_ref(ref_data, ofmap_size);

  // keep org output
  memcpy(_ref_data, ref_data, ofmap_bytesize);

  testbenchs[testcase_idx].ref(ref_data, ofmap_size, mask, input_data2, input_data3, update_tbl,
                               threshold, w1, w2);

  int tiles = std::ceil(ifmap_shape.c / (float)cvk_ctx->info.npu_num);

  ifmap_shape.c = ifmap_shape.c / tiles;

  cvk_tl_shape_t kernel_shape = ifmap_shape;
  kernel_shape.h = 1;
  kernel_shape.w = 1;

  cvk_tl_shape_t bias_shape = ifmap_shape;
  bias_shape.h = 1;
  bias_shape.w = 1;
  bias_shape.n = 2;

  uint64_t kernel_size = tl_shape_size(&kernel_shape);
  uint64_t bias_size = tl_shape_size(&bias_shape);
  uint64_t kernel_bytesize = kernel_size * data_type_size;
  uint64_t bias_bytesize = bias_size * data_type_size;
  uint8_t *kernel_data = (uint8_t *)xmalloc(kernel_bytesize);
  uint8_t *bias_data = (uint8_t *)xmalloc(bias_bytesize);

  // NOTICE: must init with it
  init_kernel(kernel_data, kernel_size, -1);
  init_bias(bias_data, bias_size, 1);

  if (!strcmp(testbenchs[testcase_idx].name, "set_image_by_two_info_i8_dp")) {
    init_kernel(kernel_data, kernel_size, 1);
    init_bias(bias_data, bias_size, -1 * threshold);
  }

  if (is_pingpong) {
    // quirk that we tile h for easy implemenetation
    ifmap_shape.h /= 2;
    tiles *= 2;
  }

  // sync input/output
  ofmap_shape = ifmap_shape;

  // NOTICE: dont care batch
  int shape_sz = ifmap_shape.c * ifmap_shape.h * ifmap_shape.w;

  // alloc on sram, just once
  cvk_fmt_t fmt = CVK_FMT_U8;  // for mac used
  int eu_align = 1;            // dont care
  cvk_tl_t *tl_ifmap2[2] = {NULL, NULL};
  cvk_tl_t *tl_ifmap3[2] = {NULL, NULL};
  cvk_tl_t *tl_ofmap[2] = {NULL, NULL};
  cvk_tl_t *tl_mask[2] = {NULL, NULL};
  cvk_tl_t *tl_update_tbl[2] = {NULL, NULL};
  // must place last for high part of 'mac'
  cvk_tl_t *tl_buf[2] = {NULL, NULL};
  cvk_tl_t *tl_kernel, *tl_bias;

  // alloc sram
  tl_kernel = test_alloc_tl(cvk_ctx, kernel_shape, CVK_FMT_I8, eu_align);
  tl_bias = test_alloc_tl(cvk_ctx, bias_shape, CVK_FMT_I8, /*eu_align=*/0);

  int alloc_nr = is_pingpong ? 2 : 1;
  for (int i = 0; i < alloc_nr; i++) {
    tl_ifmap2[i] = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
    tl_ifmap3[i] = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
    tl_ofmap[i] = test_alloc_tl(cvk_ctx, ofmap_shape, fmt, eu_align);
    tl_mask[i] = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
    tl_update_tbl[i] = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
    // must place last for high part of 'mac'
    tl_buf[i] = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
  }

  // NOTICE: consider residual
  int load_offset = 0;
  int store_offset = 0;
  int ret;
  int curr = flip;
  long elapsed;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  if (!is_pingpong) {
    int off = 0;
    for (int i = 0; i < tiles; i++) {
      // NOTICE: load each loop
      test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_kernel, kernel_data);
      test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_bias, bias_data);

      load(rt_ctx, cvk_ctx, tl_ifmap2[curr], input_data2 + off, tl_ifmap3[curr], input_data3 + off,
           tl_ofmap[curr], _ref_data + off, tl_mask[curr], mask + off, tl_update_tbl[curr],
           update_tbl + off);

      int ret = testbenchs[testcase_idx].cvm_run(
          cvk_ctx, tl_ifmap2[curr], tl_ifmap3[curr], tl_buf[curr], tl_mask[curr],
          tl_update_tbl[curr], tl_kernel, tl_bias, threshold, w1, w2, tl_ofmap[curr]);

      if (ret) {
        fflush(stderr);
        printf("%s", "generate commands fail, return\n");
        exit(-1);
      }

      store(rt_ctx, cvk_ctx, testbenchs[testcase_idx].name, tl_ofmap[curr], tpu_output_data + off,
            tl_mask[curr], tpu_output_mask + off, shape_sz);

      off += shape_sz;
    }
  } else {
    // TODO: not load at once
    int operand_num = 1;
    int input_flip = 0;
    int output_flip = 0;
    for (int i = 0; i < tiles + 2; i++) {
      cvk_ctx->ops->parallel_enable(cvk_ctx);
      // NOTICE: load each loop
      test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_kernel, kernel_data);
      test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_bias, bias_data);

      // send device memory to sram
      if ((i - 2) >= 0 && (i - 2) % operand_num == operand_num - 1) {
        int curr = 1 - output_flip;
        store(rt_ctx, cvk_ctx, testbenchs[testcase_idx].name, tl_ofmap[curr],
              tpu_output_data + store_offset, tl_mask[curr], tpu_output_mask + store_offset,
              shape_sz);
        store_offset += shape_sz;
      }

      if (i - 1 >= 0 && i - 1 < tiles) {
        // get data from tl
        int curr = 1 - input_flip;
        // prepare command buffer
        ret = testbenchs[testcase_idx].cvm_run(
            cvk_ctx, tl_ifmap2[curr], tl_ifmap3[curr], tl_buf[curr], tl_mask[curr],
            tl_update_tbl[curr], tl_kernel, tl_bias, threshold, w1, w2, tl_ofmap[curr]);

        if (ret) {
          fflush(stderr);
          printf("%s", "generate commands fail, return\n");
          exit(-1);
        }
        output_flip = 1 - output_flip;
      }

      if (i < tiles) {
        load(rt_ctx, cvk_ctx, tl_ifmap2[input_flip], input_data2 + load_offset,
             tl_ifmap3[input_flip], input_data3 + load_offset, tl_ofmap[input_flip],
             _ref_data + load_offset, tl_mask[input_flip], mask + load_offset,
             tl_update_tbl[input_flip], update_tbl + load_offset);
        load_offset += shape_sz;
        input_flip = 1 - input_flip;
      }
      cvk_ctx->ops->parallel_disable(cvk_ctx);
    }
  }

  // submit descriptor
  test_submit_comp(rt_ctx, cvk_ctx);

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  // compare with reference with byte
  debug_print("%s comparing...", testbenchs[testcase_idx].name);
  for (uint32_t i = 0; i < (uint32_t)ofmap_bytesize; i++) {
    if (ref_data[i] != tpu_output_data[i]) {
      debug_print("comparing failed output[%u] got %u, ref %u\n", i, tpu_output_data[i],
                  ref_data[i]);
      // fail case
      fflush(stderr);
      exit(-1);
    }
  }

  // compare another export information
  if (!strcmp(testbenchs[testcase_idx].name, "update_tbl_by_threshold")) {
    for (uint32_t i = 0; i < (uint32_t)shape_sz; i++) {
      if (mask[i] != tpu_output_mask[i]) {
        debug_print("comparing mask failed output[%u] got %u, ref %u\n", i, tpu_output_mask[i],
                    mask[i]);
        // fail case
        fflush(stderr);
        exit(-1);
      }
    }
  }

  if (tiles == 1) {
    debug_print("%s", " pass\n");
  } else {
    // get elapsed time
    debug_print("(takes %ld us)\n", elapsed);
  }

  // free resource from tpu memory
  for (int i = alloc_nr - 1; i >= 0; --i) {
    free_tl(cvk_ctx, tl_buf[i]);
    free_tl(cvk_ctx, tl_update_tbl[i]);
    free_tl(cvk_ctx, tl_mask[i]);
    free_tl(cvk_ctx, tl_ofmap[i]);
    free_tl(cvk_ctx, tl_ifmap3[i]);
    free_tl(cvk_ctx, tl_ifmap2[i]);
  }
  free_tl(cvk_ctx, tl_bias);
  free_tl(cvk_ctx, tl_kernel);

  // free resource from host memory
  // free(input_data);
  free(ref_data);
  free(tpu_output_data);
  free(tpu_output_mask);
  free(input_data2);
  free(input_data3);
  free(mask);
  free(update_tbl);
  free(_ref_data);
  free(kernel_data);
  free(bias_data);
}

int main() {
  CVI_RT_HANDLE rt_ctx;
  cvk_context_t *cvk_ctx;

  // init runtime / kerenl structure
  test_init(&rt_ctx, &cvk_ctx);

  cvk_tg_shape_t tg_shape = {1, 20, 3, 4};

  // run test
  int testbench_nr = sizeof(testbenchs) / sizeof(testbenchs[0]);

  for (int i = 0; i < testbench_nr; i++) {
    testbench(&rt_ctx, cvk_ctx, &tg_shape, i);
  }
#if 1

  // run test without ping-pong
  tg_shape = {1, 128, 340, 16};

  printf("[heavy data] w/o ping pong\n");

  // NOTICE: only check c
  int tiles = std::ceil(tg_shape.c / (float)cvk_ctx->info.npu_num);
  if (tg_shape.c > cvk_ctx->info.npu_num) {
    debug_print("tile nr %d channel base one npu nr %d\n", tiles, cvk_ctx->info.npu_num);
  }

  for (int i = 0; i < testbench_nr; i++) {
    testbench(&rt_ctx, cvk_ctx, &tg_shape, i);
  }

  tg_shape = {1, 128, 340, 16};
  printf("[heavy data] w/ ping pong\n");
  for (int i = 0; i < testbench_nr; i++) {
    testbench(&rt_ctx, cvk_ctx, &tg_shape, i, /*is_pingpong=*/true);
  }
#endif
  // de-init runtime / kerenl structure
  test_exit(&rt_ctx, cvk_ctx);

  printf("all pass\n");

  return 0;
}
