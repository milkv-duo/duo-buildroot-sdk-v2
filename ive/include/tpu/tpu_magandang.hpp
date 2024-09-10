#pragma once
#include "core.hpp"
#include "table_manager.hpp"

class IveTPUMagAndAng : public IveCore {
 public:
  void setTblMgr(TblMgr *tblmgr);
  void exportOption(bool mag_value, bool ang_value, bool output_degree = true);
  void magDistMethod(int method);
  void noNegative(bool value);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  TblMgr *mp_tblmgr = nullptr;
  bool m_export_mag = true;
  int m_dist_method = 1;
  cvk_tiu_mul_param_t m_p_mul_a;
  // L1 dist
  cvk_tiu_mul_param_t m_p_mul_b;
  cvk_tiu_max_param_t m_p_max_a;
  cvk_tiu_max_param_t m_p_max_b;
  cvk_tiu_add_param_t m_p_add;

  // L2 dist
  cvk_tiu_mac_param_t m_p_mac;
  cvm_tiu_sqrt_param_t m_p_sqrt;
  bool m_export_ang = true;
  cvm_tiu_atan2_param_t m_p_atan2;

  bool m_no_negative = false;
  cvm_tiu_mask_param_t m_p_mask;
  cvk_tiu_mac_param_t m_p_mac_mask;
#ifdef MAGnANG_DEBUG
  uint64_t counting = 0;
#endif
};