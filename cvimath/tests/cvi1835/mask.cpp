#include <cvimath_internal.h>
#include <test_cvikernel_util.h>

#define OUT
#define IN
#include <cfloat>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
//#define DBG

using namespace std;

/**
 * pre_data means we test fixed pattern, it should be same sa lut
 */
// enum TEST_MODE {
//  CVM_MASK_TYPE_GT_0 = 0,  // remain >  0
//  //CVM_MASK_TYPE_GE_0,      // remain >= 0
//  //CVM_MASK_TYPE_EQ_0,      // remain  = 0
//  //CVM_MASK_TYPE_LT_0,      // remain <  0
//  //CVM_MASK_TYPE_LE_0,      // remain <= 0
//  CVM_MASK_MAX
//};

enum CVM_MASK_TYPE mode;

struct pattern {
  float *input;
  float *ref;
  int len;
};
#define SIZEOF(x) (sizeof(x) / sizeof(x[0]))
float cvm_mask_type_gt_0_input[] = {-1 * pow(2, -62), -0.003, -1.0, -100000, 0.000001, 1, 1000,
                                    pow(2, 62),       0};

float cvm_mask_type_gt_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 0};
float cvm_mask_type_ge_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 1};
float cvm_mask_type_eq_0_output[] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
float cvm_mask_type_lt_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 0};
float cvm_mask_type_le_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 1};

int input_sz = sizeof(cvm_mask_type_gt_0_input) / sizeof(cvm_mask_type_gt_0_input[0]);

static struct pattern patterns[] = {
    {cvm_mask_type_gt_0_input, cvm_mask_type_gt_0_output, input_sz},
    {cvm_mask_type_gt_0_input, cvm_mask_type_ge_0_output, input_sz},
    {cvm_mask_type_gt_0_input, cvm_mask_type_eq_0_output, input_sz},
    {cvm_mask_type_gt_0_input, cvm_mask_type_lt_0_output, input_sz},
    {cvm_mask_type_gt_0_input, cvm_mask_type_le_0_output, input_sz},
};

static void testbench(CVI_RT_HANDLE *ctx, cvk_context_t *bmk) {
  cvk_fmt_t fmt = CVK_FMT_BF16;
  struct pattern *p = &patterns[mode];
  uint32_t input_n = 1;
  uint32_t input_c = 1;
  uint32_t input_h = 1;
  uint32_t input_w = p->len;

  cvk_tl_shape_t ifmap_shape = {input_n, input_c, input_h, input_w};
  cvk_tl_shape_t ofmap_shape = ifmap_shape;
  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  int data_type_size = bytesize_of_fmt(fmt);
  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;

  cvk_tl_shape_t table_shape;
  uint64_t table_bytesize = cvm_lut_tbl_bytesize(bmk, &table_shape, fmt);

  cvk_tl_t *tl_ifmap = test_alloc_tl(bmk, ifmap_shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_ofmap_bf16 = test_alloc_tl(bmk, ofmap_shape, fmt, /*align*/ 1);
  cvk_tl_t *out = tl_ofmap_bf16;
  cvk_tl_t *tl_pos_neg_buf = test_alloc_tl(bmk, table_shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_0_idx_table = test_alloc_tl(bmk, table_shape, fmt, /*align*/ 1);

  // temp buf
  cvk_tl_t *tl_buf = test_alloc_tl(bmk, ofmap_shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_buf2 = test_alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, /*align*/ 1);
  cvk_tl_t *tl_buf4 = test_alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, /*align*/ 1);

  uint16_t *input_data = (uint16_t *)xmalloc(ifmap_bytesize);
  uint16_t *ref_data = (uint16_t *)xmalloc(ofmap_bytesize);
  uint16_t *table_data_atan_pos_neg = (uint16_t *)xmalloc(table_bytesize);
  uint16_t *idx_0_table_data = (uint16_t *)xmalloc(table_bytesize);

  cvm_gen_0_tbl(idx_0_table_data, &table_shape);
  cvm_pos_neg_tbl(table_data_atan_pos_neg, &table_shape);

  for (uint32_t i = 0; i < ifmap_size; i++) {
    input_data[i] = convert_fp32_bf16(p->input[i]);
    ref_data[i] = convert_fp32_bf16(p->ref[i]);
  }

  test_put_tensor_g2l_comp(ctx, bmk, tl_ifmap, (uint8_t *)input_data);
  test_put_tensor_g2l_comp(ctx, bmk, tl_pos_neg_buf, (uint8_t *)table_data_atan_pos_neg);
  test_put_tensor_g2l_comp(ctx, bmk, tl_0_idx_table, (uint8_t *)idx_0_table_data);

  cvm_emit_mask(bmk, tl_ifmap, tl_buf, tl_buf2, tl_buf4, tl_pos_neg_buf, tl_0_idx_table, out, fmt,
                mode);

  test_submit_comp(ctx, bmk);

  uint16_t *ofmap_data = (uint16_t *)test_get_tensor_l2g_comp(ctx, bmk, out);

  for (uint32_t i = 0; i < ifmap_size; i++) {
    if (ref_data[i] != ofmap_data[i]) {
      fprintf(stderr,
              "comparing failed at mode %d ofmap_data[%u] got %f(0x%x), ref "
              "%f(0x%x)\n",
              mode, i, convert_bf16_fp32(ofmap_data[i]), ofmap_data[i],
              convert_bf16_fp32(ref_data[i]), ref_data[i]);
      exit(-1);
    }
  }
#if 0
  if (!is_close) {
    float input = convert_bf16_fp32(ifmap[i]);
      }
#endif
  free_tl(bmk, tl_buf4);
  free_tl(bmk, tl_buf2);
  free_tl(bmk, tl_buf);
  free_tl(bmk, tl_0_idx_table);
  free_tl(bmk, tl_pos_neg_buf);
  free_tl(bmk, tl_ofmap_bf16);
  free_tl(bmk, tl_ifmap);

  free(input_data);
  free(ref_data);
  free(ofmap_data);
  free(table_data_atan_pos_neg);
  free(idx_0_table_data);
}

int main() {
  CVI_RT_HANDLE ctx;
  cvk_context_t *bmk;
  int round_mode;

  round_mode = set_store_feround();

  test_init(&ctx, &bmk);

  for (int i = CVM_MASK_TYPE_GT_0; i < CVM_MASK_MAX; i++) {
    mode = static_cast<enum CVM_MASK_TYPE>(i);
    printf("test mode %d...\n", mode);
    testbench(&ctx, bmk);
  }

  test_exit(&ctx, bmk);
  restore_feround(round_mode);
  return 0;
}
