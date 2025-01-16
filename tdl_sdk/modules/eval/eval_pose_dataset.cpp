#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "core/core/cvtdl_core_types.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.hpp"

std::vector<cv::Scalar> color = {cv::Scalar(51, 153, 255), cv::Scalar(0, 153, 76),
                                 cv::Scalar(255, 215, 0), cv::Scalar(255, 128, 0),
                                 cv::Scalar(0, 255, 0)};

int color_map[17] = {0, 0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 4, 3, 4, 3, 4, 3};
int line_map[19] = {4, 4, 3, 3, 0, 0, 0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0};
int skeleton[19][2] = {{15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
                       {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
                       {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6}};

void show_keypoints(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta, string save_path,
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

std::string process_simcc_pose(cvitdl_handle_t tdl_handle, std::string &pd_model_path,
                               std::string &pose_model_path, cvtdl_object_t *p_obj,
                               VIDEO_FRAME_INFO_S *bg, std::vector<std::vector<int>> &boxes,
                               std::string &file_name) {
  static int model_init = 0;
  std::string no_model("None");
  CVI_S32 ret;
  if (model_init == 0) {
    if (pd_model_path != no_model) {
      ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                              pd_model_path.c_str());

      if (ret != CVI_SUCCESS) {
        printf("open CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN model failed with %#x!\n", ret);
        return "";
      }
    }

    ret =
        CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, pose_model_path.c_str());

    if (ret != CVI_SUCCESS) {
      printf("open CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE model failed with %#x!\n", ret);
      return "";
    }

    // CVI_TDL_SetMaxDetNum(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, 100);
    model_init = 1;
  }

  if (pd_model_path != no_model) {
    ret = CVI_TDL_Detection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_MOBILEDETV2_PEDESTRIAN failed with %#x!\n", ret);
      return "";
    }
  } else {
    CVI_TDL_MemAllocInit(boxes.size(), p_obj);
    p_obj->height = bg->stVFrame.u32Height;
    p_obj->width = bg->stVFrame.u32Width;
    p_obj->rescale_type = RESCALE_RB;

    memset(p_obj->info, 0, sizeof(cvtdl_object_info_t) * p_obj->size);
    for (uint32_t i = 0; i < p_obj->size; ++i) {
      p_obj->info[i].bbox.x1 = boxes[i][0];
      p_obj->info[i].bbox.y1 = boxes[i][1];
      p_obj->info[i].bbox.x2 = boxes[i][2];
      p_obj->info[i].bbox.y2 = boxes[i][3];
      p_obj->info[i].bbox.score = 1.0;
      p_obj->info[i].classes = 0;
      const string &classname = "person";
      strncpy(p_obj->info[i].name, classname.c_str(), sizeof(p_obj->info[i].name));
    }
  }

  if (p_obj->size > 0) {
    ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_Simcc_Pose failed with %#x!\n", ret);
      return "";
    }
    std::stringstream ss;
    for (uint32_t i = 0; i < p_obj->size; i++) {
      ss << file_name << ";";
      for (uint32_t j = 0; j < 17; j++) {
        ss << std::fixed << std::setprecision(4)
           << p_obj->info[i].pedestrian_properity->pose_17.x[j] << " " << std::fixed
           << std::setprecision(4) << p_obj->info[i].pedestrian_properity->pose_17.y[j] << " "
           << std::fixed << std::setprecision(4)
           << p_obj->info[i].pedestrian_properity->pose_17.score[j] << ";";
      }
      ss << "\n";
    }
    return ss.str();

  } else {
    printf("cannot find person\n");
    return "";
  }
}

std::string process_yolov8_pose(cvitdl_handle_t tdl_handle, std::string &pose_model_path,
                                cvtdl_object_t *p_obj, VIDEO_FRAME_INFO_S *bg,
                                std::string &file_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    ret =
        CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, pose_model_path.c_str());
    printf("set interval\n");
    if (ret != CVI_SUCCESS) {
      printf("open CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE model failed with %#x!\n", ret);
      return "";
    }

    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, 0.01);
    model_init = 1;
  }

  ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Yolov8_Pose failed with %#x!\n", ret);
    return "";
  }
  std::stringstream ss;
  ss << file_name;

  for (uint32_t i = 0; i < p_obj->size; i++) {
    ss << "#" << std::fixed << std::setprecision(4) << p_obj->info[i].bbox.x1 << " " << std::fixed
       << std::setprecision(4) << p_obj->info[i].bbox.y1 << " " << std::fixed
       << std::setprecision(4) << p_obj->info[i].bbox.x2 << " " << std::fixed
       << std::setprecision(4) << p_obj->info[i].bbox.y2 << " " << std::fixed
       << std::setprecision(4) << p_obj->info[i].bbox.score;

    for (uint32_t j = 0; j < 17; j++) {
      ss << ";" << std::fixed << std::setprecision(4)
         << p_obj->info[i].pedestrian_properity->pose_17.x[j] << " " << std::fixed
         << std::setprecision(4) << p_obj->info[i].pedestrian_properity->pose_17.y[j] << " "
         << std::fixed << std::setprecision(4)
         << p_obj->info[i].pedestrian_properity->pose_17.score[j];
    }
  }
  ss << "\n";

  return ss.str();
}

