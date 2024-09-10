#pragma once
#include "core.hpp"

class IveTPUSigmoid : public IveCore {
 public:
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
  CviImg *table = nullptr, *table_slope = nullptr;
  std::vector<cvk_tl_t *> m_input;
  std::vector<cvk_tl_t *> m_buf;
  std::vector<cvk_tl_t *> m_output;
  cvm_tiu_sigmoid_param_t m_p_sig;
};