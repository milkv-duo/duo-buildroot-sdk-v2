#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>
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
// #include "sys_utils.hpp"

bool read_binary_file(const std::string &strf, void *p_buffer, int buffer_len) {
  FILE *fp = fopen(strf.c_str(), "rb");
  if (fp == nullptr) {
    std::cout << "read file failed," << strf << std::endl;
    return false;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (len != buffer_len) {
    std::cout << "size not equal,expect:" << buffer_len << ",has " << len << std::endl;
    return false;
  }
  fread(p_buffer, len, 1, fp);
  fclose(fp);
  return true;
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

  std::string model_path = argv[1];
  std::string str_src_dir = argv[2];

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION,
                          model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }

  // imgprocess_t img_handle;
  // CVI_TDL_Create_ImageProcessor(&img_handle);

  // VIDEO_FRAME_INFO_S fdFrame;
  // ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  // if (ret != CVI_SUCCESS) {
  //   std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
  //             << std::endl;
  //   // continue;
  // }

  VIDEO_FRAME_INFO_S fdFrame;
  memset(&fdFrame, 0, sizeof(fdFrame));
  int imgwh = 256;
  int imgc = 4;
  int frame_size = imgc * imgwh * imgwh;

  uint8_t *p_buffer = new uint8_t[frame_size];
  fdFrame.stVFrame.pu8VirAddr[0] = p_buffer;  // Global buffer
  fdFrame.stVFrame.u32Height = imgwh;
  fdFrame.stVFrame.u32Width = imgwh;
  fdFrame.stVFrame.u32Length[0] = frame_size;
  if (!read_binary_file(str_src_dir.c_str(), fdFrame.stVFrame.pu8VirAddr[0], frame_size)) {
    printf("read file failed\n");
    return -1;
  }
  float qscale = 126.504;
  for (int i = 0; i < frame_size; i++) {
    int val = p_buffer[i] * qscale / 255;
    if (val >= 127) {
      val = 127;
    }
    p_buffer[i] = val;
  }
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION, true);
  cvtdl_class_meta_t cls_meta = {0};

  CVI_TDL_Image_Classification(tdl_handle, &fdFrame, &cls_meta);

  for (uint32_t i = 0; i < 5; i++) {
    printf("no %d class %d score: %f \n", i + 1, cls_meta.cls[i], cls_meta.score[i]);
  }

  // CVI_TDL_ReleaseImage(img_handle, &fdFrame);
  delete fdFrame.stVFrame.pu8VirAddr[0];
  CVI_TDL_Free(&cls_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  // CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}