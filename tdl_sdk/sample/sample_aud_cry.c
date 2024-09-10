#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include "cvi_audio.h"
#include "cvi_tdl.h"
#include "sample_utils.h"

#define AUDIOFORMATSIZE 2
#define SECOND 3
#define CVI_AUDIO_BLOCK_MODE -1
#define PERIOD_SIZE 640
#define SAMPLE_RATE 16000
#define FRAME_SIZE SAMPLE_RATE *AUDIOFORMATSIZE *SECOND  // PCM_FORMAT_S16_LE (2bytes) 3 seconds

// ESC class name
enum Classes { NO_BABY_CRY, BABY_CRY };
// This model has 6 classes, including Sneezing, Coughing, Clapping, Baby Cry, Glass breaking,
// Office. There will be a Normal option in the end, becuase the score is lower than threshold, we
// will set it to Normal.
static const char *enumStr[] = {"no_baby_cry", "baby_cry"};

bool gRun = true;     // signal
bool record = false;  // record to output
char *outpath;        // output file path

// Init cvi_tdl handle.
cvitdl_handle_t tdl_handle = NULL;

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    gRun = false;
  }
}

// Get frame and set it to global buffer
void *thread_uplink_audio(void *arg) {
  CVI_S32 s32Ret;
  AUDIO_FRAME_S stFrame;
  AEC_FRAME_S stAecFrm;
  int loop = SAMPLE_RATE / PERIOD_SIZE * SECOND;  // 3 seconds
  int size = PERIOD_SIZE * AUDIOFORMATSIZE;       // PCM_FORMAT_S16_LE (2bytes)

  // Set video frame interface
  CVI_U8 buffer[FRAME_SIZE];  // 3 seconds
  memset(buffer, 0, FRAME_SIZE);
  VIDEO_FRAME_INFO_S Frame;
  Frame.stVFrame.pu8VirAddr[0] = buffer;  // Global buffer
  Frame.stVFrame.u32Height = 1;
  Frame.stVFrame.u32Width = FRAME_SIZE;

  // classify the sound result
  int index = -1;
  int maxval_sound = 0;
  while (gRun) {
    for (int i = 0; i < loop; ++i) {
      s32Ret = CVI_AI_GetFrame(0, 0, &stFrame, &stAecFrm, CVI_AUDIO_BLOCK_MODE);  // Get audio frame
      if (s32Ret != CVI_TDL_SUCCESS) {
        printf("CVI_AI_GetFrame --> none!!\n");
        continue;
      } else {
        memcpy(buffer + i * size, (CVI_U8 *)stFrame.u64VirAddr[0],
               size);  // Set the period size date to global buffer
      }
    }
    int16_t *psound = (int16_t *)buffer;
    float meanval = 0;
    for (int i = 0; i < SAMPLE_RATE * SECOND; i++) {
      meanval += psound[i];
      if (psound[i] > maxval_sound) {
        maxval_sound = psound[i];
      }
    }
    printf("maxvalsound:%d,meanv:%f\n", maxval_sound, meanval / (SAMPLE_RATE * SECOND));
    if (!record) {
      int ret = CVI_TDL_SoundClassification(tdl_handle, &Frame, &index);  // Detect the audio
      if (ret == CVI_TDL_SUCCESS) {
        printf("esc class: %s\n", enumStr[index]);
      } else {
        printf("detect failed\n");
      }
    } else {
      FILE *fp = fopen(outpath, "wb");
      printf("to write:%s\n", outpath);
      fwrite((char *)buffer, 1, FRAME_SIZE, fp);
      fclose(fp);
      gRun = false;
    }
  }
  s32Ret = CVI_AI_ReleaseFrame(0, 0, &stFrame, &stAecFrm);
  if (s32Ret != CVI_TDL_SUCCESS) printf("CVI_AI_ReleaseFrame Failed!!\n");

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
  if (argc != 2 && argc != 4) {
    printf(
        "Usage: %s ESC_MODEL_PATH RECORD OUTPUT\n"
        "\t\tESC_MODEL_PATH, esc model path.\n"
        "\t\tRECORD, record input to file, 0: disable 1. enable.\n"
        "\t\tOUTPUT, output file path: {output file path}.raw\n",
        argv[0]);
    return CVI_TDL_FAILURE;
  }
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

  ret = CVI_TDL_CreateHandle3(&tdl_handle);

  if (ret != CVI_TDL_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, argv[1]);
  if (ret != CVI_TDL_SUCCESS) {
    CVI_TDL_DestroyHandle(tdl_handle);
    return ret;
  }

  // CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
  // true);
  CVI_TDL_SetPerfEvalInterval(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION,
                              10);  // only used to performance evaluation
  if (argc == 4) {
    record = atoi(argv[2]) ? true : false;
    outpath = (char *)malloc(strlen(argv[3]));
    strcpy(outpath, argv[3]);
  }

  pthread_t pcm_output_thread;
  pthread_create(&pcm_output_thread, NULL, thread_uplink_audio, NULL);

  pthread_join(pcm_output_thread, NULL);
  if (argc == 4) {
    free(outpath);
  }

  CVI_TDL_DestroyHandle(tdl_handle);
  return 0;
}
