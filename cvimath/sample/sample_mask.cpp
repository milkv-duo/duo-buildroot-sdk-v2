// \file mask sample for gt(great than), ge(great equal), eq(equal), lt(less than), le(less equal)

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

// global variable for loop all test case
static enum CVM_MASK_TYPE mode;

// global structure for test
struct pattern {
  float *input;  // input
  float *ref;    // reference output
  int len;       // data lenth
#define HELP_LEN (10)
  char help[HELP_LEN];  // help message
};

// input
float cvm_mask_type_gt_0_input[] = {-1 * pow(2, -62), -0.003, -1.0, -100000, 0.000001, 1, 1000,
                                    pow(2, 62),       0};

// ref, 0 means false, 1 means true
float cvm_mask_type_gt_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 0};
float cvm_mask_type_ge_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 1};
float cvm_mask_type_eq_0_output[] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
float cvm_mask_type_lt_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 0};
float cvm_mask_type_le_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 1};

// size of input
int input_sz = sizeof(cvm_mask_type_gt_0_input) / sizeof(cvm_mask_type_gt_0_input[0]);

// init test case
static struct pattern patterns[] = {
    {cvm_mask_type_gt_0_input, cvm_mask_type_gt_0_output, input_sz, "gt test"},
    {cvm_mask_type_gt_0_input, cvm_mask_type_ge_0_output, input_sz, "ge test"},
    {cvm_mask_type_gt_0_input, cvm_mask_type_eq_0_output, input_sz, "eq test"},
    {cvm_mask_type_gt_0_input, cvm_mask_type_lt_0_output, input_sz, "lt test"},
    {cvm_mask_type_gt_0_input, cvm_mask_type_le_0_output, input_sz, "le test"},
};

static void testbench(CVI_RT_HANDLE *ctx, cvk_context_t *bmk) {
  // default test bf16 case
  cvk_fmt_t fmt = CVK_FMT_BF16;

  struct pattern *p = &patterns[mode];

  // alloc shape, align with \len
  uint32_t input_n = 1;
  uint32_t input_c = 1;
  uint32_t input_h = 1;
  uint32_t input_w = p->len;

  cvk_tl_shape_t ifmap_shape = {input_n, input_c, input_h, input_w};
  cvk_tl_shape_t ofmap_shape = ifmap_shape;

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  // unit size is 1 bytes, bf16 takes 2 bytes
  int data_type_size = 1;
  if (fmt == CVK_FMT_BF16) {
    data_type_size = 2;
  }

  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;

  // get table shape
  cvk_tl_shape_t table_shape;
  uint64_t table_bytesize = cvm_lut_tbl_bytesize(bmk, &table_shape, fmt);

  // alloc input/output tl
  cvk_tl_t *tl_ifmap = test_alloc_tl(bmk, ifmap_shape, fmt, CTRL_AL);
  cvk_tl_t *tl_ofmap_bf16 = test_alloc_tl(bmk, ofmap_shape, fmt, CTRL_AL);

  // alloc lookup table
  cvk_tl_t *tl_pos_neg_buf = test_alloc_tl(bmk, table_shape, fmt, CTRL_AL);
  cvk_tl_t *tl_0_idx_table = test_alloc_tl(bmk, table_shape, fmt, CTRL_AL);

  // alloc tmp tl
  cvk_tl_t *tl_buf = test_alloc_tl(bmk, ofmap_shape, fmt, CTRL_AL);
  cvk_tl_t *tl_buf2 = test_alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, CTRL_AL);
  cvk_tl_t *tl_buf4 = test_alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, CTRL_AL);

  // alloc data from ddr
  uint16_t *input_data = (uint16_t *)xmalloc(ifmap_bytesize);
  uint16_t *ref_data = (uint16_t *)xmalloc(ofmap_bytesize);
  uint16_t *table_data_atan_pos_neg = (uint16_t *)xmalloc(table_bytesize);
  uint16_t *idx_0_table_data = (uint16_t *)xmalloc(table_bytesize);

  // init lookup table data in ddr
  cvm_gen_0_tbl(idx_0_table_data, &table_shape);
  cvm_pos_neg_tbl(table_data_atan_pos_neg, &table_shape);

  // init input / output data in ddr
  for (uint32_t i = 0; i < ifmap_size; i++) {
    input_data[i] = convert_fp32_bf16(p->input[i]);
    ref_data[i] = convert_fp32_bf16(p->ref[i]);
  }

  // send ddr data to tl
  test_put_tensor_g2l_comp(ctx, bmk, tl_ifmap, (uint8_t *)input_data);
  test_put_tensor_g2l_comp(ctx, bmk, tl_pos_neg_buf, (uint8_t *)table_data_atan_pos_neg);
  test_put_tensor_g2l_comp(ctx, bmk, tl_0_idx_table, (uint8_t *)idx_0_table_data);

  // emit mask function
  cvm_emit_mask(bmk,
                tl_ifmap,                        // input
                tl_buf, tl_buf2, tl_buf4,        // tmp buffer
                tl_pos_neg_buf, tl_0_idx_table,  // lookup table
                tl_ofmap_bf16,                   // output
                fmt, mode);

  // submit descriptor
  test_submit_comp(ctx, bmk);

  // get data from tl
  uint16_t *ofmap_data = (uint16_t *)test_get_tensor_l2g_comp(ctx, bmk, tl_ofmap_bf16);

  // compare with reference
  for (uint32_t i = 0; i < ifmap_size; i++) {
    if (ref_data[i] != ofmap_data[i]) {
      fprintf(stderr, "comparing failed at mode (%s) output[%u] got %f(0x%x), ref %f(0x%x)\n",
              p->help, i, convert_bf16_fp32(ofmap_data[i]), ofmap_data[i],
              convert_bf16_fp32(ref_data[i]), ref_data[i]);
      // fail case
      exit(-1);
    }
  }

  // free resource from kernel
  free_tl(bmk, tl_buf4);
  free_tl(bmk, tl_buf2);
  free_tl(bmk, tl_buf);
  free_tl(bmk, tl_0_idx_table);
  free_tl(bmk, tl_pos_neg_buf);
  free_tl(bmk, tl_ofmap_bf16);
  free_tl(bmk, tl_ifmap);

  // free resource from heap
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

  // align kerenl rounding mode
  round_mode = set_store_feround();

  // init runtime / kerenl structure
  test_init(&ctx, &bmk);

  for (int i = CVM_MASK_TYPE_GT_0; i < CVM_MASK_MAX; i++) {
    mode = static_cast<enum CVM_MASK_TYPE>(i);
    struct pattern *p = &patterns[mode];
    printf("test %s...\n", p->help);
    testbench(&ctx, bmk);
  }

  // de-init runtime / kerenl structure
  test_exit(&ctx, bmk);

  // restore rounding mode
  restore_feround(round_mode);

  return 0;
}
