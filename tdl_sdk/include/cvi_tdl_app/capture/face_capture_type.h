#ifndef _CVI_TDL_APP_FACE_CAPTURE_TYPE_H_
#define _CVI_TDL_APP_FACE_CAPTURE_TYPE_H_

#include "capture_type.h"
#include "core/cvi_tdl_core.h"

typedef struct {
  cvtdl_face_info_t info;
  tracker_state_e state;
  uint32_t miss_counter;
  cvtdl_image_t image;
  cvtdl_bbox_t crop_box;       // box used to crop image
  cvtdl_bbox_t crop_face_box;  // box used to crop face from image
  bool _capture;
  uint64_t _timestamp;  // output timestamp
  uint32_t _out_counter;
  uint64_t cap_timestamp;       // frame id of the captured image
  uint64_t last_cap_timestamp;  // frame id of the last captured image
  int matched_gallery_idx;      // used for face recognition
  cvtdl_face_t face_data;       // store the result of face attribute inference

} face_cpt_data_t;

typedef struct {
  int thr_size_min;
  int thr_size_max;
  int qa_method;  // 0:use posture (default),1:laplacian,2:posture+laplacian
  float thr_quality;
  float thr_yaw;
  float thr_pitch;
  float thr_roll;
  float thr_laplacian;
  uint32_t miss_time_limit;  // missed time limit,when reach this limit the track would be destroyed
  uint32_t m_interval;     // frame interval between current frame and last captured frame,if reach
                           // this val would send captured image out
  uint32_t m_capture_num;  // max capture times
  bool auto_m_fast_cap;    // if true,would send first image out in auto mode
  uint8_t venc_channel_id;
  int img_capture_flag;         // 0:capture extended face,1:capture whole frame and extended
                                // face,2:capture half body,3:capture whole frame and half body
  bool store_feature;           // if true,would export face feature ,should set FR
  float eye_dist_thresh;        // default 20
  float landmark_score_thresh;  // default 0.5
} face_capture_config_t;

typedef struct {
  capture_mode_e mode;
  face_capture_config_t cfg;

  uint32_t size;
  face_cpt_data_t *data;  // 缓存的20张人脸存放在data
  cvtdl_face_t last_faces;
  cvtdl_object_t last_objects;  // if fuse with PD,would have PD result stored here
  cvtdl_object_t pet_objects;
  cvtdl_tracker_t last_trackers;
  CVI_TDL_SUPPORTED_MODEL_E fd_model;
  CVI_TDL_SUPPORTED_MODEL_E od_model;
  CVI_TDL_SUPPORTED_MODEL_E fl_model;
  CVI_TDL_SUPPORTED_MODEL_E fa_model;

  int fa_flag;  // 1: open face attribute classification

  int fr_flag;  // 0:only face detect and track,no fr,1:reid,2:no reid but fr for captured face
                // image,used for later face_recognition

  bool use_FQNet; /* don't set manually */

  int (*fd_inference)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, CVI_TDL_SUPPORTED_MODEL_E,
                      cvtdl_face_t *);
  int (*fr_inference)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, cvtdl_face_t *);
  int (*fa_inference)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, cvtdl_face_t *);
  int (*od_inference)(cvitdl_handle_t, VIDEO_FRAME_INFO_S *, CVI_TDL_SUPPORTED_MODEL_E,
                      cvtdl_object_t *);

  bool *_output;   // output signal (# = .size)   // 缓存的20张人脸对应的_output
  uint64_t _time;  // timer
  uint32_t _m_limit;

  CVI_U64 tmp_buf_physic_addr;
  CVI_VOID *p_tmp_buf_addr;
} face_capture_t;

#endif  // End of _CVI_TDL_APP_FACE_CAPTURE_TYPE_H_
