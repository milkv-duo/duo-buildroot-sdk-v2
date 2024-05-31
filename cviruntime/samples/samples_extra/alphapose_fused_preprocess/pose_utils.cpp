#include "math.h"
#include "stdio.h"
#include "assert.h"
#include <unordered_map>
#include "pose_utils.h"

using namespace std;
using namespace cv;

static std::vector<pair<int, int>> l_pair = {{0, 1},   {0, 2},   {1, 3},   {2, 4},   {5, 6},
                                        {5, 7},   {7, 9},   {6, 8},   {8, 10},  {17, 11},
                                        {17, 12}, {11, 13}, {12, 14}, {13, 15}, {14, 16}};

static std::vector<cv::Scalar> p_color = {
    {0, 255, 255},  {0, 191, 255},  {0, 255, 102},  {0, 77, 255},   {0, 255, 0},
    {77, 255, 255}, {77, 255, 204}, {77, 204, 255}, {191, 255, 77}, {77, 191, 255},
    {191, 255, 77}, {204, 77, 255}, {77, 255, 204}, {191, 77, 255}, {77, 255, 191},
    {127, 77, 255}, {77, 255, 127}, {0, 255, 255}};

static std::vector<cv::Scalar> line_color = {
    {0, 215, 255},   {0, 255, 204},  {0, 134, 255},  {0, 255, 50},  {77, 255, 222},
    {77, 196, 255},  {77, 135, 255}, {191, 255, 77}, {77, 255, 77}, {77, 222, 255},
    {255, 156, 127}, {0, 127, 255},  {255, 127, 77}, {0, 77, 255},  {255, 77, 36}};

std::vector<bbox_t> yolo_detect() {
  std::vector<bbox_t> yolo_bbox_list(5);
  yolo_bbox_list[0] = bbox_t(137.53755547, 114.67451232, 78.44751164, 182.63019933);
  yolo_bbox_list[1] = bbox_t(49.44112301, 134.16529877, 73.67089391, 159.10105168);
  yolo_bbox_list[2] = bbox_t(464.84487668, 77.33579667, 98.59243602, 229.26176143);
  yolo_bbox_list[3] = bbox_t(353.29705574, 90.37665582, 79.01046082, 211.99965902);
  yolo_bbox_list[4] = bbox_t(228.44323056, 108.68967827, 107.76577916, 190.25104091);
  // x, y, w, h to x1, y1, x2, y2
  for (int i = 0; i < 5; ++i) {
    yolo_bbox_list[i].x2 += yolo_bbox_list[i].x1;
    yolo_bbox_list[i].y2 += yolo_bbox_list[i].y1;
  }

  return std::move(yolo_bbox_list);
}

cv::Point2f get_3rd_point(cv::Point2f a, cv::Point2f b) {
  Point2f direct;
  direct.x = b.x - (a - b).y;
  direct.y = b.y + (a - b).x;

  return direct;
}

std::vector<float> get_dir(float src_w) {

  float sn = sin(0);
  float cs = cos(0);

  std::vector<float> src_result(2, 0);
  src_result[0] = -src_w * sn;
  src_result[1] = src_w * cs;

  return src_result;
}

cv::Mat get_affine_transform(const std::vector<float> &center, const std::vector<float> &scale,
                             const std::vector<float> &output_size, bool inv) {
  std::vector<float> shift(2, 0);
  float src_w = scale[0];
  int dst_h = output_size[0];
  int dst_w = output_size[1];

  std::vector<float> src_dir = get_dir(src_w * -0.5);
  std::vector<float> dst_dir(2, 0);
  dst_dir[1] = dst_w * -0.5;

  cv::Point2f src[3];
  cv::Point2f dst[3];

  src[0] = Point2f(center[0], center[1]);
  src[1] = Point2f(center[0] + src_dir[0], center[1] + src_dir[1]);
  src[2] = get_3rd_point(src[0], src[1]);
  dst[0] = Point2f(dst_w * 0.5, dst_h * 0.5);
  dst[1] = Point2f(dst_w * 0.5 + dst_dir[0], dst_h * 0.5 + dst_dir[1]);
  dst[2] = get_3rd_point(dst[0], dst[1]);

  if (inv)
    return cv::getAffineTransform(dst, src);
  else
    return cv::getAffineTransform(src, dst);
}

bbox_t center_scale_to_box(const std::vector<float> &center, const std::vector<float> &scale) {
  float w = scale[0] * 1.0;
  float h = scale[1] * 1.0;
  bbox_t bbox;

  bbox.x1 = center[0] - w * 0.5;
  bbox.y1 = center[1] - h * 0.5;
  bbox.x2 = bbox.x1 + w;
  bbox.y2 = bbox.y1 + h;

  return bbox;
}

void box_to_center_scale(float x, float y, float w, float h, float aspect_ratio,
                         std::vector<float> &scale, std::vector<float> &center) {
  float pixel_std = 1;
  float scale_mult = 1.25;

  center[0] = x + w * 0.5;
  center[1] = y + h * 0.5;

  if (w > aspect_ratio * h) {
    h = w / aspect_ratio;
  } else if (w < aspect_ratio * h) {
    w = h * aspect_ratio;
  }

  scale[0] = w * 1.0 / pixel_std;
  scale[1] = h * 1.0 / pixel_std;
  if (center[0] != -1) {
    scale[0] = scale[0] * scale_mult;
    scale[1] = scale[1] * scale_mult;
  }
}

