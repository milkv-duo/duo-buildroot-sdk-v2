#pragma once

#include "core/cvi_tdl_utils.h"
#include "cvi_tdl_app/capture/face_capture_type.h"
#include "default_config.h"

static uint8_t venc_extern_init = 0;
#define EYE_DISTANCE_STANDARD 80.
#define MEMORY_LIMIT (16 * 1024 * 1024) /* example: 16MB */

void face_capture_init_venc(VENC_CHN VeChn);
void release_venc(VENC_CHN VeChn);

float get_score(cvtdl_face_info_t *face_info, uint32_t img_w, uint32_t img_h, bool fl_model);
void face_quality_assessment(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face, bool *skip,
                             quality_assessment_e qa_method, float thr_laplacian);

void encode_img2jpg(VENC_CHN VeChn, VIDEO_FRAME_INFO_S *src_frame, VIDEO_FRAME_INFO_S *crop_frame,
                    cvtdl_image_t *dst_image);
int image_to_video_frame(face_capture_t *face_cpt_info, cvtdl_image_t *image,
                         VIDEO_FRAME_INFO_S *dstFrame);

int update_extend_resize_info(const float frame_width, const float frame_height,
                              cvtdl_face_info_t *face_info, cvtdl_bbox_t *p_dst_box,
                              bool update_info);
CVI_S32 extract_cropped_face(const cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info);

int cal_crop_box(const float frame_width, const float frame_height, cvtdl_bbox_t crop_box,
                 cvtdl_bbox_t *p_dst_box);
float cal_crop_big_box(const float frame_width, const float frame_height, cvtdl_bbox_t crop_box,
                       cvtdl_bbox_t *p_dst_box, int *dts_w, int *dst_h);

CVI_S32 _FaceCapture_SetMode(face_capture_t *face_cpt_info, capture_mode_e mode);
CVI_S32 _FaceCapture_Init(face_capture_t **face_cpt_info, uint32_t buffer_size);
CVI_S32 _FaceCapture_GetDefaultConfig(face_capture_config_t *cfg);
CVI_S32 _FaceCapture_SetConfig(face_capture_t *face_cpt_info, face_capture_config_t *cfg,
                               cvitdl_handle_t tdl_handle);
CVI_S32 _FaceCapture_CleanAll(face_capture_t *face_cpt_info);
CVI_S32 _FaceCapture_Free(face_capture_t *face_cpt_info);

CVI_S32 update_data(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                    VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face_meta,
                    cvtdl_tracker_t *tracker_meta, bool with_pts);
CVI_S32 clean_data(face_capture_t *face_cpt_info);
CVI_S32 update_output_state(const cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info);

void SHOW_CONFIG(face_capture_config_t *cfg);
uint32_t get_time_in_ms();
void set_skipFQsignal(face_capture_t *face_cpt_info, cvtdl_face_t *face_info, bool *skip);
