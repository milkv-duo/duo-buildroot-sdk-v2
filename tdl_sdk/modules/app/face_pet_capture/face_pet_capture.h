#ifndef _CVI_TDL_APP_FACE_PET_CAPTURE_H_
#define _CVI_TDL_APP_FACE_PET_CAPTURE_H_

#include "core/cvi_tdl_core.h"
#include "cvi_tdl_app/capture/face_capture_type.h"

CVI_S32 _FacePetCapture_QuickSetUp(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                                   int od_model_id, int fr_model_id, const char *od_model_path,
                                   const char *fr_model_path, const char *fl_model_path,
                                   const char *fa_model_path);

CVI_S32 _FacePetCapture_Run(face_capture_t *face_cpt_info, const cvitdl_handle_t tdl_handle,
                            VIDEO_FRAME_INFO_S *frame);

void object_to_face(cvtdl_object_t *obj_meta, cvtdl_face_t *face_meta, int class_id, int dst_size);
void object_to_object(cvtdl_object_t *obj_meta_src, cvtdl_object_t *obj_meta_dst, int class_id,
                      int dst_size);
void assign_object(const cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                   cvtdl_object_t *det_obj_meta);
#endif  // End of _CVI_TDL_APP_FACE_PET_CAPTURE_H_
