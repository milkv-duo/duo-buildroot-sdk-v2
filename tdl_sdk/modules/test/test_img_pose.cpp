#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

std::vector<cv::Scalar> color = {cv::Scalar(51, 153, 255), cv::Scalar(0, 153, 76),
                                 cv::Scalar(255, 215, 0), cv::Scalar(255, 128, 0),
                                 cv::Scalar(0, 255, 0)};

int color_map[17] = {0, 0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 4, 3, 4, 3, 4, 3};
int line_map[19] = {4, 4, 3, 3, 0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0};
int skeleton[19][2] = {{15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
                       {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
                       {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6}};

void show_keypoints(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta, std::string save_path,
                    float score) {
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
}

int process_img_simcc(cvitdl_handle_t tdl_handle, std::string &pd_model, std::string &pose_model,
                      VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *p_obj) {
  int ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                              pd_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, pose_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE model failed with %#x!\n", ret);
    return ret;
  }

  //   CVI_TDL_SetMaxDetNum(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, 1);

  ret = CVI_TDL_Detection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_MOBILEDETV2_PEDESTRIAN failed with %#x!\n", ret);
    return ret;
  }

  if (p_obj->size > 0) {
    ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_Simcc_Pose failed with %#x!\n", ret);
      return ret;
    }
  } else {
    printf("cannot find person\n");
  }

  return ret;
}

int process_img_yolov8pose(cvitdl_handle_t tdl_handle, std::string &pose_model,
                           VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *p_obj) {
  int ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, pose_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Yolov8_Pose failed with %#x!\n", ret);
    return ret;
  }

  return ret;
}

int main(int argc, char *argv[]) {
  std::string pose_model(argv[1]);    // pose detection model path
  std::string process_flag(argv[2]);  // simcc | yolov8pose
  std::string imgf(argv[3]);          // img path;
  int show = atoi(argv[4]);           // 1 for show keypoints, 0 for not show;

  std::string pd_model;  // person detection model path,
  if (process_flag == "simcc") {
    pd_model = argv[5];
  }

  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  PIXEL_FORMAT_E img_format = PIXEL_FORMAT_RGB_888_PLANAR;
  if (show) img_format = PIXEL_FORMAT_BGR_888;

  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  ret = CVI_TDL_ReadImage(img_handle, imgf.c_str(), &bg, img_format);
  if (ret != CVI_SUCCESS) {
    std::cout << "failed to open file:" << imgf << std::endl;
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }

  cvtdl_object_t obj_meta = {0};

  if (process_flag == "simcc") {
    process_img_simcc(tdl_handle, pd_model, pose_model, &bg, &obj_meta);
  } else {
    process_img_yolov8pose(tdl_handle, pose_model, &bg, &obj_meta);
  }

  printf("obj_meta.size:%d\n", obj_meta.size);
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    std::cout << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
              << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << "]," << std::endl;

    for (int j = 0; j < 17; j++) {
      std::cout << j << ": " << obj_meta.info[i].pedestrian_properity->pose_17.x[j] << " "
                << obj_meta.info[i].pedestrian_properity->pose_17.y[j] << " "
                << obj_meta.info[i].pedestrian_properity->pose_17.score[j] << std::endl;
    }
  }

  if (show) {  // img format should be PIXEL_FORMAT_BGR_888
    float score;
    CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, &score);
    std::string save_path = "/mnt/data/xiaochuan.liao/sdk_package/cviai/test_simcc.jpg";
    show_keypoints(&bg, &obj_meta, save_path, score);
    // cvitdl::service::DrawKeypoints(&bg, &obj_meta, save_path, score );
    // cvitdl::service::DrawPose17(&obj_meta, &bg);
  }

  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
