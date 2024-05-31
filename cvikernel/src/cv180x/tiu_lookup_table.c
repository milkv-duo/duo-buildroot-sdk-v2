#include "cvkcv180x.h"

void cvkcv180x_tiu_lookup_table(
    cvk_context_t *ctx,
    const cvk_tiu_lookup_table_param_t *p)
{
  int8_t status = 0;
  uint32_t eu_num = ctx->info.eu_num;
  uint32_t npu_num = ctx->info.npu_num;

  status |= check_tiu_tensor_3(p->ofmap, p->ifmap, p->table);
  status |= check_stride_type_0(ctx, p->ofmap);
  status |= check_stride_type_0(ctx, p->ifmap);
  status |= check_stride_type_0(ctx, p->table);

  uint8_t is_bf16 = (p->ofmap->fmt == CVK_FMT_BF16 && p->ifmap->fmt == CVK_FMT_BF16);

  CHECK(status, p->table->shape.n == 1);
  CHECK(status, p->table->shape.c == npu_num);

  if (is_bf16) {
    CHECK(status, p->table->shape.h == 32);
    CHECK(status, p->table->shape.w == 8);
  }
  else {
    CHECK(status, p->table->shape.h == 16);
    CHECK(status, p->table->shape.w == 16);
  }

  CHECK(status, p->ifmap->start_address % eu_num == 0);
  CHECK(status, p->ofmap->start_address % eu_num == 0);
  CHECK(status, p->table->start_address % eu_num == 0);

  // fmt MUST be same under bf16
  if (p->ofmap->fmt == CVK_FMT_BF16) {
    CHECK(status, p->ifmap->fmt == CVK_FMT_BF16);
  }
  CHECK(status, p->ofmap->fmt == CVK_FMT_I8 || p->ofmap->fmt == CVK_FMT_U8 || p->ofmap->fmt == CVK_FMT_BF16);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  //reg.tens_lookup = 1;
  reg.tsk_opd_num = 2;
  reg.opt_shift_typ = 0;
  reg.opt_res_shift = 0;
  reg.opt_relu_typ = 0;
  reg.opd_typ = is_bf16;

  reg.res0_addr = p->ofmap->start_address;
  if (is_bf16) {
    reg.opt_res0_sign = 1;
    reg.opt_res0_seg = 1;
  }
  else {
    reg.opt_res0_sign = 0;
    reg.opt_res0_seg = 1;
  }

  // <! input / output shape SHOULD be same
  CHECK(status, p->ifmap->shape.n == p->ofmap->shape.n);
  CHECK(status, p->ifmap->shape.c == p->ofmap->shape.c);
  CHECK(status, p->ifmap->shape.h == p->ofmap->shape.h);
  CHECK(status, p->ifmap->shape.w == p->ofmap->shape.w);

  reg.res0_n = p->ifmap->shape.n;
  reg.res0_c = p->ifmap->shape.c;
  reg.res0_h = p->ifmap->shape.h;
  reg.res0_w = p->ifmap->shape.w;
  reg.short_res0_str = 0;

  reg.opd0_addr = p->ifmap->start_address;
  if (is_bf16) {
    reg.opt_opd0_sign = 1;
    reg.opt_opd0_seg = 1;
  }
  else {
    reg.opt_opd0_sign = 0;
    reg.opt_opd0_seg = 1;
  }
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.short_opd0_str = 0;

  reg.opd1_addr = p->table->start_address;
  if (is_bf16) {
    reg.opt_opd1_sign = 1;
    reg.opt_opd1_seg = 1;
  }
  else {
    reg.opt_opd1_sign = 0;
    reg.opt_opd1_seg = 1;
  }
  reg.opd1_n = p->table->shape.n;
  reg.opd1_c = p->table->shape.c;
  reg.opd1_h = p->table->shape.h;
  reg.opd1_w = p->table->shape.w;
  reg.short_opd1_str = 0;
  reg.tsk_eu_typ = 12; // 12 means lut
  if (is_bf16) {
    reg.opt_opd2_seg = 1; // hw check
    // dont care once short_xxx_str set to 0
  }

  reg.layer_info = p->layer_id;
  
  if (status) {
    printf("cvkcv180x tiu lookup: wrong parameter\n");
    return;
  }
  
  //trace_tiu_reg(&reg, __FUNCTION__);

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
