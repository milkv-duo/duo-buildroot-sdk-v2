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
#include <mutex>
#include <sys/time.h>
#include <cviruntime_context.h>
#include <runtime/debug.h>
#include "cviruntime.h"
#include <runtime/version.h>
#include "argparse.hpp"
#include "assert.h"

static int g_infer_cnt;
static std::string g_model_file;
static std::mutex g_ctx_mutex;
static CVI_MODEL_HANDLE g_model_handle = nullptr;


static void *thread_entry(void *p) {
  (void)p;
  CVI_RC ret;
  CVI_MODEL_HANDLE model;
  do {
    const std::lock_guard<std::mutex> lock(g_ctx_mutex);
    if (!g_model_handle) {
      ret = CVI_NN_RegisterModel(g_model_file.c_str(), &g_model_handle);
      assert(ret == CVI_RC_SUCCESS);
      model = g_model_handle;
    } else {
      ret = CVI_NN_CloneModel(g_model_handle, &model);
      assert(ret == CVI_RC_SUCCESS);
    }
  } while (0);

  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  assert(ret == CVI_RC_SUCCESS);

  int32_t count = g_infer_cnt;
  while (count--) {
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
          rand = rand < 0 ? 0 : rand;
          rand = rand > 1 ? 1 : rand;
          data[i] = rand;
        }
      } else {
        std::normal_distribution<float> d{50, 50};
        int8_t *data = (int8_t *)CVI_NN_TensorPtr(tensor);
        for (int i = 0; i < (int)CVI_NN_TensorCount(tensor); i++) {
          float rand = std::round(d(gen));
          rand = rand < 0 ? 0 : rand;
          rand = rand > 127 ? 127 : rand;
          data[i] = (int8_t)rand;
        }
      }
    }

    CVI_RC rc =
        CVI_NN_Forward(model, input_tensors, input_num,
                       output_tensors, output_num);
    TPU_ASSERT(rc == CVI_RC_SUCCESS, nullptr);
  }

  CVI_NN_CleanupModel(model);
  return NULL;
}

int main(int argc, const char **argv) {
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-m", "--model", 1, false); // required
  parser.addArgument("-n", "--threads", 1, false); // thread count
  parser.addArgument("-c", "--count", 1, false); // inference count
  parser.addArgument("-v", "--verbose", 1);
  parser.parse(argc, argv);

  int thread_num = parser.retrieve<int>("threads");
  g_model_file = parser.retrieve<std::string>("model");
  g_infer_cnt = parser.retrieve<int>("count");

  pthread_t *thread = new pthread_t[thread_num];
  for (int i = 0; i < thread_num; i++) {
    pthread_create(&thread[i], NULL, thread_entry, nullptr);
  }
#ifdef __riscv_d
#include <sched.h>
  sched_yield();
#else
  pthread_yield();
#endif
  for (int i = 0; i < thread_num; i++) {
    if (pthread_join(thread[i], NULL)) {
      printf("failed to join thread #%d\n", i);
      exit(1);
    }
  }
  delete[] thread;

  return 0;
}
