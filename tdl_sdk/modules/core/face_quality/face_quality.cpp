#include "face_quality.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "image_utils.hpp"

// include core_c.h if opencv version greater than 4.5
#if CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 5
#include "opencv2/core/core_c.h"
#endif

#define SCALE_R (1.0 / (255.0 * 0.229))
#define SCALE_G (1.0 / (255.0 * 0.224))
#define SCALE_B (1.0 / (255.0 * 0.225))
#define MEAN_R (0.485 / 0.229)
#define MEAN_G (0.456 / 0.224)
#define MEAN_B (0.406 / 0.225)
#define NAME_SCORE "score_Softmax_dequant"

static bool IS_SUPPORTED_FORMAT(VIDEO_FRAME_INFO_S *frame) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21) {
    LOGE("Pixel format [%d] is not supported.\n", frame->stVFrame.enPixelFormat);
    return false;
  }
  return true;
}

namespace cvitdl {

FaceQuality::FaceQuality() : Core(CVI_MEM_DEVICE) {
  std::vector<float> mean = {MEAN_R, MEAN_G, MEAN_B};
  std::vector<float> scale = {SCALE_R, SCALE_G, SCALE_B};
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = scale[i];
    m_preprocess_param[0].mean[i] = mean[i];
  }
}

FaceQuality::~FaceQuality() {}

int FaceQuality::onModelOpened() { return allocateION(); }

int FaceQuality::onModelClosed() {
  releaseION();
  return CVI_TDL_SUCCESS;
}

CVI_S32 FaceQuality::allocateION() {
  CVI_SHAPE shape = getInputShape(0);
  if (CREATE_ION_HELPER(&m_wrap_frame, shape.dim[3], shape.dim[2], PIXEL_FORMAT_RGB_888, "tpu") !=
      CVI_SUCCESS) {
    LOGE("Cannot allocate ion for preprocess\n");
    return CVI_TDL_ERR_ALLOC_ION_FAIL;
  }
  return CVI_TDL_SUCCESS;
}

void FaceQuality::releaseION() {
  if (m_wrap_frame.stVFrame.u64PhyAddr[0] != 0) {
    CVI_SYS_IonFree(m_wrap_frame.stVFrame.u64PhyAddr[0], m_wrap_frame.stVFrame.pu8VirAddr[0]);
    m_wrap_frame.stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    m_wrap_frame.stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    m_wrap_frame.stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    m_wrap_frame.stVFrame.pu8VirAddr[0] = NULL;
    m_wrap_frame.stVFrame.pu8VirAddr[1] = NULL;
    m_wrap_frame.stVFrame.pu8VirAddr[2] = NULL;
  }
}

int FaceQuality::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta, bool *skip) {
  if (false == IS_SUPPORTED_FORMAT(frame)) {
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  CVI_U32 frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  bool do_unmap = false;
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  int ret = CVI_TDL_SUCCESS;
  for (uint32_t i = 0; i < meta->size; i++) {
    if (skip != NULL && skip[i]) {
      continue;
    }
    cvtdl_face_info_t face_info =
        info_rescale_c(frame->stVFrame.u32Width, frame->stVFrame.u32Height, *meta, i);
    ALIGN_FACE_TO_FRAME(frame, &m_wrap_frame, face_info);

    std::vector<VIDEO_FRAME_INFO_S *> frames = {&m_wrap_frame};
    ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    float *score = getOutputRawPtr<float>(NAME_SCORE);
    meta->info[i].face_quality = score[1];

    CVI_TDL_FreeCpp(&face_info);
  }
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return ret;
}

}  // namespace cvitdl
