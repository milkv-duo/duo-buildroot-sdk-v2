#include "cvi_kit.h"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
#include "cvi_tdl_log.hpp"
#include "utils/token.hpp"
#include "utils/clip_postprocess.hpp"
#ifndef NO_OPENCV

using namespace cvitdl;
CVI_S32 CVI_TDL_ShowLanePoints(VIDEO_FRAME_INFO_S *bg, cvtdl_lane_t *lane_meta, char *save_path) {
  std::vector<cv::Scalar> color = {cv::Scalar(51, 153, 255), cv::Scalar(0, 153, 76),
                                   cv::Scalar(255, 215, 0), cv::Scalar(255, 128, 0),
                                   cv::Scalar(0, 255, 0)};
  bg->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);

  cv::Mat img_rgb(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC3,
                  bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Stride[0]);

  int det_num = lane_meta->size;

  for (int i = 0; i < det_num; i++) {
    int x0 = (int)lane_meta->lane[i].x[0];
    int y0 = (int)lane_meta->lane[i].y[0];
    int x1 = (int)lane_meta->lane[i].x[1];
    int y1 = (int)lane_meta->lane[i].y[1];

    cv::line(img_rgb, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(0, 255, 0), 3);
  }

  cv::imwrite(save_path, img_rgb);
  CVI_SYS_Munmap((void *)bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Length[0]);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_ShowKeypoints(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta,
                                         float score, char *save_path) {
  std::vector<cv::Scalar> color = {cv::Scalar(51, 153, 255), cv::Scalar(0, 153, 76),
                                   cv::Scalar(255, 215, 0), cv::Scalar(255, 128, 0),
                                   cv::Scalar(0, 255, 0)};

  int color_map[17] = {0, 0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 4, 3, 4, 3, 4, 3};
  int line_map[19] = {4, 4, 3, 3, 0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0};
  int skeleton[19][2] = {{15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
                         {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
                         {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6}};
  bg->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);

  cv::Mat img_rgb(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC3,
                  bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Stride[0]);

  for (uint32_t i = 0; i < obj_meta->size; i++) {
    for (uint32_t j = 0; j < 17; j++) {
      if (obj_meta->info[i].pedestrian_properity->pose_17.score[i] < score) {
        continue;
      }
      int x = (int)obj_meta->info[i].pedestrian_properity->pose_17.x[j];
      int y = (int)obj_meta->info[i].pedestrian_properity->pose_17.y[j];
      cv::circle(img_rgb, cv::Point(x, y), 7, color[color_map[j]], -1);
    }

    for (uint32_t k = 0; k < 19; k++) {
      int kps1 = skeleton[k][0];
      int kps2 = skeleton[k][1];
      if (obj_meta->info[i].pedestrian_properity->pose_17.score[kps1] < score ||
          obj_meta->info[i].pedestrian_properity->pose_17.score[kps2] < score)
        continue;

      int x1 = (int)obj_meta->info[i].pedestrian_properity->pose_17.x[kps1];
      int y1 = (int)obj_meta->info[i].pedestrian_properity->pose_17.y[kps1];

      int x2 = (int)obj_meta->info[i].pedestrian_properity->pose_17.x[kps2];
      int y2 = (int)obj_meta->info[i].pedestrian_properity->pose_17.y[kps2];

      cv::line(img_rgb, cv::Point(x1, y1), cv::Point(x2, y2), color[line_map[k]], 2);
    }
  }

  cv::imwrite(save_path, img_rgb);
  CVI_SYS_Munmap((void *)bg->stVFrame.pu8VirAddr[0], bg->stVFrame.u32Length[0]);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_SavePicture(VIDEO_FRAME_INFO_S *bg, char *save_path) {
  CVI_U8 *r_plane = (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);
  CVI_U8 *g_plane = (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[1], bg->stVFrame.u32Length[1]);
  CVI_U8 *b_plane = (CVI_U8 *)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[2], bg->stVFrame.u32Length[2]);

  cv::Mat r_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, r_plane);
  cv::Mat g_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, g_plane);
  cv::Mat b_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, b_plane);

  std::vector<cv::Mat> channels = {r_mat, g_mat, b_mat};
  cv::Mat img_rgb;
  cv::merge(channels, img_rgb);

  cv::imwrite(save_path, img_rgb);

  CVI_SYS_Munmap(r_plane, bg->stVFrame.u32Length[0]);
  CVI_SYS_Munmap(g_plane, bg->stVFrame.u32Length[1]);
  CVI_SYS_Munmap(b_plane, bg->stVFrame.u32Length[2]);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_Cal_Similarity(cvtdl_feature_t feature, cvtdl_feature_t feature1,
                                          float *similarity) {
  cv::Mat mat_feature(feature.size, 1, CV_8SC1);
  cv::Mat mat_feature1(feature1.size, 1, CV_8SC1);
  memcpy(mat_feature.data, feature.ptr, feature.size);
  memcpy(mat_feature1.data, feature1.ptr, feature1.size);
  mat_feature.convertTo(mat_feature, CV_32FC1, 1.);
  mat_feature1.convertTo(mat_feature1, CV_32FC1, 1.);
  *similarity = mat_feature.dot(mat_feature1) / (cv::norm(mat_feature) * cv::norm(mat_feature1));
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_Set_MaskOutlinePoint(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta) {
  int proto_h = obj_meta->mask_height;
  int proto_w = obj_meta->mask_width;
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    cv::Mat src(proto_h, proto_w, CV_8UC1, obj_meta->info[i].mask_properity->mask,
                proto_w * sizeof(uint8_t));

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // search for contours
    cv::findContours(src, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    // find the longest contour
    int longest_index = -1;
    size_t max_length = 0;
    for (size_t i = 0; i < contours.size(); i++) {
      if (contours[i].size() > max_length) {
        max_length = contours[i].size();
        longest_index = i;
      }
    }
    if (longest_index >= 0 && max_length >= 1) {
      float ratio_height = (proto_h / static_cast<float>(frame->stVFrame.u32Height));
      float ratio_width = (proto_w / static_cast<float>(frame->stVFrame.u32Width));
      int source_y_offset, source_x_offset;
      if (ratio_height > ratio_width) {
        source_x_offset = 0;
        source_y_offset = (proto_h - frame->stVFrame.u32Height * ratio_width) / 2;
      } else {
        source_x_offset = (proto_w - frame->stVFrame.u32Width * ratio_height) / 2;
        source_y_offset = 0;
      }
      int source_region_height = proto_h - 2 * source_y_offset;
      int source_region_width = proto_w - 2 * source_x_offset;
      // calculate scaling factor
      float height_scale =
          static_cast<float>(frame->stVFrame.u32Height) / static_cast<float>(source_region_height);
      float width_scale =
          static_cast<float>(frame->stVFrame.u32Width) / static_cast<float>(source_region_width);
      obj_meta->info[i].mask_properity->mask_point_size = max_length;
      obj_meta->info[i].mask_properity->mask_point =
          (float *)malloc(2 * max_length * sizeof(float));

      size_t j = 0;
      for (const auto &point : contours[longest_index]) {
        obj_meta->info[i].mask_properity->mask_point[2 * j] =
            (point.x - source_x_offset) * width_scale;
        obj_meta->info[i].mask_properity->mask_point[2 * j + 1] =
            (point.y - source_y_offset) * height_scale;
        j++;
      }
    }
  }
  return CVI_SUCCESS;
}
#endif

CVI_S32 CVI_TDL_Set_ClipPostprocess(float **text_features, int text_features_num,
                                    float **image_features, int image_features_num, float **probs) {
  Eigen::MatrixXf text_features_eigen(text_features_num, 512);
  for (int i = 0; i < text_features_num; ++i) {
    for (int j = 0; j < 512; ++j) {
      text_features_eigen(i, j) = text_features[i][j];
    }
  }
  Eigen::MatrixXf image_features_eigen(image_features_num, 512);
  for (int i = 0; i < image_features_num; ++i) {
    for (int j = 0; j < 512; ++j) {
      image_features_eigen(i, j) = image_features[i][j];
    }
  }

  Eigen::MatrixXf result_eigen;
  // using clip_postprocess which can be found in utils/clip_postpostprocess.cpp
  int res = cvitdl::clip_postprocess(text_features_eigen, image_features_eigen, result_eigen);

  // providing image classification functionality.
  // using softmax after mutil 100 scale
  for (int i = 0; i < result_eigen.rows(); ++i) {
    float sum = 0.0;
    float maxVal = result_eigen.row(i).maxCoeff();
    Eigen::MatrixXf expInput = (result_eigen.row(i).array() - maxVal).exp();
    sum = expInput.sum();
    result_eigen.row(i) = expInput / sum;
  }

  for (int i = 0; i < result_eigen.rows(); i++) {
    float max_score = 0;
    for (int j = 0; j < result_eigen.cols(); j++) {
      probs[i][j] = result_eigen(i, j);
    }
  }

  if (res == 0) {
    return CVI_SUCCESS;
  }

  LOGE("Tokenization error\n");
  return CVI_FAILURE;
}

CVI_S32 CVI_TDL_Set_TextPreprocess(const char *encoderFile, const char *bpeFile,
                                   const char *textFile, int32_t **tokens, int numSentences) {
  std::vector<std::vector<int32_t>> tokens_cpp(numSentences);
  // call token_bpe function
  int result =
      cvitdl::token_bpe(std::string(encoderFile), std::string(bpeFile), std::string(textFile), tokens_cpp);
  // calculate the total number of elements
  for (int i = 0; i < numSentences; i++) {
    // tokens[i] = new int32_t[tokens_cpp[i].size()];
    tokens[i] = (int32_t *)malloc(tokens_cpp[i].size() * sizeof(int32_t));
    memcpy(tokens[i], tokens_cpp[i].data(), tokens_cpp[i].size() * sizeof(int32_t));
  }

  if (result == 0) {
    return CVI_SUCCESS;
  }
  LOGE("Tokenization error\n");
  return CVI_FAILURE;
}