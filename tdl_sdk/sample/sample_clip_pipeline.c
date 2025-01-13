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

double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }

cvitdl_handle_t tdl_handle = NULL;
static CVI_S32 vpssgrp_width = 1920;
static CVI_S32 vpssgrp_height = 1080;

int main(int argc, char* argv[]) {
  if (argc != 7) {
    printf(
        "Usage: %s <clip model path> <input image directory list.txt> <output result "
        "directory/> <min th>.\n",
        argv[0]);
    printf("clip image model path: Path to clip image bmodel.\n");
    printf("Input image directory: Directory containing input images for clip.\n");
    printf("clip text model path: Path to clip text bmodel.\n");
    printf("Input text directory: Directory containing input text for clip.\n");
    printf("output text directory: Directory containing output class for clip.\n");
    printf("If top1 score < th, return -1 not in dataset, else return top1 class.\n");
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

  char* image_list = argv[2];
  int image_file_count = 0;
  char** image_file_list = read_file_lines(argv[2], &image_file_count);

  if (image_file_count == 0) {
    printf(", file_list empty\n");
    return -1;
  }

  char* input_image_path;
  cvtdl_clip_feature clip_feature_image;

  int rows_image = image_file_count;
  int cols_image = 512;

  float** image_features = (float**)malloc(image_file_count * sizeof(float*));
  for (size_t i = 0; i < image_file_count; i++) {
    input_image_path = image_file_list[i];

    VIDEO_FRAME_INFO_S rgb_frame;
    size_t line_position = strrchr(input_image_path, '/') - input_image_path;
    size_t dot_position = strrchr(input_image_path, '.') - input_image_path;
    char pic_name[256];
    strncpy(pic_name, input_image_path + line_position + 1, dot_position - line_position - 1);
    pic_name[dot_position - line_position - 1] = '\0';
    printf("number of img:%zu; last of imgname:%s\n", i, pic_name);
    imgprocess_t img_handle;
    CVI_TDL_Create_ImageProcessor(&img_handle);
    ret = CVI_TDL_ReadImage_CenrerCrop_Resize(img_handle, input_image_path, &rgb_frame,
                                              PIXEL_FORMAT_RGB_888_PLANAR, 224, 224);
    if (ret != CVI_SUCCESS) {
      printf("open img failed with %#x!\n", ret);
      return ret;
    }
    ret = CVI_TDL_Clip_Image_Feature(tdl_handle, &rgb_frame, &clip_feature_image);
    if (ret != CVI_SUCCESS) {
      printf("Failed to CVI_TDL_Clip_Feature\n");
      return 0;
    }
    image_features[i] = (float*)malloc(clip_feature_image.feature_dim * sizeof(float));
    for (int y = 0; y < clip_feature_image.feature_dim; ++y) {
      image_features[i][y] = clip_feature_image.out_feature[y];
    }

    CVI_TDL_Free(&clip_feature_image);
    printf("after free:\n");
    CVI_TDL_ReleaseImage(img_handle, &rgb_frame);
  }

  for (size_t i = 0; i < image_file_count; i++) {
    free(image_features[i]);
  }
  free(image_features);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT, argv[3]);
  if (ret != CVI_SUCCESS) {
    printf("Set model retinaface failed with %#x!\n", ret);
    return ret;
  }

  printf("to read file_list:%s\n", argv[4]);
  int text_file_count = 0;
  char** text_file_list = read_file_lines(argv[4], &text_file_count);

  if (text_file_count == 0) {
    printf(", file_list empty\n");
    return -1;
  }
  cvtdl_clip_feature clip_feature_text;

  printf("%d\n", text_file_count);

  char* encoderFile = "/mnt/sd/186ah_sdk/clip_dataset/encoder.txt";
  char* bpeFile = "/mnt/sd/186ah_sdk/clip_dataset/bpe_simple_vocab_16e6.txt";
  int32_t** tokens = (int32_t**)malloc(text_file_count * sizeof(int32_t*));
  ret = CVI_TDL_Set_TextPreprocess(encoderFile, bpeFile, argv[4], tokens, text_file_count);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_Set_TextPreprocess\n");
    return 0;
  }

  float** text_features = (float**)malloc(text_file_count * sizeof(float*));

  for (int i = 0; i < text_file_count; i++) {
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

    text_features[i] = (float*)malloc(clip_feature_text.feature_dim * sizeof(float));
    for (int y = 0; y < clip_feature_text.feature_dim; ++y) {
      text_features[i][y] = clip_feature_text.out_feature[y];
    }
    CVI_TDL_Free(&clip_feature_text);
  }

  for (int i = 0; i < text_file_count; i++) {
    free(tokens[i]);
  }
  free(tokens);

  float thres = atof(argv[6]);
  int function_id = 0;
  float** probs = (float**)malloc(sizeof(float*));
  probs[0] = (float*)malloc(sizeof(float));
  ret = CVI_TDL_Set_ClipPostprocess(text_features, text_file_count, image_features,
                                    image_file_count, probs);

  if (ret != 0) {
    printf("Postprocessing failed\n");
    return ret;
  }
  FILE* outfile = fopen(argv[5], "w");
  if (!outfile) {
    perror("Failed to open the file");
    return 1;
  }

  for (int i = 0; i < image_file_count; i++) {
    float max_prob = 0;
    int top1_id = -1;
    for (int j = 0; j < text_file_count; j++) {
      if (probs[i][j] > max_prob) {
        max_prob = probs[i][j];
        top1_id = j;
      }
    }
    if (max_prob < thres) {
      top1_id = -1;
    }
    fprintf(outfile, "%s:%d\n", image_file_list[i], top1_id);
  }

  fclose(outfile);
  for (int i = 0; i < image_file_count; i++) {
    free(probs[i]);
  }
  free(probs);

  for (int i = 0; i < text_file_count; i++) {
    free(text_features[i]);
  }
  free(text_features);

  CVI_TDL_DestroyHandle(tdl_handle);
  return CVI_SUCCESS;
}
