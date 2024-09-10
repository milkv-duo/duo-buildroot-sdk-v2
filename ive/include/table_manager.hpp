#pragma once
#include "tpu_data.hpp"

enum TBLATAN { TBLATAN_Y0, TBLATAN_Y0_DEGREE, TBLATAN_SLOPE, TBLATAN_INVERT, TBLATAN_POSNEG };

enum TBLRECIPROCAL { TBLRECIPROCAL_DATA, TBLRECIPROCAL_MANTISSA };

enum TBLSQRT { TBLSQRT_DATA, TBLSQRT_MANTISSA };

enum TBLMASK { TBLMASK_POSNEG, TBLMASK_ZERO };

class TblMgr {
 public:
  TblMgr();
  ~TblMgr();
  int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx);
  int free(CVI_RT_HANDLE rt_handle);
  const cvk_tl_shape_t getTblTLShape(cvk_fmt_t fmt);
  const CviImg *atan(enum TBLATAN tblatan);
  const CviImg *reciprocal(enum TBLRECIPROCAL tblrpc);
  const CviImg *sqrt(enum TBLSQRT tblsqrt);
  const CviImg *mask(enum TBLMASK tblmsk);

 private:
  cvk_tl_shape_t m_table_s_u8;
  cvk_tl_shape_t m_table_s;
  // atan2 CviImg
  CviImg *mp_atan_y0_degree = nullptr;
  CviImg *mp_atan_y0 = nullptr;
  CviImg *mp_atan_slope = nullptr;
  CviImg *mp_atan_invert = nullptr;

  CviImg *mp_reciprocal_data = nullptr;
  CviImg *mp_reciprocal_mantissa = nullptr;

  CviImg *mp_sqrt_data = nullptr;
  CviImg *mp_sqrt_mantissa = nullptr;

  CviImg *mp_pos_neg = nullptr;
  CviImg *mp_zero = nullptr;
};