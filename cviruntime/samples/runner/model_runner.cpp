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
#include <cviruntime.h>
#include <cvitpu_debug.h>
#include "argparse.hpp"
#include "similarity.hpp"
#include "cnpy.h"
#include "assert.h"

static std::string optInputFile;
static std::string optModelFile;
static std::string optOutputFile;
static std::string optSetChipName;
static int32_t optProgramId = 0;
static int32_t optInferenceCount = 1;
static int32_t optTimerRunCount = 1;
static bool optDumpAllTensors = false;
static bool optAsyncForward = false;
static bool optEnableTimer = false;
static float optCosineTolerance = 1.0f;
static float optCorrelationTolerance = 1.0f;
static float optEuclideanTolerance = 1.0f;

#define EXIT_IF_ERROR(cond, statement)                                                   \
  if ((cond)) {                                                                          \
    TPU_LOG_ERROR("%s\n", statement);                                                         \
    exit(1);                                                                             \
  }

static const char* formatToStr(CVI_FMT fmt) {
  switch(fmt) {
    case CVI_FMT_FP32:  return "fp32";
    case CVI_FMT_INT32:  return "i32";
    case CVI_FMT_UINT32: return "u32";
    case CVI_FMT_BF16:   return "bf16";
    case CVI_FMT_INT16:  return "i16";
    case CVI_FMT_UINT16: return "u16";
    case CVI_FMT_INT8:   return "i8";
    case CVI_FMT_UINT8:  return "u8";
    default:
      TPU_LOG_FATAL("unknown fmt:%d\n", fmt);
  }
  return nullptr;
}

static std::ifstream *openFile(const std::string &name, size_t &size) {
  auto *f = new std::ifstream(name, std::ios::binary);
  if (!f->is_open()) {
    TPU_LOG_ERROR("Error, failed to open %s\n", name.c_str());
    return nullptr;
  }
  f->seekg(0, std::ios::end);
  size = f->tellg();
  f->seekg(0, std::ios::beg);
  return f;
}

static bool isNpzFile(const std::string &name) {
  std::string extension = name.substr(name.size() - 4);
  if (extension == ".npz")
    return true;
  return false;
}

