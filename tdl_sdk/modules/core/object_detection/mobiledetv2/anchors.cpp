#include "anchors.hpp"
#include <cmath>
using namespace std;

namespace cvitdl {

AnchorConfig::AnchorConfig(int _stride, float _octave_scale, std::pair<float, float> _aspect)
    : stride(_stride), octave_scale(_octave_scale), aspect(_aspect) {}

RetinaNetAnchorGenerator::RetinaNetAnchorGenerator(
    int _min_level, int _max_level, int _num_scales,
    const std::vector<pair<float, float>> &_aspect_ratios, float _anchor_scale, int _image_width,
    int _image_height)
    : min_level(_min_level),
      max_level(_max_level),
      num_scales(_num_scales),
      aspect_ratios(_aspect_ratios),
      anchor_scale(_anchor_scale),
      image_width(_image_width),
      image_height(_image_height) {
  _generate_configs();
  _generate_boxes();
}

void RetinaNetAnchorGenerator::_generate_configs() {
  for (int level = min_level; level <= max_level; level++) {
    anchor_configs[level] = {};
    for (int scale_octave = 0; scale_octave < num_scales; scale_octave++) {
      for (auto aspect : aspect_ratios) {
        anchor_configs[level].push_back(
            AnchorConfig(pow(2, level), static_cast<float>(scale_octave) / num_scales, aspect));
      }
    }
  }
}

void RetinaNetAnchorGenerator::_generate_boxes() {
  for (int level = min_level; level <= max_level; level++) {
    anchor_bboxes.push_back(vector<AnchorBox>());
    vector<AnchorBox> &boxes_level = anchor_bboxes[anchor_bboxes.size() - 1];
    int stride = anchor_configs[level][0].stride;

    for (int y = stride / 2; y < image_height; y += stride) {
      for (int x = stride / 2; x < image_width; x += stride) {
        for (auto config : anchor_configs[level]) {
          float octave_scale = config.octave_scale;

          float base_anchor_size = anchor_scale * stride * pow(2, octave_scale);

          float box_width = base_anchor_size * config.aspect.first;
          float box_height = base_anchor_size * config.aspect.second;
          float anchor_size_x_2 = box_width / 2;
          float anchor_size_y_2 = box_height / 2;

          boxes_level.push_back(
              AnchorBox(x - anchor_size_x_2, y - anchor_size_y_2, box_width, box_height));
        }
      }
    }
  }
}

const vector<vector<AnchorBox>> &RetinaNetAnchorGenerator::get_anchor_boxes() const {
  return anchor_bboxes;
}
}  // namespace cvitdl