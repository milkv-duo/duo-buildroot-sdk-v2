#include "draw_rect.hpp"

#include <cvi_sys.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include "core/core/cvtdl_errno.h"
#include "core_utils.hpp"
#ifndef NO_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#endif
#include "cvi_tdl_log.hpp"
#define min(x, y) (((x) <= (y)) ? (x) : (y))
#define max(x, y) (((x) >= (y)) ? (x) : (y))

#define COLOR_WRAPPER(R, G, B) \
  { .r = (R / 255.), .g = (G / 255.), .b = (B / 255.) }

static std::vector<std::pair<int, int>> l_pair = {{0, 1},   {0, 2},   {1, 3},   {2, 4},   {5, 6},
                                                  {5, 7},   {7, 9},   {6, 8},   {8, 10},  {17, 11},
                                                  {17, 12}, {11, 13}, {12, 14}, {13, 15}, {14, 16}};

#ifndef NO_OPENCV
static std::vector<cv::Scalar> p_color = {
    {0, 255, 255},  {0, 191, 255},  {0, 255, 102},  {0, 77, 255},   {0, 255, 0},    {77, 255, 255},
    {77, 255, 204}, {77, 204, 255}, {191, 255, 77}, {77, 191, 255}, {191, 255, 77}, {204, 77, 255},
    {77, 255, 204}, {191, 77, 255}, {77, 255, 191}, {127, 77, 255}, {77, 255, 127}, {0, 255, 255}};

static std::vector<cv::Scalar> line_color = {
    {0, 215, 255},   {0, 255, 204},  {0, 134, 255},  {0, 255, 50},  {77, 255, 222},
    {77, 196, 255},  {77, 135, 255}, {191, 255, 77}, {77, 255, 77}, {77, 222, 255},
    {255, 156, 127}, {0, 127, 255},  {255, 127, 77}, {0, 77, 255},  {255, 77, 36}};