static bool compareResultWithNpz(CVI_TENSOR *tensors, int32_t num, cnpy::npz_t &reference) {
  float euclidean = 0;
  float cosine = 0;
  float correlation = 0;
  for (int i = 0; i < num; i++) {
    auto &tensor = tensors[i];
    std::string name(tensor.name);
    if (reference.find(name) == reference.end()) {
      TPU_LOG_WARNING("Warning, Cannot find %s in reference\n", name.c_str());
      continue;
    }
    auto &refData = reference[name];
    if (tensor.count != refData.num_vals) {
      TPU_LOG_ERROR("%s %zu vs %zu, size are not equal.\n", name.c_str(), tensor.count, refData.num_vals);
      return false;
    }

    if (refData.type == 'f') {
      if (tensor.fmt == CVI_FMT_INT8) {
        array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_UINT8) {
        array_similarity((uint8_t *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_BF16) {
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor), refData.data<float>(),
                         tensor.count, euclidean, cosine, correlation);
      } else {
        array_similarity((float *)CVI_NN_TensorPtr(&tensor), refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      }
    } else if (refData.type == 'u') {
      if (tensor.fmt == CVI_FMT_BF16) {
        assert(refData.word_size == 2);
        array_similarity((uint16_t *)CVI_NN_TensorPtr(&tensor), refData.data<uint16_t>(),
                         tensor.count, euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_UINT8) {
        assert(refData.word_size == 1);
        array_similarity((uint8_t *)CVI_NN_TensorPtr(&tensor), refData.data<uint8_t>(),
                         tensor.count, euclidean, cosine, correlation);
      }
    } else if (refData.type == 'i') {
      assert(refData.word_size == 1);
      assert(tensor.fmt == CVI_FMT_INT8);
      array_similarity((int8_t *)CVI_NN_TensorPtr(&tensor), refData.data<int8_t>(),
                        tensor.count, euclidean, cosine, correlation);
    }

    if (cosine < optCosineTolerance || correlation < optCorrelationTolerance ||
        euclidean < optEuclideanTolerance) {

      printf("Error, [%s] cosine:%f correlation:%f euclidean:%f\n", name.c_str(), cosine,
             correlation, euclidean);
      return false;
    }
  }
  TPU_LOG_INFO("Compare pass.\n");
  return true;
}

static void saveResultToNpz(const std::string &name, CVI_TENSOR *tensors, int32_t num) {
  assert(isNpzFile(name) && "output should be a npz file");

  cnpy::npz_t npz;
  for (int i = 0; i < num; i++) {
    auto &tensor = tensors[i];
    std::vector<size_t> shape = {(size_t)tensor.shape.dim[0], (size_t)tensor.shape.dim[1],
                                 (size_t)tensor.shape.dim[2],
                                 (size_t)tensor.shape.dim[3]};
    switch (tensor.fmt) {
      case CVI_FMT_FP32:
        cnpy::npz_add_array<float>(npz, tensor.name, (float *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      case CVI_FMT_BF16: // we use uint16_t to represent BF16
        cnpy::npz_add_array<uint16_t>(npz, tensor.name, (uint16_t *)CVI_NN_TensorPtr(&tensor),
                                      shape);
        break;
      case CVI_FMT_INT8:
        cnpy::npz_add_array<int8_t>(npz, tensor.name, (int8_t *)CVI_NN_TensorPtr(&tensor),
                                    shape);
        break;
      case CVI_FMT_UINT8:
        cnpy::npz_add_array<uint8_t>(npz, tensor.name, (uint8_t *)CVI_NN_TensorPtr(&tensor),
                                    shape);
        break;
      default:
        TPU_LOG_ERROR("Error, Current unsupported type:%d\n", tensor.fmt);
        assert(0);
    }
  }
  cnpy::npz_save_all(name, npz);
}

static void ConvertFp32ToInt8(float *src, int8_t *dst, int count,
                              float qscale, int zero_point = 0) {
  for (int i = 0; i < count; i++) {
    int val = std::round((*src++) * qscale) + zero_point;
    if (val > 127) {
      val = 127;
    } else if (val < -128) {
      val = -128;
    }
    *dst++ = (int8_t)val;
  }
}

static void ConvertFp32ToUint8(float *src, uint8_t *dst, int count,
                               float qscale, int zero_point = 0) {
  for (int i = 0; i < count; i++) {
    int val = std::round((*src++) * qscale) + zero_point;
    if (val > 255) {
      val = 255;
    }
    *dst++ = (uint8_t)val;
  }
}

static void ConvertFp32ToBf16(float *src, uint16_t *dst, int count) {
  for (int i = 0; i < count; ++i) {
    auto fval = src[i];
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    dst[i] = ((uint16_t *)&fval)[0];
#else
    dst[i] = ((uint16_t *)&fval)[1];
#endif
  }
}

static void loadInput(std::string &input_file, CVI_TENSOR *tensors, int num) {
  assert(isNpzFile(input_file) && "input should be npz file");

  cnpy::npz_t input_npz = cnpy::npz_load(input_file);
  EXIT_IF_ERROR(!input_npz.size(), "cannot open input npz file");
  assert(num == (int)input_npz.size());

  int idx = 0;
  for (auto &npy : input_npz) {
    auto &arr = npy.second;
    auto &tensor = tensors[idx++];
    if (arr.type == 'f' && tensor.fmt == CVI_FMT_INT8) {
      assert(arr.num_vals == tensor.mem_size);
      ConvertFp32ToInt8(
          arr.data<float>(),
          (int8_t *)CVI_NN_TensorPtr(&tensor),
          CVI_NN_TensorCount(&tensor),
          CVI_NN_TensorQuantScale(&tensor));
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_UINT8) {
      assert(arr.num_vals == tensor.mem_size);
      ConvertFp32ToUint8(
          arr.data<float>(),
          (uint8_t *)CVI_NN_TensorPtr(&tensor),
          CVI_NN_TensorCount(&tensor),
          CVI_NN_TensorQuantScale(&tensor));
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_BF16) {
      assert(arr.num_vals == tensor.count);
      ConvertFp32ToBf16(
          arr.data<float>(),
          (uint16_t *)CVI_NN_TensorPtr(&tensor),
          CVI_NN_TensorCount(&tensor));
    } else {
      if (arr.num_bytes() != tensor.mem_size){
        std::stringstream err;
        err << "arr.num_bytes: (" << arr.num_bytes()
            << ")not same as mem.size: (" << tensor.mem_size << ")\n";
        throw std::runtime_error(err.str());
      }
      memcpy(CVI_NN_TensorPtr(&tensor), arr.data<uint8_t>(), tensor.mem_size);
    }
  }
}

int main(int argc, const char **argv) {
  argparse::ArgumentParser parser;
  parser.addArgument("-i", "--input", 1, false); // required
  parser.addArgument("-m", "--model", 1, false);   // required
  parser.addArgument("-o", "--output", 1, false);  // required
  parser.addArgument("-p", "--pmu", 1);
  parser.addArgument("-s", "--program-id", 1); // select program by id
  parser.addArgument("-b", "--batch-num", 1);  // deprecated
  parser.addArgument("-c", "--count", 1);      // inference count
  parser.addArgument("-r", "--reference", 1);  // must be npz file
  parser.addArgument("-t", "--tolerances", 1); // cosine_tol,correlation_tol,euclidean_tol
  parser.addArgument("-v", "--verbose", 1); // set verbose level, 0: only error & warning, 1: info, 2: debug
  parser.addArgument("--dump-all-tensors");
  parser.addArgument("--async-forward");
  parser.addArgument("--skip-preprocess");
  parser.addArgument("--enable-timer");
  parser.parse(argc, argv);

  optInputFile = parser.retrieve<std::string>("input");
  optModelFile = parser.retrieve<std::string>("model");
  optOutputFile = parser.retrieve<std::string>("output");
  cnpy::npz_t ref_npz;

  if (parser.gotArgument("pmu")) {
    std::string pmu = parser.retrieve<std::string>("pmu");
    setenv("TPU_PMUBUF_OUTPUT_FILE", pmu.c_str(), true);
  }
  if (parser.gotArgument("dump-all-tensors")) {
    optDumpAllTensors = true;
  }
  if (parser.gotArgument("async-forward")) {
    optAsyncForward = true;
  }
  if (parser.gotArgument("enable-timer")) {
    optEnableTimer = true;
  }
  if (parser.gotArgument("count")) {
    if (optEnableTimer) {
      optTimerRunCount = parser.retrieve<int>("count");
    } else {
      optInferenceCount = parser.retrieve<int>("count");
    }
  }
  if (parser.gotArgument("program-id")) {
    optProgramId = parser.retrieve<int>("program-id");
  }
  if (parser.gotArgument("reference")) {
    auto name = parser.retrieve<std::string>("reference");
    assert(isNpzFile(name));
    ref_npz = cnpy::npz_load(name);
    EXIT_IF_ERROR(!ref_npz.size(), "cannot open reference npz file");
  }

  if (parser.gotArgument("tolerances")) {
    std::istringstream option(parser.retrieve<std::string>("tolerances"));
    std::vector<std::string> tolerances;
    std::string tol;
    while (std::getline(option, tol, ',')) {
      tolerances.push_back(std::move(tol));
    }
    assert(tolerances.size() == 3);
    optCosineTolerance = std::stof(tolerances[0]);
    optCorrelationTolerance = std::stof(tolerances[1]);
    optEuclideanTolerance = std::stof(tolerances[2]);
    printf("Tolerance, cosine:%f, correlation:%f, euclidean:%f\n", optCosineTolerance,
           optCorrelationTolerance, optEuclideanTolerance);
  }

  CVI_MODEL_HANDLE model = NULL;
  CVI_RC ret = CVI_NN_RegisterModel(optModelFile.c_str(), &model);
  EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "failed to register cvimodel");

  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, optProgramId);
  CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, optDumpAllTensors);

  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                               &output_tensors, &output_num);
  EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "failed to get inputs & outputs from model");

  // print the inputs & outputs's information
  if (1) {
    TPU_LOG_INFO("Inputs:\n");
    for (int i = 0; i < input_num; ++i) {
      auto &tensor = input_tensors[i];
      TPU_LOG_INFO("  [%d] %s <%d,%d,%d,%d>,%s\n",
                   i, tensor.name, tensor.shape.dim[0], tensor.shape.dim[1], tensor.shape.dim[2],
                   tensor.shape.dim[3], formatToStr(tensor.fmt));
    }
    TPU_LOG_INFO("Outputs:\n");
    for (int i = 0; i < output_num; ++i) {
      auto &tensor = output_tensors[i];
      TPU_LOG_INFO("  [%d] %s <%d,%d,%d,%d>,%s\n",
                   i, tensor.name, tensor.shape.dim[0], tensor.shape.dim[1], tensor.shape.dim[2],
                   tensor.shape.dim[3], formatToStr(tensor.fmt));
    }
  }

  loadInput(optInputFile, input_tensors, input_num);

  int fail_cnt = 0;
  for (int i = 0; i < optInferenceCount; ++i) {
    if (optAsyncForward) {
      void *task = nullptr;
      ret = CVI_NN_ForwardAsync(model, input_tensors, input_num, output_tensors,
                                output_num, &task);
      EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "async forward failed");

      ret = CVI_NN_ForwardWait(model, task);
      EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward wait failed");

    } else {
      ret = CVI_NN_Forward(model, input_tensors, input_num, output_tensors,
                           output_num);
      EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward failed");
    }
    if (ref_npz.size() && !compareResultWithNpz(output_tensors, output_num, ref_npz)) {
      fail_cnt++;
    }
  }

  if (optEnableTimer) {
    struct timeval t0, t1;
    long elapsed;
    gettimeofday(&t0, NULL);

    for (int i = 0; i < optTimerRunCount; ++i) {
      if (optAsyncForward) {
        void *task = nullptr;
        ret = CVI_NN_ForwardAsync(model, input_tensors, input_num, output_tensors,
                                  output_num, &task);
        EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "async forward failed");

        ret = CVI_NN_ForwardWait(model, task);
        EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward wait failed");
      } else {
        ret = CVI_NN_Forward(model, input_tensors, input_num, output_tensors,
                             output_num);
        EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward failed");
      }
    }

    gettimeofday(&t1, NULL);
    elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
    double ms_per_iter = elapsed/optTimerRunCount/1000.0;
    double fps = 1000.0/ms_per_iter;
    std::cout << "Performance result: "
              << optTimerRunCount << " runs take "
              << elapsed/1000.0 << " ms, each run takes "
              << std::to_string(ms_per_iter) << " ms, fps "
              << std::to_string(fps) << std::endl;
  }

  saveResultToNpz(optOutputFile, output_tensors, output_num);

  CVI_NN_CleanupModel(model);

  if (ref_npz.size()) {
    std::cout << "Compare result: " << (optInferenceCount - fail_cnt) << "/"
              << optInferenceCount << " passed.\n";
    if (fail_cnt)
      return 1;
  }
  return 0;
}
