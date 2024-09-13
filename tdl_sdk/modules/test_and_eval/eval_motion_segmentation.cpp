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

#include <signal.h>
#include <unistd.h>
#define MAX_PTRS 10

void *ptrs[MAX_PTRS] = {nullptr};  // 用于存储指针的全局数组
int ptr_count = 0;                 // 当前存储的指针数量

void register_ptr(void *ptr) {
  if (ptr_count < MAX_PTRS) {
    ptrs[ptr_count++] = ptr;
  }
}

void unregister_ptr(void *ptr) {
  for (int i = 0; i < ptr_count; i++) {
    if (ptrs[i] == ptr) {
      ptrs[i] = ptrs[--ptr_count];  // 移除指针并减少计数
      break;
    }
  }
}

void signal_handler(int signal) {
  if (signal == SIGSEGV) {
    printf("Caught segmentation fault, cleaning up...\n");
    for (int i = 0; i < ptr_count; i++) {
      free(ptrs[i]);
    }
    _exit(1);
  }
}
int run_motion_segmentation(VIDEO_FRAME_INFO_S *input0, VIDEO_FRAME_INFO_S *input1,
                            cvtdl_seg_logits_t *output, cvitdl_handle_t tdl_handle,
                            std::string model_path) {
  static int model_init = 0;
  CVI_S32 ret;
  if (model_init == 0) {
    std::cout << "Initialize model ..." << std::endl;

    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOTIONSEGMENTATION,
                            model_path.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "Open model failed: " << model_path << std::endl;
      return 0;
    }
    std::cout << "Initialize model success" << std::endl;
    model_init = 1;
  }

  ret = CVI_TDL_MotionSegmentation(tdl_handle, input0, input1, output);
  if (ret != CVI_SUCCESS) {
    std::cout << "Motion segmentation failed: " << ret << std::endl;
  }
  std::cout << "Motion segmentation success" << std::endl;
  return 1;
}

int main(int argc, char *argv[]) {
  std::string model_path(argv[1]);
  std::string image_root(argv[2]);
  std::string image_list(argv[3]);
  std::string dst_root(argv[4]);

  if (image_root.at(image_root.size() - 1) != '/') {
    image_root = image_root + std::string("/");
  }
  if (dst_root.at(dst_root.size() - 1) != '/') {
    dst_root = dst_root + std::string("/");
  };
  // Init VB pool size.
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 4,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 4);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }
  // getchar();
  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  std::cout << "\nRead image list ..." << std::endl;
  std::vector<std::string> image_files = read_file_lines(image_list);
  if (image_root.size() == 0) {
    std::cout << "Empty image list" << std::endl;
    return -1;
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (size_t i = 0; i < image_files.size() / 2; i++) {
    VIDEO_FRAME_INFO_S input0, input1;
    cvtdl_seg_logits_t output = {0};
    std::cout << "processing " << i << "/" << image_files.size() / 2
              << " sample: " << image_files[2 * i] << std::endl;
    std::string strf = image_root + image_files[2 * i];
    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &input0, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      std::cout << "reading " << 2 * i << "th image  failed: " << strf << std::endl;
      continue;
    }

    strf = image_root + image_files[2 * i + 1];
    ret = CVI_TDL_ReadImage(img_handle, strf.c_str(), &input1, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      std::cout << "reading " << 2 * i + 1 << "th image  failed: " << strf << std::endl;
      continue;
    }

    int infer_res = run_motion_segmentation(&input0, &input1, &output, tdl_handle, model_path);

    std::cout << "processed " << i << "/" << image_files.size() / 2 << " sample success"
              << std::endl;

    if (infer_res > 0) {
      std::string dstf = dst_root + replace_file_ext(image_files[2 * i + 1], "txt");
      std::cout << "writing the inference results: " << dstf << std::endl;
      std::cout << output.b << " " << output.c << " " << output.h << " " << output.w << std::endl;
      std::ofstream output_file(dstf.c_str());
      output_file << output.b << " " << output.c << " " << output.h << " " << output.w << std::endl;
      output_file << output.qscale << std::endl;

      if (output.is_int) {
        for (int i = 0; i < output.b * output.c * output.h * output.w; i++) {
          output_file << (int)output.int_logits[i] << " ";
        }
      } else {
        for (int i = 0; i < output.b * output.c * output.h * output.w; i++) {
          output_file << output.float_logits[i] << " ";
        }
      }
      output_file.close();
      std::cout << "write results done" << std::endl;
    }
    CVI_TDL_ReleaseImage(img_handle, &input0);
    CVI_TDL_ReleaseImage(img_handle, &input1);
    std::cout << "CVI_TDL_ReleaseImage done\t" << std::endl;
    CVI_TDL_Free(&output);
    std::cout << "FREE cvtdl_seg_logits_t done\t" << std::endl;
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
