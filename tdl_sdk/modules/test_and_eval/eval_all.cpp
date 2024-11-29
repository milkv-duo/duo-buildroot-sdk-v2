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
#include "sys_utils.hpp"

std::string g_model_root;

std::string run_image_hand_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                     std::string model_name, std::string image_name) {
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

std::string run_image_object_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                       std::string model_name, std::string image_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init object model\t";
    std::string str_object_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80,
                            str_object_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_object_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    model_init = 1;
  }

  cvtdl_object_t obj = {0};
  memset(&obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80, &obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect object failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;

  for (uint32_t i = 0; i < obj.size; i++) {
    cvtdl_bbox_t box = obj.info[i].bbox;
    ss << (obj.info[i].classes) << " " << box.x1 << " " << box.y1 << " " << box.x2 << " " << box.y2
       << " " << box.score << "\n";
  }
  CVI_TDL_Free(&obj);
  return ss.str();
}

std::string run_image_pet_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                    std::string model_name, std::string image_name) {
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
                                        std::string model_name, std::string image_name) {
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
                                                 cvitdl_handle_t tdl_handle, std::string model_name,
                                                 std::string image_name) {
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
                                          std::string model_name, std::string image_name) {
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

std::string run_image_scrfd_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                      std::string model_name, std::string image_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init vehicle model\t";
    std::string str_hand_model = g_model_root + std::string("/") + model_name;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, str_hand_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, 0.01);
    if (ret != CVI_SUCCESS) {
      std::cout << "open vehicle model failed:" << str_hand_model << std::endl;
      return "";
    }
    std::cout << "init vehicle model done\t";
    model_init = 1;
  }

  cvtdl_face_t obj_meta = {0};
  memset(&obj_meta, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_FaceDetection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, &obj_meta);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect vehicle failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream str_res;

  str_res << "# " << image_name << std::endl;
  str_res << "boxes=[";
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    if (i < obj_meta.size - 1) {
      str_res << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
              << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
              << obj_meta.info[i].bbox.score << "],";
    } else {
      str_res << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
              << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
              << obj_meta.info[i].bbox.score << "]";
    }
  }
  str_res << "]" << std::endl;
}

std::string run_image_person_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                       std::string model_name, std::string image_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init Person model\t";
    std::string str_person_model = g_model_root + std::string("/") + model_name;
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                            str_person_model.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_person_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, 0.01);
    std::cout << "set threshold done\t";
    model_init = 1;
  }
  cvtdl_object_t person_obj;
  memset(&person_obj, 0, sizeof(cvtdl_object_t));
  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                          &person_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect person failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;
  for (uint32_t i = 0; i < person_obj.size; i++) {
    cvtdl_bbox_t box = person_obj.info[i].bbox;
    ss << (person_obj.info[i].classes + 1) << " " << box.score << " " << box.x1 << " " << box.y1
       << " " << box.x2 << " " << box.y2 << "\n";
  }
  CVI_TDL_Free(&person_obj);
  return ss.str();
}

std::string run_image_hand_keypoint(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                    std::string model_name, std::string image_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init hand keypoint model\t";
    std::string str_person_model = g_model_root + std::string("/") + model_name;
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT,
                            str_person_model.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_person_model << std::endl;
      return "";
    }
    std::cout << "init model done\t";
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT, 0.01);
    std::cout << "set threshold done\t";
    model_init = 1;
  }
  cvtdl_handpose21_meta_ts hand_obj;
  memset(&hand_obj, 0, sizeof(cvtdl_handpose21_meta_ts));
  hand_obj.size = 1;
  hand_obj.info =
      (cvtdl_handpose21_meta_t *)malloc(sizeof(cvtdl_handpose21_meta_t) * hand_obj.size);
  hand_obj.height = p_frame->stVFrame.u32Height;
  hand_obj.width = p_frame->stVFrame.u32Width;
  for (uint32_t i = 0; i < hand_obj.size; i++) {
    hand_obj.info[i].bbox_x = 0;
    hand_obj.info[i].bbox_y = 0;
    hand_obj.info[i].bbox_w = p_frame->stVFrame.u32Width - 1;
    hand_obj.info[i].bbox_h = p_frame->stVFrame.u32Height - 1;
  }
  ret = CVI_TDL_HandKeypoint(tdl_handle, p_frame, &hand_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "keypoint hand failed:" << ret << std::endl;
  }

  std::cout << "keypoint hand success" << std::endl;
  // generate detection result
  std::stringstream ss;
  // hand_obj.size
  for (uint32_t i = 0; i < 21; i++) {
    ss << hand_obj.info[0].xn[i] << " " << hand_obj.info[0].yn[i] << "\n";
  }
  CVI_TDL_Free(&hand_obj);
  return ss.str();
}

