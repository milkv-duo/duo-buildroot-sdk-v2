#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"
#include "vpss_engine.hpp"

#include <cmath>

namespace cvitdl {
namespace service {

struct Rect {
  Rect() {}
  Rect(float l, float r, float t, float b) : l(l), r(r), t(t), b(b) {}

  void add_padding(float ratio) {
    float w = std::abs(r - l) * ratio;
    float h = std::abs(t - b) * ratio;
    l -= w;
    r += w;
    t -= h;
    b += h;
  }
  void add_padding(float ratio_l, float ratio_r, float ratio_t, float ratio_b) {
    float w = std::abs(r - l);
    float h = std::abs(t - b);
    l -= w * ratio_l;
    r += w * ratio_r;
    t -= h * ratio_t;
    b += h * ratio_b;
  }

  bool is_valid() { return (l != -1); }

  float l = -1;
  float r = -1;
  float t = -1;
  float b = -1;
};

class DigitalTracking {
 public:
  int setVpssTimeout(uint32_t timeout);
  int setVpssEngine(VpssEngine *engine);
  template <typename T>
  int run(const VIDEO_FRAME_INFO_S *srcFrame, const T *meta, VIDEO_FRAME_INFO_S *dstFrame,
          const float pad_l = 0.3f, const float pad_r = 0.3f, const float pad_t = 0.3f,
          const float pad_b = 0.3f, const float face_skip_ratio = 0.05f,
          const float trans_ratio = 0.1f);

 private:
  inline void transformRect(const float trans_ratio, const Rect &prev_rect, Rect *curr_rect);
  inline void fitRatio(const float width, const float height, Rect *rect);
  inline void fitFrame(const float width, const float height, Rect *rect);

  Rect m_prev_rect;
  uint32_t m_vpss_timeout = 100;
  VpssEngine *mp_vpss_inst = nullptr;
};
}  // namespace service
}  // namespace cvitdl
