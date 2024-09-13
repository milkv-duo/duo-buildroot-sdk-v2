#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <iterator>

#include <core/core/cvtdl_errno.h>
#include <error_msg.hpp>
#include <iostream>
#include <string>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "object_utils.hpp"
#include "yolov5.hpp"

template <typename T>
int yolov5_argmax(T *ptr, int start_idx, int arr_len) {
  int max_idx = start_idx;
  for (int i = start_idx + 1; i < start_idx + arr_len; i++) {
    if (ptr[i] > ptr[max_idx]) {
      max_idx = i;
    }
  }
  return max_idx - start_idx;
}

int max_val(int x, int y) {
  if (x > y) return x;
  return y;
}

float sigmoid(float x) { return 1.0 / (1 + exp(-x)); }

namespace cvitdl {

static void convert_det_struct(const Detections &dets, cvtdl_object_t *obj, int im_height,
                               int im_width) {
  CVI_TDL_MemAllocInit(dets.size(), obj);
  obj->height = im_height;
  obj->width = im_width;
  memset(obj->info, 0, sizeof(cvtdl_object_info_t) * obj->size);

  for (uint32_t i = 0; i < obj->size; ++i) {
    obj->info[i].bbox.x1 = dets[i]->x1;
    obj->info[i].bbox.y1 = dets[i]->y1;
    obj->info[i].bbox.x2 = dets[i]->x2;
    obj->info[i].bbox.y2 = dets[i]->y2;
    obj->info[i].bbox.score = dets[i]->score;
    obj->info[i].classes = dets[i]->label;
  }
}

Yolov5::Yolov5() {
  // default param
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 0.003922;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;

  uint32_t *anchors = new uint32_t[18];
  uint32_t p_anchors[18] = {10, 13, 16,  30,  33, 23,  30,  61,  62,
                            45, 59, 119, 116, 90, 156, 198, 373, 326};
  memcpy(anchors, p_anchors, sizeof(p_anchors));
  alg_param_.anchors = anchors;
  alg_param_.anchor_len = 18;
  uint32_t *strides = new uint32_t[3];
  uint32_t p_strides[3] = {8, 16, 32};
  memcpy(strides, p_strides, sizeof(p_strides));
  alg_param_.strides = strides;
  alg_param_.stride_len = 3;
  alg_param_.cls = 80;
}

int Yolov5::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);
  int input_h = input_shape.dim[2];

  strides_.clear();
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;
    LOGI("%s: %d %d %d %d\n", oinfo.tensor_name.c_str(), output_shape.dim[0], output_shape.dim[1],
         output_shape.dim[2], output_shape.dim[3]);
    int feat_h = output_shape.dim[1];
    uint32_t channel = output_shape.dim[3];
    int stride_h = input_h / feat_h;

    if (channel == 1) {
      conf_out_names_[stride_h] = oinfo.tensor_name;
      strides_.push_back(stride_h);
    } else if (channel == 4) {
      box_out_names_[stride_h] = oinfo.tensor_name;
    } else if (channel == alg_param_.cls) {
      class_out_names_[stride_h] = oinfo.tensor_name;
    } else {
      LOGE("unmatched channel!\n");
      return CVI_TDL_FAILURE;
    }
  }

  for (size_t i = 0; i < strides_.size(); i++) {
    if (conf_out_names_.count(strides_[i]) == 0 || box_out_names_.count(strides_[i]) == 0 ||
        class_out_names_.count(strides_[i]) == 0) {
      return CVI_TDL_FAILURE;
    }
  }

  return CVI_TDL_SUCCESS;
}

Yolov5::~Yolov5() {}

uint32_t Yolov5::set_roi(Point_t &roi) {
  yolo_box.x1 = (float)(roi.x1);
  yolo_box.x2 = (float)(roi.x2);
  yolo_box.y1 = (float)(roi.y1);
  yolo_box.y2 = (float)(roi.y2);
  roi_flag = true;
  return 0;
}

