#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/frcn_detection.hpp>

namespace cvi {
namespace runtime {

typedef struct {
    float x1, y1, x2, y2;
} coord;

typedef struct  {
    coord bbox;
    int cls;
    float score;
} detections;

static void bbox_transform_inv(const float* boxes, const float* deltas, float* pred, int num, int class_num)
{
  for (int i = 0; i < num; ++i) {
    float height = boxes[i*4+3] - boxes[i*4+1] + 1;
    float width = boxes[i*4+2] - boxes[i*4+0] + 1;
    float ctr_x = boxes[i*4+0] + width * 0.5;
    float ctr_y = boxes[i*4+1] + height * 0.5;

    for (int j = 0; j < class_num; ++j) {
      float dx = deltas[i*class_num*4 + j*4 + 0];
      float dy = deltas[i*class_num*4 + j*4 + 1];
      float dw = deltas[i*class_num*4 + j*4 + 2];
      float dh = deltas[i*class_num*4 + j*4 + 3];

      float pred_ctr_x = dx * width + ctr_x;
      float pred_ctr_y = dy * height + ctr_y;
      float pred_w = std::exp(dw) * width;
      float pred_h = std::exp(dh) * height;

      pred[i*class_num*4 + j*4 + 0] = pred_ctr_x - pred_w / 2;
      pred[i*class_num*4 + j*4 + 1] = pred_ctr_y - pred_h / 2;
      pred[i*class_num*4 + j*4 + 2] = pred_ctr_x + pred_w / 2;
      pred[i*class_num*4 + j*4 + 3] = pred_ctr_y + pred_h / 2;
    }
  }
}

static void nms(detections *dets, int num, float nms_threshold)
{
  for (int i = 0; i < num; i++) {
    if (dets[i].score == 0) {
      // erased already
      continue;
    }

    float s1 = (dets[i].bbox.x2 - dets[i].bbox.x1 + 1) * (dets[i].bbox.y2 - dets[i].bbox.y1 + 1);
    for (int j = i + 1; j < num; j++) {
      if (dets[j].score == 0) {
        // erased already
        continue;
      }
      if (dets[i].cls != dets[j].cls) {
        // not the same class
        continue;
      }

      float s2 = (dets[j].bbox.x2 - dets[j].bbox.x1 + 1) * (dets[j].bbox.y2 - dets[j].bbox.y1 + 1);

      float x1 = std::max(dets[i].bbox.x1, dets[j].bbox.x1);
      float y1 = std::max(dets[i].bbox.y1, dets[j].bbox.y1);
      float x2 = std::min(dets[i].bbox.x2, dets[j].bbox.x2);
      float y2 = std::min(dets[i].bbox.y2, dets[j].bbox.y2);

      float width = x2 - x1;
      float height = y2 - y1;
      if (width > 0 && height > 0) {
        float iou = width * height / (s1 + s2 - width * height);
        assert(iou <= 1.0f);
        if (iou > nms_threshold) {
          // overlapped, select one to erase
          if (dets[i].score < dets[j].score) {
            dets[i].score = 0;
          } else {
            dets[j].score = 0;
          }
        }
      }
    }
  }
}

FrcnDetectionFunc::~FrcnDetectionFunc() {}

void FrcnDetectionFunc::setup(tensor_list_t &inputs,
            tensor_list_t &outputs,
            OpParam &param) {
  nms_threshold = param.get<float>("nms_threshold");
  obj_threshold = param.get<float>("obj_threshold");
  keep_topk = param.get<int32_t>("keep_topk");
  class_num = param.get<int32_t>("class_num");

  std::sort(inputs.begin(), inputs.end(),
    [](const std::shared_ptr<Neuron> &a, const std::shared_ptr<Neuron> &b) {
      return a->shape[1] > b->shape[1];
    });

  _bottoms = inputs;
  _tops = outputs;
}

void FrcnDetectionFunc::run() {
  auto top_data = _tops[0]->cpu_data<float>();
  memset(top_data, 0, _tops[0]->size());

  size_t bottom_count = _bottoms.size();
  assert(bottom_count == 3);

  float *bbox_deltas = (float *)_bottoms[0]->cpu_data<float>();
  float *scores = (float *)_bottoms[1]->cpu_data<float>();
  float *rois = (float *)_bottoms[2]->cpu_data<float>();

  int batch = _bottoms[2]->shape[0];
  int num = _bottoms[2]->shape[2];
  auto deltas_size = _bottoms[0]->size() / batch;
  auto scores_size = _bottoms[1]->size() / batch;

  for (int b = 0; b < batch; ++b) {
    auto batch_bbox_deltas = bbox_deltas + b * deltas_size;
    auto batch_scores = scores + b * scores_size;
    auto batch_rois = rois + _bottoms[2]->offset(b);
    std::vector<float> boxes(num * 4, 0);
    for (int i = 0; i < num; ++i) {
      for (int j = 0; j < 4; ++j) {
        boxes[i*4 + j] = batch_rois[i*5 + j + 1];
      }
    }

    std::vector<float> pred(num * class_num * 4, 0);
    float *pred_data = pred.data();
    std::vector<float> deltas(batch_bbox_deltas, batch_bbox_deltas + deltas_size);
    bbox_transform_inv(boxes.data(), deltas.data(), pred_data, num, class_num);

    int det_num = 0;
    detections dets[num];

    for (int i = 0; i < num; ++i) {
      for (int j = 1; j < class_num; ++j) {
        if (batch_scores[i*class_num + j] > obj_threshold) {
          dets[det_num].bbox.x1 = pred[i*class_num*4 + j*4 + 0];
          dets[det_num].bbox.y1 = pred[i*class_num*4 + j*4 + 1];
          dets[det_num].bbox.x2 = pred[i*class_num*4 + j*4 + 2];
          dets[det_num].bbox.y2 = pred[i*class_num*4 + j*4 + 3];
          dets[det_num].cls = j;
          dets[det_num].score = batch_scores[i*class_num + j];
          det_num++;
        }
      }
    }

    nms(dets, det_num, nms_threshold);
    detections dets_nms[det_num];
    int det_idx = 0;
    for (int i = 0; i < det_num; i++) {
      if (dets[i].score > 0) {
        dets_nms[det_idx] = dets[i];
        det_idx ++;
      }
    }

    auto tmp_topk = keep_topk;
    if (tmp_topk > det_idx)
        tmp_topk = det_idx;

    long long count = 0;
    auto batch_top_data = top_data + _tops[0]->offset(b);
    for(int i = 0; i < tmp_topk; ++i) {
      batch_top_data[count++] = dets_nms[i].bbox.x1;
      batch_top_data[count++] = dets_nms[i].bbox.y1;
      batch_top_data[count++] = dets_nms[i].bbox.x2;
      batch_top_data[count++] = dets_nms[i].bbox.y2;
      batch_top_data[count++] = dets_nms[i].cls;
      batch_top_data[count++] = dets_nms[i].score;
    }
  }
}

}
}