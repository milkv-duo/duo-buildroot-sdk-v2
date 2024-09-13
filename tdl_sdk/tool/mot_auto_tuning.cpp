#include <inttypes.h>
#include <sys/time.h>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "utils/mot_auto_tuning_helper.hpp"

#include <map>
#include <set>
#include <utility>
#include <vector>
#include "unistd.h"
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define ENABLE_DEEPSORT_EVALUATION

// #define OUTPUT_MOT_DATA

#define DEFAULT_RESULT_FILE_NAME "cvitdl_MOT_result.txt"
#define DEFAULT_DATA_DIR "cvitdl_MOT_data"
#define DEFAULT_DATA_INFO_NAME "MOT_data_info.txt"
#define DEFAULT_OUTPUT_CONFIG_NAME "opt_deepsort_config.bin"

typedef struct {
  TARGET_TYPE_e target_type;
  bool enable_DeepSORT;
  char input_data_dir[128];
  char input_info_name[128];
  bool use_predefined;
  char init_config[128];
  char output_config[128];
  bool output_result;
  char result_path[128];
  int inference_num;
  bool evaluation;
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
      "\n"
      "options:\n"
      "    -d <dir>           input data directory (default: %s)\n"
      "    -n <number>        inference number (default: -1, inference all)\n"
      "    -i <config>        initial DeepSORT config (default: use predefined config)\n"
      "    -o <config>        output DeepSORT config (default: %s)\n"
      "    -r <result>        enable output MOT result and set path (default: false, %s)\n"
      "    -e                 only evaluate performance (default: false)\n"
      "    -z                 enable DeepSORT (default: disable)\n"
      "    -h                 help\n",
      getFileName(bin_path), DEFAULT_DATA_DIR, DEFAULT_OUTPUT_CONFIG_NAME,
      DEFAULT_RESULT_FILE_NAME);
}

CVI_S32 parse_args(int argc, char **argv, ARGS_t *args) {
  const char *OPT_STRING = "hn:d:i:o:r:ez";
  const int ARGS_N = 1;
  /* set default argument value*/
  args->enable_DeepSORT = false;
  args->inference_num = -1;
  sprintf(args->input_data_dir, "%s", DEFAULT_DATA_DIR);
  sprintf(args->input_info_name, "%s", DEFAULT_DATA_INFO_NAME);
  args->use_predefined = true;
  args->evaluation = false;
  args->output_result = false;
  sprintf(args->result_path, "%s", DEFAULT_RESULT_FILE_NAME);

  char ch;
  while ((ch = getopt(argc, argv, OPT_STRING)) != -1) {
    switch (ch) {
      case 'h': {
        usage(argv[0]);
        return CVI_FAILURE;
      } break;
      case 'n': {
        args->inference_num = atoi(optarg);
      } break;
      case 'd': {
        sprintf(args->input_data_dir, "%s", optarg);
      } break;
      case 'i': {
        args->use_predefined = false;
        sprintf(args->init_config, "%s", optarg);
      } break;
      case 'o': {
        sprintf(args->output_config, "%s", optarg);
      } break;
      case 'r': {
        args->output_result = true;
        sprintf(args->result_path, "%s", optarg);
      } break;
      case 'e': {
        args->evaluation = true;
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
  return CVI_SUCCESS;
}

int main(int argc, char *argv[]) {
  ARGS_t args;
  if (CVI_SUCCESS != parse_args(argc, argv, &args)) {
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;

#ifndef ENABLE_DEEPSORT_EVALUATION
  if (args.enable_DeepSORT) {
    printf("DeepSORT is not support now.\n");
    return CVI_FAILURE;
  }
#endif

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

  ret = CVI_TDL_CreateHandle2(&tdl_handle, 1, 0);

  cvtdl_deepsort_config_t ds_conf;
  // Init DeepSORT
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

  MOT_EVALUATION_ARGS_t mot_eval_args;
  mot_eval_args.target_type = args.target_type;
  mot_eval_args.enable_DeepSORT = args.enable_DeepSORT;
  mot_eval_args.mot_data_path = args.input_data_dir;
  MOT_Performance_t performance;

  struct timeval t0, t1;
  unsigned long elapsed_tpu;

  printf("--- Init MOT Evaluation\n");
  gettimeofday(&t0, NULL);
  ret = RUN_MOT_EVALUATION(tdl_handle, mot_eval_args, performance, args.output_result,
                           args.result_path);
  if (CVI_SUCCESS != ret) {
    CVI_TDL_DestroyHandle(tdl_handle);
    return CVI_FAILURE;
  }
  gettimeofday(&t1, NULL);
  elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("execution time: %.2f(ms)\n", (float)elapsed_tpu / 1000.);

  printf("init: score[%.4lf] (coverage rate[%.4lf], stable ids[%u], total entropy[%.4lf])\n",
         performance.score, performance.coverage_rate, performance.stable_id_num,
         performance.total_entropy);

  if (args.evaluation) {
    return CVI_SUCCESS;
  }

  MOT_GRID_SEARCH_PARAMS_t opt_1_params;
  GET_PREDEFINED_OPT_1_PARAMS(opt_1_params);

  printf("--- Do OPTIMIZE_CONFIG_1\n");
  MOT_PERFORMANCE_CONSTRAINT_t constraint;
  constraint.min_coverage_rate = 0.8;
  cvtdl_deepsort_config_t opt_config;
  gettimeofday(&t0, NULL);
  OPTIMIZE_CONFIG_1(tdl_handle, mot_eval_args, opt_1_params, constraint, opt_config, performance);
  gettimeofday(&t1, NULL);
  elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("execution time (OPT 1): %.2f(ms)\n", (float)elapsed_tpu / 1000.);

  printf("@@@ Optimize Performance @@@\n");
  printf("Stable IDs: %u\n", performance.stable_id_num);
  printf("Coverate Rate: %.6lf\n", performance.coverage_rate);
  printf("Total Entorpy: %.6lf\n", performance.total_entropy);
  printf("Score: %lf\n", performance.score);

  // GET_PREDEFINED_CONFIG(&opt_config, target_type);

  FILE *outFile_config = fopen(args.output_config, "w");
  fwrite(&opt_config, sizeof(cvtdl_deepsort_config_t), 1, outFile_config);
  fclose(outFile_config);

  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_SYS_Exit();
}
