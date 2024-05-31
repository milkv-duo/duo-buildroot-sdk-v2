#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/proposal.hpp>

namespace cvi {
namespace runtime {

static void _mkanchors(std::vector<float> ctrs, std::vector<float> &anchors) {
  anchors.push_back(ctrs[2] - 0.5*(ctrs[0] - 1));
  anchors.push_back(ctrs[3] - 0.5*(ctrs[1] - 1));
  anchors.push_back(ctrs[2] + 0.5*(ctrs[0] - 1));
  anchors.push_back(ctrs[3] + 0.5*(ctrs[1] - 1));
}

static void _whctrs(std::vector<float> anchor, std::vector<float> &ctrs) {
  float w = anchor[2] - anchor[0] + 1;
  float h = anchor[3] - anchor[1] + 1;
  float x_ctr = anchor[0] + 0.5 * (w - 1);
  float y_ctr = anchor[1] + 0.5 * (h - 1);
  ctrs.push_back(w);
  ctrs.push_back(h);
  ctrs.push_back(x_ctr);
  ctrs.push_back(y_ctr);
}

static void _ratio_enum(std::vector<float> anchor, std::vector<float> anchor_ratio,
                 std::vector<float> &ratio_anchors) {
  std::vector<float> ctrs;
  _whctrs(anchor, ctrs);
  float size = ctrs[0] * ctrs[1];
  int ratio_num = anchor_ratio.size();
  for (int i = 0; i < ratio_num; i++)
  {
    float ratio = size / anchor_ratio[i];
    int ws = int(std::round(std::sqrt(ratio)));
    int hs = int(std::round(ws * anchor_ratio[i]));
    std::vector<float> ctrs_in;
    ctrs_in.push_back(ws);
    ctrs_in.push_back(hs);
    ctrs_in.push_back(ctrs[2]);
    ctrs_in.push_back(ctrs[3]);
    _mkanchors(ctrs_in, ratio_anchors);
  }
}

static void _scale_enum(std::vector<float> ratio_anchors, std::vector<float> anchor_scale,
                 std::vector<float> &anchor_boxes) {
  int anchors_ratio_num = ratio_anchors.size() / 4;
  for (int i = 0; i < anchors_ratio_num; i++)
  {
    std::vector<float> anchor;
    anchor.push_back(ratio_anchors[i * 4]);
    anchor.push_back(ratio_anchors[i * 4 + 1]);
    anchor.push_back(ratio_anchors[i * 4 + 2]);
    anchor.push_back(ratio_anchors[i * 4 + 3]);
    std::vector<float> ctrs;
    _whctrs(anchor, ctrs);
    int scale_num = anchor_scale.size();
    for (int j = 0; j < scale_num; j++)
    {
      float ws = ctrs[0] * anchor_scale[j];
      float hs = ctrs[1] * anchor_scale[j];
      std::vector<float> ctrs_in;
      ctrs_in.push_back(ws);
      ctrs_in.push_back(hs);
      ctrs_in.push_back(ctrs[2]);
      ctrs_in.push_back(ctrs[3]);
      _mkanchors(ctrs_in, anchor_boxes);
    }
  }
}

static void generate_anchors(int anchor_base_size, std::vector<float> anchor_scale,
                    std::vector<float> anchor_ratio, std::vector<float> &anchor_boxes) {
  std::vector<float> base_anchor = {0, 0, (float)(anchor_base_size - 1), (float)(anchor_base_size - 1)};
  std::vector<float> ratio_anchors;
  _ratio_enum(base_anchor, anchor_ratio, ratio_anchors);
  _scale_enum(ratio_anchors, anchor_scale, anchor_boxes);
}

static void anchor_box_transform_inv(float img_width, float img_height, std::vector<std::vector<float>> bbox,
                    std::vector<std::vector<float>> select_anchor, std::vector<std::vector<float>> &pred)
{
  int num = bbox.size();
  for (int i = 0; i< num; i++)
  {
    float dx = bbox[i][0];
    float dy = bbox[i][1];
    float dw = bbox[i][2];
    float dh = bbox[i][3];
    float pred_ctr_x = select_anchor[i][0] + select_anchor[i][2] * dx;
    float pred_ctr_y = select_anchor[i][1] + select_anchor[i][3] * dy;
    float pred_w = select_anchor[i][2] * std::exp(dw);
    float pred_h = select_anchor[i][3] * std::exp(dh);
    std::vector<float> tmp_pred;
    tmp_pred.push_back(std::max(std::min((float)(pred_ctr_x - 0.5* pred_w), img_width - 1), (float)0.0));
    tmp_pred.push_back(std::max(std::min((float)(pred_ctr_y - 0.5* pred_h), img_height - 1), (float)0.0));
    tmp_pred.push_back(std::max(std::min((float)(pred_ctr_x + 0.5* pred_w), img_width - 1), (float)0.0));
    tmp_pred.push_back(std::max(std::min((float)(pred_ctr_y + 0.5* pred_h), img_height - 1), (float)0.0));
    pred.push_back(tmp_pred);
  }
}

static void anchor_box_nms(std::vector<std::vector<float>> &pred_boxes, std::vector<float> &confidence, float nms_threshold)
{
  for (size_t i = 0; i < pred_boxes.size() - 1; i++)
  {
    float s1 = (pred_boxes[i][2] - pred_boxes[i][0] + 1) *(pred_boxes[i][3] - pred_boxes[i][1] + 1);
    for (size_t j = i + 1; j < pred_boxes.size(); j++)
    {
      float s2 = (pred_boxes[j][2] - pred_boxes[j][0] + 1) *(pred_boxes[j][3] - pred_boxes[j][1] + 1);

      float x1 = std::max(pred_boxes[i][0], pred_boxes[j][0]);
      float y1 = std::max(pred_boxes[i][1], pred_boxes[j][1]);
      float x2 = std::min(pred_boxes[i][2], pred_boxes[j][2]);
      float y2 = std::min(pred_boxes[i][3], pred_boxes[j][3]);

      float width = x2 - x1;
      float height = y2 - y1;
      if (width > 0 && height > 0)
      {
        float IOU = width * height / (s1 + s2 - width * height);
        if (IOU > nms_threshold)
        {
          if (confidence[i] >= confidence[j])
          {
            pred_boxes.erase(pred_boxes.begin() + j);
            confidence.erase(confidence.begin() + j);
            j--;
          }
          else
          {
            pred_boxes.erase(pred_boxes.begin() + i);
            confidence.erase(confidence.begin() + i);
            i--;
            break;
          }
        }
      }
    }
  }
}

ProposalFunc::~ProposalFunc() {}

void ProposalFunc::setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param) {
  feat_stride = param.get<int32_t>("feat_stride");
  anchor_base_size = param.get<int32_t>("anchor_base_size");
  net_input_h = param.get<int32_t>("net_input_h");
  net_input_w = param.get<int32_t>("net_input_w");
  rpn_obj_threshold = param.get<float>("rpn_obj_threshold");
  rpn_nms_threshold = param.get<float>("rpn_nms_threshold");
  rpn_nms_post_top_n = param.get<int32_t>("rpn_nms_post_top_n");

  std::sort(inputs.begin(), inputs.end(),
    [](const std::shared_ptr<Neuron> &a, const std::shared_ptr<Neuron> &b) {
      return a->shape[1] < b->shape[1];
    });

  _bottoms = inputs;
  _tops = outputs;
}

void ProposalFunc::run() {
  auto top_data = _tops[0]->cpu_data<float>();
  memset(top_data, 0, _tops[0]->size());

  size_t bottom_count = _bottoms.size();
  assert(bottom_count == 2);

  float *score = (float *)_bottoms[0]->cpu_data<float>();
  float *bbox_deltas = (float *)_bottoms[1]->cpu_data<float>();

  int batch = _bottoms[0]->shape[0];

  int height = _bottoms[0]->shape[2];
  int width = _bottoms[0]->shape[3];

  std::vector<float> anchor_scale = {8, 16, 32};
  std::vector<float> anchor_ratio = {0.5, 1, 2};

  std::vector<float> anchor_boxes;
  generate_anchors(anchor_base_size, anchor_scale, anchor_ratio, anchor_boxes);

  float thresh = rpn_obj_threshold;

  for (int b = 0; b < batch; ++b) {
    auto batch_score = score + _bottoms[0]->offset(b);
    auto batch_bbox_deltas = bbox_deltas + _bottoms[1]->offset(b);

    std::vector<std::vector<float>> select_anchor;
    std::vector<float> confidence;
    std::vector<std::vector<float>> bbox;
    int anchor_num = anchor_scale.size() * anchor_ratio.size();

    for (int k = 0; k < anchor_num; k++) {
      float w = anchor_boxes[4 * k + 2] - anchor_boxes[4 * k] + 1;
      float h = anchor_boxes[4 * k + 3] - anchor_boxes[4 * k + 1] + 1;
      float x_ctr = anchor_boxes[4 * k] + 0.5 * (w - 1);
      float y_ctr = anchor_boxes[4 * k + 1] + 0.5 * (h - 1);

      for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
          if (batch_score[anchor_num * height * width + (k * height + i) * width + j] >= thresh) {
            std::vector<float> tmp_anchor;
            std::vector<float> tmp_bbox;

            tmp_anchor.push_back(j * feat_stride + x_ctr);
            tmp_anchor.push_back(i * feat_stride + y_ctr);
            tmp_anchor.push_back(w);
            tmp_anchor.push_back(h);
            select_anchor.push_back(tmp_anchor);
            confidence.push_back(batch_score[anchor_num * height * width + (k * height + i) * width + j]);
            tmp_bbox.push_back(batch_bbox_deltas[(4 * k * height + i) * width + j]);
            tmp_bbox.push_back(batch_bbox_deltas[((4 * k +1) * height + i) * width + j]);
            tmp_bbox.push_back(batch_bbox_deltas[((4 * k + 2) * height + i) * width + j]);
            tmp_bbox.push_back(batch_bbox_deltas[((4 * k + 3) * height + i) * width + j]);
            bbox.push_back(tmp_bbox);
          }
        }
      }
    }
    std::vector<std::vector<float>> pred_boxes;
    anchor_box_transform_inv(net_input_w, net_input_h, bbox, select_anchor, pred_boxes);
    anchor_box_nms(pred_boxes, confidence, rpn_nms_threshold);
    int num = pred_boxes.size() > (size_t)(rpn_nms_post_top_n) ? rpn_nms_post_top_n : pred_boxes.size();

    auto batch_top_data = top_data + _tops[0]->offset(b);
    for (int i = 0; i < num; i++) {
      batch_top_data[5 * i] = b;
      batch_top_data[5 * i + 1] = pred_boxes[i][0];
      batch_top_data[5 * i + 2] = pred_boxes[i][1];
      batch_top_data[5 * i + 3] = pred_boxes[i][2];
      batch_top_data[5 * i + 4] = pred_boxes[i][3];
    }
  }
}

}
}
