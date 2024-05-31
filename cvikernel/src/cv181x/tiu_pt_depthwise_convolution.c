#include "cvkcv181x.h"

void cvkcv181x_tiu_pt_depthwise_convolution(
    cvk_context_t *ctx,
    const cvk_tiu_depthwise_pt_convolution_param_t *p)
{
  int8_t status = 0;
  int bf16_enable = (p->ifmap->fmt == CVK_FMT_BF16) ? 1 : 0;

  int8_t isMulConst = (p->weight_is_const == 1) ? 1 : 0;

  if(isMulConst) {
    status |= check_tiu_tensor_2(p->ifmap, p->ofmap);
  } else {
    status |= check_tiu_tensor_3(p->ifmap, p->ofmap, p->weight);
  }
  if (bf16_enable) {
    status |= check_bf16_stride_type_0(ctx, p->ifmap);
    if(!isMulConst)
      status |= check_bf16_stride_type_0(ctx, p->weight);
    if (p->bias) {
      status |= check_tiu_tensor(p->bias);
      status |= check_bf16_stride_type_2(ctx, p->bias);
    }
  } else {
    status |= check_stride_type_0(ctx, p->ifmap);
    if(!isMulConst)
      status |= check_stride_type_0(ctx, p->weight);
    if (p->bias) {
      status |= check_tiu_tensor(p->bias);
      status |= check_stride_type_2(ctx, p->bias);
    }
  }

  // n stride must align 16B
  CHECK(status, (p->ofmap->stride.n % 16) == 0);

  CHECK(status, p->ifmap->shape.n == p->ofmap->shape.n);
  CHECK(status, p->ifmap->shape.c == p->ofmap->shape.c);
  if (!isMulConst){
    CHECK(status, p->ifmap->shape.c == p->weight->shape.c);
    CHECK(status, p->weight->shape.n == 1);
  }
  CHECK(status, p->relu_enable == 0 || p->relu_enable == 1);
  CHECK(status, p->stride_h < 32 && p->stride_h > 0);
  CHECK(status, p->stride_w < 32 &&  p->stride_w > 0);
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

  int opd0_sign = tensor_is_signed(p->ifmap);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_DEPTHWISE_POOL_FIX8B;
  reg.tsk_eu_typ = 2;
  reg.opt_relu_typ = p->relu_enable;
  reg.opt_shift_typ = 1;
  reg.opt_res_shift = p->rshift_bits;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;

  int res0_sign = tensor_is_signed(p->ofmap);
  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = res0_sign;
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

  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_seg = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.opd0_n_str = p->ifmap->stride.n;
  reg.opd0_c_str = p->ifmap->stride.c;
  reg.opd0_h_str = p->ifmap->stride.h;
  reg.opd0_w_str = p->ifmap->stride.w;
  reg.short_opd0_str = 3;   // Manual instead of h/w
  reg.conv_opd0_up_pad = p->pad_top;
  reg.conv_opd0_dn_pad = p->pad_bottom;
  reg.conv_opd0_lf_pad = p->pad_left;
  reg.conv_opd0_rt_pad = p->pad_right;
  reg.conv_opd0_x_ins0 = p->ins_w;
  reg.conv_opd0_y_ins0 = p->ins_h;
  reg.conv_opd0_x_ins0_last = p->ins_last_w;
  reg.conv_opd0_y_ins0_last = p->ins_last_h;

  reg.opt_opd1_sign = 1;
  reg.opt_opd1_seg = 1;
  reg.conv_opd1_x_ins0 = p->dilation_w - 1;
  reg.conv_opd1_y_ins0 = p->dilation_h - 1;
  if (isMulConst) {
    reg.opt_opd1_const = 1;
    reg.opt_opd1_sign = p->weight_const.is_signed;
    reg.opd1_addr = p->weight_const.val;
    reg.opd1_n = p->weight->shape.n;
    reg.opd1_c = p->weight->shape.c;
    reg.opd1_h = p->weight->shape.h;
    reg.opd1_w = p->weight->shape.w;
  } else {
    reg.opd1_addr = p->weight->start_address;
    reg.opd1_n = p->weight->shape.n;
    reg.opd1_c = p->weight->shape.c;
    reg.opd1_h = p->weight->shape.h;
    reg.opd1_w = p->weight->shape.w;
  }
  reg.conv_op_x_str = p->stride_w;
  reg.conv_op_y_str = p->stride_h;
  reg.opd0_ins_val = bf16_enable ?
                     (uint32_t)p->ins_fp : (uint32_t)p->ins_val;

  if (p->bias) {
    CHECK(status, p->bias->shape.n == 2);
    CHECK(status, p->bias->shape.c == p->ofmap->shape.c);
    CHECK(status, p->bias->shape.h == 1);
    CHECK(status, p->bias->shape.w == 1);

    reg.tsk_opd_num = 3;
    reg.opd2_addr = p->bias->start_address;
    reg.opt_opd2_seg = 0;
    reg.opd2_n = 1;
    reg.opd2_c = p->bias->shape.c;
    reg.opd2_h = 1;
    reg.opd2_w = 1;
    reg.short_opd2_str = 2;
    reg.opd2_b_str = p->bias->stride.n;
  }

  reg.layer_info = p->layer_id;

  reg.cmd_pre_exe_typ = p->cmd_pre_exe_typ;
  reg.cmd_pre_exe = p->cmd_pre_exe;

  if (status) {
    printf("cvkcv181x pt dw-conv: invalid param\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
