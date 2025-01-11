#ifndef _CVI_TDL_CORE_H_
#define _CVI_TDL_CORE_H_
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "core/core/cvtdl_vpss_types.h"
#include "core/cvi_tdl_rescale_bbox.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/deepsort/cvtdl_deepsort_types.h"
#include "core/face/cvtdl_face_helper.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"

/** @def CVI_TDL_Free
 *  @ingroup core_cvitdlcore
 * @brief Free the content inside the structure, not the structure itself.
 *        Support the following structure types written in _Generic.
 *
 * @param X Input data structure.
 */
#ifdef __cplusplus
#define CVI_TDL_Free(X) CVI_TDL_FreeCpp(X)
#else
// clang-format off
#define CVI_TDL_Free(X) _Generic((X),                   \
           cvtdl_feature_t*: CVI_TDL_FreeFeature,        \
           cvtdl_pts_t*: CVI_TDL_FreePts,                \
           cvtdl_tracker_t*: CVI_TDL_FreeTracker,        \
           cvtdl_face_info_t*: CVI_TDL_FreeFaceInfo,     \
           cvtdl_face_t*: CVI_TDL_FreeFace,              \
           cvtdl_object_info_t*: CVI_TDL_FreeObjectInfo, \
           cvtdl_object_t*: CVI_TDL_FreeObject,          \
           cvtdl_handpose21_meta_ts*: CVI_TDL_FreeHandPoses, \
           cvtdl_class_meta_t*: CVI_TDL_FreeClassMeta, \
           cvtdl_image_t*: CVI_TDL_FreeImage,          \
           cvtdl_clip_feature*: CVI_TDL_FreeClip,      \
           cvtdl_lane_t* : CVI_TDL_FreeLane)(X)

// clang-format on
#endif

/** @def CVI_TDL_CopyInfo
 *  @ingroup core_cvitdlcore
 * @brief Fully copy the info structure. (including allocating new memory for you.)
 *
 * @param IN Input info structure.
 * @param OUT Output info structure (uninitialized structure required).
 */
#ifdef __cplusplus
#define CVI_TDL_CopyInfo(IN, OUT) CVI_TDL_CopyInfoCpp(IN, OUT)
#else
// clang-format off
#define CVI_TDL_CopyInfoG(OUT) _Generic((OUT),                       \
           cvtdl_face_info_t*: CVI_TDL_CopyFaceInfo,                  \
           cvtdl_object_info_t*: CVI_TDL_CopyObjectInfo,              \
           cvtdl_image_t*: CVI_TDL_CopyImage)
#define CVI_TDL_CopyInfo(IN, OUT) _Generic((IN),                     \
           cvtdl_face_info_t*: CVI_TDL_CopyInfoG(OUT),                \
           cvtdl_object_info_t*: CVI_TDL_CopyInfoG(OUT),              \
           cvtdl_image_t*: CVI_TDL_CopyInfoG(OUT))((IN), (OUT))
// clang-format on
#endif

/** @def CVI_TDL_RescaleMetaCenter
 * @ingroup core_cvitdlcore
 * @brief Rescale the output coordinate to original image. Padding in four directions. Support the
 * following structure types written in _Generic.
 *
 * @param videoFrame Original input image.
 * @param X Input data structure.
 */

/** @def CVI_TDL_RescaleMetaRB
 * @ingroup core_cvitdlcore
 * @brief Rescale the output coordinate to original image. Padding in right, bottom directions.
 * Support the following structure types written in _Generic.
 *
 * @param videoFrame Original input image.
 * @param X Input data structure.
 */
#ifdef __cplusplus
#define CVI_TDL_RescaleMetaCenter(videoFrame, X) CVI_TDL_RescaleMetaCenterCpp(videoFrame, X);
#define CVI_TDL_RescaleMetaRB(videoFrame, X) CVI_TDL_RescaleMetaRBCpp(videoFrame, X);
#else
// clang-format off
#define CVI_TDL_RescaleMetaCenter(videoFrame, X) _Generic((X), \
           cvtdl_face_t*: CVI_TDL_RescaleMetaCenterFace,        \
           cvtdl_object_t*: CVI_TDL_RescaleMetaCenterObj)(videoFrame, X);
#define CVI_TDL_RescaleMetaRB(videoFrame, X) _Generic((X),     \
           cvtdl_face_t*: CVI_TDL_RescaleMetaRBFace,            \
           cvtdl_object_t*: CVI_TDL_RescaleMetaRBObj)(videoFrame, X);
// clang-format on
#endif

/** @typedef cvitdl_handle_t
 * @ingroup core_cvitdlcore
 * @brief An cvitdl handle
 */
typedef void *cvitdl_handle_t;

/**
 * \addtogroup core Inference Functions
 * \ingroup core_cvitdlcore
 */

/**
 * \addtogroup core_tdl_settings TDL Inference Setting Functions
 * \ingroup core_tdl
 */
/**@{*/

/**
 * IMPORTENT!! Add supported model here!
 */
