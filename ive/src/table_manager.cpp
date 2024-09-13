#include "table_manager.hpp"
#include "ive_log.hpp"

TblMgr::TblMgr() {}
TblMgr::~TblMgr() {}

int TblMgr::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_table_s_u8 = {1, cvk_ctx->info.npu_num, 16, 16};  // Kernel does not support U8 table info.
  // cvm_lut_tbl_bytesize(cvk_ctx, &m_table_s_u8, CVK_FMT_U8);  // 16 * 16
  cvm_lut_tbl_bytesize(cvk_ctx, &m_table_s, CVK_FMT_BF16);  // 32 * 8
  if (mp_atan_y0_degree == nullptr) {
    mp_atan_y0_degree = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_atan_fast_degree_y0((uint16_t *)mp_atan_y0_degree->GetVAddr(), &m_table_s);
    mp_atan_y0_degree->Flush(rt_handle);
  }
  if (mp_atan_y0 == nullptr) {
    mp_atan_y0 = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_atan_y0((uint16_t *)mp_atan_y0->GetVAddr(), &m_table_s);
    mp_atan_y0->Flush(rt_handle);
  }
  if (mp_atan_slope == nullptr) {
    mp_atan_slope = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_atan_slope((uint16_t *)mp_atan_slope->GetVAddr(), &m_table_s);
    mp_atan_slope->Flush(rt_handle);
  }
  if (mp_atan_invert == nullptr) {
    mp_atan_invert = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_atan_s_01((uint16_t *)mp_atan_invert->GetVAddr(), &m_table_s);
    mp_atan_invert->Flush(rt_handle);
  }

  if (mp_reciprocal_data == nullptr) {
    mp_reciprocal_data = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_gen_reciprocal((uint16_t *)mp_reciprocal_data->GetVAddr(), &m_table_s);
    mp_reciprocal_data->Flush(rt_handle);
  }
  if (mp_reciprocal_mantissa == nullptr) {
    mp_reciprocal_mantissa =
        new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_gen_reciprocal_mantissa((uint16_t *)mp_reciprocal_mantissa->GetVAddr(), &m_table_s);
    mp_reciprocal_mantissa->Flush(rt_handle);
  }

  if (mp_sqrt_data == nullptr) {
    mp_sqrt_data = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_gen_sqrt((uint16_t *)mp_sqrt_data->GetVAddr(), &m_table_s);
    mp_sqrt_data->Flush(rt_handle);
  }
  if (mp_sqrt_mantissa == nullptr) {
    mp_sqrt_mantissa = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_gen_sqrt_mantissa((uint16_t *)mp_sqrt_mantissa->GetVAddr(), &m_table_s);
    mp_sqrt_mantissa->Flush(rt_handle);
  }

  if (mp_zero == nullptr) {
    mp_zero = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_gen_0_tbl((uint16_t *)mp_zero->GetVAddr(), &m_table_s);
    mp_zero->Flush(rt_handle);
  }
  if (mp_pos_neg == nullptr) {
    mp_pos_neg = new CviImg(rt_handle, m_table_s.c, m_table_s.h, m_table_s.w, CVK_FMT_BF16);
    cvm_atan_pos_neg((uint16_t *)mp_pos_neg->GetVAddr(), &m_table_s);
    mp_pos_neg->Flush(rt_handle);
  }
  return CVI_SUCCESS;
}

int TblMgr::free(CVI_RT_HANDLE rt_handle) {
  if (mp_atan_y0_degree) {
    mp_atan_y0_degree->Free(rt_handle);
    delete mp_atan_y0_degree;
    mp_atan_y0_degree = nullptr;
  }
  if (mp_atan_y0) {
    mp_atan_y0->Free(rt_handle);
    delete mp_atan_y0;
    mp_atan_y0 = nullptr;
  }
  if (mp_atan_slope) {
    mp_atan_slope->Free(rt_handle);
    delete mp_atan_slope;
    mp_atan_slope = nullptr;
  }
  if (mp_atan_invert) {
    mp_atan_invert->Free(rt_handle);
    delete mp_atan_invert;
    mp_atan_invert = nullptr;
  }

  if (mp_reciprocal_data) {
    mp_reciprocal_data->Free(rt_handle);
    delete mp_reciprocal_data;
    mp_reciprocal_data = nullptr;
  }
  if (mp_reciprocal_mantissa) {
    mp_reciprocal_mantissa->Free(rt_handle);
    delete mp_reciprocal_mantissa;
    mp_reciprocal_mantissa = nullptr;
  }

  if (mp_sqrt_data) {
    mp_sqrt_data->Free(rt_handle);
    delete mp_sqrt_data;
    mp_sqrt_data = nullptr;
  }
  if (mp_sqrt_mantissa) {
    mp_sqrt_mantissa->Free(rt_handle);
    delete mp_sqrt_mantissa;
    mp_sqrt_mantissa = nullptr;
  }

  if (mp_pos_neg) {
    mp_pos_neg->Free(rt_handle);
    delete mp_pos_neg;
    mp_pos_neg = nullptr;
  }
  if (mp_zero) {
    mp_zero->Free(rt_handle);
    delete mp_zero;
    mp_zero = nullptr;
  }
  return CVI_SUCCESS;
}

const cvk_tl_shape_t TblMgr::getTblTLShape(cvk_fmt_t fmt) {
  if (fmt == CVK_FMT_U8) {
    return m_table_s_u8;
  } else if (fmt == CVK_FMT_BF16) {
    return m_table_s;
  }
  LOGE("Unsupported fmt %u.\n", fmt);
  return {0, 0, 0, 0};
}

const CviImg *TblMgr::atan(enum TBLATAN tblatan) {
  CviImg *img = nullptr;
  switch (tblatan) {
    case TBLATAN::TBLATAN_Y0: {
      img = mp_atan_y0;
    } break;
    case TBLATAN::TBLATAN_Y0_DEGREE: {
      img = mp_atan_y0_degree;
    } break;
    case TBLATAN::TBLATAN_SLOPE: {
      img = mp_atan_slope;
    } break;
    case TBLATAN::TBLATAN_INVERT: {
      img = mp_atan_invert;
    } break;
    case TBLATAN::TBLATAN_POSNEG: {
      img = mp_pos_neg;
    } break;
  }
  return img;
}

const CviImg *TblMgr::reciprocal(enum TBLRECIPROCAL tblrpc) {
  CviImg *img = nullptr;
  switch (tblrpc) {
    case TBLRECIPROCAL::TBLRECIPROCAL_DATA: {
      img = mp_reciprocal_data;
    } break;
    case TBLRECIPROCAL::TBLRECIPROCAL_MANTISSA: {
      img = mp_reciprocal_mantissa;
    } break;
  }
  return img;
}

const CviImg *TblMgr::sqrt(enum TBLSQRT tblsqrt) {
  CviImg *img = nullptr;
  switch (tblsqrt) {
    case TBLSQRT::TBLSQRT_DATA: {
      img = mp_sqrt_data;
    } break;
    case TBLSQRT::TBLSQRT_MANTISSA: {
      img = mp_sqrt_mantissa;
    } break;
  }
  return img;
}

const CviImg *TblMgr::mask(enum TBLMASK tblmask) {
  CviImg *img = nullptr;
  switch (tblmask) {
    case TBLMASK::TBLMASK_ZERO: {
      img = mp_zero;
    } break;
    case TBLMASK::TBLMASK_POSNEG: {
      img = mp_pos_neg;
    } break;
  }
  return img;
}