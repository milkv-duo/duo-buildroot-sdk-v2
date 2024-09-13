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
#include "cvi_sys.h"
#include "object_utils.hpp"
#include "yolov6.hpp"

#define SCALE 0.003922
#define MEAN 0

namespace cvitdl {

template <typename T>
inline void parse_cls_info(T *p_cls_ptr, int num_cls, int anchor_idx, float qscale,
                           float *p_max_logit, int *p_max_cls) {
  int max_logit_c = -1;
  float max_logit = -1000;
  for (int c = 0; c < num_cls; c++) {
    float logit = p_cls_ptr[num_cls * anchor_idx + c];
    if (logit > max_logit) {
      max_logit = logit;
      max_logit_c = c;
    }
  }
  *p_max_logit = max_logit * qscale;
  *p_max_cls = max_logit_c;
}

float sigmoid(float x) { return 1.0 / (1.0 + exp(-x)); }

int max_val(int x, int y) { return x > y ? x : y; }

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

Yolov6::Yolov6() {
  // defalut param

  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 0.003922;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  alg_param_.cls = 80;
}

int Yolov6::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                           VPSSConfig &vpss_config) {
  auto &vpssChnAttr = vpss_config.chn_attr;
  auto &factor = vpssChnAttr.stNormalize.factor;
  auto &mean = vpssChnAttr.stNormalize.mean;

  // set dump config
  vpssChnAttr.stNormalize.bEnable = false;
  vpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;

  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
                        vpssChnAttr.u32Width, vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                        factor, mean, false);
  int ret = mp_vpss_inst->sendFrame(srcFrame, &vpssChnAttr, &vpss_config.chn_coeff, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("vpssPreprocess Send frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("get frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }
  return CVI_TDL_SUCCESS;
}

int Yolov6::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);
  int input_h = input_shape.dim[2];
  strides.clear();
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;

    int feat_h = output_shape.dim[1];
    uint32_t channel = output_shape.dim[3];
    int stride_h = input_h / feat_h;

    if (channel == alg_param_.cls) {
      class_out_names[stride_h] = oinfo.tensor_name;
      strides.push_back(stride_h);
      LOGI("parase class decode branch: %s, channel: %d\n", oinfo.tensor_name.c_str(), channel);
    } else {
      bbox_out_names[stride_h] = oinfo.tensor_name;
    }
  }

  for (size_t i = 0; i < strides.size(); i++) {
    if (!class_out_names.count(strides[i]) || !bbox_out_names.count(strides[i])) {
      return CVI_TDL_FAILURE;
    }
  }

  return CVI_TDL_SUCCESS;
}

Yolov6::~Yolov6() {}

int Yolov6::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("Yolov6 run inference failed!\n");
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);

  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void Yolov6::decode_bbox_feature_map(int stride, int anchor_idx, std::vector<float> &decode_box) {
  std::string box_name = bbox_out_names[stride];
  TensorInfo boxinfo = getOutputTensorInfo(box_name);
  CVI_SHAPE input_shape = getInputShape(0);
  int box_val_num = 4;

  int num_per_pixel = boxinfo.tensor_size / boxinfo.tensor_elem;
  int8_t *p_box_int8 = static_cast<int8_t *>(boxinfo.raw_pointer);
  float *p_box_float = static_cast<float *>(boxinfo.raw_pointer);

  int32_t feat_w = boxinfo.shape.dim[1];
  int32_t feat_h = boxinfo.shape.dim[2];

  int stride_x = input_shape.dim[2] / feat_w;
  int stride_y = input_shape.dim[3] / feat_h;

  int anchor_y = anchor_idx / feat_w;
  int anchor_x = anchor_idx % feat_w;

  float grid_y = anchor_y + 0.5;
  float grid_x = anchor_x + 0.5;

  std::vector<float> box_vals;
  for (int i = 0; i < box_val_num; i++) {
    if (num_per_pixel == 1) {
      box_vals.push_back(p_box_int8[anchor_idx * 4 + i] * boxinfo.qscale);
    } else {
      box_vals.push_back(p_box_float[anchor_idx * 4 + i]);
    }
  }

  std::vector<float> box = {(grid_x - box_vals[0]) * stride_x, (grid_y - box_vals[1]) * stride_y,
                            (grid_x + box_vals[2]) * stride_x, (grid_y + box_vals[3]) * stride_y};

  decode_box = box;
}

