#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.h"

cvitdl_handle_t tdl_handle = NULL;
static CVI_S32 vpssgrp_width = 1920;
static CVI_S32 vpssgrp_height = 1080;

int main(int argc, char* argv[]) {
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
  printf("****input data type****\n%d\n", m_input_data_type);
  if (ret != CVI_SUCCESS) {
    printf("get model input data type failed with %#x!\n", ret);
    return ret;
  }

  int line_count = 0;
  char** file_list = read_file_lines(argv[2], &line_count);

  if (line_count == 0) {
    printf("file_list empty\n");
    return -1;
  }

  cvtdl_clip_feature clip_feature;

  // opencv_params ={width,height,mean[3],std[3],interpolationMethod,rgbFormat}
  cvtdl_opencv_params opencv_params = {
      224, 224, {0.40821073, 0.4578275, 0.48145466}, {0.27577711, 0.26130258, 0.26862954}, 1, 0};

  FILE* outfile = fopen("a2_image_output_device.txt", "w");
  if (!outfile) {
    printf("Failed to open output file.\n");
    return -1;
  }
  for (int i = 0; i < line_count; i++) {
    char* input_image_path = file_list[i];
    VIDEO_FRAME_INFO_S rgb_frame;

    char* last_slash = strrchr(input_image_path, '/');
    char* last_dot = strrchr(input_image_path, '.');
    char pic_name[256];
    strncpy(pic_name, last_slash + 1, last_dot - last_slash - 1);
    pic_name[last_dot - last_slash - 1] = '\0';

    printf("number of img:%d; last of imgname:%s\n", i, pic_name);
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
      fprintf(outfile, "%f", clip_feature.out_feature[y]);
      if (y < clip_feature.feature_dim - 1) {
        fprintf(outfile, " ");
      }
    }
    fprintf(outfile, "\n");
    free(clip_feature.out_feature);
    printf("after free:\n");
    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
    CVI_TDL_Destroy_ImageProcessor(img_handle);
  }
  fclose(outfile);
  CVI_TDL_DestroyHandle(tdl_handle);

  for (int i = 0; i < line_count; i++) {
    free(file_list[i]);
  }
  free(file_list);

  return CVI_SUCCESS;
}
