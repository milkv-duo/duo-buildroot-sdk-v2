#ifndef SAMPLE_UTILS_H_
#define SAMPLE_UTILS_H_
#include <pthread.h>
#include "cvi_comm.h"
#include "cvi_tdl.h"
#define RETURN_IF_FAILED(func)    \
  do {                            \
    CVI_S32 tdl_ret = (func);     \
    if (tdl_ret != CVI_SUCCESS) { \
      goto tdl_failed;            \
    }                             \
  } while (0)

#define GOTO_IF_FAILED(func, result, label)                              \
  do {                                                                   \
    result = (func);                                                     \
    if (result != CVI_SUCCESS) {                                         \
      printf("failed! ret=%#x, at %s:%d\n", result, __FILE__, __LINE__); \
      goto label;                                                        \
    }                                                                    \
  } while (0)

/**
 * @typedef ODInferenceFunc
 * @brief Inference function pointer of face deteciton
 */
typedef int (*FaceDetInferFunc)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, CVI_TDL_SUPPORTED_MODEL_E,
                                cvtdl_face_t *);
/**
 * @typedef ODInferenceFunc
 * @brief Inference function pointer of facec recognition
 */
typedef int (*FaceRecInferFunc)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, cvtdl_face_t *);
/**
 * @typedef ODInferenceFunc
 * @brief Inference function pointer of object deteciton
 */
typedef int (*ODInferenceFunc)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, CVI_TDL_SUPPORTED_MODEL_E,
                               cvtdl_object_t *);

/**
 * @typedef LPRInferenceFunc
 * @brief Inference function pointer of license recognition
 */
typedef int (*LPRInferenceFunc)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, cvtdl_object_t *);

/**
 * @brief Get object detection model's ID and inference function according to model_name.
 * @param model_name [in] model name
 * @param model_id [out] model id
 * @param inference_func [out] inference function in TDL SDK
 * @return int Return CVI_TDL_SUCCESS if corresponding model information is found. Otherwise, return
 * CVI_TDL_FAILURE
 */
CVI_S32 get_od_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_id,
                          ODInferenceFunc *inference_func);

/**
 * @brief Get person detection model's ID and inference function according to model_name.
 * @param model_name [in] model name
 * @param model_id [out] model id
 * @param inference_func [out] inference function in TDL SDK
 * @return int Return CVI_TDL_SUCCESS if corresponding model information is found. Otherwise, return
 * CVI_TDL_FAILURE
 */
CVI_S32 get_pd_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_id,
                          ODInferenceFunc *inference_func);

/**
 * @brief Get vehicle detection model's ID and inference function according to model_name.
 * @param model_name [in] model name
 * @param model_id [out] model id
 * @param inference_func [out] inference function in TDL SDK
 * @return int Return CVI_TDL_SUCCESS if corresponding model information is found. Otherwise, return
 * CVI_TDL_FAILURE
 */
CVI_S32 get_vehicle_model_info(const char *model_name, CVI_TDL_SUPPORTED_MODEL_E *model_index,
                               ODInferenceFunc *inference_func);

int release_image(VIDEO_FRAME_INFO_S *frame);
CVI_S32 register_gallery_face(cvitdl_handle_t tdl_handle, const char *sz_img_file,
                              CVI_TDL_SUPPORTED_MODEL_E *fd_index, FaceDetInferFunc fd_func,
                              FaceRecInferFunc fr_func,
                              cvtdl_service_feature_array_t *p_feat_gallery);
CVI_S32 register_gallery_feature(cvitdl_handle_t tdl_handle, const char *sz_feat_file,
                                 cvtdl_service_feature_array_t *p_feat_gallery);

CVI_S32 do_face_match(cvitdl_service_handle_t service_handle, cvtdl_face_info_t *p_face,
                      float thresh);
#define MUTEXAUTOLOCK_INIT(mutex) pthread_mutex_t AUTOLOCK_##mutex = PTHREAD_MUTEX_INITIALIZER;

#define MutexAutoLock(mutex, lock)                                                \
  __attribute__((cleanup(AutoUnLock))) pthread_mutex_t *lock = &AUTOLOCK_##mutex; \
  pthread_mutex_lock(lock);

__attribute__((always_inline)) inline void AutoUnLock(void *mutex) {
  pthread_mutex_unlock(*(pthread_mutex_t **)mutex);
}

#endif
