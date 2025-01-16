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
#include "cvi_kit.h"

int process_image_file(cvitdl_handle_t tdl_handle, char* imgf, cvtdl_face_t* p_obj) {
  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  int ret = CVI_TDL_ReadImage(img_handle, imgf, &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("failed to open file:%s\n", imgf);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }

  ret = CVI_TDL_FaceDetection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, p_obj);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_ScrFDFace failed with %#x!\n", ret);
    return ret;
  }

  if (p_obj->size > 0) {
    ret = CVI_TDL_FaceRecognition(tdl_handle, &bg, p_obj);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_FaceAttribute failed with %#x!\n", ret);
      return ret;
    }
  } else {
    printf("cannot find faces\n");
  }
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}

int main(int argc, char* argv[]) {
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

  char* fd_model = argv[1];  // fd ai_model
  char* fr_model = argv[2];  // fr ai_model
  char* img = argv[3];       // img1;

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, fd_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, fr_model);
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION model failed with %#x!\n", ret);
    return ret;
  }

  cvtdl_face_t obj_meta = {0};
  process_image_file(tdl_handle, img, &obj_meta);

  printf("boxes=[");
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("[%d,%d,%d,%d],", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2);
  }
  printf("]\n");

  if (argc >= 5) {
    // other picture
    char* img1 = argv[4];  // img2;
    cvtdl_face_t obj_meta1 = {0};
    process_image_file(tdl_handle, img1, &obj_meta1);

    printf("boxes=[");
    for (uint32_t i = 0; i < obj_meta1.size; i++) {
      printf("[%d,%d,%d,%d],", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
             obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2);
    }
    printf("]\n");

    // compare cal_similarity
    cvtdl_feature_t feature = obj_meta.info[0].feature;
    cvtdl_feature_t feature1 = obj_meta1.info[0].feature;
    float similarity;
    CVI_TDL_Cal_Similarity(feature, feature1, &similarity);
    printf("similarity:%f", similarity);
    CVI_TDL_Free(&obj_meta1);
  }

  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