std::string process_hrnet_pose(cvitdl_handle_t tdl_handle, std::string &pd_model_path,
                               std::string &pose_model_path, cvtdl_object_t *p_obj,
                               VIDEO_FRAME_INFO_S *bg, std::vector<std::vector<int>> &boxes,
                               std::string &file_name) {
  static int model_init = 0;
  std::string no_model("None");
  CVI_S32 ret;
  if (model_init == 0) {
    if (pd_model_path != no_model) {
      ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                              pd_model_path.c_str());

      if (ret != CVI_SUCCESS) {
        printf("open CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN model failed with %#x!\n", ret);
        return "";
      }
    }

    ret =
        CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HRNET_POSE, pose_model_path.c_str());
    // CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HRNET_POSE, true);

    if (ret != CVI_SUCCESS) {
      printf("open CVI_TDL_SUPPORTED_MODEL_HRNET_POSE model failed with %#x!\n", ret);
      return "";
    }
    model_init = 1;
  }

  if (pd_model_path != no_model) {
    ret = CVI_TDL_Detection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_MOBILEDETV2_PEDESTRIAN failed with %#x!\n", ret);
      return "";
    }
  } else {
    CVI_TDL_MemAllocInit(boxes.size(), p_obj);
    p_obj->height = bg->stVFrame.u32Height;
    p_obj->width = bg->stVFrame.u32Width;
    p_obj->rescale_type = RESCALE_RB;

    std::cout << "p_obj->height = " << p_obj->height << ", p_obj->width = " << p_obj->width
              << std::endl;

    memset(p_obj->info, 0, sizeof(cvtdl_object_info_t) * p_obj->size);
    for (uint32_t i = 0; i < p_obj->size; ++i) {
      p_obj->info[i].bbox.x1 = boxes[i][0];
      p_obj->info[i].bbox.y1 = boxes[i][1];
      p_obj->info[i].bbox.x2 = boxes[i][2];
      p_obj->info[i].bbox.y2 = boxes[i][3];
      p_obj->info[i].bbox.score = 1.0;
      p_obj->info[i].classes = 0;
      const string &classname = "person";
      strncpy(p_obj->info[i].name, classname.c_str(), sizeof(p_obj->info[i].name));
    }
  }

  if (p_obj->size > 0) {
    ret = CVI_TDL_PoseDetection(tdl_handle, bg, CVI_TDL_SUPPORTED_MODEL_HRNET_POSE, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_Hrnet_Pose failed with %#x!\n", ret);
      return "";
    }
    std::stringstream ss;
    for (uint32_t i = 0; i < p_obj->size; i++) {
      ss << file_name << ";";
      for (uint32_t j = 0; j < 17; j++) {
        ss << std::fixed << std::setprecision(4)
           << p_obj->info[i].pedestrian_properity->pose_17.x[j] << " " << std::fixed
           << std::setprecision(4) << p_obj->info[i].pedestrian_properity->pose_17.y[j] << " "
           << std::fixed << std::setprecision(4)
           << p_obj->info[i].pedestrian_properity->pose_17.score[j] << ";";
      }
      ss << "\n";
    }
    return ss.str();

  } else {
    printf("cannot find person\n");
    return "";
  }
}

