#ifndef _ANCHOR_GENERATOR_H_
#define _ANCHOR_GENERATOR_H_

#include <vector>

enum PROCESS { CAFFE = 0, PYTORCH };

struct anchor_win {
  float x_ctr;
  float y_ctr;
  float w;
  float h;
};

struct anchor_box {
  float x1;
  float y1;
  float x2;
  float y2;
};

struct anchor_cfg {
 public:
  int STRIDE;
  std::vector<int> SCALES;
  int BASE_SIZE;
  std::vector<float> RATIOS;
  int ALLOWED_BORDER;

  anchor_cfg() {
    STRIDE = 0;
    SCALES.clear();
    BASE_SIZE = 0;
    RATIOS.clear();
    ALLOWED_BORDER = 0;
  }
};

std::vector<anchor_box> generate_anchors(int base_size, const std::vector<float> &ratios,
                                         const std::vector<int> &scales, int stride,
                                         bool dense_anchor);
std::vector<std::vector<anchor_box> > generate_anchors_fpn(bool dense_anchor,
                                                           const std::vector<anchor_cfg> &cfg,
                                                           PROCESS proces);
std::vector<anchor_box> anchors_plane(int height, int width, int stride,
                                      const std::vector<anchor_box> &base_anchors);

#endif
