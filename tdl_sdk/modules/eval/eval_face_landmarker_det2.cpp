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

void bench_mark_all(std::string bench_path, std::string image_root, std::string res_path,
                    cvitdl_handle_t tdl_handle) {
  std::fstream file(bench_path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  std::stringstream res_ss;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  while (getline(file, line)) {
    if (!line.empty()) {
      stringstream ss(line);
      std::string image_name;
      while (ss >> image_name) {
        cvtdl_face_t meta = {0};
        VIDEO_FRAME_INFO_S fdFrame;
        // cout << "get image name: " << image_root + image_name << endl;

        auto name = image_root + image_name;
        CVI_TDL_ReadImage(img_handle, name.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);

        int ret = CVI_TDL_FaceLandmarkerDet2(tdl_handle, &fdFrame, &meta);
        if (ret != CVI_SUCCESS) {
          CVI_TDL_Free(&meta);
          // CVI_VPSS_ReleaseChnFrame(0, 0, &fdFrame);
          CVI_TDL_ReleaseImage(img_handle, &fdFrame);
          break;
        }

        float score = meta.info[0].score;
        // float blur_score = meta.info[0].blurness;
        res_ss << image_name << " " << score;
        for (int i = 0; i < 5; i++) {
          res_ss << " " << meta.info[0].pts.x[i] << " " << meta.info[0].pts.y[i];
        }
        res_ss << "\n";

        CVI_TDL_Free(&meta);
        // CVI_VPSS_ReleaseChnFrame(0, 0, &fdFrame);
        CVI_TDL_ReleaseImage(img_handle, &fdFrame);
        break;
      }
    }
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  std::cout << "write results to file: " << res_path << std::endl;
  FILE* fp = fopen(res_path.c_str(), "w");
  fwrite(res_ss.str().c_str(), res_ss.str().size(), 1, fp);
  fclose(fp);
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

  ret =
      CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACELANDMARKERDET2, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x %s!\n", ret, model_path.c_str());
    return ret;
  }
  std::cout << "model opened:" << model_path << std::endl;

  bench_mark_all(bench_path, image_root, res_path, tdl_handle);

  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}