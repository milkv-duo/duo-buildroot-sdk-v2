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

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 2,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 2);
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

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  std::string image_list(argv[2]);
  // std::string dir_name(argv[3]);
  // std::string
  // dir_name="/tmp/zsy/sdk_package_cv1811h/install/soc_cv1811h_wevb_0007a_spinand/tpu_glibc_riscv64/cvitek_ai_sdk/bin/output/";

  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }
  // std::sort(file_list.begin(), file_list.end(), CompareFileNames);
  std::string input_image_path;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  std::ofstream outfile("all_images_output_info.txt");  // 在循环外打开文件
  if (!outfile) {
    std::cerr << "无法打开文件" << std::endl;
    return -1;
  }

  for (size_t i = 0; i < file_list.size(); i++) {
    input_image_path = file_list[i];
    VIDEO_FRAME_INFO_S rgb_frame;
    size_t line_position = input_image_path.find_last_of('/');
    size_t dot_position = input_image_path.find_last_of('.');
    std::string pic_name =
        input_image_path.substr(line_position + 1, dot_position - line_position - 1).c_str();
    std::cout << "number of img:" << i << ";last of imgname:" << pic_name << std::endl;

    ret = CVI_TDL_ReadImage(img_handle, input_image_path.c_str(), &rgb_frame,
                            PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }

    cvtdl_face_t faces = {0};
    CVI_TDL_FaceAttribute_cls(tdl_handle, &rgb_frame, &faces);

    outfile << pic_name;
    for (uint32_t i = 0; i < faces.size; i++) {
      outfile << ' ' << faces.info->gender_score << ' ' << faces.info->age << " "
              << faces.info->glass << " " << faces.info->mask_score;
    }
    outfile << "\n";
    CVI_TDL_Free(&faces);

    std::cout << "after free:" << std::endl;
    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
  }
  outfile.close();  // 关闭文件
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
