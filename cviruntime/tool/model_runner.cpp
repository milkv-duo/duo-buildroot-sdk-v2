#include "argparse.hpp"
#include "assert.h"
#include "cnpy.h"
#include "cviruntime.h"
#include "similarity.hpp"
#include <cviruntime_context.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <runtime/debug.h>
#include <runtime/version.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <vector>

static std::string optInputFile;
static std::string optModelFile;
static std::string optOutputFile;
static int32_t optProgramId = 0;
static int32_t optInferenceCount = 1;
static bool optEnableTimer = false;
static bool optDumpAllTensors = false;
static float optCosineTolerance = 0.99f;
static float optCorrelationTolerance = 0.99f;
static float optEuclideanTolerance = 0.90f;

#define EXIT_IF_ERROR(cond, statement)       \
  if ((cond)) {                              \
    printf("%s\n", statement);             \
    exit(1);                                 \
  }

static const char *formatToStr(CVI_FMT fmt) {
  switch (fmt) {
  case CVI_FMT_FP32: return "fp32";
  case CVI_FMT_INT32: return "i32";
  case CVI_FMT_UINT32: return "u32";
  case CVI_FMT_BF16: return "bf16";
  case CVI_FMT_INT16: return "i16";
  case CVI_FMT_UINT16: return "u16";
  case CVI_FMT_INT8: return "i8";
  case CVI_FMT_UINT8: return "u8";
  default:
    printf("unknown fmt:%d\n", fmt);
  }
  return nullptr;
}

static const char*
pixelFormatToStr(CVI_NN_PIXEL_FORMAT_E pixel_format) {
  switch(pixel_format) {
    case CVI_NN_PIXEL_RGB_PACKED: return "RGB_PACKED";
    case CVI_NN_PIXEL_BGR_PACKED: return "BGR_PACKED";
    case CVI_NN_PIXEL_RGB_PLANAR: return "RGB_PLANAR";
    case CVI_NN_PIXEL_BGR_PLANAR: return "BGR_PLANAR";
    case CVI_NN_PIXEL_YUV_420_PLANAR: return "YUV420_PLANAR";
    case CVI_NN_PIXEL_YUV_NV12: return "YUV_NV12";
    case CVI_NN_PIXEL_YUV_NV21: return "YUV_NV21";
    case CVI_NN_PIXEL_GRAYSCALE: return "GRAYSCALE";
    case CVI_NN_PIXEL_TENSOR: return "TENSOR";
    case CVI_NN_PIXEL_RGBA_PLANAR: return "RGBA_PLANAR";
    default:
      printf("unknown pixel format:%d\n", pixel_format);
  }
  return nullptr;
}

static bool isNpzFile(const std::string &name) {
  std::string extension = name.substr(name.size() - 4);
  if (extension == ".npz")
    return true;
  return false;
}

static int8_t *readFileToMemory(const std::string &fileName, size_t &size) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  size = file.tellg();
  file.seekg(0, std::ios::beg);
  char *buf = new char[size];
  if (!buf) {
    printf("failed to allocate memory, size:%zu\n", size);
    return nullptr;
  }
  file.read(buf, size);
  return (int8_t *)buf;
}

static bool compareResultWithNpz(CVI_TENSOR *tensors, int32_t num,
                                 std::string &ref_npz) {
  float euclidean = 0;
  float cosine = 0;
  float correlation = 0;
  int32_t errCnt = 0;
  for (int i = 0; i < num; i++) {
    auto &tensor = tensors[i];
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
      } else if (tensor.fmt == CVI_FMT_INT16) {
        array_similarity((int16_t *)CVI_NN_TensorPtr(&tensor),
                         refData.data<float>(), tensor.count,
                         euclidean, cosine, correlation);
      } else if (tensor.fmt == CVI_FMT_UINT16) {
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

    if (cosine < optCosineTolerance || correlation < optCorrelationTolerance ||
        euclidean < optEuclideanTolerance) {
      printf("Error, [%s] cosine:%f correlation:%f euclidean:%f\n",
                    name.c_str(), cosine, correlation, euclidean);
      errCnt++;
    } else {
      printf("Pass, [%s] cosine:%f correlation:%f euclidean:%f\n",
                    name.c_str(), cosine, correlation, euclidean);
    }
  }
  if (errCnt) {
    printf("Compare Failed.\n");
    return false;
  }
  printf("Compare Pass.\n");
  return true;
}

