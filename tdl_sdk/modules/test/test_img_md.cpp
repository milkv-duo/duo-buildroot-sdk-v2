#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

#include <iostream>
#include <sstream>
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
  std::string strf1(argv[1]);
  std::string strf2(argv[2]);
  imgprocess_t img_handle = NULL;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  VIDEO_FRAME_INFO_S bg;
  CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_YUV_400);
  std::cout << "read image1 done\n";
  VIDEO_FRAME_INFO_S frame;
  cvtdl_object_t obj_meta;
  memset(&obj_meta, 0, sizeof(cvtdl_object_t));
  CVI_TDL_ReadImage(img_handle, strf2.c_str(), &frame, PIXEL_FORMAT_YUV_400);
  std::cout << "read image2 done\n";
  MDROI_t roi_s;
  roi_s.num = 2;
  roi_s.pnt[0].x1 = 0;
  roi_s.pnt[0].y1 = 0;
  roi_s.pnt[0].x2 = 512;
  roi_s.pnt[0].y2 = 512;
  roi_s.pnt[1].x1 = 1000;
  roi_s.pnt[1].y1 = 150;
  roi_s.pnt[1].x2 = 1150;
  roi_s.pnt[1].y2 = 250;
  CVI_TDL_Set_MotionDetection_Background(tdl_handle, &bg);
  std::cout << "set image bg done\n";
  CVI_TDL_Set_MotionDetection_ROI(tdl_handle, &roi_s);
  CVI_TDL_MotionDetection(tdl_handle, &frame, &obj_meta, 30, 100);
  std::stringstream ss;
  ss << "boxes=[";
  for (size_t i = 0; i < obj_meta.size; i++) {
    ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
       << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << "],";
  }
  std::cout << ss.str() << "]\n";
  CVI_TDL_Free(&obj_meta);
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_ReleaseImage(img_handle, &frame);

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
