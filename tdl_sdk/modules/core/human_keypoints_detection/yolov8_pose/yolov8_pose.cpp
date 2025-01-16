#include <algorithm>
#include <cmath>
#include <iterator>

#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "yolov8_pose.hpp"

namespace cvitdl {

template <typename T>
inline void parse_cls_info(T *p_cls_ptr, int num_anchor, int num_cls, int anchor_idx, float qscale,
                           float *p_max_logit, int *p_max_cls) {
  int max_logit_c = -1;
  float max_logit = -1000;
  for (int c = 0; c < num_cls; c++) {
    float logit = p_cls_ptr[c * num_anchor + anchor_idx];
    if (logit > max_logit) {
      max_logit = logit;
      max_logit_c = c;
    }
  }
  *p_max_logit = max_logit * qscale;
  *p_max_cls = max_logit_c;
}

YoloV8Pose::YoloV8Pose() : YoloV8Pose(std::make_tuple(64, 17, 1)) {}

YoloV8Pose::YoloV8Pose(TUPLE_INT pose_pair) {
  for (int i = 0; i < 3; i++) {
    // default param
    m_preprocess_param[0].factor[i] = 1 / 255.f;
    m_preprocess_param[0].mean[i] = 0.0;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_model_nms_threshold = 0.7;
  m_box_channel_ = std::get<0>(pose_pair);
  m_kpts_channel_ = std::get<1>(pose_pair) * 3;
  m_cls_channel_ = std::get<2>(pose_pair);
}

int YoloV8Pose::onModelOpened() {
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

    if (stride_h != stride_w) {
      LOGE("stride not equal,stridew:%d,strideh:%d,featw:%d,feath:%d\n", stride_w, stride_h, feat_w,
           feat_h);
      return CVI_FAILURE;
    }
    if (channel == m_box_channel_) {
      bbox_out_names[stride_h] = oinfo.tensor_name;
      strides.push_back(stride_h);
      LOGI("parse box branch,name:%s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else if (channel == m_kpts_channel_) {
      keypoints_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse keypoints branch,name:%s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else if (channel == m_cls_channel_) {
      class_out_names[stride_h] = oinfo.tensor_name;
      LOGI("parse class branch,name: %s,stride:%d\n", oinfo.tensor_name.c_str(), stride_h);
    } else {
      LOGE("unexpected branch:%s,channel:%d\n", oinfo.tensor_name.c_str(), channel);
      return CVI_FAILURE;
    }
  }

  return CVI_TDL_SUCCESS;
}

int YoloV8Pose::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGW("YoloV8Pose run inference failed\n");
    return ret;
  }
  CVI_SHAPE shape = getInputShape(0);

  // LOGI("start to outputParser\n");
  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj_meta);

  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

// the bbox featuremap shape is b x 4*regmax x h x w
void YoloV8Pose::decode_bbox_feature_map(int stride, int anchor_idx,
                                         std::vector<float> &decode_box) {
  std::string box_name;

  box_name = bbox_out_names[stride];
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

void YoloV8Pose::decode_keypoints_feature_map(int stride, int anchor_idx,
                                              std::vector<float> &decode_kpts) {
  decode_kpts.clear();
  std::string kpts_name = keypoints_out_names[stride];
  TensorInfo kpts_info = getOutputTensorInfo(kpts_name);

  int num_per_pixel = kpts_info.tensor_size / kpts_info.tensor_elem;
  int8_t *p_kpts_int8 = static_cast<int8_t *>(kpts_info.raw_pointer);
  float *p_kpts_float = static_cast<float *>(kpts_info.raw_pointer);
  int num_anchor = kpts_info.shape.dim[2] * kpts_info.shape.dim[3];

  int32_t feat_w = kpts_info.shape.dim[3];

  int anchor_y = anchor_idx / feat_w;
  int anchor_x = anchor_idx % feat_w;

  float grid_x = anchor_x + 0.5;
  float grid_y = anchor_y + 0.5;

  float val;
  if (num_per_pixel == 1) {
    for (int c = 0; c < m_kpts_channel_; c++) {
      if (c % 3 == 0) {
        val = (p_kpts_int8[c * num_anchor + anchor_idx] * kpts_info.qscale * 2.0 + grid_x - 0.5) *
              (float)stride;
      } else if (c % 3 == 1) {
        val = (p_kpts_int8[c * num_anchor + anchor_idx] * kpts_info.qscale * 2.0 + grid_y - 0.5) *
              (float)stride;
      } else {
        val = 1.0 / (1.0 + std::exp(-p_kpts_int8[c * num_anchor + anchor_idx] * kpts_info.qscale));
      }
      decode_kpts.push_back(val);
    }
  } else {
    for (int c = 0; c < m_kpts_channel_; c++) {
      if (c % 3 == 0) {
        val = (p_kpts_float[c * num_anchor + anchor_idx] * 2.0 + grid_x - 0.5) * (float)stride;
      } else if (c % 3 == 1) {
        val = (p_kpts_float[c * num_anchor + anchor_idx] * 2.0 + grid_y - 0.5) * (float)stride;
      } else {
        val = 1.0 / (1.0 + std::exp(-p_kpts_float[c * num_anchor + anchor_idx]));
      }
      decode_kpts.push_back(val);
    }
  }
}

void YoloV8Pose::outputParser(const int image_width, const int image_height, const int frame_width,
                              const int frame_height, cvtdl_object_t *obj_meta) {
  Detections vec_obj;
  CVI_SHAPE shape = getInputShape(0);
  int nn_width = shape.dim[3];
  int nn_height = shape.dim[2];
  float inverse_th = std::log(m_model_threshold / (1 - m_model_threshold));

  std::vector<std::pair<int, int>> valild_pairs;
  for (size_t i = 0; i < strides.size(); i++) {
    int stride = strides[i];
    std::string cls_name = class_out_names[stride];

    TensorInfo classinfo = getOutputTensorInfo(cls_name);

    int num_per_pixel = classinfo.tensor_size / classinfo.tensor_elem;
    int8_t *p_cls_int8 = static_cast<int8_t *>(classinfo.raw_pointer);
    float *p_cls_float = static_cast<float *>(classinfo.raw_pointer);

    int num_cls = m_cls_channel_;
    int num_anchor = classinfo.shape.dim[2] * classinfo.shape.dim[3];
    // LOGI("stride:%d,featw:%d,feath:%d,numperpixel:%d,numcls:%d\n", stride,
    // classinfo.shape.dim[3],
    //     classinfo.shape.dim[2], num_per_pixel, num_cls);
    float cls_qscale = num_per_pixel == 1 ? classinfo.qscale : 1;
    for (int j = 0; j < num_anchor; j++) {
      int max_logit_c = -1;
      float max_logit = -1000;
      if (num_per_pixel == 1) {
        parse_cls_info<int8_t>(p_cls_int8, num_anchor, num_cls, j, cls_qscale, &max_logit,
                               &max_logit_c);
      } else {
        parse_cls_info<float>(p_cls_float, num_anchor, num_cls, j, cls_qscale, &max_logit,
                              &max_logit_c);
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
        vec_obj.push_back(det);
        valild_pairs.push_back(std::make_pair(stride, j));
      }
    }
  }
  postProcess(vec_obj, frame_width, frame_height, obj_meta, valild_pairs);
}

void YoloV8Pose::postProcess(Detections &dets, int frame_width, int frame_height,
                             cvtdl_object_t *obj, std::vector<std::pair<int, int>> &valild_pairs) {
  CVI_SHAPE shape = getInputShape(0);

  std::vector<int> keep(dets.size(), 0);

  Detections final_dets = nms_multi_class_with_ids(dets, m_model_nms_threshold, keep);

  CVI_TDL_MemAllocInit(final_dets.size(), obj);
  obj->height = shape.dim[2];
  obj->width = shape.dim[3];
  memset(obj->info, 0, sizeof(cvtdl_object_info_t) * obj->size);

  for (uint32_t i = 0; i < final_dets.size(); i++) {
    obj->info[i].bbox.x1 = final_dets[i]->x1;
    obj->info[i].bbox.y1 = final_dets[i]->y1;
    obj->info[i].bbox.x2 = final_dets[i]->x2;
    obj->info[i].bbox.y2 = final_dets[i]->y2;
    obj->info[i].bbox.score = final_dets[i]->score;
    obj->info[i].classes = final_dets[i]->label;

    obj->info[i].pedestrian_properity =
        (cvtdl_pedestrian_meta *)malloc(sizeof(cvtdl_pedestrian_meta));

    std::pair<int, int> final_ids = valild_pairs[keep[i]];

    std::vector<float> decode_kpts;
    decode_keypoints_feature_map(final_ids.first, final_ids.second, decode_kpts);

    int num_keypoints = m_kpts_channel_ / 3;
    for (int j = 0; j < num_keypoints; j++) {
      obj->info[i].pedestrian_properity->pose_17.x[j] = decode_kpts[j * 3];
      obj->info[i].pedestrian_properity->pose_17.y[j] = decode_kpts[j * 3 + 1];
      obj->info[i].pedestrian_properity->pose_17.score[j] = decode_kpts[j * 3 + 2];
    }
  }
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj->size; ++i) {
      cvtdl_object_info_t rescale_info =
          info_rescale_c(frame_width, frame_height, obj->width, obj->height, obj->info[i]);

      obj->info[i].bbox.x1 = rescale_info.bbox.x1;
      obj->info[i].bbox.y1 = rescale_info.bbox.y1;
      obj->info[i].bbox.x2 = rescale_info.bbox.x2;
      obj->info[i].bbox.y2 = rescale_info.bbox.y2;

      int num_keypoints = m_kpts_channel_ / 3;
      for (int j = 0; j < num_keypoints; j++) {
        obj->info[i].pedestrian_properity->pose_17.x[j] =
            rescale_info.pedestrian_properity->pose_17.x[j];
        obj->info[i].pedestrian_properity->pose_17.y[j] =
            rescale_info.pedestrian_properity->pose_17.y[j];
      }
      CVI_TDL_FreeCpp(&rescale_info);
      
    }
    obj->width = frame_width;
    obj->height = frame_height;
  }
}

}  // namespace cvitdl