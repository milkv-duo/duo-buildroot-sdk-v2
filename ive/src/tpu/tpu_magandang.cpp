#include "tpu/tpu_magandang.hpp"
#include "ive_log.hpp"

#include <string.h>
#ifdef MAGnANG_DEBUG
#include "../tpu_math/1880v2_utils.h"
#endif

void IveTPUMagAndAng::setTblMgr(TblMgr *tblmgr) { mp_tblmgr = tblmgr; }

void IveTPUMagAndAng::exportOption(bool mag_value, bool ang_value, bool output_degree) {
  m_export_mag = mag_value;
  m_export_ang = ang_value;
  m_p_atan2.output_degree = output_degree;
}

void IveTPUMagAndAng::magDistMethod(int method) {
  if (method != 0 && method != 1) {
    LOGE("Unsupported dist method.\n");
  }
  m_dist_method = method;
}

void IveTPUMagAndAng::noNegative(bool value) { m_no_negative = value; }

int IveTPUMagAndAng::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "magnang";
  m_slice_info.io_fmt = CVK_FMT_BF16;
  // 2 input tl
  // 6 tmp buf
  // 1 atan & 1 final sqrt result
  int total_tls = 7;
  int total_table = 7;

  if (!m_export_mag) {
    total_tls -= 1;
    total_table -= 2;
  }

  if (!m_export_ang) {
    total_tls -= 2;
    total_table -= 5;
    if (m_dist_method == 0) {
      total_tls -= 1;
    }
  }

  m_slice_info.nums_of_tl = total_tls * 2;
  m_slice_info.nums_of_table = total_table * 2;
  m_kernel_info.nums_of_kernel = 0;  // 2 BF16 kernels

  return CVI_SUCCESS;
}

