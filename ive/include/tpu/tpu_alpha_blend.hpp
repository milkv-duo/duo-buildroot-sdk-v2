#pragma once
#include "core.hpp"

class IveTPUBlend : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

  void setWeight(uint8_t weight) { m_weight = weight; }

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

  void get_less_large_mask(cvk_context_t *ctx, cvk_tl_t *buf, cvk_tl_t *buf2,
                           cvk_tl_t *tl_update_tbl, uint8_t threshold, bool is_less);

 private:
  std::vector<cvk_tl_t *> m_input1;
  std::vector<cvk_tl_t *> m_input2;
  std::vector<cvk_tl_t *> m_buf1;
  std::vector<cvk_tl_t *> m_buf2;
  uint8_t m_weight;
};