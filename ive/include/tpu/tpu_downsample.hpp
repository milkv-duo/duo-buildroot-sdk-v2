#pragma once
#include "core.hpp"

class IveTPUDownSample : public IveCore {
 public:
  void setScaleNum(const float scale_num);
  void setCellSize(const int cell_size, const int channel = 3);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

 protected:
  virtual int sliceSetup(SliceRes &slice_res, SliceRes *tg_in_res, SliceRes *tg_out_res) override;
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;
  virtual int postProcess(CVI_RT_HANDLE rt_handle) override;
  void GetSliceUnitProperty(const uint32_t length, const uint32_t slice, const int kernel_sz,
                            const int default_stride, sliceUnit *unit);

 private:
  float m_scale_num = 1;
  uint32_t m_channel = 1;
  CviImg *mp_multiplier = nullptr;
  cvk_tiu_depthwise_convolution_param_t m_p_conv;
};