int IveTPUMagAndAng::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                              const std::vector<cvk_tg_shape_t> &tg_in_slices,
                              const std::vector<cvk_tg_shape_t> &tg_out_slices,
                              std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                              const bool enable_cext) {
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  auto *tl_input2 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  // FIXME: Should reuse.
  auto *tl_mag = m_export_mag ? allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1) : NULL;
  auto *tl_angle = m_export_ang ? allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1) : NULL;

  auto *tl_buf =
      (m_dist_method == 1 || m_export_ang) ? allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1) : NULL;
  cvk_tl_t *tl_buf2 = NULL, *tl_buf3 = NULL, *tl_buf4 = NULL, *tl_buf5 = NULL, *tl_buf6 = NULL;
  tl_buf2 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  if (m_export_ang) {
    tl_buf3 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  }

  const cvk_tl_shape_t tl_table_s = mp_tblmgr->getTblTLShape(CVK_FMT_BF16);
  cvk_tl_t *tl_sqrt_table_answer = NULL, *tl_sqrt_table_answer_mantissa = NULL;
  if (m_export_mag) {
    tl_sqrt_table_answer = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
    tl_sqrt_table_answer_mantissa =
        allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
    const CviImg *table_data = mp_tblmgr->sqrt(TBLSQRT::TBLSQRT_DATA);
    const CviImg *table_data_mantissa = mp_tblmgr->sqrt(TBLSQRT::TBLSQRT_MANTISSA);
    cviImg2TL(rt_handle, cvk_ctx, *table_data, tl_sqrt_table_answer);
    cviImg2TL(rt_handle, cvk_ctx, *table_data_mantissa, tl_sqrt_table_answer_mantissa);
  }

  cvk_tl_t *tl_y0_table = NULL, *tl_invert_table = NULL, *tl_pos_neg_table = NULL,
           *tl_reciprocal_table_answer = NULL, *tl_reciprocal_table_answer_mantissa = NULL,
           *tl_slope_table = NULL, *tl_idx_0_table = NULL;
  if (m_export_ang) {
    // atan buf
    {
      tl_y0_table = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
      tl_invert_table = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
      tl_pos_neg_table = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
      const CviImg *table_data_atan_y0 = m_p_atan2.output_degree
                                             ? mp_tblmgr->atan(TBLATAN::TBLATAN_Y0_DEGREE)
                                             : mp_tblmgr->atan(TBLATAN::TBLATAN_Y0);
      const CviImg *table_data_atan_invert = mp_tblmgr->atan(TBLATAN::TBLATAN_INVERT);
      const CviImg *table_data_atan_pos_neg = mp_tblmgr->atan(TBLATAN::TBLATAN_POSNEG);
      cviImg2TL(rt_handle, cvk_ctx, *table_data_atan_y0, tl_y0_table);
      cviImg2TL(rt_handle, cvk_ctx, *table_data_atan_invert, tl_invert_table);
      cviImg2TL(rt_handle, cvk_ctx, *table_data_atan_pos_neg, tl_pos_neg_table);
    }
    {
      tl_reciprocal_table_answer =
          allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
      tl_reciprocal_table_answer_mantissa =
          allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
      const CviImg *table_data = mp_tblmgr->reciprocal(TBLRECIPROCAL::TBLRECIPROCAL_DATA);
      const CviImg *table_data_mantissa =
          mp_tblmgr->reciprocal(TBLRECIPROCAL::TBLRECIPROCAL_MANTISSA);
      cviImg2TL(rt_handle, cvk_ctx, *table_data, tl_reciprocal_table_answer);
      cviImg2TL(rt_handle, cvk_ctx, *table_data_mantissa, tl_reciprocal_table_answer_mantissa);
    }
  }

  if (m_dist_method == 0) {
    m_p_mul_a.rshift_bits = 0;
    m_p_mul_a.relu_enable = 1;
    m_p_mul_a.res_high = NULL;
    m_p_mul_a.res_low = tl_mag;
    m_p_mul_a.a = tl_input;
    m_p_mul_a.b_is_const = 1;
    m_p_mul_a.b_const.val = convert_fp32_bf16(-1.f);
    m_p_max_a.a = tl_input;
    m_p_max_a.b = tl_mag;
    m_p_max_a.max = tl_input;
    m_p_max_a.b_is_const = 0;

    m_p_mul_b.rshift_bits = 0;
    m_p_mul_b.relu_enable = 1;
    m_p_mul_b.res_high = NULL;
    m_p_mul_b.res_low = tl_mag;
    m_p_mul_b.a = tl_input2;
    m_p_mul_b.b_is_const = 1;
    m_p_mul_b.b_const.val = convert_fp32_bf16(-1.f);
    m_p_max_b.a = tl_input2;
    m_p_max_b.b = tl_mag;
    m_p_max_b.max = tl_input2;
    m_p_max_b.b_is_const = 0;

    m_p_add.a_low = tl_input;
    m_p_add.a_high = NULL;
    m_p_add.b.low = tl_input2;
    m_p_add.b.high = NULL;
    m_p_add.res_low = tl_mag;
    m_p_add.res_high = NULL;
    m_p_add.relu_enable = 0;
    m_p_add.b_is_const = 0;
    m_p_add.rshift_bits = 0;
  } else if (m_dist_method == 1) {
    m_p_mul_a.rshift_bits = 0;
    m_p_mul_a.relu_enable = 1;
    m_p_mul_a.res_high = NULL;
    m_p_mul_a.res_low = tl_buf;
    m_p_mul_a.a = tl_input;
    m_p_mul_a.b_is_const = 0;
    m_p_mul_a.b = tl_input;

    m_p_mac.res_is_int8 = 0;
    m_p_mac.lshift_bits = 0;
    m_p_mac.rshift_bits = 0;
    m_p_mac.relu_enable = 1;
    m_p_mac.res_high = NULL;
    m_p_mac.res_low = tl_buf;
    m_p_mac.a = tl_input2;
    m_p_mac.b_is_const = 0;
    m_p_mac.b = tl_input2;

    m_p_sqrt.a = tl_buf;
    m_p_sqrt.res = tl_mag;
    m_p_sqrt.buf = tl_buf2;
    m_p_sqrt.sqrt_table_answer = tl_sqrt_table_answer;
    m_p_sqrt.sqrt_table_answer_mantissa = tl_sqrt_table_answer_mantissa;
  }

  m_p_atan2.a = tl_input;
  m_p_atan2.b = tl_input2;
  m_p_atan2.res = tl_angle;
  m_p_atan2.buf1 = tl_buf;
  m_p_atan2.buf2 = tl_buf2;
  m_p_atan2.buf3 = tl_buf3;
  m_p_atan2.buf4 = tl_buf4;
  m_p_atan2.buf5 = tl_buf5;
  m_p_atan2.buf6 = tl_buf6;
  m_p_atan2.y0 = tl_y0_table;
  m_p_atan2.slope = tl_slope_table;
  m_p_atan2.invert = tl_invert_table;
  m_p_atan2.pos_neg_table = tl_pos_neg_table;
  m_p_atan2.reciprocal_table_answer = tl_reciprocal_table_answer;
  m_p_atan2.reciprocal_table_answer_mantissa = tl_reciprocal_table_answer_mantissa;
  m_p_atan2.sqrt_table_answer = tl_sqrt_table_answer;
  m_p_atan2.sqrt_table_answer_mantissa = tl_sqrt_table_answer_mantissa;
  m_p_atan2.idx_0_table = tl_idx_0_table;
  m_p_atan2.fmt = CVK_FMT_BF16;

  if (m_no_negative) {
    m_p_mask.ifmap = tl_angle;
    m_p_mask.ofmap = tl_buf;
    m_p_mask.buf = tl_buf2;
    m_p_mask.pos_neg_table = tl_pos_neg_table;
    m_p_mask.fmt = CVK_FMT_BF16;

    m_p_mac_mask.res_is_int8 = 0;
    m_p_mac_mask.lshift_bits = 0;
    m_p_mac_mask.rshift_bits = 0;
    m_p_mac_mask.relu_enable = 0;
    m_p_mac_mask.res_high = NULL;
    m_p_mac_mask.res_low = tl_angle;
    m_p_mac_mask.a = tl_buf;
    m_p_mac_mask.b_is_const = 1;
    m_p_mac_mask.b_const.val = convert_fp32_bf16(360.f);
  }

  tl_in_idx->push_back(0);
  tl_in_idx->push_back(1);
  if (m_export_mag) {
    tl_out_idx->push_back(2);
  }
  if (m_export_ang) {
    if (m_export_mag) {
      tl_out_idx->push_back(3);
    } else {
      tl_out_idx->push_back(2);
    }
  }
