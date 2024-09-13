#ifndef _CVI_TDL_EVALUATION_H_
#define _CVI_TDL_EVALUATION_H_

#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

#include <cvi_sys.h>

typedef void *cvitdl_eval_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CreateHandle(cvitdl_eval_handle_t *handle);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_DestroyHandle(cvitdl_eval_handle_t handle);

/****************************************************************
 * Cityscapes evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_CityscapesInit(cvitdl_eval_handle_t handle, const char *image_dir,
                                               const char *output_dir);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CityscapesGetImage(cvitdl_eval_handle_t handle,
                                                   const uint32_t index, char **fileName);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CityscapesGetImageNum(cvitdl_eval_handle_t handle, uint32_t *num);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CityscapesWriteResult(cvitdl_eval_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *label_frame,
                                                      const int index);

/****************************************************************
 * Coco evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_CocoInit(cvitdl_eval_handle_t handle, const char *pathPrefix,
                                         const char *jsonPath, uint32_t *imageNum);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CocoGetImageIdPair(cvitdl_eval_handle_t handle,
                                                   const uint32_t index, char **filepath, int *id);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CocoInsertObject(cvitdl_eval_handle_t handle, const int id,
                                                 cvtdl_object_t *obj);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CocoStartEval(cvitdl_eval_handle_t handle, const char *filepath);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_CocoEndEval(cvitdl_eval_handle_t handle);

/****************************************************************
 * LFW evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwInit(cvitdl_eval_handle_t handle, const char *filepath,
                                        bool label_pos_first, uint32_t *imageNum);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwGetImageLabelPair(cvitdl_eval_handle_t handle,
                                                     const uint32_t index, char **filepath,
                                                     char **filepath2, int *label);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwInsertFace(cvitdl_eval_handle_t handle, const int index,
                                              const int label, const cvtdl_face_t *face1,
                                              const cvtdl_face_t *face2);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwInsertLabelScore(cvitdl_eval_handle_t handle, const int index,
                                                    const int label, const float score);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwSave2File(cvitdl_eval_handle_t handle, const char *filepath);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwClearInput(cvitdl_eval_handle_t handle);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LfwClearEvalData(cvitdl_eval_handle_t handle);

/****************************************************************
 * Wider Face evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_WiderFaceInit(cvitdl_eval_handle_t handle, const char *datasetDir,
                                              const char *resultDir, uint32_t *imageNum);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_WiderFaceGetImagePath(cvitdl_eval_handle_t handle,
                                                      const uint32_t index, char **filepath);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_WiderFaceResultSave2File(cvitdl_eval_handle_t handle,
                                                         const int index,
                                                         const VIDEO_FRAME_INFO_S *frame,
                                                         const cvtdl_face_t *face);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_WiderFaceClearInput(cvitdl_eval_handle_t handle);

/****************************************************************
 * Market1501 evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_Market1501Init(cvitdl_eval_handle_t handle, const char *filepath);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_Market1501GetImageNum(cvitdl_eval_handle_t handle, bool is_query,
                                                      uint32_t *num);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_Market1501GetPathIdPair(cvitdl_eval_handle_t handle,
                                                        const uint32_t index, bool is_query,
                                                        char **filepath, int *cam_id, int *pid);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_Market1501InsertFeature(cvitdl_eval_handle_t handle,
                                                        const int index, bool is_query,
                                                        const cvtdl_feature_t *feature);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_Market1501EvalCMC(cvitdl_eval_handle_t handle);

/****************************************************************
 * WLFW evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_WflwInit(cvitdl_eval_handle_t handle, const char *filepath,
                                         uint32_t *imageNum);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_WflwGetImage(cvitdl_eval_handle_t handle, const uint32_t index,
                                             char **fileName);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_WflwInsertPoints(cvitdl_eval_handle_t handle, const int index,
                                                 const cvtdl_pts_t points, const int width,
                                                 const int height);
DLL_EXPORT CVI_S32 CVI_TDL_Eval_WflwDistance(cvitdl_eval_handle_t handle);

/****************************************************************
 * LPDR evaluation functions
 **/
DLL_EXPORT CVI_S32 CVI_TDL_Eval_LPDRInit(cvitdl_eval_handle_t handle, const char *pathPrefix,
                                         const char *jsonPath, uint32_t *imageNum);

DLL_EXPORT CVI_S32 CVI_TDL_Eval_LPDRGetImageIdPair(cvitdl_eval_handle_t handle,
                                                   const uint32_t index, char **filepath, int *id);

#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_EVALUATION_H_