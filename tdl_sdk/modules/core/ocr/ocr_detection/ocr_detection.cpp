#include "ocr_detection.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#include "core/utils/vpss_helper.h"

#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include "opencv2/imgproc.hpp"

#define R_SCALE (0.003922)
#define G_SCALE (0.003922)
#define B_SCALE (0.003922)
#define R_MEAN (0)
#define G_MEAN (0)
#define B_MEAN (0)

namespace cvitdl {

OCRDetection::OCRDetection() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
}
OCRDetection::~OCRDetection() {}

int OCRDetection::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
  printf("OCRDetection running!\n");
  int ret = run(frames);
  float thresh = 0.95;
  float boxThresh = 0.7;
  obj_meta->height = frame->stVFrame.u32Height;
  obj_meta->width = frame->stVFrame.u32Width;
  outputParser(thresh, boxThresh, obj_meta);
  return CVI_TDL_SUCCESS;
}

void NMS(std::vector<cvtdl_object_info_t> &bboxes, std::vector<cvtdl_object_info_t> &bboxes_nms,
         const float threshold, const char method) {
  int select_idx = 0;
  int num_bbox = bboxes.size();
  std::vector<int> mask_merged(num_bbox, 0);
  bool all_merged = false;

  while (!all_merged) {
    while (select_idx < num_bbox && mask_merged[select_idx] == 1) select_idx++;
    if (select_idx == num_bbox) {
      all_merged = true;
      continue;
    }

    bboxes_nms.emplace_back(bboxes[select_idx]);
    mask_merged[select_idx] = 1;

    cvtdl_bbox_t select_bbox = bboxes[select_idx].bbox;
    float area1 = static_cast<float>((select_bbox.x2 - select_bbox.x1 + 1) *
                                     (select_bbox.y2 - select_bbox.y1 + 1));
    float x1 = static_cast<float>(select_bbox.x1);
    float y1 = static_cast<float>(select_bbox.y1);
    float x2 = static_cast<float>(select_bbox.x2);
    float y2 = static_cast<float>(select_bbox.y2);

    select_idx++;
    for (int i = select_idx; i < num_bbox; i++) {
      if (mask_merged[i] == 1) continue;

      cvtdl_bbox_t &bbox_i(bboxes[i].bbox);
      float x = std::max<float>(x1, static_cast<float>(bbox_i.x1));
      float y = std::max<float>(y1, static_cast<float>(bbox_i.y1));
      float w = std::min<float>(x2, static_cast<float>(bbox_i.x2)) - x + 1;
      float h = std::min<float>(y2, static_cast<float>(bbox_i.y2)) - y + 1;
      if (w <= 0 || h <= 0) {
        continue;
      }

      float area2 = static_cast<float>((bbox_i.x2 - bbox_i.x1 + 1) * (bbox_i.y2 - bbox_i.y1 + 1));
      float area_intersect = w * h;
      if (method == 'u' &&
          static_cast<float>(area_intersect) / (area1 + area2 - area_intersect) > threshold) {
        mask_merged[i] = 1;
        continue;
      }
      if (method == 'm' &&
          static_cast<float>(area_intersect) / std::min(area1, area2) > threshold) {
        mask_merged[i] = 1;
      }
    }
  }
}

void OCRDetection::outputParser(float thresh, float boxThresh, cvtdl_object_t *obj_meta) {
  float *out = getOutputRawPtr<float>(0);
  CVI_SHAPE output_shape = getOutputShape(0);
  int outHeight = output_shape.dim[2];
  int outWidth = output_shape.dim[3];
  cv::Mat probMap(outHeight, outWidth, CV_32FC1, out);
  cv::Mat binaryMap;
  probMap.convertTo(binaryMap, CV_8UC1, 255.0);
  cv::threshold(binaryMap, binaryMap, thresh * 255, 255, cv::THRESH_BINARY);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(binaryMap, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

  std::vector<cvtdl_object_info_t> bboxes;
  CVI_SHAPE shape = getInputShape(0);
  for (auto &contour : contours) {
    double area = cv::contourArea(contour);
    if (area < 10) continue;

    cv::Rect boundingBox = cv::boundingRect(contour);

    cvtdl_bbox_t bbox = {boundingBox.x, boundingBox.y, boundingBox.x + boundingBox.width,
                         boundingBox.y + boundingBox.height};
    cvtdl_bbox_t rescaled_bbox =
        box_rescale(obj_meta->width, obj_meta->height, shape.dim[3], shape.dim[2], bbox,
                    meta_rescale_type_e::RESCALE_CENTER);

    cvtdl_object_info_t objInfo;
    objInfo.bbox = rescaled_bbox;
    bboxes.push_back(objInfo);
  }

  std::vector<cvtdl_object_info_t> bboxes_nms;
  NMS(bboxes, bboxes_nms, boxThresh, 'u');

  CVI_TDL_MemAllocInit(bboxes_nms.size(), obj_meta);

  obj_meta->rescale_type = RESCALE_RB;
  memset(obj_meta->info, 0, sizeof(cvtdl_object_info_t) * bboxes_nms.size());
  obj_meta->size = bboxes_nms.size();
  obj_meta->entry_num = 0;
  for (auto &bbox_nms : bboxes_nms) {
    obj_meta->info[obj_meta->entry_num] = bbox_nms;
    obj_meta->entry_num++;
  }
}

}  // namespace cvitdl