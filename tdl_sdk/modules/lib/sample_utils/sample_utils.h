#ifndef _CVI_DARW_RECT_HEAD_
#define _CVI_DARW_RECT_HEAD_

#include <cvi_type.h>
#define DLL_EXPORT __attribute__((visibility("default")))
#include "cvi_tdl.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_OPENCV
DLL_EXPORT CVI_S32 CVI_TDL_ShowLanePoints(VIDEO_FRAME_INFO_S *bg, cvtdl_lane_t *lane_meta,
                                          char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_ShowKeypoints(VIDEO_FRAME_INFO_S *bg, cvtdl_object_t *obj_meta,
                                         float score, char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_SavePicture(VIDEO_FRAME_INFO_S *bg, char *save_path);

DLL_EXPORT CVI_S32 CVI_TDL_Cal_Similarity(cvtdl_feature_t feature, cvtdl_feature_t feature1,
                                          float *similarity);
#endif

#ifdef __cplusplus
}
#endif
#endif