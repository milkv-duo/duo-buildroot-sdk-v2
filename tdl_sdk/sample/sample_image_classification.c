#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"

int read_binary_file(const char *strf, void *p_buffer, int buffer_len) {
  FILE *fp = fopen(strf, "rb");
  if (fp == NULL) {
    printf("read file failed, %s\n", strf);
    return 0;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (len != buffer_len) {
    printf("size not equal, expect: %d, has %d\n", buffer_len, len);
    fclose(fp);
    return 0;
  }
  fread(p_buffer, len, 1, fp);
  fclose(fp);
  return 1;
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

  char *model_path = argv[1];
  char *str_src_dir = argv[2];

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION, model_path);
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }

  VIDEO_FRAME_INFO_S fdFrame;
  memset(&fdFrame, 0, sizeof(fdFrame));
  int imgwh = 256;
  int imgc = 4;
  int frame_size = imgc * imgwh * imgwh;

  uint8_t *p_buffer = (uint8_t *)malloc(frame_size);
  if (p_buffer == NULL) {
    printf("Memory allocation failed\n");
    return -1;
  }
  fdFrame.stVFrame.pu8VirAddr[0] = p_buffer;
  fdFrame.stVFrame.u32Height = imgwh;
  fdFrame.stVFrame.u32Width = imgwh;
  fdFrame.stVFrame.u32Length[0] = frame_size;

  if (!read_binary_file(str_src_dir, fdFrame.stVFrame.pu8VirAddr[0], frame_size)) {
    printf("read file failed\n");
    free(p_buffer);
    return -1;
  }

  float qscale = 126.504;
  for (int i = 0; i < frame_size; i++) {
    int val = p_buffer[i] * qscale / 255;
    if (val >= 127) {
      val = 127;
    }
    p_buffer[i] = (uint8_t)val;
  }

  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION, true);
  cvtdl_class_meta_t cls_meta = {0};

  CVI_TDL_Image_Classification(tdl_handle, &fdFrame, &cls_meta);

  for (uint32_t i = 0; i < 5; i++) {
    printf("no %d class %d score: %f\n", i + 1, cls_meta.cls[i], cls_meta.score[i]);
  }

  free(p_buffer);
  CVI_TDL_Free(&cls_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}