#ifdef MAGnANG_DEBUG
  counting = 0;
#endif
  return CVI_SUCCESS;
}

void IveTPUMagAndAng::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                uint32_t ping_idx) {
  if (m_export_mag) {
    if (m_dist_method == 0) {
      cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul_a);
      cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max_a);
      cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul_b);
      cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max_b);
      cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add);
    } else if (m_dist_method == 1) {
      cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul_a);
      cvk_ctx->ops->tiu_mac(cvk_ctx, &m_p_mac);
      cvm_emit_sqrt(cvk_ctx, m_p_sqrt.a, m_p_sqrt.buf, m_p_sqrt.sqrt_table_answer,
                    m_p_sqrt.sqrt_table_answer_mantissa, m_p_sqrt.res);
    }
  }
  if (m_export_ang) {
    // sqrt goes first cause atan2 changes y value
    if (m_p_atan2.output_degree) {
      cvm_atan2_fast_degree_emit(cvk_ctx, m_p_atan2.b, m_p_atan2.a, m_p_atan2.buf1, m_p_atan2.buf2,
                                 m_p_atan2.buf3, m_p_atan2.y0, m_p_atan2.invert,
                                 m_p_atan2.pos_neg_table, m_p_atan2.reciprocal_table_answer,
                                 m_p_atan2.reciprocal_table_answer_mantissa, m_p_atan2.res,
                                 m_p_atan2.fmt);
    } else {
      cvm_atan2_merge_emit(cvk_ctx, m_p_atan2.b, m_p_atan2.a, m_p_atan2.buf1, m_p_atan2.buf2,
                           m_p_atan2.buf3, m_p_atan2.y0, m_p_atan2.invert, m_p_atan2.pos_neg_table,
                           m_p_atan2.reciprocal_table_answer,
                           m_p_atan2.reciprocal_table_answer_mantissa, m_p_atan2.res,
                           m_p_atan2.fmt);
    }

    if (m_no_negative) {
      cvm_emit_neg_idx(cvk_ctx, m_p_mask.ifmap, m_p_mask.buf, m_p_mask.pos_neg_table,
                       m_p_mask.ofmap, m_p_mask.fmt);
      cvk_ctx->ops->tiu_mac(cvk_ctx, &m_p_mac_mask);
    }
  }
#ifdef MAGnANG_DEBUG
  counting++;
#endif
}