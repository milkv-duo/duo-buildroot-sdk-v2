#include "../1880v2_test_util.h"
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
//  BF16_MASK_TYPE_GT_0 = 0,  // remain >  0
//  //BF16_MASK_TYPE_GE_0,      // remain >= 0
//  //BF16_MASK_TYPE_EQ_0,      // remain  = 0
//  //BF16_MASK_TYPE_LT_0,      // remain <  0
//  //BF16_MASK_TYPE_LE_0,      // remain <= 0
//  BF16_MASK_MAX
//};

enum BF16_MASK_TYPE mode;

struct pattern {
  float *input;
  float *ref;
  int len;
};
#define SIZEOF(x) (sizeof(x) / sizeof(x[0]))
float bf16_mask_type_gt_0_input[] = {
    -1 * pow(2, -62), -0.003, -1.0, -100000, 0.000001, 1, 1000, pow(2, 62), 0};

float bf16_mask_type_gt_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 0};
float bf16_mask_type_ge_0_output[] = {0, 0, 0, 0, 1, 1, 1, 1, 1};
float bf16_mask_type_eq_0_output[] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
float bf16_mask_type_lt_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 0};
float bf16_mask_type_le_0_output[] = {1, 1, 1, 1, 0, 0, 0, 0, 1};

int input_sz =
    sizeof(bf16_mask_type_gt_0_input) / sizeof(bf16_mask_type_gt_0_input[0]);

static struct pattern patterns[] = {
    {bf16_mask_type_gt_0_input, bf16_mask_type_gt_0_output, input_sz},
    {bf16_mask_type_gt_0_input, bf16_mask_type_ge_0_output, input_sz},
    {bf16_mask_type_gt_0_input, bf16_mask_type_eq_0_output, input_sz},
    {bf16_mask_type_gt_0_input, bf16_mask_type_lt_0_output, input_sz},
    {bf16_mask_type_gt_0_input, bf16_mask_type_le_0_output, input_sz},
};

static void testbench(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk)
{
  fmt_t fmt = FMT_BF16;
  struct pattern *p = &patterns[mode];
  u32 input_n = 1;
  u32 input_c = 1;
  u32 input_h = 1;
  u32 input_w = p->len;

  tl_shape_t ifmap_shape = {input_n, input_c, input_h, input_w};
  tl_shape_t ofmap_shape = ifmap_shape;
  u64 ifmap_size = tl_shape_size(&ifmap_shape);
  u64 ofmap_size = tl_shape_size(&ofmap_shape);

  int data_type_size = bytesize_of_fmt(fmt);
  u64 ifmap_bytesize = ifmap_size * data_type_size;
  u64 ofmap_bytesize = ofmap_size * data_type_size;

  tl_shape_t table_shape;
  u64 table_bytesize = bf16_lut_tbl_bytesize(bmk, &table_shape, fmt);

  tl_t *tl_ifmap = alloc_tl(bmk, ifmap_shape, fmt, /*align*/1);
  tl_t *tl_ofmap_bf16 = alloc_tl(bmk, ofmap_shape, fmt, /*align*/1);
  tl_t *out = tl_ofmap_bf16;
  tl_t *tl_pos_neg_buf = alloc_tl(bmk, table_shape, fmt, /*align*/1);
  tl_t *tl_0_idx_table = alloc_tl(bmk, table_shape, fmt, /*align*/1);

  // temp buf
  tl_t *tl_buf = alloc_tl(bmk, ofmap_shape, fmt, /*align*/1);
  tl_t *tl_buf2 = alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, /*align*/1);
  tl_t *tl_buf4 = alloc_tl(bmk, tl_ofmap_bf16->shape, fmt, /*align*/1);

  u16 *input_data = (u16 *)xmalloc(ifmap_bytesize);
  u16 *ref_data = (u16 *)xmalloc(ofmap_bytesize);
  u16 *table_data_atan_pos_neg = (u16 *)xmalloc(table_bytesize);
  u16 *idx_0_table_data = (u16 *)xmalloc(table_bytesize);

  bf16_gen_0_tbl(idx_0_table_data, &table_shape);
  bf16_atan_pos_neg(table_data_atan_pos_neg, &table_shape);

  for (u32 i = 0; i < ifmap_size; i++) {
    input_data[i] = convert_fp32_bf16(p->input[i]);
    ref_data[i] = convert_fp32_bf16(p->ref[i]);
  }

  put_bf16_tensor_g2l(ctx, bmk, tl_ifmap, (u16 *)input_data, fmt);
  put_bf16_tensor_g2l(ctx, bmk, tl_pos_neg_buf, (u16 *)table_data_atan_pos_neg,
                      fmt);
  put_bf16_tensor_g2l(ctx, bmk, tl_0_idx_table, (u16 *)idx_0_table_data, fmt);

  bf16_emit_mask(bmk, tl_ifmap, tl_buf, tl_buf2, tl_buf4, tl_pos_neg_buf,
                 tl_0_idx_table, out, fmt, mode);

  test_submit(ctx);

  u16 *ofmap_data = (u16 *)get_bf16_tensor_l2g(ctx, bmk, out, out->fmt);

  for (u32 i = 0; i < ifmap_size; i++) {
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

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bmk;
  int round_mode;

  round_mode = set_store_feround();

  test_init(&ctx, &bmk);

  for (int i = BF16_MASK_TYPE_GT_0; i < BF16_MASK_MAX; i++) {
    mode = static_cast<enum BF16_MASK_TYPE>(i);
    printf("test mode %d...\n", mode);
    testbench(&ctx, bmk);
  }

  test_exit(&ctx);
  restore_feround(round_mode);
  return 0;
}
