#include "cvkcv181x.h"

typedef cvk_tiu_convolution_param_t param_t;

static int can_do_double_conv(cvk_context_t *ctx, const param_t *p)
{
  if ((p->ifmap->start_address % ctx->info.lmem_size) % 2 == 0 &&
      p->ifmap->shape.c % 2 == 0 &&
      p->ifmap->shape.c >= 4 &&
      p->weight->start_address % 2 == 0)
    return 1;

  return 0;
}

static int8_t check_conv_param(cvk_context_t *ctx, const param_t *p)
{
  int8_t status = 0;
  uint32_t eu_num = ctx->info.eu_num;

  status |= check_tiu_tensor_3(p->ifmap, p->ofmap, p->weight);
  status |= check_stride_type_0(ctx, p->ifmap);

  CHECK(status, (p->ofmap->stride.n % eu_num) == 0);
  CHECK(status, p->ifmap->start_address % eu_num == 0);
  CHECK(status, p->ofmap->start_address % eu_num == 0);
  CHECK(status, p->ifmap->shape.n == p->ofmap->shape.n);
  CHECK(status, !(p->ifmap->shape.h == 1 && p->ins_h > 0));
  CHECK(status, p->weight->shape.n == p->ifmap->shape.c);
  CHECK(status, p->weight->shape.c == p->ofmap->shape.c);

  if (p->chl_quan_param) {
    status |= check_tiu_tensor(p->chl_quan_param);
    status |= check_stride_type_2(ctx, p->chl_quan_param);
    CHECK(status, p->chl_quan_param->start_address % eu_num == 0);
  }
  if (can_do_double_conv(ctx, p)) {
    uint32_t lmem_i = p->ifmap->start_address % ctx->info.lmem_size;
    CHECK(status, lmem_i % 2 == 0);
    CHECK(status, p->ifmap->shape.c % 2 == 0);
    CHECK(status, p->ifmap->shape.c >= 4); /* Otherwise performance will suffer */
    CHECK(status, p->weight->start_address % 2 == 0);
  }
  if(p->ps32_mode & 0x2)
  {
    CHECK(status, !p->relu_enable);
    CHECK(status, !p->has_bias);

    CHECK(status, p->cmd_pre_exe <= 1);
  }
  CHECK(status, p->stride_h < 32 && p->stride_h > 0);
  CHECK(status, p->stride_w < 32 && p->stride_w > 0);
  CHECK(status, p->pad_top < 16);
  CHECK(status, p->pad_bottom < 16);
  CHECK(status, p->pad_left < 16);
  CHECK(status, p->pad_right < 16);
  CHECK(status, p->ins_h < 15);
  CHECK(status, p->ins_last_h < 15);
  CHECK(status, p->ins_w < 15);
  CHECK(status, p->ins_last_w < 15);
  CHECK(status, p->dilation_h >= 1);
  CHECK(status, p->dilation_w >= 1);
  CHECK(status, p->relu_enable == 0 || p->relu_enable == 1);

  return status;
}

void cvkcv181x_tiu_convolution(cvk_context_t *ctx, const param_t *p)
{
  int8_t status = 0;

  status |= check_conv_param(ctx, p);

  int opd0_sign = tensor_is_signed(p->ifmap);
  int opd1_sign = tensor_is_signed(p->weight);
  int arith_shift = opd0_sign || opd1_sign;

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_CONV_FIX8B;
  reg.opt_shift_typ = arith_shift;
  reg.opt_relu_typ = !!(p->relu_enable);
  reg.tsk_opd_num = 2;

  /*always automatically enabel double conv at those situations*/
  if (can_do_double_conv(ctx, p))
    reg.double_conv = 1;

  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->ofmap);
  reg.opt_res0_seg = 1;
  reg.res0_n = p->ofmap->shape.n;
  reg.res0_c = p->ofmap->shape.c;
  reg.res0_h = p->ofmap->shape.h;
  reg.res0_w = p->ofmap->shape.w;
  reg.res0_n_str = p->ofmap->stride.n;
  reg.res0_c_str = p->ofmap->stride.c;
  reg.res0_h_str = p->ofmap->stride.h;
  reg.res0_w_str = p->ofmap->stride.w;
  reg.short_res0_str = 3;   // Manual instead of h/w
  reg.ps32_md = p->ps32_mode;
  if (p->ps32_mode > 0) {
    reg.res0_b_str = p->ofmap->shape.n * p->ofmap->stride.n;

    // Per-channel parameter does not has right shift (default is 10).
    // Set zero.
    reg.opt_res_shift = 0;
  }

  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_seg = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.opd0_ins_val = (uint32_t)p->ins_val;
  reg.short_opd0_str = 0;
  reg.conv_opd0_up_pad = p->pad_top;
  reg.conv_opd0_dn_pad = p->pad_bottom;
  reg.conv_opd0_lf_pad = p->pad_left;
  reg.conv_opd0_rt_pad = p->pad_right;
  reg.conv_opd0_x_ins0 = p->ins_w;
  reg.conv_opd0_y_ins0 = p->ins_h;
  reg.conv_opd0_x_ins0_last = p->ins_last_w;
  reg.conv_opd0_y_ins0_last = p->ins_last_h;

  reg.opd1_addr = p->weight->start_address;
  reg.opt_opd1_sign = opd1_sign;
  reg.opt_opd1_seg = 1;
  reg.opt_opd1_const = p->w_is_const;
  reg.opd1_n = p->weight->shape.n;
  reg.opd1_c = p->weight->shape.c;
  reg.opd1_h = p->weight->shape.h;
  reg.opd1_w = p->weight->shape.w;
  reg.short_opd1_str = 1;
  reg.conv_opd1_x_ins0 = p->dilation_w - 1;
  reg.conv_opd1_y_ins0 = p->dilation_h - 1;
  reg.conv_op_x_str = p->stride_w;
  reg.conv_op_y_str = p->stride_h;

  if (p->chl_quan_param) {
    CHECK(status, p->chl_quan_param->shape.n == 1);
    CHECK(status, p->chl_quan_param->shape.c == p->ofmap->shape.c);
    CHECK(status, p->chl_quan_param->shape.h == 1);
    CHECK(status, p->chl_quan_param->shape.w == 1);
    reg.opt_chl_quan = 1;
    reg.opt_res_shift = 0;  // useless
    reg.opd2_addr = p->chl_quan_param->start_address;
    reg.opd2_n = p->chl_quan_param->shape.n;
    reg.opd2_c = p->chl_quan_param->shape.c;
    reg.opd2_h = p->chl_quan_param->shape.h;
    reg.opd2_w = p->chl_quan_param->shape.w;
  }
  reg.opt_opd2_seg = 1;  // useless, force to 1 to skip b_stride check
  reg.short_opd2_str = 2; // useless
  reg.opd2_b_str = 0;     // useless

  if (p->has_bias) {
    reg.tsk_opd_num = 3;
    reg.opt_opd2_sign = 1;
  }

  reg.layer_info = p->layer_id;
  reg.cmd_pre_exe_typ = p->cmd_pre_exe_typ;
  reg.cmd_pre_exe = p->cmd_pre_exe;

  if (status) {
    printf("cvkcv181x tiu conv: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
