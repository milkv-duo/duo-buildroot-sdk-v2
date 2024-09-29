#include "core/cvi_tdl_core.h"
#include "clip/clip_image/clip_image.hpp"
#include "clip/clip_text/clip_text.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "cvi_tdl_core_internal.hpp"
#include "cvi_tdl_experimental.h"
#include "cvi_tdl_log.hpp"

#include "deepsort/cvi_deepsort.hpp"
#include "face_attribute/face_attribute.hpp"
#include "face_attribute_cls/face_attribute_cls.hpp"
#include "face_landmarker/dms_landmark.hpp"
#include "face_landmarker/face_landmark_det3.hpp"
#include "face_landmarker/face_landmarker.hpp"
#include "face_landmarker/face_landmarker_det2.hpp"

#include "hand_classification/hand_classification.hpp"
#include "hand_keypoint/hand_keypoint.hpp"
#include "hand_keypoint_classification/hand_keypoint_classification.hpp"

#include "human_keypoints_detection/hrnet/hrnet.hpp"
#include "human_keypoints_detection/simcc/simcc.hpp"
#include "human_keypoints_detection/yolov8_pose/yolov8_pose.hpp"

#include "image_classification/image_classification.hpp"
#include "incar_object_detection/incar_object_detection.hpp"
#include "lane_detection/lane_detection.hpp"
#include "lane_detection/lstr/lstr.hpp"
#include "lane_detection/polylanenet/polylanenet.hpp"

#include "license_plate_detection/license_plate_detection.hpp"
#include "license_plate_recognition/license_plate_recognitionv2.hpp"
#include "liveness/ir_liveness/ir_liveness.hpp"
#include "motion_detection/md.hpp"
#include "motion_segmentation/motion_segmentation.hpp"

#include "object_detection/mobiledetv2/mobiledetv2.hpp"
#include "object_detection/ppyoloe/ppyoloe.hpp"
#include "object_detection/thermal_person_detection/thermal_person.hpp"
#include "object_detection/yolo/yolo.hpp"
#include "object_detection/yolov10/yolov10.hpp"
#include "object_detection/yolov3/yolov3.hpp"
#include "object_detection/yolov5/yolov5.hpp"
#include "object_detection/yolov6/yolov6.hpp"
#include "object_detection/yolov8/yolov8.hpp"
#include "object_detection/yolox/yolox.hpp"

#include "depth_estimation/stereo.hpp"
#include "face_detection/face_mask_detection/retinaface_yolox.hpp"
#include "face_detection/retina_face/retina_face.hpp"
#include "face_detection/retina_face/scrfd_face.hpp"
#include "face_detection/thermal_face_detection/thermal_face.hpp"
#include "mask_classification/mask_classification.hpp"
#include "ocr/ocr_recognition/ocr_recognition.hpp"
#include "osnet/osnet.hpp"
#include "raw_image_classification/raw_image_classification.hpp"
#include "segmentation/deeplabv3.hpp"
#include "sound_classification/sound_classification_v2.hpp"
#include "super_resolution/super_resolution.hpp"
#ifdef CV186X
#include "isp_image_classification/isp_image_classification.hpp"
#endif
#ifndef NO_OPENCV
#include "eye_classification/eye_classification.hpp"
#include "face_quality/face_quality.hpp"
#include "fall_detection/fall_det_monitor.hpp"
#include "fall_detection/fall_detection.hpp"
#include "instance_segmentation/yolov8_seg/yolov8_seg.hpp"
#include "license_plate_recognition/license_plate_recognition.hpp"
#include "liveness/liveness.hpp"
#include "mask_face_recognition/mask_face_recognition.hpp"
#include "ocr/ocr_detection/ocr_detection.hpp"
#include "opencv2/opencv.hpp"
#include "smoke_classification/smoke_classification.hpp"
#include "utils/image_utils.hpp"
#include "yawn_classification/yawn_classification.hpp"
#endif
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "utils/clip_postprocess.hpp"
#include "utils/core_utils.hpp"
#include "utils/token.hpp"
#include "version.hpp"

using namespace std;
using namespace cvitdl;

struct ModelParams {
  VpssEngine *vpss_engine;
  uint32_t vpss_timeout_value;
};

using CreatorFunc = std::function<Core *(const ModelParams &)>;
using CreatorFuncAud = std::function<Core *()>;
using namespace std::placeholders;

template <typename C, typename... Args>
Core *create_model(const ModelParams &params, Args... arg) {
  C *instance = new C(arg...);

  instance->setVpssEngine(params.vpss_engine);
  instance->setVpssTimeout(params.vpss_timeout_value);
  return instance;
}

template <typename C>
Core *create_model_aud() {
  C *instance = new C();
  return instance;
}

static int createIVEHandleIfNeeded(cvitdl_context_t *ctx) {
  if (ctx->ive_handle == nullptr) {
    ctx->ive_handle = new ive::IVE;
    if (ctx->ive_handle->init() != CVI_SUCCESS) {
      LOGC("IVE handle init failed, please insmod cv18?x_ive.ko.\n");
      return CVI_FAILURE;
    }
  }
  return CVI_SUCCESS;
}

static CVI_S32 initVPSSIfNeeded(cvitdl_context_t *ctx, CVI_TDL_SUPPORTED_MODEL_E model_id) {
  bool skipped;
  CVI_S32 ret = CVI_TDL_GetSkipVpssPreprocess(ctx, model_id, &skipped);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  // Don't create vpss if preprocessing is skipped.
  if (skipped) {
    return CVI_TDL_SUCCESS;
  }

  if (ctx->vec_vpss_engine.size() == 0) {
    return CVI_TDL_SUCCESS;
  }

  uint32_t thread;
  ret = CVI_TDL_GetVpssThread(ctx, model_id, &thread);
  if (ret == CVI_TDL_SUCCESS) {
    if (!ctx->vec_vpss_engine[thread]->isInitialized()) {
      ret = ctx->vec_vpss_engine[thread]->init();
    }
  }
  return ret;
}

// Convenience macros for creator
#define CREATOR(type) CreatorFunc(create_model<type>)

// Convenience macros for creator
#define CREATOR_AUD(type) CreatorFuncAud(create_model_aud<type>)

// Convenience macros for creator, P{NUM} stands for how many parameters for creator
#define CREATOR_P1(type, arg_type, arg1) \
  CreatorFunc(std::bind(create_model<type, arg_type>, _1, arg1))

/**
 * IMPORTANT!!
 * Creators for all DNN model. Please remember to register model creator here, or
 * TDL SDK cannot instantiate model correctly.
 */
unordered_map<int, CreatorFuncAud> MODEL_CREATORS_AUD = {
    {CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, CREATOR_AUD(SoundClassification)},
};

unordered_map<int, CreatorFunc> MODEL_CREATORS = {
#ifndef NO_OPENCV
    {CVI_TDL_SUPPORTED_MODEL_FACEQUALITY, CREATOR(FaceQuality)},
    {CVI_TDL_SUPPORTED_MODEL_LIVENESS, CREATOR(Liveness)},
    {CVI_TDL_SUPPORTED_MODEL_LP_RECONGNITION, CREATOR(LicensePlateRecognitionV2)},
    {CVI_TDL_SUPPORTED_MODEL_EYECLASSIFICATION, CREATOR(EyeClassification)},
    {CVI_TDL_SUPPORTED_MODEL_YAWNCLASSIFICATION, CREATOR(YawnClassification)},
    {CVI_TDL_SUPPORTED_MODEL_SMOKECLASSIFICATION, CREATOR(SmokeClassification)},
    {CVI_TDL_SUPPORTED_MODEL_LPRNET_TW, CREATOR_P1(LicensePlateRecognition, LP_FORMAT, TAIWAN)},
    {CVI_TDL_SUPPORTED_MODEL_LPRNET_CN, CREATOR_P1(LicensePlateRecognition, LP_FORMAT, CHINA)},
    {CVI_TDL_SUPPORTED_MODEL_OCR_DETECTION, CREATOR(OCRDetection)},
    {CVI_TDL_SUPPORTED_MODEL_MASKFACERECOGNITION, CREATOR(MaskFaceRecognition)},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG, CREATOR(YoloV8Seg)},
#endif
#ifdef CV186X
    {CVI_TDL_SUPPORTED_MODEL_ISP_IMAGE_CLASSIFICATION, CREATOR(IspImageClassification)},
