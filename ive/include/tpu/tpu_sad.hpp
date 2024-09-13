#pragma once
#include "core.hpp"
#include "table_manager.hpp"

class IveTPUSAD : public IveCore {
 public:
  void setTblMgr(TblMgr *tblmgr);
  void outputThresholdOnly(bool value);
  void doThreshold(bool value);
  void setThreshold(const uint16_t threshold, const uint8_t min_val, const uint8_t max_val);
  void setWindowSize(const int window_size);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;
  virtual int postProcess(CVI_RT_HANDLE rt_handle) override;

 private:
  TblMgr *mp_tblmgr = nullptr;
  CviImg *mp_table_pos_neg = nullptr;
  bool m_output_thresh_only = false;
  bool m_do_threshold = false;
  uint16_t m_threshold = 0;
  uint8_t m_min_value = 0;
  uint8_t m_max_value = 255;
  cvk_tiu_max_param_t m_p_max;
  cvk_tiu_min_param_t m_p_min;
  cvk_tiu_sub_param_t m_p_sub;
  cvk_tiu_depthwise_pt_convolution_param_t m_p_conv;
  cvk_tiu_add_param_t m_p_add_thresh;
  cvm_tiu_mask_param_t m_p_mask;
};