void Yolov6::clip_bbox(int frame_width, int frame_height, cvtdl_bbox_t *bbox) {
  if (bbox->x1 < 0) {
    bbox->x1 = 0;
  } else if (bbox->x1 > frame_width) {
    bbox->x1 = frame_width;
  }

  if (bbox->x2 < 0) {
    bbox->x2 = 0;
  } else if (bbox->x2 > frame_width) {
    bbox->x2 = frame_width;
  }

  if (bbox->y1 < 0) {
    bbox->y1 = 0;
  } else if (bbox->y1 > frame_height) {
    bbox->y1 = frame_height;
  }

  if (bbox->y2 < 0) {
    bbox->y2 = 0;
  } else if (bbox->y2 > frame_height) {
    bbox->y2 = frame_height;
  }
}
cvtdl_bbox_t Yolov6::boxRescale(int frame_width, int frame_height, int width, int height,
                                cvtdl_bbox_t bbox) {
  cvtdl_bbox_t rescale_bbox;
  int max_board = max_val(frame_width, frame_height);
  float ratio = float(max_board) / float(width);
  rescale_bbox.x1 = int(bbox.x1 * ratio);
  rescale_bbox.x2 = int(bbox.x2 * ratio);
  rescale_bbox.y1 = int(bbox.y1 * ratio);
  rescale_bbox.y2 = int(bbox.y2 * ratio);
  rescale_bbox.score = bbox.score;
  clip_bbox(frame_width, frame_height, &rescale_bbox);
  return rescale_bbox;
}

void Yolov6::postProcess(Detections &dets, int frame_width, int frame_height,
                         cvtdl_object_t *obj_meta) {
  Detections final_dets = nms_multi_class(dets, m_model_nms_threshold);
  CVI_SHAPE shape = getInputShape(0);
  convert_det_struct(final_dets, obj_meta, shape.dim[2], shape.dim[3]);
  // rescale bounding box to original image
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj_meta->size; ++i) {
      obj_meta->info[i].bbox = boxRescale(frame_width, frame_height, obj_meta->width,
                                          obj_meta->height, obj_meta->info[i].bbox);
    }
    obj_meta->width = frame_width;
    obj_meta->height = frame_height;
  }
}

void Yolov6::outputParser(const int iamge_width, const int image_height, const int frame_width,
                          const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;

  float inverse_th = std::log(m_model_threshold / (1 - m_model_threshold));

  for (size_t i = 0; i < strides.size(); i++) {
    int stride = strides[i];
    std::string cls_name = class_out_names[stride];

    TensorInfo classinfo = getOutputTensorInfo(cls_name);
    int num_per_pixel = classinfo.tensor_size / classinfo.tensor_elem;
    int8_t *p_cls_int8 = static_cast<int8_t *>(classinfo.raw_pointer);
    float *p_cls_float = static_cast<float *>(classinfo.raw_pointer);

    int num_cls = alg_param_.cls;
    int num_anchor = classinfo.shape.dim[1] * classinfo.shape.dim[2];
    float cls_qscale = num_per_pixel == 1 ? classinfo.qscale : 1;

    for (int j = 0; j < num_anchor; j++) {
      int max_logit_c = -1;
      float max_logit = -1000;
      if (num_per_pixel == 1) {
        parse_cls_info<int8_t>(p_cls_int8, num_cls, j, cls_qscale, &max_logit, &max_logit_c);
      } else {
        parse_cls_info<float>(p_cls_float, num_cls, j, cls_qscale, &max_logit, &max_logit_c);
      }
      if (max_logit < inverse_th) {
        continue;
      }

      float score = sigmoid(max_logit);
      std::vector<float> box;
      decode_bbox_feature_map(stride, j, box);

      PtrDectRect det = std::make_shared<object_detect_rect_t>();
      det->score = score;
      det->x1 = box[0];
      det->y1 = box[1];
      det->x2 = box[2];
      det->y2 = box[3];
      det->label = max_logit_c;
      vec_obj.push_back(det);
    }
  }
  postProcess(vec_obj, frame_width, frame_height, obj_meta);
}
// namespace cvitdl
}  // namespace cvitdl