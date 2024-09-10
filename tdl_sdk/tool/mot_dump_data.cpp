#include <inttypes.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "utils/mot_evaluation.hpp"
#include "utils/od.h"

#include <map>
#include <set>
#include <utility>

#include "sys/stat.h"
#include "sys/types.h"
#include "unistd.h"
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define ENABLE_FACE_DEEPSORT_EVALUATION

#define OUTPUT_MOT_RESULT

#define DEFAULT_RESULT_FILE_NAME "cvitdl_MOT_result.txt"
#define DEFAULT_DUMP_DATA_DIR "cvitdl_MOT_data"
#define DEFAULT_DUMP_DATA_INFO_NAME "MOT_data_info.txt"

typedef struct {
  TARGET_TYPE_e target_type;
  char *od_m_name;
  char *od_m_path;
  char *fd_m_path;
  char *reid_m_path;
  char *fr_m_path;
  float det_threshold;
  bool enable_DeepSORT;
  bool use_predefined;
  char init_config[128];
  char *imagelist_file_path;
  char output_dir[128];
  char output_info_name[128];
  int inference_num;
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
      "    <target type(=face|person|vehicle|pet)>\n"
      "    <object detection model name>\n"
      "    <object detection model path>\n"
      "    <face detection model path>\n"
      "    <reid model path>\n"
      "    <face recognition model path>\n"
      "    <imagelist path>\n"
      "\n"
      "options:\n"
      "    -t <threshold>     detection threshold (default: 0.5)\n"
      "    -n <number>        inference number (default: -1, inference all)\n"
      "    -i <config>        initial DeepSORT config (default: use predefined config)\n"
      "    -d <dir>           dump data directory (default: %s)\n"
      "    -z                 enable DeepSORT (default: disable)\n"
      "    -h                 help\n",
      getFileName(bin_path), DEFAULT_DUMP_DATA_DIR);
}