#endif
namespace cvitdl {
namespace service {

static const color_rgb COLOR_BLACK = COLOR_WRAPPER(0, 0, 0);
static const color_rgb COLOR_WHITE = COLOR_WRAPPER(255, 255, 255);
static const color_rgb COLOR_RED = COLOR_WRAPPER(255, 0, 0);
static const color_rgb COLOR_GREEN = COLOR_WRAPPER(0, 255, 0);
static const color_rgb COLOR_BLUE = COLOR_WRAPPER(0, 0, 255);
static const color_rgb COLOR_YELLOW = COLOR_WRAPPER(255, 255, 0);
static const color_rgb COLOR_CYAN = COLOR_WRAPPER(0, 255, 255);
static const color_rgb COLOR_MAGENTA = COLOR_WRAPPER(255, 0, 255);

enum { PLANE_Y = 0, PLANE_U, PLANE_V, PLANE_NUM };

static float GetYuvColor(int chanel, color_rgb *color) {
  if (color == NULL) {
    return 0;
  }

  float yuv_color = 0;
  if (chanel == PLANE_Y) {
    yuv_color = (0.257 * color->r) + (0.504 * color->g) + (0.098 * color->b) + 16;
  } else if (chanel == PLANE_U) {
    yuv_color = -(.148 * color->r) - (.291 * color->g) + (.439 * color->b) + 128;
  } else if (chanel == PLANE_V) {
    yuv_color = (0.439 * color->r) - (0.368 * color->g) - (0.071 * color->b) + 128;
  }

  return (yuv_color < 0) ? 0 : ((yuv_color > 255.) ? 255 : yuv_color);
}
// TODO: Need refactor
void _DrawPts(VIDEO_FRAME_INFO_S *frame, cvtdl_pts_t *pts, color_rgb color, int radius) {
#ifdef NO_OPENCV
  LOGW("no opencv could not draw points");
#else
  int width = frame->stVFrame.u32Width;
  int height = frame->stVFrame.u32Height;

  color.r *= 255;
  color.g *= 255;
  color.b *= 255;
  char color_y = GetYuvColor(PLANE_Y, &color);
  char color_u = GetYuvColor(PLANE_U, &color);
  char color_v = GetYuvColor(PLANE_V, &color);

  size_t image_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  bool do_unmap = false;
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  if (frame->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
    // 0: Y-plane, 1: U-plane, 2: V-plane
    for (int i = PLANE_Y; i < PLANE_NUM; i++) {
      char draw_color;
      if (i == PLANE_Y) {
        draw_color = color_y;
      } else if (i == PLANE_U) {
        draw_color = color_u;
      } else {
        draw_color = color_v;
      }

      cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
      if (i != 0) {
        cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
      }
      // FIXME: Color incorrect.
      cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i], frame->stVFrame.u32Stride[i]);
      for (int t = 0; t < (int)pts->size; ++t) {
        if (i == 0) {
          cv::circle(image, cv::Point(pts->x[t], pts->y[t] - 2), radius / 2, cv::Scalar(draw_color),
                     -1);
        } else {
          cv::circle(image, cv::Point(pts->x[t] / 2, (pts->y[t] - 2) / 2), radius,
                     cv::Scalar(draw_color), -1);
        }
      }
    }
  } else { /* PIXEL_FORMAT_NV21 */
    // 0: Y-plane, 1: VU-plane
    for (int i = 0; i < 2; i++) {
      for (int t = 0; t < (int)pts->size; ++t) {
        float x = max(min(pts->x[t], width - 1), 0);
        float y = max(min(pts->y[t], height - 1), 0);
        cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
        cv::Point cv_point = cv::Point(x, y - 2);
        double font_scale = 1;
        int draw_radius = max(radius, 2);
        if (i != 0) {
          cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
          cv_point = cv::Point(x / 2, (y - 2) / 2);
          font_scale /= 2;
          draw_radius /= 2;
        }

        if (i == 0) {
          cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i],
                        frame->stVFrame.u32Stride[i]);
          cv::circle(image, cv_point, draw_radius, cv::Scalar(static_cast<uint8_t>(color_y)), -1);
        } else {
          cv::Mat image(cv_size, CV_8UC2, frame->stVFrame.pu8VirAddr[i],
                        frame->stVFrame.u32Stride[i]);
          cv::circle(image, cv_point, draw_radius,
                     cv::Scalar(static_cast<uint8_t>(color_v), static_cast<uint8_t>(color_u)), -1);
        }
      }
    }
  }

  CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0], image_size);
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
#endif
}

// TODO: Need refactor
int _WriteText(VIDEO_FRAME_INFO_S *frame, int x, int y, const char *name, color_rgb color,
               int thickness) {
#ifdef NO_OPENCV
  return CVI_TDL_FAILURE;
#else
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
    LOGE("Only PIXEL_FORMAT_NV21 and PIXEL_FORMAT_YUV_PLANAR_420 are supported in DrawPolygon\n");
    return CVI_TDL_FAILURE;
  }
  std::string name_str = name;
  int width = frame->stVFrame.u32Width;
  int height = frame->stVFrame.u32Height;
  x = max(min(x, width - 1), 0);
  y = max(min(y, height - 1), 0);

  color.r *= 255;
  color.g *= 255;
  color.b *= 255;
  char color_y = GetYuvColor(PLANE_Y, &color);
  char color_u = GetYuvColor(PLANE_U, &color);
  char color_v = GetYuvColor(PLANE_V, &color);

  size_t image_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  bool do_unmap = false;
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  if (frame->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
    // 0: Y-plane, 1: U-plane, 2: V-plane
    for (int i = PLANE_Y; i < PLANE_NUM; i++) {
      char draw_color;
      if (i == PLANE_Y) {
        draw_color = color_y;
      } else if (i == PLANE_U) {
        draw_color = color_u;
      } else {
        draw_color = color_v;
      }

      cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
      cv::Point cv_point = cv::Point(x, y - 2);
      double font_scale = 1;
      if (i != 0) {
        cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
        cv_point = cv::Point(x / 2, (y - 2) / 2);
        font_scale /= 2;
        // FIXME: Should div but don't know why it's not correct.
        // thickness /= 2;
      }
      // FIXME: Color incorrect.
      cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i], frame->stVFrame.u32Stride[i]);
      cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_COMPLEX_SMALL, font_scale,
                  cv::Scalar(draw_color), thickness, cv::LINE_AA);
    }
  } else { /* PIXEL_FORMAT_NV21 */
    // 0: Y-plane, 1: VU-plane
    for (int i = 0; i < 2; i++) {
      cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
      cv::Point cv_point = cv::Point(x, y - 2);
      double font_scale = 1;
      int text_thickness = max(thickness, 2);
      if (i != 0) {
        cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
        cv_point = cv::Point(x / 2, (y - 2) / 2);
        font_scale /= 2;
        text_thickness /= 2;
      }

      if (i == 0) {
        cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i],
                      frame->stVFrame.u32Stride[i]);
        cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(static_cast<uint8_t>(color_y)), text_thickness, 8);
      } else {
        cv::Mat image(cv_size, CV_8UC2, frame->stVFrame.pu8VirAddr[i],
                      frame->stVFrame.u32Stride[i]);
        cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(static_cast<uint8_t>(color_v), static_cast<uint8_t>(color_u)),
                    text_thickness, 8);
      }
    }
  }
  CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0], image_size);
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }

  return CVI_TDL_SUCCESS;
