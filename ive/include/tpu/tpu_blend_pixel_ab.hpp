#pragma once
#include "core.hpp"

class IveTPUBlendPixelAB : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  void set_right_shift_bit(int num_shift) { m_ishift_r = num_shift; }

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
  int m_ishift_r = 8;
  std::vector<cvk_tl_t *> m_input1;
  std::vector<cvk_tl_t *> m_input2;
  std::vector<cvk_tl_t *> m_alpha;
  std::vector<cvk_tl_t *> m_buf_low;
  std::vector<cvk_tl_t *> m_buf_high;
  std::vector<cvk_tl_t *> m_alpha2;
  std::vector<cvk_tl_t *> m_hight_zeros;
};