#endif
    {CVI_TDL_SUPPORTED_MODEL_IRLIVENESS, CREATOR(IrLiveness)},

    {CVI_TDL_SUPPORTED_MODEL_YOLOV3, CREATOR(Yolov3)},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV5, CREATOR(Yolov5)},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV6, CREATOR(Yolov6)},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV7, CREATOR(Yolov5)},
    {CVI_TDL_SUPPORTED_MODEL_YOLO, CREATOR(Yolo)},
    {CVI_TDL_SUPPORTED_MODEL_YOLOX, CREATOR(YoloX)},
    {CVI_TDL_SUPPORTED_MODEL_PPYOLOE, CREATOR(PPYoloE)},
    {CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, CREATOR(ScrFDFace)},
    {CVI_TDL_SUPPORTED_MODEL_RETINAFACE, CREATOR_P1(RetinaFace, PROCESS, CAFFE)},
    {CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, CREATOR_P1(RetinaFace, PROCESS, PYTORCH)},
    {CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE, CREATOR_P1(FaceAttribute, bool, true)},
    {CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, CREATOR_P1(FaceAttribute, bool, false)},

    {CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 1))},
    {CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 3))},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 80))},
    {CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 7))},
    {CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 3))},
    {CVI_TDL_SUPPORTED_MODEL_HEAD_PERSON_DETECTION,
     CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 2))},
    {CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80,
     CREATOR_P1(MobileDetV2, MobileDetV2::Category, MobileDetV2::Category::coco80)},
    {CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE,
     CREATOR_P1(MobileDetV2, MobileDetV2::Category, MobileDetV2::Category::person_vehicle)},
    {CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE,
     CREATOR_P1(MobileDetV2, MobileDetV2::Category, MobileDetV2::Category::vehicle)},
    {CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
     CREATOR_P1(MobileDetV2, MobileDetV2::Category, MobileDetV2::Category::pedestrian)},
    {CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS,
     CREATOR_P1(MobileDetV2, MobileDetV2::Category, MobileDetV2::Category::person_pets)},

    // {CVI_TDL_SUPPORTED_MODEL_YOLOV8_HARDHAT,
    //  CREATOR_P1(YoloV8Detection, PAIR_INT, std::make_pair(64, 2))},
    {CVI_TDL_SUPPORTED_MODEL_LANE_DET, CREATOR(BezierLaneNet)},

    {CVI_TDL_SUPPORTED_MODEL_LSTR, CREATOR(LSTR)},

#ifndef CONFIG_ALIOS
    {CVI_TDL_SUPPORTED_MODEL_THERMALFACE, CREATOR(ThermalFace)},
    {CVI_TDL_SUPPORTED_MODEL_THERMALPERSON, CREATOR(ThermalPerson)},

    {CVI_TDL_SUPPORTED_MODEL_MASKCLASSIFICATION, CREATOR(MaskClassification)},
    {CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, CREATOR(HandClassification)},
    {CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT, CREATOR(HandKeypoint)},
    {CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT_CLASSIFICATION, CREATOR(HandKeypointClassification)},

    {CVI_TDL_SUPPORTED_MODEL_FACEMASKDETECTION, CREATOR(RetinafaceYolox)},
    {CVI_TDL_SUPPORTED_MODEL_OSNET, CREATOR(OSNet)},
    {CVI_TDL_SUPPORTED_MODEL_WPODNET, CREATOR(LicensePlateDetection)},

    {CVI_TDL_SUPPORTED_MODEL_DEEPLABV3, CREATOR(Deeplabv3)},
    {CVI_TDL_SUPPORTED_MODEL_MOTIONSEGMENTATION, CREATOR(MotionSegmentation)},

    {CVI_TDL_SUPPORTED_MODEL_FACELANDMARKER, CREATOR(FaceLandmarker)},
    {CVI_TDL_SUPPORTED_MODEL_FACELANDMARKERDET2, CREATOR(FaceLandmarkerDet2)},
    {CVI_TDL_SUPPORTED_MODEL_INCAROBJECTDETECTION, CREATOR(IncarObjectDetection)},

    {CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, CREATOR(FaceAttribute_cls)},

    {CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE,
     CREATOR_P1(YoloV8Pose, TUPLE_INT, std::make_tuple(64, 17, 1))},
    {CVI_TDL_SUPPORTED_MODEL_LP_DETECTION,
     CREATOR_P1(YoloV8Pose, TUPLE_INT, std::make_tuple(64, 4, 2))},
    {CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION,
     CREATOR_P1(YoloV10Detection, PAIR_INT, std::make_pair(64, 80))},
    {CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE, CREATOR(Simcc)},
    {CVI_TDL_SUPPORTED_MODEL_HRNET_POSE, CREATOR(Hrnet)},
    {CVI_TDL_SUPPORTED_MODEL_LANDMARK_DET3, CREATOR(FaceLandmarkDet3)},
    {CVI_TDL_SUPPORTED_MODEL_DMSLANDMARKERDET, CREATOR(DMSLandmarkerDet)},
    {CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION, CREATOR(ImageClassification)},
    {CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE, CREATOR(Clip_Image)},
    {CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT, CREATOR(Clip_Text)},
    {CVI_TDL_SUPPORTED_MODEL_RAW_IMAGE_CLASSIFICATION, CREATOR(RawImageClassification)},

    {CVI_TDL_SUPPORTED_MODEL_POLYLANE, CREATOR(Polylanenet)},
    {CVI_TDL_SUPPORTED_MODEL_SUPER_RESOLUTION, CREATOR(SuperResolution)},

    {CVI_TDL_SUPPORTED_MODEL_OCR_RECOGNITION, CREATOR(OCRRecognition)},
    {CVI_TDL_SUPPORTED_MODEL_STEREO, CREATOR(Stereo)},
#endif
};

//*************************************************
// Experimental features
void CVI_TDL_EnableGDC(cvitdl_handle_t handle, bool use_gdc) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  ctx->use_gdc_wrap = use_gdc;
  LOGI("Experimental feature GDC hardware %s.\n", use_gdc ? "enabled" : "disabled");
}
//*************************************************

#ifndef NO_OPENCV
inline void __attribute__((always_inline)) removeCtx(cvitdl_context_t *ctx) {
  delete ctx->ds_tracker;
  delete ctx->td_model;
  delete ctx->md_model;

  if (ctx->ive_handle) {
    ctx->ive_handle->destroy();
  }

  for (auto it : ctx->vec_vpss_engine) {
    delete it;
  }
  delete ctx;
}
#else
inline void __attribute__((always_inline)) removeCtx(cvitdl_context_t *ctx) {
  if (ctx->ds_tracker) {
    delete ctx->ds_tracker;
    ctx->ds_tracker = nullptr;
  }

  // delete ctx->td_model;
  if (ctx->md_model) {
    delete ctx->md_model;
    ctx->md_model = nullptr;
  }

  if (ctx->ive_handle) {
    ctx->ive_handle->destroy();
    ctx->ive_handle = nullptr;
  }

  for (auto it : ctx->vec_vpss_engine) {
    delete it;
  }
  delete ctx;
}
#endif

inline Core *__attribute__((always_inline))
getInferenceInstance(const CVI_TDL_SUPPORTED_MODEL_E index, cvitdl_context_t *ctx) {
  cvitdl_model_t &m_t = ctx->model_cont[index];
  if (m_t.instance == nullptr) {
    // create custom instance here
    if (index == CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION) {
      if (MODEL_CREATORS_AUD.find(index) == MODEL_CREATORS_AUD.end()) {
        LOGE("Cannot find creator for %s, Please register a creator for this model!\n",
             CVI_TDL_GetModelName(index));
        return nullptr;
      }
      auto creator = MODEL_CREATORS_AUD[index];
      m_t.instance = creator();
    } else {
      if (MODEL_CREATORS.find(index) == MODEL_CREATORS.end()) {
        LOGE("Cannot find creator for %s, Please register a creator for this model!\n",
             CVI_TDL_GetModelName(index));
        return nullptr;
      }

      auto creator = MODEL_CREATORS[index];
      ModelParams params = {.vpss_engine = ctx->vec_vpss_engine[m_t.vpss_thread],
                            .vpss_timeout_value = ctx->vpss_timeout_value};

      m_t.instance = creator(params);
      m_t.instance->setVpssEngine(ctx->vec_vpss_engine[m_t.vpss_thread]);
      m_t.instance->setVpssTimeout(ctx->vpss_timeout_value);
    }
  }
  return m_t.instance;
}

CVI_S32 CVI_TDL_CreateHandle(cvitdl_handle_t *handle) {
  return CVI_TDL_CreateHandle2(handle, -1, 0);
}

CVI_S32 CVI_TDL_CreateHandle2(cvitdl_handle_t *handle, const VPSS_GRP vpssGroupId,
                              const CVI_U8 vpssDev) {
  if (vpssGroupId < -1 || vpssGroupId >= VPSS_MAX_GRP_NUM) {
    LOGE("Invalid Vpss Grp: %d.\n", vpssGroupId);
    return CVI_TDL_ERR_INIT_VPSS;
  }

  cvitdl_context_t *ctx = new cvitdl_context_t;
  ctx->ive_handle = NULL;
  ctx->vec_vpss_engine.push_back(new VpssEngine(vpssGroupId, vpssDev));
  const char timestamp[] = __DATE__ " " __TIME__;
  LOGI("cvitdl_handle_t is created, version %s-%s\n", CVI_TDL_TAG, timestamp);
  *handle = ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_CreateHandle3(cvitdl_handle_t *handle) {
  cvitdl_context_t *ctx = new cvitdl_context_t;
  ctx->ive_handle = NULL;
  const char timestamp[] = __DATE__ " " __TIME__;
  LOGI("cvitdl_handle_t is created, version %s-%s\n", CVI_TDL_TAG, timestamp);
  *handle = ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_DestroyHandle(cvitdl_handle_t handle) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  CVI_TDL_CloseAllModel(handle);
  removeCtx(ctx);
  LOGI("cvitdl_handle_t is destroyed.\n");
  return CVI_TDL_SUCCESS;
}

static bool checkModelFile(const char *filepath) {
  struct stat buffer;
  bool ret = false;
  if (stat(filepath, &buffer) == 0) {
    if (S_ISREG(buffer.st_mode)) {
      ret = true;
    } else {
      LOGE("Path of model file isn't a regular file: %s\n", filepath);
    }
  } else {
    LOGE("Model file doesn't exists: %s\n", filepath);
  }
  return ret;
}

CVI_S32 CVI_TDL_OpenModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                          const char *filepath) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t &m_t = ctx->model_cont[config];
  Core *instance = getInferenceInstance(config, ctx);

  if (instance != nullptr) {
    if (instance->isInitialized()) {
      LOGW("%s: Inference has already initialized. Please call CVI_TDL_CloseModel to reset.\n",
           CVI_TDL_GetModelName(config));
      return CVI_TDL_ERR_MODEL_INITIALIZED;
    }
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  if (!checkModelFile(filepath)) {
    return CVI_TDL_ERR_INVALID_MODEL_PATH;
  }

  m_t.model_path = filepath;
  CVI_S32 ret = m_t.instance->modelOpen(m_t.model_path.c_str());
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("Failed to open model: %s (%s)\n", CVI_TDL_GetModelName(config), m_t.model_path.c_str());
    return ret;
  }
  LOGI("Model is opened successfully: %s \n", CVI_TDL_GetModelName(config));
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetModelInputTpye(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                  int *inputDTpye) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    *inputDTpye = instance->getModelInputDType();
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

