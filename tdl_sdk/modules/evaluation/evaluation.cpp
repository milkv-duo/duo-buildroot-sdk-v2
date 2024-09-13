#include "cvi_tdl_evaluation.h"

#include "cityscapes/cityscapes.hpp"
#include "coco/coco.hpp"
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "cvi_lpdr/cvi_lpdr.hpp"
#include "lfw/lfw.hpp"
#include "market1501/market1501.hpp"
#include "wflw/wflw.hpp"
#include "wider_face/wider_face.hpp"

typedef struct {
  cvitdl::evaluation::cityscapesEval *cityscapes_eval = nullptr;
  cvitdl::evaluation::CocoEval *coco_eval = nullptr;
  cvitdl::evaluation::market1501Eval *market1501_eval = nullptr;
  cvitdl::evaluation::lfwEval *lfw_eval = nullptr;
  cvitdl::evaluation::widerFaceEval *widerface_eval = nullptr;
  cvitdl::evaluation::wflwEval *wflw_eval = nullptr;
  cvitdl::evaluation::LPDREval *lpdr_eval = nullptr;
} cvitdl_eval_context_t;

CVI_S32 CVI_TDL_Eval_CreateHandle(cvitdl_eval_handle_t *handle) {
  cvitdl_eval_context_t *ctx = new cvitdl_eval_context_t;
  *handle = ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_DestroyHandle(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  delete ctx->coco_eval;
  delete ctx->lpdr_eval;
  delete ctx;
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * Cityscapes evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_CityscapesInit(cvitdl_eval_handle_t handle, const char *image_dir,
                                    const char *output_dir) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->cityscapes_eval == nullptr) {
    ctx->cityscapes_eval = new cvitdl::evaluation::cityscapesEval(image_dir, output_dir);
  }

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CityscapesGetImage(cvitdl_eval_handle_t handle, const uint32_t index,
                                        char **fileName) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->cityscapes_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr;
  ctx->cityscapes_eval->getImage(index, filestr);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *fileName = (char *)malloc(stringlength);
  strncpy(*fileName, filestr.c_str(), stringlength);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CityscapesGetImageNum(cvitdl_eval_handle_t handle, uint32_t *num) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->cityscapes_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->cityscapes_eval->getImageNum(num);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CityscapesWriteResult(cvitdl_eval_handle_t handle,
                                           VIDEO_FRAME_INFO_S *label_frame, const int index) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->cityscapes_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->cityscapes_eval->writeResult(label_frame, index);
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * Coco evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_CocoInit(cvitdl_eval_handle_t handle, const char *pathPrefix,
                              const char *jsonPath, uint32_t *imageNum) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->coco_eval == nullptr) {
    ctx->coco_eval = new cvitdl::evaluation::CocoEval(pathPrefix, jsonPath);
  } else {
    ctx->coco_eval->getEvalData(pathPrefix, jsonPath);
  }
  *imageNum = ctx->coco_eval->getTotalImage();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CocoGetImageIdPair(cvitdl_eval_handle_t handle, const uint32_t index,
                                        char **filepath, int *id) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->coco_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr;
  ctx->coco_eval->getImageIdPair(index, &filestr, id);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *filepath = (char *)malloc(stringlength);
  strncpy(*filepath, filestr.c_str(), stringlength);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CocoInsertObject(cvitdl_eval_handle_t handle, const int id,
                                      cvtdl_object_t *obj) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->coco_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->coco_eval->insertObjectData(id, obj);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CocoStartEval(cvitdl_eval_handle_t handle, const char *filepath) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->coco_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->coco_eval->start_eval(filepath);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_CocoEndEval(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->coco_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->coco_eval->end_eval();
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * LFW evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_LfwInit(cvitdl_eval_handle_t handle, const char *filepath,
                             bool label_pos_first, uint32_t *imageNum) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    ctx->lfw_eval = new cvitdl::evaluation::lfwEval();
  }

  if (ctx->lfw_eval->getEvalData(filepath, label_pos_first) != CVI_TDL_SUCCESS) {
    return CVI_TDL_FAILURE;
  }

  *imageNum = ctx->lfw_eval->getTotalImage();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwGetImageLabelPair(cvitdl_eval_handle_t handle, const uint32_t index,
                                          char **filepath, char **filepath2, int *label) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr, filestr2;
  ctx->lfw_eval->getImageLabelPair(index, &filestr, &filestr2, label);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *filepath = (char *)malloc(stringlength);
  strncpy(*filepath, filestr.c_str(), stringlength);
  stringlength = strlen(filestr2.c_str()) + 1;
  *filepath2 = (char *)malloc(stringlength);
  strncpy(*filepath2, filestr2.c_str(), stringlength);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwInsertFace(cvitdl_eval_handle_t handle, const int index, const int label,
                                   const cvtdl_face_t *face1, const cvtdl_face_t *face2) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->lfw_eval->insertFaceData(index, label, face1, face2);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwInsertLabelScore(cvitdl_eval_handle_t handle, const int index,
                                         const int label, const float score) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->lfw_eval->insertLabelScore(index, label, score);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwSave2File(cvitdl_eval_handle_t handle, const char *filepath) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->lfw_eval->saveEval2File(filepath);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwClearInput(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->lfw_eval->resetData();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LfwClearEvalData(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lfw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->lfw_eval->resetEvalData();
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * Wider Face evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_WiderFaceInit(cvitdl_eval_handle_t handle, const char *datasetDir,
                                   const char *resultDir, uint32_t *imageNum) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->widerface_eval == nullptr) {
    ctx->widerface_eval = new cvitdl::evaluation::widerFaceEval(datasetDir, resultDir);
  } else {
    ctx->widerface_eval->getEvalData(datasetDir, resultDir);
  }
  *imageNum = ctx->widerface_eval->getTotalImage();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WiderFaceGetImagePath(cvitdl_eval_handle_t handle, const uint32_t index,
                                           char **filepath) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->widerface_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr;
  ctx->widerface_eval->getImageFilePath(index, &filestr);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *filepath = (char *)malloc(stringlength);
  strncpy(*filepath, filestr.c_str(), stringlength);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WiderFaceResultSave2File(cvitdl_eval_handle_t handle, const int index,
                                              const VIDEO_FRAME_INFO_S *frame,
                                              const cvtdl_face_t *face) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->widerface_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->widerface_eval->saveFaceData(index, frame, face);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WiderFaceClearInput(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->widerface_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->widerface_eval->resetData();
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * Market1501 evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_Market1501Init(cvitdl_eval_handle_t handle, const char *filepath) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->market1501_eval == nullptr) {
    ctx->market1501_eval = new cvitdl::evaluation::market1501Eval(filepath);
  } else {
    ctx->market1501_eval->getEvalData(filepath);
  }

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_Market1501GetImageNum(cvitdl_eval_handle_t handle, bool is_query,
                                           uint32_t *num) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->market1501_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }

  *num = ctx->market1501_eval->getImageNum(is_query);
  printf("query_dir: %d\n", *num);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_Market1501GetPathIdPair(cvitdl_eval_handle_t handle, const uint32_t index,
                                             bool is_query, char **filepath, int *cam_id,
                                             int *pid) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->market1501_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }

  std::string filestr;
  ctx->market1501_eval->getPathIdPair(index, is_query, &filestr, cam_id, pid);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *filepath = (char *)malloc(stringlength);
  strncpy(*filepath, filestr.c_str(), stringlength);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_Market1501InsertFeature(cvitdl_eval_handle_t handle, const int index,
                                             bool is_query, const cvtdl_feature_t *feature) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->market1501_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->market1501_eval->insertFeature(index, is_query, feature);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_Market1501EvalCMC(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->market1501_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->market1501_eval->evalCMC();
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * WLFW evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_WflwInit(cvitdl_eval_handle_t handle, const char *filepath,
                              uint32_t *imageNum) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->wflw_eval == nullptr) {
    ctx->wflw_eval = new cvitdl::evaluation::wflwEval(filepath);
  } else {
    ctx->wflw_eval->getEvalData(filepath);
  }
  *imageNum = ctx->wflw_eval->getTotalImage();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WflwGetImage(cvitdl_eval_handle_t handle, const uint32_t index,
                                  char **fileName) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->wflw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr;
  ctx->wflw_eval->getImage(index, &filestr);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *fileName = (char *)malloc(stringlength);
  strncpy(*fileName, filestr.c_str(), stringlength);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WflwInsertPoints(cvitdl_eval_handle_t handle, const int index,
                                      const cvtdl_pts_t points, const int width, const int height) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->wflw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->wflw_eval->insertPoints(index, points, width, height);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_WflwDistance(cvitdl_eval_handle_t handle) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->wflw_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  ctx->wflw_eval->distance();
  return CVI_TDL_SUCCESS;
}

