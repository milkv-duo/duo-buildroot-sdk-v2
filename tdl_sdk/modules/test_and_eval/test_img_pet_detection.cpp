#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 2,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 2);
  if (ret != CVI_SUCCESS) {
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
  int eval_perf = 0;
  if (argc > 3) {
    eval_perf = atoi(argv[3]);
  }
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

  printf("---------------------openmodel-----------------------");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, argv[1]);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, 0.1);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }
  printf("---------------------to do detection-----------------------\n");

  std::string str_res;
  cvtdl_object_t obj_meta = {0};
  CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, &obj_meta);

  // CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, argv[2]);
  // CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, false);
  // CVI_TDL_HandClassification(tdl_handle, &bg, &obj_meta);

  std::cout << "objnum:" << obj_meta.size << std::endl;
  std::stringstream ss;
  ss << "boxes=[";
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
       << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
       << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << "],";
  }
  ss << "]\n";
  std::cout << ss.str();
  CVI_TDL_Free(&obj_meta);
  if (eval_perf) {
    for (int i = 0; i < 101; i++) {
      cvtdl_object_t obj_meta = {0};
      CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, &obj_meta);
      CVI_TDL_Free(&obj_meta);
    }
  }
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}