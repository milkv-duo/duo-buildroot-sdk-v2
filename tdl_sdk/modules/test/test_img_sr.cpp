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

void save_picture(VIDEO_FRAME_INFO_S* bg, std::string save_path) {
  CVI_U8* r_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);
  CVI_U8* g_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[1], bg->stVFrame.u32Length[1]);
  CVI_U8* b_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[2], bg->stVFrame.u32Length[2]);

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
}

int main(int argc, char* argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3);
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

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SUPER_RESOLUTION, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S bg;
  // printf("toread image:%s\n",argv[1]);
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }
  for (int i = 0; i < 1; i++) {
    cvtdl_sr_feature obj_meta = {0};
    ret = CVI_TDL_Super_Resolution(tdl_handle, &bg, &obj_meta);
    std::string save_path = "./save_test.jpg";
    save_picture(obj_meta.dstframe, save_path);
    free(obj_meta.dstframe);
  }
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
