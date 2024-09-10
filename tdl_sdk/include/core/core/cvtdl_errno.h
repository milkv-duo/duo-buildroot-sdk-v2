#ifndef _CVI_CORE_ERROR_H_
#define _CVI_CORE_ERROR_H_

#include "cvi_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CVI_TDL Error Code Definition */
/***********************************************************/
/*|-------------------------------------------------------|*/
/*| 11|   MODULE_ID   |   FUNC_ID    |   ERR_ID           |*/
/*|-------------------------------------------------------|*/
/*|<--><----8bits----><-----8bits----><------8bits------->|*/
/***********************************************************/
#define CVI_TDL_DEF_ERR(module, func, errid) \
  ((CVI_S32)(0xC0000000L | ((module) << 16) | ((func) << 8) | (errid)))

// clang-format off
typedef enum _MODULE_ID {
  CVI_TDL_MODULE_ID_CORE       = 1,
  CVI_TDL_MODULE_ID_SERVICE    = 2,
  CVI_TDL_MODULE_ID_EVALUATION = 3,
} CVI_MODULE_ID;

typedef enum _CVI_TDL_FUNC_ID {
  CVI_TDL_FUNC_ID_CORE                       = 1,
  CVI_TDL_FUNC_ID_DEEPSORT                   = 2,
  CVI_TDL_FUNC_ID_ALPHA_POSE                 = 3,
  CVI_TDL_FUNC_ID_FACE_RECORGNITION          = 4,
  CVI_TDL_FUNC_ID_FACE_LANDMARKER            = 5,
  CVI_TDL_FUNC_ID_FACE_QUALITY               = 6,
  CVI_TDL_FUNC_ID_FALL_DETECTION             = 7,
  CVI_TDL_FUNC_ID_INCAR_OBJECT_DETECTION     = 8,
  CVI_TDL_FUNC_ID_LICENSE_PLATE_DETECTION    = 9,
  CVI_TDL_FUNC_ID_LICENSE_PLATE_RECOGNITION  = 10,
  CVI_TDL_FUNC_ID_LIVENESS                   = 11,
  CVI_TDL_FUNC_ID_MASK_CLASSIFICATION        = 12,
  CVI_TDL_FUNC_ID_MASK_FACE_RECOGNITION      = 13,
  CVI_TDL_FUNC_ID_MOTION_DETECTION           = 14,
  CVI_TDL_FUNC_ID_MOBILEDET                  = 15,
  CVI_TDL_FUNC_ID_YOLOV3                     = 16,
  CVI_TDL_FUNC_ID_OSNET                      = 17,
  CVI_TDL_FUNC_ID_RETINAFACE                 = 18,
  CVI_TDL_FUNC_ID_SEGMENTATION               = 19,
  CVI_TDL_FUNC_ID_SMOKE_CLASSIFICATION       = 20,
  CVI_TDL_FUNC_ID_SOUND_CLASSIFICATION       = 21,
  CVI_TDL_FUNC_ID_TAMPER_DETECTION           = 22,
  CVI_TDL_FUNC_ID_THERMAL_FACE_DETECTION     = 23,
  CVI_TDL_FUNC_ID_YAWN_CLASSIFICATION        = 24,
  CVI_TDL_FUNC_ID_THERMAL_PERSON_DETECTION   = 25,
  CVI_TDL_FUNC_ID_FACE_MASK_DETECTION        = 26,
} CVI_TDL_FUNC_ID;

typedef enum _CVI_TDL_CORE_ERROR_ID {
  EN_CVI_TDL_INVALID_MODEL_PATH         = 1,
  EN_CVI_TDL_FAILED_OPEN_MODEL          = 2,
  EN_CVI_TDL_FAILED_CLOSE_MODEL         = 3,
  EN_CVI_TDL_FAILED_GET_VPSS_CHN_CONFIG = 4,
  EN_CVI_TDL_FAILED_INFERENCE           = 5,
  EN_CVI_TDL_INVALID_ARGS               = 6,
  EN_CVI_TDL_FAILED_VPSS_INIT           = 7,
  EN_CVI_TDL_FAILED_VPSS_SEND_FRAME     = 8,
  EN_CVI_TDL_FAILED_VPSS_GET_FRAME      = 9,
  EN_CVI_TDL_MODEL_INITIALIZED          = 10,
  EN_CVI_TDL_NOT_YET_INITIALIZED        = 11,
  EN_CVI_TDL_NOT_YET_IMPLEMENTED        = 12,
  EN_CVI_TDL_ERR_ALLOC_ION_FAIL         = 13,
} CVI_TDL_CORE_ERROR_ID;

typedef enum _CVI_TDL_MD_ERROR_ID {
  EN_CVI_TDL_OPER_FAILED = 1,
} CVI_TDL_MD_ERROR_ID;

/** @enum CVI_TDL_RC_CODE
 * @ingroup core_cvitdlcore
 * @brief Return code for CVI_TDL.
 */
typedef enum _CVI_TDL_RC_CODE {
  /* Return code for general condition*/
  // Success
  CVI_TDL_SUCCESS                     = CVI_SUCCESS,
  // General failure
  CVI_TDL_FAILURE                     = CVI_FAILURE,
  // Invalid DNN model path
  CVI_TDL_ERR_INVALID_MODEL_PATH      = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_INVALID_MODEL_PATH),
  // Failed to open DNN model
  CVI_TDL_ERR_OPEN_MODEL              = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_OPEN_MODEL),
  // Failed to close DNN model
  CVI_TDL_ERR_CLOSE_MODEL             = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_CLOSE_MODEL),
  // Failed to get vpss channel config for DNN model
  CVI_TDL_ERR_GET_VPSS_CHN_CONFIG     = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_GET_VPSS_CHN_CONFIG),
  // Failed to inference
  CVI_TDL_ERR_INFERENCE               = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_INFERENCE),
  // Invalid model arguments
  CVI_TDL_ERR_INVALID_ARGS            = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_INVALID_ARGS),
  // Failed to initialize VPSS
  CVI_TDL_ERR_INIT_VPSS               = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_VPSS_INIT),
  // VPSS send frame fail
  CVI_TDL_ERR_VPSS_SEND_FRAME         = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_VPSS_SEND_FRAME),
  // VPSS get frame fail
  CVI_TDL_ERR_VPSS_GET_FRAME          = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_FAILED_VPSS_GET_FRAME),
  // Model has initialized
  CVI_TDL_ERR_MODEL_INITIALIZED       = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_MODEL_INITIALIZED),
  // Not yet initialized
  CVI_TDL_ERR_NOT_YET_INITIALIZED     = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_NOT_YET_INITIALIZED),
  // Not yet implemented
  CVI_TDL_ERR_NOT_YET_IMPLEMENTED     = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_NOT_YET_IMPLEMENTED),
  // Failed to allocate ION
  CVI_TDL_ERR_ALLOC_ION_FAIL          = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_CORE, EN_CVI_TDL_ERR_ALLOC_ION_FAIL),
  /* Algorithm specific return code */

  // Operation failed of Motion Detection
  CVI_TDL_ERR_MD_OPERATION_FAILED     = CVI_TDL_DEF_ERR(CVI_TDL_MODULE_ID_CORE, CVI_TDL_FUNC_ID_MOTION_DETECTION, EN_CVI_TDL_OPER_FAILED),

} CVI_TDL_RC_CODE;
// clang-format on

#ifdef __cplusplus
}
#endif

#endif
