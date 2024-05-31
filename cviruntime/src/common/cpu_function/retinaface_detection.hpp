#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

struct AnchorCfg {
public:
  AnchorCfg(int stride, std::vector<int> scales, int base_size, std::vector<float> ratios,
            int allowed_border)
      : stride(stride), scales(scales), base_size(base_size), ratios(ratios),
        allowed_border(allowed_border) {}

  int stride;
  std::vector<int> scales;
  int base_size;
  std::vector<float> ratios;
  int allowed_border;
};

struct AnchorBox {
  float x1, y1, x2, y2;
};

struct AnchorCenter {
  float ctr_x, ctr_y, w, h;
};

struct FaceInfo {
  float x1, y1, x2, y2;
  float score;
  float x[5];
  float y[5];
};

class RetinaFaceDetectionFunc : public ICpuFunction {

public:
  RetinaFaceDetectionFunc() {
    _cfg.clear();
    AnchorCfg cfg1(32, {32, 16}, 16, {1.0}, 9999);
    AnchorCfg cfg2(16, {8, 4}, 16, {1.0}, 9999);
    AnchorCfg cfg3(8, {2, 1}, 16, {1.0}, 9999);
    _cfg.push_back(cfg1);
    _cfg.push_back(cfg2);
    _cfg.push_back(cfg3);

    _anchors_fpn.clear();
    auto anchors = generate_anchors_fpn(false, _cfg);
    for (size_t i = 0; i < _feature_stride_fpn.size(); ++i) {
      std::string key = "stride" + std::to_string(_feature_stride_fpn[i]);
      _anchors_fpn[key] = anchors[i];
      _num_anchors[key] = anchors[i].size();
    }
  }

