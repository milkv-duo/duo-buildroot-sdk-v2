#ifndef _POSE_UTILS_H_
#define _POSE_UTILS_H_
#include <vector>
#include <opencv2/opencv.hpp>

#define POSE_PTS_NUM 17

struct bbox_t {
  bbox_t(float _x1 = 0, float _y1 = 0, float _x2 = 0, float _y2 = 0)
      : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}

  float x1;
  float y1;
  float x2;
  float y2;
};

struct pose_t {
  float x[POSE_PTS_NUM];
  float y[POSE_PTS_NUM];
  float score[POSE_PTS_NUM];
};

cv::Mat get_affine_transform(const std::vector<float> &center,
                             const std::vector<float> &scale,
                             const std::vector<float> &output_size, bool inv = false);

bbox_t center_scale_to_box(const std::vector<float> &center,
                           const std::vector<float> &scale);

void box_to_center_scale(float x, float y, float w, float h, float aspect_ratio,
                         std::vector<float> &scale, std::vector<float> &center);

void simple_postprocess(const std::vector<cv::Mat> &pose_pred_list,
                        const std::vector<bbox_t> &align_bbox_list,
                        std::vector<pose_t> &dst_pose_list);
cv::Mat draw_pose(cv::Mat &image, std::vector<pose_t> &pose_list);

#endif // _POSE_UTILS_H_
