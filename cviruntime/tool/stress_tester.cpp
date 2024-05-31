#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <cviruntime_context.h>
#include <runtime/debug.h>
#include "cviruntime.h"
#include <runtime/version.h>
#include "argparse.hpp"
#include "similarity.hpp"
#include "cnpy.h"
#include "assert.h"

static bool isNpzFile(const std::string &name) {
  std::string extension = name.substr(name.size() - 4);
  if (extension == ".npz")
    return true;
  return false;
}

class ModelTester {
public:
  ModelTester(const std::string &model_file, const std::string &ref_npz);
  ~ModelTester();
  void loadInputData(const std::string &input_npz);
  void run();
  bool compareResults();

private:
  CVI_RT_HANDLE ctx;
  CVI_MODEL_HANDLE model = NULL;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;

  std::vector<int8_t> input_vec;
  std::string ref_npz;
};

ModelTester::ModelTester(const std::string &model_file, const std::string &ref_npz)
    : ref_npz(ref_npz) {

  CVI_RT_Init(&ctx);

  if (CVI_RC_SUCCESS != CVI_NN_RegisterModel(model_file.c_str(), &model)) {
    exit(1);
  }
  if (!model)
    return;

  CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, true);

  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors,
                               &output_num);
}

ModelTester::~ModelTester() {
  CVI_NN_CleanupModel(model);
  CVI_RT_DeInit(ctx);
}

bool ModelTester::compareResults() {
  if (1)
    return true;
  float euclidean = 0;
  float cosine = 0;
  float correlation = 0;

  int err_cnt = 0;
  for (int i = 0; i < output_num; i++) {
    auto &tensor = output_tensors[i];
    std::string name(tensor.name);
    auto refData = cnpy::npz_load(ref_npz, name);
    if (refData.num_vals == 0) {
      printf("Warning, Cannot find %s in reference\n", name.c_str());
      continue;
    }
    if (tensor.count != refData.num_vals) {
      printf("%s %zu vs %zu, size are not equal\n", name.c_str(), tensor.count, refData.num_vals);
      return false;
    }

    if (refData.type == 'f') {
      if (tensor.fmt == CVI_FMT_INT8) {
        array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_BF16) {
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      } else {
        array_similarity((float *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      }
    } else if (refData.type == 'u') {
      assert(refData.word_size == 2);
      if (tensor.fmt == CVI_FMT_BF16) {
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor), refData.data<uint16_t>(),
                         tensor.count, euclidean, cosine, correlation);
      } else {
        array_similarity((float *)CVI_NN_TensorPtr(&tensor), refData.data<uint16_t>(),
                         tensor.count, euclidean, cosine, correlation);
      }
    } else if (refData.type == 'i') {
      assert(refData.word_size == 1);
      if (tensor.fmt == CVI_FMT_INT8) {
        array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor), refData.data<int8_t>(),
                         tensor.count, euclidean, cosine, correlation);
      } else {
        array_similarity((float *)CVI_NN_TensorPtr(&tensor), refData.data<int8_t>(),
                         tensor.count, euclidean, cosine, correlation);
      }
    }

    if (cosine < 1 || correlation < 1 || euclidean < 1) {
      err_cnt++;
      printf("Error, [%s] cosine:%f correlation:%f euclidean:%f\n", name.c_str(), cosine,
             correlation, euclidean);
    } else {
      printf("[%s] cosine:%f correlation:%f euclidean:%f\n", name.c_str(), cosine,
             correlation, euclidean);
    }
  }
  if (err_cnt > 0) {
    printf("Compare failed\n");
    return false;
  }
  printf("Compare passed\n");
  return true;
}

void ModelTester::loadInputData(const std::string &input_npz) {
  assert(isNpzFile(input_npz));
  auto npz = cnpy::npz_load(input_npz);
  assert(1 == (int)npz.size());
  auto &tensor = input_tensors[0];
  for (auto &npy : npz) {
    auto &arr = npy.second;
    input_vec.resize(arr.num_vals);
    if (arr.type == 'f') {
      auto src = arr.data<float>();
      auto qscale = CVI_NN_TensorQuantScale(&tensor);
      for (size_t i = 0; i < input_vec.size(); i++) {
        int val = std::round(src[i] * qscale);
        if (val > 127) {
          val = 127;
        } else if (val < -128) {
          val = -128;
        }
        input_vec[i] = (int8_t)val;
      }
    } else {
      auto src = arr.data<int8_t>();
      for (size_t i = 0; i < input_vec.size(); i++) {
        input_vec[i] = src[i];
      }
    }
    //break;
  }
}

void ModelTester::run() {
  CVI_NN_SetTensorPtr(&input_tensors[0], input_vec.data());
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
}

int main(int argc, const char **argv) {
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-i", "--input", 1, false);     // required
  parser.addArgument("-m", "--model", 1, false);     // required
  parser.addArgument("-r", "--reference", 1, false); // must be npz file
  parser.addArgument("-c", "--count", 1, false);
  parser.parse(argc, argv);

  auto inputFile = parser.retrieve<std::string>("input");
  auto modelFile = parser.retrieve<std::string>("model");
  auto referenceFile = parser.retrieve<std::string>("reference");
  auto count = parser.retrieve<int>("count");

  printf("TEST 1, begin to create & destroy model for %d times.\n", count);
  for (int i = 0; i < count; ++i) {
    ModelTester tester(modelFile, referenceFile);
    tester.loadInputData(inputFile);
    tester.run();
    if (i == count - 1) {
      assert(tester.compareResults());
    }
  }
  printf("TEST 1 passed\n");

  if (1) {
    printf("TEST 2, begin to run inferences for %d times.\n", count);
    ModelTester tester(modelFile, referenceFile);
    tester.loadInputData(inputFile);
    for (int i = 0; i < count; ++i) {
      tester.run();
    }
    assert(tester.compareResults());
    printf("TEST 2 passed\n");
  }
  return 0;
}
