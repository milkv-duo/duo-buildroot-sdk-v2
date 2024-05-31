#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/yolo_detection.hpp>

namespace cvi {
namespace runtime {

#define MAX_DET 200
#define MAX_DET_RAW 500

typedef struct box_ {
  float x, y, w, h;
} box;

typedef struct detection_ {
  box bbox;
  int cls;
  float score;
} detection;

static inline float exp_fast(float x) {
  union {
    unsigned int i;
    float f;
  } v;
  v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);

  return v.f;
}

static inline float _sigmoid(float x, bool fast) {
  if (fast)
    return 1.0f / (1.0f + exp_fast(-x));
  else
    return 1.0f / (1.0f + std::exp(-x));
}

static inline float _softmax(float *probs, float *data, int input_stride,
                             int num_of_class, int *max_cls, bool fast) {
  //assert(num_of_class == 80);
  float x[num_of_class];
  float max_x = -INFINITY;
  float min_x = INFINITY;
  for (int i = 0; i < num_of_class; i++) {
    x[i] = data[i * input_stride];
    if (x[i] > max_x) {
      max_x = x[i];
    }
    if (x[i] < min_x) {
      min_x = x[i];
    }
  }
#define t (-100.0f)
  float exp_x[num_of_class];
  float sum = 0;
  for (int i = 0; i < num_of_class; i++) {
    x[i] = x[i] - max_x;
    if (min_x < t)
      x[i] = x[i] / min_x * t;
    if (fast)
      exp_x[i] = exp_fast(x[i]);
    else
      exp_x[i] = std::exp(x[i]);
    sum += exp_x[i];
  }
  float max_prob = 0;
  for (int i = 0; i < num_of_class; i++) {
    probs[i] = exp_x[i] / sum;
    if (probs[i] > max_prob) {
      max_prob = probs[i];
      *max_cls = i;
    }
  }
  return max_prob;
}

// feature in shape [3][5+80][grid_size][grid_size]
#define GET_INDEX(cell_idx, box_idx_in_cell, data_idx, num_cell, class_num)                         \
  (box_idx_in_cell * (class_num + 5) * num_cell + data_idx * num_cell + cell_idx)

static void process_feature(detection *det, int *det_idx, float *feature,
                            std::vector<int> grid_size, float *anchor,
                            std::vector<int> yolo_size, int num_of_class,
                            float obj_threshold) {
  int yolo_w = yolo_size[1];
  int yolo_h = yolo_size[0];
  //TPU_LOG_DEBUG("grid_size_h: %d\n", grid_size[0]);
  //TPU_LOG_DEBUG("grid_size_w: %d\n", grid_size[1]);
  //TPU_LOG_DEBUG("obj_threshold: %f\n", obj_threshold);
  int num_boxes_per_cell = 3;
  //assert(num_of_class == 80);

// 255 = 3 * (5 + 80)
// feature in shape [3][5+80][grid_size][grid_size]
#define COORD_X_INDEX (0)
#define COORD_Y_INDEX (1)
#define COORD_W_INDEX (2)
#define COORD_H_INDEX (3)
#define CONF_INDEX (4)
#define CLS_INDEX (5)
  int num_cell = grid_size[0] * grid_size[1];
  // int box_dim = 5 + num_of_class;

  int idx = *det_idx;
  int hit = 0, hit2 = 0;
  ;
  for (int i = 0; i < num_cell; i++) {
    for (int j = 0; j < num_boxes_per_cell; j++) {
      float box_confidence =
          _sigmoid(feature[GET_INDEX(i, j, CONF_INDEX, num_cell, num_of_class)], false);
      if (box_confidence < obj_threshold) {
        continue;
      }
      hit++;
      float box_class_probs[num_of_class];
      int box_max_cls = -1;
      float box_max_prob =
          _softmax(box_class_probs, &feature[GET_INDEX(i, j, CLS_INDEX, num_cell, num_of_class)],
                   num_cell, num_of_class, &box_max_cls, false);
      float box_max_score = box_confidence * box_max_prob;
      if (box_max_score < obj_threshold) {
        continue;
      }
      // get coord now
      int grid_x = i % grid_size[1];
      int grid_y = i / grid_size[1];
      float box_x = _sigmoid(feature[GET_INDEX(i, j, COORD_X_INDEX, num_cell, num_of_class)], false);
      box_x += grid_x;
      box_x /= grid_size[1];
      float box_y = _sigmoid(feature[GET_INDEX(i, j, COORD_Y_INDEX, num_cell, num_of_class)], false);
      box_y += grid_y;
      box_y /= grid_size[0];
      // anchor is in shape [3][2]
      float box_w = std::exp(feature[GET_INDEX(i, j, COORD_W_INDEX, num_cell, num_of_class)]);
      box_w *= anchor[j * 2];
      box_w /= yolo_w;
      float box_h = std::exp(feature[GET_INDEX(i, j, COORD_H_INDEX, num_cell, num_of_class)]);
      box_h *= anchor[j * 2 + 1];
      box_h /= yolo_h;
      hit2++;
      // DBG("  hit2 %d, conf = %f, cls = %d, coord = [%f, %f, %f, %f]\n",
      //    hit2, box_max_score, box_max_cls, box_x, box_y, box_w, box_h);
      det[idx].bbox = box{box_x, box_y, box_w, box_h};
      det[idx].score = box_max_score;
      det[idx].cls = box_max_cls;
      idx++;
      assert(idx <= MAX_DET);
    }
  }
  *det_idx = idx;
}

// https://github.com/ChenYingpeng/caffe-yolov3/blob/master/box.cpp
static float overlap(float x1, float w1, float x2, float w2) {
  float l1 = x1 - w1 / 2;
  float l2 = x2 - w2 / 2;
  float left = l1 > l2 ? l1 : l2;
  float r1 = x1 + w1 / 2;
  float r2 = x2 + w2 / 2;
  float right = r1 < r2 ? r1 : r2;
  return right - left;
}

static float box_intersection(box a, box b) {
  float w = overlap(a.x, a.w, b.x, b.w);
  float h = overlap(a.y, a.h, b.y, b.h);
  if (w < 0 || h < 0)
    return 0;
  float area = w * h;
  return area;
}

static float box_union(box a, box b) {
  float i = box_intersection(a, b);
  float u = a.w * a.h + b.w * b.h - i;
  return u;
}

//
// more aboud iou
//   https://github.com/ultralytics/yolov3/blob/master/utils/utils.py
// IoU = inter / (a + b - inter), can't handle enclosure issue
// GIoU, DIoU, CIoU?
//
static float box_iou(box a, box b) {
  return box_intersection(a, b) / box_union(a, b);
}

static void nms(detection *det, int num, float nms_threshold) {
  for (int i = 0; i < num; i++) {
    if (det[i].score == 0) {
      // erased already
      continue;
    }
    for (int j = i + 1; j < num; j++) {
      if (det[j].score == 0) {
        // erased already
        continue;
      }
      if (det[i].cls != det[j].cls) {
        // not the same class
        continue;
      }
      float iou = box_iou(det[i].bbox, det[j].bbox);
      assert(iou <= 1.0f);
      if (iou > nms_threshold) {
        // overlapped, select one to erase
        if (det[i].score < det[j].score) {
          det[i].score = 0;
        } else {
          det[j].score = 0;
        }
      }
    }
  }
}

YoloDetectionFunc::~YoloDetectionFunc() {}

void YoloDetectionFunc::setup(tensor_list_t &inputs,
                              tensor_list_t &outputs,
                              OpParam &param) {
  _net_input_h = param.get<int32_t>("net_input_h");
  _net_input_w = param.get<int32_t>("net_input_w");
  _nms_threshold = param.get<float>("nms_threshold");
  _obj_threshold = param.get<float>("obj_threshold");
  _keep_topk = param.get<int32_t>("keep_topk");

  if (param.has("tiny")) {
    _tiny = param.get<bool>("tiny");
  }

  if (param.has("yolo_v4")) {
    _yolo_v4 = param.get<bool>("yolo_v4");
  }

  if (param.has("spp_net")) {
    _spp_net = param.get<bool>("spp_net");
  }

  if (param.has("class_num")) {
    _class_num = param.get<int32_t>("class_num");
  }

  _anchors.clear();
  if (param.has("anchors")) {
    auto anchors = param.get<std::string>("anchors");

    std::istringstream iss(anchors);
    std::string s;
    while (std::getline(iss, s, ',')) {
      _anchors.push_back(atof(s.c_str()));
    }
  }

  std::sort(inputs.begin(), inputs.end(),
   [](const std::shared_ptr<Neuron> &a, const std::shared_ptr<Neuron> &b) {
     return a->shape[3] > b->shape[3];
   });

  _bottoms = inputs;
  _tops = outputs;

  if (_tiny) {
    assert(_bottoms.size() == 2);
    if (_anchors.size() == 0) {
      // Yolov3-tiny default anchors
      _anchors = {
        10,14, 23,27, 37,58,      // layer23-conv (26*26)
        81,82, 135,169, 344,319   // layer16-conv (13*13)
      };
    }
  } else {
    assert(_bottoms.size() == 3);
    if (_anchors.size() == 0) {
      if (_yolo_v4) {
        _anchors = {
          142, 110, 192, 243, 459, 401, // layer161-conv
          36, 75, 76, 55, 72, 146,// layer150-conv
          12, 16, 19, 36, 40, 28, // layer139-conv
        };
      }
      else {
        // Yolov3 default anchors
        _anchors = {
          10,13, 16,30, 33,23,      // layer106-conv (52*52)
          30,61, 62,45, 59,119,     // layer94-conv  (26*26)
          116,90, 156,198, 373,326  // layer82-conv  (13*13)
        };
      }
    }
  }
}

void YoloDetectionFunc::run() {
  auto top_data = _tops[0]->cpu_data<float>();
  memset(top_data, 0, _tops[0]->size());
  int batch = _tops[0]->shape[0];

  size_t bottom_count = _bottoms.size();
  assert(_anchors.size() == bottom_count * 6);
  float (*anchors)[6] = (float (*)[6])_anchors.data();

  for (int b = 0; b < batch; ++b) {
    std::vector<std::vector<int>> grid_size;
    std::vector<float*> features;

    for (size_t i = 0; i < bottom_count; ++i) {
      int offset = b * _bottoms[i]->shape[1] * _bottoms[i]->shape[2] * _bottoms[i]->shape[3];
      grid_size.push_back({_bottoms[i]->shape[2], _bottoms[i]->shape[3]});
      auto data = _bottoms[i]->cpu_data<float>() + offset;
      //auto size = _bottoms[i]->count() / batch;
      //std::vector<float> bottom_data(data, data + size);
      features.push_back(data);
    }

    detection det_raw[MAX_DET_RAW];
    detection dets[MAX_DET];
    int det_raw_idx = 0;
    for (size_t i = 0; i < features.size(); i++) {
      process_feature(det_raw, &det_raw_idx, features[i], grid_size[i],
                    &anchors[i][0], {_net_input_h, _net_input_w}, _class_num, _obj_threshold);
    }
    nms(det_raw, det_raw_idx, _nms_threshold);
    int det_idx = 0;
    for (int i = 0; i < det_raw_idx; i++) {
      if (det_raw[i].score > 0) {
        dets[det_idx] = det_raw[i];
        det_idx++;
      }
    }

    auto keep_topk = _keep_topk;
    if (keep_topk > det_idx)
      keep_topk = det_idx;

    long long count = 0;
    auto batch_output_data = top_data + b * _tops[0]->shape[1] * _tops[0]->shape[2] * _tops[0]->shape[3];
    for (int i = 0; i < keep_topk; ++i) {
      batch_output_data[count++] = dets[i].bbox.x;
      batch_output_data[count++] = dets[i].bbox.y;
      batch_output_data[count++] = dets[i].bbox.w;
      batch_output_data[count++] = dets[i].bbox.h;
      batch_output_data[count++] = dets[i].cls;
      batch_output_data[count++] = dets[i].score;

      //TPU_LOG_DEBUG("x = %f, y = %f, w = %f, h = %f, class = %d, score = %f\n",
      //              dets[i].bbox.x, dets[i].bbox.y, dets[i].bbox.w, dets[i].bbox.h, dets[i].cls, dets[i].score);
    }
  }
}

} // namespace runtime
} // namespace cvi