std::string run_image_yolov8_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                       std::string model_name, std::string image_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "init model done\t";
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov8 param \n");
    CVI_S32 ret =
        CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov8 preprocess parameters %#x\n", ret);
    }

    // setup yolo algorithm preprocess

    cvtdl_det_algo_param_t yolov8_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
    yolov8_param.cls = 1;

    printf("setup yolov8 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                        yolov8_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov8 algorithm parameters %#x\n", ret);
    }

    // set theshold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);

    printf("yolov8 algorithm parameters setup success!\n");
    std::cout << "set threshold done\t";
    model_init = 1;
    std::cout << "to init hand keypoint model\t";
    std::string str_person_model = g_model_root + std::string("/") + model_name;
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                            str_person_model.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_person_model << std::endl;
      return "";
    }
  }
  cvtdl_object_t hand_obj = {0};
  memset(&hand_obj, 0, sizeof(cvtdl_object_t));

  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, &hand_obj);
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

int main(int argc, char *argv[]) {
  printf("222222\n");
  g_model_root = std::string(argv[1]);
  std::string image_root(argv[2]);
  std::string image_list(argv[3]);
  std::string dst_root(argv[4]);
  std::string process_flag(argv[5]);
  std::string model_name(argv[6]);
  int starti = 0;
  if (argc > 7) starti = atoi(argv[7]);

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
  printf("1111111111\n");
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (image_root.size() == 0) {
    std::cout << ", imageroot empty\n";
    return -1;
  }
  std::map<std::string, std::function<std::string(VIDEO_FRAME_INFO_S *, cvitdl_handle_t,
                                                  std::string, std::string)>>
      process_funcs = {
          {"detect", run_image_hand_detection},
          {"classify", run_image_hand_classification},
          {"pet", run_image_pet_detection},
          {"vehicle", run_image_vehicle_detection},
          {"meeting", run_image_face_hand_person_detection},
          {"scrfd", run_image_scrfd_detection},
          {"pedestrian", run_image_person_detection},
          {"handkeypoint", run_image_hand_keypoint},
          {"coco", run_image_object_detection},
          {"yolo", run_image_yolov8_detection},
      };
  if (process_funcs.count(process_flag) == 0) {
    std::cout << "error flag:" << process_flag << std::endl;
    return -1;
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  printf("222222\n");
  for (size_t i = starti; i < image_files.size(); i++) {
    printf("33333333333\n");
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
    std::string str_res;
    std::string image_name = image_files[i];

    str_res = process_funcs[process_flag](&fdFrame, tdl_handle, model_name, image_name);
    std::cout << "process_funcs done\t";
    // std::cout << "str_res.size():" << str_res.size() << std::endl;

    std::cout << "writing file:" << dstf << std::endl;
    FILE *fp = fopen(dstf.c_str(), "w");
    fwrite(str_res.c_str(), str_res.size(), 1, fp);
    fclose(fp);

    std::cout << "write results done\t";
    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
    std::cout << "CVI_TDL_ReleaseImage done\t" << std::endl;
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
