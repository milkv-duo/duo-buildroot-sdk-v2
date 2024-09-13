#pragma once
#include <cstdint>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>

namespace cvitdl {

struct AnchorConfig {
  int stride;
  float octave_scale;
  std::pair<float, float> aspect;
  AnchorConfig(int _stride, float _octave_scale, std::pair<float, float> _aspect);
};

struct AnchorBox {
  float x;
  float y;
  float w;
  float h;
  AnchorBox(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
};

class RetinaNetAnchorGenerator {
 public:
  // RetinaNet Anchors class

  RetinaNetAnchorGenerator(int _min_level, int _max_level, int _num_scales,
                           const std::vector<std::pair<float, float>> &_aspect_ratios,
                           float _anchor_scale, int _image_width, int _image_height);

  /**
   * @brief   Create anchor configurations
   * @note
   **/
  void _generate_configs();

  /**
   * @brief   Create anchor boxes
   * @note
   **/
  void _generate_boxes();

  const std::vector<std::vector<AnchorBox>> &get_anchor_boxes() const;

 private:
  int min_level;
  int max_level;
  int num_scales;
  std::vector<std::pair<float, float>> aspect_ratios;
  float anchor_scale;
  int image_width;
  int image_height;
  std::vector<std::vector<AnchorBox>> anchor_bboxes;
  std::map<int, std::vector<AnchorConfig>> anchor_configs;
};
}  // namespace cvitdl