int Yolov5::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  if (roi_flag == true) {
    VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
    memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));

    uint32_t bbox_w = yolo_box.x2 - yolo_box.x1;
    uint32_t bbox_h = yolo_box.y2 - yolo_box.y1;
    vpssCropImage(srcFrame, f, yolo_box, bbox_w, bbox_h, PIXEL_FORMAT_RGB_888);
    std::vector<VIDEO_FRAME_INFO_S *> frames = {f};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      LOGE("Yolov5 run inference failed!\n");
      return ret;
    }

    CVI_SHAPE shape = getInputShape(0);

    outputParser(shape.dim[3], shape.dim[2], f->stVFrame.u32Width, f->stVFrame.u32Height, obj_meta);

    if (f->stVFrame.u64PhyAddr[0] != 0) {
      mp_vpss_inst->releaseFrame(f, 0);
    }
    delete f;
  } else {
    std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};

    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      LOGE("Yolov5 run inference failed!\n");
      return ret;
    }

    CVI_SHAPE shape = getInputShape(0);

    outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
                 srcFrame->stVFrame.u32Height, obj_meta);
  }

  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void xywh2xxyy(float x, float y, float w, float h, PtrDectRect &det) {
  det->x1 = x - w / 2;
  det->y1 = y - h / 2;
  det->x2 = x + w / 2;
  det->y2 = y + h / 2;
}

template <typename T>
void parseDet(T *ptr, int start_idx, float qscale, int cls, float box_prob, int grid_x, int grid_y,
              int stride_x, int stride_y, float pw, float ph, int im_height, int im_width,
              Detections &vec_obj) {
  float sigmoid_x = sigmoid(ptr[start_idx] * qscale);
  float sigmoid_y = sigmoid(ptr[start_idx + 1] * qscale);
  float sigmoid_w = sigmoid(ptr[start_idx + 2] * qscale);
  float sigmoid_h = sigmoid(ptr[start_idx + 3] * qscale);
  float object_score = box_prob;

  PtrDectRect det = std::make_shared<object_detect_rect_t>();
  det->score = object_score;
  det->label = cls;
  // decode predicted bounding box of each grid to whole image
  float x = (2 * sigmoid_x - 0.5 + (float)grid_x) * (float)stride_x;
  float y = (2 * sigmoid_y - 0.5 + (float)grid_y) * (float)stride_y;
  float w = pow((sigmoid_w * 2), 2) * pw;
  float h = pow((sigmoid_h * 2), 2) * ph;
  xywh2xxyy(x, y, w, h, det);

  clip_bbox(im_height, im_width, det);
  float box_width = det->x2 - det->x1;
  float box_height = det->y2 - det->y1;
  if (box_width > 1 && box_height > 1) {
    vec_obj.push_back(det);
  }
}