// clang-format off
#define CVI_TDL_MODEL_LIST \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_RETINAFACE)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_SCRFDFACE)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_RETINAFACE_HARDHAT)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_THERMALFACE)                      \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_THERMALPERSON)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION)                  \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MASKFACERECOGNITION)              \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACEQUALITY)                      \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MASKCLASSIFICATION)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT_CLASSIFICATION)     \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LIVENESS)                         \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE)       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE)              \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN)           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS)          \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV3)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV5)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV6)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV7)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLO)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOX)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_PPYOLOE)                           \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_OSNET)                            \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION)            \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_WPODNET)                          \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LPRNET_TW)                        \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LPRNET_CN)                        \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_DEEPLABV3)                        \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_MOTIONSEGMENTATION)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_EYECLASSIFICATION)                \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YAWNCLASSIFICATION)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACELANDMARKER)                   \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACELANDMARKERDET2)               \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_INCAROBJECTDETECTION)             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_SMOKECLASSIFICATION)              \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_FACEMASKDETECTION)                \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_IRLIVENESS)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION)            \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION)                 \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION)                 \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION)         \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION)       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HEAD_PERSON_DETECTION)            \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LANDMARK_DET3)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LP_RECONGNITION)                  \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LP_KEYPOINT)                      \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LP_DETECTION)                     \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION)             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_RAW_IMAGE_CLASSIFICATION)             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_ISP_IMAGE_CLASSIFICATION)             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_HRNET_POSE)                       \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_DMSLANDMARKERDET)                 \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE)                             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT)                             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG)                             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_YOLOV8_HARDHAT)                   \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LANE_DET)                         \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_POLYLANE)                         \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_SUPER_RESOLUTION)                 \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_OCR_DETECTION)                    \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_OCR_RECOGNITION)                  \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_LSTR)                             \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_STEREO) \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_OCCLUSION_CLASSIFICATION)         \
  CVI_TDL_NAME_WRAP(CVI_TDL_SUPPORTED_MODEL_TOPFORMER_SEG)                    \
  // clang-format on

#define CVI_TDL_NAME_WRAP(x) x,

/** @enum CVI_TDL_SUPPORTED_MODEL_E
 * @brief Supported NN model list. Can be used to config function behavior.
 *
 */
typedef enum { CVI_TDL_MODEL_LIST CVI_TDL_SUPPORTED_MODEL_END } CVI_TDL_SUPPORTED_MODEL_E;
#undef CVI_TDL_NAME_WRAP

#define CVI_TDL_NAME_WRAP(x) #x,

static inline const char *CVI_TDL_GetModelName(CVI_TDL_SUPPORTED_MODEL_E index) {
  static const char *model_names[] = {CVI_TDL_MODEL_LIST};
  uint32_t length = sizeof(model_names) / sizeof(model_names[0]);
  if (index < length) {
    return model_names[index];
  } else {
    return "Unknown";
  }
}

#undef CVI_TDL_NAME_WRAP
#undef CVI_TDL_MODEL_LIST

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Create a cvitdl_handle_t, will automatically find a vpss group id.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CreateHandle(cvitdl_handle_t *handle);

/**
 * @brief Create a cvitdl_handle_t, need to manually assign a vpss group id.
 *
 * @param handle An TDL SDK handle.
 * @param vpssGroupId Assign a group id to cvitdl_handle_t.
 * @param vpssDev Assign a device id to cvitdl_handle_t.
 * @return int Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CreateHandle2(cvitdl_handle_t *handle, const VPSS_GRP vpssGroupId,
                                         const CVI_U8 vpssDev);

DLL_EXPORT CVI_S32 CVI_TDL_CreateHandle3(cvitdl_handle_t *handle);

/**
 * @brief Destroy a cvitdl_handle_t.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS if success to destroy handle.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DestroyHandle(cvitdl_handle_t handle);

/**
 * @brief Open model with given file path.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param filepath File path to the cvimodel file.
 * @return int Return CVI_TDL_SUCCESS if load model succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_OpenModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                     const char *filepath);
/**
 * @brief Get model input data tpye.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param filepath File path to the cvimodel file.
 * @return int Return CVI_TDL_SUCCESS if load model succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetModelInputTpye(cvitdl_handle_t handle,
                                             CVI_TDL_SUPPORTED_MODEL_E model, int *inputDTpye);
/**
 * @brief Open model with given file path.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param bug memory buffer to the cvimodel.
 * @param size cvimodel buffer size
 * @return int Return CVI_TDL_SUCCESS if load model succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_OpenModel_FromBuffer(cvitdl_handle_t handle,
                                                CVI_TDL_SUPPORTED_MODEL_E model, int8_t *buf,
                                                uint32_t size);

/**
 * @brief Get set model path from supported models.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @return model path.
 */
DLL_EXPORT const char *CVI_TDL_GetModelPath(cvitdl_handle_t handle,
                                            CVI_TDL_SUPPORTED_MODEL_E model);

