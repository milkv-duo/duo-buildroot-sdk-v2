#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

#include <vector>
#include "cvi_audio.h"
#include "cvi_tdl.h"
#include "sample_utils.h"

// #include <fcntl.h>
// #include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "acodec.h"

#define ACODEC_ADC "/dev/cvitekaadc"

#define AUDIOFORMATSIZE 2
// #define SECOND 3
#define CVI_AUDIO_BLOCK_MODE -1
#define PERIOD_SIZE 640
#define SIZE PERIOD_SIZE *AUDIOFORMATSIZE

static int g_flag = 0;
static int g_class_num = 0;

static const char *enumStr[] = {"无指令", "小爱小爱", "拨打电话", "关闭屏幕", "打开屏幕",
                                "无指令", "小宝小宝", "拨打电话", "关闭屏幕", "打开屏幕",
                                "无指令", "小蓝小蓝", "拨打电话", "关闭屏幕", "打开屏幕",
                                "无指令", "小胖小胖", "拨打电话", "关闭屏幕", "打开屏幕",
                                "无指令", "你好视云", "拨打电话", "关闭屏幕", "打开屏幕"};

static const char *enumStr_juntaida[] = {
    "无指令",   "打开前录", "打开后录", "关闭屏幕", "打开屏幕", "紧急录像",
    "我要拍照", "关闭录音", "打开录音", "打开wifi", "关闭wifi",
};

static std::vector<uint8_t *> g_pack_buf_vec;
static int g_pack_idx = 0;
bool gRun = true;  // signal
int g_record = false;
char *g_outpath = nullptr;  // output file path

static int start_index;
static int SAMPLE_RATE;
static int SECOND;
static int FRAME_SIZE;

// Init cvi_tdl handle.
cvitdl_handle_t tdl_handle = NULL;
MUTEXAUTOLOCK_INIT(ResultMutex);
static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    gRun = false;
  }
}

void *thread_audio(void *arg) {
  CVI_S32 s32Ret;
  AUDIO_FRAME_S stFrame;
  AEC_FRAME_S stAecFrm;
  uint32_t loop = SAMPLE_RATE / PERIOD_SIZE * SECOND;
  printf("enter thread_audio\n");
  while (gRun) {
    s32Ret = CVI_AI_GetFrame(0, 0, &stFrame, &stAecFrm, CVI_AUDIO_BLOCK_MODE);  // Get audio frame
    if (s32Ret != CVI_TDL_SUCCESS) {
      printf("CVI_AI_GetFrame --> none!!\n");
      continue;
    } else {
      MutexAutoLock(ResultMutex, lock);
      uint8_t *p_pack_buf = nullptr;
      if (g_pack_buf_vec.size() >= loop) {
        p_pack_buf = g_pack_buf_vec[0];
        g_pack_buf_vec.erase(g_pack_buf_vec.begin());
        g_pack_idx += 1;
      } else {
        p_pack_buf = (uint8_t *)malloc(SIZE);
      }
      short *p_data = (short *)stFrame.u64VirAddr[0];
      int min_val = 1e5, max_val = 0;
      for (int k = 0; k < PERIOD_SIZE; k++) {
        if (p_data[k] < min_val) min_val = p_data[k];
        if (p_data[k] > max_val) max_val = p_data[k];
      }

      memcpy(p_pack_buf, (CVI_U8 *)stFrame.u64VirAddr[0], SIZE);
      g_pack_buf_vec.push_back(p_pack_buf);
    }
    s32Ret = CVI_AI_ReleaseFrame(0, 0, &stFrame, &stAecFrm);
    if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_ReleaseFrame Failed!!\n");
  }

  for (size_t i = 0; i < g_pack_buf_vec.size(); i++) {
    free(g_pack_buf_vec[i]);
  }
  g_pack_buf_vec.clear();

  pthread_exit(NULL);
}

