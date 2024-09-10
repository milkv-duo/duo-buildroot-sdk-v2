#include "core/cvi_tdl_custom.h"
#include "core/cvi_tdl_core.h"
#include "cvi_tdl_core_internal.hpp"
#include "cvi_tdl_log.hpp"

#include "custom/custom.hpp"
#include "cvi_tdl_experimental.h"

#include <string.h>

CVI_S32 CVI_TDL_Custom_AddInference(cvitdl_handle_t handle, uint32_t *id) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t model;
  model.instance = new cvitdl::Custom();
  ctx->custom_cont.push_back(model);
  *id = ctx->custom_cont.size() - 1;
  LOGI("Custom AI instance added.");
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Custom_SetModelPath(cvitdl_handle_t handle, const uint32_t id,
                                    const char *filepath) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return CVI_TDL_FAILURE;
  }
  cvitdl_model_t &mt = ctx->custom_cont[id];
  if (mt.instance->isInitialized()) {
    LOGE("Inference already init. Please call CVI_TDL_Custom_CloseModel to reset.\n");
    return CVI_TDL_FAILURE;
  }
  mt.model_path = filepath;
  return CVI_TDL_SUCCESS;
}

const char *CVI_TDL_Custom_GetModelPath(cvitdl_handle_t handle, const uint32_t id) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return NULL;
  }
  return GetModelName(ctx->custom_cont[id]);
}

CVI_S32 CVI_TDL_Custom_SetVpssThread(cvitdl_handle_t handle, const uint32_t id,
                                     const uint32_t thread) {
  return CVI_TDL_Custom_SetVpssThread2(handle, id, thread, -1);
}

CVI_S32 CVI_TDL_Custom_SetVpssThread2(cvitdl_handle_t handle, const uint32_t id,
                                      const uint32_t thread, const VPSS_GRP vpssGroupId) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return CVI_TDL_FAILURE;
  }
  return setVPSSThread(ctx->custom_cont[id], ctx->vec_vpss_engine, thread, vpssGroupId, 0);
}

CVI_S32 CVI_TDL_Custom_GetVpssThread(cvitdl_handle_t handle, const uint32_t id, uint32_t *thread) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return CVI_TDL_FAILURE;
  }
  *thread = ctx->custom_cont[id].vpss_thread;
  return CVI_TDL_SUCCESS;
}

inline cvitdl::Custom *__attribute__((always_inline))
getCustomInstance(const uint32_t id, cvitdl_context_t *ctx) {
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return nullptr;
  }
  cvitdl_model_t &mt = ctx->custom_cont[id];
  if (mt.instance->isInitialized()) {
    LOGE("Inference already init. Please call CVI_TDL_Custom_CloseModel to reset.\n");
    return nullptr;
  }
  return dynamic_cast<cvitdl::Custom *>(mt.instance);
}

CVI_S32 CVI_TDL_Custom_SetVpssPreprocessParam(cvitdl_handle_t handle, const uint32_t id,
                                              const uint32_t inputIndex, const float *factor,
                                              const float *mean, const uint32_t length,
                                              const bool keepAspectRatio) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstance(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->setSQParam(inputIndex, factor, mean, length, true, keepAspectRatio);
}

CVI_S32 CVI_TDL_Custom_SetVpssPreprocessParamRaw(cvitdl_handle_t handle, const uint32_t id,
                                                 const uint32_t inputIndex, const float *qFactor,
                                                 const float *qMean, const uint32_t length,
                                                 const bool keepAspectRatio) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstance(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->setSQParam(inputIndex, qFactor, qMean, length, false, keepAspectRatio);
}

CVI_S32 CVI_TDL_Custom_SetPreprocessFuncPtr(cvitdl_handle_t handle, const uint32_t id,
                                            preProcessFunc func, const bool use_tensor_input,
                                            const bool use_vpss_sq) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstance(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->setPreProcessFunc(func, use_tensor_input, use_vpss_sq);
}

CVI_S32 CVI_TDL_Custom_CloseModel(cvitdl_handle_t handle, const uint32_t id) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return CVI_TDL_FAILURE;
  }
  cvitdl_model_t &mt = ctx->custom_cont[id];
  mt.instance->modelClose();
  return CVI_TDL_SUCCESS;
}

inline cvitdl::Custom *__attribute__((always_inline))
getCustomInstanceInit(const uint32_t id, cvitdl_context_t *ctx) {
  if (id >= (uint32_t)ctx->custom_cont.size()) {
    LOGE("Exceed id, given %d, total %zu.\n", id, ctx->custom_cont.size());
    return nullptr;
  }
  cvitdl_model_t &mt = ctx->custom_cont[id];
  if (mt.instance->isInitialized() == false) {
    if (mt.model_path.empty()) {
      LOGE("Model path for FaceAttribute is empty.\n");
      return nullptr;
    }
    if (mt.instance->modelOpen(mt.model_path.c_str()) != CVI_TDL_SUCCESS) {
      LOGE("Open model failed (%s).\n", mt.model_path.c_str());
      return nullptr;
    }
    mt.instance->setVpssEngine(ctx->vec_vpss_engine[mt.vpss_thread]);
  }
  return dynamic_cast<cvitdl::Custom *>(mt.instance);
}

CVI_S32 CVI_TDL_Custom_GetInputTensorNCHW(cvitdl_handle_t handle, const uint32_t id,
                                          const char *tensorName, uint32_t *n, uint32_t *c,
                                          uint32_t *h, uint32_t *w) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstanceInit(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->getInputShape(tensorName, n, c, h, w);
}

CVI_S32 CVI_TDL_Custom_RunInference(cvitdl_handle_t handle, const uint32_t id,
                                    VIDEO_FRAME_INFO_S *frame, uint32_t numOfFrames) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstanceInit(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->inference(frame, numOfFrames);
}

CVI_S32 CVI_TDL_Custom_GetOutputTensor(cvitdl_handle_t handle, const uint32_t id,
                                       const char *tensorName, int8_t **tensor,
                                       uint32_t *tensorCount, uint16_t *unitSize) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  auto *inst_ptr = getCustomInstanceInit(id, ctx);
  if (inst_ptr == nullptr) {
    return CVI_TDL_FAILURE;
  }
  return inst_ptr->getOutputTensor(tensorName, tensor, tensorCount, unitSize);
}

// CVI_S32 CVI_TDL_FaceFeatureExtract(const cvitdl_handle_t handle, const uint8_t *p_rgb_pack, int
// width,
//                                   int height, int stride, cvtdl_face_info_t *p_face_info) {
//   cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
//   FaceAttribute *inst = dynamic_cast<FaceAttribute *>(
//       getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE, ctx));
//   if (inst == nullptr) {
//     LOGE("No instance found for FaceAttribute\n");
//     return CVI_FAILURE;
//   }
//   return inst->extract_face_feature(p_rgb_pack, width, height, stride, p_face_info);
// }