/**
 * @brief Set skip vpss preprocess for supported networks.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param skip To skip preprocess or not.
 * @return int Return CVI_TDL_SUCCESS if load model succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetSkipVpssPreprocess(cvitdl_handle_t handle,
                                                 CVI_TDL_SUPPORTED_MODEL_E model, bool skip);

/**
 * @brief Set skip vpss preprocess for supported networks.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param interval number of frames used to performance evaluation
 * @return int Return CVI_TDL_SUCCESS if load model succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetPerfEvalInterval(cvitdl_handle_t handle,
                                               CVI_TDL_SUPPORTED_MODEL_E config, int interval);

/**
 * @brief Set list depth for VPSS.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param input_id input index of model.
 * @param depth list depth of VPSS.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetVpssDepth(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                        uint32_t input_id, uint32_t depth);

/**
 * @brief Get list depth for VPSS.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param input_id input index of model.
 * @param depth list depth of VPSS.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetVpssDepth(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                        uint32_t input_id, uint32_t *depth);

/**
 * @brief Get skip preprocess value for given supported model.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param skip Output setting value.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetSkipVpssPreprocess(cvitdl_handle_t handle,
                                                 CVI_TDL_SUPPORTED_MODEL_E model, bool *skip);

/**
 * @brief Set the threshold of an TDL inference.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param threshold Threshold in float, usually a number between 0 and 1.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetModelThreshold(cvitdl_handle_t handle,
                                             CVI_TDL_SUPPORTED_MODEL_E model, float threshold);

/**
 * @brief Set the nms threshold of yolo inference.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param threshold Threshold in float, usually a number between 0 and 1.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetModelNmsThreshold(cvitdl_handle_t handle,
                                                CVI_TDL_SUPPORTED_MODEL_E model, float threshold);

/**
 * @brief Get the threshold of an TDL Inference
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param threshold Threshold in float.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetModelThreshold(cvitdl_handle_t handle,
                                             CVI_TDL_SUPPORTED_MODEL_E model, float *threshold);

/**
 * @brief Get the nms threshold of yolo Inference
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param threshold Threshold in float.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetModelNmsThreshold(cvitdl_handle_t handle,
                                                CVI_TDL_SUPPORTED_MODEL_E model, float *threshold);

/**
 * @brief Use the mmap in model
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param mmap mmap in bool.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_UseMmap(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                   bool mmap);
/**
 * @brief Set different vpss thread for each model. Vpss group id is not thread safe. We recommended
 * to change a thread if the process is not sequential.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param thread The vpss thread index user desired. Note this param will changed if previous index
 * is not used.
 * @return int Return CVI_TDL_SUCCESS if successfully changed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                         const uint32_t thread);

/**
 * @brief Set different vpss thread for each model. Vpss group id is not thread safe. We recommended
 * to change a thread if the process is not sequential. This function requires manually assigning a
 * vpss group id and device id.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param thread The vpss thread index user desired. Note this param will changed if previous index
 * is not used.
 * @param vpssGroupId Assign a vpss group id if a new vpss instance needs to be created.
 * @param dev Assign Vpss device id to Vpss group
 * @return int Return CVI_TDL_SUCCESS if successfully changed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetVpssThread2(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                          const uint32_t thread, const VPSS_GRP vpssGroupId,
                                          const CVI_U8 dev);

/**
 * @brief Get the set thread index for given supported model.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param thread Output thread index.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                         uint32_t *thread);

/**
 * @brief Set VB pool id to VPSS in TDLSDK. By default, VPSS in TDLSDK acquires VB from all
 * system-wide VB_POOLs which are created via CVI_VB_Init. In this situation, system decides which
 * VB_POOL is used according to VB allocation mechanism. The size of aquired VB maybe not optimal
 * and it could cause resource competition. To prevents this problem, you can assign a specific
 * VB_POOL to TDLSDK via this function. The VB_POOL created by CVI_VB_Init or CVI_VB_CreatePool are
 * both accepted.
 *
 * @param handle An TDL SDK handle.
 * @param thread VPSS thread index.
 * @param pool_id vb pool id. if pool id is VB_INVALID_POOLID than VPSS will get VB from all
 * system-wide VB_POOLs like default.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetVBPool(cvitdl_handle_t handle, uint32_t thread, VB_POOL pool_id);

/**
 * @brief Get VB pool id used by internal VPSS.
 *
 * @param handle An TDL SDK handle.
 * @param thread VPSS thread index.
 * @param pool_id Output vb pool id.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetVBPool(cvitdl_handle_t handle, uint32_t thread, VB_POOL *pool_id);

/**
 * @brief Get the vpss group ids used by the handle.
 *
 * @param handle An TDL SDK handle.
 * @param groups Return the list of used vpss group id.
 * @param num Return the length of the list.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetVpssGrpIds(cvitdl_handle_t handle, VPSS_GRP **groups, uint32_t *num);

/**
 * @brief Set VPSS waiting time.
 *
 * @param handle An TDL SDK handle.
 * @param timeout Timeout value.
 * @return int Return CVI_TDL_SUCCESS.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetVpssTimeout(cvitdl_handle_t handle, uint32_t timeout);

/**
 * @brief Close all opened models and delete the model instances.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CloseAllModel(cvitdl_handle_t handle);

/**
 * @brief Close the chosen model and delete its model instance.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @return int Return CVI_TDL_SUCCESS if close succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CloseModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model);

/**
 * @brief Export vpss channel attribute.
 *
 * @param handle An TDL SDK handle.
 * @param model Supported model id.
 * @param frameWidth The input frame width.
 * @param frameHeight The input frame height.
 * @param idx The index of the input tensors.
 * @param chnAttr Exported VPSS channel config settings.
 * @return int Return CVI_TDL_SUCCESS on success, CVI_TDL_FAILURE if exporting not supported.
 */
DLL_EXPORT CVI_S32 CVI_TDL_GetVpssChnConfig(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                                            const CVI_U32 frameWidth, const CVI_U32 frameHeight,
                                            const CVI_U32 idx, cvtdl_vpssconfig_t *chnConfig);

/**@}*/

/**
 * \addtogroup core_fd Face Detection TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

DLL_EXPORT CVI_S32 CVI_TDL_FLDet3(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                  cvtdl_face_t *faces);

/**
 * @brief Detect face with score.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param face_meta Output detect result. The bbox will be given.
 * @param model_index The face detection model id selected to use.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         CVI_TDL_SUPPORTED_MODEL_E model_index,
                                         cvtdl_face_t *face_meta);

/**@}*/