// Get frame and set it to global buffer
void *thread_infer(void *arg) {
  uint32_t loop = SAMPLE_RATE / PERIOD_SIZE * SECOND;  // 3 seconds

  // Set video frame interface
  CVI_U8 buffer[FRAME_SIZE];  // 3 seconds
  memset(buffer, 0, FRAME_SIZE);
  VIDEO_FRAME_INFO_S Frame;
  Frame.stVFrame.pu8VirAddr[0] = buffer;  // Global buffer
  Frame.stVFrame.u32Height = 1;
  Frame.stVFrame.u32Width = FRAME_SIZE;

  printf("SAMPLE_RATE %d, SIZE:%d, FRAME_SIZE:%d \n", SAMPLE_RATE, SIZE, FRAME_SIZE);

  // classify the sound result
  int cal_counter = 0;
  int pre_label = -1;
  int maxval_sound = 0;
  int pack_idx = 0;
  int pack_len = 512;
  int ret = CVI_TDL_SUCCESS;
  float ts_avg = 0;
  int wait_ts = 250;
  if (g_flag == 1) {
    wait_ts =
        310;  // because inference time of pack version is about 60ms less than original version
  }
  struct timeval last_ts;
  gettimeofday(&last_ts, NULL);
  while (gRun) {
    if (g_pack_buf_vec.size() < loop) {
      usleep(100);
      continue;
    }
    {
      MutexAutoLock(ResultMutex, lock);
      pack_idx = g_pack_idx;
      for (uint32_t i = 0; i < loop; i++) {
        memcpy(buffer + i * SIZE, (CVI_U8 *)g_pack_buf_vec[i],
               SIZE);  // Set the period size date to global buffer
      }
    }
    struct timeval start;
    gettimeofday(&start, NULL);
    if (g_flag == 0) {
      ret = CVI_TDL_SoundClassification(tdl_handle, &Frame, &pre_label);  // Detect the audio
    } else {
      ret = CVI_TDL_SoundClassificationPack(tdl_handle, &Frame, pack_idx, pack_len,
                                            &pre_label);  // Detect the audio
    }
    struct timeval end;
    gettimeofday(&end, NULL);
    float sec = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.;
    float frame_sec = end.tv_sec - last_ts.tv_sec + (end.tv_usec - last_ts.tv_usec) / 1000000.;
    last_ts = end;
    if (ts_avg == 0) {
      ts_avg = sec;

    } else {
      ts_avg = ts_avg * 0.9 + sec * 0.1;
    }
    if (ret == CVI_TDL_SUCCESS) {
      if (g_class_num == 5) {
        printf("class: %s,infer ts(ms):%.2f,fps:%.2f\n", enumStr[pre_label + start_index],
               ts_avg * 1000, 1.0 / frame_sec);
      } else {
        printf("class: %s,infer ts(ms):%.2f,fps:%.2f\n", enumStr_juntaida[pre_label + start_index],
               ts_avg * 1000, 1.0 / frame_sec);
      }

      if (pre_label != 0) {
        usleep(2000 * 1000);

      } else {
        usleep(wait_ts * 1000);
      }

    } else {
      printf("detect failed\n");
      gRun = false;
    }
    if (g_outpath != nullptr) {
      char output_cur[128];
      sprintf(output_cur, "%s%d%s", g_outpath, cal_counter, ".raw");
      // sprintf(output_cur, "%s%s", output_cur, ".raw");
      FILE *fp = fopen(output_cur, "wb");
      printf("to write:%s\n", output_cur);
      fwrite((char *)buffer, 1, FRAME_SIZE, fp);
      fclose(fp);
      usleep(1000 * 1000);
      // gRun = false;
    }

    cal_counter++;
  }

  pthread_exit(NULL);
}

