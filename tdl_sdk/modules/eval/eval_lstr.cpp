#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

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

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LSTR, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_Set_LSTR_ExportFeature(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LSTR, 1);
  if (ret != CVI_SUCCESS) {
    printf("set export feature failed with %#x!\n", ret);
    return ret;
  }  

  std::string image_dir(argv[2]);
  std::string image_list(argv[3]);
  std::string save_dir(argv[4]);
  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }
  if (image_dir.at(image_dir.size() - 1) != '/') {
    image_dir = image_dir + std::string("/");
  }
  if (save_dir.at(save_dir.size() - 1) != '/') {
    save_dir = save_dir + std::string("/");
  }

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  for (size_t i = 0; i < file_list.size(); i++) {

    std::string input_image_path = image_dir + file_list[i];
    VIDEO_FRAME_INFO_S bg;

    std::cout << "processing :" << i + 1 << "/" << file_list.size() << "\t"
              << file_list[i] << std::endl;

    ret = CVI_TDL_ReadImage(img_handle, input_image_path.c_str(), &bg,
                            PIXEL_FORMAT_RGB_888_PLANAR);
    cvtdl_object_t obj_meta = {0};

    cvtdl_lane_t lane_meta = {0};
    CVI_TDL_LSTR_Det(tdl_handle, &bg, &lane_meta);

    std::string save_path = save_dir + replace_file_ext(file_list[i], "txt");

    std::ofstream outFile(save_path);
    if (!outFile) {
        std::cout << "Error: Could not open file " << save_path << " for writing." << std::endl;
        return 1;
    }
    for (size_t j = 0; j < 56; ++j) {
        outFile << lane_meta.feature[j] << " ";
    }
    outFile << std::endl;

    for (size_t j = 0; j < 14; ++j) {
        outFile << lane_meta.feature[56 + j] << " ";
    }

    outFile.close();

    CVI_TDL_Free(&lane_meta);

    CVI_TDL_ReleaseImage(img_handle, &bg);

  }

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
