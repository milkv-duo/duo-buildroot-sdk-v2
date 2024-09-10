#pragma once
#include "core.hpp"
#include "table_manager.hpp"

class IveTPUSobel : public IveCore {
 public:
  void setTblMgr(TblMgr *tblmgr);
  void setKernel(IveKernel &kernel_x, IveKernel &kernel_y);
  void magDistMethod(int method);
  virtual int init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) override;

 private:
  virtual int runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) override;
  virtual void operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         uint32_t ping_idx) override;

 private:
  TblMgr *mp_tblmgr = nullptr;
  IveKernel *m_kernel_x = nullptr;
  IveKernel *m_kernel_y = nullptr;
  cvk_tiu_depthwise_pt_convolution_param_t m_p_conv_x;
  cvk_tiu_depthwise_pt_convolution_param_t m_p_conv_y;
  int m_dist_method = 1;
  cvk_tiu_mul_param_t m_p_mul_a;
  // L1 dist
  cvk_tiu_mul_param_t m_p_mul_b;
  cvk_tiu_max_param_t m_p_max_a;
  cvk_tiu_max_param_t m_p_max_b;
  cvk_tiu_add_param_t m_p_add;
  // L2 dist
  cvk_tiu_mac_param_t m_p_mac;
  cvm_tiu_sqrt_param_t m_p_sqrt;
};

class IveTPUSobelGradOnly : public IveCore {
 public:
  void setKernel(IveKernel &kernel_x, IveKernel &kernel_y);
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
  IveKernel *m_kernel_x = nullptr;
  IveKernel *m_kernel_y = nullptr;
  cvk_tiu_depthwise_pt_convolution_param_t m_p_conv;
};