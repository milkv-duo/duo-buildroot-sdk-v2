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
#include "sys_utils.hpp"

std::string g_model_root;

std::string run_image_hand_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                     std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init hand model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION,
                            str_hand_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_hand_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    model_init = 1;
  }

  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION, &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect hand failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;

  for (uint32_t i = 0; i < hand_obj.size; i++) {
    cvtdl_bbox_t box = hand_obj.info[i].bbox;
    ss << (hand_obj.info[i].classes) << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << " " << box.score << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}

std::string run_image_pet_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                    std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init hand model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION,
                            str_hand_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_hand_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    model_init = 1;
  }

  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION,
                          &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect hand failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;

  for (uint32_t i = 0; i < hand_obj.size; i++) {
    cvtdl_bbox_t box = hand_obj.info[i].bbox;
    ss << (hand_obj.info[i].classes) << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << " " << box.score << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}

std::string run_image_vehicle_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                        std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init vehicle model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION,
                            str_hand_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open vehicle model failed:" << str_hand_model << std::endl;
      return "";
    }
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION,
                                  true);
    std::cout << "init vehicle model done\t";
    model_init = 1;
  }

  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_PersonVehicle_Detection(tdl_handle, p_frame, &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect vehicle failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;

  for (uint32_t i = 0; i < hand_obj.size; i++) {
    cvtdl_bbox_t box = hand_obj.info[i].bbox;
    ss << (hand_obj.info[i].classes) << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << " " << box.score << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}
std::string run_image_face_hand_person_detection(VIDEO_FRAME_INFO_S *p_frame,
                                                 cvitdl_handle_t tdl_handle,
                                                 std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init vehicle model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION,
                            str_hand_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open vehicle model failed:" << str_hand_model << std::endl;
      return "";
    }
    std::cout << "init vehicle model done\t";
    model_init = 1;
  }

  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION,
                          &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect vehicle failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;

  for (uint32_t i = 0; i < hand_obj.size; i++) {
    cvtdl_bbox_t box = hand_obj.info[i].bbox;
    ss << (hand_obj.info[i].classes) << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << " " << box.score << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}

std::string run_image_hand_classification(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                          std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init hand model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION,
                            str_hand_model.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_hand_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    model_init = 1;
  }

  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));
  CVI_TDL_MemAllocInit(1, &hand_obj);
  hand_obj.height = p_frame->stVFrame.u32Height;
  hand_obj.width = p_frame->stVFrame.u32Width;
  for (uint32_t i = 0; i < hand_obj.size; i++) {
    hand_obj.info[i].bbox.x1 = 0;
    hand_obj.info[i].bbox.x2 = p_frame->stVFrame.u32Width - 1;
    hand_obj.info[i].bbox.y1 = 0;
    hand_obj.info[i].bbox.y2 = p_frame->stVFrame.u32Height - 1;
    // printf("init
    // objbox:%f,%f,%f,%f\n",obj_meta.info[i].bbox.x1,obj_meta.info[i].bbox.y1,obj_meta.info[i].bbox.x2,obj_meta.info[i].bbox.y2);
  }
  ret = CVI_TDL_HandClassification(tdl_handle, p_frame, &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "classify hand failed:" << ret << std::endl;
  }
  std::cout << "classify hand success" << std::endl;
  // generate detection result
  std::stringstream ss;
  // hand_obj.size
  for (uint32_t i = 0; i < hand_obj.size; i++) {
    // cvtdl_bbox_t box = hand_obj.info[i].bbox;(hand_obj.info[i].classes + 1)
    ss << hand_obj.info[i].name << " " << hand_obj.info[i].bbox.score << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}

int main(int argc, char *argv[]) {
  g_model_root = std::string(argv[1]);
  std::string image_root(argv[2]);
  std::string image_list(argv[3]);
  std::string dst_root(argv[4]);
  std::string process_flag(argv[5]);
  std::string model_name(argv[6]);

  if (image_root.at(image_root.size() - 1) != '/') {
    image_root = image_root + std::string("/");
  }
  if (dst_root.at(dst_root.size() - 1) != '/') {
    dst_root = dst_root + std::string("/");
  }
  // Init VB pool size.
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;

  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 3);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }
  std::cout << "to read imagelist:" << image_list << std::endl;
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (image_root.size() == 0) {
    std::cout << ", imageroot empty\n";
    return -1;
  }
  std::map<std::string,
           std::function<std::string(VIDEO_FRAME_INFO_S *, cvitdl_handle_t, std::string)>>
      process_funcs = {{"detect", run_image_hand_detection},
                       {"classify", run_image_hand_classification},
                       {"pet", run_image_pet_detection},
                       {"vehicle", run_image_vehicle_detection},
                       {"meeting", run_image_face_hand_person_detection}};
  if (process_funcs.count(process_flag) == 0) {
    std::cout << "error flag:" << process_flag << std::endl;
    return -1;
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (size_t i = 0; i < image_files.size(); i++) {
    std::cout << "processing :" << i << "/" << image_files.size() << "\t" << image_files[i]
              << std::endl;
    std::string strf = image_root + image_files[i];
    std::string dstf = dst_root + replace_file_ext(image_files[i], "txt");
    VIDEO_FRAME_INFO_S fdFrame;
    std::string s_vehicle = "vehicle";
    if (process_flag == s_vehicle) {
      ret = CVI_TDL_ReadImage_Resize(img_handle, strf.c_str(), &fdFrame,
                                     PIXEL_FORMAT_RGB_888_PLANAR, 640, 384);
    } else {
      ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    }
    std::cout << "CVI_TDL_ReadImage done\t";

    if (ret != CVI_SUCCESS) {
      std::cout << "Convert to video frame failed with:" << ret << ",file:" << strf << std::endl;
      continue;
    }

    std::string str_res = process_funcs[process_flag](&fdFrame, tdl_handle, model_name);

    std::cout << "process_funcs done\t";
    std::cout << "str_res.size():" << str_res.size() << std::endl;

    if (str_res.size() > 0) {
      std::cout << "writing file:" << dstf << std::endl;
      FILE *fp = fopen(dstf.c_str(), "w");
      fwrite(str_res.c_str(), str_res.size(), 1, fp);
      fclose(fp);
    }
    std::cout << "write results done\t";
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
    std::cout << "CVI_TDL_ReleaseImage done\t" << std::endl;
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
