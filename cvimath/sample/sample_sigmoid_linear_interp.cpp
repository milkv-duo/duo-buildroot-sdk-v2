// \file implement activation function(sigmoid) by interpolation lookup table,
// please refer [here](https://en.wikipedia.org/wiki/Linear_interpolation) for more details

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

// ========== user config ============
#define MAX_ERROR (0.004)  // tolerance
// for current example, we quauntize data to -8 ~ +8
// range depend on ur activation
static int range_start = -8;
static int range_end = 8;
// ========== end of user config ============

// gen reference by cpu
static double _gen_sigmoid(float x) { return 1.0 / (1.0 + exp(-(x))); }

// gen reference
static void gen_ref(uint16_t *ofmap, uint16_t *ifmap, cvk_tl_shape_t ifmap_shape) {
  for (uint64_t i = 0; i < tl_shape_size(&ifmap_shape); i++) {
    ofmap[i] = convert_fp32_bf16(_gen_sigmoid(convert_bf16_fp32(ifmap[i])));
  }
}

// verify cpu data with tpu
static bool verify(uint16_t *ofmap_data, uint16_t *ref_data, uint64_t ofmap_size) {
  int count = 0;
  uint64_t size = ofmap_size;

  for (uint64_t i = 0; i < size; i++) {
    float got = convert_bf16_fp32(ofmap_data[i]);
    float exp = convert_bf16_fp32(ref_data[i]);

    if (fabs(got - exp) > MAX_ERROR) {
      fprintf(stderr,
              "[%d] comparing failed at ofmap_data[%u], got %x, exp %x, "
              "diff(%f - %f) is %f\n",
              count, (uint32_t)i, ofmap_data[i], ref_data[i], got, exp, fabs(got - exp));
      count++;
    }
  }

  // exit if fail
  if (count != 0) {
    printf("error count is %d\n", count);
    exit(-1);
  }

  return true;
}

// gen random input for test
static void gen_input(uint16_t *ifmap, uint64_t ifmap_size) {
  int table_hw = 256;
  for (uint64_t i = 0; i < ifmap_size; i++) {
    // input range is -8 ~ +8
    float input = ((int)i % 7) * (((int)i % 2) ? 1 : -1) + 0.03 + (i % table_hw) * 0.002;
    ifmap[i] = convert_fp32_bf16(input);
  }
}

// main code for test sigmoid interpolate implement by lookup table
static void testbench(CVI_RT_HANDLE *ctx, cvk_context_t *bmk) {
  // example for input tensor
  cvk_tl_shape_t ifmap_shape = {1, 32, 16, 16};
  cvk_fmt_t fmt = CVK_FMT_BF16;

  // get table / input shape
  cvk_tl_shape_t table_shape;
  cvm_table_shape(bmk, &table_shape);
  cvk_tl_shape_t ofmap_shape = ifmap_shape;

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t table_size = tl_shape_size(&table_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  // get table/input size
  int data_type_size = 1;
  if (fmt == CVK_FMT_BF16) {
    // bf16 takes 2 bytes
    data_type_size = 2;
  }

  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t table_bytesize = table_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;

  // alloc host memory
  uint16_t *ifmap = (uint16_t *)xmalloc(ifmap_bytesize);
  uint16_t *table_data = (uint16_t *)xmalloc(table_bytesize);
  uint16_t *table_data_slope = (uint16_t *)xmalloc(table_bytesize);
  uint16_t *ref_data = (uint16_t *)xmalloc(ofmap_bytesize);

  // gen input and assign data in host
  gen_input(ifmap, ifmap_size);

  // gen table, interpolation need 2 tables, one for lookup, another one is slope
  cvm_sigmoid_tbl(table_data, table_data_slope, &table_shape, range_start, range_end);

  // gen reference
  gen_ref(ref_data, ifmap, ofmap_shape);

  // alloc input / output / tmp / lookup table / slope table
  cvk_tl_t *tl_ifmap = test_alloc_tl(bmk, ifmap_shape, fmt, /*align*/ 1);
  cvk_tl_t *cvk_tl_table_answer = test_alloc_tl(bmk, table_shape, fmt, /*align*/ 1);
  cvk_tl_t *cvk_tl_table_answer_slope = test_alloc_tl(bmk, table_shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_buf = test_alloc_tl(bmk, ofmap_shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_ofmap_bf16 = test_alloc_tl(bmk, ofmap_shape, fmt, /*align*/ 1);

  // device memory load to local memory
  test_put_tensor_g2l_comp(ctx, bmk, tl_ifmap, (uint8_t *)ifmap);
  test_put_tensor_g2l_comp(ctx, bmk, cvk_tl_table_answer, (uint8_t *)table_data);
  test_put_tensor_g2l_comp(ctx, bmk, cvk_tl_table_answer_slope, (uint8_t *)table_data_slope);

  // get quantize(scale) value
  float scale = cvm_sigmoid_scale(range_start, range_end);

  // emit core function
  cvm_emit_sigmoid(bmk, tl_ifmap, tl_buf, cvk_tl_table_answer, cvk_tl_table_answer_slope,
                   tl_ofmap_bf16, scale);

  // get result from device to host
  uint16_t *ofmap_data = (uint16_t *)test_get_tensor_l2g_comp(ctx, bmk, tl_ofmap_bf16);

  // verify data with tolerance
  verify(ofmap_data, ref_data, ofmap_size);

  // release device memory in revert order
  free_tl(bmk, tl_ofmap_bf16);
  free_tl(bmk, tl_buf);
  free_tl(bmk, cvk_tl_table_answer_slope);
  free_tl(bmk, cvk_tl_table_answer);
  free_tl(bmk, tl_ifmap);

  // release host memory
  free(ifmap);
  free(table_data);
  free(table_data_slope);
  free(ref_data);
  free(ofmap_data);
}

int main() {
  CVI_RT_HANDLE ctx;
  cvk_context_t *bmk;
  int round_mode;

  round_mode = set_store_feround();

  // init runtime / kerenl structure
  test_init(&ctx, &bmk);

  // emit test case
  testbench(&ctx, bmk);

  // de-init runtime / kerenl structure
  test_exit(&ctx, bmk);

  // restore rounding
  restore_feround(round_mode);

  return 0;
}
