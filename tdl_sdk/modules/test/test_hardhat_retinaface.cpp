#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.hpp"
bool CompareFileNames(std::string a, std::string b) { return a < b; }
// set preprocess and algorithm param for yolov8 detection
// if use official model, no need to change param

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  if (argc != 4) {
    printf(
        "Usage: %s <clip model path> <input image directory list.txt> <output result "
        "directory/>.\n",
        argv[0]);
    printf("clip model path: Path to clip bmodel.\n");
    printf("Input image directory: Directory containing input images for clip.\n");
    printf("Output result directory: Directory to save clip feature.bin\n");
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;
  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
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

  printf("---------------------openmodel-----------------------");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, 0.5);
  printf("---------------------to do detection-----------------------\n");

  //  IMAGE_LIST
  std::string image_list(argv[2]);

  //  dir_name
  std::string dir_name(argv[3]);

  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }
  std::sort(file_list.begin(), file_list.end(), CompareFileNames);
  std::string input_image_path;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (size_t i = 0; i < file_list.size(); i++) {
    input_image_path = file_list[i];
    VIDEO_FRAME_INFO_S rgb_frame;
    size_t line_position = input_image_path.find_last_of('/');
    size_t dot_position = input_image_path.find_last_of('.');
    std::string pic_name =
        input_image_path.substr(line_position + 1, dot_position - line_position - 1).c_str();
    std::cout << "number of img:" << i << ";last of imgname:" << pic_name << std::endl;
    ret = CVI_TDL_ReadImage(img_handle, input_image_path.c_str(), &rgb_frame,
                            PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }
    std::ofstream outfile(dir_name + pic_name + ".txt");
    if (!outfile) {
      std::cerr << "无法打开文件" << std::endl;
      return -1;
    }
    std::string str_res;
    cvtdl_face_t obj_meta = {0};
    CVI_TDL_FaceDetection(tdl_handle, &rgb_frame, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, &obj_meta);
    std::cout << "objnum:" << obj_meta.size << std::endl;
    int classes = 0;
    for (uint32_t i = 0; i < obj_meta.size; i++) {
      if (obj_meta.info[i].hardhat_score > 0.5)
        classes = 0;
      else
        classes = 1;
      outfile << obj_meta.info[i].hardhat_score << ' ' << obj_meta.info[i].bbox.x1 << " "
              << obj_meta.info[i].bbox.y1 << " " << obj_meta.info[i].bbox.x2 << " "
              << obj_meta.info[i].bbox.y2 << " " << obj_meta.info[i].bbox.score << "\n";
    }
    CVI_TDL_Free(&obj_meta);
    outfile.close();
    std::cout << "after free:" << std::endl;
    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}