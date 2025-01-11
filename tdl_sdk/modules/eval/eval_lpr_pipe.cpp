#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <sys/time.h>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "sys_utils.hpp"
#include "cvi_tdl_media.h"

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }
bool CompareFileNames(std::string a, std::string b) { return a < b; }

CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
  // setup preprocess
  InputPreParam preprocess_cfg =
      CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);

  for (int i = 0; i < 3; i++) {
    printf("asign val %d \n", i);
    preprocess_cfg.factor[i] = 0.003922;
    preprocess_cfg.mean[i] = 0.0;
  }
  preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;
  printf("setup yolov8 param \n");
  CVI_S32 ret =
      CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, preprocess_cfg);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 preprocess parameters %#x\n", ret);
    return ret;
  }

  // setup yolo algorithm preprocess
  cvtdl_det_algo_param_t yolov8_param =
      CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
  yolov8_param.cls = 1;

  printf("setup yolov8 algorithm param \n");
  ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                      yolov8_param);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 algorithm parameters %#x\n", ret);
    return ret;
  }

  // set theshold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);

  printf("yolov8 algorithm parameters setup success!\n");
  return ret;
}
int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 4,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 4);
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


  if (atoi(argv[5]) == 0){
    ret = init_param(tdl_handle);
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, argv[1]);
  }else{
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LP_DETECTION, argv[1]);
  }
  if (ret != CVI_SUCCESS) {
    printf("open model DETECTION failed with %#x!\n", ret);
    return ret;
  }
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LP_KEYPOINT, argv[2]);
  if (ret != CVI_SUCCESS) {
    printf("open model KEYPOINT failed with %#x!\n", ret);
    return ret;
  }
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LP_RECONGNITION, argv[3]);
  if (ret != CVI_SUCCESS) {
    printf("open model RECONGNITION failed with %#x!\n", ret);
    return ret;
  }
  std::string image_list(argv[4]);
  std::map<std::string, int> char_map;
  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }
  std::sort(file_list.begin(), file_list.end(), CompareFileNames);
  std::string input_image_path;
  int correct_count = 0;
  for (size_t i = 0; i < file_list.size(); i++) {
    vector<int> plat_gt;
    int pre_true = 1;
    VIDEO_FRAME_INFO_S bg;
    imgprocess_t img_handle;
    CVI_TDL_Create_ImageProcessor(&img_handle);
    input_image_path = file_list[i];
    size_t line_position = input_image_path.find_last_of('/');
    size_t dot_position = input_image_path.find_last_of('.');
    std::string pic_name = input_image_path.substr(line_position+1, dot_position-line_position-1);
    std::cout<<"number of img:" << i <<";last of imgname:" << pic_name << std::endl;
    ret = CVI_TDL_ReadImage(img_handle,input_image_path.c_str(), &bg, PIXEL_FORMAT_BGR_888);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    } else {
      printf("image read,width:%d\n", bg.stVFrame.u32Width);
    }
    cvtdl_object_t obj_meta = {0};
    if (atoi(argv[5])==0){
      CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, &obj_meta);
    }else{
      CVI_TDL_License_Plate_Detectionv2(tdl_handle, &bg, &obj_meta);
    }
    printf("obj_size: %d\n", obj_meta.size);
    if (obj_meta.size>0){
      CVI_TDL_License_Plate_Keypoint(tdl_handle, &bg, &obj_meta);
      // Both two interfaces are able to call
      // CVI_TDL_LicensePlateRecognition_V2(tdl_handle, &bg, &obj_meta);
      CVI_TDL_LicensePlateRecognition(tdl_handle, &bg,CVI_TDL_SUPPORTED_MODEL_LP_RECONGNITION, &obj_meta);
      for (size_t n = 0; n < obj_meta.size; n++) {
        std::string license_str(obj_meta.info[n].vehicle_properity->license_char);
        license_str.erase(std::remove(license_str.begin(), license_str.end(), ' '), license_str.end());
        std::cout << "plate i :"<<n<<";pre License char:" <<license_str<< std::endl;
      }
      std::string license_str(obj_meta.info[0].vehicle_properity->license_char);
      license_str.erase(std::remove(license_str.begin(), license_str.end(), ' '), license_str.end());
      if (license_str == pic_name) {
        correct_count ++;
      } else {
        std::cout << "The strings are not equal." << std::endl;
      }
    }
    CVI_TDL_Free(&obj_meta);
    CVI_TDL_ReleaseImage(img_handle, &bg);
    CVI_TDL_Destroy_ImageProcessor(img_handle);
  }
  std::cout << "correct_count:"<<correct_count<<";correct:" <<float(correct_count)/file_list.size()<< std::endl;
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}