#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "sys_utils.h"
#include "cvi_kit.h"
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

  char *text_list = argv[2];
  cvtdl_clip_feature clip_feature_text;
  char *encoderFile = "./encoder.txt";
  char *bpeFile = "./bpe_simple_vocab_16e6.txt";

  printf("to read file_list: %s\n", text_list);
  int text_list_size = count_file_lines(text_list);
  if (text_list_size == 0) {
    printf("file_list empty\n");
    return -1;
  }
  int32_t **tokens = (int32_t **)malloc(text_list_size * sizeof(int32_t *));
  ret = CVI_TDL_Set_TextPreprocess(encoderFile, bpeFile, text_list, tokens, text_list_size);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Set_TextPreprocess\n");
    return 0;
  }

  float **text_features = (float **)malloc(text_list_size * sizeof(float *));
  for (int i = 0; i < text_list_size; i++) {
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

    text_features[i] = (float *)malloc(clip_feature_text.feature_dim * sizeof(float));
    for (int y = 0; y < clip_feature_text.feature_dim; ++y) {
      text_features[i][y] = clip_feature_text.out_feature[y];
    }
    CVI_TDL_Free(&clip_feature_text);
  }

  for (int i = 0; i < text_list_size; i++) {
    free(tokens[i]);
  }
  free(tokens);

  for (size_t i = 0; i < text_list_size; i++) {
    for (int j = 0; j < 512; j++) {
      printf("%f ", text_features[i][j]);
    }
    printf("\n");
  }

  for (size_t i = 0; i < text_list_size; i++) {
    free(text_features[i]);
  }
  free(text_features);

  printf("after free:\n");

  CVI_TDL_DestroyHandle(tdl_handle);

  return CVI_SUCCESS;
}
