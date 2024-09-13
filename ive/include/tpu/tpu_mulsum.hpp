#pragma once
#include "core.hpp"

class IveTPUMulSum : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  double getSum();

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;
  virtual void beforeSubmit(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                            const std::vector<CviImg *> &input,
                            std::vector<CviImg *> &output) override;
  virtual int postProcess(CVI_RT_HANDLE rt_handle) override;

 private:
  double m_sum = 1.f;
  std::vector<cvk_tl_t *> m_input;
  cvk_tl_t *mp_tl_mulsum = nullptr;
  cvk_tl_shape_t m_tl_mulsum_shape;
  cvk_tl_stride_t m_tl_mulsum_stride;
  cvk_tiu_mul_param_t m_p_mul;

  CVI_RT_MEM m_rt_dev = NULL;
};