#endif
}

typedef enum {
  FORMAT_YUV_420P,
  FORMAT_NV21,
} PixelFormat;

template <PixelFormat format>
void DrawRect(VIDEO_FRAME_INFO_S *frame, float x1, float x2, float y1, float y2, const char *name,
              color_rgb color, int rect_thickness, const bool draw_text);

template <>
void DrawRect<FORMAT_YUV_420P>(VIDEO_FRAME_INFO_S *frame, float x1, float x2, float y1, float y2,
                               const char *name, color_rgb color, int rect_thickness,
                               const bool draw_text) {
  std::string name_str = name;
  int width = frame->stVFrame.u32Width;
  int height = frame->stVFrame.u32Height;
  x1 = max(min(x1, width - 1), 0);
  x2 = max(min(x2, width - 1), 0);
  y1 = max(min(y1, height - 1), 0);
  y2 = max(min(y2, height - 1), 0);

  uint8_t color_y = GetYuvColor(PLANE_Y, &color);
  uint8_t color_u = GetYuvColor(PLANE_U, &color);
  uint8_t color_v = GetYuvColor(PLANE_V, &color);

  for (int i = PLANE_Y; i < PLANE_NUM; i++) {
    int stride = frame->stVFrame.u32Stride[i];

    int draw_x1 = ((int)x1 >> 2) << 2;
    int draw_x2 = ((int)x2 >> 2) << 2;
    int draw_y1 = ((int)y1 >> 2) << 2;
    int draw_y2 = ((int)y2 >> 2) << 2;
    int draw_rect_thickness = rect_thickness;
    uint8_t draw_color;
    if (i == PLANE_Y) {
      draw_color = color_y;
    } else if (i == PLANE_U) {
      draw_color = color_u;
    } else {
      draw_color = color_v;
    }

    if (i > PLANE_Y) {
      // uv plane has half size
      draw_x1 /= 2;
      draw_x2 /= 2;
      draw_y1 /= 2;
      draw_y2 /= 2;
      draw_rect_thickness /= 2;
    }

    // draw rect vertical line
    for (int h = draw_y1; h < draw_y2; ++h) {
      for (int w = draw_x1; w < draw_x1 + draw_rect_thickness; ++w) {
        memset((void *)(frame->stVFrame.pu8VirAddr[i] + h * stride + w), draw_color,
               sizeof(draw_color));
      }
      for (int w = draw_x2 - draw_rect_thickness; (w < draw_x2) && (w >= 0); ++w) {
        memset((void *)(frame->stVFrame.pu8VirAddr[i] + h * stride + w), draw_color,
               sizeof(draw_color));
      }
    }

    // draw rect horizontal line
    for (int w = draw_x1; w < draw_x2; ++w) {
      for (int h = draw_y1; h < draw_y1 + draw_rect_thickness; ++h) {
        memset((void *)(frame->stVFrame.pu8VirAddr[i] + h * stride + w), draw_color,
               sizeof(draw_color));
      }
      for (int h = draw_y2 - draw_rect_thickness; (h < draw_y2) && (h >= 0); ++h) {
        memset((void *)(frame->stVFrame.pu8VirAddr[i] + h * stride + w), draw_color,
               sizeof(draw_color));
      }
    }

    if (!draw_text) {
      continue;
    }
#ifdef NO_OPENCV
    LOGW("no opencv support,could not draw text");
#else
    cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
    cv::Point cv_point = cv::Point(x1, y1 - 2);
    double font_scale = 2;
    int thickness = rect_thickness;
    if (i != 0) {
      cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
      cv_point = cv::Point(x1 / 2, (y1 - 2) / 2);
      font_scale /= 2;
      thickness /= 2;
    }

    cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i], frame->stVFrame.u32Stride[i]);
    cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                cv::Scalar(draw_color), thickness, 8);