#ifndef CV186X
CVI_S32 CVI_TDL_OpenModel_FromBuffer(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                     int8_t *buf, uint32_t size) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t &m_t = ctx->model_cont[config];
  Core *instance = getInferenceInstance(config, ctx);

  if (instance != nullptr) {
    if (instance->isInitialized()) {
      LOGW("%s: Inference has already initialized. Please call CVI_TDL_CloseModel to reset.\n",
           CVI_TDL_GetModelName(config));
      return CVI_TDL_ERR_MODEL_INITIALIZED;
    }
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  m_t.buf = buf;
  CVI_S32 ret = m_t.instance->modelOpen(m_t.buf, size);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("Failed to open model: %s (%d)", CVI_TDL_GetModelName(config), (int)*m_t.buf);
    return ret;
  }
  LOGI("Model is opened successfully: %s \n", CVI_TDL_GetModelName(config));
  return CVI_TDL_SUCCESS;
}
#endif

const char *CVI_TDL_GetModelPath(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  return GetModelName(ctx->model_cont[config]);
}

CVI_S32 CVI_TDL_SetSkipVpssPreprocess(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                      bool skip) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    instance->skipVpssPreprocess(skip);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetPerfEvalInterval(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                    int interval) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    instance->set_perf_eval_interval(interval);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetSkipVpssPreprocess(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                      bool *skip) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    *skip = instance->hasSkippedVpssPreprocess();
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetModelThreshold(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                  float threshold) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    instance->setModelThreshold(threshold);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetModelThreshold(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                  float *threshold) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    *threshold = instance->getModelThreshold();
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetModelNmsThreshold(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                     float threshold) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    instance->setModelNmsThreshold(threshold);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetModelNmsThreshold(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                     float *threshold) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    *threshold = instance->getModelNmsThreshold();
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_UseMmap(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model, bool mmap) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(model, ctx);
  if (instance == nullptr) {
    return CVI_TDL_FAILURE;
  }
  instance->setUseMmap(mmap);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                              const uint32_t thread) {
  return CVI_TDL_SetVpssThread2(handle, config, thread, -1, 0);
}

CVI_S32 CVI_TDL_SetVpssThread2(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                               const uint32_t thread, const VPSS_GRP vpssGroupId,
                               const CVI_U8 dev) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    return setVPSSThread(ctx->model_cont[config], ctx->vec_vpss_engine, thread, vpssGroupId, dev);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
}

CVI_S32 CVI_TDL_SetVBPool(cvitdl_handle_t handle, uint32_t thread, VB_POOL pool_id) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (thread >= ctx->vec_vpss_engine.size()) {
    LOGE("Invalid vpss thread: %d, should be 0 to %d\n", thread,
         static_cast<uint32_t>(ctx->vec_vpss_engine.size() - 1));
    return CVI_TDL_FAILURE;
  }
  ctx->vec_vpss_engine[thread]->attachVBPool(pool_id);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetVBPool(cvitdl_handle_t handle, uint32_t thread, VB_POOL *pool_id) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (thread >= ctx->vec_vpss_engine.size()) {
    LOGE("Invalid vpss thread: %d, should be 0 to %d\n", thread,
         static_cast<uint32_t>(ctx->vec_vpss_engine.size() - 1));
    return CVI_TDL_FAILURE;
  }
  *pool_id = ctx->vec_vpss_engine[thread]->getVBPool();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                              uint32_t *thread) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  *thread = ctx->model_cont[config].vpss_thread;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetVpssDepth(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                             uint32_t input_id, uint32_t depth) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(model, ctx);
  if (instance != nullptr) {
    return instance->setVpssDepth(input_id, depth);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(model));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
}

CVI_S32 CVI_TDL_GetVpssDepth(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model,
                             uint32_t input_id, uint32_t *depth) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Core *instance = getInferenceInstance(model, ctx);
  if (instance != nullptr) {
    return instance->getVpssDepth(input_id, depth);
  } else {
    LOGE("Cannot create model: %s\n", CVI_TDL_GetModelName(model));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
}

CVI_S32 CVI_TDL_GetVpssGrpIds(cvitdl_handle_t handle, VPSS_GRP **groups, uint32_t *num) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  VPSS_GRP *ids = (VPSS_GRP *)malloc(ctx->vec_vpss_engine.size() * sizeof(VPSS_GRP));
  for (size_t i = 0; i < ctx->vec_vpss_engine.size(); i++) {
    ids[i] = ctx->vec_vpss_engine[i]->getGrpId();
  }
  *groups = ids;
  *num = ctx->vec_vpss_engine.size();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_SetVpssTimeout(cvitdl_handle_t handle, uint32_t timeout) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  ctx->vpss_timeout_value = timeout;

  for (auto &m_inst : ctx->model_cont) {
    if (m_inst.second.instance != nullptr) {
      m_inst.second.instance->setVpssTimeout(timeout);
    }
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_CloseAllModel(cvitdl_handle_t handle) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  for (auto &m_inst : ctx->model_cont) {
    if (m_inst.second.instance != nullptr) {
      m_inst.second.instance->modelClose();
      LOGI("Model is closed: %s\n", CVI_TDL_GetModelName(m_inst.first));
      delete m_inst.second.instance;
      m_inst.second.instance = nullptr;
    }
  }
  for (auto &m_inst : ctx->custom_cont) {
    if (m_inst.instance != nullptr) {
      m_inst.instance->modelClose();
      delete m_inst.instance;
      m_inst.instance = nullptr;
    }
  }
  ctx->model_cont.clear();
  ctx->custom_cont.clear();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_CloseModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t &m_t = ctx->model_cont[config];
  if (m_t.instance == nullptr) {
    return CVI_TDL_ERR_CLOSE_MODEL;
  }

  m_t.instance->modelClose();
  LOGI("Model is closed: %s\n", CVI_TDL_GetModelName(config));
  delete m_t.instance;
  m_t.instance = nullptr;
  return CVI_TDL_SUCCESS;
}