CVI_S32 parse_args(int argc, char **argv, ARGS_t *args) {
  const char *OPT_STRING = "ht:n:i:d:z";
  const int ARGS_N = 7;
  /* set default argument value*/
  args->det_threshold = 0.5;
  args->enable_DeepSORT = false;
  args->inference_num = -1;
  sprintf(args->output_dir, "%s", DEFAULT_DUMP_DATA_DIR);
  sprintf(args->output_info_name, "%s", DEFAULT_DUMP_DATA_INFO_NAME);
  args->use_predefined = true;

  char ch;
  while ((ch = getopt(argc, argv, OPT_STRING)) != -1) {
    switch (ch) {
      case 'h': {
        usage(argv[0]);
        return CVI_FAILURE;
      } break;
      case 't': {
        args->det_threshold = atof(optarg);
      } break;
      case 'n': {
        args->inference_num = atoi(optarg);
      } break;
      case 'i': {
        args->use_predefined = false;
        sprintf(args->init_config, "%s", optarg);
      } break;
      case 'd': {
        sprintf(args->output_dir, "%s", optarg);
        // args->output_dir = optarg;
      } break;
      case 'z': {
        args->enable_DeepSORT = true;
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
  if (CVI_SUCCESS != GET_TARGET_TYPE(&args->target_type, argv[i++])) {
    return CVI_FAILURE;
  }
  args->od_m_name = argv[i++];
  args->od_m_path = argv[i++];
  args->fd_m_path = argv[i++];
  args->reid_m_path = argv[i++];
  args->fr_m_path = argv[i++];
  args->imagelist_file_path = argv[i++];
  return CVI_SUCCESS;
}

void SHOW_ARGS(ARGS_t *args) {
  printf(
      "Target Type: %d\n"
      "Obj Detection Model Name  : %s\n"
      "Obj Detection Model Path  : %s\n"
      "Face Detection Model Path : %s\n"
      "ReID Model Path: %s\n"
      "Face Recognition Model Path : %s\n"
      "Detection Threshold : %f\n"
      "Enable DeepSORT: %s\n"
      "Imagelist File Path : %s\n"
      "Inference Number : %d\n"
      "Output Dir : %s\n"
      "Output Info Name : %s\n",
      args->target_type, args->od_m_name, args->od_m_path, args->fd_m_path, args->reid_m_path,
      args->fr_m_path, args->det_threshold, args->enable_DeepSORT ? "True" : "False",
      args->imagelist_file_path, args->inference_num, args->output_dir, args->output_info_name);
}

int main(int argc, char *argv[]) {
  ARGS_t args;
  if (CVI_SUCCESS != parse_args(argc, argv, &args)) {
    return CVI_FAILURE;
  }
  SHOW_ARGS(&args);

  struct stat fst;
  if (stat(args.output_dir, &fst) != 0) {
    printf("create output data directory: %s\n", args.output_dir);
    mkdir(args.output_dir, 0755);
    if (args.enable_DeepSORT) {
      char features_root_dir[256];
      sprintf(features_root_dir, "%s/features", args.output_dir);
      mkdir(features_root_dir, 0755);
    }
  } else {
    printf("output data directory exist.\n");
    return CVI_FAILURE;
  }

  CVI_S32 ret = CVI_SUCCESS;

  // Init VB pool size.
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;
  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 5, vpssgrp_width,
                         vpssgrp_height, PIXEL_FORMAT_RGB_888_PLANAR, 5);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }
  cvitdl_handle_t tdl_handle = NULL;

  ODInferenceFunc inference;
  CVI_TDL_SUPPORTED_MODEL_E od_model_id;
  if (get_od_model_info(args.od_m_name, &od_model_id, &inference) == CVI_TDL_FAILURE) {
    printf("unsupported model: %s\n", argv[1]);
    return CVI_TDL_FAILURE;
  }

  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);

  ret |= CVI_TDL_OpenModel(tdl_handle, od_model_id, args.od_m_path);
  ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, args.fd_m_path);
  ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, args.reid_m_path);
  ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, args.fr_m_path);
  if (ret != CVI_SUCCESS) {
    printf("model open failed with %#x!\n", ret);
    return ret;
  }

  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, od_model_id, false);
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, false);
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, false);
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, false);

  switch (args.target_type) {
    case FACE:
      break;
    case PERSON:
      CVI_TDL_SelectDetectClass(tdl_handle, od_model_id, 1, CVI_TDL_DET_TYPE_PERSON);
      break;
    case VEHICLE:
    case PET:
    default:
      printf("not support target type[%d] now\n", args.target_type);
      return CVI_FAILURE;
  }
  CVI_TDL_SetModelThreshold(tdl_handle, od_model_id, args.det_threshold);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, args.det_threshold);

#ifdef OUTPUT_MOT_RESULT
  // Init DeepSORT
  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_Init(tdl_handle, false);
  if (args.use_predefined) {
    printf("use predefined config.\n");
    if (CVI_SUCCESS != GET_PREDEFINED_CONFIG(&ds_conf, args.target_type)) {
      printf("GET_PREDEFINED_CONFIG error!\n");
      return CVI_FAILURE;
    }
  } else {
    printf("read specific config.\n");
    FILE *inFile_mot_config;
    inFile_mot_config = fopen(args.init_config, "r");
    if (inFile_mot_config == NULL) {
      printf("failed to read DeepSORT config file: %s\n", args.init_config);
      return CVI_FAILURE;
    } else {
      fread(&ds_conf, sizeof(cvtdl_deepsort_config_t), 1, inFile_mot_config);
      fclose(inFile_mot_config);
    }
  }
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, true);

  char outFile_result_path[256];
  sprintf(outFile_result_path, "%s/%s", args.output_dir, DEFAULT_RESULT_FILE_NAME);
  FILE *outFile_result;
  outFile_result = fopen(outFile_result_path, "w");
  if (outFile_result == NULL) {
    printf("There is a problem opening the output file: %s.\n", outFile_result_path);
    return CVI_FAILURE;
  }
