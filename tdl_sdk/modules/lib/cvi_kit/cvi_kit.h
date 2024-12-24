#ifndef _CVI_DARW_RECT_HEAD_
#define _CVI_DARW_RECT_HEAD_

#include <cvi_type.h>
#define DLL_EXPORT __attribute__((visibility("default")))
#include "cvi_tdl.h"
#include "cvi_tdl_app.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_OPENCV
DLL_EXPORT CVI_S32 CVI_TDL_ShowLanePoints(VIDEO_FRAME_INFO_S *bg, cvtdl_lane_t *lane_meta,
                                          char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_ShowKeypoints(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta,
                                         float score, char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_ShowKeypointsAndText(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta,
                                                char* save_path, char* text, float score);

DLL_EXPORT CVI_S32 CVI_TDL_SavePicture(VIDEO_FRAME_INFO_S *bg, char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_Cal_Similarity(cvtdl_feature_t feature, cvtdl_feature_t feature1,
                                          float *similarity);

DLL_EXPORT CVI_S32 CVI_TDL_Set_MaskOutlinePoint(VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_object_t *obj_meta);

DLL_EXPORT CVI_S32 CVI_TDL_Draw_ADAS(cvitdl_app_handle_t app_handle, VIDEO_FRAME_INFO_S *bg, 
                                                char* save_path);

#endif
DLL_EXPORT CVI_S32 CVI_TDL_Set_ClipPostprocess(float **text_features, int text_features_num,
                                               float **image_features, int image_features_num,
                                               float **probs);
                                               
DLL_EXPORT CVI_S32 CVI_TDL_Set_TextPreprocess(const char *encoderFile, const char *bpeFile,
                                              const char *textFile, int32_t **tokens,
                                              int numSentences);

#ifdef __cplusplus
}
#endif
#endif