#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cvi_tdl.h"

#define AUDIOFORMATSIZE 2
#define SECOND 3
#define CVI_AUDIO_BLOCK_MODE -1
#define PERIOD_SIZE 640
#define SAMPLE_RATE 16000
#define FRAME_SIZE SAMPLE_RATE *AUDIOFORMATSIZE *SECOND  // PCM_FORMAT_S16_LE (2bytes) 3 seconds

static bool read_binary_file(const char *szfile, void **pp_buffer, int *p_buffer_len) {
  FILE *fp = fopen(szfile, "rb");
  if (fp == NULL) {
    printf("read file failed,%s\n", szfile);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  *pp_buffer = (uint8_t *)malloc(len);
  fread(*pp_buffer, len, 1, fp);
  *p_buffer_len = len;
  fclose(fp);
  return true;
}

int main(int argc, char *argv[]) {
  cvitdl_handle_t tdl_handle = NULL;
  CVI_S32 ret = CVI_TDL_CreateHandle3(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create ai handle failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_sound_param param =
      CVI_TDL_GetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
  param.hop_len = 128;
  param.fix = true;

  printf("setup audio algorithm parameters \n");

  ret = CVI_TDL_SetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
                                            param);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open modelfile failed %#x!\n", ret);
    return ret;
  } else {
    printf("model opened\n");
  }

  void *p_buffer = NULL;
  int buffer_len = 0;
  if (!read_binary_file(argv[2], &p_buffer, &buffer_len)) {
    printf("read file failed\n");
    return -1;
  } else {
    printf("read file done,datalen:%d,addr:%0x\n", buffer_len, p_buffer);
  }

  param =
      CVI_TDL_GetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
  // printf("get new param,sample_rate:%d,\n");

  int flag = atoi(argv[3]);

  int pack_len = 640;
  int num_wav_len = param.sample_rate * param.time_len;
  int num_pack = num_wav_len / (float)pack_len + 0.5;
  int buf_que_len = num_pack * pack_len;

  printf("num_wav_len:%d,num_pack:%d\n", num_wav_len, num_pack);

  short *p_frame_buf = (short *)p_buffer;
  int pack_idx = 0;

  while ((pack_idx + num_pack) * pack_len * AUDIOFORMATSIZE <= buffer_len) {
    VIDEO_FRAME_INFO_S Frame;
    Frame.stVFrame.pu8VirAddr[0] = (uint8_t *)(p_frame_buf + pack_idx * pack_len);  // Global buffer
    Frame.stVFrame.u32Height = 1;
    Frame.stVFrame.u32Width = buf_que_len * AUDIOFORMATSIZE;

    int src_label = 0, pack_label = 0;
    if (flag == 0 || flag == 2) {
      ret = CVI_TDL_SoundClassification(tdl_handle, &Frame, &src_label);
      if (ret != 0) {
        printf("sound classification failed\n");
        break;
      }
    }

    if (flag == 1 || flag == 2) {
      ret = CVI_TDL_SoundClassificationPack(tdl_handle, &Frame, pack_idx, pack_len, &pack_label);
      if (ret != 0) {
        printf("sound classification failed\n");
        break;
      }
    }

    printf("pack_idx:%d,src_label:%d,pack_label:%d\n", pack_idx, src_label, pack_label);
    if (pack_idx > 10) {
      break;
    }
    pack_idx += (0.25 * param.sample_rate) / pack_len;
  }

  free(p_buffer);

  CVI_TDL_DestroyHandle(tdl_handle);

  return ret;
}
