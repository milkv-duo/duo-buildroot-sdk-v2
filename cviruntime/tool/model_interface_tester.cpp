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
  void prepareAlignedData();
  void testEmulateSendDataFromSystemMem();
  void testEmulateSendDataFromVpss();

private:
  bool compareResults();
  void forwardAndCompareResults(std::string api);

  CVI_RT_HANDLE ctx;
  CVI_MODEL_HANDLE model = NULL;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  int32_t _n, _c, _h, _w;
  int32_t _aligned_w;
  bool _nhwc = false;

  std::vector<int8_t> origin_input;
  std::vector<int8_t> aligned_input;
  std::string ref_npz;
};

ModelTester::ModelTester(const std::string &model_file, const std::string &ref_npz)
  : ref_npz(ref_npz) {
  _n = _c = _h = _w = _aligned_w = 0;

  CVI_RT_Init(&ctx);

  if (CVI_RC_SUCCESS != CVI_NN_RegisterModel(model_file.c_str(), &model)) {
    exit(1);
  }

  CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, true);

  CVI_NN_GetInputOutputTensors(
      model, &input_tensors, &input_num,
      &output_tensors, &output_num);
}

ModelTester::~ModelTester() {
  CVI_NN_CleanupModel(model);
  CVI_RT_DeInit(ctx);
}


bool ModelTester::compareResults() {
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
      printf("%s %zu vs %zu, size are not equal.\n", name.c_str(), tensor.count, refData.num_vals);
      continue;
    }

    if (refData.type == 'f') {
      if (tensor.fmt == CVI_FMT_INT8) {
        array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_UINT8) {
        array_similarity((uint8_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_BF16) {
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      } else {
        array_similarity((float *)CVI_NN_TensorPtr(&tensor),
                         refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      }
    } else if (refData.type == 'u') {
      if (tensor.fmt == CVI_FMT_BF16) {
        assert(refData.word_size == 2);
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<uint16_t>(), tensor.count, euclidean,
                         cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_UINT8) {
        assert(refData.word_size == 1);
        array_similarity((uint8_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<uint8_t>(), tensor.count, euclidean,
                         cosine, correlation);
      } else {
        assert(0);
      }
    } else if (refData.type == 'i') {
      assert(refData.word_size == 1);
      assert(tensor.fmt == CVI_FMT_INT8);
      array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor),
                        refData.data<int8_t>(), tensor.count, euclidean,
                        cosine, correlation);
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
  auto &arr = npz.begin()->second;
  auto size = CVI_NN_TensorSize(&tensor);
  assert(arr.num_vals == size);

  origin_input.resize(size);
  if (arr.type == 'f') {
    auto src = arr.data<float>();
    auto qscale = CVI_NN_TensorQuantScale(&tensor);
    for (size_t i = 0; i < arr.num_vals; i++) {
      int val = std::round(src[i] * qscale);
      if (tensor.fmt == CVI_FMT_INT8) {
        if (val > 127) {
          val = 127;
        } else if (val < -128) {
          val = -128;
        }
        origin_input[i] = (int8_t)val;
      } else {
        if (val > 255) {
          val = 255;
        }
        origin_input[i] = static_cast<int8_t>(val);
      }
    }
  } else {
    auto src = arr.data<int8_t>();
    for (size_t i = 0; i < arr.num_vals; i++) {
      origin_input[i] = static_cast<int8_t>(src[i]);
    }
  }
}

static inline int align_up(int x, int n) {
  return ((x + n - 1) / n) * n;
}

void ModelTester::prepareAlignedData() {
  auto &tensor = input_tensors[0];
  if (tensor.aligned) {
    aligned_input.assign(origin_input.begin(),
                         origin_input.end());
  }
  _n = tensor.shape.dim[0];
  _c = tensor.shape.dim[1];
  _h = tensor.shape.dim[2];
  _w = tensor.shape.dim[3];
  if (_w == 3) {
    _c = 1;
    _h = tensor.shape.dim[1];
    _w = tensor.shape.dim[2] * tensor.shape.dim[3];
    _nhwc = true;
  }

  _aligned_w = align_up(_w, 32);
  size_t aligned_size = (size_t)(_n * _c * _h * _aligned_w);
  auto dst = [&](int i) {
    return ((i / _w) * _aligned_w) + (i % _w);
  };
  aligned_input.resize(aligned_size);
  for (int i = 0; i < (int)origin_input.size(); i++) {
    aligned_input[dst(i)] = origin_input[i];
  }
}

void ModelTester::forwardAndCompareResults(std::string api) {
  std::cout << "\ntest " << api << "....\n";
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
  if (!compareResults()) {
    std::cout << "test " << api << " failed\n";
    exit(1);
  }
  std::cout << "test " << api << " successed\n";
}

void ModelTester::testEmulateSendDataFromVpss() {
  auto &tensor = input_tensors[0];

  uint8_t *ptr = (uint8_t *)aligned_input.data();
  auto devMem = CVI_RT_MemAlloc(ctx, aligned_input.size());
  auto paddr = (uint64_t)CVI_RT_MemGetPAddr(devMem);
  CVI_RT_MemCopyS2D(ctx, devMem, ptr);

  if (tensor.shape.dim[0] == 1) {
    CVI_VIDEO_FRAME_INFO frame;
    frame.type = tensor.pixel_format;
    for (int i = 0; i < _c; ++i) {
      frame.pyaddr[i] = paddr + i * _h * _aligned_w;
    }
    CVI_NN_SetTensorWithVideoFrame(nullptr, &tensor, &frame);
    forwardAndCompareResults("CVI_NN_SetTensorWithVideoFrame");

    if (!tensor.aligned) {
      int channel_num = _n * _c;
      uint64_t channel_paddrs[channel_num];
      for (int i = 0; i < channel_num; ++i) {
        channel_paddrs[i] = paddr + i * _h * _aligned_w;
      }
      CVI_NN_FeedTensorWithFrames(nullptr, &tensor, tensor.pixel_format,
                                  CVI_FMT_INT8, channel_num, channel_paddrs, 0, 0, 0);
      forwardAndCompareResults("CVI_NN_FeedTensorWithFrames");
    }
  }

  CVI_NN_SetTensorWithAlignedFrames(&tensor, &paddr, 1, tensor.pixel_format);
  forwardAndCompareResults("CVI_NN_SetTensorWithAlignedFrames");

  CVI_RT_MemFree(ctx, devMem);
}

void ModelTester::testEmulateSendDataFromSystemMem() {
  auto sys_mem = new uint8_t[origin_input.size()];
  memcpy(sys_mem, origin_input.data(), origin_input.size());
  CVI_NN_SetTensorPtr(&input_tensors[0], sys_mem);
  forwardAndCompareResults("CVI_NN_SetTensorPtr");
  delete[] sys_mem;
}

int main(int argc, const char **argv) {
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-i", "--input", 1, false);   // required
  parser.addArgument("-m", "--model", 1, false);     // required
  parser.addArgument("-r", "--reference", 1, false); // must be npz file
  parser.addArgument("-v", "--verbose",
                     1); // set verbose level, 0: only error & warning, 1: info, 2: debug
  parser.parse(argc, argv);

  auto inputFile = parser.retrieve<std::string>("input");
  auto modelFile = parser.retrieve<std::string>("model");
  auto referenceFile = parser.retrieve<std::string>("reference");

  ModelTester tester(modelFile, referenceFile);
  tester.loadInputData(inputFile);
  tester.prepareAlignedData();

  tester.testEmulateSendDataFromSystemMem();
  tester.testEmulateSendDataFromVpss();
  return 0;
}
