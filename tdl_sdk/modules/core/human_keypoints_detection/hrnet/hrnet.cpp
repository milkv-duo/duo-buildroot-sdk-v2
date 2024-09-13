
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
#include "hrnet.hpp"
#include "object_utils.hpp"

// #define R_SCALE (float)(1.0 / 58.395)
// #define G_SCALE (float)(1.0 / 57.12)
// #define B_SCALE (float)(1.0 / 57.375)
// #define R_MEAN 123.675
// #define G_MEAN 116.28
// #define B_MEAN 103.53

#define NUM_KEYPOINTS 17
#define EXPAND_RATIO 2.0f
#define MAX_NUM 20

namespace cvitdl {

Hrnet::Hrnet() {
  const float STD_RGB[3] = {255.0 * 0.229, 255.0 * 0.224, 255.0 * 0.225};
  const float MODEL_MEAN_RGB[3] = {0.485 * 255.0, 0.456 * 255.0, 0.406 * 255.0};
  for (int i = 0; i < 3; i++) {
    // default param
    m_preprocess_param[0].factor[i] = 1.0 / STD_RGB[i];
    m_preprocess_param[0].mean[i] = MODEL_MEAN_RGB[i] / STD_RGB[i];
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].rescale_type = RESCALE_NOASPECT;
  m_model_threshold = 0.3f;
}

float getSignedValue(float value) {
  if (value < 0) {
    return -1.0;
  } else if (value == 0.0) {
    return 0.0;
  } else {
    return 1.0;
  }
}

int Hrnet::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *obj_meta) {
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
      LOGW("Hrnet pose inference failed\n");
      return ret;
    }

    CVI_SHAPE shape = getInputShape(0);

    std::vector<float> box = {(float)box_x1, (float)box_y1, (float)box_w, (float)box_h};
    outputParser(shape.dim[3], shape.dim[2], img_width, img_height, obj_meta, box, i);
  }
  model_timer_.TicToc("after_run");

  return CVI_TDL_SUCCESS;
}

void Hrnet::outputParser(const float nn_width, const float nn_height, const int frame_width,
                         const int frame_height, cvtdl_object_t *obj, std::vector<float> &box,
                         int index) {
  TensorInfo oinfo = getOutputTensorInfo(0);
  CVI_SHAPE output_shape = oinfo.shape;

  int num_per_pixel = oinfo.tensor_size / oinfo.tensor_elem;
  float qscale = num_per_pixel == 1 ? oinfo.qscale : 1;

  obj->info[index].pedestrian_properity =
      (cvtdl_pedestrian_meta *)malloc(sizeof(cvtdl_pedestrian_meta));
  int batch_size = output_shape.dim[0];
  int num_joints = output_shape.dim[1];
  int height = output_shape.dim[2];
  int width = output_shape.dim[3];

  if (qscale == 1) {
    float *ptr_float = static_cast<float *>(oinfo.raw_pointer);
    getMaxPreds(batch_size, num_joints, height, width, ptr_float, preds, maxvals, qscale);
    for (int i = 0; i < num_joints; i++) {
      int row = static_cast<int>(std::floor(preds[2 * i] + 0.5));
      int col = static_cast<int>(std::floor(preds[2 * i + 1] + 0.5));
      if (1 < col && col < width - 1 && 1 < row && row < height - 1) {
        float diff_x = ptr_float[i * height * width + row * width + col + 1] * qscale -
                       ptr_float[i * height * width + row * width + col - 1] * qscale;
        float diff_y = ptr_float[i * height * width + (row + 1) * width + col] * qscale -
                       ptr_float[i * height * width + (row - 1) * width + col] * qscale;
        std::cout << "before" << preds[2 * i] << "," << preds[2 * i + 1] << std::endl;

        diff_x = getSignedValue(diff_x);
        diff_y = getSignedValue(diff_y);
        preds[2 * i] += diff_x * 0.25;
        preds[2 * i + 1] += diff_y * 0.25;

        std::cout << "after" << preds[2 * i] << "," << preds[2 * i + 1] << std::endl;
        std::cout << preds[2 * i] << "," << preds[2 * i + 1] << std::endl;
        std::cout << "float ptr: diff_x=" << diff_x * 0.25 << ", diff_y=" << diff_y << std::endl;
      }
      obj->info[index].pedestrian_properity->pose_17.x[i] = preds[2 * i + 1] * 4;
      obj->info[index].pedestrian_properity->pose_17.y[i] = preds[2 * i] * 4;
      obj->info[index].pedestrian_properity->pose_17.score[i] = maxvals[i];
    }
  } else {
    int8_t *ptr_int8 = static_cast<int8_t *>(oinfo.raw_pointer);
    getMaxPreds(batch_size, num_joints, height, width, ptr_int8, preds, maxvals, qscale);
    for (int i = 0; i < num_joints; i++) {
      int row = static_cast<int>(std::floor(preds[2 * i] + 0.5));
      int col = static_cast<int>(std::floor(preds[2 * i + 1] + 0.5));
      if (1 < col && col < width - 1 && 1 < row && row < height - 1) {
        float diff_x = ptr_int8[i * height * width + row * width + col + 1] * qscale -
                       ptr_int8[i * height * width + row * width + col - 1] * qscale;
        float diff_y = ptr_int8[i * height * width + (row + 1) * width + col] * qscale -
                       ptr_int8[i * height * width + (row - 1) * width + col] * qscale;
        diff_x = getSignedValue(diff_x);
        diff_y = getSignedValue(diff_y);
        std::cout << "before" << preds[2 * i] << "," << preds[2 * i + 1] << std::endl;
        preds[2 * i] += diff_x * 0.25;
        preds[2 * i + 1] += diff_y * 0.25;
        std::cout << "after" << preds[2 * i] << "," << preds[2 * i + 1] << std::endl;
        std::cout << "int ptr: diff_x=" << diff_x * 0.25 << ", diff_y=" << diff_y << std::endl;
      }
      obj->info[index].pedestrian_properity->pose_17.x[i] = preds[2 * i + 1] * 4;
      obj->info[index].pedestrian_properity->pose_17.y[i] = preds[2 * i] * 4;
      obj->info[index].pedestrian_properity->pose_17.score[i] = maxvals[i];
    }
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

template <typename T>
void Hrnet::getMaxPreds(int batch_size, int num_joints, int height, int width, T *ptr_data,
                        std::vector<float> &preds, std::vector<float> &maxvals, float qscale) {
  preds.clear();
  maxvals.clear();
  for (int i = 0; i < batch_size; i++) {
    for (int j = 0; j < num_joints; j++) {
      float maxval = 0.0;
      float maxrow = 0;
      float maxcol = 0;
      for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
          float val =
              ptr_data[i * num_joints * height * width + j * height * width + h * width + w] *
              qscale;
          if (val > maxval) {
            maxval = val;
            maxrow = h * 1.0;
            maxcol = w * 1.0;
          }
        }
      }
      maxvals.push_back(maxval);
      preds.push_back(maxrow);
      preds.push_back(maxcol);
      // std::cout<<"row: "<<maxrow<<", col: "<<maxcol<<std::endl;
    }
  }
}

}  // namespace cvitdl