#endif
  }
}

template <>
void DrawRect<FORMAT_NV21>(VIDEO_FRAME_INFO_S *frame, float x1, float x2, float y1, float y2,
                           const char *name, color_rgb color, int rect_thickness,
                           const bool draw_text) {
  std::string name_str = name;
  int width = frame->stVFrame.u32Width;
  int height = frame->stVFrame.u32Height;
  x1 = max(min(x1, width - 1), 0);
  x2 = max(min(x2, width - 1), 0);
  y1 = max(min(y1, height - 1), 0);
  y2 = max(min(y2, height - 1), 0);
  uint8_t color_y = GetYuvColor(PLANE_Y, &color);
  uint8_t color_u = GetYuvColor(PLANE_U, &color);
  uint8_t color_v = GetYuvColor(PLANE_V, &color);
  // 0: Y-plane, 1: VU-plane
  for (int i = 0; i < 2; i++) {
    int stride = frame->stVFrame.u32Stride[i];
    int draw_x1 = ((int)x1 >> 2) << 2;
    int draw_x2 = ((int)x2 >> 2) << 2;
    int draw_y1 = ((int)y1 >> 2) << 2;
    int draw_y2 = ((int)y2 >> 2) << 2;
    int draw_thickness_width = rect_thickness;
    int color = 0;
    if (i == 0) {
      color = color_y;
    } else {
      color = ((uint16_t)color_u << 8) | color_v;
    }

    if (i > 0) {
      // vu plane has half size
      draw_x1 /= 2;
      draw_x2 /= 2;
      draw_y1 /= 2;
      draw_y2 /= 2;
      draw_thickness_width /= 2;
    }
    // draw rect vertical line
    for (int h = draw_y1; h < draw_y2; ++h) {
      if (i > 0) {
        int offset = h * stride + draw_x1 * 2;
        if (uint32_t(offset + draw_thickness_width * 2) >= frame->stVFrame.u32Length[i]) {
          printf("draw_rect overflow\n");
        }
        if (draw_x1 * 2 + draw_thickness_width * 2 >= stride) {
          int overflow = stride - (draw_x1 * 2 + draw_thickness_width * 2) + 2;
          offset -= overflow;
        }
        std::fill_n((uint16_t *)(frame->stVFrame.pu8VirAddr[i] + offset), draw_thickness_width,
                    (uint16_t)color);
        // this would not be overflowed
        offset = h * stride + (draw_x2 - draw_thickness_width) * 2;
        std::fill_n((uint16_t *)(frame->stVFrame.pu8VirAddr[i] + offset), draw_thickness_width,
                    (uint16_t)color);
      } else {
        int offset = h * stride + draw_x1;
        std::fill_n((uint8_t *)(frame->stVFrame.pu8VirAddr[i] + offset), draw_thickness_width,
                    (uint8_t)color);
        offset = h * stride + (draw_x2 - draw_thickness_width);
        std::fill_n((uint8_t *)(frame->stVFrame.pu8VirAddr[i] + offset), draw_thickness_width,
                    (uint8_t)color);
      }
    }
    // draw rect horizontal line
    int hstart = draw_y1;
    if (hstart + draw_thickness_width >= height) {
      hstart = height - draw_thickness_width;
    }
    for (int h = hstart; h < hstart + draw_thickness_width; ++h) {
      if (i > 0) {
        int offset = h * stride + draw_x1 * 2;
        int box_width = ((draw_x2 - draw_thickness_width) - draw_x1);
        if (box_width < 0) {
          box_width = 0;
        }
        std::fill_n((uint16_t *)(frame->stVFrame.pu8VirAddr[i] + offset), box_width,
                    (uint16_t)color);
      } else {
        int offset = h * stride + draw_x1;
        int box_width = ((draw_x2 - draw_thickness_width) - draw_x1);
        if (box_width < 0) {
          box_width = 0;
        }
        std::fill_n((uint8_t *)(frame->stVFrame.pu8VirAddr[i] + offset), box_width, (uint8_t)color);
      }
    }

    for (int h = draw_y2 - draw_thickness_width; (h < draw_y2) && (h >= 0); ++h) {
      if (i > 0) {
        int offset = h * stride + draw_x1 * 2;
        int box_width = ((draw_x2 - draw_thickness_width) - draw_x1);
        if (box_width < 0) {
          box_width = 0;
        }
        std::fill_n((uint16_t *)(frame->stVFrame.pu8VirAddr[i] + offset), box_width,
                    (uint16_t)color);
      } else {
        int offset = h * stride + draw_x1;
        int box_width = ((draw_x2 - draw_thickness_width) - draw_x1);
        if (box_width < 0) {
          box_width = 0;
        }
        std::fill_n((uint8_t *)(frame->stVFrame.pu8VirAddr[i] + offset), box_width, (uint8_t)color);
      }
    }

    if (!draw_text) {
      continue;
    }
#ifdef NO_OPENCV
    LOGW("no opencv support,could not draw text");
#else
    cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
    cv::Point cv_point = cv::Point(x1, y1 - 2);
    double font_scale = 2;
    int thickness = rect_thickness;
    if (i != 0) {
      cv_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
      cv_point = cv::Point(x1 / 2, (y1 - 2) / 2);
      font_scale /= 2;
      thickness /= 2;
    }

    if (i == 0) {
      cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[i], frame->stVFrame.u32Stride[i]);
      cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                  cv::Scalar(static_cast<uint8_t>(color)), thickness, 8);
    } else {
      cv::Mat image(cv_size, CV_8UC2, frame->stVFrame.pu8VirAddr[i], frame->stVFrame.u32Stride[i]);
      cv::putText(image, name_str, cv_point, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                  cv::Scalar(static_cast<uint8_t>(color_v), static_cast<uint8_t>(color_u)),
                  thickness, 8);
    }
