#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <iterator>

#include <core/core/cvtdl_errno.h>
#include <error_msg.hpp>
#include <fstream>
#include <iostream>
#include "Eigen/Core"
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "object_utils.hpp"
#include "opencv2/opencv.hpp"
#include "yolov8_seg.hpp"

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

template <typename T>
inline void parse_cls_info(T *p_cls_ptr, int num_anchor, int num_cls, int anchor_idx,
                           int cls_offset, float qscale, float *p_max_logit, int *p_max_cls) {
  int max_logit_c = -1;
  float max_logit = -1000;
  for (int c = 0; c < num_cls; c++) {
    float logit = p_cls_ptr[(c + cls_offset) * num_anchor + anchor_idx];
    if (logit > max_logit) {
      max_logit = logit;
      max_logit_c = c;
    }
  }
  *p_max_logit = max_logit * qscale;
  *p_max_cls = max_logit_c;
}
YoloV8Seg::YoloV8Seg() {
  // Default value
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 0.003922;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  alg_param_.cls = 0;
}
YoloV8Seg::YoloV8Seg(PAIR_INT yolov8_pair) {
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = 0.003922;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;

  m_box_channel_ = yolov8_pair.first;
  alg_param_.cls = yolov8_pair.second;
}
YoloV8Seg::~YoloV8Seg() {}

