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

static float cal_similarity(cv::Mat feature1, cv::Mat feature2) {
  return feature1.dot(feature2) / (cv::norm(feature1) * cv::norm(feature2));
}

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

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, fd_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_SCRFDFACE model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, fr_model.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION model failed with %#x!\n", ret);
    return ret;
  }

  std::string str_res;
  std::stringstream ss;
  cvtdl_face_t obj_meta = {0};
  process_image_file(tdl_handle, img, &obj_meta);

  ss << "boxes=[";
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
       << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << "],";
  }
  str_res = ss.str();
  str_res.at(str_res.length() - 1) = ']';

  std::cout << str_res << std::endl;

  if (argc >= 5) {
    // other picture
    std::string img1(argv[4]);  // img2;
    std::string str_res1;
    std::stringstream ss1;
    cvtdl_face_t obj_meta1 = {0};
    process_image_file(tdl_handle, img1, &obj_meta1);

    ss1 << "boxes=[";
    for (uint32_t i = 0; i < obj_meta1.size; i++) {
      ss1 << "[" << obj_meta1.info[i].bbox.x1 << "," << obj_meta1.info[i].bbox.y1 << ","
          << obj_meta1.info[i].bbox.x2 << "," << obj_meta1.info[i].bbox.y2 << "],";
    }
    str_res1 = ss1.str();
    str_res1.at(str_res1.length() - 1) = ']';
    ;
    std::cout << str_res1 << std::endl;

    // compare cal_similarity
    cvtdl_feature_t feature = obj_meta.info[0].feature;
    cvtdl_feature_t feature1 = obj_meta1.info[0].feature;
    cv::Mat mat_feature(feature.size, 1, CV_8SC1);
    cv::Mat mat_feature1(feature1.size, 1, CV_8SC1);
    memcpy(mat_feature.data, feature.ptr, feature.size);
    memcpy(mat_feature1.data, feature1.ptr, feature1.size);
    mat_feature.convertTo(mat_feature, CV_32FC1, 1.);
    mat_feature1.convertTo(mat_feature1, CV_32FC1, 1.);

    float similarity = cal_similarity(mat_feature, mat_feature1);
    std::cout << "similarity:" << similarity << std::endl;
    CVI_TDL_Free(&obj_meta1);
  }

  CVI_TDL_Free(&obj_meta);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