/**
 * \addtogroup core_fr Face Recognition TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Do face recognition and attribute with bbox info stored in faces.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceAttribute(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         cvtdl_face_t *faces);

/*
 * @brief Do face recognition and attribute with bbox info stored in faces.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceAttribute_cls(const cvitdl_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *faces);

/*
 * @brief Do face recognition and attribute with bbox info stored in faces.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceAttribute_cls(const cvitdl_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *faces);

/**
 * @brief Do face recognition and attribute with bbox info stored in faces. Only do inference on the
 * given index of cvtdl_face_info_t.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @param face_idx The index of cvtdl_face_info_t inside cvtdl_face_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceAttributeOne(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                            cvtdl_face_t *faces, int face_idx);

/**
 * @brief Do face recognition with bbox info stored in faces.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceRecognition(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                           cvtdl_face_t *faces);

/**
 * @brief Do face recognition with bbox info stored in faces. Only do inference on the given index
 * of cvtdl_face_info_t.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @param face_idx The index of cvtdl_face_info_t inside cvtdl_face_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

/**
 * @brief Do face recognition with bbox info stored in faces.
 *
 * @param handle An TDL SDK handle.
 * @param p_rgb_pack Input video frame.
 * @param p_face_info, if no data in p_face_info,p_rgb_pack means aligned packed rgb data
 * @return int Return CVI_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceFeatureExtract(const cvitdl_handle_t handle,
                                              const uint8_t *p_rgb_pack, int width, int height,
                                              int stride, cvtdl_face_info_t *p_face_info);

DLL_EXPORT CVI_S32 CVI_TDL_FaceRecognitionOne(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *faces,
                                              int face_idx);

/**@}*/

/**
 * \addtogroup core_fc Face classification TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Crop image in given frame.
 *
 * @param srcFrame Input frame. (only support RGB Packed format)
 * @param dst Output image.
 * @param bbox The bounding box.
 * @param cvtRGB888 convert to RGB888 format.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CropImage(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                                     cvtdl_bbox_t *bbox, bool cvtRGB888);

/**
 * @brief Crop image (extended) in given frame.
 *
 * @param srcFrame Input frame. (only support RGB Packed format)
 * @param dst Output image.
 * @param bbox The bounding box.
 * @param cvtRGB888 convert to RGB888 format.
 * @param exten_ratio extension ration.
 * @param offset_x original bounding box x offset.
 * @param offset_y original bounding box y offset.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CropImage_Exten(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                                           cvtdl_bbox_t *bbox, bool cvtRGB888, float exten_ratio,
                                           float *offset_x, float *offset_y);

/**
 * @brief Crop face image in given frame.
 *
 * @param srcFrame Input frame. (only support RGB Packed format)
 * @param dst Output image.
 * @param face_info Face information, contain bbox and 5 landmark.
 * @param align Align face to standard size if true.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CropImage_Face(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                                          cvtdl_face_info_t *face_info, bool align, bool cvtRGB888);

/**
 * @brief Mask classification. Tells if a face is wearing a mask.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param face cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_MaskClassification(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);

/**
 * @brief Hand classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param meta cvtdl_object_t structure.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_HandClassification(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *meta);

/**
 * @brief 2D hand keypoints.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param meta cvtdl_handpose21_meta_ts structure.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_HandKeypoint(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                        cvtdl_handpose21_meta_ts *meta);

/**
 * @brief Hand classification by hand keypoints.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param meta cvtdl_handpose21_meta_t structure.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_HandKeypointClassification(const cvitdl_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame,
                                                      cvtdl_handpose21_meta_t *meta);

/**@}*/

/**
 * \addtogroup core_od Object Detection TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Select classes for detection model. Model will output objects belong to these classes.
 * Currently only support MobileDetV2 family.
 *
 * @param handle An TDL SDK handle.
 * @param model model id.
 * @param num_classes number of classes you want to select.
 * @param ... class indexs
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SelectDetectClass(cvitdl_handle_t handle,
                                             CVI_TDL_SUPPORTED_MODEL_E model, uint32_t num_classes,
                                             ...);

/**
 * @brief car,bus,truck,person,bike,motor Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_PersonVehicle_Detection(const cvitdl_handle_t handle,
                                                   VIDEO_FRAME_INFO_S *frame,
                                                   cvtdl_object_t *obj_meta);

/**
 * @brief Common object detection task interface
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param obj Output detect result. The name, bbox, and classes will be given.
 * @param model_index The object detection model id selected to use.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Detection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                     CVI_TDL_SUPPORTED_MODEL_E model_index, cvtdl_object_t *obj);

/**
 * @brief Set object model output layer names.
 *
 * @param handle An TDL SDK handle.
 * @param model_index The object detection model id selected to use.
 * @param output_names output layer names
 * @param size output layer name's size
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Set_Outputlayer_Names(const cvitdl_handle_t handle,
                                                 CVI_TDL_SUPPORTED_MODEL_E model_index,
                                                 const char **output_names, size_t size);
/**@}*/
/**
 * \addtogroup core_pr Person Re-Id TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Do person Re-Id with bbox info stored in obj.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param obj cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_OSNet(cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                 cvtdl_object_t *obj);

/**
 * @brief Do person Re-Id with bbox info stored in obj. Only do inference on the given index of
 * cvtdl_object_info_t.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param obj cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @param obj_idx The index of cvtdl_object_info_t inside cvtdl_object_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_OSNetOne(cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                    cvtdl_object_t *obj, int obj_idx);

/**@}*/

