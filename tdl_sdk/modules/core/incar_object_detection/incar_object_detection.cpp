#include "incar_object_detection.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"

namespace cvitdl {

IncarObjectDetection::IncarObjectDetection() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = 0.999;
  m_preprocess_param[0].factor[1] = 0.999;
  m_preprocess_param[0].factor[2] = 0.999;
  m_preprocess_param[0].mean[0] = 0.0;
  m_preprocess_param[0].mean[1] = 0.0;
  m_preprocess_param[0].mean[2] = 0.0;

  m_preprocess_param[0].use_crop = false;
  m_preprocess_param[0].rescale_type = RESCALE_CENTER;
}
int IncarObjectDetection::inference(VIDEO_FRAME_INFO_S* frame, cvtdl_face_t* meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
    LOGE("Error: pixel format not match PIXEL_FORMAT_RGB_888.\n");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  std::vector<VIDEO_FRAME_INFO_S*> frames = {frame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  outputParser(input_size, input_size, meta);
  return CVI_TDL_SUCCESS;
}

void IncarObjectDetection::outputParser(int image_width, int image_height, cvtdl_face_t* meta) {
  std::vector<std::vector<cvtdl_dms_od_info_t>> results;
  std::vector<cvtdl_dms_od_info_t> vec_bbox_nms;
  results.resize(num_class);

  for (const auto& head_info : heads_info) {
    float* cls_pred = getOutputRawPtr<float>(head_info.cls_layer);
    float* dis_pred = getOutputRawPtr<float>(head_info.dis_layer);
    decode_infer(cls_pred, dis_pred, head_info.stride, 0.6, results);
  }
  for (int i = 0; i < (int)results.size(); i++) {
    NonMaximumSuppression(results[i], vec_bbox_nms, 0.5, 'u');
  }
  meta->dms->dms_od.size = vec_bbox_nms.size();
  meta->dms->dms_od.width = image_width;
  meta->dms->dms_od.height = image_height;
  meta->dms->dms_od.rescale_type = m_vpss_config[0].rescale_type;
  meta->dms->dms_od.info =
      (cvtdl_dms_od_info_t*)malloc(sizeof(cvtdl_dms_od_info_t) * meta->dms->dms_od.size);

  for (uint32_t i = 0; i < meta->dms->dms_od.size; ++i) {
    clip_boxes(image_width, image_height, vec_bbox_nms[i].bbox);
    meta->dms->dms_od.info[i].bbox.x1 = vec_bbox_nms[i].bbox.x1;
    meta->dms->dms_od.info[i].bbox.x2 = vec_bbox_nms[i].bbox.x2;
    meta->dms->dms_od.info[i].bbox.y1 = vec_bbox_nms[i].bbox.y1;
    meta->dms->dms_od.info[i].bbox.y2 = vec_bbox_nms[i].bbox.y2;
    meta->dms->dms_od.info[i].bbox.score = vec_bbox_nms[i].bbox.score;
    meta->dms->dms_od.info[i].classes = vec_bbox_nms[i].classes;
    strcpy(meta->dms->dms_od.info[i].name, vec_bbox_nms[i].name);
  }
}

void IncarObjectDetection::decode_infer(float* cls_pred, float* dis_pred, int stride,
                                        float threshold,
                                        std::vector<std::vector<cvtdl_dms_od_info_t>>& results) {
  int feature_h = 320 / stride;
  int feature_w = 320 / stride;
  for (int idx = 0; idx < feature_h * feature_w; idx++) {
    const float* scores = (cls_pred + idx * num_class);
    int row = idx / feature_w;
    int col = idx % feature_w;
    float score = 0;
    int cur_label = 0;
    for (int label = 0; label < num_class; label++) {
      if (scores[label] > score) {
        score = scores[label];
        cur_label = label;
      }
    }
    if (score > threshold) {
      const float* bbox_pred = (dis_pred + idx * 4 * (reg_max + 1));
      disPred2Bbox(results, bbox_pred, cur_label, score, col, row, stride);
    }
  }
}

void IncarObjectDetection::disPred2Bbox(std::vector<std::vector<cvtdl_dms_od_info_t>>& results,
                                        const float*& dfl_det, int classes, float score, int x,
                                        int y, int stride) {
  cvtdl_dms_od_info_t temp;
  strcpy(temp.name, class_name[classes]);
  temp.bbox.score = score;
  temp.classes = classes;
  float ct_x = (x + 0.5) * stride;
  float ct_y = (y + 0.5) * stride;
  std::vector<float> dis_pred;
  dis_pred.resize(4);
  for (int i = 0; i < 4; i++) {
    float dis = 0;
    float* dis_after_sm = new float[reg_max + 1];
    activation_function_softmax(dfl_det + i * (reg_max + 1), dis_after_sm, reg_max + 1);
    for (int j = 0; j < reg_max + 1; j++) {
      dis += j * dis_after_sm[j];
    }
    dis *= stride;
    dis_pred[i] = dis;
    delete[] dis_after_sm;
  }
  temp.bbox.x1 = (std::max)(ct_x - dis_pred[0], .0f);
  temp.bbox.y1 = (std::max)(ct_y - dis_pred[1], .0f);
  temp.bbox.x2 = (std::min)(ct_x + dis_pred[2], (float)input_size);
  temp.bbox.y2 = (std::min)(ct_y + dis_pred[3], (float)input_size);
  results[classes].push_back(temp);
}

inline float IncarObjectDetection::fast_exp(float x) {
  union {
    uint32_t i;
    float f;
  } v{};
  v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);
  return v.f;
}

template <typename _Tp>
int IncarObjectDetection::activation_function_softmax(const _Tp* src, _Tp* dst, int length) {
  const _Tp alpha = *std::max_element(src, src + length);
  _Tp denominator{0};

  for (int i = 0; i < length; ++i) {
    dst[i] = fast_exp(src[i] - alpha);
    denominator += dst[i];
  }

  for (int i = 0; i < length; ++i) {
    dst[i] /= denominator;
  }

  return 0;
}

}  // namespace cvitdl
