#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
#define AUDIOFORMATSIZE 2
// #define SECOND 3
#define CVI_AUDIO_BLOCK_MODE -1
#define PERIOD_SIZE 640
// #define SAMPLE_RATE 16000
// #define FRAME_SIZE SAMPLE_RATE *AUDIOFORMATSIZE *SECOND  // PCM_FORMAT_S16_LE (2bytes) 3 seconds

int test_binary_short_audio_data(const std::string &strf, CVI_U8 *p_buffer,
                                 cvitdl_handle_t tdl_handle, int sample_rate, int seconds) {
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

int main(int argc, char *argv[]) {
  int sample_rate = atoi(argv[3]);
  int seconds = atoi(argv[4]);
  float threshold = atof(argv[5]);
  int frame_size = sample_rate * AUDIOFORMATSIZE * seconds;
  CVI_U8 buffer[frame_size];  // 3 seconds

  cvitdl_handle_t tdl_handle = NULL;
  CVI_S32 ret = CVI_TDL_CreateHandle3(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_sound_param audio_param =
      CVI_TDL_GetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
  audio_param.hop_len = 128;
  audio_param.fix = true;
  printf("setup audio algorithm parameters \n");
  ret = CVI_TDL_SetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
                                            audio_param);
  if (ret != CVI_SUCCESS) {
    printf("Can not set audio algorithm parameters %#x\n", ret);
    return ret;
  }

  std::string modelf(argv[1]);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, modelf.c_str());
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
  std::cout << "model opened:" << modelf << ", set threshold:" << threshold << std::endl;
  if (argc == 8) {
    std::string str_root_dir = argv[2];
    std::string str_list_file = argv[6];
    std::string str_res_file = argv[7];
    std::vector<std::string> strfiles = read_file_lines(str_list_file);
    FILE *fp = fopen(str_res_file.c_str(), "w");
    int TP = 0;
    int FN = 0;
    int FP = 0;
    int TN = 0;
    int total;
    float accuracy;

    size_t num_total = strfiles.size();
    for (size_t i = 0; i < num_total; i++) {
      std::cout << "process:" << i << "/" << num_total << ",   file:" << strfiles[i] << "\t";
      std::string strf = str_root_dir + std::string("/") + strfiles[i];
      int cls = test_binary_short_audio_data(strf, buffer, tdl_handle, sample_rate, seconds);
      std::string str_res =
          strfiles[i] + std::string(",") + std::to_string(cls) + std::string("\t");
      fwrite(str_res.c_str(), str_res.length(), 1, fp);
      if (cls == -1) continue;
      std::string strlabel = std::string("/") + std::to_string(cls) + std::string("/");

      bool correct = strf.find(strlabel) != strf.npos;

      if (cls == 0) {
        if (correct) {
          TN += 1;
        } else {
          FN += 1;
        }
      } else {
        if (correct) {
          TP += 1;
        } else {
          FP += 1;
        }
      }
      total = TP + FN + FP + TN;
      accuracy = (float)(TP + TN) / total;

      std::cout << "accuracy: " << accuracy << std::endl;
    }
    fclose(fp);

    float recall = (float)TP / (TP + FN);
    float false_det_rate = (float)FP / total;

    std::cout << "accuracy: " << accuracy << std::endl;
    std::cout << "recall: " << recall << std::endl;
    std::cout << "false_det_rate: " << false_det_rate << std::endl;

  } else {
    int cls = test_binary_short_audio_data(argv[2], buffer, tdl_handle, sample_rate, seconds);
    std::cout << "result:" << cls << std::endl;
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
/tmp/yzx/infer/sound/data/nihaoshiyun/5m_normsound/files_list.txt \    # data relative path
/tmp/yzx/infer/sound/output/shiyun_v5_5m_normsound.txt                 # result path

for single data:

./test_audio_cls \
/tmp/yzx/infer/sound/models/shiyun_v5_int8_cv181x.cvimodel  \ #model path
/tmp/yzx/infer/sound/data/nihaoshiyun/test_normsound/3/collection_guanbipingmu/0_23_3_2.bin \ #data
path 8000 \ #sample rate  [8000 | 16000] 2  \ #time (s) 0.5 #threshold    0 - 1.0

*/