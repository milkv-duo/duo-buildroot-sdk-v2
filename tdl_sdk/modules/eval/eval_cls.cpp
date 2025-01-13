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

typedef int (*CLSInferenceFunc)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *,cvtdl_class_meta_t *);

typedef struct {
  CLSInferenceFunc inference_func;
  cvitdl_handle_t stTDLHandle;
} EVAL_TDL_TDL_THREAD_ARG_S;

CVI_S32 get_cls_model_info(std::string model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                          CLSInferenceFunc *inference_func) {
  CVI_S32 ret = CVI_SUCCESS;
  if (strcmp(model_name.c_str(), "image_cls") == 0) {
    *model_index = CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION;
    *inference_func = CVI_TDL_Image_Classification;
  } else {
    ret = CVI_TDL_FAILURE;
  }
  return ret;
}

void bench_mark_all(std::string bench_path, std::string image_root, std::string res_path, int class_num,
                    EVAL_TDL_TDL_THREAD_ARG_S *pstTDLArgs) {
  std::fstream file(bench_path);
  if (!file.is_open()) {
    printf("can not open bench path %s\n", bench_path.c_str());
    return;
  }
  printf("open bench path %s success!\n", bench_path.c_str());
  std::string line;
  std::stringstream res_ss;
  int cnt = 0;

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  while (getline(file, line)) {
    if (!line.empty()) {
      stringstream ss(line);
      std::string image_name;
      while (ss >> image_name) {
        cvtdl_class_meta_t meta = {0};
        VIDEO_FRAME_INFO_S fdFrame;
        if (++cnt % 100 == 0) {
          cout << "processing idx: " << cnt << endl;
        }

        auto name = image_root + image_name;
        CVI_S32 ret =
            CVI_TDL_ReadImage(img_handle, name.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888_PLANAR);
        ret = pstTDLArgs->inference_func(pstTDLArgs->stTDLHandle, &fdFrame,
                                        &meta);
        if (ret != CVI_SUCCESS) {
          CVI_TDL_Free(&meta);
          CVI_TDL_ReleaseImage(img_handle, &fdFrame);
          break;
        }

        res_ss << image_name << " ";
        for (int i = 0; i < class_num; i++) {
          res_ss << " " << meta.cls[i];
        }
        res_ss << "\n";

        CVI_TDL_Free(&meta);
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
  std::string model_index = argv[2];
  int class_num = std::stoi(argv[3]);
  std::string bench_path = argv[4];
  std::string image_root = argv[5];
  std::string res_path = argv[6];

  CLSInferenceFunc inference_func;
  CVI_TDL_SUPPORTED_MODEL_E enOdModelId;
  if (get_cls_model_info(model_index, &enOdModelId, &inference_func) == CVI_TDL_FAILURE) {
    printf("unsupported model: %s\n", model_index);
    return -1;
  }
  ret = CVI_TDL_OpenModel(tdl_handle, enOdModelId, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x %s!\n", ret, model_path.c_str());
    return ret;
  }

  std::cout << "model opened:" << model_path << std::endl;
  EVAL_TDL_TDL_THREAD_ARG_S pstTDLArgs = {inference_func,tdl_handle};
  bench_mark_all(bench_path, image_root, res_path, class_num, &pstTDLArgs);

  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}