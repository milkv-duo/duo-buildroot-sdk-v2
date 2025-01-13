#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
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
#include "sys_utils.hpp"

int process_image_file(cvitdl_handle_t tdl_handle, const std::string &imgf, cvtdl_face_t *p_obj) {
  VIDEO_FRAME_INFO_S bg;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  int ret = CVI_TDL_ReadImage(img_handle, imgf.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    std::cout << "failed to open file:" << imgf << std::endl;
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
    ret = CVI_TDL_FaceAttribute_cls(tdl_handle, &bg, p_obj);
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

  std::string fd_model(argv[1]);  // fd ai_model
  std::string fr_model(argv[2]);  // fr ai_model
  std::string img(argv[3]);       // img1;

  printf("------------------------------------------------------\n");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, fd_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }
  printf("****************************************************\n");
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, fr_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION model failed with %#x!\n", ret);
    return ret;
  }

  cvtdl_face_t faces = {0};
  process_image_file(tdl_handle, img, &faces);
  printf("faces_size: %d\n", faces.size);
  for (uint32_t i = 0; i < faces.size; i++) {
    printf("face_label: %d\n", i);
    if (faces.info[i].gender_score < 0.5) {
      printf("gender: Female\n");
    } else {
      printf("gender: Male\n");
    }

    int age_as_int = (int)(faces.info[i].age * 100);
    printf("age: %d\n", age_as_int);

    if (faces.info[i].glass < 0.5) {
      printf("glass: NO\n");
    } else {
      printf("glass: YES\n");
    }

    if (faces.info[i].mask_score < 0.5) {
      printf("mask: NO\n");
    } else {
      printf("mask: YES\n");
    }
  }

  CVI_TDL_Free(&faces);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
