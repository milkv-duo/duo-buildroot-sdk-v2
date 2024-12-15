#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "mapi.hpp"
#include "sys_utils.hpp"

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }
cvitdl_handle_t tdl_handle = NULL;
static CVI_S32 vpssgrp_width = 1920;
static CVI_S32 vpssgrp_height = 1080;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf(
        "Usage: %s <clip model path> <input image directory list.txt> <output result "
        "directory/>.\n",
        argv[0]);
    printf("clip model path: Path to clip bmodel.\n");
    printf("Input image directory: Directory containing input images for clip.\n");
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;
  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888, 3);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("Set model retinaface failed with %#x!\n", ret);
    return ret;
  }

  int m_input_data_type = -1;
  ret =
      CVI_TDL_GetModelInputTpye(tdl_handle, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, &m_input_data_type);
  std::cout << "****input data type****" << std::endl;
  std::cout << m_input_data_type << std::endl;
  if (ret != CVI_SUCCESS) {
    printf("get model input data type failed with %#x!\n", ret);
    return ret;
  }
  std::string image_list(argv[2]);

  std::cout << "to read file_list:" << image_list << std::endl;
  std::vector<std::string> file_list = read_file_lines(image_list);
  if (file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }

  std::string input_image_path;
  cvtdl_clip_feature clip_feature;

  // opencv_params ={width,height,mean[3],std[3],interpolationMethod,rgbFormat}
  cvtdl_opencv_params opencv_params = {
      224, 224, {0.40821073, 0.4578275, 0.48145466}, {0.27577711, 0.26130258, 0.26862954}, 1, 0};

  std::ofstream outfile("a2_image_output_device.txt");

  std::cout << file_list.size() << std::endl;
  for (size_t i = 0; i < file_list.size(); i++) {
    input_image_path = file_list[i];
    VIDEO_FRAME_INFO_S rgb_frame;

    size_t line_position = input_image_path.find_last_of('/');
    size_t dot_position = input_image_path.find_last_of('.');
    string pic_name =
        input_image_path.substr(line_position + 1, dot_position - line_position - 1).c_str();
    std::cout << "number of img:" << i << ";last of imgname:" << pic_name << std::endl;
    imgprocess_t img_handle;
    CVI_TDL_Create_ImageProcessor(&img_handle);
    if (m_input_data_type == 2) {
      ret = CVI_TDL_ReadImage_CenrerCrop_Resize(img_handle, input_image_path.c_str(), &rgb_frame,
                                                PIXEL_FORMAT_RGB_888_PLANAR, 224, 224);
    } else if (m_input_data_type == 0) {
      ret = CVI_TDL_OpenCV_ReadImage_Float(img_handle, input_image_path.c_str(), &rgb_frame,
                                           opencv_params);
    }

    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }

    ret = CVI_TDL_Clip_Image_Feature(tdl_handle, &rgb_frame, &clip_feature);
    if (ret != CVI_SUCCESS) {
      printf("Failed to CVI_TDL_Clip_Feature\n");
      return 0;
    }

    for (int y = 0; y < clip_feature.feature_dim; ++y) {
      outfile << clip_feature.out_feature[y];
      if (y < clip_feature.feature_dim - 1) {
        outfile << " ";
      }
    }
    outfile << "\n";
    free(clip_feature.out_feature);

    std::cout << "after free:" << std::endl;

    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
  }
  outfile.close();
  CVI_TDL_DestroyHandle(tdl_handle);

  return CVI_SUCCESS;
}
