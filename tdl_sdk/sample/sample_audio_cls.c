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

#define AUDIOFORMATSIZE 2
// #define SECOND 3
#define CVI_AUDIO_BLOCK_MODE -1
#define PERIOD_SIZE 640
// #define SAMPLE_RATE 16000
// #define FRAME_SIZE SAMPLE_RATE *AUDIOFORMATSIZE *SECOND  // PCM_FORMAT_S16_LE (2bytes) 3 seconds

int test_binary_short_audio_data(char* strf, CVI_U8* p_buffer, cvitdl_handle_t tdl_handle,
                                 int sample_rate, int seconds) {
  VIDEO_FRAME_INFO_S Frame;
  int frame_size = sample_rate * AUDIOFORMATSIZE * seconds;
  Frame.stVFrame.pu8VirAddr[0] = p_buffer;  // Global buffer
  Frame.stVFrame.u32Height = 1;
  Frame.stVFrame.u32Width = frame_size;
  if (!read_binary_file(strf, p_buffer, frame_size)) {
    printf("read file failed\n");
    return -1;
  }
  int index = -1;
  int ret = CVI_TDL_SoundClassification(tdl_handle, &Frame, &index);
  if (ret != 0) {
    printf("sound classification failed\n");
    return -1;
  }
  return index;
}

int main(int argc, char* argv[]) {
  int sample_rate = atoi(argv[3]);
  int seconds = atoi(argv[4]);
  float threshold = atof(argv[5]);
  int fix = atof(argv[6]);
  int frame_size = sample_rate * AUDIOFORMATSIZE * seconds;
  CVI_U8 buffer[frame_size];  // 3 seconds

  cvitdl_handle_t tdl_handle = NULL;
  CVI_S32 ret = CVI_TDL_CreateHandle3(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  if (fix) {
    cvitdl_sound_param audio_param = CVI_TDL_GetSoundClassificationParam(
        tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
    audio_param.hop_len = 128;
    audio_param.fix = true;
    printf("setup audio algorithm parameters \n");
    ret = CVI_TDL_SetSoundClassificationParam(
        tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, audio_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set audio algorithm parameters %#x\n", ret);
      return ret;
    }
  }

  char* modelf = argv[1];

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, modelf);
  if (ret != CVI_SUCCESS) {
    printf("open modelfile failed %#x!\n", ret);
    return ret;
  }

  ret =
      CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, threshold);
  if (ret != CVI_SUCCESS) {
    printf("set threshold failed %#x!\n", ret);
    return ret;
  }

  printf("model opened: %s , set threshold: %s\n", modelf, threshold);
  if (argc == 9) {
    char* str_root_dir = argv[2];
    char* str_list_file = argv[7];
    char* str_res_file = argv[8];
    // std::vector<std::string> strfiles = read_file_lines(str_list_file);
    int line_count = 0;
    char** strfiles = read_file_lines(str_list_file, &line_count);
    FILE* fp = fopen(str_res_file, "w");
    int correct;
    int total;
    float accuracy;

    size_t num_total = line_count;
    for (size_t i = 0; i < num_total; i++) {
      printf("process:%zu/%zu,   file:%s\t", i, num_total, strfiles[i]);
      char strf[1024];
      snprintf(strf, sizeof(strf), "%s/%s", str_root_dir, strfiles[i]);

      int cls = test_binary_short_audio_data(strf, buffer, tdl_handle, sample_rate, seconds);
      char str_res[2048];
      snprintf(str_res, sizeof(str_res), "%s %d\n", strf, cls);
      fwrite(str_res, strlen(str_res), 1, fp);
      if (cls == -1) continue;

      char strlabel[50];
      snprintf(strlabel, sizeof(strlabel), "/%d/", cls);

      if (strstr(strf, strlabel) != NULL) {
        correct++;
      }

      total += 1;

      accuracy = (float)correct / total;
      printf("accuracy: %s\n", accuracy);
    }
    fclose(fp);

    printf("accuracy: %s\n", accuracy);

  } else {
    int cls = test_binary_short_audio_data(argv[2], buffer, tdl_handle, sample_rate, seconds);
    printf("result: %s\n", cls);
  }

  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}

/*
how to use:

for dataset:

./test_audio_cls
/tmp/yzx/infer/sound/models/shiyun_v5_int8_cv181x.cvimodel \           #model path
/tmp/yzx/infer/sound/data/nihaoshiyun/5m_normsound                     #data dir
8000  \                                                                #sample rate  [8000 | 16000]
2     \                                                                #time (s)
0.5   \                                                                #threshold    0 - 1.0
1     \                                                                #fix (0 for babycry, 1 for
other) /tmp/yzx/infer/sound/data/nihaoshiyun/5m_normsound/files_list.txt \    # data relative path
/tmp/yzx/infer/sound/output/shiyun_v5_5m_normsound.txt                 # result path


for single data:

./test_audio_cls \
/tmp/yzx/infer/sound/models/shiyun_v5_int8_cv181x.cvimodel  \ #model path
/tmp/yzx/infer/sound/data/nihaoshiyun/test_normsound/3/collection_guanbipingmu/0_23_3_2.bin \ #data
8000  \                                                                #sample rate  [8000 | 16000]
2     \                                                                #time (s)
0.5   \                                                                #threshold    0 - 1.0
1     \                                                                #fix (0 for babycry, 1 for
other)
*/