// TODO: remove this func
CVI_S32 CVI_TDL_SelectDetectClass(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                  uint32_t num_selection, ...) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  va_list args;
  va_start(args, num_selection);

  std::vector<uint32_t> selected_classes;
  for (uint32_t i = 0; i < num_selection; i++) {
    uint32_t selected_class = va_arg(args, uint32_t);

    if (selected_class & CVI_TDL_DET_GROUP_MASK_HEAD) {
      uint32_t group_start = (selected_class & CVI_TDL_DET_GROUP_MASK_START) >> 16;
      uint32_t group_end = (selected_class & CVI_TDL_DET_GROUP_MASK_END);
      for (uint32_t i = group_start; i <= group_end; i++) {
        selected_classes.push_back(i);
      }
    } else {
      if (selected_class >= CVI_TDL_DET_TYPE_END) {
        LOGE("Invalid class id: %d\n", selected_class);
        return CVI_TDL_ERR_INVALID_ARGS;
      }
      selected_classes.push_back(selected_class);
    }
  }

  Core *instance = getInferenceInstance(config, ctx);
  if (instance != nullptr) {
    // TODO: only supports MobileDetV2 and YOLOX for now
    if (MobileDetV2 *mdetv2 = dynamic_cast<MobileDetV2 *>(instance)) {
      mdetv2->select_classes(selected_classes);
    } else {
      LOGW("CVI_TDL_SelectDetectClass only supports MobileDetV2for now.\n");
    }
  } else {
    LOGE("Failed to create model: %s\n", CVI_TDL_GetModelName(config));
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_GetVpssChnConfig(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                 const CVI_U32 frameWidth, const CVI_U32 frameHeight,
                                 const CVI_U32 idx, cvtdl_vpssconfig_t *chnConfig) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl::Core *instance = getInferenceInstance(config, ctx);
  if (instance == nullptr) {
    LOGE("Instance is null.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  return instance->getChnConfig(frameWidth, frameHeight, idx, chnConfig);
}

/**
 *  Convenience macros for defining inference functions. F{NUM} stands for how many input frame
 *  variables, P{NUM} stands for how many input parameters in inference function. All inference
 *  function should follow same function signature, that is,
 *  CVI_S32 inference(Frame1, Frame2, ... FrameN, Param1, Param2, ..., ParamN)
 */
#define DEFINE_INF_FUNC_F1_P1(func_name, class_name, model_index, arg_type)                    \
  CVI_S32 func_name(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame, arg_type arg1) {  \
    cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);                           \
    class_name *obj = dynamic_cast<class_name *>(getInferenceInstance(model_index, ctx));      \
    if (obj == nullptr) {                                                                      \
      LOGE("No instance found for %s.\n", #class_name);                                        \
      return CVI_TDL_ERR_OPEN_MODEL;                                                           \
    }                                                                                          \
    if (obj->isInitialized()) {                                                                \
      if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {                                 \
        return CVI_TDL_ERR_INIT_VPSS;                                                          \
      } else {                                                                                 \
        CVI_S32 ret = obj->inference(frame, arg1);                                             \
        if (ret != CVI_TDL_SUCCESS)                                                            \
          return ret;                                                                          \
        else                                                                                   \
          return obj->after_inference();                                                       \
      }                                                                                        \
    } else {                                                                                   \
      LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n", \
           CVI_TDL_GetModelName(model_index));                                                 \
      return CVI_TDL_ERR_NOT_YET_INITIALIZED;                                                  \
    }                                                                                          \
  }

#define DEFINE_INF_FUNC_F1_P2(func_name, class_name, model_index, arg1_type, arg2_type)        \
  CVI_S32 func_name(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame, arg1_type arg1,   \
                    arg2_type arg2) {                                                          \
    cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);                           \
    class_name *obj = dynamic_cast<class_name *>(getInferenceInstance(model_index, ctx));      \
    if (obj == nullptr) {                                                                      \
      LOGE("No instance found for %s.\n", #class_name);                                        \
      return CVI_TDL_ERR_OPEN_MODEL;                                                           \
    }                                                                                          \
    if (obj->isInitialized()) {                                                                \
      if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {                                 \
        return CVI_TDL_ERR_INIT_VPSS;                                                          \
      } else {                                                                                 \
        CVI_S32 ret = obj->inference(frame, arg1, arg2);                                       \
        if (ret != CVI_TDL_SUCCESS)                                                            \
          return ret;                                                                          \
        else                                                                                   \
          return obj->after_inference();                                                       \
      }                                                                                        \
    } else {                                                                                   \
      LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n", \
           CVI_TDL_GetModelName(model_index));                                                 \
      return CVI_TDL_ERR_NOT_YET_INITIALIZED;                                                  \
    }                                                                                          \
  }

#define DEFINE_INF_FUNC_F2_P1(func_name, class_name, model_index, arg_type)                    \
  CVI_S32 func_name(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame1,                  \
                    VIDEO_FRAME_INFO_S *frame2, arg_type arg1) {                               \
    cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);                           \
    class_name *obj = dynamic_cast<class_name *>(getInferenceInstance(model_index, ctx));      \
    if (obj == nullptr) {                                                                      \
      LOGE("No instance found for %s.\n", #class_name);                                        \
      return CVI_TDL_ERR_OPEN_MODEL;                                                           \
    }                                                                                          \
    if (obj->isInitialized()) {                                                                \
      if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {                                 \
        return CVI_TDL_ERR_INIT_VPSS;                                                          \
      } else {                                                                                 \
        CVI_S32 ret = obj->inference(frame1, frame2, arg1);                                    \
        if (ret != CVI_TDL_SUCCESS)                                                            \
          return ret;                                                                          \
        else                                                                                   \
          return obj->after_inference();                                                       \
      }                                                                                        \
    } else {                                                                                   \
      LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n", \
           CVI_TDL_GetModelName(model_index));                                                 \
      return CVI_TDL_ERR_NOT_YET_INITIALIZED;                                                  \
    }                                                                                          \
  }

#define DEFINE_INF_FUNC_F2_P2(func_name, class_name, model_index, arg1_type, arg2_type)        \
  CVI_S32 func_name(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame1,                  \
                    VIDEO_FRAME_INFO_S *frame2, arg1_type arg1, arg2_type arg2) {              \
    cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);                           \
    class_name *obj = dynamic_cast<class_name *>(getInferenceInstance(model_index, ctx));      \
    if (obj == nullptr) {                                                                      \
      LOGE("No instance found for %s.\n", #class_name);                                        \
      return CVI_TDL_ERR_OPEN_MODEL;                                                           \
    }                                                                                          \
    if (obj->isInitialized()) {                                                                \
      if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {                                 \
        return CVI_TDL_ERR_INIT_VPSS;                                                          \
      } else {                                                                                 \
        CVI_S32 ret = obj->inference(frame1, frame2, arg1, arg2);                              \
        if (ret != CVI_TDL_SUCCESS)                                                            \
          return ret;                                                                          \
        else                                                                                   \
          return obj->after_inference();                                                       \
      }                                                                                        \
    } else {                                                                                   \
      LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n", \
           CVI_TDL_GetModelName(model_index));                                                 \
      return CVI_TDL_ERR_NOT_YET_INITIALIZED;                                                  \
    }                                                                                          \
  }

/**
 *  Define model inference function here.
 *
 *  IMPORTANT!!
 *  Please remember to register creator function in MODEL_CREATORS first, or TDL SDK cannot
 *  find a correct way to create model object.
 *
 */
#ifndef NO_OPENCV
DEFINE_INF_FUNC_F2_P2(CVI_TDL_Liveness, Liveness, CVI_TDL_SUPPORTED_MODEL_LIVENESS, cvtdl_face_t *,
                      cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_OCR_Detection, OCRDetection, CVI_TDL_SUPPORTED_MODEL_OCR_DETECTION,
                      cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_EyeClassification, EyeClassification,
                      CVI_TDL_SUPPORTED_MODEL_EYECLASSIFICATION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_YawnClassification, YawnClassification,
                      CVI_TDL_SUPPORTED_MODEL_YAWNCLASSIFICATION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_SmokeClassification, SmokeClassification,
                      CVI_TDL_SUPPORTED_MODEL_SMOKECLASSIFICATION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_LicensePlateRecognition_TW, LicensePlateRecognition,
                      CVI_TDL_SUPPORTED_MODEL_LPRNET_TW, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_LicensePlateRecognition_CN, LicensePlateRecognition,
                      CVI_TDL_SUPPORTED_MODEL_LPRNET_CN, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P2(CVI_TDL_FaceQuality, FaceQuality, CVI_TDL_SUPPORTED_MODEL_FACEQUALITY,
                      cvtdl_face_t *, bool *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_MaskFaceRecognition, MaskFaceRecognition,
                      CVI_TDL_SUPPORTED_MODEL_MASKFACERECOGNITION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_YoloV8_Seg, YoloV8Seg, CVI_TDL_SUPPORTED_MODEL_YOLOV8_SEG,
                      cvtdl_object_t *)
CVI_S32 CVI_TDL_CropImage(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst, cvtdl_bbox_t *bbox,
                          bool cvtRGB888) {
  return crop_image(srcFrame, dst, bbox, cvtRGB888);
}

CVI_S32 CVI_TDL_CropImage_Exten(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                                cvtdl_bbox_t *bbox, bool cvtRGB888, float exten_ratio,
                                float *offset_x, float *offset_y) {
  return crop_image_exten(srcFrame, dst, bbox, cvtRGB888, exten_ratio, offset_x, offset_y);
}

CVI_S32 CVI_TDL_CropImage_Face(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                               cvtdl_face_info_t *face_info, bool align, bool cvtRGB888) {
  return crop_image_face(srcFrame, dst, face_info, align, cvtRGB888);
}

// Fall Detection

CVI_S32 CVI_TDL_Fall(const cvitdl_handle_t handle, cvtdl_object_t *objects) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  FallMD *fall_model = ctx->fall_model;
  if (fall_model == nullptr) {
    LOGD("Init Fall Detection Model.\n");
    ctx->fall_model = new FallMD();
    ctx->fall_model->detect(objects);
    return CVI_TDL_SUCCESS;
  }
  return ctx->fall_model->detect(objects);
}

// New Fall Detection
CVI_S32 CVI_TDL_Fall_Monitor(const cvitdl_handle_t handle, cvtdl_object_t *objects) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  FallDetMonitor *fall_monitor_model = ctx->fall_monitor_model;
  if (fall_monitor_model == nullptr) {
    LOGD("Init Fall Detection Model.\n");
    ctx->fall_monitor_model = new FallDetMonitor();
    ctx->fall_monitor_model->monitor(objects);
    return CVI_TDL_SUCCESS;
  }
  return ctx->fall_monitor_model->monitor(objects);
}

CVI_S32 CVI_TDL_Set_MaskOutlinePoint(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta) {
  int proto_h = obj_meta->mask_height;
  int proto_w = obj_meta->mask_width;
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    cv::Mat src(proto_h, proto_w, CV_8UC1, obj_meta->info[i].mask_properity->mask,
                proto_w * sizeof(uint8_t));

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // search for contours
    cv::findContours(src, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    // find the longest contour
    int longest_index = -1;
    size_t max_length = 0;
    for (size_t i = 0; i < contours.size(); i++) {
      if (contours[i].size() > max_length) {
        max_length = contours[i].size();
        longest_index = i;
      }
    }
    if (longest_index >= 0 && max_length >= 1) {
      float ratio_height = (proto_h / static_cast<float>(frame->stVFrame.u32Height));
      float ratio_width = (proto_w / static_cast<float>(frame->stVFrame.u32Width));
      int source_y_offset, source_x_offset;
      if (ratio_height > ratio_width) {
        source_x_offset = 0;
        source_y_offset = (proto_h - frame->stVFrame.u32Height * ratio_width) / 2;
      } else {
        source_x_offset = (proto_w - frame->stVFrame.u32Width * ratio_height) / 2;
        source_y_offset = 0;
      }
      int source_region_height = proto_h - 2 * source_y_offset;
      int source_region_width = proto_w - 2 * source_x_offset;
      // calculate scaling factor
      float height_scale =
          static_cast<float>(frame->stVFrame.u32Height) / static_cast<float>(source_region_height);
      float width_scale =
          static_cast<float>(frame->stVFrame.u32Width) / static_cast<float>(source_region_width);
      obj_meta->info[i].mask_properity->mask_point_size = max_length;
      obj_meta->info[i].mask_properity->mask_point =
          (float *)malloc(2 * max_length * sizeof(float));

      size_t j = 0;
      for (const auto &point : contours[longest_index]) {
        obj_meta->info[i].mask_properity->mask_point[2 * j] =
            (point.x - source_x_offset) * width_scale;
        obj_meta->info[i].mask_properity->mask_point[2 * j + 1] =
            (point.y - source_y_offset) * height_scale;
        j++;
      }
    }
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_Set_Fall_FPS(const cvitdl_handle_t handle, float fps) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  FallDetMonitor *fall_monitor_model = ctx->fall_monitor_model;
  if (fall_monitor_model == nullptr) {
    LOGD("Init Fall Detection Model.\n");
    ctx->fall_monitor_model = new FallDetMonitor();
    ctx->fall_monitor_model->set_fps(fps);
    return CVI_TDL_SUCCESS;
  }
  return ctx->fall_monitor_model->set_fps(fps);
}
#else
CVI_S32 CVI_TDL_CropImage(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *p_dst, cvtdl_bbox_t *bbox,
                          bool cvtRGB888) {
  return CVI_TDL_ERR_NOT_YET_INITIALIZED;
}

CVI_S32 CVI_TDL_CropImage_Exten(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *p_dst,
                                cvtdl_bbox_t *bbox, bool cvtRGB888, float exten_ratio,
                                float *offset_x, float *offset_y) {
  return CVI_TDL_ERR_NOT_YET_INITIALIZED;
}

CVI_S32 CVI_TDL_CropImage_Face(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *p_dst,
                               cvtdl_face_info_t *face_info, bool align, bool cvtRGB888) {
  return CVI_TDL_ERR_NOT_YET_INITIALIZED;
}
#endif

DEFINE_INF_FUNC_F1_P1(CVI_TDL_DMSLDet, DMSLandmarkerDet, CVI_TDL_SUPPORTED_MODEL_DMSLANDMARKERDET,
                      cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FLDet3, FaceLandmarkDet3, CVI_TDL_SUPPORTED_MODEL_LANDMARK_DET3,
                      cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FaceAttribute, FaceAttribute, CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE,
                      cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FaceAttribute_cls, FaceAttribute_cls,
                      CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P2(CVI_TDL_FaceAttributeOne, FaceAttribute,
                      CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE, cvtdl_face_t *, int)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FaceRecognition, FaceAttribute,
                      CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P2(CVI_TDL_FaceRecognitionOne, FaceAttribute,
                      CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, cvtdl_face_t *, int)

DEFINE_INF_FUNC_F1_P1(CVI_TDL_MaskClassification, MaskClassification,
                      CVI_TDL_SUPPORTED_MODEL_MASKCLASSIFICATION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_HandClassification, HandClassification,
                      CVI_TDL_SUPPORTED_MODEL_HANDCLASSIFICATION, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_HandKeypoint, HandKeypoint, CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT,
                      cvtdl_handpose21_meta_ts *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_HandKeypointClassification, HandKeypointClassification,
                      CVI_TDL_SUPPORTED_MODEL_HAND_KEYPOINT_CLASSIFICATION,
                      cvtdl_handpose21_meta_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_OSNet, OSNet, CVI_TDL_SUPPORTED_MODEL_OSNET, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P2(CVI_TDL_OSNetOne, OSNet, CVI_TDL_SUPPORTED_MODEL_OSNET, cvtdl_object_t *, int)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_SoundClassification, SoundClassification,
                      CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, int *)
DEFINE_INF_FUNC_F2_P1(CVI_TDL_DeeplabV3, Deeplabv3, CVI_TDL_SUPPORTED_MODEL_DEEPLABV3,
                      cvtdl_class_filter_t *)
DEFINE_INF_FUNC_F2_P1(CVI_TDL_MotionSegmentation, MotionSegmentation,
                      CVI_TDL_SUPPORTED_MODEL_MOTIONSEGMENTATION, cvtdl_seg_logits_t *)

DEFINE_INF_FUNC_F2_P1(CVI_TDL_Depth_Stereo, Stereo, CVI_TDL_SUPPORTED_MODEL_STEREO,
                      cvtdl_depth_logits_t *)

DEFINE_INF_FUNC_F1_P1(CVI_TDL_LicensePlateDetection, LicensePlateDetection,
                      CVI_TDL_SUPPORTED_MODEL_WPODNET, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FaceLandmarker, FaceLandmarker,
                      CVI_TDL_SUPPORTED_MODEL_FACELANDMARKER, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_FaceLandmarkerDet2, FaceLandmarkerDet2,
                      CVI_TDL_SUPPORTED_MODEL_FACELANDMARKERDET2, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_IncarObjectDetection, IncarObjectDetection,
                      CVI_TDL_SUPPORTED_MODEL_INCAROBJECTDETECTION, cvtdl_face_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_License_Plate_Detectionv2, YoloV8Pose,
                      CVI_TDL_SUPPORTED_MODEL_LP_DETECTION, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Image_Classification, ImageClassification,
                      CVI_TDL_SUPPORTED_MODEL_IMAGE_CLASSIFICATION, cvtdl_class_meta_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Raw_Image_Classification, RawImageClassification,
                      CVI_TDL_SUPPORTED_MODEL_RAW_IMAGE_CLASSIFICATION, cvtdl_class_meta_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Clip_Image_Feature, Clip_Image, CVI_TDL_SUPPORTED_MODEL_CLIP_IMAGE,
                      cvtdl_clip_feature *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Clip_Text_Feature, Clip_Text, CVI_TDL_SUPPORTED_MODEL_CLIP_TEXT,
                      cvtdl_clip_feature *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Lane_Det, BezierLaneNet, CVI_TDL_SUPPORTED_MODEL_LANE_DET,
                      cvtdl_lane_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_PolyLane_Det, Polylanenet, CVI_TDL_SUPPORTED_MODEL_POLYLANE,
                      cvtdl_lane_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_Super_Resolution, SuperResolution,
                      CVI_TDL_SUPPORTED_MODEL_SUPER_RESOLUTION, cvtdl_sr_feature *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_OCR_Recognition, OCRRecognition,
                      CVI_TDL_SUPPORTED_MODEL_OCR_RECOGNITION, cvtdl_object_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_LSTR_Det, LSTR, CVI_TDL_SUPPORTED_MODEL_LSTR, cvtdl_lane_t *)
DEFINE_INF_FUNC_F1_P1(CVI_TDL_IrLiveness, IrLiveness, CVI_TDL_SUPPORTED_MODEL_IRLIVENESS,
                      cvtdl_face_t *)
#ifdef CV186X
DEFINE_INF_FUNC_F1_P2(CVI_TDL_Isp_Image_Classification, IspImageClassification,
                      CVI_TDL_SUPPORTED_MODEL_ISP_IMAGE_CLASSIFICATION, cvtdl_class_meta_t *,
                      cvtdl_isp_meta_t *)
#endif

CVI_S32 CVI_TDL_Detection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                          CVI_TDL_SUPPORTED_MODEL_E model_index, cvtdl_object_t *obj) {
  std::set<CVI_TDL_SUPPORTED_MODEL_E> detect_set = {
      CVI_TDL_SUPPORTED_MODEL_YOLO,
      CVI_TDL_SUPPORTED_MODEL_YOLOV3,
      CVI_TDL_SUPPORTED_MODEL_YOLOV5,
      CVI_TDL_SUPPORTED_MODEL_YOLOV6,
      CVI_TDL_SUPPORTED_MODEL_YOLOV7,
      CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
      CVI_TDL_SUPPORTED_MODEL_YOLOV8_HARDHAT,
      CVI_TDL_SUPPORTED_MODEL_YOLOX,
      CVI_TDL_SUPPORTED_MODEL_PPYOLOE,
      CVI_TDL_SUPPORTED_MODEL_HAND_DETECTION,
      CVI_TDL_SUPPORTED_MODEL_PERSON_PETS_DETECTION,
      CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION,  // TODO:need class mapping
      CVI_TDL_SUPPORTED_MODEL_HAND_FACE_PERSON_DETECTION,
      CVI_TDL_SUPPORTED_MODEL_HEAD_PERSON_DETECTION,
      CVI_TDL_SUPPORTED_MODEL_THERMALPERSON,
      CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80,
      CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE,
      CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_VEHICLE,
      CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
      CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS,
      CVI_TDL_SUPPORTED_MODEL_YOLOV8_HARDHAT,
      CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION};
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (detect_set.find(model_index) == detect_set.end()) {
    LOGE("unknown object detection model index.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }

  DetectionBase *model = dynamic_cast<DetectionBase *>(getInferenceInstance(model_index, ctx));
  if (model == nullptr) {
    LOGE("No instance found\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (model->isInitialized()) {
    if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {
      return CVI_TDL_ERR_INIT_VPSS;
    } else {
      CVI_S32 ret = model->inference(frame, obj);
      if (ret != CVI_TDL_SUCCESS)
        return ret;
      else
        return model->after_inference();
    }
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(model_index));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_Set_Outputlayer_Names(const cvitdl_handle_t handle,
                                      CVI_TDL_SUPPORTED_MODEL_E model_index,
                                      const char **output_names, size_t size) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DetectionBase *model = dynamic_cast<DetectionBase *>(getInferenceInstance(model_index, ctx));
  if (model != nullptr) {
    std::vector<std::string> names;
    names.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      names.emplace_back(std::string(output_names[i]));
    }
    model->set_out_names(names);
  } else {
    LOGE("No instance found\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_FaceDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                              CVI_TDL_SUPPORTED_MODEL_E model_index, cvtdl_face_t *face_meta) {
  std::set<CVI_TDL_SUPPORTED_MODEL_E> face_detect_set = {
      CVI_TDL_SUPPORTED_MODEL_RETINAFACE, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE,
      CVI_TDL_SUPPORTED_MODEL_RETINAFACE_IR, CVI_TDL_SUPPORTED_MODEL_THERMALFACE,
      CVI_TDL_SUPPORTED_MODEL_FACEMASKDETECTION};
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (face_detect_set.find(model_index) == face_detect_set.end()) {
    LOGE("unknown face detection model index.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  FaceDetectionBase *model =
      dynamic_cast<FaceDetectionBase *>(getInferenceInstance(model_index, ctx));
  if (model == nullptr) {
    LOGE("No instance found\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (model->isInitialized()) {
    if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {
      return CVI_TDL_ERR_INIT_VPSS;
    } else {
      CVI_S32 ret = model->inference(frame, face_meta);
      if (ret != CVI_TDL_SUCCESS)
        return ret;
      else
        return model->after_inference();
    }
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(model_index));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_PoseDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                              CVI_TDL_SUPPORTED_MODEL_E model_index, cvtdl_object_t *obj_meta) {
  std::set<CVI_TDL_SUPPORTED_MODEL_E> pose_detect_set = {CVI_TDL_SUPPORTED_MODEL_HRNET_POSE,
                                                         CVI_TDL_SUPPORTED_MODEL_SIMCC_POSE,
                                                         CVI_TDL_SUPPORTED_MODEL_YOLOV8POSE};
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (pose_detect_set.find(model_index) == pose_detect_set.end()) {
    LOGE("unknown pose detection model index.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  PoseDetectionBase *model =
      dynamic_cast<PoseDetectionBase *>(getInferenceInstance(model_index, ctx));
  if (model == nullptr) {
    LOGE("No instance found\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (model->isInitialized()) {
    if (initVPSSIfNeeded(ctx, model_index) != CVI_SUCCESS) {
      return CVI_TDL_ERR_INIT_VPSS;
    } else {
      CVI_S32 ret = model->inference(frame, obj_meta);
      if (ret != CVI_TDL_SUCCESS)
        return ret;
      else
        return model->after_inference();
    }
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(model_index));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_LicensePlateRecognition(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                        CVI_TDL_SUPPORTED_MODEL_E model_id, cvtdl_object_t *obj) {
  std::set<CVI_TDL_SUPPORTED_MODEL_E> detect_set = {CVI_TDL_SUPPORTED_MODEL_LPRNET_TW,
                                                    CVI_TDL_SUPPORTED_MODEL_LPRNET_CN,
                                                    CVI_TDL_SUPPORTED_MODEL_LP_RECONGNITION};
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (detect_set.find(model_id) == detect_set.end()) {
    LOGE("unknown license plate recognition model index.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  LicensePlateRecognitionBase *sc_model =
      dynamic_cast<LicensePlateRecognitionBase *>(getInferenceInstance(model_id, ctx));
  if (sc_model == nullptr) {
    LOGE("No instance found for LicensePlateRecognition.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (sc_model->isInitialized()) {
    return sc_model->inference(frame, obj);
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(model_id));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

// Tracker

CVI_S32 CVI_TDL_DeepSORT_Init(const cvitdl_handle_t handle, bool use_specific_counter) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGI("Init DeepSORT.\n");
    ctx->ds_tracker = new DeepSORT(use_specific_counter);
  } else {
    delete ds_tracker;
    LOGI("Re-init DeepSORT.\n");
    ctx->ds_tracker = new DeepSORT(use_specific_counter);
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_GetDefaultConfig(cvtdl_deepsort_config_t *ds_conf) {
  cvtdl_deepsort_config_t default_conf = DeepSORT::get_DefaultConfig();
  memcpy(ds_conf, &default_conf, sizeof(cvtdl_deepsort_config_t));

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_GetConfig(const cvitdl_handle_t handle, cvtdl_deepsort_config_t *ds_conf,
                                   int cvitdl_obj_type) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ds_tracker->getConfig(ds_conf, cvitdl_obj_type);
}

CVI_S32 CVI_TDL_DeepSORT_SetConfig(const cvitdl_handle_t handle, cvtdl_deepsort_config_t *ds_conf,
                                   int cvitdl_obj_type, bool show_config) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ds_tracker->setConfig(ds_conf, cvitdl_obj_type, show_config);
}

CVI_S32 CVI_TDL_DeepSORT_CleanCounter(const cvitdl_handle_t handle) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  ds_tracker->cleanCounter();

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_Head_FusePed(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                      cvtdl_tracker_t *tracker_t, bool use_reid,
                                      cvtdl_object_t *head, cvtdl_object_t *ped,
                                      const cvtdl_counting_line_t *counting_line_t,
                                      const randomRect *rect) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  ds_tracker->set_image_size(obj->width, obj->height);
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  ctx->ds_tracker->track_headfuse(obj, tracker_t, use_reid, head, ped, counting_line_t, rect);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_Obj(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                             cvtdl_tracker_t *tracker, bool use_reid) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ctx->ds_tracker->track(obj, tracker, use_reid);
}

CVI_S32 CVI_TDL_DeepSORT_Byte(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                              cvtdl_tracker_t *tracker, bool use_reid) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ctx->ds_tracker->byte_track(obj, tracker, use_reid);
}
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_Obj_Cross(const cvitdl_handle_t handle, cvtdl_object_t *obj,
                                              cvtdl_tracker_t *tracker, bool use_reid,
                                              const cvtdl_counting_line_t *cross_line_t,
                                              const randomRect *rect) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ctx->ds_tracker->track_cross(obj, tracker, use_reid, cross_line_t, rect);
}

CVI_S32 CVI_TDL_DeepSORT_Face(const cvitdl_handle_t handle, cvtdl_face_t *face,
                              cvtdl_tracker_t *tracker) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  return ctx->ds_tracker->track(face, tracker);
}
DLL_EXPORT CVI_S32 CVI_TDL_DeepSORT_FaceFusePed(const cvitdl_handle_t handle, cvtdl_face_t *face,
                                                cvtdl_object_t *obj, cvtdl_tracker_t *tracker_t) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  ds_tracker->set_image_size(face->width, face->height);
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  ctx->ds_tracker->track_fuse(obj, face, tracker_t);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_Set_Timestamp(const cvitdl_handle_t handle, uint32_t ts) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  ctx->ds_tracker->set_timestamp(ts);
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_UpdateOutNum(const cvitdl_handle_t handle, cvtdl_tracker_t *tracker_t) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;

  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  ctx->ds_tracker->update_out_num(tracker_t);
  return CVI_SUCCESS;
}
CVI_S32 CVI_TDL_DeepSORT_DebugInfo_1(const cvitdl_handle_t handle, char *debug_info) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_TDL_FAILURE;
  }
  std::string str_info;
  ctx->ds_tracker->get_TrackersInfo_UnmatchedLastTime(str_info);
  strncpy(debug_info, str_info.c_str(), 8192);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_DeepSORT_GetTracker_Inactive(const cvitdl_handle_t handle,
                                             cvtdl_tracker_t *tracker) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  return ctx->ds_tracker->get_trackers_inactive(tracker);
}

CVI_S32 CVI_TDL_FaceHeadIouScore(const cvitdl_handle_t handle, cvtdl_face_t *faces,
                                 cvtdl_face_t *heads) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DeepSORT *ds_tracker = ctx->ds_tracker;
  if (ds_tracker == nullptr) {
    LOGE("Please initialize DeepSORT first.\n");
    return CVI_FAILURE;
  }
  return ctx->ds_tracker->face_head_iou_score(faces, heads);
}

// Others
CVI_S32 CVI_TDL_TamperDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                float *moving_score) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  TamperDetectorMD *td_model = ctx->td_model;
  if (td_model == nullptr) {
    LOGD("Init Tamper Detection Model.\n");
    createIVEHandleIfNeeded(ctx);
    ctx->td_model = new TamperDetectorMD(ctx->ive_handle, frame, (float)0.05, (int)10);

    *moving_score = -1.0;
    return CVI_TDL_SUCCESS;
  }
  return ctx->td_model->detect(frame, moving_score);
}

CVI_S32 CVI_TDL_Set_MotionDetection_Background(const cvitdl_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  MotionDetection *md_model = ctx->md_model;
  if (md_model == nullptr) {
    LOGD("Init Motion Detection.\n");
    if (createIVEHandleIfNeeded(ctx) == CVI_TDL_FAILURE) {
      return CVI_TDL_FAILURE;
    }
    ctx->md_model = new MotionDetection(ctx->ive_handle);
    return ctx->md_model->init(frame);
  }
  return ctx->md_model->update_background(frame);
}

CVI_S32 CVI_TDL_Set_MotionDetection_ROI(const cvitdl_handle_t handle, MDROI_t *roi_s) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  MotionDetection *md_model = ctx->md_model;
  if (md_model == nullptr) {
    LOGE("MD has not been inited\n");
    return CVI_TDL_FAILURE;
  }
  return ctx->md_model->set_roi(roi_s);
}

CVI_S32 CVI_TDL_MotionDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                cvtdl_object_t *obj_meta, uint8_t threshold, double min_area) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  MotionDetection *md_model = ctx->md_model;
  if (md_model == nullptr) {
    LOGE(
        "Failed to do motion detection! Please invoke CVI_TDL_Set_MotionDetection_Background to "
        "set "
        "background image first.\n");
    return CVI_TDL_FAILURE;
  }
  std::vector<std::vector<float>> boxes;
  CVI_S32 ret = ctx->md_model->detect(frame, boxes, threshold, min_area);
  memset(obj_meta, 0, sizeof(cvtdl_object_t));
  size_t num_boxes = boxes.size();
  if (num_boxes > 0) {
    CVI_TDL_MemAllocInit(num_boxes, obj_meta);
    obj_meta->height = frame->stVFrame.u32Height;
    obj_meta->width = frame->stVFrame.u32Width;
    obj_meta->rescale_type = RESCALE_RB;
    memset(obj_meta->info, 0, sizeof(cvtdl_object_info_t) * num_boxes);
    for (uint32_t i = 0; i < (uint32_t)num_boxes; ++i) {
      obj_meta->info[i].bbox.x1 = boxes[i][0];
      obj_meta->info[i].bbox.y1 = boxes[i][1];
      obj_meta->info[i].bbox.x2 = boxes[i][2];
      obj_meta->info[i].bbox.y2 = boxes[i][3];
      obj_meta->info[i].bbox.score = 0;
      obj_meta->info[i].classes = -1;
      memset(obj_meta->info[i].name, 0, sizeof(obj_meta->info[i].name));
    }
  }
  return ret;
}

CVI_S32 CVI_TDL_FaceFeatureExtract(const cvitdl_handle_t handle, const uint8_t *p_rgb_pack,
                                   int width, int height, int stride,
                                   cvtdl_face_info_t *p_face_info) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  FaceAttribute *inst = dynamic_cast<FaceAttribute *>(
      getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION, ctx));
  if (inst == nullptr) {
    LOGE("No instance found for FaceAttribute\n");
    return CVI_FAILURE;
  }
  if (inst->isInitialized()) {
    if (initVPSSIfNeeded(ctx, CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION) != CVI_SUCCESS) {
      return CVI_TDL_ERR_INIT_VPSS;
    }
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  return inst->extract_face_feature(p_rgb_pack, width, height, stride, p_face_info);
}

CVI_S32 CVI_TDL_GetSoundClassificationClassesNum(const cvitdl_handle_t handle) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  SoundClassification *sc_model = dynamic_cast<SoundClassification *>(
      getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, ctx));
  if (sc_model == nullptr) {
    LOGE("No instance found for SoundClassification.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (sc_model->isInitialized()) {
    return sc_model->getClassesNum();
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_SetSoundClassificationThreshold(const cvitdl_handle_t handle, const float th) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  SoundClassification *sc_model = dynamic_cast<SoundClassification *>(
      getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, ctx));
  if (sc_model == nullptr) {
    LOGE("No instance found for SoundClassification.\n");
    return CVI_TDL_ERR_OPEN_MODEL;
  }
  if (sc_model->isInitialized()) {
    return sc_model->setThreshold(th);
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_Change_Img(const cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model_type,
                           VIDEO_FRAME_INFO_S *frame, VIDEO_FRAME_INFO_S **dst_frame,
                           PIXEL_FORMAT_E enDstFormat) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }

  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();
  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }

  VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
  memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
  modelt.instance->vpssChangeImage(frame, f, frame->stVFrame.u32Width, frame->stVFrame.u32Height,
                                   enDstFormat);
  *dst_frame = f;
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_Delete_Img(const cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model_type,
                           VIDEO_FRAME_INFO_S *p_f) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();

  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  p_vpss_inst->releaseFrame(p_f, 0);
  delete p_f;
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_CropImage_With_VPSS(const cvitdl_handle_t handle,
                                    CVI_TDL_SUPPORTED_MODEL_E model_type, VIDEO_FRAME_INFO_S *frame,
                                    const cvtdl_bbox_t *p_crop_box, cvtdl_image_t *p_dst) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();

  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  if (p_dst->pix_format != PIXEL_FORMAT_RGB_888) {
    LOGE("only PIXEL_FORMAT_RGB_888 format supported,got:%d\n", (int)p_dst->pix_format);
    return CVI_FAILURE;
  }

  VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
  memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
  modelt.instance->vpssCropImage(frame, f, *p_crop_box, p_dst->width, p_dst->height,
                                 p_dst->pix_format);
  mmap_video_frame(f);

  int ret = CVI_SUCCESS;
  for (int i = 0; i < 3; i++) {
    if ((p_dst->pix[i] == 0 && f->stVFrame.pu8VirAddr[i] != 0) ||
        (p_dst->pix[i] != 0 && f->stVFrame.pu8VirAddr[i] == 0)) {
      LOGE("error,plane:%d,dst_addr:%p,video_frame_addr:%p\n", i, p_dst->pix[i],
           f->stVFrame.pu8VirAddr[i]);
      ret = CVI_FAILURE;
      break;
    }
    if (f->stVFrame.u32Length[i] > p_dst->length[i]) {
      LOGE("size overflow,plane:%d,dst_len:%u,video_frame_len:%u\n", i, p_dst->length[i],
           f->stVFrame.u32Length[i]);
      ret = CVI_FAILURE;
      break;
    }
    memcpy(p_dst->pix[i], f->stVFrame.pu8VirAddr[i], f->stVFrame.u32Length[i]);
  }
  unmap_video_frame(f);
  if (f->stVFrame.u64PhyAddr[0] != 0) {
    p_vpss_inst->releaseFrame(f, 0);
  }
  delete f;
  return ret;
}
CVI_S32 CVI_TDL_CropResizeImage(const cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E model_type,
                                VIDEO_FRAME_INFO_S *frame, const cvtdl_bbox_t *p_crop_box,
                                int dst_width, int dst_height, PIXEL_FORMAT_E enDstFormat,
                                VIDEO_FRAME_INFO_S **p_dst_img) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();

  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
  memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
  int ret =
      modelt.instance->vpssCropImage(frame, f, *p_crop_box, dst_width, dst_height, enDstFormat);
  *p_dst_img = f;
  return ret;
}
CVI_S32 CVI_TDL_Copy_VideoFrameToImage(VIDEO_FRAME_INFO_S *f, cvtdl_image_t *p_dst) {
  mmap_video_frame(f);

  int ret = CVI_SUCCESS;
  for (int i = 0; i < 3; i++) {
    if ((p_dst->pix[i] == 0 && f->stVFrame.pu8VirAddr[i] != 0) ||
        (p_dst->pix[i] != 0 && f->stVFrame.pu8VirAddr[i] == 0)) {
      LOGE("error,plane:%d,dst_addr:%p,video_frame_addr:%p\n", i, p_dst->pix[i],
           f->stVFrame.pu8VirAddr[i]);
      ret = CVI_FAILURE;
      break;
    }
    if (f->stVFrame.u32Length[i] > p_dst->length[i]) {
      LOGE("size overflow,plane:%d,dst_len:%u,video_frame_len:%u\n", i, p_dst->length[i],
           f->stVFrame.u32Length[i]);
      ret = CVI_FAILURE;
      break;
    }
    memcpy(p_dst->pix[i], f->stVFrame.pu8VirAddr[i], f->stVFrame.u32Length[i]);
  }
  unmap_video_frame(f);
  return ret;
}
CVI_S32 CVI_TDL_Resize_VideoFrame(const cvitdl_handle_t handle,
                                  CVI_TDL_SUPPORTED_MODEL_E model_type, VIDEO_FRAME_INFO_S *frame,
                                  const int dst_w, const int dst_h, PIXEL_FORMAT_E dst_format,
                                  VIDEO_FRAME_INFO_S **dst_frame) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();

  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  VIDEO_FRAME_INFO_S *f = new VIDEO_FRAME_INFO_S;
  memset(f, 0, sizeof(VIDEO_FRAME_INFO_S));
  cvtdl_bbox_t bbox;
  bbox.x1 = 0;
  bbox.y1 = 0;
  bbox.x2 = frame->stVFrame.u32Width;
  bbox.y2 = frame->stVFrame.u32Height;
  VPSS_SCALE_COEF_E scale = VPSS_SCALE_COEF_NEAREST;
  CVI_S32 ret = modelt.instance->vpssCropImage(frame, f, bbox, dst_w, dst_h, dst_format, scale);
  *dst_frame = f;
  return ret;
}
DLL_EXPORT CVI_S32 CVI_TDL_Release_VideoFrame(const cvitdl_handle_t handle,
                                              CVI_TDL_SUPPORTED_MODEL_E model_type,
                                              VIDEO_FRAME_INFO_S *frame, bool del_frame) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl_model_t modelt = ctx->model_cont[model_type];
  if (modelt.instance == nullptr) {
    LOGE("model not initialized:%d\n", (int)model_type);
    return CVI_FAILURE;
  }
  VpssEngine *p_vpss_inst = modelt.instance->get_vpss_instance();

  if (p_vpss_inst == nullptr) {
    LOGE("vpssmodel not initialized:%d\n", (int)model_type);
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }

  if (frame->stVFrame.u64PhyAddr[0] != 0) {
    p_vpss_inst->releaseFrame(frame, 0);
  }
  if (del_frame) {
    delete frame;
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_PersonVehicle_Detection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                        cvtdl_object_t *obj_meta) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  YoloV8Detection *yolo_model = dynamic_cast<YoloV8Detection *>(
      getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION, ctx));
  if (yolo_model == nullptr) {
    LOGE("No instance found for CVI_TDL_PersonVehicle_Detection.\n");
    return CVI_FAILURE;
  }
  LOGI("got yolov8 instance\n");
  if (yolo_model->isInitialized()) {
    if (initVPSSIfNeeded(ctx, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION) != CVI_SUCCESS) {
      return CVI_TDL_ERR_INIT_VPSS;
    } else {
      int ret = yolo_model->inference(frame, obj_meta);
      ret = yolo_model->after_inference();

      if (ret == CVI_TDL_SUCCESS) {
        for (uint32_t i = 0; i < obj_meta->size; i++) {
          if (obj_meta->info[i].classes == 4) {
            obj_meta->info[i].classes = 0;  // person
          } else if (obj_meta->info[i].classes == 0 || obj_meta->info[i].classes == 1 ||
                     obj_meta->info[i].classes == 2) {
            obj_meta->info[i].classes = 1;  // motor vehicle
          } else {
            obj_meta->info[i].classes = 2;  // non-motor vehicle
          }
        }
      }
      return ret;
    }
  } else {
    LOGE("Model (%s)is not yet opened! Please call CVI_TDL_OpenModel to initialize model\n",
         CVI_TDL_GetModelName(CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION));
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
}

CVI_S32 CVI_TDL_Set_Yolov5_ROI(const cvitdl_handle_t handle, Point_t roi_s) {
  printf("enter CVI_TDL_Set_Yolov5_ROI...\n");
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  Yolov5 *yolov5_model =
      dynamic_cast<Yolov5 *>(getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_YOLOV5, ctx));
  if (yolov5_model == nullptr) {
    LOGE("yolov5_model has not been inited\n");
    return CVI_TDL_FAILURE;
  }
  return yolov5_model->set_roi(roi_s);
}

InputPreParam CVI_TDL_GetPreParam(const cvitdl_handle_t handle,
                                  const CVI_TDL_SUPPORTED_MODEL_E model_index) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl::Core *instance = getInferenceInstance(model_index, ctx);
  return instance->get_preparam();
}

CVI_S32 CVI_TDL_SetPreParam(const cvitdl_handle_t handle,
                            const CVI_TDL_SUPPORTED_MODEL_E model_index, InputPreParam pre_param) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  cvitdl::Core *instance = getInferenceInstance(model_index, ctx);
  instance->set_preparam(pre_param);
  return CVI_SUCCESS;
}

cvtdl_det_algo_param_t CVI_TDL_GetDetectionAlgoParam(const cvitdl_handle_t handle,
                                                     const CVI_TDL_SUPPORTED_MODEL_E model_index) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DetectionBase *model = dynamic_cast<DetectionBase *>(getInferenceInstance(model_index, ctx));
  return model->get_algparam();
}

CVI_S32 CVI_TDL_SetDetectionAlgoParam(const cvitdl_handle_t handle,
                                      const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                      cvtdl_det_algo_param_t alg_param) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  DetectionBase *model = dynamic_cast<DetectionBase *>(getInferenceInstance(model_index, ctx));
  model->set_algparam(alg_param);
  return CVI_SUCCESS;
}

