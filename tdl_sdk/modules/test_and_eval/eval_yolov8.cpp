#define _GNU_SOURCE
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

void bench_mark_all(std::string bench_path, std::string image_root, std::string res_path,
                    cvitdl_handle_t tdl_handle) {
  std::fstream file(bench_path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  while (getline(file, line)) {
    if (!line.empty()) {
      stringstream ss(line);
      std::string image_name;
      while (ss >> image_name) {
        cvtdl_object_t obj_meta = {0};
        VIDEO_FRAME_INFO_S fdFrame;
        auto img_path = image_root + image_name;
        CVI_TDL_ReadImage(img_handle, img_path.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
        CVI_S32 ret = CVI_TDL_Detection(tdl_handle, &fdFrame,
                                        CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, &obj_meta);
        if (ret != CVI_SUCCESS) {
          CVI_TDL_Free(&obj_meta);
          CVI_TDL_ReleaseImage(img_handle, &fdFrame);
          break;
        }
        std::stringstream res_ss;

        for (uint32_t i = 0; i < obj_meta.size; i++) {
          res_ss << obj_meta.info[i].classes << " " << obj_meta.info[i].bbox.x1 << " "
                 << obj_meta.info[i].bbox.y1 << " " << obj_meta.info[i].bbox.x2 << " "
                 << obj_meta.info[i].bbox.y2 << " " << obj_meta.info[i].bbox.score << "\n";
        }

        size_t pos = image_name.find_last_of("/");
        size_t start_idx = pos == std::string::npos ? 0 : pos + 1;
        std::string save_path =
            res_path + image_name.substr(start_idx, image_name.length() - 4 - (pos + 1)) + ".txt";
        FILE* fp = fopen(save_path.c_str(), "w");
        fwrite(res_ss.str().c_str(), res_ss.str().size(), 1, fp);
        fclose(fp);

        CVI_TDL_Free(&obj_meta);
        CVI_TDL_ReleaseImage(img_handle, &fdFrame);
        break;
      }
    }
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  std::cout << "write done!" << std::endl;
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

  std::string model_path = argv[1];
  std::string bench_path = argv[2];
  std::string image_root = argv[3];
  std::string res_path = argv[4];

  float conf_threshold = 0.5;
  float nms_threshold = 0.5;
  if (argc > 5) {
    conf_threshold = std::stof(argv[5]);
  }

  if (argc > 6) {
    nms_threshold = std::stof(argv[6]);
  }

  if (argc > 7) {
    cvtdl_det_algo_param_t yolov8_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
    yolov8_param.cls = atoi(argv[7]);

    printf("setup yolov8 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                        yolov8_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov8 algorithm parameters %#x\n", ret);
      return ret;
    }
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, model_path.c_str());
  // set conf threshold and nms threshold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, conf_threshold);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, nms_threshold);

  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x %s!\n", ret, model_path.c_str());
    return ret;
  }

  std::cout << "model opened:" << model_path << std::endl;

  bench_mark_all(bench_path, image_root, res_path, tdl_handle);

  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}