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
#include "yolo.hpp"

template <typename T>
int yolo_argmax(T *ptr, int start_idx, int arr_len) {
  int max_idx = start_idx;
  for (int i = start_idx + 1; i < start_idx + arr_len; i++) {
    if (ptr[i] > ptr[max_idx]) {
      max_idx = i;
    }
  }
  return max_idx - start_idx;
}

int yolo_max_val(int x, int y) {
  if (x > y) return x;
  return y;
}

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

Yolo::Yolo() {
  // default param
  float mean[3] = {123.675, 116.28, 103.52};
  float std[3] = {58.395, 57.12, 57.375};

  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].mean[i] = mean[i] / std[i];
    m_preprocess_param[0].factor[i] = 1.0 / std[i];
  }

  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  alg_param_.cls = 80;
}

int Yolo::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
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

int Yolo::onModelOpened() {
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;
    printf("output shape: %d %d %d %d\n", output_shape.dim[0], output_shape.dim[1],
           output_shape.dim[2], output_shape.dim[3]);
  }

  return CVI_TDL_SUCCESS;
}

int Yolo::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("Yolo run inference failed!\n");
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);

  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, obj_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void yolo_xywh2xxyy(float x, float y, float w, float h, PtrDectRect &det) {
  det->x1 = x - w / 2;
  det->y1 = y - h / 2;
  det->x2 = x + w / 2;
  det->y2 = y + h / 2;
}

template <typename T>
void parseDet(T *ptr, int start_idx, float qscale, int cls, PtrDectRect &det) {
  float x = ptr[start_idx] * qscale;
  float y = ptr[start_idx + 1] * qscale;
  float w = ptr[start_idx + 2] * qscale;
  float h = ptr[start_idx + 3] * qscale;

  yolo_xywh2xxyy(x, y, w, h, det);
}

void Yolo::getYoloDetections(int8_t *ptr_int8, float *ptr_float, int num_per_pixel, float qscale,
                             int det_num, int channel, Detections &vec_obj) {
  int start_idx = 0;

  for (int i = 0; i < det_num; i++) {
    float obj_conf, score;
    int cls = -1;
    if (num_per_pixel == 1) {
      cls = yolo_argmax<int8_t>(ptr_int8, start_idx + channel - 80, start_idx + channel);
      if (channel == 84) {
        obj_conf = ptr_int8[start_idx + cls + 4] * qscale;
        score = obj_conf;
      } else {
        obj_conf = ptr_int8[start_idx + 4] * qscale;
        score = obj_conf * ptr_int8[start_idx + 5 + cls] * qscale;
      }
    } else {
      cls = yolo_argmax<float>(ptr_float, start_idx + channel - 80, start_idx + channel);
      if (channel == 84) {
        obj_conf = ptr_float[start_idx + cls + 4] * qscale;
        score = obj_conf;
      } else {
        obj_conf = ptr_float[start_idx + 4] * qscale;
        score = obj_conf * ptr_float[start_idx + 5 + cls] * qscale;
      }
    }

    if (obj_conf < m_model_threshold) {
      start_idx += channel;
      continue;
    }

    PtrDectRect det = std::make_shared<object_detect_rect_t>();
    det->score = score;
    det->label = cls;
    if (num_per_pixel == 1) {
      parseDet<int8_t>(ptr_int8, start_idx, qscale, cls, det);
    } else {
      parseDet<float>(ptr_float, start_idx, qscale, cls, det);
    }
    vec_obj.push_back(det);
    start_idx += channel;
  }
}

void Yolo::clip_bbox(int frame_width, int frame_height, cvtdl_bbox_t *bbox) {
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

cvtdl_bbox_t Yolo::Yolo_box_rescale(int frame_width, int frame_height, int width, int height,
                                    cvtdl_bbox_t bbox) {
  cvtdl_bbox_t rescale_bbox;
  int max_board = yolo_max_val(frame_width, frame_height);
  float ratio = float(max_board) / float(width);
  rescale_bbox.x1 = int(bbox.x1 * ratio);
  rescale_bbox.x2 = int(bbox.x2 * ratio);
  rescale_bbox.y1 = int(bbox.y1 * ratio);
  rescale_bbox.y2 = int(bbox.y2 * ratio);
  rescale_bbox.score = bbox.score;
  clip_bbox(frame_width, frame_height, &rescale_bbox);
  return rescale_bbox;
}

void Yolo::YoloPostProcess(Detections &dets, int frame_width, int frame_height,
                           cvtdl_object_t *obj_meta) {
  Detections final_dets = nms_multi_class(dets, m_model_nms_threshold);
  CVI_SHAPE shape = getInputShape(0);
  convert_det_struct(final_dets, obj_meta, shape.dim[2], shape.dim[3]);
  // rescale bounding box to original image
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj_meta->size; ++i) {
      obj_meta->info[i].bbox = Yolo_box_rescale(frame_width, frame_height, obj_meta->width,
                                                obj_meta->height, obj_meta->info[i].bbox);
    }
    obj_meta->width = frame_width;
    obj_meta->height = frame_height;
  }
}

void Yolo::outputParser(const int image_width, const int image_height, const int frame_width,
                        const int frame_height, cvtdl_object_t *obj_meta) {
  return;

  Detections vec_obj;

  for (uint32_t branch = 0; branch < getNumOutputTensor(); branch++) {
    TensorInfo oinfo = getOutputTensorInfo(branch);
    CVI_SHAPE output_shape = oinfo.shape;

    int num_per_pixel = oinfo.tensor_size / oinfo.tensor_elem;
    float qscale = num_per_pixel == 1 ? oinfo.qscale : 1;

    int8_t *ptr_int8 = static_cast<int8_t *>(oinfo.raw_pointer);
    float *ptr_float = static_cast<float *>(oinfo.raw_pointer);
    printf("entering process: %d %d\n", output_shape.dim[1], output_shape.dim[2]);
    getYoloDetections(ptr_int8, ptr_float, num_per_pixel, qscale, output_shape.dim[1],
                      output_shape.dim[2], vec_obj);
  }

  YoloPostProcess(vec_obj, frame_width, frame_height, obj_meta);
}
// namespace cvitdl
}  // namespace cvitdl
