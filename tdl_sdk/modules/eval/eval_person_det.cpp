
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
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

std::string run_image_person_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                       std::string model_name) {
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
    ss << person_obj.info[i].classes + 1 << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << " " << box.score << "\n";
  }
  CVI_TDL_Free(&person_obj);
  return ss.str();
}

std::string run_image_person_pets_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle,
                                            std::string model_name) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "to init Person pets model\t";
    std::string str_model = g_model_root + std::string("/") + model_name;
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS,
                            str_model.c_str());
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS, 0.01);
    model_init = 1;
  }
  cvtdl_object_t person_pets_obj;
  memset(&person_pets_obj, 0, sizeof(cvtdl_object_t));
  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS,
                          &person_pets_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect person pets failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;
  // cat: (17 or 2); dog: (18 or 3)
  std::map<int, int> label_map = {{1, 1}, {17, 2}, {18, 3}};
  for (uint32_t i = 0; i < person_pets_obj.size; i++) {
    cvtdl_bbox_t box = person_pets_obj.info[i].bbox;
    int label = label_map[person_pets_obj.info[i].classes + 1];
    ss << label << " " << box.score << " " << box.x1 << " " << box.y1 << " " << box.x2 << " "
       << box.y2 << "\n";
  }
  CVI_TDL_Free(&person_pets_obj);
  return ss.str();
}

int main(int argc, char *argv[]) {
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
  };
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
      process_funcs = {
          // {"person_vehicle", run_image_person_vehicle_detection},
          {"pets", run_image_person_pets_detection},
          {"person", run_image_person_detection},
          {"baby", run_image_baby_detection},
      };
  if (process_funcs.count(process_flag) == 0) {
    std::cout << "error flag:" << process_flag << std::endl;
    return -1;
  }

  std::cout << "start i:" << starti << "\t";
  std::cout << image_files.size() << "\t";
  decltype(image_files.size()) i = starti;
  std::cout << "processing :" << i << "/" << image_files.size() << "\t" << image_files[i] << "\t";
  std::cout << (i < image_files.size()) << "\t";
  std::cout << "init i done" << std::endl;

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (i = starti; i < image_files.size(); i++) {
    std::cout << "processing :" << i << "/" << image_files.size() << "\t" << image_files[i];
    std::string strf = image_root + image_files[i];
    std::string dstf = dst_root + replace_file_ext(image_files[i], "txt");
    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
    std::cout << "CVI_TDL_ReadImage done\t";
    if (ret != CVI_SUCCESS) {
      std::cout << "Convert to video frame failed with:" << ret << ",file:" << strf << std::endl;

      continue;
    }
    std::string str_res = process_funcs[process_flag](&fdFrame, tdl_handle, model_name);
    std::cout << "process_funcs done\t";
    std::cout << "str_res.size():" << str_res.size() << std::endl;
    if (str_res.size() > 0) {
      std::cout << "writing file" << std::endl;
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
