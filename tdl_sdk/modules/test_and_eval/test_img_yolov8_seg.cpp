#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
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
// set preprocess and algorithm param for yolov8 detection
// if use official model, no need to change param
CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
  // setup preprocess
  InputPreParam preprocess_cfg =
      CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG);

  for (int i = 0; i < 3; i++) {
    printf("asign val %d \n", i);
    preprocess_cfg.factor[i] = 0.003922;
    preprocess_cfg.mean[i] = 0.0;
  }
  preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

  printf("setup yolov8 param \n");
  CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, preprocess_cfg);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 preprocess parameters %#x\n", ret);
    return ret;
  }

  // setup yolo algorithm preprocess
  cvtdl_det_algo_param_t yolov8_param =
      CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG);
  yolov8_param.cls = 1;

  printf("setup yolov8 algorithm param \n");
  ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, yolov8_param);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 algorithm parameters %#x\n", ret);
    return ret;
  }

  // set theshold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, 0.5);

  printf("yolov8 algorithm parameters setup success!\n");
  return ret;
}

int main(int argc, char *argv[]) {
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

  std::string strf1(argv[2]);
  // change param of yolov8_detection
  ret = init_param(tdl_handle);

  printf("---------------------openmodel-----------------------");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, argv[1]);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, 0.5);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }
  printf("---------------------to do inference-----------------------\n");

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S bg;
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
    printf("image read,hidth:%d\n", bg.stVFrame.u32Height);
  }
  cvtdl_object_t obj_meta = {0};

  CVI_TDL_YoloV8_Seg(tdl_handle, &bg, &obj_meta);
  CVI_TDL_Set_MaskOutlinePoint(&bg, &obj_meta);

  std::cout << "objnum:" << obj_meta.size << std::endl;
  std::ofstream file("boxes_points_data.txt");

  for (uint32_t i = 0; i < obj_meta.size; i++) {
    std::cout << "boxes=[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
              << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
              << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << "]" << std::endl;

    file << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
         << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
         << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << std::endl;

    std::cout << "points=[";
    for (uint32_t j = 0; j < obj_meta.info[i].mask_properity->mask_point_size; j++) {
      std::cout << "(" << obj_meta.info[i].mask_properity->mask_point[2 * j] << ","
                << obj_meta.info[i].mask_properity->mask_point[2 * j + 1] << "),";

      file << "(" << obj_meta.info[i].mask_properity->mask_point[2 * j] << ","
           << obj_meta.info[i].mask_properity->mask_point[2 * j + 1] << "),";
    }
    std::cout << "]" << std::endl;
  }
  file.close();

  CVI_TDL_Free(&obj_meta);
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}