  ~RetinaFaceDetectionFunc() = default;
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs, OpParam &param);
  void run();

  static ICpuFunction *open() { return new RetinaFaceDetectionFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  AnchorCenter mkcenter(AnchorBox &base_anchor) {
    AnchorCenter ctr;
    ctr.w = base_anchor.x2 - base_anchor.x1 + 1;
    ctr.h = base_anchor.y2 - base_anchor.y1 + 1;
    ctr.ctr_x = base_anchor.x1 + 0.5 * (ctr.w - 1);
    ctr.ctr_y = base_anchor.y1 + 0.5 * (ctr.h - 1);
    return ctr;
  }

  AnchorBox mkanchor(AnchorCenter &ctr) {
    AnchorBox anchor;
    anchor.x1 = ctr.ctr_x - 0.5 * (ctr.w - 1);
    anchor.y1 = ctr.ctr_y - 0.5 * (ctr.h - 1);
    anchor.x2 = ctr.ctr_x + 0.5 * (ctr.w + 1);
    anchor.y2 = ctr.ctr_y + 0.5 * (ctr.h + 1);
    return anchor;
  }

  std::vector<AnchorBox> ratio_enum(AnchorBox &base_anchor, std::vector<float> &ratios) {
    std::vector<AnchorBox> anchors;
    for (size_t i = 0; i < ratios.size(); ++i) {
      AnchorCenter ctr = mkcenter(base_anchor);

      float scale = (ctr.w * ctr.h) / ratios[i];
      ctr.w = std::round(std::sqrt(scale));
      ctr.h = std::round(ctr.w * ratios[i]);

      AnchorBox anchor = mkanchor(ctr);
      anchors.push_back(anchor);
    }
    return anchors;
  }

  std::vector<AnchorBox> scale_enum(AnchorBox anchor, std::vector<int> &scales) {
    std::vector<AnchorBox> anchors;
    for (size_t i = 0; i < scales.size(); ++i) {
      auto ctr = mkcenter(anchor);
      ctr.w = ctr.w * scales[i];
      ctr.h = ctr.h * scales[i];

      auto scale_anchor = mkanchor(ctr);
      // LOGI << "x1 = " << scale_anchor.x1 << ",y1 = " << scale_anchor.y1
      //     << ",x2 = " << scale_anchor.x2 << ",y2 = " << scale_anchor.y2;
      anchors.push_back(scale_anchor);
    }

    return anchors;
  }

  std::vector<AnchorBox> generate_anchors(bool dense, AnchorCfg &cfg) {
    AnchorBox base_anchor;
    base_anchor.x1 = 0;
    base_anchor.y1 = 0;
    base_anchor.x2 = cfg.base_size - 1;
    base_anchor.y2 = cfg.base_size - 1;

    auto ratio_anchors = ratio_enum(base_anchor, cfg.ratios);

    std::vector<AnchorBox> anchors;
    for (size_t i = 0; i < ratio_anchors.size(); ++i) {
      auto scale_anchors = scale_enum(ratio_anchors[i], cfg.scales);
      anchors.insert(anchors.end(), scale_anchors.begin(), scale_anchors.end());
    }

    if (dense) {
      // TODO: anchors x and y need to add stride / 2
    }
    return anchors;
  }

  std::vector<std::vector<AnchorBox>>
  generate_anchors_fpn(bool dense, std::vector<AnchorCfg> &cfg) {
    std::vector<std::vector<AnchorBox>> anchors_fpn;
    for (size_t i = 0; i < cfg.size(); ++i) {
      auto anchors = generate_anchors(dense, cfg[i]);
      anchors_fpn.push_back(anchors);
    }
    return anchors_fpn;
  }

  std::vector<AnchorBox> anchors_plane(int height, int width, int stride,
                                       std::vector<AnchorBox> anchors_fpn) {
    std::vector<AnchorBox> anchors;
    for (size_t k = 0; k < anchors_fpn.size(); ++k) {
      for (int ih = 0; ih < height; ++ih) {
        int sh = ih * stride;
        for (int iw = 0; iw < width; ++iw) {
          int sw = iw * stride;
          AnchorBox anchor;
          anchor.x1 = anchors_fpn[k].x1 + sw;
          anchor.y1 = anchors_fpn[k].y1 + sh;
          anchor.x2 = anchors_fpn[k].x2 + sw;
          anchor.y2 = anchors_fpn[k].y2 + sh;
          anchors.push_back(anchor);
          // LOGI << "x1 = " << anchor.x1 << ",y1 = " << anchor.y1
          //      << ",x2 = " << anchor.x2 << ",y2 = " << anchor.y2;
        }
      }
    }

    return anchors;
  }

  std::vector<float> bbox_pred(AnchorBox anchor, std::vector<float> bbox_deltas) {
    std::vector<float> bbox(4, 0);

    float width = anchor.x2 - anchor.x1 + 1;
    float height = anchor.y2 - anchor.y1 + 1;
    float center_x = anchor.x1 + 0.5 * (width - 1);
    float center_y = anchor.y1 + 0.5 * (height - 1);

    float pred_center_x = bbox_deltas[0] * width + center_x;
    float pred_center_y = bbox_deltas[1] * height + center_y;
    float pred_w = exp(bbox_deltas[2]) * width;
    float pred_h = exp(bbox_deltas[3]) * height;

    bbox[0] = pred_center_x - 0.5 * (pred_w - 1);
    bbox[1] = pred_center_y - 0.5 * (pred_h - 1);
    bbox[2] = pred_center_x + 0.5 * (pred_w - 1);
    bbox[3] = pred_center_y + 0.5 * (pred_h - 1);

    return bbox;
  }

  std::vector<float> landmark_pred(AnchorBox anchor, std::vector<float> landmark_deltas) {
    std::vector<float> pts(10, 0);

    float width = anchor.x2 - anchor.x1 + 1;
    float height = anchor.y2 - anchor.y1 + 1;
    float center_x = anchor.x1 + 0.5 * (width - 1);
    float center_y = anchor.y1 + 0.5 * (height - 1);

    for (int i = 0; i < 5; ++i) {
      pts[i] = center_x + landmark_deltas[i] * width;
      pts[i + 5] = center_y + landmark_deltas[i + 5] * height;
    }

    return pts;
  }

  std::vector<FaceInfo> nms(std::vector<FaceInfo> infos, float nms_threshold) {
    std::vector<FaceInfo> infos_nms;
    std::sort(infos.begin(), infos.end(),
              [](FaceInfo &a, FaceInfo &b) { return a.score > b.score; });

    int selected = 0;
    int count = infos.size();
    std::vector<int> mask(count, 0);
    bool exit = false;
    while (!exit) {
      while (selected < count && mask[selected] == 1)
        selected++;

      if (selected == count) {
        exit = true;
        continue;
      }

      infos_nms.push_back(infos[selected]);
      mask[selected] = 1;

      float w1 = infos[selected].x2 - infos[selected].x1 + 1;
      float h1 = infos[selected].y2 - infos[selected].y1 + 1;
      float area1 = w1 * h1;

      selected++;
      for (int i = selected; i < count; ++i) {
        if (mask[i] == 1)
          continue;

        float w2 = infos[i].x2 - infos[i].x1 + 1;
        float h2 = infos[i].y2 - infos[i].y1 + 1;
        float area2 = w2 * h2;

        float inter_x1 = std::max(infos[selected].x1, infos[i].x1);
        float inter_y1 = std::max(infos[selected].y1, infos[i].y1);
        float inter_x2 = std::min(infos[selected].x2, infos[i].x2);
        float inter_y2 = std::min(infos[selected].y2, infos[i].y2);

        float w = inter_x2 - inter_x1 + 1;
        float h = inter_y2 - inter_y1 + 1;

        if (w <= 0 || h <= 0)
          continue;

        float iou = w * h / (area1 + area2 - w * h);
        if (iou > nms_threshold) {
          mask[i] = 1;
        }
      }
    }

    return infos_nms;
  }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  float _nms_threshold;
  float _confidence_threshold;
  int _keep_topk;

  std::unordered_map<std::string, std::vector<AnchorBox>> _anchors_fpn;
  std::unordered_map<std::string, int> _num_anchors;
  std::vector<AnchorCfg> _cfg;
  std::vector<int> _feature_stride_fpn{32, 16, 8};
};

} // namespace runtime
} // namespace cvi