CVI_S32 SET_AUDIO_ATTR(CVI_VOID) {
  // STEP 1: cvitek_audin_set
  //_update_audin_config
  AIO_ATTR_S AudinAttr;
  AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)SAMPLE_RATE;
  AudinAttr.u32ChnCnt = 1;
  AudinAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
  AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
  AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
  AudinAttr.u32EXFlag = 0;
  AudinAttr.u32FrmNum = 10;                /* only use in bind mode */
  AudinAttr.u32PtNumPerFrm = PERIOD_SIZE;  // sample rate / fps
  AudinAttr.u32ClkSel = 0;
  AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;
  CVI_S32 s32Ret;
  // STEP 2:cvitek_audin_uplink_start
  //_set_audin_config
  s32Ret = CVI_AI_SetPubAttr(0, &AudinAttr);
  if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_SetPubAttr failed with %#x!\n", s32Ret);

  s32Ret = CVI_AI_Enable(0);
  if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_Enable failed with %#x!\n", s32Ret);

  s32Ret = CVI_AI_EnableChn(0, 0);
  if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_EnableChn failed with %#x!\n", s32Ret);

  s32Ret = CVI_AI_SetVolume(0, 4);
  if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_SetVolume failed with %#x!\n", s32Ret);

  printf("SET_AUDIO_ATTR success!!\n");
  return CVI_TDL_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc != 6 && argc != 7) {
    printf(
        "Usage: %s ESC_MODEL_PATH SAMPLE_RATE ORDER_TYPE SECOND tORDER_TYPE "
        "FLAG(0:oriversion,1:packversion) "
        "[record_path]\n"
        "\t\tESC_MODEL_PATH, esc model path.\n"
        "\t\tSAMPLE_RATE, sample rate.\n"
        "\t\tSECOND, input time (s).\n"
        "\t\tORDER_TYPE, 0 for ipc, 1 for car.\n"
        "\t\tFLAG, 0: use original version, 1. use optimized version.\n"
        "\t\trecord_path, optional,specify path to dump reord data\n",
        argv[0]);
    printf("xxxxx\n");
    return CVI_TDL_FAILURE;
  }

  SAMPLE_RATE = atoi(argv[2]);
  SECOND = atoi(argv[3]);
  int ORDER_TYPE = atoi(argv[4]);
  g_flag = atoi(argv[5]);

  start_index = ORDER_TYPE * 5;

  FRAME_SIZE = SAMPLE_RATE * AUDIOFORMATSIZE * SECOND;

  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);

  CVI_S32 ret = CVI_TDL_SUCCESS;
  if (CVI_AUDIO_INIT() == CVI_TDL_SUCCESS) {
    printf("CVI_AUDIO_INIT success!!\n");
  } else {
    printf("CVI_AUDIO_INIT failure!!\n");
    return 0;
  }

  SET_AUDIO_ATTR();

  ret = CVI_AI_SetVolume(0, 12);
  if (ret != CVI_TDL_SUCCESS) {
    printf("SetVolume failed !!!!!!!!!!!!!!!!!! with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_CreateHandle3(&tdl_handle);

  if (ret != CVI_TDL_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_sound_param param =
      CVI_TDL_GetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION);
  param.hop_len = 128;
  param.fix = true;

  ret = CVI_TDL_SetSoundClassificationParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
                                            param);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, argv[1]);
  if (ret != CVI_TDL_SUCCESS) {
    CVI_TDL_DestroyHandle(tdl_handle);
    return ret;
  }

  g_class_num = CVI_TDL_GetSoundClassificationClassesNum(tdl_handle);
  printf("g_class_num: %d\n", g_class_num);

  if (argc == 7) {
    g_outpath = (char *)malloc(strlen(argv[6]));
    strcpy(g_outpath, argv[6]);
  }

  pthread_t pcm_output_thread, get_sound;
  pthread_create(&pcm_output_thread, NULL, thread_infer, NULL);
  pthread_create(&get_sound, NULL, thread_audio, NULL);

  pthread_join(pcm_output_thread, NULL);
  pthread_join(get_sound, NULL);

  CVI_TDL_DestroyHandle(tdl_handle);
  return 0;
}
