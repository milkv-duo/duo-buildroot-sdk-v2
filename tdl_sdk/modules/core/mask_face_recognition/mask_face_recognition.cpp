#include "mask_face_recognition.hpp"

#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/face/cvtdl_face_helper.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "face_utils.hpp"

#define FACE_ATTRIBUTE_MEAN (0.99609375)
#define FACE_ATTRIBUTE_SCALE (1 / 128.f)

#define FACE_OUT_NAME "pre_fc1"

namespace cvitdl {

MaskFaceRecognition::MaskFaceRecognition() : Core(CVI_MEM_DEVICE) {
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = FACE_ATTRIBUTE_SCALE;
    m_preprocess_param[0].mean[i] = FACE_ATTRIBUTE_MEAN;
  }
}

MaskFaceRecognition::~MaskFaceRecognition() {}

int MaskFaceRecognition::onModelOpened() { return allocateION(); }

int MaskFaceRecognition::onModelClosed() {
  releaseION();
  return CVI_TDL_SUCCESS;
}

CVI_S32 MaskFaceRecognition::allocateION() {
  CVI_SHAPE shape = getInputShape(0);
  if (CREATE_ION_HELPER(&m_wrap_frame, shape.dim[3], shape.dim[2], PIXEL_FORMAT_RGB_888, "tpu") !=
      CVI_SUCCESS) {
    LOGE("Cannot allocate ion for preprocess\n");
    return CVI_TDL_ERR_ALLOC_ION_FAIL;
  }
  return CVI_TDL_SUCCESS;
}

void MaskFaceRecognition::releaseION() {
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

int MaskFaceRecognition::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta) {
  uint32_t img_width = frame->stVFrame.u32Width;
  uint32_t img_height = frame->stVFrame.u32Height;
  cv::Mat image(img_height, img_width, CV_8UC3);
  frame->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.u32Length[0]);
  char *va_rgb = (char *)frame->stVFrame.pu8VirAddr[0];
  uint32_t dst_width = image.cols;
  uint32_t dst_height = image.rows;

  for (size_t i = 0; i < (size_t)dst_height; i++) {
    memcpy(image.ptr(i, 0), va_rgb + frame->stVFrame.u32Stride[0] * i, dst_width * 3);
  }

  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
  frame->stVFrame.pu8VirAddr[0] = NULL;

  for (uint32_t i = 0; i < meta->size; ++i) {
    cvtdl_face_info_t face_info =
        info_rescale_c(frame->stVFrame.u32Width, frame->stVFrame.u32Height, *meta, i);
    cv::Mat warp_image(cv::Size(m_wrap_frame.stVFrame.u32Width, m_wrap_frame.stVFrame.u32Height),
                       image.type(), m_wrap_frame.stVFrame.pu8VirAddr[0],
                       m_wrap_frame.stVFrame.u32Stride[0]);

    face_align(image, warp_image, face_info);
    CVI_SYS_IonFlushCache(m_wrap_frame.stVFrame.u64PhyAddr[0], m_wrap_frame.stVFrame.pu8VirAddr[0],
                          m_wrap_frame.stVFrame.u32Length[0]);

    std::vector<VIDEO_FRAME_INFO_S *> frames = {&m_wrap_frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }
    outputParser(meta, i);
    CVI_TDL_FreeCpp(&face_info);
  }
  return CVI_TDL_SUCCESS;
}

void MaskFaceRecognition::outputParser(cvtdl_face_t *meta, int meta_i) {
  int8_t *face_blob = getOutputRawPtr<int8_t>(FACE_OUT_NAME);
  size_t face_feature_size = getOutputTensorElem(FACE_OUT_NAME);

  CVI_TDL_MemAlloc(sizeof(int8_t), face_feature_size, TYPE_INT8, &meta->info[meta_i].feature);
  memcpy(meta->info[meta_i].feature.ptr, face_blob, face_feature_size);
}

}  // namespace cvitdl