/**
 * \addtogroup core_audio Audio TDL Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Do sound classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param index The index of sound classes.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SoundClassification(const cvitdl_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame, int *index);
/**
 * @brief Do sound classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param pack_idx The start pack index of this frame
 * @param pack_len Pack length,the frame is combined with many packs
 * @param index The index of sound classes.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SoundClassificationPack(const cvitdl_handle_t handle,
                                                   VIDEO_FRAME_INFO_S *frame, int pack_idx,
                                                   int pack_len, int *index);
/**
 * @brief Get sound classification classes num.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

DLL_EXPORT CVI_S32 CVI_TDL_GetSoundClassificationClassesNum(const cvitdl_handle_t handle);

/**
 * @brief Set sound classification threshold.
 *
 * @param handle An TDL SDK handle.
 * @param th Sound classifiction threshold
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetSoundClassificationThreshold(const cvitdl_handle_t handle,
                                                           const float th);

/**@}*/

/**
 * \addtogroup core_tracker Tracker
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Initialize DeepSORT.
 *
 * @param handle An TDL SDK handle.
 * @param use_specific_counter true for using individual id counter for each class
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Init(const cvitdl_handle_t handle, bool use_specific_counter);

/**
 * @brief Get default DeepSORT config.
 *
 * @param ds_conf Output config.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_GetDefaultConfig(cvtdl_deepsort_config_t *ds_conf);

/**
 * @brief Get DeepSORT config.
 *
 * @param handle An TDL SDK handle.
 * @param ds_conf Output config.
 * @param cvitdl_obj_type The specific class type (-1 for setting default config).
 * @return int Return CVI_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_GetConfig(const cvitdl_handle_t handle,
                                              cvtdl_deepsort_config_t *ds_conf,
                                              int cvitdl_obj_type);

/**
 * @brief Set DeepSORT with specific config.
 *
 * @param handle An TDL SDK handle.
 * @param ds_conf The specific config.
 * @param cvitdl_obj_type The specific class type (-1 for setting default config).
 * @param show_config show detail information or not.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_SetConfig(const cvitdl_handle_t handle,
                                              cvtdl_deepsort_config_t *ds_conf, int cvitdl_obj_type,
                                              bool show_config);

/**
 * @brief clean DeepSORT ID counter.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_CleanCounter(const cvitdl_handle_t handle);

/**
 * @brief Run DeepSORT/SORT track for object.
 *
 * @param handle An TDL SDK handle.
 * @param obj Input detected object with feature.
 * @param tracker_t Output tracker results.
 * @param use_reid If true, track by DeepSORT algorithm, else SORT.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Obj(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                        cvtdl_tracker_t *tracker, bool use_reid);

/**
 * @brief Run DeepSORT/SORT track for object, add function to judge cross the border.
 *
 * @param handle An TDL SDK handle.
 * @param obj Input detected object with feature.
 * @param tracker_t Output tracker results.
 * @param use_reid If true, track by DeepSORT algorithm, else SORT.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Obj_Cross(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                              cvtdl_tracker_t *tracker, bool use_reid,
                                              const cvtdl_counting_line_t *cross_line_t,
                                              const randomRect *rect);

/**
 * @brief Run DeepSORT/SORT track for object to Consumer counting.
 *
 * @param handle An TDL SDK handle.
 * @param obj Input detected object with feature.
 * @param tracker_t Output tracker results.
 * @param use_reid If true, track by DeepSORT algorithm, else SORT.
 * @param head save head data
 * @param ped save ped data
 * @param counting_line_t the line of consumer counting
 * @param counting_line_t the buffer rectangle
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Head_FusePed(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                                 cvtdl_tracker_t *tracker_t, bool use_reid,
                                                 cvtdl_object_t *head, cvtdl_object_t *ped,
                                                 const cvtdl_counting_line_t *counting_line_t,
                                                 const randomRect *rect);

/**
 * @brief Run SORT track for face.
 *
 * @param handle An TDL SDK handle.
 * @param face Input detected face with feature.
 * @param tracker_t Output tracker results.
 * @param use_reid Set false for SORT.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Face(const cvitdl_handle_t handle, cvtdl_face_t *face,
                                         cvtdl_tracker_t *tracker);

DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_FaceFusePed(const cvitdl_handle_t handle, cvtdl_face_t *face,
                                                cvtdl_object_t *obj, cvtdl_tracker_t *tracker_t);

DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Set_Timestamp(const cvitdl_handle_t handle, uint32_t ts);

DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_UpdateOutNum(const cvitdl_handle_t handle,
                                                 cvtdl_tracker_t *tracker_t);
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_DebugInfo_1(const cvitdl_handle_t handle, char *debug_info);

DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_GetTracker_Inactive(const cvitdl_handle_t handle,
                                                        cvtdl_tracker_t *tracker);

/**
 * @brief Calculate iou score between faces and heads.
 *
 * @param handle An TDL SDK handle.
 * @param faces Input detected faces.
 * @param heads Input detected heads.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceHeadIouScore(const cvitdl_handle_t handle, cvtdl_face_t *faces,
                                            cvtdl_face_t *heads);

/**@}*/

/**
 * \addtogroup core_segmentation Segmentation Inference
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Deeplabv3 segmentation.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param out_frame Output frame which each pixel represents class label.
 * @param filter Class id filter. Set NULL to ignore.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeeplabV3(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                     VIDEO_FRAME_INFO_S *out_frame, cvtdl_class_filter_t *filter);
/**@}*/