#endif
  }
}

int DrawPolygon(VIDEO_FRAME_INFO_S *frame, const cvtdl_pts_t *pts, cvtdl_service_brush_t brush) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
    LOGE("Only PIXEL_FORMAT_NV21 and PIXEL_FORMAT_YUV_PLANAR_420 are supported in DrawPolygon\n");
    return CVI_TDL_FAILURE;
  }
#ifdef NO_OPENCV  // TODO:use draw_rect to support
  LOGW("no opencv do not support draw polygon");
  return CVI_TDL_FAILURE;
#else
  std::vector<cv::Point> cv_points;
  for (uint32_t point_index = 0; point_index < pts->size; point_index++) {
    cv_points.push_back(
        {static_cast<int>(pts->x[point_index]), static_cast<int>(pts->y[point_index])});
  }

  color_rgb color;
  color.r = brush.color.r;
  color.g = brush.color.g;
  color.b = brush.color.b;
  uint8_t color_y = static_cast<uint8_t>(GetYuvColor(PLANE_Y, &color));
  uint8_t color_u = static_cast<uint8_t>(GetYuvColor(PLANE_U, &color));
  uint8_t color_v = static_cast<uint8_t>(GetYuvColor(PLANE_V, &color));

  size_t image_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];

  bool do_unmap = false;
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  int thinkness = max(brush.size, 2);

  // Draw Y-plane
  cv::Size cv_size = cv::Size(frame->stVFrame.u32Width, frame->stVFrame.u32Height);
  cv::Mat image(cv_size, CV_8UC1, frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Stride[0]);
  cv::polylines(image, cv_points, true, cv::Scalar(color_y), thinkness);

  // points for V, U planes
  std::vector<cv::Point> cv_vu_points;
  for (uint32_t point_index = 0; point_index < pts->size; point_index++) {
    cv_vu_points.push_back(
        {static_cast<int>(pts->x[point_index] / 2), static_cast<int>(pts->y[point_index] / 2)});
  }

  thinkness /= 2;
  cv::Size cv_vu_size = cv::Size(frame->stVFrame.u32Width / 2, frame->stVFrame.u32Height / 2);
  if (frame->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
    cv::Mat vu_image(cv_vu_size, CV_8UC2, frame->stVFrame.pu8VirAddr[1],
                     frame->stVFrame.u32Stride[1]);
    cv::polylines(vu_image, cv_vu_points, true, cv::Scalar(color_v, color_u), thinkness);
  } else {  // PIXEL_FORMAT_YUV_PLANAR_420
    uint8_t uv_colos[] = {color_u, color_v};
    for (uint32_t uv_index = 1; uv_index < PLANE_NUM; uv_index++) {
      cv::Mat u_image(cv_vu_size, CV_8UC1, frame->stVFrame.pu8VirAddr[uv_index],
                      frame->stVFrame.u32Stride[uv_index]);
      cv::polylines(u_image, cv_vu_points, true, cv::Scalar(uv_colos[uv_index - 1]), thinkness);
    }
  }

  CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0], image_size);
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }

  return CVI_TDL_SUCCESS;
