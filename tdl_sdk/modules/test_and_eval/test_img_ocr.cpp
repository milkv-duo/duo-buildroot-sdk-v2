#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

void draw_and_save_picture(VIDEO_FRAME_INFO_S* bg, const cvtdl_object_t& object_meta,
                           const std::string& save_path) {
  CVI_U8* r_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[0], bg->stVFrame.u32Length[0]);
  CVI_U8* g_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[1], bg->stVFrame.u32Length[1]);
  CVI_U8* b_plane = (CVI_U8*)CVI_SYS_Mmap(bg->stVFrame.u64PhyAddr[2], bg->stVFrame.u32Length[2]);

  cv::Mat r_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, r_plane);
  cv::Mat g_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, g_plane);
  cv::Mat b_mat(bg->stVFrame.u32Height, bg->stVFrame.u32Width, CV_8UC1, b_plane);

  std::vector<cv::Mat> channels = {b_mat, g_mat, r_mat};  // 注意 OpenCV 中的顺序是 BGR
  cv::Mat img_rgb;
  cv::merge(channels, img_rgb);

  for (uint32_t i = 0; i < object_meta.entry_num; ++i) {
    const auto& bbox_info = object_meta.info[i].bbox;
    cv::rectangle(img_rgb,
                  cv::Point(static_cast<int>(bbox_info.x1), static_cast<int>(bbox_info.y1)),
                  cv::Point(static_cast<int>(bbox_info.x2), static_cast<int>(bbox_info.y2)),
                  cv::Scalar(0, 255, 0), 2);
  }

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

  std::string strf1(argv[3]);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OCR_DETECTION, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OCR_RECOGNITION, argv[2]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
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
  }

  cvtdl_object_t obj_meta = {0};
  ret = CVI_TDL_OCR_Detection(tdl_handle, &bg, &obj_meta);
  if (obj_meta.size > 0) {
    ret = CVI_TDL_OCR_Recognition(tdl_handle, &bg, &obj_meta);
  } else {
    printf("cannot find ocr bbox\n");
  }

  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