/**
 * @brief Motion segmentation.
 *
 * @param handle An TDL SDK handle.
 * @param input0 Input image 0.
 * @param input1 Input image 1.
 * @param output Output segmentation mask.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_MotionSegmentation(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *input0,
                                              VIDEO_FRAME_INFO_S *input1,
                                              cvtdl_seg_logits_t *seg_logits);
/**@}*/

/**
 * @brief LicensePlateDetection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param vehicle License plate object info
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_License_Plate_Detectionv2(const cvitdl_handle_t handle,
                                                     VIDEO_FRAME_INFO_S *frame,
                                                     cvtdl_object_t *vehicle);

/*useless: old license plate detection and recongition*/
DLL_EXPORT CVI_S32 CVI_TDL_LicensePlateDetection(const cvitdl_handle_t handle,
                                                 VIDEO_FRAME_INFO_S *frame,
                                                 cvtdl_object_t *vehicle_meta);
DLL_EXPORT CVI_S32 CVI_TDL_IrLiveness(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *irFrame,
                                      cvtdl_face_t *ir_face);

#ifndef NO_OPENCV
/**
 * @brief Do face recognition with mask wearing.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param faces cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_MaskFaceRecognition(const cvitdl_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *faces);
/**
 * @brief FaceQuality. Assess the quality of the faces.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param face cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @param skip bool array, whether skip quailty assessment at corresponding index (NULL for running
 * without skip)
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceQuality(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                       cvtdl_face_t *face, bool *skip);
/**
 * @brief Do smoke classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param cvtdl_face_t structure. Calculate the smoke_score in cvtdl_dms_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

DLL_EXPORT CVI_S32 CVI_TDL_SmokeClassification(const cvitdl_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
/**
 * @brief Do eye classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param cvtdl_face_t structure. Calculate the eye_score in cvtdl_dms_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

DLL_EXPORT CVI_S32 CVI_TDL_EyeClassification(const cvitdl_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);

/**
 * @brief Do yawn classification.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param cvtdl_face_t structure. Calculate the yawn_score in cvtdl_dms_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

DLL_EXPORT CVI_S32 CVI_TDL_YawnClassification(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
/**
 * @brief Liveness. Gives a score to present how real the face is. The score will be low if the face
 * is not directly taken by a camera.
 *
 * @param handle An TDL SDK handle.
 * @param rgbFrame Input RGB video frame.
 * @param irFrame Input IR video frame.
 * @param face cvtdl_face_t structure, the cvtdl_face_info_t and cvtdl_bbox_t must be set.
 * @param ir_position The position relationship netween the ir and the rgb camera.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Liveness(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *rgbFrame,
                                    VIDEO_FRAME_INFO_S *irFrame, cvtdl_face_t *rgb_face,
                                    cvtdl_face_t *ir_face);
DLL_EXPORT CVI_S32 CVI_TDL_License_Plate_Keypoint(const cvitdl_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta);

DLL_EXPORT CVI_S32 CVI_TDL_LicensePlateRecognition_CN(const cvitdl_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame,
                                                      cvtdl_object_t *vehicle);
DLL_EXPORT CVI_S32 CVI_TDL_LicensePlateRecognition_TW(const cvitdl_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame,
                                                      cvtdl_object_t *vehicle);
DLL_EXPORT CVI_S32 CVI_TDL_LicensePlateRecognition(const cvitdl_handle_t handle,
                                                   VIDEO_FRAME_INFO_S *frame,
                                                   CVI_TDL_SUPPORTED_MODEL_E model_id,
                                                   cvtdl_object_t *vehicle);
DLL_EXPORT CVI_S32 CVI_TDL_LicensePlateRecognition_V2(const cvitdl_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta);

DLL_EXPORT CVI_S32 CVI_TDL_OCR_Detection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         cvtdl_object_t *obj_meta);
/**
 * \addtogroup core_fall Fall Detection
 * \ingroup core_tdl
 */
/**@{*/
/**
 * @brief Fall.
 *
 * @param handle An TDL SDK handle.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Fall(const cvitdl_handle_t handle, cvtdl_object_t *objects);
/**@{*/

/**
 * \addtogroup core_fall Fall Detection new API
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Fall Detection.
 *
 * @param handle An TDL SDK handle.
 * @param objects Detected object info.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Fall_Monitor(const cvitdl_handle_t handle, cvtdl_object_t *objects);

/**
 * @brief Set fall detection FPS.
 *
 * @param handle An TDL SDK handle.
 * @param fps Current frame fps.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Set_Fall_FPS(const cvitdl_handle_t handle, float fps);
#endif

/**
 * \addtogroup core_others Others
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Do background subtraction.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param moving_score Check the unit diff sum of a frame.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_TamperDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                           float *moving_score);

/**
 * @brief Set background frame for motion detection.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * be returned.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Set_MotionDetection_Background(const cvitdl_handle_t handle,
                                                          VIDEO_FRAME_INFO_S *frame);

/**
 * @brief Set ROI frame for motion detection.
 *
 * @param handle An TDL SDK handle.
 * @param x1 left x coordinate of roi
 * @param y1 top y coordinate of roi
 * @param x2 right x coordinate of roi
 * @param y2 bottom y coordinate of roi
 *
 * be returned.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Set_MotionDetection_ROI(const cvitdl_handle_t handle, MDROI_t *roi_s);
/**
 * @brief Do Motion Detection with background subtraction method.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * @param objects Detected object info
 * @param threshold Threshold of motion detection, the range between 0 and 255.
 * @param min_area Minimal pixel area. The bounding box whose area is larger than this value would
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_MotionDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                           cvtdl_object_t *objects, uint8_t threshold,
                                           double min_area);

/**@}*/

