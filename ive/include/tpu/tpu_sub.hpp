#pragma once
#include "core.hpp"

class IveTPUSub : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  void setSignedOutput(bool is_signed_output) { m_is_signed_output = is_signed_output; }
  void setRightShiftOneBit(bool enable) { m_enable_right_shift = enable; }

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  std::vector<cvk_tl_t *> m_input1;
  std::vector<cvk_tl_t *> m_input2;
  cvk_tiu_mac_param_t m_p_mac;
  bool m_is_signed_output;
  bool m_enable_right_shift;
};

class IveTPUSubAbs : public IveCore {
 public:
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  void setBinaryOutput(bool is_binary) { m_is_binary_output = is_binary; }
  void setClipOutput(bool is_clip) { m_clip_128 = is_clip; }

 protected:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  std::vector<cvk_tl_t *> m_input1;
  std::vector<cvk_tl_t *> m_input2;
  std::vector<cvk_tl_t *> m_min;
  cvk_tiu_max_param_t m_p_max;
  cvk_tiu_min_param_t m_p_min;
  cvk_tiu_mac_param_t m_p_mac;
  cvk_tiu_mul_param_t m_p_mul;
  bool m_is_binary_output;
  bool m_clip_128;
};
