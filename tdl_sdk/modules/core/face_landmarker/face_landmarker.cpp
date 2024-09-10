#include "face_landmarker.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"

#define NAME_SCORE "fc1_Gemm_dequant"

namespace cvitdl {

FaceLandmarker::FaceLandmarker() : Core(CVI_MEM_DEVICE) {
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 0.99;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].use_crop = true;
}

FaceLandmarker::~FaceLandmarker() {}

int FaceLandmarker::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
    LOGE("Error: pixel format not match PIXEL_FORMAT_RGB_888.\n");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  uint32_t img_width = frame->stVFrame.u32Width;
  uint32_t img_height = frame->stVFrame.u32Height;

  // just one face
  for (uint32_t i = 0; i < 1; i++) {
    cvtdl_face_info_t face_info = info_rescale_c(img_width, img_height, *meta, i);

    int max_side = 0;
    Preprocessing(&face_info, &max_side, img_width, img_height);
    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {(int)face_info.bbox.x1, (int)face_info.bbox.y1,
                                             (uint32_t)(face_info.bbox.x2 - face_info.bbox.x1),
                                             (uint32_t)(face_info.bbox.y2 - face_info.bbox.y1)};

    std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    const TensorInfo &tinfo = getOutputTensorInfo(NAME_SCORE);
    float *pts = tinfo.get<float>();
    size_t pts_size = tinfo.tensor_elem;
    cvtdl_pts_t landmarks_106;
    landmarks_106.size = landmark_num;
    landmarks_106.x = (float *)malloc(sizeof(float) * landmark_num);
    landmarks_106.y = (float *)malloc(sizeof(float) * landmark_num);

    int half_max_side = max_side / 2;
    // change the [1,-1] coordination to image coordination
    for (int i = 0; i < (int)pts_size / 2; ++i) {
      landmarks_106.x[i] = (pts[2 * i] + 1.0f) * half_max_side + face_info.bbox.x1;
      landmarks_106.y[i] = (pts[2 * i + 1] + 1.0f) * half_max_side + face_info.bbox.y1;
    }
    cvtdl_pts_t landmarks_5;
    landmarks_5.size = 5;
    landmarks_5.x = (float *)malloc(sizeof(float) * landmarks_5.size);
    landmarks_5.y = (float *)malloc(sizeof(float) * landmarks_5.size);
    landmarks_5.x[0] = landmarks_106.x[38];
    landmarks_5.x[1] = landmarks_106.x[88];
    landmarks_5.x[2] = landmarks_106.x[80];
    landmarks_5.x[3] = landmarks_106.x[65];
    landmarks_5.x[4] = landmarks_106.x[69];
    landmarks_5.y[0] = landmarks_106.y[38];
    landmarks_5.y[1] = landmarks_106.y[88];
    landmarks_5.y[2] = landmarks_106.y[80];
    landmarks_5.y[3] = landmarks_106.y[65];
    landmarks_5.y[4] = landmarks_106.y[69];

    meta->dms->landmarks_106 = landmarks_106;
    meta->dms->landmarks_5 = landmarks_5;
    CVI_TDL_FreeCpp(&face_info);
  }
  return CVI_TDL_SUCCESS;
}

void FaceLandmarker::Preprocessing(cvtdl_face_info_t *face_info, int *max_side, int img_width,
                                   int img_height) {
  // scale to 1.5 times
  float half_width = (face_info->bbox.x2 - face_info->bbox.x1) / 4;
  float half_height = (face_info->bbox.y2 - face_info->bbox.y1) / 4;
  face_info->bbox.x1 = face_info->bbox.x1 - half_width;
  face_info->bbox.x2 = face_info->bbox.x2 + half_width;
  face_info->bbox.y1 = face_info->bbox.y1 - half_height;
  face_info->bbox.y2 = face_info->bbox.y2 + half_height;

  // square the roi
  *max_side =
      std::max(face_info->bbox.x2 - face_info->bbox.x1, face_info->bbox.y2 - face_info->bbox.y1);
  int offset_x = (*max_side - (int)face_info->bbox.x2 + face_info->bbox.x1) / 2;
  int offset_y = (*max_side - (int)face_info->bbox.y2 + face_info->bbox.y1) / 2;
  face_info->bbox.x1 = std::max(((int)face_info->bbox.x1 - offset_x), 0);
  face_info->bbox.x2 = std::min(((int)face_info->bbox.x1 + *max_side), img_width);
  face_info->bbox.y1 = std::max(((int)face_info->bbox.y1 - offset_y), 0);
  face_info->bbox.y2 = std::min(((int)face_info->bbox.y1 + *max_side), img_height);
}
}  // namespace cvitdl
