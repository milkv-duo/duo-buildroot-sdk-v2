#pragma once

#include "cvi_tdl_log.hpp"
#include "service/cvi_tdl_service_types.h"

#include <cvikernel/cvikernel.h>
#include <cvimath/cvimath.h>
#include <cviruntime_context.h>
#include "cvi_comm.h"

namespace cvitdl {
namespace service {

typedef struct {
  CVI_RT_MEM rtmem = NULL;   // Set to NULL if not initialized
  uint64_t paddr = -1;       // Set to maximum of uint64_t if not initaulized
  uint8_t *vaddr = nullptr;  // Set to nullptr if not initualized
} rtinfo;

typedef struct {
  cvtdl_service_feature_array_t feature_array;
  float *feature_unit_length = nullptr;
  float *feature_array_buffer = nullptr;
} cvtdl_service_feature_array_ext_t;

typedef struct {
  uint32_t feature_length;
  uint32_t data_num;
  rtinfo feature_input;
  rtinfo feature_array;
  rtinfo buffer_array;
  size_t *slice_num = nullptr;
  float *feature_unit_length = nullptr;
  uint32_t *array_buffer_32 = nullptr;
  float *array_buffer_f = nullptr;
} cvtdl_service_feature_array_tpu_ext_t;

class FeatureMatching {
 public:
  ~FeatureMatching();
  int init();
  static int createHandle(CVI_RT_HANDLE *rt_handle, cvk_context_t **cvk_ctx);
  static int destroyHandle(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx);
  int registerData(const cvtdl_service_feature_array_t &feature_array,
                   const cvtdl_service_feature_matching_e &matching_method);

  int run(const void *feature, const feature_type_e &type, const uint32_t k, uint32_t *indices,
          float *scores, uint32_t *size, float threshold);

 private:
  int cosSimilarityRegister(const cvtdl_service_feature_array_t &feature_array);
  int cosSimilarityRun(const void *feature, const feature_type_e &type, const uint32_t k,
                       uint32_t *index, float *scores, float threshold, uint32_t *size);
  CVI_RT_HANDLE m_rt_handle;
  cvk_context_t *m_cvk_ctx = NULL;

  cvtdl_service_feature_matching_e m_matching_method;
  bool m_is_cpu = true;

  cvtdl_service_feature_array_tpu_ext_t m_tpu_ipfeature;
  cvtdl_service_feature_array_ext_t m_cpu_ipfeature;
  uint32_t m_data_num;
};
}  // namespace service
}  // namespace cvitdl
