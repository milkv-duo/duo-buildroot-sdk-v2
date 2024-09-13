#pragma once
#include "core.hpp"

class IveTPUCopyDirect {
 public:
  static int run(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const CviImg *input,
                 CviImg *output);
};

class IveTPUCopyInterval : public IveCore {
 public:
  void setInvertal(uint32_t hori, uint32_t verti);
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

 private:
  uint32_t m_hori = 1;
  uint32_t m_verti = 1;
  cvk_tiu_copy_param_t m_p_copy;
};