#include "yolov3.hpp"
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core_utils.hpp"

#define YOLOV3_CLASSES 80
#define YOLOV3_NMS_THRESHOLD 0.45
#define YOLOV3_ANCHOR_NUM 3
#define YOLOV3_COORDS 4
#define YOLOV3_DEFAULT_DET_BUFFER 100
#define YOLOV3_SCALE (float)(1 / 255.0)
#define YOLOV3_OUTPUT1 "layer82-conv_dequant"
#define YOLOV3_OUTPUT2 "layer94-conv_dequant"
#define YOLOV3_OUTPUT3 "layer106-conv_dequant"

using namespace std;

namespace cvitdl {

Yolov3::Yolov3() {
  m_det_buf_size = YOLOV3_DEFAULT_DET_BUFFER;

  m_yolov3_param = {
      YOLOV3_CLASSES,                                                                  // m_classes
      {10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326},  // m_biases
      YOLOV3_NMS_THRESHOLD,              // m_nms_threshold
      YOLOV3_ANCHOR_NUM,                 // m_anchor_nums
      YOLOV3_COORDS,                     // m_coords
      1,                                 // m_batch
      v3,                                // type
      {{6, 7, 8}, {3, 4, 5}, {0, 1, 2}}  // m_mask
  };
}

Yolov3::~Yolov3() { free(mp_total_dets); }

int Yolov3::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (run(frames) == CVI_TDL_SUCCESS) {
    outputParser(srcFrame, obj);
  }

  return ret;
}

void Yolov3::outputParser(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj) {
  vector<float *> features;
  vector<string> output_name = {YOLOV3_OUTPUT1, YOLOV3_OUTPUT2, YOLOV3_OUTPUT3};
  vector<CVI_SHAPE> output_shape;

  for (auto name : output_name) {
    output_shape.push_back(getOutputShape(name));
    features.push_back(getOutputRawPtr<float>(name));
  }

  CVI_SHAPE input_shape = getInputShape(0);

  int yolov3_h = input_shape.dim[2];
  int yolov3_w = input_shape.dim[3];

  // Yolov3 has 3 different size outputs
  vector<YOLOLayer> net_outputs;
  for (unsigned int i = 0; i < features.size(); i++) {
    YOLOLayer l = {features[i], int(output_shape[i].dim[0]), int(output_shape[i].dim[1]),
                   int(output_shape[i].dim[3]), int(output_shape[i].dim[2])};
    net_outputs.push_back(l);
  }

  vector<object_detect_rect_t> results;
  if (mp_total_dets == nullptr) {
    mp_total_dets = (detection *)malloc(sizeof(detection) * m_det_buf_size);
  }
  int total_boxes = 0;

  for (size_t i = 0; i < net_outputs.size(); i++) {
    int nboxes = 0;

    doYolo(net_outputs.at(i));
    detection *dets = GetNetworkBoxes(net_outputs.at(i), m_yolov3_param.m_classes, yolov3_w,
                                      yolov3_h, m_model_threshold, 1, &nboxes, m_yolov3_param, i);

    uint32_t next_size = total_boxes + nboxes;
    // Not a good design?
    if (next_size > m_det_buf_size) {
      mp_total_dets = (detection *)realloc(mp_total_dets, next_size * sizeof(detection));
      m_det_buf_size = next_size;
    }

    memcpy(mp_total_dets + total_boxes, dets, sizeof(detection) * nboxes);
    total_boxes += nboxes;

    // we do not use FreeDetections because we use just use memcpy,
    // FreeDetections will free det.prob
    delete[] dets;
  }

  DoNmsSort(mp_total_dets, total_boxes, m_yolov3_param.m_classes, m_yolov3_param.m_nms_threshold);
  getYOLOResults(mp_total_dets, total_boxes, m_model_threshold, yolov3_w, yolov3_h, results);
  for (int i = 0; i < total_boxes; ++i) {
    delete[] mp_total_dets[i].prob;
  }

  // fill obj
  obj->size = results.size();
  obj->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * obj->size);
  obj->width = yolov3_w;
  obj->height = yolov3_h;
  obj->rescale_type = m_vpss_config[0].rescale_type;

  memset(obj->info, 0, sizeof(cvtdl_object_info_t) * obj->size);
  for (uint32_t i = 0; i < obj->size; ++i) {
    obj->info[i].bbox.x1 = results[i].x1;
    obj->info[i].bbox.y1 = results[i].y1;
    obj->info[i].bbox.x2 = results[i].x2;
    obj->info[i].bbox.y2 = results[i].y2;
    obj->info[i].bbox.score = results[i].score;
    obj->info[i].classes = results[i].label;
    strncpy(obj->info[i].name, coco_utils::class_names_80[results[i].label].c_str(),
            sizeof(obj->info[i].name));
    // printf("YOLO3: %s (%d): %lf %lf %lf %lf, score=%.2f\n", obj->info[i].name,
    // obj->info[i].classes,
    //        obj->info[i].bbox.x1, obj->info[i].bbox.x2, obj->info[i].bbox.y1,
    //        obj->info[i].bbox.y2, results[i].score);
  }
  if (!hasSkippedVpssPreprocess()) {
    for (uint32_t i = 0; i < obj->size; ++i) {
      obj->info[i].bbox =
          box_rescale(srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height, obj->width,
                      obj->height, obj->info[i].bbox, meta_rescale_type_e::RESCALE_CENTER);
    }
    obj->width = srcFrame->stVFrame.u32Width;
    obj->height = srcFrame->stVFrame.u32Height;
  }
}

