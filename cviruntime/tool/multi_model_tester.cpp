#include "pthread.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>
#include <sys/time.h>
#include <cviruntime_context.h>
#include <runtime/debug.h>
#include "cviruntime.h"
#include <runtime/version.h>
#include "argparse.hpp"
#include "assert.h"

static int32_t optCount = 1;

#define EXIT_IF_ERROR(cond, statement)                                                   \
  if ((cond)) {                                                                          \
    printf("%s\n", statement);                                                    \
    exit(1);                                                                             \
  }

static bool compare = false;
static void *thread_entry(void *p) {
  CVI_MODEL_HANDLE model = (CVI_MODEL_HANDLE)p;

  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  int32_t count = optCount;
  bool init_in_out = false;
  std::vector<std::vector<uint8_t>> results;
  while (count--) {
    CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                 &output_tensors, &output_num);
    if (!compare || !init_in_out) {
      // fill random data to inputs.
      std::random_device rd{};
      std::mt19937 gen{rd()};
      for (int i = 0; i < input_num; i++) {
          CVI_TENSOR *tensor = &input_tensors[i];
          if (tensor->fmt == CVI_FMT_FP32) {
              std::normal_distribution<float> d{0.3, 0.2};
              float *data = (float *)CVI_NN_TensorPtr(tensor);
              for (int i = 0; i < (int)CVI_NN_TensorCount(tensor); i++) {
                  float rand = d(gen);
                  rand       = rand < 0 ? 0 : rand;
                  rand       = rand > 1 ? 1 : rand;
                  data[i]    = rand;
              }
          } else {
              std::normal_distribution<float> d{50, 50};
              int8_t *data = (int8_t *)CVI_NN_TensorPtr(tensor);
              for (int i = 0; i < (int)CVI_NN_TensorCount(tensor); i++) {
                  float rand = std::round(d(gen));
                  rand       = rand < 0 ? 0 : rand;
                  rand       = rand > 127 ? 127 : rand;
                  data[i]    = (int8_t)rand;
              }
          }
      }
    }

    CVI_RC rc =
        CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
    TPU_ASSERT(rc == CVI_RC_SUCCESS, nullptr);
    if (compare) {
      if (!init_in_out) {
        for (int i = 0; i < output_num; ++i) {
          uint8_t *data = (uint8_t*)CVI_NN_TensorPtr(&output_tensors[i]);
          std::vector<uint8_t> rst;
          rst.assign(data, data + CVI_NN_TensorSize(&output_tensors[i]));
          results.emplace_back(std::move(rst));
        }
        init_in_out = true;
      } else {
        for (int i = 0; i < output_num; ++i) {
          uint8_t *data = (uint8_t*)CVI_NN_TensorPtr(&output_tensors[i]);
          for (size_t j = 0; j < CVI_NN_TensorCount(&output_tensors[i]); ++j) {
            if (data[j] != results[i][j]) {
              printf("check reuslt fail! tensor:%s\n", CVI_NN_TensorName(&output_tensors[i]));
            }
          }
        }
      }
    }
  }
  return NULL;
}

int main(int argc, const char **argv) {
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-m", "--models", '+', false); // required
  parser.addArgument("-c", "--count", 1);      // inference count
  parser.addArgument("-v", "--verbose", 1);
  parser.addArgument("-s", "--shmsize", 1);
  parser.addArgument("-p", "--compare", 1);
  parser.parse(argc, argv);

  if (parser.gotArgument("count")) {
    optCount = parser.retrieve<int>("count");
  }

  if (parser.gotArgument("shmsize")) {
    int shmsize = parser.retrieve<int>("shmsize");
    CVI_NN_Global_SetSharedMemorySize(shmsize);
    printf("set global shared memory size:%d", shmsize);
  }

  if (parser.gotArgument("compare")) {
    compare = parser.retrieve<bool>("compare");
  }

  std::vector<std::string> optModelFiles;
  optModelFiles = parser.retrieve<std::vector<std::string>>("models");
  EXIT_IF_ERROR(optModelFiles.size() == 0, "please set one cvimodels at least");

  dumpSysfsDebugFile("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary");

  std::vector<CVI_MODEL_HANDLE> models;
  for (auto &modelFile : optModelFiles) {
    CVI_MODEL_HANDLE model;
    printf("get model file:%s\n", modelFile.c_str());
    CVI_RC ret = CVI_NN_RegisterModel(modelFile.c_str(), &model);
    EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "failed to register cvimodel");
    models.push_back(model);
  }

  int thread_num = optModelFiles.size();
  pthread_t *thread = new pthread_t[thread_num];
  for (int i = 0; i < thread_num; i++) {
    pthread_create(&thread[i], NULL, thread_entry, models[i]);
  }

#ifdef __riscv_d
#include <sched.h>
  sched_yield();
#else
  // pthread_yield();
#include <sched.h>
  sched_yield();
#endif
  for (int i = 0; i < thread_num; i++) {
    if (pthread_join(thread[i], NULL)) {
      printf("failed to join thread #%d\n", i);
      exit(1);
    }
  }
  delete[] thread;

  dumpSysfsDebugFile("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary");
  for (int i = 0; i < thread_num; i++) {
    CVI_NN_CleanupModel(models[i]);
  }
  return 0;
}
