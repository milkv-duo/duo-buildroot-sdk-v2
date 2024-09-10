#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "sample_comm.h"
#include "vi_vo_utils.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// #define EXECUTION_TIME
#define OUTPUT_CAPTURE_FACE_FEATURE
#define FACE_FEATURE_SIZE 256
#define CAPTURE_FACE_FEATURE_TIME 100

static volatile bool bExit = false;

static void SampleHandleSig(CVI_S32 signo) {
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (SIGINT == signo || SIGTERM == signo) {
    bExit = true;
  }
}

/* helper functions */
CVI_S32 COSINE_SIMILARITY(cvtdl_feature_t *a, cvtdl_feature_t *b, float *score);
CVI_S32 GET_PIXEL_FORMAT_BY_NAME(PIXEL_FORMAT_E *format, char *name);

typedef struct {
  char *fd_m_path;
  char *fr_m_path;
  bool input_feature;
  char input_feature_file[128];
  int voType;
  PIXEL_FORMAT_E aiInputFormat;
} ARGS_t;

char *getFileName(char *path) {
  char *retVal = path, *p;
  for (p = path; *p; p++) {
    if (*p == '/' || *p == '\\' || *p == ':') {
      retVal = p + 1;
    }
  }
  return retVal;
}

void usage(char *bin_path) {
  printf(
      "Usage: %s [options]\n"
      "    <face detection model path>\n"
      "    <face recognition model path>\n"
      "    <video output type>\n"
      "\n"
      "options:\n"
      "    -i <PIXEL FORMAT>  video input format (default: RGB_888)\n"
      "    -f <file>          feature data file (default: NULL)\n"
      "    -h                 help\n"
      "\n"
      "video output type:\n"
      "    0: disable, 1: panel, 2: rtsp\n"
      "PIXEL FORMAT:\n"
      "    RGB_888            = RGB\n"
      "    YUV_PLANAR_420     = YUV420\n"
      "    NV21               = \n",
      getFileName(bin_path));
}

CVI_S32 parse_args(int argc, char **argv, ARGS_t *args) {
  const char *OPT_STRING = "hi:f:";
  const int ARGS_N = 3;
  /* set default argument value*/
  args->input_feature = false;
  args->aiInputFormat = PIXEL_FORMAT_RGB_888;

  char ch;
  while ((ch = getopt(argc, argv, OPT_STRING)) != -1) {
    switch (ch) {
      case 'h': {
        usage(argv[0]);
        return CVI_FAILURE;
      } break;
      case 'i': {
        if (CVI_SUCCESS != GET_PIXEL_FORMAT_BY_NAME(&args->aiInputFormat, optarg)) {
          return CVI_FAILURE;
        }
      } break;
      case 'f': {
        args->input_feature = true;
        sprintf(args->input_feature_file, "%s", optarg);
      } break;
      case '?': {
        printf("error optopt: %c\n", optopt);
        printf("error opterr: %d\n", opterr);
        return CVI_FAILURE;
      }
    }
  }
  if (ARGS_N != (argc - optind)) {
    printf("Args number error (given %d, except %d)\n", (argc - optind), ARGS_N);
    return CVI_FAILURE;
  }
  int i = optind;
  args->fd_m_path = argv[i++];
  args->fr_m_path = argv[i++];
  args->voType = atoi(argv[i++]);
  return CVI_SUCCESS;
}

int main(int argc, char *argv[]) {
  ARGS_t args;
  if (CVI_SUCCESS != parse_args(argc, argv, &args)) {
    return CVI_FAILURE;
  }
  // Set signal catch
  signal(SIGINT, SampleHandleSig);
  signal(SIGTERM, SampleHandleSig);
  CVI_S32 s32Ret = CVI_SUCCESS;

  VideoSystemContext vs_ctx = {0};
  int fps = 25;
  if (InitVideoSystem(&vs_ctx, fps) != CVI_SUCCESS) {
    printf("failed to init video system\n");
    return CVI_FAILURE;
  }

  cvitdl_handle_t tdl_handle = NULL;
  cvitdl_service_handle_t service_handle = NULL;

  int ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);
  ret |= CVI_TDL_Service_CreateHandle(&service_handle, tdl_handle);
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, args.fd_m_path);
  if (ret != CVI_SUCCESS) {
    printf("Facelib open failed with %#x!\n", ret);
    return ret;
  }
  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, args.fr_m_path);
  if (ret != CVI_SUCCESS) {
    printf("Facelib open failed with %#x!\n", ret);
    return ret;
  }
  // Do vpss frame transform in retina face
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, false);

  cvtdl_feature_t base_face_feature;
  memset(&base_face_feature, 0, sizeof(cvtdl_feature_t));
  if (args.input_feature) {
    base_face_feature.size = FACE_FEATURE_SIZE;
    base_face_feature.ptr = (int8_t *)malloc(FACE_FEATURE_SIZE * sizeof(uint8_t));
    FILE *inFile_feature_data;
    inFile_feature_data = fopen(args.input_feature_file, "r");
    if (inFile_feature_data == NULL) {
      printf("failed to read face feature data file: %s\n", args.input_feature_file);
      return CVI_FAILURE;
    } else {
      fread(base_face_feature.ptr, sizeof(uint8_t), FACE_FEATURE_SIZE, inFile_feature_data);
      fclose(inFile_feature_data);
    }
  }

  VIDEO_FRAME_INFO_S stfdFrame, stVOFrame;
  cvtdl_face_t face;
  memset(&face, 0, sizeof(cvtdl_face_t));
  uint32_t counter = 0;
  while (bExit == false) {
    printf("\nFrame[%u]\n", counter++);
    if (CVI_SUCCESS != CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp,
                                            vs_ctx.vpssConfigs.vpssChntdl, &stfdFrame, 2000)) {
      printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
      break;
    }

    CVI_TDL_FaceDetection(tdl_handle, &stfdFrame, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, &face);