void Yolov5::generate_yolov5_proposals(Detections &vec_obj) {
  CVI_SHAPE shape = getInputShape(0);
  int target_w = shape.dim[3];
  int target_h = shape.dim[2];
  int anchor_pos = 0;
  for (uint32_t i = 0; i < strides_.size(); i++) {
    int stride = alg_param_.strides[i];
    int num_grid_w = target_w / stride;
    int num_grid_h = target_h / stride;

    int basic_pos_class = 0;
    int basic_pos_object = 0;
    int basic_pos_box = 0;

    TensorInfo oinfo_class = getOutputTensorInfo(class_out_names_[stride]);
    int num_per_pixel_class = oinfo_class.tensor_size / oinfo_class.tensor_elem;
    float qscale_class = num_per_pixel_class == 1 ? oinfo_class.qscale : 1;
    int8_t *ptr_int8_class = static_cast<int8_t *>(oinfo_class.raw_pointer);
    float *ptr_float_class = static_cast<float *>(oinfo_class.raw_pointer);

    TensorInfo oinfo_object = getOutputTensorInfo(conf_out_names_[stride]);
    int num_per_pixel_object = oinfo_object.tensor_size / oinfo_object.tensor_elem;
    float qscale_object = num_per_pixel_object == 1 ? oinfo_object.qscale : 1;
    int8_t *ptr_int8_object = static_cast<int8_t *>(oinfo_object.raw_pointer);
    float *ptr_float_object = static_cast<float *>(oinfo_object.raw_pointer);

    TensorInfo oinfo_box = getOutputTensorInfo(box_out_names_[stride]);
    int num_per_pixel_box = oinfo_box.tensor_size / oinfo_box.tensor_elem;
    float qscale_box = num_per_pixel_box == 1 ? oinfo_box.qscale : 1;
    int8_t *ptr_int8_box = static_cast<int8_t *>(oinfo_box.raw_pointer);
    float *ptr_float_box = static_cast<float *>(oinfo_box.raw_pointer);

    CVI_SHAPE output_shape = oinfo_class.shape;
    uint32_t anchor_len = output_shape.dim[0];
    for (uint32_t anchor_idx = 0; anchor_idx < anchor_len; anchor_idx++) {
      uint32_t *anchors = alg_param_.anchors + anchor_pos;

      float pw = anchors[0];
      float ph = anchors[1];

      for (int grid_y = 0; grid_y < num_grid_h; grid_y++) {
        for (int grid_x = 0; grid_x < num_grid_w; grid_x++) {
          float class_score = 0.0f;
          float box_objectness = 0.0f;
          int label = 0;

          // parse object conf
          if (num_per_pixel_class == 1) {
            box_objectness = ptr_int8_object[basic_pos_object] * qscale_object;
          } else {
            box_objectness = ptr_float_object[basic_pos_object];
          }

          // parse class score
          if (num_per_pixel_object == 1) {
            label = yolov5_argmax<int8_t>(ptr_int8_class, basic_pos_class, alg_param_.cls);
            class_score = ptr_int8_class[basic_pos_class + label] * qscale_class;
          } else {
            label = yolov5_argmax<float>(ptr_float_class, basic_pos_class, alg_param_.cls);
            class_score = ptr_float_class[basic_pos_class + label];
          }

          box_objectness = sigmoid(box_objectness);
          class_score = sigmoid(class_score);
          float box_prob = box_objectness * class_score;

          if (box_prob < m_model_threshold) {
            basic_pos_class += alg_param_.cls;
            basic_pos_box += 4;
            basic_pos_object += 1;
            continue;
          }

          if (num_per_pixel_box == 1) {
            parseDet<int8_t>(ptr_int8_box, basic_pos_box, qscale_box, label, box_prob, grid_x,
                             grid_y, stride, stride, pw, ph, target_w, target_h, vec_obj);
          } else {
            parseDet<float>(ptr_float_box, basic_pos_box, qscale_box, label, box_prob, grid_x,
                            grid_y, stride, stride, pw, ph, target_w, target_h, vec_obj);
          }

          basic_pos_class += alg_param_.cls;
          basic_pos_box += 4;
          basic_pos_object += 1;
        }
      }
      anchor_pos += 2;
    }
  }
}

void Yolov5::Yolov5PostProcess(Detections &dets, int frame_width, int frame_height,
                               cvtdl_object_t *obj_meta) {
  Detections final_dets = nms_multi_class(dets, m_model_nms_threshold);
  CVI_SHAPE shape = getInputShape(0);
  convert_det_struct(final_dets, obj_meta, shape.dim[2], shape.dim[3]);
  // rescale bounding box to original image
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj_meta->size; ++i) {
      obj_meta->info[i].bbox =
          box_rescale(frame_width, frame_height, obj_meta->width, obj_meta->height,
                      obj_meta->info[i].bbox, meta_rescale_type_e::RESCALE_CENTER);
    }
    obj_meta->width = frame_width;
    obj_meta->height = frame_height;
  }
}

void Yolov5::outputParser(const int image_width, const int image_height, const int frame_width,
                          const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;
  generate_yolov5_proposals(vec_obj);

  Yolov5PostProcess(vec_obj, frame_width, frame_height, obj_meta);
}
// namespace cvitdl
}  // namespace cvitdl
