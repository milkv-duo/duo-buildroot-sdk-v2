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
#include "sys_utils.h"

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
  int file_count = 0;
  char **file_list = read_file_lines(argv[2], &file_count);
  if (file_count == 0) {
    fprintf(stderr, "File list is empty\n");
    return CVI_FAILURE;
  }

  qsort(file_list, file_count, sizeof(char *), compareFileNames);
  char input_image_path[256];
  imgprocess_t img_handle;

  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (size_t i = 0; i < file_count; i++) {
    printf("Processing file: %s\n", file_list[i]);
    strcpy(input_image_path, file_list[i]);
    VIDEO_FRAME_INFO_S rgb_frame;
    size_t line_position = strrchr(input_image_path, '/') - input_image_path;
    size_t dot_position = strrchr(input_image_path, '.') - input_image_path;
    char pic_name[256];
    strncpy(pic_name, input_image_path + line_position + 1, dot_position - line_position - 1);
    pic_name[dot_position - line_position - 1] = '\0';
    printf("number of img:%zu; last of imgname:%s\n", i, pic_name);

    ret = CVI_TDL_ReadImage(img_handle, input_image_path, &rgb_frame, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }

    char filename[512];
    sprintf(filename, "%s%s.txt", argv[3], pic_name);
    FILE *outfile = fopen(filename, "w");
    if (!outfile) {
      fprintf(stderr, "无法打开文件\n");
      return -1;
    }

    cvtdl_face_t obj_meta = {0};
    CVI_TDL_FaceDetection(tdl_handle, &rgb_frame, CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, &obj_meta);

    printf("objnum:%d\n", obj_meta.size);
    int classes = 0;
    for (uint32_t i = 0; i < obj_meta.size; i++) {
      if (obj_meta.info[i].hardhat_score > 0.5)
        classes = 0;
      else
        classes = 1;
      printf("[%f,%f,%f,%f,%f,%f],", obj_meta.info[i].hardhat_score, obj_meta.info[i].bbox.x1,
             obj_meta.info[i].bbox.y1, obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2,
             obj_meta.info[i].bbox.score);
    }
    CVI_TDL_Free(&obj_meta);
    printf("after free:");
    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}