int main(int argc, char *argv[]) {
  std::string pose_model(argv[1]);    // pose detection model path
  std::string process_flag(argv[2]);  // simcc | yolov8pose |hrnet
  std::string image_root(argv[3]);    // img dir
  std::string image_list(argv[4]);    // txt file , yolov8pose: image name,  simcc and hrnet: image
                                      // name, x1 y1 x2 y2, x1 y1 x2 y2,...
  std::string dst_root(argv[5]);      // predict result path, end with .txt
  int show = atoi(argv[6]);           // 1 for show keypoints, 0 for not show;

  std::string pd_model;
  std::string show_root;

  std::cout << "image_root: " << image_root << std::endl;

  if (process_flag == "simcc" || process_flag == "hrnet") {
    pd_model = argv[7];  // person detection model path, set to "None" if do not use pd_model
  }
  if (show) {
    show_root = argv[8];  // dir to save img
  }
  int starti = 0;
  if (argc > 9) starti = atoi(argv[9]);

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

  std::cout << "to read imagelist:" << image_list << std::endl;
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (image_root.size() == 0) {
    std::cout << ", imageroot empty\n";
    return -1;
  }

  std::cout << "start i:" << starti << "\t";
  std::cout << image_files.size() << "\t";
  decltype(image_files.size()) i = starti;
  // std::cout << "processing :" << i << "/" << image_files.size() << "\t" << image_files[i] <<
  // "\t";
  std::cout << (i < image_files.size()) << "\t";
  std::cout << "init i done" << std::endl;

  PIXEL_FORMAT_E img_format = PIXEL_FORMAT_RGB_888_PLANAR;
  if (show)
    img_format =
        PIXEL_FORMAT_BGR_888;  // if show, img format should be PIXEL_FORMAT_BGR_888(for opencv)
  std::cout << image_files.size() << std::endl;

  std::vector<std::vector<int>> boxes;
  FILE *fp = fopen(dst_root.c_str(), "w");
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (i = starti; i < image_files.size(); i++) {
    boxes.clear();

    std::string file_name = split_file_line(image_files[i], boxes);

    std::cout << "processing :" << i + 1 << "/" << image_files.size() << "\t" << file_name << "\t";

    std::string strf = join_path(image_root, file_name);

    VIDEO_FRAME_INFO_S bg;
    // std::cout << strf<< std::endl;

    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &bg, img_format);
    std::cout << "ret: " << ret << std::endl;

    std::cout << "CVI_TDL_ReadImage done\t";
    if (ret != CVI_SUCCESS) {
      std::cout << "Convert to video frame failed with:" << ret << ",file:" << strf << std::endl;
      continue;
    }

    float score;  // for show
    cvtdl_object_t obj_meta = {0};
    std::string str_res;
    if (process_flag == "simcc") {
      str_res =
          process_simcc_pose(tdl_handle, pd_model, pose_model, &obj_meta, &bg, boxes, file_name);
      CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, &score);

    } else if (process_flag == "yolov8pose") {
      str_res = process_yolov8_pose(tdl_handle, pose_model, &obj_meta, &bg, file_name);
      CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE, &score);

    } else if (process_flag == "hrnet") {
      str_res =
          process_hrnet_pose(tdl_handle, pd_model, pose_model, &obj_meta, &bg, boxes, file_name);
      CVI_TDL_GetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HRNET_POSE, &score);
    } else {
      printf("Error: process_flag should be simcc, hrnet or yolov8pose, but got %s !\n",
             process_flag.c_str());
      fclose(fp);
      CVI_TDL_ReleaseImage(img_handle, &bg);
      CVI_TDL_DestroyHandle(tdl_handle);
      return -1;
    }

    //////////////////////////////////////////////////////////////////
    if (str_res.size() > 0) {
      fwrite(str_res.c_str(), str_res.size(), 1, fp);
    } else {
      std::string fname = file_name + std::string(";\n");
      fwrite(fname.c_str(), fname.size(), 1, fp);
    }
    for (uint32_t i = 0; i < obj_meta.size; i++) {
      std::cout << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
                << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << "]," << std::endl;
      for (uint32_t j = 0; j < 17; j++) {
        std::cout << j << ": " << obj_meta.info[i].pedestrian_properity->pose_17.x[j] << " "
                  << obj_meta.info[i].pedestrian_properity->pose_17.y[j] << " "
                  << obj_meta.info[i].pedestrian_properity->pose_17.score[j] << std::endl;
      }
    }

    PIXEL_FORMAT_E img_format2 = PIXEL_FORMAT_BGR_888;
    VIDEO_FRAME_INFO_S bg2;
    CVI_TDL_ReadImage(img_handle, strf.c_str(), &bg2, img_format2);
    std::cout << "done 1 " << std::endl;
    std::string show_root2 = argv[8];
    std::string save_path = join_path(show_root2, file_name);
    std::cout << "save_path is  " << save_path << std::endl;
    // show_keypoints(&bg2, &obj_meta, save_path, score);

    // if (show) {  // img format should be PIXEL_FORMAT_BGR_888
    //   std::string save_path = join_path(show_root, file_name);
    //   show_keypoints(&bg, &obj_meta, save_path, score);
    // }
    CVI_TDL_ReleaseImage(img_handle, &bg);
    CVI_TDL_ReleaseImage(img_handle, &bg2);
    CVI_TDL_Free(&obj_meta);
  }
  fclose(fp);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
