#include "kernel_1880v2.h"

typedef bmk1880v2_tiu_convolution_param_t param_t;

static int can_do_double_conv(ctx_t *ctx, const param_t *p)
{
  uint8_t bf16_enable = (p->ifmap->fmt == FMT_BF16) ? 1 : 0;

  if (((p->ifmap->start_address % ctx->chip_info.lmem_size) % 2 == 0 &&
      p->ifmap->shape.c % 2 == 0 &&
      p->ifmap->shape.c >= 4 &&
      p->weight->start_address % 2 == 0) && !bf16_enable)
    return 1;

  return 0;
}

static void check_conv_param(ctx_t *ctx, const param_t *p)
{
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint8_t bf16_enable = (p->ifmap->fmt == FMT_BF16) ? 1 : 0;

  check_tiu_tensor_3(p->ifmap, p->ofmap, p->weight);
  if (bf16_enable) {
    assert_bf16_stride_type_0(ctx, p->ifmap);
  } else {
    assert_stride_type_0(ctx, p->ifmap);
  }
  //assert_stride_type_1(ctx, p->weight);
  if (p->bias) {
    check_tiu_tensor(p->bias);
    if (bf16_enable)
      assert_bf16_stride_type_2(ctx, p->bias);
    else
      assert_stride_type_2(ctx, p->bias);
  }

  // n stride must align 16B
  ASSERT((p->ofmap->stride.n % 16) == 0);

  ASSERT(p->ifmap->start_address % eu_num == 0);
  ASSERT(p->ofmap->start_address % eu_num == 0);
  ASSERT(p->ifmap->shape.n == p->ofmap->shape.n);
  ASSERT(!(p->ifmap->shape.h == 1 && p->ins_h > 0));
  ASSERT(p->weight->shape.n == p->ifmap->shape.c);
  ASSERT(p->weight->shape.c == p->ofmap->shape.c);
  if (can_do_double_conv(ctx, p)) {
    uint32_t lmem_i = p->ifmap->start_address % ctx->chip_info.lmem_size;
    ASSERT(lmem_i % 2 == 0);
    ASSERT(p->ifmap->shape.c % 2 == 0);
    ASSERT(p->ifmap->shape.c >= 4); /* Otherwise performance will suffer */
    ASSERT(p->weight->start_address % 2 == 0);
  }
  if(p->ps32_mode & 0x2)
  {
    ASSERT(!p->relu_enable);
    ASSERT(!p->bias);
    ASSERT(!p->rshift_bits);
  }
  ASSERT(p->stride_h < 16);
  ASSERT(p->stride_w < 16);
  ASSERT(p->pad_top < 16);
  ASSERT(p->pad_bottom < 16);
  ASSERT(p->pad_left < 16);
  ASSERT(p->pad_right < 16);
  ASSERT(p->ins_h < 16);
  ASSERT(p->ins_last_h < 16);
  ASSERT(p->ins_w < 16);
  ASSERT(p->ins_last_w < 16);
  ASSERT(p->dilation_h >= 1);
  ASSERT(p->dilation_w >= 1);
}

bmk1880v2_op_t * bmk1880v2_tiu_convolution(ctx_t *ctx, const param_t *p)
{
  check_conv_param(ctx, p);

  uint32_t npu_num = ctx->chip_info.npu_num;
  int opd0_sign = tensor_is_signed(p->ifmap);
  int opd1_sign = tensor_is_signed(p->weight);
  int opd2_sign = p->bias? tensor_is_signed(p->bias): 1;
  int arith_shift = opd0_sign || opd1_sign || opd2_sign;
  int bf16_enable = (p->ifmap->fmt == FMT_BF16) ? 1 : 0;

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_CONV_FIX8B;
  reg.opt_shift_typ = arith_shift;
  reg.opt_right_shift = p->rshift_bits;
  reg.opt_relu = !!(p->relu_enable);
  reg.tsk_opd_num = 2;

  reg.opd_typ = bf16_enable ? 1: 0;

  /*always automatically enabel double conv at those situations*/
  if (can_do_double_conv(ctx, p))
    reg.double_conv = 1;

  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->ofmap);
  reg.opt_res0_int8 = 1;
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
  if (p->ps32_mode > 0)
    reg.res0_b_str = p->ofmap->shape.n * p->ofmap->stride.n;

  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_int8 = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.opd0_ins_val = bf16_enable ? 0 : (uint32_t)p->ins_val;
  reg.opd0_ins_fp = bf16_enable ? (uint32_t)p->ins_fp : 0;
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
  reg.opt_opd1_int8 = 1;
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

  if (p->bias) {
    ASSERT(p->bias->shape.n == 2);
    ASSERT(p->bias->shape.c == p->ofmap->shape.c);
    ASSERT(p->bias->shape.h == 1);
    ASSERT(p->bias->shape.w == 1);

    reg.tsk_opd_num = 3;
    reg.opt_opd2_sign = opd2_sign;
    reg.opt_opd2_int8 = 0;
    reg.opd2_addr = p->bias->start_address;
    reg.opd2_n = 1;
    reg.opd2_c = p->bias->shape.c;
    reg.opd2_h = 1;
    reg.opd2_w = 1;
    reg.short_opd2_str = 2;
    reg.opd2_b_str = ceiling_func(p->bias->shape.c, npu_num) * (bf16_enable ? 2 : 1);
  }

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;
  return emit_tiu_cmdbuf(ctx, &reg);
}