#endif

  char outFile_data_path[256];
  sprintf(outFile_data_path, "%s/%s", args.output_dir, args.output_info_name);
  FILE *outFile_data;
  outFile_data = fopen(outFile_data_path, "w");
  if (outFile_data == NULL) {
    printf("There is a problem opening the output file: %s.\n", outFile_data_path);
    return CVI_FAILURE;
  }

  char text_buf[256];
  FILE *inFile = fopen(args.imagelist_file_path, "r");
  fscanf(inFile, "%s", text_buf);
  int img_num = atoi(text_buf);
  printf("Images Num: %d\n", img_num);

#ifdef OUTPUT_MOT_RESULT
  fprintf(outFile_result, "%u\n", img_num);
#endif

  fprintf(outFile_data, "%u %d\n", img_num, (int)args.enable_DeepSORT);

  cvtdl_object_t obj_meta;
  cvtdl_face_t face_meta;
  cvtdl_tracker_t tracker_meta;
  memset(&obj_meta, 0, sizeof(cvtdl_object_t));
  memset(&face_meta, 0, sizeof(cvtdl_face_t));
  memset(&tracker_meta, 0, sizeof(cvtdl_tracker_t));

#ifdef OUTPUT_MOT_RESULT
  MOT_Evaluation mot_eval_data;