#endif
}

int DrawPts(cvtdl_pts_t *pts, VIDEO_FRAME_INFO_S *drawFrame) {
  color_rgb rgb_color;
  rgb_color.r = DEFAULT_RECT_COLOR_R;
  rgb_color.g = DEFAULT_RECT_COLOR_G;
  rgb_color.b = DEFAULT_RECT_COLOR_B;
  _DrawPts(drawFrame, pts, rgb_color, DEFAULT_RADIUS);
  return CVI_TDL_SUCCESS;
}

int WriteText(char *name, int x, int y, VIDEO_FRAME_INFO_S *drawFrame, float r, float g, float b) {
  color_rgb rgb_color;
  if (r == -1)
    rgb_color.r = DEFAULT_RECT_COLOR_R;
  else
    rgb_color.r = r;
  if (g == -1)
    rgb_color.g = DEFAULT_RECT_COLOR_G;
  else
    rgb_color.g = g;
  if (b == -1)
    rgb_color.b = DEFAULT_RECT_COLOR_B;
  else
    rgb_color.b = b;
  return _WriteText(drawFrame, x, y, name, rgb_color, DEFAULT_TEXT_THICKNESS);
}

template <typename T>
int DrawMeta(const T *meta, VIDEO_FRAME_INFO_S *drawFrame, const bool drawText,
             const std::vector<cvtdl_service_brush_t> &brushes) {
  if (drawFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21 &&
      drawFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
    LOGE("Only PIXEL_FORMAT_NV21 and PIXEL_FORMAT_YUV_PLANAR_420 are supported in DrawMeta\n");
    return CVI_TDL_FAILURE;
  }

  if (meta->size == 0) {
    return CVI_TDL_SUCCESS;
  }

  size_t image_size = drawFrame->stVFrame.u32Length[0] + drawFrame->stVFrame.u32Length[1] +
                      drawFrame->stVFrame.u32Length[2];

  bool do_unmap = false;
  if (drawFrame->stVFrame.pu8VirAddr[0] == NULL) {
    drawFrame->stVFrame.pu8VirAddr[0] =
        (uint8_t *)CVI_SYS_Mmap(drawFrame->stVFrame.u64PhyAddr[0], image_size);
    drawFrame->stVFrame.pu8VirAddr[1] =
        drawFrame->stVFrame.pu8VirAddr[0] + drawFrame->stVFrame.u32Length[0];
    drawFrame->stVFrame.pu8VirAddr[2] =
        drawFrame->stVFrame.pu8VirAddr[1] + drawFrame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  for (size_t i = 0; i < meta->size; i++) {
    cvtdl_service_brush_t brush = brushes[i];
    color_rgb rgb_color;
    rgb_color.r = brush.color.r;
    rgb_color.g = brush.color.g;
    rgb_color.b = brush.color.b;

    int thickness = max(brush.size, 2);
    if ((brush.size % 2) != 0) {
      brush.size += 1;
    }

    cvtdl_bbox_t bbox =
        box_rescale(drawFrame->stVFrame.u32Width, drawFrame->stVFrame.u32Height, meta->width,
                    meta->height, meta->info[i].bbox, meta->rescale_type);
    if (drawFrame->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
      DrawRect<FORMAT_NV21>(drawFrame, bbox.x1, bbox.x2, bbox.y1, bbox.y2, meta->info[i].name,
                            rgb_color, thickness, drawText);
    } else {
      DrawRect<FORMAT_YUV_420P>(drawFrame, bbox.x1, bbox.x2, bbox.y1, bbox.y2, meta->info[i].name,
                                rgb_color, thickness, drawText);
    }
  }

  CVI_SYS_IonFlushCache(drawFrame->stVFrame.u64PhyAddr[0], drawFrame->stVFrame.pu8VirAddr[0],
                        image_size);
  if (do_unmap) {
    CVI_SYS_Munmap((void *)drawFrame->stVFrame.pu8VirAddr[0], image_size);
    drawFrame->stVFrame.pu8VirAddr[0] = NULL;
    drawFrame->stVFrame.pu8VirAddr[1] = NULL;
    drawFrame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return CVI_TDL_SUCCESS;
}

template int DrawMeta<cvtdl_face_t>(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *drawFrame,
                                    const bool drawText,
                                    const std::vector<cvtdl_service_brush_t> &brushes);
template int DrawMeta<cvtdl_object_t>(const cvtdl_object_t *meta, VIDEO_FRAME_INFO_S *drawFrame,
                                      const bool drawText,
                                      const std::vector<cvtdl_service_brush_t> &brushes);
template int DrawMeta<cvtdl_dms_od_t>(const cvtdl_dms_od_t *meta, VIDEO_FRAME_INFO_S *drawFrame,
                                      const bool drawText,
                                      const std::vector<cvtdl_service_brush_t> &brushes);

int DrawPose17(const cvtdl_object_t *obj, VIDEO_FRAME_INFO_S *frame) {
#ifdef NO_OPENCV
  LOGW("no opencv could not draw pose");
  return CVI_TDL_FAILURE;
#else
  frame->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.u32Length[0]);
  cv::Mat img(frame->stVFrame.u32Height, frame->stVFrame.u32Width, CV_8UC3,
              frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Stride[0]);
  if (img.data == nullptr) {
    return CVI_TDL_FAILURE;
  }

  for (uint32_t i = 0; i < obj->size; ++i) {
    std::vector<cv::Point2f> kp_preds(17);
    std::vector<float> kp_scores(17);

    if (!obj->info[i].pedestrian_properity) continue;

    cvtdl_pose17_meta_t pose = obj->info[i].pedestrian_properity->pose_17;
    for (int i = 0; i < 17; ++i) {
      kp_preds[i].x = pose.x[i];
      kp_preds[i].y = pose.y[i];
      kp_scores[i] = pose.score[i];
    }

    cv::Point2f extra_pred;
    extra_pred.x = (kp_preds[5].x + kp_preds[6].x) / 2;
    extra_pred.y = (kp_preds[5].y + kp_preds[6].y) / 2;
    kp_preds.push_back(extra_pred);

    float extra_score = (kp_scores[5] + kp_scores[6]) / 2;
    kp_scores.push_back(extra_score);

    // Draw keypoints
    std::unordered_map<int, std::pair<int, int>> part_line;
    for (uint32_t n = 0; n < kp_scores.size(); n++) {
      if (kp_scores[n] <= 0.35) continue;

      int cor_x = kp_preds[n].x;
      int cor_y = kp_preds[n].y;
      part_line[n] = std::make_pair(cor_x, cor_y);

      cv::Mat bg;
      img.copyTo(bg);
      cv::circle(bg, cv::Size(cor_x, cor_y), 2, p_color[n], -1);
      float transparency = max(float(0.0), min(float(1.0), kp_scores[n]));
      cv::addWeighted(bg, transparency, img, 1 - transparency, 0, img);
    }

    // Draw limbs
    for (uint32_t i = 0; i < l_pair.size(); i++) {
      int start_p = l_pair[i].first;
      int end_p = l_pair[i].second;
      if (part_line.count(start_p) > 0 && part_line.count(end_p) > 0) {
        std::pair<int, int> start_xy = part_line[start_p];
        std::pair<int, int> end_xy = part_line[end_p];

        float mX = (start_xy.first + end_xy.first) / 2;
        float mY = (start_xy.second + end_xy.second) / 2;
        float length = sqrt(pow((start_xy.second - end_xy.second), 2) +
                            pow((start_xy.first - end_xy.first), 2));
        float angle =
            atan2(start_xy.second - end_xy.second, start_xy.first - end_xy.first) * 180.0 / M_PI;
        float stickwidth = (kp_scores[start_p] + kp_scores[end_p]) + 1;
        std::vector<cv::Point> polygon;
        cv::ellipse2Poly(cv::Point(int(mX), int(mY)), cv::Size(int(length / 2), stickwidth),
                         int(angle), 0, 360, 1, polygon);

        cv::Mat bg;
        img.copyTo(bg);
        cv::fillConvexPoly(bg, polygon, line_color[i]);
        float transparency =
            max(float(0.0), min(float(1.0), float(0.5) * (kp_scores[start_p] + kp_scores[end_p])));
        cv::addWeighted(bg, transparency, img, 1 - transparency, 0, img);
      }
    }
  }

  CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0],
                        frame->stVFrame.u32Length[0]);
  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
  frame->stVFrame.pu8VirAddr[0] = NULL;

  // frame->stVFrame.pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0],
  //                                                             frame->stVFrame.u32Length[0]);
  // cv::Mat draw_img(frame->stVFrame.u32Height, frame->stVFrame.u32Width, CV_8UC3,
  //             frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Stride[0]);
  // cv::cvtColor(draw_img, draw_img, CV_RGB2BGR);
  // cv::imwrite("/mnt/data/out2.jpg", draw_img);

  return CVI_TDL_SUCCESS;
