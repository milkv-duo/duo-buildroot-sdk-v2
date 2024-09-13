#include "feature_matching.hpp"
#include "core/core/cvtdl_errno.h"

#include <cvimath/cvimath_internal.h>
#include <cviruntime.h>
#include <string.h>
#ifndef CONFIG_ALIOS
#include <sys/sysinfo.h>
#else
#include <yoc/sysinfo.h>
#endif
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace cvitdl {
namespace service {

static const char *TypeToStr(feature_type_e type) {
  switch (type) {
    case TYPE_INT8:
      return "TYPE_INT8";
    case TYPE_UINT8:
      return "TYPE_UINT8";
    case TYPE_INT16:
      return "TYPE_INT16";
    case TYPE_UINT16:
      return "TYPE_UINT16";
    case TYPE_INT32:
      return "TYPE_INT32";
    case TYPE_UINT32:
      return "TYPE_UINT32";
    case TYPE_BF16:
      return "TYPE_BF16";
    case TYPE_FLOAT:
      return "TYPE_FLOAT";
    default:
      return "Unknown";
  }
}

inline void __attribute__((always_inline))
FreeFeatureArrayExt(cvtdl_service_feature_array_ext_t *feature_array_ext) {
  if (feature_array_ext->feature_unit_length != nullptr) {
    delete feature_array_ext->feature_unit_length;
    feature_array_ext->feature_unit_length = nullptr;
  }
  if (feature_array_ext->feature_array_buffer != nullptr) {
    delete feature_array_ext->feature_array_buffer;
    feature_array_ext->feature_array_buffer = nullptr;
  }
}

inline void __attribute__((always_inline))
FreeFeatureArrayTpuExt(CVI_RT_HANDLE rt_handle,
                       cvtdl_service_feature_array_tpu_ext_t *feature_array_ext) {
  if (feature_array_ext->feature_input.rtmem != NULL) {
    CVI_RT_MemFree(rt_handle, feature_array_ext->feature_input.rtmem);
    feature_array_ext->feature_input.rtmem = NULL;
  }
  if (feature_array_ext->feature_array.rtmem != NULL) {
    CVI_RT_MemFree(rt_handle, feature_array_ext->feature_array.rtmem);
    feature_array_ext->feature_array.rtmem = NULL;
  }
  if (feature_array_ext->buffer_array.rtmem != NULL) {
    CVI_RT_MemFree(rt_handle, feature_array_ext->buffer_array.rtmem);
    feature_array_ext->buffer_array.rtmem = NULL;
  }
  if (feature_array_ext->slice_num != nullptr) {
    delete feature_array_ext->slice_num;
    feature_array_ext->slice_num = nullptr;
  }
  if (feature_array_ext->feature_unit_length != nullptr) {
    delete feature_array_ext->feature_unit_length;
    feature_array_ext->feature_unit_length = nullptr;
  }
  if (feature_array_ext->array_buffer_32 != nullptr) {
    delete feature_array_ext->array_buffer_32;
    feature_array_ext->array_buffer_32 = nullptr;
  }
  if (feature_array_ext->array_buffer_f != nullptr) {
    delete feature_array_ext->array_buffer_f;
    feature_array_ext->array_buffer_f = nullptr;
  }
}

FeatureMatching::~FeatureMatching() {
  FreeFeatureArrayExt(&m_cpu_ipfeature);
  FreeFeatureArrayTpuExt(m_rt_handle, &m_tpu_ipfeature);
  destroyHandle(m_rt_handle, m_cvk_ctx);
}

int FeatureMatching::init() {
  m_tpu_ipfeature.array_buffer_32 = NULL;
  m_tpu_ipfeature.array_buffer_f = NULL;
  m_tpu_ipfeature.data_num = 0;
  m_tpu_ipfeature.feature_length = 0;
  m_tpu_ipfeature.feature_unit_length = 0;
  m_tpu_ipfeature.slice_num = NULL;

  m_cpu_ipfeature.feature_array.data_num = 0;
  m_cpu_ipfeature.feature_array.feature_length = 0;
  m_cpu_ipfeature.feature_array.ptr = NULL;
  m_cpu_ipfeature.feature_array_buffer = NULL;
  m_cpu_ipfeature.feature_unit_length = 0;
  m_is_cpu = true;
  return createHandle(&m_rt_handle, &m_cvk_ctx);
}

int FeatureMatching::createHandle(CVI_RT_HANDLE *rt_handle, cvk_context_t **cvk_ctx) {
  if (CVI_RT_Init(rt_handle) != CVI_SUCCESS) {
    LOGE("Runtime init failed.\n");
    return CVI_TDL_FAILURE;
  }
#ifndef CONFIG_ALIOS
  struct sysinfo info;
  if (sysinfo(&info) < 0) {
    return CVI_TDL_FAILURE;
  }
#endif
  // FIXME: Rewrite command buffer to fit feature matching size.
  uint64_t mem = 50000;
#ifndef CONFIG_ALIOS
  if (info.freeram <= mem) {
    LOGE("Memory insufficient.\n");
    return CVI_TDL_FAILURE;
  }
#endif
  *cvk_ctx = (cvk_context_t *)CVI_RT_RegisterKernel(*rt_handle, mem);
  return CVI_TDL_SUCCESS;
}

int FeatureMatching::destroyHandle(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);
  return CVI_TDL_SUCCESS;
}

int FeatureMatching::registerData(const cvtdl_service_feature_array_t &feature_array,
                                  const cvtdl_service_feature_matching_e &matching_method) {
  int ret = CVI_TDL_SUCCESS;
  m_matching_method = matching_method;
  switch (m_matching_method) {
    case COS_SIMILARITY: {
      ret = cosSimilarityRegister(feature_array);
    } break;
    default:
      LOGE("Unsupported matching method %u\n", m_matching_method);
      ret = CVI_TDL_ERR_INVALID_ARGS;
      break;
  }
  return ret;
}

int FeatureMatching::run(const void *feature, const feature_type_e &type, const uint32_t topk,
                         uint32_t *indices, float *scores, uint32_t *size, float threshold) {
  int ret = CVI_TDL_SUCCESS;
  switch (m_matching_method) {
    case COS_SIMILARITY: {
      ret = cosSimilarityRun(feature, type, topk, indices, scores, threshold, size);
    } break;
    default:
      LOGE("Unsupported matching method %u\n", m_matching_method);
      ret = CVI_TDL_ERR_INVALID_ARGS;
      break;
  }
  return ret;
}

int FeatureMatching::cosSimilarityRegister(const cvtdl_service_feature_array_t &feature_array) {
  const uint32_t total_length = feature_array.feature_length * feature_array.data_num;
  float *unit_length = new float[total_length];
  switch (feature_array.type) {
    case TYPE_INT8: {
      cvm_gen_precached_i8_unit_length((int8_t *)feature_array.ptr, unit_length,
                                       feature_array.feature_length, feature_array.data_num);
    } break;
    default: {
      LOGE("Unsupported register data type %s.\n", TypeToStr(feature_array.type));
      delete[] unit_length;
      return CVI_TDL_ERR_INVALID_ARGS;
    } break;
  }
  m_data_num = feature_array.data_num;
  FreeFeatureArrayExt(&m_cpu_ipfeature);
  FreeFeatureArrayTpuExt(m_rt_handle, &m_tpu_ipfeature);
  if (feature_array.data_num < 1000) {
    m_is_cpu = true;
    m_cpu_ipfeature.feature_array = feature_array;
    m_cpu_ipfeature.feature_unit_length = unit_length;
    m_cpu_ipfeature.feature_array_buffer = new float[total_length];
  } else {
    m_is_cpu = false;
    m_tpu_ipfeature.feature_length = feature_array.feature_length;
    m_tpu_ipfeature.data_num = feature_array.data_num;
    m_tpu_ipfeature.feature_unit_length = unit_length;
    m_tpu_ipfeature.array_buffer_32 = new uint32_t[feature_array.data_num];
    m_tpu_ipfeature.array_buffer_f = new float[feature_array.data_num];
    // Clear buffer first
    // Gen cmd buffer here
    // Create buffer for input
    rtinfo &input = m_tpu_ipfeature.feature_input;
    input.rtmem = CVI_RT_MemAlloc(m_rt_handle, feature_array.feature_length);
    input.paddr = CVI_RT_MemGetPAddr(input.rtmem);
    input.vaddr = CVI_RT_MemGetVAddr(input.rtmem);
    // Create buffer for array
    rtinfo &info = m_tpu_ipfeature.feature_array;
    info.rtmem = CVI_RT_MemAlloc(m_rt_handle, total_length);
    info.paddr = CVI_RT_MemGetPAddr(info.rtmem);
    info.vaddr = CVI_RT_MemGetVAddr(info.rtmem);
    // Create buffer for array
    rtinfo &buffer = m_tpu_ipfeature.buffer_array;
    buffer.rtmem = CVI_RT_MemAlloc(m_rt_handle, feature_array.data_num * sizeof(uint32_t));
    buffer.paddr = CVI_RT_MemGetPAddr(buffer.rtmem);
    buffer.vaddr = CVI_RT_MemGetVAddr(buffer.rtmem);
    // Copy feature array to ion
    memcpy(info.vaddr, feature_array.ptr, total_length);
    for (uint32_t n = 0; n < total_length; n++) {
      int i = n / feature_array.data_num;
      int j = n % feature_array.data_num;
      ((int8_t *)info.vaddr)[n] = feature_array.ptr[feature_array.feature_length * j + i];
    }
    CVI_RT_MemFlush(m_rt_handle, info.rtmem);
  }

  return CVI_TDL_SUCCESS;
}

template <typename T>
static std::vector<size_t> sort_indexes(const std::vector<T> &v) {
  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  std::stable_sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) { return v[i1] > v[i2]; });

  return idx;
}