/**
 * \addtogroup core_dms Driving Monitor System
 * \ingroup core_tdl
 */
/**@{*/

/**
 * @brief Do face landmark.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param cvtdl_face_t structure. Calculate the landmarks in cvtdl_dms_t.
 * @return int Return CVI_TDL_SUCCESS on success.
 */

DLL_EXPORT CVI_S32 CVI_TDL_IncarObjectDetection(const cvitdl_handle_t handle,
                                                VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);

/**@}*/

/**
 * \addtogroup core_face_landmark Face Landmark
 * \ingroup core_tdl
 */
/**@{*/

DLL_EXPORT CVI_S32 CVI_TDL_FaceLandmarker(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                          cvtdl_face_t *face);

/**@}*/

/**
 * \addtogroup core_face_landmarkdet3 Face Landmark
 * \ingroup core_tdl
 */
/**@{*/

DLL_EXPORT CVI_S32 CVI_TDL_FaceLandmarkerDet2(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);

/**@}*/

/**
 * \addtogroup core_ Dms face Landmark
 * \ingroup core_tdl
 */
/**@{*/
DLL_EXPORT CVI_S32 CVI_TDL_DMSLDet(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                   cvtdl_face_t *face);

DLL_EXPORT CVI_S32 CVI_TDL_CropImage_With_VPSS(const cvitdl_handle_t handle,
                                               CVI_TDL_SUPPORTED_MODEL_E model,
                                               VIDEO_FRAME_INFO_S *frame,
                                               const cvtdl_bbox_t *p_crop_box,
                                               cvtdl_image_t *p_dst);

DLL_EXPORT CVI_S32 CVI_TDL_CropResizeImage(const cvitdl_handle_t handle,
                                           CVI_TDL_SUPPORTED_MODEL_E model_type,
                                           VIDEO_FRAME_INFO_S *frame,
                                           const cvtdl_bbox_t *p_crop_box, int dst_width,
                                           int dst_height, PIXEL_FORMAT_E enDstFormat,
                                           VIDEO_FRAME_INFO_S **p_dst_img);

DLL_EXPORT CVI_S32 CVI_TDL_Copy_VideoFrameToImage(VIDEO_FRAME_INFO_S *frame, cvtdl_image_t *p_dst);
DLL_EXPORT CVI_S32 CVI_TDL_Resize_VideoFrame(const cvitdl_handle_t handle,
                                             CVI_TDL_SUPPORTED_MODEL_E model,
                                             VIDEO_FRAME_INFO_S *frame, const int dst_w,
                                             const int dst_h, PIXEL_FORMAT_E dst_format,
                                             VIDEO_FRAME_INFO_S **dst_frame);
DLL_EXPORT CVI_S32 CVI_TDL_Release_VideoFrame(const cvitdl_handle_t handle,
                                              CVI_TDL_SUPPORTED_MODEL_E model,
                                              VIDEO_FRAME_INFO_S *frame, bool del_frame);
DLL_EXPORT CVI_S32 CVI_TDL_Change_Img(const cvitdl_handle_t handle,
                                      CVI_TDL_SUPPORTED_MODEL_E model_type,
                                      VIDEO_FRAME_INFO_S *frame, VIDEO_FRAME_INFO_S **dst_frame,
                                      PIXEL_FORMAT_E enDstFormat);

DLL_EXPORT CVI_S32 CVI_TDL_Delete_Img(const cvitdl_handle_t handle,
                                      CVI_TDL_SUPPORTED_MODEL_E model_type,
                                      VIDEO_FRAME_INFO_S *p_f);
/**
 * @brief person and pet(cat,dog) Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_PersonPet_Detection(const cvitdl_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta);

/**
 * @brief Yolov8 Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_YOLOV8_Detection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                            cvtdl_object_t *obj_meta);

/**
 * @brief Yolov10 Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_YOLOV10_Detection(const cvitdl_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta);

/**
 * @brief car,bus,truck,person,bike,motor Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_PersonVehicle_Detection(const cvitdl_handle_t handle,
                                                   VIDEO_FRAME_INFO_S *frame,
                                                   cvtdl_object_t *obj_meta);
/**
 * @brief hand,face,person Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_HandFacePerson_Detection(const cvitdl_handle_t handle,
                                                    VIDEO_FRAME_INFO_S *frame,
                                                    cvtdl_object_t *obj_meta);
/**
 * @brief human keypoints detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param model_index The object detection model id selected to use.
 * @param obj_meta cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t and
 * pedestrian_properity must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_PoseDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         CVI_TDL_SUPPORTED_MODEL_E model_index,
                                         cvtdl_object_t *obj_meta);

/**
 * @brief human keypoints detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_object_t structure, the cvtdl_object_info_t and cvtdl_bbox_t and
 * pedestrian_properity must be set.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Super_Resolution(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                            cvtdl_sr_feature *srfeature);

DLL_EXPORT CVI_S32 CVI_TDL_OCR_Recognition(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                           cvtdl_object_t *obj_meta);

/**
 * @brief image classification
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_class_meta_t structure, top 5 class info and score
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Image_Classification(const cvitdl_handle_t handle,
                                                VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_class_meta_t *obj_meta);

/**
 * @brief occlusion classification
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_class_meta_t structure, no occlusion score and occlusion score
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Occlusion_Classification(const cvitdl_handle_t handle,
                                                VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_class_meta_t *obj_meta);

 /**
 * @brief Topformer segmentation.
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param out_frame Output frame which each pixel represents class label.
 * @param filter Class id filter. Set NULL to ignore.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Topformer_Seg(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                     cvtdl_seg_t *filter);   


DLL_EXPORT CVI_S32 CVI_TDL_Set_Segmentation_DownRato(const cvitdl_handle_t handle,
                                             const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                             int down_rato);
/**
 * @brief raw image classification
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_class_meta_t structure, top 5 class info and score
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Raw_Image_Classification(const cvitdl_handle_t handle,
                                                    VIDEO_FRAME_INFO_S *frame,
                                                    cvtdl_class_meta_t *obj_meta);

/**
 * @brief isp image classification
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param object cvtdl_class_meta_t structure, top 5 class info and score
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Isp_Image_Classification(const cvitdl_handle_t handle,
                                                    VIDEO_FRAME_INFO_S *frame,
                                                    cvtdl_class_meta_t *obj_meta,
                                                    cvtdl_isp_meta_t *isp_meta);

/**
 * @brief get model preprocess param struct
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @return  InputPreParam preprocess param struct.
 */
