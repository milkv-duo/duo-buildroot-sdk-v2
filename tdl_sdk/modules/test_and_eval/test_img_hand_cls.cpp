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
  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  printf("---------------------read_image-----------------------\n");
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
    printf("image read,hidth:%d\n", bg.stVFrame.u32Height);
  }

  printf("---------------------openmodel-----------------------");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, argv[1]);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, 0.1);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  std::string str_res;
  cvtdl_object_t obj_meta = {0};
  CVI_TDL_MemAllocInit(1, &obj_meta);
  obj_meta.height = bg.stVFrame.u32Height;
  obj_meta.width = bg.stVFrame.u32Width;

  for (uint32_t i = 0; i < obj_meta.size; i++) {
    obj_meta.info[i].bbox.x1 = 0;
    obj_meta.info[i].bbox.x2 = bg.stVFrame.u32Width - 1;
    obj_meta.info[i].bbox.y1 = 0;
    obj_meta.info[i].bbox.y2 = bg.stVFrame.u32Height - 1;
    printf("init objbox:%f,%f,%f,%f\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2);
  }
  CVI_TDL_HandClassification(tdl_handle, &bg, &obj_meta);

  std::cout << "cls result:" << obj_meta.info[0].name << std::endl;

  CVI_TDL_Free(&obj_meta);
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}