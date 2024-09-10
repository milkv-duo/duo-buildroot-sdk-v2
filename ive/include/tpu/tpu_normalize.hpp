#pragma once
#include "core.hpp"

class IveTPUNormalize : public IveCore {
 public:
  void setMinMax(float min, float max);
  void setOutputFMT(cvk_fmt_t fmt);
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
  float m_min = 0, m_max = 0;
  cvk_fmt_t m_fmt = CVK_FMT_INVALID;
  std::vector<cvk_tl_t *> m_input;
  cvk_tiu_add_param_t m_p_add;
  cvk_tiu_mul_param_t m_p_mul;
  cvk_tiu_add_param_t m_p_add_offset;
};