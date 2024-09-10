#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

#include <vector>
#include "cvi_comm.h"
#include "service/cvi_tdl_service_types.h"

#define DEFAULT_RECT_COLOR_R (53. / 255.)
#define DEFAULT_RECT_COLOR_G (208. / 255.)
#define DEFAULT_RECT_COLOR_B (217. / 255.)
#define DEFAULT_RECT_THICKNESS 4
#define DEFAULT_TEXT_THICKNESS 1
#define DEFAULT_RADIUS 1

namespace cvitdl {
namespace service {

typedef struct {
  float r;
  float g;
  float b;
} color_rgb;

void DrawRect(VIDEO_FRAME_INFO_S *frame, float x1, float x2, float y1, float y2, const char *name,
              color_rgb color, int rect_thinkness, const bool draw_text);

int _WriteText(VIDEO_FRAME_INFO_S *frame, int x, int y, const char *name, color_rgb color,
               int thinkness);

int WriteText(char *name, int x, int y, VIDEO_FRAME_INFO_S *drawFrame, float r, float g, float b);

template <typename T>
int DrawMeta(const T *meta, VIDEO_FRAME_INFO_S *drawFrame, const bool drawText,
             cvtdl_service_brush_t brush);

template <typename T>
int DrawMeta(const T *meta, VIDEO_FRAME_INFO_S *drawFrame, const bool drawText,
             const std::vector<cvtdl_service_brush_t> &brushes);

int DrawPose17(const cvtdl_object_t *obj, VIDEO_FRAME_INFO_S *frame);

int DrawPts(cvtdl_pts_t *pts, VIDEO_FRAME_INFO_S *drawFrame);

void _DrawPts(VIDEO_FRAME_INFO_S *frame, cvtdl_pts_t *pts, color_rgb color, int raduis);

int Draw5Landmark(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame);

int DrawPolygon(VIDEO_FRAME_INFO_S *frame, const cvtdl_pts_t *pts, cvtdl_service_brush_t brush);

int DrawHandPose21(const cvtdl_handpose21_meta_ts *obj, VIDEO_FRAME_INFO_S *bg);
}  // namespace service
}  // namespace cvitdl