/****************************************************************
 * LPDR evaluation functions
 **/
CVI_S32 CVI_TDL_Eval_LPDRInit(cvitdl_eval_handle_t handle, const char *pathPrefix,
                              const char *jsonPath, uint32_t *imageNum) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lpdr_eval == nullptr) {
    ctx->lpdr_eval = new cvitdl::evaluation::LPDREval(pathPrefix, jsonPath);
  } else {
    ctx->lpdr_eval->getEvalData(pathPrefix, jsonPath);
    return CVI_TDL_FAILURE;
  }
  *imageNum = ctx->lpdr_eval->getTotalImage();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Eval_LPDRGetImageIdPair(cvitdl_eval_handle_t handle, const uint32_t index,
                                        char **filepath, int *id) {
  cvitdl_eval_context_t *ctx = static_cast<cvitdl_eval_context_t *>(handle);
  if (ctx->lpdr_eval == nullptr) {
    return CVI_TDL_FAILURE;
  }
  std::string filestr;
  ctx->lpdr_eval->getImageIdPair(index, &filestr, id);
  auto stringlength = strlen(filestr.c_str()) + 1;
  *filepath = (char *)malloc(stringlength);
  strncpy(*filepath, filestr.c_str(), stringlength);
  return CVI_TDL_SUCCESS;
}