void Yolov3::doYolo(YOLOLayer &l) {
  float *data = l.data;
  int w = l.width;
  int h = l.height;
  int output_size = l.norm * l.channels * w * h;

  for (int b = 0; b < m_yolov3_param.m_batch; ++b) {
    for (int p = 0; p < w * h; ++p) {
      for (int n = 0; n < m_yolov3_param.m_anchor_nums; ++n) {
        int obj_index = EntryIndex(w, h, m_yolov3_param.m_classes, b, n * w * h + p,
                                   m_yolov3_param.m_coords, output_size);
        ActivateArray(data + obj_index, 1, true);
        float objectness = data[obj_index];

        if (objectness >= m_model_threshold) {
          int box_index =
              EntryIndex(w, h, m_yolov3_param.m_classes, b, n * w * h + p, 0, output_size);
          ActivateArray(data + box_index, 1, true);
          ActivateArray(data + box_index + (w * h), 1, true);

          for (int j = 0; j < m_yolov3_param.m_classes; ++j) {
            int class_index = EntryIndex(w, h, m_yolov3_param.m_classes, b, n * w * h + p,
                                         4 + 1 + j, output_size);
            ActivateArray(data + class_index, 1, true);
          }
        }
      }
    }
  }
}

void Yolov3::getYOLOResults(detection *dets, int num, float threshold, int ori_w, int ori_h,
                            vector<object_detect_rect_t> &results) {
  for (int i = 0; i < num; ++i) {
    std::string labelstr;
    int obj_class = -1;
    object_detect_rect_t obj_result;
    obj_result.score = 0;
    obj_result.label = obj_class;
    for (int j = 0; j < m_yolov3_param.m_classes; ++j) {
      if (dets[i].prob[j] > threshold) {
        if (obj_class < 0) {
          labelstr = coco_utils::class_names_80[j];
          obj_class = j;
          obj_result.label = obj_class;
          obj_result.score = dets[i].prob[j];
        } else {
          labelstr += ", " + coco_utils::class_names_80[j];
          if (dets[i].prob[j] > obj_result.score) {
            obj_result.score = dets[i].prob[j];
            obj_result.label = obj_class;
          }
        }
      }
    }

    if (obj_class < 0) {
      continue;
    }

    box b = dets[i].bbox;
    int left = (b.x - b.w / 2.) * ori_w;
    int right = (b.x + b.w / 2.) * ori_w;
    int top = (b.y - b.h / 2.) * ori_h;
    int bot = (b.y + b.h / 2.) * ori_h;
    if (left < 0) left = 0;
    if (right > ori_w - 1) right = ori_w - 1;
    if (top < 0) top = 0;
    if (bot > ori_h - 1) bot = ori_h - 1;

    object_detect_rect_t rect;

    rect.x1 = left;
    rect.y1 = top;
    rect.x2 = right;
    rect.y2 = bot;
    rect.label = obj_result.label;
    rect.score = obj_result.score;

    results.emplace_back(move(rect));
  }
}

}  // namespace cvitdl
