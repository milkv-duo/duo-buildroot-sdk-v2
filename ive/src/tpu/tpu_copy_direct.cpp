#include <string.h>
#include "tpu/tpu_copy.hpp"
inline void DirectCopyWrapper(cvk_context_t *cvk_ctx, const cvk_tg_t &in, const cvk_tg_t &out) {
  cvk_tdma_g2g_tensor_copy_param_t copy_param;
  LOGD("h:%d,w:%d,strideh:%d\n", (int)in.shape.h, (int)in.shape.w, (int)in.stride.h);
  if (in.shape.w > 4096 || in.shape.h > 4096) {
    cvk_tg_t tmp_in = in;
    cvk_tg_t tmp_out = out;
    copy_param.src = &tmp_in;
    copy_param.dst = &tmp_out;
    const uint32_t hw_max = 4096;
    for (uint32_t i = 0; i < in.shape.h; i += hw_max) {
      for (uint32_t j = 0; j < in.shape.w; j += hw_max) {
        uint32_t tmp_h = tmp_out.shape.h - hw_max * i;
        uint32_t tmp_w = tmp_out.shape.w - hw_max * j;
        tmp_in.shape.h = tmp_out.shape.h = tmp_h > 4096 ? 4096 : tmp_h;
        tmp_in.shape.w = tmp_out.shape.w = tmp_w > 4096 ? 4096 : tmp_w;
        tmp_in.start_address = in.start_address + i * in.stride.h + j * 4096;
        tmp_out.start_address = out.start_address + i * out.stride.h + j * 4096;
        cvk_ctx->ops->tdma_g2g_bf16_tensor_copy(cvk_ctx, &copy_param);
      }
    }
  } else {
    copy_param.src = &in;
    copy_param.dst = &out;
    cvk_ctx->ops->tdma_g2g_bf16_tensor_copy(cvk_ctx, &copy_param);
  }
}

int IveTPUCopyDirect::run(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, const CviImg *input,
                          CviImg *output) {
  if (input == NULL) {
    LOGE("IveTPUCopyDirect::run input is null\n");
    assert(0);
  }
  if (output == NULL) {
    LOGE("IveTPUCopyDirect::run output is null\n");
    assert(0);
  }
  LOGD("input is planar:%d,strideeq:%d,H:%d,W:%d\n", (int)input->IsStideCEQ(), input->IsPlanar(),
       (int)input->m_tg.shape.h, (int)input->m_tg.shape.w);
  // Special case handling for YUV sub-images
  if (input->IsStideCEQ() && input->IsPlanar() && output->IsStideCEQ() && output->IsPlanar()) {
    LOGD("go DirectCopyWrapper ");
    DirectCopyWrapper(cvk_ctx, input->m_tg, output->m_tg);
  } else {
    if (input->GetImgType() == CVI_YUV422P) {
      if (input->m_tg.shape.w % 2 != 0 || output->m_tg.shape.w % 2 != 0) {
        LOGE("Currently does not support odd width for YUV 422\n");
        return CVI_FAILURE;
      }
      uint8_t div[3] = {1, 2, 2};
      cvk_tg_t in, out;
      memset(&in, 0, sizeof(cvk_tg_t));
      memset(&out, 0, sizeof(cvk_tg_t));
      for (uint8_t i = 0; i < 3; i++) {
        in.start_address = input->GetPAddr() + input->GetImgCOffsets()[i];
        in.shape = {1, 1, input->m_tg.shape.h, input->m_tg.shape.w / div[i]};
        in.stride.h = input->GetImgStrides()[i];
        in.stride.c = input->m_tg.shape.h * in.stride.h;
        in.stride.n = in.stride.c;
        out.start_address = output->GetPAddr() + output->GetImgCOffsets()[i];
        out.shape = {1, 1, output->m_tg.shape.h, output->m_tg.shape.w / div[i]};
        out.stride.h = output->GetImgStrides()[i];
        out.stride.c = output->m_tg.shape.h * out.stride.h;
        out.stride.n = out.stride.c;
        DirectCopyWrapper(cvk_ctx, in, out);
      }
    } else if (input->GetImgType() == CVI_YUV420P) {
      if (input->m_tg.shape.w % 2 != 0 || output->m_tg.shape.w % 2 != 0) {
        LOGE("Currently does not support odd width for YUV 420\n");
        return CVI_FAILURE;
      }
      if (input->m_tg.shape.h % 2 != 0 || output->m_tg.shape.h % 2 != 0) {
        LOGE("Currently does not support odd height for YUV 420\n");
        return CVI_FAILURE;
      }
      LOGD("copy with 3 loop\n");
      uint8_t div[3] = {1, 2, 2};
      cvk_tg_t in, out;
      memset(&in, 0, sizeof(cvk_tg_t));
      memset(&out, 0, sizeof(cvk_tg_t));
      for (uint8_t i = 0; i < 3; i++) {
        in.start_address = input->GetPAddr() + input->GetImgCOffsets()[i];
        in.shape = {1, 1, input->m_tg.shape.h / div[i], input->m_tg.shape.w / div[i]};
        in.stride.h = input->GetImgStrides()[i];
        in.stride.c = input->m_tg.shape.h * in.stride.h;
        in.stride.n = in.stride.c;
        out.start_address = output->GetPAddr() + output->GetImgCOffsets()[i];
        out.shape = {1, 1, output->m_tg.shape.h / div[i], output->m_tg.shape.w / div[i]};
        out.stride.h = output->GetImgStrides()[i];
        out.stride.c = output->m_tg.shape.h * out.stride.h;
        out.stride.n = out.stride.c;
        DirectCopyWrapper(cvk_ctx, in, out);
      }
    } else if (input->GetImgType() == CVI_YUV420SP) {
      if (input->m_tg.shape.w % 2 != 0 || output->m_tg.shape.w % 2 != 0) {
        LOGE("Currently does not support odd width for YUV 420\n");
        return CVI_FAILURE;
      }
      if (input->m_tg.shape.h % 2 != 0 || output->m_tg.shape.h % 2 != 0) {
        LOGE("Currently does not support odd height for YUV 420\n");
        return CVI_FAILURE;
      }
      uint8_t div[2] = {1, 2};
      cvk_tg_t in, out;
      memset(&in, 0, sizeof(cvk_tg_t));
      memset(&out, 0, sizeof(cvk_tg_t));
      for (uint8_t i = 0; i < 2; i++) {
        in.start_address = input->GetPAddr() + input->GetImgCOffsets()[i];
        in.shape = {1, 1, input->m_tg.shape.h / div[i], input->m_tg.shape.w};
        in.stride.h = input->GetImgStrides()[i];
        in.stride.c = input->m_tg.shape.h * in.stride.h;
        in.stride.n = in.stride.c;
        out.start_address = output->GetPAddr() + output->GetImgCOffsets()[i];
        out.shape = {1, 1, output->m_tg.shape.h / div[i], output->m_tg.shape.w};
        out.stride.h = output->GetImgStrides()[i];
        out.stride.c = output->m_tg.shape.h * out.stride.h;
        out.stride.n = out.stride.c;
        DirectCopyWrapper(cvk_ctx, in, out);
      }
    } else {
      LOGE("Unsupported copy type %d.\n", input->GetImgType());
      return CVI_FAILURE;
    }
  }
  CVI_RT_Submit(cvk_ctx);
  return CVI_SUCCESS;
}