#endif

  for (int counter = 1; counter <= img_num; counter++) {
    if (counter == args.inference_num) {
      break;
    }
    fscanf(inFile, "%s", text_buf);
    printf("[%i] image path = %s\n", counter, text_buf);

    VIDEO_FRAME_INFO_S frame;
    CVI_TDL_ReadImage(text_buf, &frame, PIXEL_FORMAT_RGB_888);

    switch (args.target_type) {
      case PERSON: {
        inference(tdl_handle, &frame, &obj_meta);
        if (args.enable_DeepSORT) {
          CVI_TDL_OSNet(tdl_handle, &frame, &obj_meta);
        }
        CVI_TDL_DeepSORT_Obj(tdl_handle, &obj_meta, &tracker_meta, args.enable_DeepSORT);
      } break;
      case FACE: {
        CVI_TDL_FaceDetection(tdl_handle, &frame, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, &face_meta);
        if (args.enable_DeepSORT) {
          CVI_TDL_FaceRecognition(tdl_handle, &frame, &face_meta);
        }
        for (uint32_t i = 0; i < face_meta.size; i++) {
          printf("face[%u] bbox: x1[%.2f], y1[%.2f], x2[%.2f], y2[%.2f]\n", i,
                 face_meta.info[i].bbox.x1, face_meta.info[i].bbox.y1, face_meta.info[i].bbox.x2,
                 face_meta.info[i].bbox.y2);
        }
#ifdef OUTPUT_MOT_RESULT
#ifdef ENABLE_FACE_DEEPSORT_EVALUATION
        CVI_TDL_DeepSORT_Face(tdl_handle, &face_meta, &tracker_meta);
#else
        CVI_TDL_DeepSORT_Face(tdl_handle, &face_meta, &tracker_meta);
#endif
#endif
      } break;
      default:
        break;
    }

#ifdef OUTPUT_MOT_RESULT
    cvtdl_tracker_t inact_trackers;
    memset(&inact_trackers, 0, sizeof(cvtdl_tracker_t));
    CVI_TDL_DeepSORT_GetTracker_Inactive(tdl_handle, &inact_trackers);
    mot_eval_data.update(tracker_meta, inact_trackers);

    fprintf(outFile_result, "%u\n", tracker_meta.size);
    for (uint32_t i = 0; i < tracker_meta.size; i++) {
      cvtdl_bbox_t *target_bbox =
          (args.target_type == FACE) ? &face_meta.info[i].bbox : &obj_meta.info[i].bbox;
      uint64_t u_id =
          (args.target_type == FACE) ? face_meta.info[i].unique_id : obj_meta.info[i].unique_id;
      fprintf(outFile_result, "%" PRIu64 ",%d,%d,%d,%d,%d,%d,%d,%d,%d\n", u_id,
              (int)target_bbox->x1, (int)target_bbox->y1, (int)target_bbox->x2,
              (int)target_bbox->y2, tracker_meta.info[i].state, (int)tracker_meta.info[i].bbox.x1,
              (int)tracker_meta.info[i].bbox.y1, (int)tracker_meta.info[i].bbox.x2,
              (int)tracker_meta.info[i].bbox.y2);
    }
    fprintf(outFile_result, "%u\n", inact_trackers.size);
    for (uint32_t i = 0; i < inact_trackers.size; i++) {
      fprintf(outFile_result, "%" PRIu64 ",-1,-1,-1,-1,%d,%d,%d,%d,%d\n", inact_trackers.info[i].id,
              inact_trackers.info[i].state, (int)inact_trackers.info[i].bbox.x1,
              (int)inact_trackers.info[i].bbox.y1, (int)inact_trackers.info[i].bbox.x2,
              (int)inact_trackers.info[i].bbox.y2);
    }
    CVI_TDL_Free(&inact_trackers);
#endif

    if (args.enable_DeepSORT) {
      char features_dir[256];
      sprintf(features_dir, "%s/features/%04d", args.output_dir, counter);
      mkdir(features_dir, 0755);
    }

    uint32_t bbox_size = (args.target_type == FACE) ? face_meta.size : obj_meta.size;
    fprintf(outFile_data, "%u\n", bbox_size);
    for (uint32_t i = 0; i < bbox_size; i++) {
      cvtdl_bbox_t *target_bbox =
          (args.target_type == FACE) ? &face_meta.info[i].bbox : &obj_meta.info[i].bbox;
      int class_id = (args.target_type == FACE) ? -1 : obj_meta.info[i].classes;
      fprintf(outFile_data, "%d %f %f %f %f\n", class_id, target_bbox->x1, target_bbox->y1,
              target_bbox->x2, target_bbox->y2);
      if (args.enable_DeepSORT) {
        cvtdl_feature_t *target_feature =
            (args.target_type == FACE) ? &face_meta.info[i].feature : &obj_meta.info[i].feature;
        if (target_feature->type != TYPE_INT8) {
          printf("[WARNING] feature type is not TYPE_INT8.\n");
          continue;
        }
        // int8_t *feature_data = (int8_t *)malloc(target_feature->size * sizeof(int8_t));
        char feature_data_name[256];
        char feature_data_path[512];
        sprintf(feature_data_name, "features/%04d/%02u.bin", counter, i);
        sprintf(feature_data_path, "%s/%s", args.output_dir, feature_data_name);
        fprintf(outFile_data, "%u %s\n", target_feature->size, feature_data_name);
        FILE *outFile_feature;
        outFile_feature = fopen(feature_data_path, "w");
        fwrite(target_feature->ptr, sizeof(int8_t), target_feature->size, outFile_feature);
        // free(feature_data);
        fclose(outFile_feature);
      } else {
        fprintf(outFile_data, "0 NULL\n");
      }
    }

    switch (args.target_type) {
      case FACE:
        CVI_TDL_Free(&face_meta);
        break;
      case PERSON:
      case VEHICLE:
      case PET:
        CVI_TDL_Free(&obj_meta);
        break;
      default:
        break;
    }
    CVI_TDL_Free(&tracker_meta);
    CVI_TDL_ReleaseImage(&frame);
  }

#ifdef OUTPUT_MOT_RESULT
  MOT_Performance_t mot_performance;
  mot_eval_data.summary(mot_performance);
  printf("@@@ Optimize Performance @@@\n");
  printf("Stable IDs: %u\n", mot_performance.stable_id_num);
  printf("Coverate Rate: %.6lf\n", mot_performance.coverage_rate);
  printf("Total Entorpy: %.6lf\n", mot_performance.total_entropy);
  printf("Score: %lf\n", mot_performance.score);

  fclose(outFile_result);
#endif

  fclose(outFile_data);

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_SYS_Exit();
}
