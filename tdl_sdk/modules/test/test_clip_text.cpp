#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "mapi.hpp"
#include "sys_utils.hpp"
#include "utils/token.hpp"

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

  CVI_S32 ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("Set model retinaface failed with %#x!\n", ret);
    return ret;
  }

  std::string text_list(argv[2]);
  cvtdl_clip_feature clip_feature_text;
  std::string encoderFile = "./encoder.txt";
  std::string bpeFile = "./bpe_simple_vocab_16e6.txt";

  std::cout << "to read file_list:" << text_list << std::endl;
  std::vector<std::string> text_file_list = read_file_lines(text_list);
  if (text_file_list.size() == 0) {
    std::cout << ", file_list empty\n";
    return -1;
  }
  int32_t** tokens = (int32_t**)malloc(text_file_list.size() * sizeof(int32_t*));
  ret = CVI_TDL_Set_TextPreprocess(encoderFile.c_str(), bpeFile.c_str(), text_list.c_str(), tokens,
                                   text_file_list.size());
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Set_TextPreprocess\n");
    return 0;
  }

  float** text_features = new float*[text_file_list.size()];
  for (int i = 0; i < text_file_list.size(); i++) {
    CVI_U8 buffer[77 * sizeof(int32_t)];
    memcpy(buffer, tokens[i], sizeof(int32_t) * 77);
    VIDEO_FRAME_INFO_S Frame;
    Frame.stVFrame.pu8VirAddr[0] = buffer;
    Frame.stVFrame.u32Height = 1;
    Frame.stVFrame.u32Width = 77;

    ret = CVI_TDL_Clip_Text_Feature(tdl_handle, &Frame, &clip_feature_text);
    if (ret != CVI_SUCCESS) {
      printf("CVI_TDL_OpenClip_Text_Feature\n");
      return 0;
    }

    text_features[i] = new float[clip_feature_text.feature_dim];
    for (int y = 0; y < clip_feature_text.feature_dim; ++y) {
      text_features[i][y] = clip_feature_text.out_feature[y];
    }
    CVI_TDL_Free(&clip_feature_text);
  }

  for (int i = 0; i < text_file_list.size(); i++) {
    free(tokens[i]);
  }
  free(tokens);

  for (int i = 0; i < text_file_list.size(); i++) {
    for (int j = 0; i < 512; j++) {
      std::cout << text_features[i][j] << " ";
    }
    std::cout << std::endl;
  }
  delete[] text_features;
  std::cout << "after free:" << std::endl;

  CVI_TDL_DestroyHandle(tdl_handle);

  return CVI_SUCCESS;
}
