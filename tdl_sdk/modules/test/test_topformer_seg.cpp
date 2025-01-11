#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <fstream>
#include <functional>
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "cvi_kit.h"
#include "sys_utils.hpp"

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }
cvitdl_handle_t tdl_handle = NULL;

static CVI_S32 vpssgrp_width = 1920;
static CVI_S32 vpssgrp_height = 1080;

bool CompareFileNames(std::string a, std::string b) { return a < b; }

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: %s <Topformer model path> <input image directory> <output result directory>.\n",
           argv[0]);
    printf("Topformer model path: Path to Topformer cvimodel.\n");
    printf("Input image directory: Directory containing input images for segmentation.\n");
    printf("Atention! Output result directory: Directory to save segmented result images.\n");
    return CVI_FAILURE;
  }

  CVI_S32 ret = CVI_SUCCESS;
  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create ai handle failed with %#x!\n", ret);
    return ret;
  }
  // DOWN_RATO
  int down_rato = atoi(argv[3]);
  ret =
      CVI_TDL_Set_Segmentation_DownRato(tdl_handle, CVI_TDL_SUPPORTED_MODEL_TOPFORMER_SEG, down_rato);
  if (ret != CVI_SUCCESS) {
    printf("Set topformer downrato file %#x!\n", ret);
    return ret;
  }
  // MODEL_DIR
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_TOPFORMER_SEG, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("Set model topformer failed with %#x!\n", ret);
    return ret;
  }
  // IMAGE_LIST
  std::string image_list(argv[2]);

  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }

  std::sort(file_list.begin(), file_list.end(), CompareFileNames);

  std::string input_image_path;
  imgprocess_t img_handle;
  int outH, outW;
  CVI_TDL_Create_ImageProcessor(&img_handle);
  
  for (size_t i = 0; i < file_list.size(); i++) {
    input_image_path = file_list[i];
    VIDEO_FRAME_INFO_S rgb_frame;
    cvtdl_seg_t seg_ann;
    size_t line_position = input_image_path.find_last_of('/');
    size_t dot_position = input_image_path.find_last_of('.');
    strcpy(seg_ann.img_name,
           input_image_path.substr(line_position + 1, dot_position - line_position - 1).c_str());
    std::cout << "number of img:" << i << ";last of imgname:" << seg_ann.img_name << std::endl;
    ret = CVI_TDL_ReadImage(img_handle,input_image_path.c_str(), &rgb_frame, PIXEL_FORMAT_RGB_888_PLANAR);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }
    outH = std::ceil(static_cast<float>(rgb_frame.stVFrame.u32Height) / down_rato);
    outW = std::ceil(static_cast<float>(rgb_frame.stVFrame.u32Width) / down_rato);

    if (ret != CVI_SUCCESS) {
      printf("Failed to read image: %s\n", input_image_path.c_str());
    } else {
      CVI_TDL_Topformer_Seg(tdl_handle, &rgb_frame, &seg_ann);
      for (int x = 0; x < outH; ++x) {
        for (int y = 0; y < outW; ++y) {
          std::cout << static_cast<int>(seg_ann.class_id[x * outW + y]) << " ";
        }
        std::cout << std::endl;
      }
    }
    CVI_TDL_ReleaseImage(img_handle,&rgb_frame);
    CVI_TDL_Free(&seg_ann);
  }
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  return CVI_SUCCESS;
}