cvitdl_sound_param CVI_TDL_GetSoundClassificationParam(
    const cvitdl_handle_t handle, const CVI_TDL_SUPPORTED_MODEL_E model_index) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  if (model_index == CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION) {
    SoundClassification *sc_v2_model =
        dynamic_cast<SoundClassification *>(getInferenceInstance(model_index, ctx));
    return sc_v2_model->get_algparam();
  }
  LOGE("not supported model index\n");
  return cvitdl_sound_param();
}

CVI_S32 CVI_TDL_SetSoundClassificationParam(const cvitdl_handle_t handle,
                                            const CVI_TDL_SUPPORTED_MODEL_E model_index,
                                            cvitdl_sound_param audio_param) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);

  if (model_index == CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION) {
    SoundClassification *sc_v2_model =
        dynamic_cast<SoundClassification *>(getInferenceInstance(model_index, ctx));
    sc_v2_model->set_algparam(audio_param);
    return CVI_SUCCESS;
  }
  LOGE("not supported model index\n");
  return CVI_FAILURE;
}

CVI_S32 CVI_TDL_SoundClassificationPack(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                        int pack_idx, int pack_len, int *index) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);
  SoundClassification *sc_model = dynamic_cast<SoundClassification *>(
      getInferenceInstance(CVI_TDL_SUPPORTED_MODEL_SOUNDCLASSIFICATION, ctx));
  if (sc_model == nullptr) {
    LOGE("No instance found for SoundClassification.\n");
    return CVI_FAILURE;
  }
  if (!sc_model->isInitialized()) {
    LOGE("SoundClassification has not been initialized\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  int ret = sc_model->inference_pack(frame, pack_idx, pack_len, index);
  return ret;
}

CVI_S32 CVI_TDL_Set_Polylanenet_Lower(const cvitdl_handle_t handle,
                                      const CVI_TDL_SUPPORTED_MODEL_E model_index, float th) {
  cvitdl_context_t *ctx = static_cast<cvitdl_context_t *>(handle);

  if (model_index == CVI_TDL_SUPPORTED_MODEL_POLYLANE) {
    Polylanenet *polylane_model =
        dynamic_cast<Polylanenet *>(getInferenceInstance(model_index, ctx));
    polylane_model->set_lower(th);
    return CVI_SUCCESS;
  }
  LOGE("not supported model index\n");
  return CVI_FAILURE;
}

CVI_S32 CVI_TDL_Set_TextPreprocess(const char *encoderFile, const char *bpeFile,
                                   const char *textFile, int32_t **tokens, int numSentences) {
  std::vector<std::vector<int32_t>> tokens_cpp(numSentences);
  // call token_bpe function
  int result =
      token_bpe(std::string(encoderFile), std::string(bpeFile), std::string(textFile), tokens_cpp);
  // calculate the total number of elements
  for (int i = 0; i < numSentences; i++) {
    // tokens[i] = new int32_t[tokens_cpp[i].size()];
    tokens[i] = (int32_t *)malloc(tokens_cpp[i].size() * sizeof(int32_t));
    memcpy(tokens[i], tokens_cpp[i].data(), tokens_cpp[i].size() * sizeof(int32_t));
  }

  if (result == 0) {
    return CVI_SUCCESS;
  }
  LOGE("Tokenization error\n");
  return CVI_FAILURE;
}

CVI_S32 CVI_TDL_Set_ClipPostprocess(float **text_features, int text_features_num,
                                    float **image_features, int image_features_num, float **probs) {
  Eigen::MatrixXf text_features_eigen(text_features_num, 512);
  for (int i = 0; i < text_features_num; ++i) {
    for (int j = 0; j < 512; ++j) {
      text_features_eigen(i, j) = text_features[i][j];
    }
  }
  Eigen::MatrixXf image_features_eigen(image_features_num, 512);
  for (int i = 0; i < image_features_num; ++i) {
    for (int j = 0; j < 512; ++j) {
      image_features_eigen(i, j) = image_features[i][j];
    }
  }

  Eigen::MatrixXf result_eigen;
  // using clip_postprocess which can be found in utils/clip_postpostprocess.cpp
  int res = clip_postprocess(text_features_eigen, image_features_eigen, result_eigen);

  // providing image classification functionality.
  // using softmax after mutil 100 scale
  for (int i = 0; i < result_eigen.rows(); ++i) {
    float sum = 0.0;
    float maxVal = result_eigen.row(i).maxCoeff();
    Eigen::MatrixXf expInput = (result_eigen.row(i).array() - maxVal).exp();
    sum = expInput.sum();
    result_eigen.row(i) = expInput / sum;
  }

  for (int i = 0; i < result_eigen.rows(); i++) {
    float max_score = 0;
    for (int j = 0; j < result_eigen.cols(); j++) {
      probs[i][j] = result_eigen(i, j);
    }
  }

  if (res == 0) {
    return CVI_SUCCESS;
  }

  LOGE("Tokenization error\n");
  return CVI_FAILURE;
}