#endif
}

int Draw5Landmark(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame) {
  static const color_rgb LANDMARK5_COLORS[5] = {COLOR_RED, COLOR_GREEN, COLOR_MAGENTA, COLOR_YELLOW,
                                                COLOR_CYAN};
  for (uint32_t i = 0; i < meta->size; i++) {
    for (int j = 0; j < 5; j++) {
      cvtdl_pts_t tmp_pts = {0};
      tmp_pts.size = 1;
      tmp_pts.x = &meta->info[i].pts.x[j];
      tmp_pts.y = &meta->info[i].pts.y[j];
      _DrawPts(frame, &tmp_pts, LANDMARK5_COLORS[j], 3);
    }
  }
  return CVI_SUCCESS;
}

int DrawHandPose21(const cvtdl_handpose21_meta_ts *obj_meta, VIDEO_FRAME_INFO_S *bg) {
#ifdef NO_OPENCV
  LOGW("no opencv could not draw pose");
  return CVI_TDL_FAILURE;
#else
  static const color_rgb LANDMARK21_COLORS[21] = {
      COLOR_RED, COLOR_GREEN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_CYAN, COLOR_RED,
      COLOR_RED, COLOR_GREEN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_CYAN, COLOR_RED,
      COLOR_RED, COLOR_GREEN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_CYAN, COLOR_RED,
      COLOR_RED, COLOR_GREEN, COLOR_MAGENTA};
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    for (int j = 0; j < 21; j++) {
      cvtdl_pts_t tmp_pts = {0};
      tmp_pts.size = 1;
      tmp_pts.x = &obj_meta->info[i].x[j];
      tmp_pts.y = &obj_meta->info[i].y[j];
      _DrawPts(bg, &tmp_pts, LANDMARK21_COLORS[j], 3);
    }
  }
  return CVI_TDL_SUCCESS;
#endif
}

}  // namespace service
}  // namespace cvitdl