#ifdef EXECUTION_TIME
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
#endif
    CVI_TDL_FaceRecognition(tdl_handle, &stfdFrame, &face);
#ifdef EXECUTION_TIME
    gettimeofday(&t1, NULL);
    unsigned long execution_time = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
    printf("FaceRecognition execution time: %.2f(ms)\n", (float)execution_time / 1000.);
    printf("faces number: %u\n", face.size);
#endif

    if (base_face_feature.size == 0 && counter >= CAPTURE_FACE_FEATURE_TIME && face.size > 0) {
      printf("capture face[0] feature\n");
      base_face_feature.size = FACE_FEATURE_SIZE;
      base_face_feature.ptr = (int8_t *)malloc(FACE_FEATURE_SIZE * sizeof(int8_t));
      memcpy(base_face_feature.ptr, face.info[0].feature.ptr, FACE_FEATURE_SIZE * sizeof(int8_t));
#ifdef OUTPUT_CAPTURE_FACE_FEATURE
      char output_file[64];
      sprintf(output_file, "face_feature-%u.bin", face.info[0].feature.size);
      FILE *outFile_feature = fopen(output_file, "w");
      fwrite(face.info[0].feature.ptr, sizeof(int8_t), face.info[0].feature.size, outFile_feature);
      fclose(outFile_feature);
#endif
    }

    if (base_face_feature.size > 0) {
      float score = 0.;
      for (uint32_t i = 0; i < face.size; i++) {
        if (CVI_SUCCESS != COSINE_SIMILARITY(&base_face_feature, &face.info[i].feature, &score)) {
          score = -1.;
        }
        printf("face[%u]: score[%.4f]\n", i, score);
      }
    }

    if (CVI_SUCCESS != CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp,
                                                vs_ctx.vpssConfigs.vpssChntdl, &stfdFrame)) {
      printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
      break;
    }

    // Send frame to VO if opened.
    if (args.voType) {
      if (CVI_SUCCESS != CVI_VPSS_GetChnFrame(vs_ctx.vpssConfigs.vpssGrp,
                                              vs_ctx.vpssConfigs.vpssChnVideoOutput, &stVOFrame,
                                              1000)) {
        printf("CVI_VPSS_GetChnFrame chn0 failed with %#x\n", s32Ret);
        break;
      }
      CVI_TDL_Service_FaceDrawRect(service_handle, &face, &stVOFrame, true,
                                   CVI_TDL_Service_GetDefaultBrush());
      if (CVI_SUCCESS != SendOutputFrame(&stVOFrame, &vs_ctx.outputContext)) {
        printf("Send Output Frame NG\n");
      }

      if (CVI_SUCCESS != CVI_VPSS_ReleaseChnFrame(vs_ctx.vpssConfigs.vpssGrp,
                                                  vs_ctx.vpssConfigs.vpssChnVideoOutput,
                                                  &stVOFrame)) {
        printf("CVI_VPSS_ReleaseChnFrame chn0 NG\n");
        break;
      }
    }

    CVI_TDL_Free(&face);
  }
  CVI_TDL_Free(&base_face_feature);

  CVI_TDL_Service_DestroyHandle(service_handle);
  CVI_TDL_DestroyHandle(tdl_handle);
  DestroyVideoSystem(&vs_ctx);
  CVI_SYS_Exit();
  CVI_VB_Exit();
}

CVI_S32 COSINE_SIMILARITY(cvtdl_feature_t *a, cvtdl_feature_t *b, float *score) {
  if (a->ptr == NULL || b->ptr == NULL || a->size != b->size) {
    printf("cosine distance failed.\n");
    return CVI_FAILURE;
  }
  float A = 0, B = 0, AB = 0;
  for (uint32_t i = 0; i < a->size; i++) {
    A += (int)a->ptr[i] * (int)a->ptr[i];
    B += (int)b->ptr[i] * (int)b->ptr[i];
    AB += (int)a->ptr[i] * (int)b->ptr[i];
  }
  A = sqrt(A);
  B = sqrt(B);
  *score = AB / (A * B);
  return CVI_SUCCESS;
}

CVI_S32 GET_PIXEL_FORMAT_BY_NAME(PIXEL_FORMAT_E *format, char *name) {
  if (!strcmp(name, "RGB_888") || !strcmp(name, "RGB")) {
    *format = PIXEL_FORMAT_RGB_888;
  } else if (!strcmp(name, "YUV_PLANAR_420") || !strcmp(name, "YUV420")) {
    *format = PIXEL_FORMAT_YUV_PLANAR_420;
  } else if (!strcmp(name, "NV21")) {
    *format = PIXEL_FORMAT_NV21;
  } else {
    printf("unknown image type %s\n", name);
    return CVI_FAILURE;
  }
  return CVI_SUCCESS;
}