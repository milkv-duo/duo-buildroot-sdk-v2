#include "face_attribute_cls.hpp"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <error_msg.hpp>
#include <numeric>
#include <string>
#include <vector>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_comm.h"
#include "misc.hpp"
#include "object_utils.hpp"

#define FACE_ATTRIBUTE_CLS_FACTOR (float)(1 / 255.0)
#define FACE_ATTRIBUTE_CLS_MEAN (0.0)

namespace cvitdl {

FaceAttribute_cls::FaceAttribute_cls() : Core(CVI_MEM_DEVICE) {
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = FACE_ATTRIBUTE_CLS_FACTOR;
    m_preprocess_param[0].mean[i] = FACE_ATTRIBUTE_CLS_MEAN;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = true;
  m_preprocess_param[0].rescale_type = RESCALE_NOASPECT;
  m_preprocess_param[0].use_crop = true;
}
FaceAttribute_cls::~FaceAttribute_cls() {}

int FaceAttribute_cls::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face_attribute_cls_meta) {
  uint32_t img_width = frame->stVFrame.u32Width;
  uint32_t img_height = frame->stVFrame.u32Height;

  if (img_width == 112 && img_height == 112) {
    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {0, 0, 111, 111};

    std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }
    float *gender_score = getOutputRawPtr<float>(1);
    float *age = getOutputRawPtr<float>(0);
    float *glass = getOutputRawPtr<float>(2);
    float *mask_score = getOutputRawPtr<float>(3);

    CVI_TDL_MemAllocInit(1, 0,
                         face_attribute_cls_meta);  // CVI_TDL_MemAllocInit(const uint32_t size,
                                                    // const uint32_t pts_num, cvtdl_face_t *meta)

    face_attribute_cls_meta->info->gender_score = gender_score[0];
    face_attribute_cls_meta->info->age = age[0];
    face_attribute_cls_meta->info->glass = glass[0];
    face_attribute_cls_meta->info->mask_score = mask_score[0];

  } else {
    for (uint32_t i = 0; i < face_attribute_cls_meta->size; i++) {
      cvtdl_face_info_t face_info =
          info_rescale_c(img_width, img_height, *face_attribute_cls_meta, i);
      int box_x1 = face_info.bbox.x1;
      int box_y1 = face_info.bbox.y1;
      uint32_t box_w = face_info.bbox.x2 - face_info.bbox.x1;
      uint32_t box_h = face_info.bbox.y2 - face_info.bbox.y1;
      CVI_TDL_FreeCpp(&face_info);

      m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
      m_vpss_config[0].crop_attr.stCropRect = {box_x1, box_y1, box_w, box_h};

      std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
      int ret = run(frames);
      if (ret != CVI_TDL_SUCCESS) {
        return ret;
      }

      float *gender_score = getOutputRawPtr<float>(1);
      float *age = getOutputRawPtr<float>(0);
      float *glass = getOutputRawPtr<float>(2);
      float *mask_score = getOutputRawPtr<float>(3);

      // CVI_TDL_MemAllocInit(1, 0, face_attribute_cls_meta);   //CVI_TDL_MemAllocInit(const
      // uint32_t size, const uint32_t pts_num, cvtdl_face_t *meta)

      face_attribute_cls_meta->info[i].gender_score = gender_score[0];
      face_attribute_cls_meta->info[i].age = age[0];
      face_attribute_cls_meta->info[i].glass = glass[0];
      face_attribute_cls_meta->info[i].mask_score = mask_score[0];
      // meta->info[i].liveness_score = out_data[0];
    }
  }

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl