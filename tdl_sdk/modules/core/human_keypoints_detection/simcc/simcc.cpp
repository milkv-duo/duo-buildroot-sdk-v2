
#include <algorithm>
#include <cmath>
#include <iterator>

#include <iostream>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "object_utils.hpp"
#include "simcc.hpp"

#define NUM_KEYPOINTS 17
#define EXPAND_RATIO 2.0f
#define MAX_NUM 5

namespace cvitdl {

Simcc::Simcc() {
  const float STD_RGB[3] = {255.0 * 0.229, 255.0 * 0.224, 255.0 * 0.225};
  const float MODEL_MEAN_RGB[3] = {0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0};
  for (int i = 0; i < 3; i++) {
    // default param
    m_preprocess_param[0].factor[i] = 1.0 / STD_RGB[i];
    m_preprocess_param[0].mean[i] = MODEL_MEAN_RGB[i] / STD_RGB[i];
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].rescale_type = RESCALE_NOASPECT;
  m_model_threshold = 3.0f;  // The model output scores are all greater than 1
}

int Simcc::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *obj_meta) {
  model_timer_.TicToc("before_run");

  int infer_num = std::min((int)obj_meta->size, MAX_NUM);
  for (int i = 0; i < infer_num; i++) {
    uint32_t img_width = stOutFrame->stVFrame.u32Width;
    uint32_t img_height = stOutFrame->stVFrame.u32Height;

    cvtdl_object_info_t obj_info = info_rescale_c(img_width, img_height, *obj_meta, i);

    int box_x1 = obj_info.bbox.x1;
    int box_y1 = obj_info.bbox.y1;
    uint32_t box_w = obj_info.bbox.x2 - obj_info.bbox.x1;
    uint32_t box_h = obj_info.bbox.y2 - obj_info.bbox.y1;

    // expand box with 1.25 scale
    box_x1 = box_x1 - box_w * 0.125;
    box_y1 = box_y1 - box_h * 0.125;
    box_w = box_w * 1.25;
    box_h = box_h * 1.25;

    if (box_x1 < 0) box_x1 = 0;
    if (box_y1 < 0) box_y1 = 0;
    if (box_x1 + box_w > img_width) {
      box_w = img_width - box_x1;
    }
    if (box_y1 + box_h > img_height) {
      box_h = img_height - box_y1;
    }

    CVI_TDL_FreeCpp(&obj_info);

    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {box_x1, box_y1, box_w, box_h};

    std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      LOGW("simcc pose inference failed\n");
      return ret;
    }

    CVI_SHAPE shape = getInputShape(0);

    std::vector<float> box = {(float)box_x1, (float)box_y1, (float)box_w, (float)box_h};
    outputParser(shape.dim[3], shape.dim[2], img_width, img_height, obj_meta, box, i);
  }
  model_timer_.TicToc("after_run");

  return CVI_TDL_SUCCESS;
}

void Simcc::outputParser(const float nn_width, const float nn_height, const int frame_width,
                         const int frame_height, cvtdl_object_t *obj, std::vector<float> &box,
                         int index) {
  float *data_x = getOutputRawPtr<float>(0);
  float *data_y = getOutputRawPtr<float>(1);

  CVI_SHAPE output0_shape = getOutputShape(0);
  CVI_SHAPE output1_shape = getOutputShape(1);

  obj->info[index].pedestrian_properity =
      (cvtdl_pedestrian_meta *)malloc(sizeof(cvtdl_pedestrian_meta));

  for (int i = 0; i < NUM_KEYPOINTS; i++) {
    float *score_start_x = data_x + i * output0_shape.dim[2];
    float *score_end_x = data_x + (i + 1) * output0_shape.dim[2];
    auto iter_x = std::max_element(score_start_x, score_end_x);
    uint32_t pos_x = iter_x - score_start_x;
    float score_x = *iter_x;

    float *score_start_y = data_y + i * output1_shape.dim[2];
    float *score_end_y = data_y + (i + 1) * output1_shape.dim[2];
    auto iter_y = std::max_element(score_start_y, score_end_y);
    uint32_t pos_y = iter_y - score_start_y;
    float score_y = *iter_y;

    obj->info[index].pedestrian_properity->pose_17.x[i] = (float)pos_x / EXPAND_RATIO;
    obj->info[index].pedestrian_properity->pose_17.y[i] = (float)pos_y / EXPAND_RATIO;
    obj->info[index].pedestrian_properity->pose_17.score[i] = std::min(score_x, score_y);
  }

  float scale_ratio, pad_w, pad_h;

  if (box[3] / box[2] > nn_height / nn_width) {
    scale_ratio = box[3] / nn_height;
    pad_h = 0;
    pad_w = (nn_width - box[2] / scale_ratio) / 2.0f;
  } else {
    scale_ratio = box[2] / nn_width;
    pad_w = 0;
    pad_h = (nn_height - box[3] / scale_ratio) / 2.0f;
  }

  for (int i = 0; i < NUM_KEYPOINTS; i++) {
    float x = obj->info[index].pedestrian_properity->pose_17.x[i];
    float y = obj->info[index].pedestrian_properity->pose_17.y[i];

    obj->info[index].pedestrian_properity->pose_17.x[i] = (x - pad_w) * scale_ratio + box[0];
    obj->info[index].pedestrian_properity->pose_17.y[i] = (y - pad_h) * scale_ratio + box[1];
  }
}
}  // namespace cvitdl