#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include "cviruntime.h"

static void dump_tensors(CVI_TENSOR *tensors, int32_t num) {
  for (int32_t i = 0; i < num; i++) {
    printf("  [%d] %s, shape (%d,%d,%d,%d), count %zu, fmt %d\n",
        i,
        tensors[i].name,
        tensors[i].shape.dim[0],
        tensors[i].shape.dim[1],
        tensors[i].shape.dim[2],
        tensors[i].shape.dim[3],
        tensors[i].count,
        tensors[i].fmt);
  }
}

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel [1|0]\n", argv[0]);
}

int main(int argc, char **argv) {
  int ret = 0;
  CVI_MODEL_HANDLE model;

  if (argc != 2 && argc != 3) {
    usage(argv);
    exit(-1);
  }
  bool output_all_tensors = false;
  if (argc >= 3) {
    output_all_tensors = (atoi(argv[2]) > 0) ? true : false;;
  }

  // normal mode
  ret = CVI_NN_RegisterModel(argv[1], &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    return -1;
  }
  printf("CVI_NN_RegisterModel succeeded\n");

  // retrieve input / output tensor struct
  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                               &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    CVI_NN_CleanupModel(model);
    return -1;
  }

  // dump param
  printf("Model Param for %s:\n", argv[1]);
  printf("  Input Tensor Number  : %d\n", input_num);
  dump_tensors(input_tensors, input_num);
  printf("  Output Tensor Number : %d\n", output_num);
  dump_tensors(output_tensors, output_num);
  printf("  Output All Tensors For Debug: [%s]\n", output_all_tensors ? "YES" : "NO");

  // clean up
  ret = CVI_NN_CleanupModel(model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_CleanupModel failed, err %d\n", ret);
    return -1;
  }
  printf("CVI_NN_CleanupModel succeeded\n");

  return 0;
}