static void saveResultToNpz(const std::string &name, CVI_TENSOR *tensors,
                            int32_t num) {
  assert(isNpzFile(name) && "output should be a npz file");

  cnpy::npz_t npz;
  for (int i = 0; i < num; i++) {
    auto &tensor = tensors[i];
    std::vector<size_t> shape = {
        (size_t)tensor.shape.dim[0], (size_t)tensor.shape.dim[1],
        (size_t)tensor.shape.dim[2], (size_t)tensor.shape.dim[3]};
    switch (tensor.fmt) {
      case CVI_FMT_FP32:
        cnpy::npz_add_array<float>(
            npz, tensor.name,
            (float *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      case CVI_FMT_UINT16: {
        // uint16 format is used in numpy to store bf16 data.
        // so we use float type to represent uint16
        auto size = CVI_NN_TensorCount(&tensor);
        auto ptr = (uint16_t *)CVI_NN_TensorPtr(&tensor);
        std::vector<float> tmp(size);
        for (size_t i = 0; i < size; ++i) {
          tmp[i] = (float)ptr[i];
        }
        cnpy::npz_add_array<float>(
            npz, tensor.name, tmp.data(), shape);
        break;
      }
      case CVI_FMT_INT16:
        cnpy::npz_add_array<int16_t>(
            npz, tensor.name,
            (int16_t *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      case CVI_FMT_BF16: // we use uint16_t to represent BF16
        cnpy::npz_add_array<uint16_t>(
            npz, tensor.name,
            (uint16_t *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      case CVI_FMT_INT8:
        if (CVI_NN_TensorCount(&tensor) != CVI_NN_TensorSize(&tensor)) {
          shape[1] = shape[2] = 1;
          shape[3] = CVI_NN_TensorSize(&tensor) / shape[0];
        }
        cnpy::npz_add_array<int8_t>(
            npz, tensor.name,
            (int8_t *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      case CVI_FMT_UINT8:
        if (CVI_NN_TensorCount(&tensor) != CVI_NN_TensorSize(&tensor)) {
          shape[1] = shape[2] = 1;
          shape[3] = CVI_NN_TensorSize(&tensor) / shape[0];
        }
        cnpy::npz_add_array<uint8_t>(
            npz, tensor.name,
            (uint8_t *)CVI_NN_TensorPtr(&tensor), shape);
        break;
      default:
        printf("Error, Current unsupported type:%d\n", tensor.fmt);
        assert(0);
    }
  }
  cnpy::npz_save_all(name, npz);
}

static void ConvertFp32ToInt8(float *src, int8_t *dst, int count, float qscale,
                              int zero_point = 0) {
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

static void ConvertFp32ToInt16(float *src, int16_t *dst, int count) {
  for (int i = 0; i < count; i++) {
    int val = std::round((*src++));
    *dst++ = (int16_t)val;
  }
}

static void ConvertFp32ToUInt16(float *src, uint16_t *dst, int count) {
  for (int i = 0; i < count; i++) {
    int val = std::round((*src++));
    *dst++ = (uint16_t)val;
  }
}

static void ConvertFp32ToBf16(float *src, uint16_t *dst, int size,
                              bool rounding) {
  // const uint16_t* p = reinterpret_cast<const uint16_t*>(src);
  const uint16_t *p = nullptr;
  /// if rounding is prefered than trancating
  /// float_val *= 1.001957f;
  float *src_round = nullptr;
  if (rounding) {
    src_round = (float *)malloc(size * sizeof(float));
    for (int i = 0; i < size; i++) {
      float value = src[i];
      uint32_t *u32_val = reinterpret_cast<uint32_t *>(&value);
      uint32_t lsb = (*u32_val >> 16) & 1;
      *u32_val += (0x7fff + lsb); // rounding_bias
      float *ret = reinterpret_cast<float *>(u32_val);
      src_round[i] = *ret;
    }
    p = reinterpret_cast<const uint16_t *>(src_round);
  } else {
    p = reinterpret_cast<const uint16_t *>(src);
  }

  uint16_t *q = reinterpret_cast<uint16_t *>(dst);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  for (; size != 0; p += 2, q++, size--) {
    *q = p[0];
    /* HW behavior */
    // infinity set to max finite positive value
    if ((*q & 0x7f80) == 0x7f80) {
      *q = 0x7f7f;
    }
  }
#else
  for (; size != 0; p += 2, q++, size--) {
    *q = p[1];
    /* HW behavior */
    // infinity set to max finite positive value
    if ((*q & 0x7f80) == 0x7f80) {
      *q = 0x7f7f;
    }
  }
#endif
  if (rounding) {
    free(src_round);
  }
}

static void loadInput(std::string &input_file, CVI_TENSOR *tensors, int num) {
  assert(isNpzFile(input_file) && "input should be a npz file");

  cnpy::npz_t input_npz = cnpy::npz_load(input_file);
  EXIT_IF_ERROR(!input_npz.size(), "cannot open input npz file");
  assert(num == (int)input_npz.size());

  for (auto &npy : input_npz) {
    auto &arr = npy.second;
    auto name = npy.first.c_str();
    int idx = 0;
    // search the target tensor, if not found, use the first one.
    for (int i = 0; i < num; i++) {
      if (strncmp((char *)name, tensors[i].name, strlen(name)) == 0) {
        idx = i;
        break;
      }
    }
    auto &tensor = tensors[idx];
    if (arr.type == 'f' && tensor.fmt == CVI_FMT_INT8) {
      assert(arr.num_vals == tensor.mem_size);
      ConvertFp32ToInt8(arr.data<float>(),
                        (int8_t *)CVI_NN_TensorPtr(&tensor),
                        CVI_NN_TensorSize(&tensor),
                        CVI_NN_TensorQuantScale(&tensor),
                        CVI_NN_TensorQuantZeroPoint(&tensor));
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_UINT8) {
      assert(arr.num_vals == tensor.mem_size);
      ConvertFp32ToUint8(arr.data<float>(),
                        (uint8_t *)CVI_NN_TensorPtr(&tensor),
                        CVI_NN_TensorSize(&tensor),
                        CVI_NN_TensorQuantScale(&tensor),
                        CVI_NN_TensorQuantZeroPoint(&tensor));
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_BF16) {
      assert(arr.num_vals == tensor.count);
      ConvertFp32ToBf16(arr.data<float>(),
                        (uint16_t *)CVI_NN_TensorPtr(&tensor),
                        CVI_NN_TensorCount(&tensor), false);
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_INT16) {
      assert(arr.num_vals == tensor.count);
      ConvertFp32ToInt16(arr.data<float>(),
                        (int16_t *)CVI_NN_TensorPtr(&tensor),
                        CVI_NN_TensorCount(&tensor));
    } else if (arr.type == 'f' && tensor.fmt == CVI_FMT_UINT16) {
      assert(arr.num_vals == tensor.count);
      ConvertFp32ToUInt16(arr.data<float>(),
                        (uint16_t *)CVI_NN_TensorPtr(&tensor),
                        CVI_NN_TensorCount(&tensor));
    } else {
      if (arr.num_bytes() != tensor.mem_size) {
        std::stringstream err;
        err << "arr.num_bytes: (" << arr.num_bytes()
            << ")not same as mem.size: (" << tensor.mem_size << ")\n";
        throw std::runtime_error(err.str());
      }
      memcpy(CVI_NN_TensorPtr(&tensor), arr.data<uint8_t>(), tensor.mem_size);
    }
  }
}

static void dumpTensorsInfo(
    CVI_TENSOR *input_tensors, int32_t input_num,
    CVI_TENSOR *output_tensors, int32_t output_num) {

  printf("Inputs:\n");
  for (int i = 0; i < input_num; ++i) {
    auto &tensor = input_tensors[i];
    if (tensor.pixel_format != CVI_NN_PIXEL_TENSOR) {
      printf("  [%d] %s <%d,%d,%d,%d>,%s, qscale:%f, zero_point: %d\n",
                   i, tensor.name, tensor.shape.dim[0], tensor.shape.dim[1], tensor.shape.dim[2],
                   tensor.shape.dim[3], formatToStr(tensor.fmt), tensor.qscale, tensor.zero_point);
      printf("           pixel_format:%s, aligned:%s\n",
                   pixelFormatToStr(tensor.pixel_format), tensor.aligned ? "True" : "False");
      printf("           scale:<%f, %f, %f>, mean:<%f, %f, %f>\n",
                   tensor.scale[0], tensor.scale[1], tensor.scale[2],
                   tensor.mean[0], tensor.mean[1], tensor.mean[2]);
    } else {
      printf("  [%d] %s <%d,%d,%d,%d>,%s, qscale:%f, zero_point: %d\n",
                   i, tensor.name, tensor.shape.dim[0], tensor.shape.dim[1], tensor.shape.dim[2],
                   tensor.shape.dim[3], formatToStr(tensor.fmt), tensor.qscale, tensor.zero_point);
    }
  }
  printf("Outputs:\n");
  for (int i = 0; i < output_num; ++i) {
    auto &tensor = output_tensors[i];
    printf("  [%d] %s <%d,%d,%d,%d>,%s\n",
                 i, tensor.name, tensor.shape.dim[0], tensor.shape.dim[1], tensor.shape.dim[2],
                 tensor.shape.dim[3], formatToStr(tensor.fmt));
  }
}

int main(int argc, const char **argv) {
  showRuntimeVersion();

  argparse::ArgumentParser parser;
  parser.addArgument("-i", "--input", 1); // required
  parser.addArgument("-m", "--model", 1, false); // required
  parser.addArgument("-o", "--output", 1);       // required
  parser.addArgument("-p", "--pmu", 1);
  parser.addArgument("-s", "--program-id", 1); // select program by id
  parser.addArgument("-b", "--batch-num", 1);  // deprecated
  parser.addArgument("-c", "--count", 1);     // inference count
  parser.addArgument("-r", "--reference", 1); // must be npz file
  // cosine_tol,correlation_tol,euclidean_tol
  parser.addArgument("-t", "--tolerances", 1);
  // set verbose level, 0: only error & warning, 1: info, 2: debug
  parser.addArgument("-v", "--verbose", 1);
  parser.addArgument("--dump-all-tensors");
  parser.addArgument("--load-from-memory");
  parser.addArgument("--enable-timer");
  parser.parse(argc, argv);

  if (parser.gotArgument("input")) {
    optInputFile = parser.retrieve<std::string>("input");
  }
  optModelFile = parser.retrieve<std::string>("model");
  std::string ref_fname;

  if (parser.gotArgument("output")) {
    optOutputFile = parser.retrieve<std::string>("output");
  }
  if (parser.gotArgument("pmu")) {
    std::string pmu = parser.retrieve<std::string>("pmu");
    setenv("TPU_PMUBUF_OUTPUT_FILE", pmu.c_str(), true);
  }
  if (parser.gotArgument("dump-all-tensors")) {
    optDumpAllTensors = true;
  }
  if (parser.gotArgument("count")) {
    optInferenceCount = parser.retrieve<int>("count");
  }
  if (parser.gotArgument("program-id")) {
    optProgramId = parser.retrieve<int>("program-id");
  }
  if (parser.gotArgument("reference")) {
    auto name = parser.retrieve<std::string>("reference");
    assert(isNpzFile(name));
    ref_fname = name;
    EXIT_IF_ERROR(!ref_fname.size(), "reference npz file name is empty");
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
    printf("Tolerance, cosine:%f, correlation:%f, euclidean:%f\n",
           optCosineTolerance, optCorrelationTolerance,
           optEuclideanTolerance);
  }
  if (parser.gotArgument("enable-timer")) {
    optEnableTimer = true;
  }

  CVI_MODEL_HANDLE model = NULL;
  CVI_RC ret;
  if (parser.gotArgument("load-from-memory")) {
    size_t size = 0;
    int8_t *buffer = readFileToMemory(optModelFile, size);
    EXIT_IF_ERROR(!buffer, "failed to read civmodel file to memory");
    ret = CVI_NN_RegisterModelFromBuffer(buffer, size, &model);
    delete[] buffer;
    printf("load cvimodel from memory\n");
  } else {
    ret = CVI_NN_RegisterModel(optModelFile.c_str(), &model);
  }
  EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "failed to register cvimodel");

  int major_ver, minor_ver;
  CVI_NN_GetModelVersion(model, &major_ver, &minor_ver);
  printf("cvimodel's version:%d.%d\n", major_ver ,minor_ver);

  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, optProgramId);
  CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, optDumpAllTensors);

  CVI_TENSOR *input_tensors, *output_tensors;
  int32_t input_num, output_num;
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "get input output tensors failed");

  dumpTensorsInfo(input_tensors, input_num,
                  output_tensors, output_num);
  if (optInputFile.empty() == false) {
    loadInput(optInputFile, input_tensors, input_num);
  }

  int err = 0;
  if (optEnableTimer) {
    struct timeval t0, t1;
    long elapsed;
    gettimeofday(&t0, NULL);

    for (int i = 0; i < optInferenceCount; ++i) {
      ret = CVI_NN_Forward(model, input_tensors, input_num,
                           output_tensors, output_num);
      EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward failed");
    }

    gettimeofday(&t1, NULL);
    elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
    double ms_per_iter = elapsed/optInferenceCount/1000.0;
    double fps = 1000.0/ms_per_iter;
    std::cout << "Performance result: "
              << optInferenceCount << " runs take "
              << elapsed/1000.0 << " ms, each run takes "
              << std::to_string(ms_per_iter) << " ms, fps "
              << std::to_string(fps) << std::endl;
  } else {
    int fail_cnt = 0;
    for (int i = 0; i < optInferenceCount; ++i) {
      ret = CVI_NN_Forward(model, input_tensors, input_num, output_tensors,
                          output_num);
      EXIT_IF_ERROR(ret != CVI_RC_SUCCESS, "forward failed");
      if (ref_fname.size() &&
          !compareResultWithNpz(output_tensors, output_num, ref_fname)) {
        fail_cnt++;
      }
    }
    if (ref_fname.size()) {
      std::cout << "Compare result: " << (optInferenceCount - fail_cnt) << "/"
                << optInferenceCount << " passed.\n";
      if (fail_cnt)
        err = 1;
    }
  }

  if (!optOutputFile.empty()) {
    saveResultToNpz(optOutputFile, output_tensors, output_num);
  }

  CVI_NN_CleanupModel(model);

  return err;
}