// identity output branches
int YoloV8Seg::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);
  int input_h = input_shape.dim[2];
  int input_w = input_shape.dim[3];
  strides.clear();
  size_t num_output = getNumOutputTensor();
  for (size_t j = 0; j < num_output; j++) {
    CVI_SHAPE oj = getOutputShape(j);
    TensorInfo oinfo = getOutputTensorInfo(j);
    int feat_h = oj.dim[2];
    int feat_w = oj.dim[3];
    int channel = oj.dim[1];
    int stride_h = input_h / feat_h;
    int stride_w = input_w / feat_w;

    if (stride_h == 0 && num_output == 2) {
      if (channel == alg_param_.cls) {
        class_out_names[stride_h] = oinfo.tensor_name;
        strides.push_back(stride_h);
        LOGI("parse class decode branch:%s,channel:%d\n", oinfo.tensor_name.c_str(), channel);
      } else {
        bbox_out_names[stride_h] = oinfo.tensor_name;
        LOGI("parse box decode branch:%s,channel:%d\n", oinfo.tensor_name.c_str(), channel);
      }
      continue;
    }

    if (stride_h != stride_w) {
      LOGE("stride not equal,stridew:%d,strideh:%d,featw:%d,feath:%d\n", stride_w, stride_h, feat_w,
           feat_h);
      return CVI_FAILURE;
    }
    if (channel == m_box_channel_) {
      bbox_out_names[stride_h] = oinfo.tensor_name;
      strides.push_back(stride_h);
      LOGI("parse box branch,name:%s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else if (alg_param_.cls == 0 && num_output == 6) {
      alg_param_.cls = channel;
      class_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse class branch,name:%s,stride:%d,num_cls:%d\n", oinfo.tensor_name.c_str(), stride_h,
           channel);
    } else if (channel == alg_param_.cls) {
      class_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse class branch,name:%s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else if (channel == m_mask_channel_) {
      mask_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse mask branch,name:%s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else if (channel == (m_box_channel_ + alg_param_.cls)) {
      strides.push_back(stride_h);
      bbox_class_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse box+class branch,name: %s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else {
      LOGE("unexpected branch:%s,channel:%d\n", oinfo.tensor_name.c_str(), channel);
      return CVI_FAILURE;
    }
  }
  if (!mask_out_names.empty()) {
    auto min_it = mask_out_names.begin();
    for (auto it = mask_out_names.begin(); it != mask_out_names.end(); ++it) {
      if (it->first < min_it->first) {
        min_it = it;
      }
    }
    // copy the entry of the minimum key to proto_out_name
    proto_out_names[min_it->first] = min_it->second;
    // remove the entry with the smallest key from mask_out_name
    mask_out_names.erase(min_it);
  }

  return CVI_TDL_SUCCESS;
}

// the bbox featuremap shape is b x 4*regmax x h x w
void YoloV8Seg::decode_bbox_feature_map(int stride, int anchor_idx,
                                        std::vector<float> &decode_box) {
  std::string box_name;
  if (bbox_out_names.count(stride)) {
    box_name = bbox_out_names[stride];
  } else if (bbox_class_out_names.count(stride)) {
    box_name = bbox_class_out_names[stride];
  }
  TensorInfo boxinfo = getOutputTensorInfo(box_name);

  int num_per_pixel = boxinfo.tensor_size / boxinfo.tensor_elem;
  int8_t *p_box_int8 = static_cast<int8_t *>(boxinfo.raw_pointer);
  float *p_box_float = static_cast<float *>(boxinfo.raw_pointer);
  int num_channel = boxinfo.shape.dim[1];
  int num_anchor = boxinfo.shape.dim[2] * boxinfo.shape.dim[3];
  int box_val_num = 4;
  int reg_max = 16;
  if (m_box_channel_ != box_val_num * reg_max) {
    LOGE("box channel size not ok,got:%d\n", num_channel);
  }

  int32_t feat_w = boxinfo.shape.dim[3];

  int anchor_y = anchor_idx / feat_w;
  int anchor_x = anchor_idx % feat_w;

  // LOGI("box numchannel:%d,numperpixel:%d,featw:%d,feath:%d,anchory:%d,anchorx:%d,numanchor:%d\n",
  //      num_channel, num_per_pixel, feat_w, feat_h, anchor_y, anchor_x, num_anchor);

  float grid_y = anchor_y + 0.5;
  float grid_x = anchor_x + 0.5;

  std::vector<float> grid_logits;  // 4x16
  if (num_per_pixel == 1) {
    for (int c = 0; c < m_box_channel_; c++) {
      grid_logits.push_back(p_box_int8[c * num_anchor + anchor_idx] * boxinfo.qscale);
    }
  } else {
    for (int c = 0; c < m_box_channel_; c++) {
      grid_logits.push_back(p_box_float[c * num_anchor + anchor_idx]);
    }
  }

  // compute softmax and accumulate val per 16
  std::vector<float> box_vals;
  for (int i = 0; i < box_val_num; i++) {
    float sum_softmax = 0;
    float sum_val = 0;
    for (int j = 0; j < reg_max; j++) {
      float expv = exp(grid_logits[i * reg_max + j]);
      sum_softmax += expv;
      sum_val += expv * j;
    }
    sum_softmax = sum_val / sum_softmax;
    box_vals.push_back(sum_softmax);
  }

  std::vector<float> box = {(grid_x - box_vals[0]) * stride, (grid_y - box_vals[1]) * stride,
                            (grid_x + box_vals[2]) * stride, (grid_y + box_vals[3]) * stride};
  decode_box = box;
}

int YoloV8Seg::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGW("YoloV8Detection run inference failed\n");
    return ret;
  }
  CVI_SHAPE shape = getInputShape(0);
  if (strides.size() == 3) {
    outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
                 srcFrame->stVFrame.u32Height, obj_meta);
  } else {
    LOGE("unexpected strides size:%s\n", strides.size());
    return CVI_FAILURE;
  }

  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void YoloV8Seg::outputParser(const int image_width, const int image_height, const int frame_width,
                             const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;
  CVI_SHAPE shape = getInputShape(0);

  std::vector<std::pair<int, int>> final_dets_id;
  detPostProcess(vec_obj, obj_meta, final_dets_id);

  int num_dets_to_crop = final_dets_id.size();

  Eigen::MatrixXf mask_map(num_dets_to_crop, m_mask_channel_);

  int row = 0;

  // extract the corresponding mask_map based on the ID of the final detection box
  for (const auto &pair : final_dets_id) {
    std::string mask_name;
    mask_name = mask_out_names[pair.first];
    TensorInfo maskinfo = getOutputTensorInfo(mask_name);
    int num_map = maskinfo.shape.dim[2] * maskinfo.shape.dim[3];
    int num_per_pixel = maskinfo.tensor_size / maskinfo.tensor_elem;
    int8_t *p_mask_int8 = static_cast<int8_t *>(maskinfo.raw_pointer);
    float *p_mask_float = static_cast<float *>(maskinfo.raw_pointer);
    if (num_per_pixel == 1) {
      for (int c = 0; c < m_mask_channel_; c++) {
        mask_map(row, c) = p_mask_int8[c * num_map + pair.second] * maskinfo.qscale;
      }
    } else {
      for (int c = 0; c < m_mask_channel_; c++) {
        mask_map(row, c) = p_mask_float[c * num_map + pair.second];
      }
    }
    row++;
  }

  // obtain prototype branch data
  auto firstElement = proto_out_names.begin();
  int proto_stride = firstElement->first;
  std::string proto_output_name = firstElement->second;

  TensorInfo protoinfo = getOutputTensorInfo(proto_output_name);

  int proto_c = protoinfo.shape.dim[1];
  int proto_h = protoinfo.shape.dim[2];
  int proto_w = protoinfo.shape.dim[3];
  int proto_hw = proto_h * proto_w;

  Eigen::MatrixXf proto_output(proto_c, proto_hw);  // (32,96*160)

  int num_per_pixel = protoinfo.tensor_size / protoinfo.tensor_elem;
  int8_t *p_proto_int8 = static_cast<int8_t *>(protoinfo.raw_pointer);
  float *p_proto_float = static_cast<float *>(protoinfo.raw_pointer);
  if (num_per_pixel == 1) {
    for (int i = 0; i < proto_c; i++) {
      for (int j = 0; j < proto_hw; j++) {
        proto_output(i, j) = p_proto_int8[i * proto_hw + j] * protoinfo.qscale;
      }
    }
  } else {
    for (int i = 0; i < proto_c; i++) {
      for (int j = 0; j < proto_hw; j++) {
        proto_output(i, j) = p_proto_float[i * proto_hw + j];
      }
    }
  }

  Eigen::MatrixXf masks_output = mask_map * proto_output;

  obj_meta->mask_height = proto_h;
  obj_meta->mask_width = proto_w;
  // 96*160
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    int x1 = static_cast<int>(round(obj_meta->info[i].bbox.x1 / proto_stride));
    int x2 = static_cast<int>(round(obj_meta->info[i].bbox.x2 / proto_stride));
    int y1 = static_cast<int>(round(obj_meta->info[i].bbox.y1 / proto_stride));
    int y2 = static_cast<int>(round(obj_meta->info[i].bbox.y2 / proto_stride));
    obj_meta->info[i].mask = (uint8_t *)calloc(proto_hw, sizeof(uint8_t));
    for (int j = y1; j < y2; ++j) {
      for (int k = x1; k < x2; ++k) {
        if (1 / (1 + exp(-masks_output(i, j * proto_w + k))) >= 0.5) {
          obj_meta->info[i].mask[j * proto_w + k] = 255;
        }
      }
    }
    if (!hasSkippedVpssPreprocess()) {
      obj_meta->info[i].bbox =
          box_rescale(frame_width, frame_height, obj_meta->width, obj_meta->height,
                      obj_meta->info[i].bbox, meta_rescale_type_e::RESCALE_CENTER);
    }
  }
}
void YoloV8Seg::detPostProcess(Detections &dets, cvtdl_object_t *obj_meta,
                               std::vector<std::pair<int, int>> &final_dets_id) {
  CVI_SHAPE shape = getInputShape(0);

  // used to record the features and index of the detection box at which stride after NMS
  // post-processing
  std::vector<std::pair<int, int>> temp;

  int nn_width = shape.dim[3];
  int nn_height = shape.dim[2];
  float inverse_th = std::log(m_model_threshold / (1 - m_model_threshold));

  for (size_t i = 0; i < strides.size(); i++) {
    int stride = strides[i];
    std::string cls_name;
    int cls_offset = 0;
    if (class_out_names.count(stride)) {
      cls_name = class_out_names[stride];
    } else if (bbox_class_out_names.count(stride)) {
      cls_name = bbox_class_out_names[stride];
      cls_offset = m_box_channel_;
    }
    TensorInfo classinfo = getOutputTensorInfo(cls_name);

    int num_per_pixel = classinfo.tensor_size / classinfo.tensor_elem;
    int8_t *p_cls_int8 = static_cast<int8_t *>(classinfo.raw_pointer);
    float *p_cls_float = static_cast<float *>(classinfo.raw_pointer);

    int num_cls = alg_param_.cls;
    int num_anchor = classinfo.shape.dim[2] * classinfo.shape.dim[3];
    float cls_qscale = num_per_pixel == 1 ? classinfo.qscale : 1;

    for (int j = 0; j < num_anchor; j++) {
      int max_logit_c = -1;
      float max_logit = -1000;
      if (num_per_pixel == 1) {
        parse_cls_info<int8_t>(p_cls_int8, num_anchor, num_cls, j, cls_offset, cls_qscale,
                               &max_logit, &max_logit_c);
      } else {
        parse_cls_info<float>(p_cls_float, num_anchor, num_cls, j, cls_offset, cls_qscale,
                              &max_logit, &max_logit_c);
      }
      if (max_logit < inverse_th) {
        continue;
      }
      float score = 1 / (1 + exp(-max_logit));
      std::vector<float> box;
      decode_bbox_feature_map(stride, j, box);
      PtrDectRect det = std::make_shared<object_detect_rect_t>();
      det->score = score;
      det->x1 = box[0];
      det->y1 = box[1];
      det->x2 = box[2];
      det->y2 = box[3];
      det->label = max_logit_c;
      clip_bbox(nn_width, nn_height, det);
      float box_width = det->x2 - det->x1;
      float box_height = det->y2 - det->y1;
      if (box_width > 1 && box_height > 1) {
        dets.push_back(det);
        temp.push_back(std::make_pair(stride, j));
      }
    }
  }

  std::vector<int> keep(dets.size(), -1);
  Detections final_dets = nms_multi_class_with_ids(dets, m_model_nms_threshold, keep);
  convert_det_struct(final_dets, obj_meta, shape.dim[2], shape.dim[3]);

  for (auto &index : keep) {
    if (index >= 0) {
      final_dets_id.push_back(temp[index]);
    }
  }
}
}  // namespace cvitdl