void get_max_pred(const Mat &pose_pred, pose_t &dst_pose) {
  int inner_size = pose_pred.size[2] * pose_pred.size[3];
  float *ptr = (float *)pose_pred.data;
  for (int c = 0; c < POSE_PTS_NUM; ++c) {
    dst_pose.score[c] = 0;
    dst_pose.x[c] = 0;
    dst_pose.y[c] = 0;
    // for (int h = 0; h < pose_pred.size[2]; ++h) {
    //    for (int w = 0; w < pose_pred.size[3]; ++w) {
    //        float current_score = blob_to_val(pose_pred, 0, c, h, w);
    //        if (current_score > dst_pose.score[c]) {
    //            dst_pose.score[c] = current_score;
    //            dst_pose.x[c] = w;
    //            dst_pose.y[c] = h;
    //        }
    //    }
    //}
    int max_idx = 0;
    for (int i = 0; i < inner_size; ++i) {
      if (ptr[i] > dst_pose.score[c]) {
        dst_pose.score[c] = ptr[i];
        max_idx = i;
      }
    }
    dst_pose.x[c] = max_idx % pose_pred.size[3];
    dst_pose.y[c] = max_idx / pose_pred.size[3];
    ptr += inner_size;
  }
}

void simple_postprocess(const std::vector<Mat> &pose_pred_list,
                        const std::vector<bbox_t> &align_bbox_list,
                        std::vector<pose_t> &dst_pose_list) {
  for (int i = 0; i < pose_pred_list.size(); ++i) {
    float x = align_bbox_list[i].x1;
    float y = align_bbox_list[i].y1;
    float w = align_bbox_list[i].x2 - align_bbox_list[i].x1;
    float h = align_bbox_list[i].y2 - align_bbox_list[i].y1;
    std::vector<float> center = {(float)(x + w * 0.5), (float)(y + h * 0.5)};
    std::vector<float> scale = {w, h};

    get_max_pred(pose_pred_list[i], dst_pose_list[i]);
    cv::Mat trans = get_affine_transform(
        center, scale,
        {(float)pose_pred_list[i].size[2], (float)pose_pred_list[i].size[3]}, true);
    for (int c = 0; c < POSE_PTS_NUM; ++c) {
      dst_pose_list[i].x[c] = trans.at<double>(0) * dst_pose_list[i].x[c] +
                              trans.at<double>(1) * dst_pose_list[i].y[c] +
                              trans.at<double>(2);
      dst_pose_list[i].y[c] = trans.at<double>(3) * dst_pose_list[i].x[c] +
                              trans.at<double>(4) * dst_pose_list[i].y[c] +
                              trans.at<double>(5);
    }
  }
}

Mat draw_pose(cv::Mat &image, std::vector<pose_t> &pose_list) {
  int height = image.rows;
  int width = image.cols;

  cv::Mat img = image.clone();

  for (pose_t pose : pose_list) {
    std::vector<Point2f> kp_preds(POSE_PTS_NUM);
    std::vector<float> kp_scores(POSE_PTS_NUM);

    for (int i = 0; i < POSE_PTS_NUM; ++i) {
      kp_preds[i].x = pose.x[i];
      kp_preds[i].y = pose.y[i];
      kp_scores[i] = pose.score[i];
    }

    Point2f extra_pred;
    extra_pred.x = (kp_preds[5].x + kp_preds[6].x) / 2;
    extra_pred.y = (kp_preds[5].y + kp_preds[6].y) / 2;
    kp_preds.push_back(extra_pred);

    float extra_score = (kp_scores[5] + kp_scores[6]) / 2;
    kp_scores.push_back(extra_score);

    // Draw keypoints
    unordered_map<int, pair<int, int>> part_line;
    for (int n = 0; n < kp_scores.size(); n++) {
      if (kp_scores[n] <= 0.35)
        continue;

      int cor_x = kp_preds[n].x;
      int cor_y = kp_preds[n].y;
      part_line[n] = make_pair(cor_x, cor_y);

      cv::Mat bg;
      img.copyTo(bg);
      cv::circle(bg, cv::Size(cor_x, cor_y), 2, p_color[n], -1);
      float transparency = max(float(0.0), min(float(1.0), kp_scores[n]));
      cv::addWeighted(bg, transparency, img, 1 - transparency, 0, img);
    }

    // Draw limbs
    for (int i = 0; i < l_pair.size(); i++) {
      int start_p = l_pair[i].first;
      int end_p = l_pair[i].second;
      if (part_line.count(start_p) > 0 && part_line.count(end_p) > 0) {
        pair<int, int> start_xy = part_line[start_p];
        pair<int, int> end_xy = part_line[end_p];

        float mX = (start_xy.first + end_xy.first) / 2;
        float mY = (start_xy.second + end_xy.second) / 2;
        float length = sqrt(pow((start_xy.second - end_xy.second), 2) +
                            pow((start_xy.first - end_xy.first), 2));
        float angle =
            (atan2(start_xy.second - end_xy.second, start_xy.first - end_xy.first)) *
            180.0 / M_PI;
        float stickwidth = (kp_scores[start_p] + kp_scores[end_p]) + 1;
        std::vector<cv::Point> polygon;
        cv::ellipse2Poly(cv::Point(int(mX), int(mY)),
                         cv::Size(int(length / 2), stickwidth), int(angle), 0, 360, 1,
                         polygon);

        cv::Mat bg;
        img.copyTo(bg);
        cv::fillConvexPoly(bg, polygon, line_color[i]);
        float transparency =
            max(float(0.0),
                min(float(1.0), float(0.5) * (kp_scores[start_p] + kp_scores[end_p])));
        cv::addWeighted(bg, transparency, img, 1 - transparency, 0, img);
      }
    }
  }
  return img;
}
