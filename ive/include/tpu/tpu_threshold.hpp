#pragma once
#include "core.hpp"

class IveTPUThreshold : public IveCore {
 public:
  void setThreshold(int threshold);
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
  int m_threshold = -1;
  cvk_tiu_mac_param_t m_p_mac;
  cvk_tiu_mul_param_t m_p_mul;
};

class IveTPUThresholdHighLow : public IveCore {
 public:
  void setThreshold(int threshold, int low, int high);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  int m_threshold = -1;
  int m_threshold_high = 255;
  int m_threshold_low = 0;
  cvk_tiu_mac_param_t m_p_mac;
  cvk_tiu_mul_param_t m_p_mul;
  cvk_tiu_max_param_t m_p_max;
  cvk_tiu_min_param_t m_p_min;
};

class IveTPUThresholdSlope : public IveCore {
 public:
  void setThreshold(int low, int high);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  int m_threshold_high = 255;
  int m_threshold_low = 0;
  cvk_tiu_max_param_t m_p_max;
  cvk_tiu_min_param_t m_p_min;
};