#pragma once
#include <unordered_map>
#include <vector>

#include "core/cvi_tdl_core.h"
#include "core/vpss_engine.hpp"
#include "core_internel.hpp"

#include "deepsort/cvi_deepsort.hpp"
#include "fall_detection/fall_det_monitor.hpp"
#include "fall_detection/fall_detection.hpp"
#include "ive/ive.hpp"
#include "motion_detection/md.hpp"
#include "tamper_detection/tamper_detection.hpp"
typedef struct {
  cvitdl::Core *instance = nullptr;
  std::string model_path = "";
  uint32_t vpss_thread = 0;
} cvitdl_model_t;

// specialize std::hash for enum CVI_TDL_SUPPORTED_MODEL_E
namespace std {
template <>
struct hash<CVI_TDL_SUPPORTED_MODEL_E> {
  size_t operator()(CVI_TDL_SUPPORTED_MODEL_E value) const { return static_cast<size_t>(value); }
};
}  // namespace std

typedef struct {
  std::unordered_map<CVI_TDL_SUPPORTED_MODEL_E, cvitdl_model_t> model_cont;
  std::vector<cvitdl_model_t> custom_cont;
  std::vector<cvitdl::VpssEngine *> vec_vpss_engine;
  uint32_t vpss_timeout_value = 100;  // default value.
  ive::IVE *ive_handle = NULL;
  MotionDetection *md_model = nullptr;
  DeepSORT *ds_tracker = nullptr;
  TamperDetectorMD *td_model = nullptr;
  FallMD *fall_model = nullptr;
  FallDetMonitor *fall_monitor_model = nullptr;
  bool use_gdc_wrap = false;
} cvitdl_context_t;

inline const char *__attribute__((always_inline)) GetModelName(cvitdl_model_t &model) {
  return model.model_path.c_str();
}

inline uint32_t __attribute__((always_inline)) CVI_TDL_GetVpssTimeout(cvitdl_handle_t handle) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  return ctx->vpss_timeout_value;
}

inline cvitdl::VpssEngine *__attribute__((always_inline))
CVI_TDL_GetVpssEngine(cvitdl_handle_t handle, uint32_t index) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (index >= ctx->vec_vpss_engine.size()) {
    return nullptr;
  }
  return ctx->vec_vpss_engine[index];
}

inline int __attribute__((always_inline))
CVI_TDL_AddVpssEngineThread(const uint32_t thread, const VPSS_GRP vpssGroupId, const CVI_U8 dev,
                            uint32_t *vpss_thread, std::vector<cvitdl::VpssEngine *> *vec_engine) {
  *vpss_thread = thread;
  if (thread >= vec_engine->size()) {
    auto inst = new cvitdl::VpssEngine(vpssGroupId, dev);
    vec_engine->push_back(inst);
    if (thread != vec_engine->size() - 1) {
      LOGW(
          "Thread %u is not in use, thus %u is changed to %u automatically. Used vpss group id is "
          "%u.\n",
          *vpss_thread, thread, *vpss_thread, inst->getGrpId());
      *vpss_thread = vec_engine->size() - 1;
    }
  } else {
    LOGW("Thread %u already exists, given group id %u will not be used.\n", thread, vpssGroupId);
  }
  return CVI_TDL_SUCCESS;
}

inline int __attribute__((always_inline))
setVPSSThread(cvitdl_model_t &model, std::vector<cvitdl::VpssEngine *> &v_engine,
              const uint32_t thread, const VPSS_GRP vpssGroupId, const CVI_U8 dev) {
  uint32_t vpss_thread;
  int ret = CVI_TDL_AddVpssEngineThread(thread, vpssGroupId, dev, &vpss_thread, &v_engine);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  model.vpss_thread = vpss_thread;
  if (model.instance != nullptr) {
    model.instance->setVpssEngine(v_engine[model.vpss_thread]);
  }
  return CVI_TDL_SUCCESS;
}
