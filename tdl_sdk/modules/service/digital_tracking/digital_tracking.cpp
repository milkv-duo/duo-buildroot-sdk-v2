#include "digital_tracking.hpp"
#include "../draw_rect/draw_rect.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl_log.hpp"
#include "rescale_utils.hpp"

#include <algorithm>

#define DEFAULT_DT_ZOOM_TRANS_RATIO 0.1f

namespace cvitdl {
namespace service {

int DigitalTracking::setVpssTimeout(uint32_t timeout) {
  m_vpss_timeout = timeout;
  return CVI_TDL_SUCCESS;
}

int DigitalTracking::setVpssEngine(VpssEngine *engine) {
  mp_vpss_inst = engine;
  return CVI_TDL_SUCCESS;
}

template <typename T>
int DigitalTracking::run(const VIDEO_FRAME_INFO_S *srcFrame, const T *meta,
                         VIDEO_FRAME_INFO_S *dstFrame, const float pad_l, const float pad_r,
                         const float pad_t, const float pad_b, const float face_skip_ratio,
                         const float trans_ratio) {
  if (mp_vpss_inst == nullptr) {
    LOGE("vpss_inst not set.\n");
    return CVI_TDL_FAILURE;
  }
  uint32_t width = srcFrame->stVFrame.u32Width;
  uint32_t height = srcFrame->stVFrame.u32Height;
  Rect rect;

  if (0 == meta->size) {
    rect = Rect(0, width - 1, 0, height - 1);
  } else {
    rect = Rect(width, 0, height, 0);
    const float total_size = width * height;
    for (uint32_t i = 0; i < meta->size; ++i) {
      cvtdl_bbox_t bbox = cvitdl::box_rescale(width, height, meta->width, meta->height,
                                              meta->info[i].bbox, meta->rescale_type);
      const float &&ww = bbox.x2 - bbox.x1;
      const float &&hh = bbox.y2 - bbox.y1;
      if (ww < 4 || hh < 4) {
        continue;
      }
      const float &&box_size = ww * hh;
      if (box_size / total_size < face_skip_ratio) {
        continue;
      }

      rect.l = std::min(rect.l, bbox.x1);
      rect.r = std::max(rect.r, bbox.x2);
      rect.t = std::min(rect.t, bbox.y1);
      rect.b = std::max(rect.b, bbox.y2);
    }
    if (std::abs(rect.l - rect.r) < 4) {
      rect.l = 0;
      rect.r = width;
    } else if (rect.l > rect.r) {
      std::swap(rect.l, rect.r);
    }
    if (std::abs(rect.t - rect.b) < 4) {
      rect.t = 0;
      rect.b = height;
    } else if (rect.t > rect.b) {
      std::swap(rect.t, rect.b);
    }
    rect.add_padding(pad_l, pad_r, pad_t, pad_b);
    fitRatio(width, height, &rect);
  }

  if (!m_prev_rect.is_valid()) {
    m_prev_rect = Rect(0, width - 1, 0, height - 1);
  }
  transformRect(trans_ratio, m_prev_rect, &rect);
  fitFrame(width, height, &rect);

  VPSS_CHN_ATTR_S chnAttr;
  VPSS_CHN_DEFAULT_HELPER(&chnAttr, width, height, srcFrame->stVFrame.enPixelFormat, true);
  VPSS_CROP_INFO_S cropAttr;
#ifdef DT_DEBUG
  cropAttr.bEnable = false;
#else
  cropAttr.bEnable = true;
#endif
  cropAttr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
  cropAttr.stCropRect = {(int)rect.l, (int)rect.t, (uint32_t)(rect.r - rect.l),
                         (uint32_t)(rect.b - rect.t)};
  mp_vpss_inst->sendCropChnFrame(srcFrame, &cropAttr, &chnAttr, 1);
  mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
#ifdef DT_DEBUG
  color_rgb rgb_color;
  rgb_color.r = DEFAULT_RECT_COLOR_R;
  rgb_color.g = DEFAULT_RECT_COLOR_G;
  rgb_color.b = DEFAULT_RECT_COLOR_B;
  cvitdl::service::DrawRect(dstFrame, rect.l, rect.r, rect.t, rect.b, "", rgb_color,
                            DEFAULT_RECT_THICKNESS, false);
#endif
  m_prev_rect = rect;
  return CVI_TDL_SUCCESS;
}

void DigitalTracking::transformRect(const float trans_ratio, const Rect &prev_rect,
                                    Rect *curr_rect) {
  curr_rect->l = prev_rect.l * (1.0 - trans_ratio) + curr_rect->l * trans_ratio;
  curr_rect->r = prev_rect.r * (1.0 - trans_ratio) + curr_rect->r * trans_ratio;
  curr_rect->t = prev_rect.t * (1.0 - trans_ratio) + curr_rect->t * trans_ratio;
  curr_rect->b = prev_rect.b * (1.0 - trans_ratio) + curr_rect->b * trans_ratio;
}

void DigitalTracking::fitRatio(const float width, const float height, Rect *rect) {
  float origin_ratio = (double)height / width;
  if ((rect->b - rect->t) > (rect->r - rect->l)) {
    float new_w = (rect->b - rect->t) / origin_ratio;
    float w_centor = (rect->l + rect->r) / 2;
    rect->l = w_centor - new_w / 2;
    rect->r = rect->l + new_w;
  } else {
    float new_h = (rect->r - rect->l) * origin_ratio;
    float h_centor = (rect->t + rect->b) / 2;
    rect->t = h_centor - new_h / 2;
    rect->b = rect->t + new_h;
  }
}

void DigitalTracking::fitFrame(const float width, const float height, Rect *rect) {
  if (((rect->r - rect->l + 1) > width) || ((rect->b - rect->t + 1) > height)) {
    rect->l = 0;
    rect->r = width - 1;
    rect->t = 0;
    rect->b = height - 1;
  } else {
    if (rect->l < 0) {
      rect->r -= rect->l;
      rect->l = 0;
    } else if (rect->r >= width) {
      rect->l -= (rect->r - width);
      rect->r = width;
    }

    if (rect->t < 0) {
      rect->b -= rect->t;
      rect->t = 0;
    } else if (rect->b >= height) {
      rect->t -= (rect->b - height);
      rect->b = height;
    }
  }
}

template int DigitalTracking::run<cvtdl_face_t>(const VIDEO_FRAME_INFO_S *srcFrame,
                                                const cvtdl_face_t *meta,
                                                VIDEO_FRAME_INFO_S *dstFrame, const float pad_l,
                                                const float pad_r, const float pad_t,
                                                const float pad_b, const float face_skip_ratio,
                                                const float trans_ratio);
template int DigitalTracking::run<cvtdl_object_t>(const VIDEO_FRAME_INFO_S *srcFrame,
                                                  const cvtdl_object_t *meta,
                                                  VIDEO_FRAME_INFO_S *dstFrame, const float pad_l,
                                                  const float pad_r, const float pad_t,
                                                  const float pad_b, const float face_skip_ratio,
                                                  const float trans_ratio);
}  // namespace service
}  // namespace cvitdl