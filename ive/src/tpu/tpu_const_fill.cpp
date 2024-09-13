#include <string.h>
#include "tpu/tpu_fill.hpp"

int IveTPUConstFill::run(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const float value,
                         std::vector<CviImg *> &output) {
  if (output.size() != 1) {
    return CVI_FAILURE;
  }
  cvk_tdma_l2g_tensor_fill_constant_param_t fill_param;
  if (output[0]->m_tg.fmt == CVK_FMT_BF16) {
    fill_param.constant = convert_fp32_bf16(value);
  } else {
    fill_param.constant = (uint16_t)(int)value;
  }
  fill_param.dst = &output[0]->m_tg;
  cvk_ctx->ops->tdma_l2g_tensor_fill_constant(cvk_ctx, &fill_param);
  CVI_RT_Submit(cvk_ctx);

  return CVI_SUCCESS;
}