DLL_EXPORT InputPreParam CVI_TDL_GetPreParam(const cvitdl_handle_t handle,
                                             const CVI_TDL_SUPPORTED_MODEL_E model_index);

/**
 * @brief set model preprocess param struct
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @param pre_param InputPreParam preprocess struct
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetPreParam(const cvitdl_handle_t handle,
                                       const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                       InputPreParam pre_param);

/**
 * @brief get Detection algorithm param struct
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @return  cvtdl_det_algo_param_t Detection algorthm param struct.
 */
DLL_EXPORT cvtdl_det_algo_param_t CVI_TDL_GetDetectionAlgoParam(
    const cvitdl_handle_t handle, const CVI_TDL_SUPPORTED_MODEL_E model_index);

/**
 * @brief set Detection algorithm param struct
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @param alg_param cvtdl_det_algo_param_t Detection algorithm struct
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetDetectionAlgoParam(const cvitdl_handle_t handle,
                                                 const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                                 cvtdl_det_algo_param_t alg_param);

DLL_EXPORT CVI_S32 CVI_TDL_Set_Yolov5_ROI(const cvitdl_handle_t handle, Point_t roi_s);
/**
 * @brief image_classification setup function
 *
 * @param handle An TDL SDK handle.
 * @param p_preprocess_cfg Input preprocess setup config.
 * @int Reture CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Byte(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                         cvtdl_tracker_t *tracker, bool use_reid);

/**
 * @brief get frame feature from clip model
 *
 * @param frame input image
 * @param cvtdl_clip_feature save feature and dim, need custom free
 */
DLL_EXPORT CVI_S32 CVI_TDL_Clip_Image_Feature(const cvitdl_handle_t handle,
                                              VIDEO_FRAME_INFO_S *frame,
                                              cvtdl_clip_feature *clip_feature);

/**
 * @brief get frame feature from openclip model
 *
 * @param frame input image
 * @param cvtdl_clip_feature save feature and dim, need custom free
 */
// DLL_EXPORT CVI_S32 CVI_TDL_OpenClip_Image_Feature(const cvitdl_handle_t handle,
// VIDEO_FRAME_INFO_S *frame,
//                                      cvtdl_clip_feature *clip_feature);

/**
 * @brief get frame feature from clip model
 *
 * @param frame input image
 * @param cvtdl_clip_feature save feature and dim, need custom free
 */
DLL_EXPORT CVI_S32 CVI_TDL_Clip_Text_Feature(const cvitdl_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame,
                                             cvtdl_clip_feature *clip_feature);

DLL_EXPORT CVI_S32 CVI_TDL_YoloV8_Seg(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                      cvtdl_object_t *obj_meta);

/**
 * @brief get audio algorithm param
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @return cvitdl_sound_param audio algorithm param struct.
 */
DLL_EXPORT cvitdl_sound_param CVI_TDL_GetSoundClassificationParam(
    const cvitdl_handle_t handle, const CVI_TDL_SUPPORTED_MODEL_E model_index);

/**
 * @brief set audio algorithm param
 *
 * @param handle An TDL SDK handle.
 * @param model_index Supported model list.
 * @param audio_param audio algorithm param struct.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SetSoundClassificationParam(const cvitdl_handle_t handle,
                                                       const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                                       cvitdl_sound_param audio_param);

/**
 * @brief Lane Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param lane_meta cvtdl_lane_t structure,.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Lane_Det(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                    cvtdl_lane_t *lane_meta);

/**
 * @brief Polylane Detection
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param lane_meta cvtdl_lane_t structure,.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_PolyLane_Det(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                        cvtdl_lane_t *lane_meta);

DLL_EXPORT CVI_S32 CVI_TDL_Set_Polylanenet_Lower(const cvitdl_handle_t handle,
                                                 const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                                 float th);

DLL_EXPORT CVI_S32 CVI_TDL_LSTR_Det(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                    cvtdl_lane_t *lane_meta);

DLL_EXPORT CVI_S32 CVI_TDL_Set_LSTR_ExportFeature(const cvitdl_handle_t handle,
                                                  const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                                  int flag);

DLL_EXPORT CVI_S32 CVI_TDL_Depth_Stereo(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame1,
                                        VIDEO_FRAME_INFO_S *frame2,
                                        cvtdl_depth_logits_t *depth_logist);


#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_CORE_H_
