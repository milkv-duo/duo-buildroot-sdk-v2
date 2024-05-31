#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/retinaface_detection.hpp>

namespace cvi {
namespace runtime {

void RetinaFaceDetectionFunc::setup(tensor_list_t &inputs,
                                    tensor_list_t &outputs,
                                    OpParam &param) {
  // sort inputs by neuron shape size
  std::sort(inputs.begin(), inputs.end(),
   [](const std::shared_ptr<Neuron> &a, const std::shared_ptr<Neuron> &b) {
    if (a->shape[3] < b->shape[3]) {
      return true;
    } else if (a->shape[3] == b->shape[3]) {
      return a->shape[1] < b->shape[1];
    } else {
      return false;
    }
  });
  _bottoms = inputs;
  _tops = outputs;

  _nms_threshold = param.get<float>("nms_threshold");
  _confidence_threshold = param.get<float>("confidence_threshold");
  _keep_topk = param.get<int32_t>("keep_topk");
}

void RetinaFaceDetectionFunc::run() {
  auto top_data = _tops[0]->cpu_data<float>();
  memset(top_data, 0, _tops[0]->size());

  size_t bottom_count = _bottoms.size();
  assert(bottom_count == 9);

  auto batch = _tops[0]->shape[0];

  for (int b = 0; b < batch; ++b) {
    std::vector<FaceInfo> infos;
    for (size_t i = 0; i < _feature_stride_fpn.size(); ++i) {
      int stride = _feature_stride_fpn[i];

      auto score_data = _bottoms[3*i]->cpu_data<float>() + _bottoms[3*i]->offset(b);
      size_t score_count = _bottoms[3*i]->count() / batch;

      auto bbox_data = _bottoms[3*i+1]->cpu_data<float>() + _bottoms[3*i+1]->offset(b);
      size_t bbox_count = _bottoms[3*i+1]->count() / batch;

      auto landmark_data = _bottoms[3*i+2]->cpu_data<float>() + _bottoms[3*i+2]->offset(b);
      size_t landmark_count = _bottoms[3*i+2]->count() / batch;

      auto shape = _bottoms[3*i]->shape;
      size_t height = shape[2];
      size_t width = shape[3];

      std::vector<float> score(score_data + score_count / 2, score_data + score_count);
      std::vector<float> bbox(bbox_data, bbox_data + bbox_count);
      std::vector<float> landmark(landmark_data, landmark_data + landmark_count);

      int count = height * width;
      std::string key = "stride" + std::to_string(stride);
      auto anchors_fpn = _anchors_fpn[key];
      auto num_anchors = _num_anchors[key];

      std::vector<AnchorBox> anchors = anchors_plane(height, width, stride, anchors_fpn);

      for (int num = 0; num < num_anchors; ++num) {
        for (int j = 0; j < count; ++j) {
          float confidence = score[j + count * num];
          if (confidence <= _confidence_threshold)
            continue;

          float dx = bbox[j + count * (0 + num * 4)];
          float dy = bbox[j + count * (1 + num * 4)];
          float dw = bbox[j + count * (2 + num * 4)];
          float dh = bbox[j + count * (3 + num * 4)];
          std::vector<float> bbox_deltas{dx, dy, dw, dh};
          auto bbox = bbox_pred(anchors[j + count * num], bbox_deltas);

          std::vector<float> landmark_deltas(10, 0);
          for (size_t k = 0; k < 5; ++k) {
            landmark_deltas[k] = landmark[j + count * (num * 10 + k * 2)];
            landmark_deltas[k + 5] = landmark[j + count * (num * 10 + k * 2 + 1)];
          }

          auto pts = landmark_pred(anchors[j + count * num], landmark_deltas);

          FaceInfo info;
          info.x1 = bbox[0];
          info.y1 = bbox[1];
          info.x2 = bbox[2];
          info.y2 = bbox[3];
          info.score = confidence;
          for (int idx = 0; idx < 5; ++idx) {
            info.x[idx] = pts[idx];
            info.y[idx] = pts[idx + 5];
          }

          infos.push_back(info);
        }
      }
    }

    auto preds = nms(infos, _nms_threshold);
    auto keep_topk = _keep_topk;
    if (keep_topk > (int)preds.size())
      keep_topk = (int)preds.size();

    long long count = 0;
    auto batch_top_data = top_data + _tops[0]->offset(b);
    for (int i = 0; i < keep_topk; ++i) {
      batch_top_data[count++] = preds[i].x1;
      batch_top_data[count++] = preds[i].y1;
      batch_top_data[count++] = preds[i].x2;
      batch_top_data[count++] = preds[i].y2;
      batch_top_data[count++] = preds[i].score;
      for (int j = 0; j < 5; ++j) {
        batch_top_data[count++] = preds[i].x[j];
        batch_top_data[count++] = preds[i].y[j];
      }

#if 0
      TPU_LOG_DEBUG(
          "x1 = %f, y1 = %f, x2 = %f, y2 = %f, score = %f,"
          "pts1 = %f, pts2 = %f, pts3 = %f, pts4 = %f, pts5 = %f"
          "pts6 = %f, pts7 = %f, pts8 = %f, pts9 = %f, pts10 = %f\n",
          preds[i].x1, preds[i].y1, preds[i].x2, preds[i].y2, preds[i].score,
          preds[i].x[0], preds[i].y[0], preds[i].x[1], preds[i].y[1],
          preds[i].x[2], preds[i].y[2], preds[i].x[3], preds[i].y[3],
          preds[i].x[4], preds[i].y[4]);
#endif
    }
  }
}

} // namespace runtime
} // namespace cvi
