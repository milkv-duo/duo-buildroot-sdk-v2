#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>

#include "anchor_generator.h"

using namespace std;

static anchor_win _whctrs(anchor_box anchor) {
  anchor_win win;
  win.w = anchor.x2 - anchor.x1 + 1;
  win.h = anchor.y2 - anchor.y1 + 1;
  win.x_ctr = anchor.x1 + 0.5 * (win.w - 1);
  win.y_ctr = anchor.y1 + 0.5 * (win.h - 1);

  return win;
}

static anchor_box _mkanchors(anchor_win win) {
  anchor_box anchor;
  anchor.x1 = win.x_ctr - 0.5 * (win.w - 1);
  anchor.y1 = win.y_ctr - 0.5 * (win.h - 1);
  anchor.x2 = win.x_ctr + 0.5 * (win.w - 1);
  anchor.y2 = win.y_ctr + 0.5 * (win.h - 1);

  return anchor;
}

static anchor_box _mkanchors_pytorch(anchor_win win) {
  anchor_box anchor;

  anchor.x1 = 0;
  anchor.y1 = 0;
  anchor.x2 = anchor.x1 + win.w - 1;
  anchor.y2 = anchor.y1 + win.h - 1;

  return anchor;
}

static vector<anchor_box> _ratio_enum(anchor_box anchor, const vector<float> &ratios) {
  vector<anchor_box> anchors;
  for (size_t i = 0; i < ratios.size(); i++) {
    anchor_win win = _whctrs(anchor);
    float size = win.w * win.h;
    float scale = size / ratios[i];

    win.w = round(std::sqrt(scale));
    win.h = round(win.w * ratios[i]);

    anchor_box tmp = _mkanchors(win);
    anchors.push_back(tmp);
  }

  return anchors;
}

static vector<anchor_box> _scale_enum(anchor_box anchor, const vector<int> &scales,
                                      PROCESS process) {
  vector<anchor_box> anchors;
  for (size_t i = 0; i < scales.size(); i++) {
    anchor_win win = _whctrs(anchor);

    win.w = win.w * scales[i];
    win.h = win.h * scales[i];

    if (process == CAFFE) {
      anchor_box tmp = _mkanchors(win);
      anchors.push_back(tmp);
    } else if (process == PYTORCH) {
      anchor_box tmp = _mkanchors_pytorch(win);
      anchors.push_back(tmp);
    }
  }

  return anchors;
}

vector<anchor_box> generate_anchors(int base_size, const vector<float> &ratios,
                                    const vector<int> &scales, int stride, bool dense_anchor,
                                    PROCESS process) {
  anchor_box base_anchor;
  base_anchor.x1 = 0;
  base_anchor.y1 = 0;
  base_anchor.x2 = base_size - 1;
  base_anchor.y2 = base_size - 1;

  vector<anchor_box> ratio_anchors;
  ratio_anchors = _ratio_enum(base_anchor, ratios);

  vector<anchor_box> anchors;
  for (size_t i = 0; i < ratio_anchors.size(); i++) {
    vector<anchor_box> tmp = _scale_enum(ratio_anchors[i], scales, process);
    anchors.insert(anchors.end(), tmp.begin(), tmp.end());
  }
  if (process == PYTORCH) {
    for (size_t i = 0; i < anchors.size(); i++) {
      float ori_ct_x = (anchors[i].x1 + anchors[i].x2) / 2;
      float ori_ct_y = (anchors[i].y1 + anchors[i].y2) / 2;
      float new_ct_x = stride / 2;
      float new_ct_y = stride / 2;
      float x_shift = round(ori_ct_x - new_ct_x);
      float y_shift = round(ori_ct_y - new_ct_y);
      anchors[i].x1 = anchors[i].x1 - x_shift;
      anchors[i].x2 = anchors[i].x2 - x_shift;
      anchors[i].y1 = anchors[i].y1 - y_shift;
      anchors[i].y2 = anchors[i].y2 - y_shift;
    }
  }

  if (dense_anchor) {
    assert(stride % 2 == 0);

    vector<anchor_box> anchors2 = anchors;
    for (size_t i = 0; i < anchors2.size(); i++) {
      anchors2[i].x1 += stride / 2;
      anchors2[i].y1 += stride / 2;
      anchors2[i].x2 += stride / 2;
      anchors2[i].y2 += stride / 2;
    }
    anchors.insert(anchors.end(), anchors2.begin(), anchors2.end());
  }

  return anchors;
}

vector<vector<anchor_box> > generate_anchors_fpn(bool dense_anchor, const vector<anchor_cfg> &cfg,
                                                 PROCESS process) {
  vector<vector<anchor_box> > anchors;
  for (size_t i = 0; i < cfg.size(); i++) {
    anchor_cfg tmp = cfg[i];
    int bs = tmp.BASE_SIZE;
    vector<float> ratios = tmp.RATIOS;
    vector<int> scales = tmp.SCALES;
    int stride = tmp.STRIDE;

    vector<anchor_box> r = generate_anchors(bs, ratios, scales, stride, dense_anchor, process);
    anchors.push_back(r);
  }

  return anchors;
}

vector<anchor_box> anchors_plane(int height, int width, int stride,
                                 const vector<anchor_box> &base_anchors) {
  vector<anchor_box> all_anchors;
  for (size_t k = 0; k < base_anchors.size(); k++) {
    for (int ih = 0; ih < height; ih++) {
      int sh = ih * stride;
      for (int iw = 0; iw < width; iw++) {
        int sw = iw * stride;

        anchor_box tmp;
        tmp.x1 = base_anchors[k].x1 + sw;
        tmp.y1 = base_anchors[k].y1 + sh;
        tmp.x2 = base_anchors[k].x2 + sw;
        tmp.y2 = base_anchors[k].y2 + sh;
        all_anchors.push_back(tmp);
      }
    }
  }

  return all_anchors;
}
