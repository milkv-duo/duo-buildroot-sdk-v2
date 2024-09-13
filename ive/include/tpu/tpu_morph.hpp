#pragma once
#include "core.hpp"

class IveTPUErode : public IveCore {
 public:
  void setKernel(IveKernel &kernel);
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
  IveKernel *m_kernel = nullptr;
  CviImg *mp_multiplier = nullptr;
  cvk_tl_t *mp_xor_ones = nullptr;
  cvk_tiu_depthwise_convolution_param_t m_p_conv;
  cvk_tiu_xor_int8_param_t m_p_xor;
};