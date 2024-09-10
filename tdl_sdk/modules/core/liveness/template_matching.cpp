#include "template_matching.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opencv2/imgproc.hpp"

#define RESIZE_SIZE 112
#define IMAGE_SIZE 32
#define CROP_RGB_SIZE 50.0

using namespace std;

vector<cv::Mat> TTA_9_cropps(cv::Mat image) {
  cv::resize(image, image, cv::Size(RESIZE_SIZE, RESIZE_SIZE));

  int width = image.cols;
  int height = image.rows;
  int target_w = IMAGE_SIZE;
  int target_h = IMAGE_SIZE;

  int start_x = (width - target_w) / 2;
  int start_y = (height - target_h) / 2;

  vector<vector<int> > starts;
  starts.push_back(vector<int>{start_x, start_y});
  starts.push_back(vector<int>{start_x - target_w, start_y});
  starts.push_back(vector<int>{start_x, start_y - target_w});
  starts.push_back(vector<int>{start_x + target_w, start_y});
  starts.push_back(vector<int>{start_x, start_y + target_w});
  starts.push_back(vector<int>{start_x + target_w, start_y + target_w});
  starts.push_back(vector<int>{start_x - target_w, start_y - target_w});
  starts.push_back(vector<int>{start_x - target_w, start_y + target_w});
  starts.push_back(vector<int>{start_x + target_w, start_y - target_w});

  vector<cv::Mat> images;
  for (size_t start_index = 0; start_index < starts.size(); start_index++) {
    cv::Mat image_;
    image.copyTo(image_);

    int x = starts[start_index][0];
    int y = starts[start_index][1];

    if (x < 0) x = 0;
    if (y < 0) y = 0;

    if (x + target_w >= RESIZE_SIZE) {
      x = RESIZE_SIZE - target_w - 1;
    }
    if (y + target_h >= RESIZE_SIZE) y = RESIZE_SIZE - target_h - 1;

    cv::Rect crop_rect(x, y, target_w, target_h);
    cv::Mat zeros = image_(crop_rect);

    zeros.copyTo(image_);
    images.push_back(image_);
  }

  return images;
}

#ifdef ENABLE_TEMPLATE_MATCHING
cv::Mat template_matching(const cv::Mat &crop_rgb_frame, const cv::Mat &ir_frame, cv::Rect box,
                          cvtdl_liveness_ir_position_e ir_pos) {
  int adj_width_start = 0;
  int adj_width_end = 0;
  int adj_height_start = max(0, box.y);
  int adj_height_end = min(ir_frame.rows, box.y + box.height + int(0.5 * box.height));

  if (ir_pos == LIVENESS_IR_RIGHT) {
    adj_width_start = max(0, box.x);
    adj_width_end = min(ir_frame.cols, box.x + box.width + int(1.5 * box.width));
  } else {
    adj_width_start = max(0, box.x - int(1.5 * box.width));
    adj_width_end = min(ir_frame.cols, box.x + box.width);
  }

  cv::Rect ir_crop(adj_width_start, adj_height_start, adj_width_end - adj_width_start,
                   adj_height_end - adj_height_start);
  cv::Mat ir_frame_small = ir_frame(ir_crop);

  int template_h = crop_rgb_frame.rows;
  int template_w = crop_rgb_frame.cols;

  double scale_h = (template_h / CROP_RGB_SIZE);
  double scale_w = (template_w / CROP_RGB_SIZE);
  cv::Mat crop_rgb_frame_scaled;
  cv::Mat ir_frame_small_scaled;
  cv::resize(crop_rgb_frame, crop_rgb_frame_scaled,
             cv::Size(int(CROP_RGB_SIZE), int(CROP_RGB_SIZE)));
  cv::resize(ir_frame_small, ir_frame_small_scaled,
             cv::Size(int(round(ir_frame_small.cols / scale_w)),
                      int(round(ir_frame_small.rows / scale_h))));

  cv::Mat crop_rgb_frame_gray, ir_frame_small_gray;
  cv::cvtColor(crop_rgb_frame_scaled, crop_rgb_frame_gray, cv::COLOR_BGR2GRAY);
  cv::cvtColor(ir_frame_small_scaled, ir_frame_small_gray, cv::COLOR_BGR2GRAY);

  cv::Mat crop_rgb_frame_gray_x, crop_rgb_frame_gray_y;
  cv::Sobel(crop_rgb_frame_gray, crop_rgb_frame_gray_x, CV_16S, 1, 0);
  cv::Sobel(crop_rgb_frame_gray, crop_rgb_frame_gray_y, CV_16S, 0, 1);

  cv::Mat absX, absY;
  cv::convertScaleAbs(crop_rgb_frame_gray_x, absX);
  cv::convertScaleAbs(crop_rgb_frame_gray_y, absY);
  cv::addWeighted(absX, 1.0, absY, 1.0, 0, crop_rgb_frame_gray);

  cv::Mat ir_frame_small_gray_x, ir_frame_small_gray_y;
  cv::Sobel(ir_frame_small_gray, ir_frame_small_gray_x, CV_16S, 1, 0);
  cv::Sobel(ir_frame_small_gray, ir_frame_small_gray_y, CV_16S, 0, 1);
  cv::convertScaleAbs(ir_frame_small_gray_x, absX);
  cv::convertScaleAbs(ir_frame_small_gray_y, absY);
  cv::addWeighted(absX, 1.0, absY, 1.0, 0, ir_frame_small_gray);

  cv::Mat res;
  cv::matchTemplate(ir_frame_small_gray, crop_rgb_frame_gray, res, cv::TM_CCOEFF_NORMED);

  double min_val, max_val;
  cv::Point min_loc, max_loc;
  cv::minMaxLoc(res, &min_val, &max_val, &min_loc, &max_loc);

  cv::Mat crop_ir_frame;
  if (max_val < 0) {
    crop_ir_frame = ir_frame(box);
  } else {
    template_h = min(ir_frame_small.rows - int(max_loc.y * scale_h) - 1, template_h);
    template_w = min(ir_frame_small.cols - int(max_loc.x * scale_w) - 1, template_w);
    cv::Rect ir_crop(int(max_loc.x * scale_w), int(max_loc.y * scale_h), template_w, template_h);
    crop_ir_frame = ir_frame_small(ir_crop);
  }

  return crop_ir_frame;
}
#endif