int FeatureMatching::cosSimilarityRun(const void *feature, const feature_type_e &type,
                                      const uint32_t topk, uint32_t *k_index, float *k_value,
                                      float threshold, uint32_t *size) {
  if (topk == 0 && threshold == 0.0f) {
    LOGE("both topk and threshold are invalid value\n");
    *size = 0;
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  if (m_cpu_ipfeature.feature_array.data_num == 0 && m_tpu_ipfeature.data_num == 0) {
    LOGE(
        "Not yet register features, please invoke CVI_TDL_Service_RegisterFeatureArray to "
        "register.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  int ret = CVI_TDL_SUCCESS;

  *size = topk == 0 ? m_data_num : std::min<uint32_t>(m_data_num, topk);

  float *scores = new float[*size];
  uint32_t *indices = new uint32_t[*size];

  switch (type) {
    case TYPE_INT8: {
      if (m_is_cpu) {
        cvm_cpu_i8data_ip_match((int8_t *)feature, (int8_t *)m_cpu_ipfeature.feature_array.ptr,
                                m_cpu_ipfeature.feature_unit_length, indices, scores,
                                m_cpu_ipfeature.feature_array_buffer,
                                m_cpu_ipfeature.feature_array.feature_length,
                                m_cpu_ipfeature.feature_array.data_num, *size);

      } else {
        int8_t *i8_feature = (int8_t *)feature;
        memcpy(m_tpu_ipfeature.feature_input.vaddr, i8_feature, m_tpu_ipfeature.feature_length);
        CVI_RT_MemFlush(m_rt_handle, m_tpu_ipfeature.feature_input.rtmem);
        // Submit command buffer without erasing it.
        size_t *slice_num =
            cvm_gemm(m_cvk_ctx, m_tpu_ipfeature.feature_input.paddr,
                     m_tpu_ipfeature.feature_array.paddr, m_tpu_ipfeature.buffer_array.paddr, 1,
                     m_tpu_ipfeature.feature_length, m_tpu_ipfeature.data_num, CVK_FMT_I8);
        CVI_RT_Submit(m_cvk_ctx);
        CVI_RT_MemInvld(m_rt_handle, m_tpu_ipfeature.buffer_array.rtmem);
        cvm_combin_gemm_i8(slice_num, m_tpu_ipfeature.buffer_array.vaddr,
                           m_tpu_ipfeature.array_buffer_32, 1, m_tpu_ipfeature.data_num);
        free(slice_num);
        // Get a length
        int32_t dot_result = 0;
        for (uint32_t i = 0; i < m_tpu_ipfeature.feature_length; i++) {
          dot_result += ((short)i8_feature[i] * i8_feature[i]);
        }
        float unit_i8 = sqrt(dot_result);
        // Get a length end

        for (uint32_t i = 0; i < m_tpu_ipfeature.data_num; i++) {
          m_tpu_ipfeature.array_buffer_f[i] = ((int32_t *)m_tpu_ipfeature.array_buffer_32)[i] /
                                              (unit_i8 * m_tpu_ipfeature.feature_unit_length[i]);
        }

        // Get k result
        if (*size == m_tpu_ipfeature.data_num) {
          std::vector<float> scores_v =
              std::vector<float>(m_tpu_ipfeature.array_buffer_f,
                                 m_tpu_ipfeature.array_buffer_f + m_tpu_ipfeature.data_num);
          std::vector<size_t> sorted_indices = sort_indexes(scores_v);

          for (uint32_t i = 0; i < *size; i++) {
            indices[i] = sorted_indices[i];
            scores[i] = m_tpu_ipfeature.array_buffer_f[indices[i]];
          }
        } else {
          for (uint32_t i = 0; i < *size; i++) {
            uint32_t largest = 0;
            for (uint32_t j = 0; j < m_tpu_ipfeature.data_num; j++) {
              if (m_tpu_ipfeature.array_buffer_f[j] > m_tpu_ipfeature.array_buffer_f[largest]) {
                largest = j;
              }
            }
            scores[i] = m_tpu_ipfeature.array_buffer_f[largest];
            indices[i] = largest;
            m_tpu_ipfeature.array_buffer_f[largest] = 0;
          }
        }
      }

      uint32_t j = 0;
      for (uint32_t i = 0; i < *size; i++) {
        if (scores[i] >= threshold) {
          k_value[j] = scores[i];
          k_index[j] = indices[i];
          j++;
        }
      }
      *size = j;
    } break;
    default: {
      LOGE("Unsupported register data type %s.\n", TypeToStr(type));
      ret = CVI_TDL_ERR_INVALID_ARGS;
    } break;
  }

  delete[] scores;
  delete[] indices;

  return ret;
}
}  // namespace service
}  // namespace cvitdl