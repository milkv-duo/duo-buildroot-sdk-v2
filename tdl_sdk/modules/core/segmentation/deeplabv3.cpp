#include "deeplabv3.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "error_msg.hpp"

#define SCALE (1 / 127.5)
#define MEAN 1.f
#define NAME_SCORE "Conv_160_dequant"

namespace cvitdl {

Deeplabv3::Deeplabv3() : Core(CVI_MEM_DEVICE) {
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = SCALE;
    m_preprocess_param[0].mean[i] = MEAN;
  }
  m_preprocess_param[0].keep_aspect_ratio = false;
}

Deeplabv3::~Deeplabv3() {}

int Deeplabv3::onModelOpened() { return allocateION(); }

int Deeplabv3::onModelClosed() {
  releaseION();
  return CVI_TDL_SUCCESS;
}

CVI_S32 Deeplabv3::allocateION() {
  CVI_SHAPE shape = getOutputShape(NAME_SCORE);
  if (CREATE_ION_HELPER(&m_label_frame, shape.dim[3], shape.dim[2], PIXEL_FORMAT_YUV_400, "tpu") !=
      CVI_SUCCESS) {
    LOGE("Cannot allocate ion for preprocess\n");
    return CVI_TDL_ERR_ALLOC_ION_FAIL;
  }
  return CVI_TDL_SUCCESS;
}

void Deeplabv3::releaseION() {
  if (m_label_frame.stVFrame.u64PhyAddr[0] != 0) {
    CVI_SYS_IonFree(m_label_frame.stVFrame.u64PhyAddr[0], m_label_frame.stVFrame.pu8VirAddr[0]);
    m_label_frame.stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    m_label_frame.stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    m_label_frame.stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    m_label_frame.stVFrame.pu8VirAddr[0] = NULL;
    m_label_frame.stVFrame.pu8VirAddr[1] = NULL;
    m_label_frame.stVFrame.pu8VirAddr[2] = NULL;
  }
}

int Deeplabv3::inference(VIDEO_FRAME_INFO_S *frame, VIDEO_FRAME_INFO_S *out_frame,
                         cvtdl_class_filter_t *filter) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
    LOGE("Error: pixel format not match PIXEL_FORMAT_RGB_888.\n");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  outputParser(filter);

  VPSS_CHN_ATTR_S vpssChnAttr;
  vpssChnAttr.u32Width = frame->stVFrame.u32Width;
  vpssChnAttr.u32Height = frame->stVFrame.u32Height;
  vpssChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
  vpssChnAttr.enPixelFormat = PIXEL_FORMAT_YUV_400;
  vpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
  vpssChnAttr.stFrameRate.s32DstFrameRate = -1;
  vpssChnAttr.u32Depth = 1;
  vpssChnAttr.bMirror = CVI_FALSE;
  vpssChnAttr.bFlip = CVI_FALSE;
  vpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
  vpssChnAttr.stNormalize.bEnable = CVI_FALSE;

  VPSS_SCALE_COEF_E enCoef = VPSS_SCALE_COEF_NEAREST;
  ret = mp_vpss_inst->sendFrame(&m_label_frame, &vpssChnAttr, &enCoef, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("Deeplabv3 send frame failed: %s\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(out_frame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("Deeplabv3 resized output label failed: %s\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }

  return CVI_TDL_SUCCESS;
}

inline bool is_preserved_class(cvtdl_class_filter_t *filter, int32_t c) {
  if (c == 0) return true;  // "unlabled class" should be preserved

  if (filter->num_preserved_classes > 0) {
    for (uint32_t j = 0; j < filter->num_preserved_classes; j++) {
      if (filter->preserved_class_ids[j] == (uint32_t)c) {
        return true;
      }
    }
  }

  return false;
}

int Deeplabv3::outputParser(cvtdl_class_filter_t *filter) {
  float *out = getOutputRawPtr<float>(NAME_SCORE);

  CVI_SHAPE output_shape = getOutputShape(NAME_SCORE);

  int size = output_shape.dim[2] * output_shape.dim[3];
  std::vector<float> max_prob(size, 0);

  for (int32_t c = 0; c < output_shape.dim[1]; ++c) {
    int size_offset = c * size;
    for (int32_t h = 0; h < output_shape.dim[2]; ++h) {
      int width_offset = h * output_shape.dim[3];
      int frame_offset = h * m_label_frame.stVFrame.u32Stride[0];

      for (int32_t w = 0; w < output_shape.dim[3]; ++w) {
        if (out[size_offset + width_offset + w] > max_prob[width_offset + w]) {
          m_label_frame.stVFrame.pu8VirAddr[0][frame_offset + w] = (int8_t)c;
          max_prob[width_offset + w] = out[size_offset + width_offset + w];
        }
      }
    }
  }

  if (filter) {
    for (int32_t h = 0; h < output_shape.dim[2]; ++h) {
      int frame_offset = h * m_label_frame.stVFrame.u32Stride[0];

      for (int32_t w = 0; w < output_shape.dim[3]; ++w) {
        if (!is_preserved_class(filter, m_label_frame.stVFrame.pu8VirAddr[0][frame_offset + w])) {
          m_label_frame.stVFrame.pu8VirAddr[0][frame_offset + w] = 0;
        }
      }
    }
  }

  CVI_SYS_IonFlushCache(m_label_frame.stVFrame.u64PhyAddr[0], m_label_frame.stVFrame.pu8VirAddr[0],
                        m_label_frame.stVFrame.u32Length[0]);

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl
