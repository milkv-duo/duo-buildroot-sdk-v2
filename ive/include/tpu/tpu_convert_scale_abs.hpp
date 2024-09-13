#pragma once
#include "core.hpp"

class IveTPUConvertScaleAbs : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

  void setAlpha(uint16_t alpha) { m_alpha = alpha; }
  void setBeta(uint16_t beta) { m_beta = beta; }

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  std::vector<cvk_tl_t *> m_input;
  std::vector<cvk_tl_t *> m_buf1;
  uint16_t m_alpha;
  uint16_t m_beta;
};