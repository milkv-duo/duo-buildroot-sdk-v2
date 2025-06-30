#include "face_capture.h"
#include "default_config.h"
#include "face_cap_utils.h"

#include <math.h>
#include <sys/time.h>
// #include "core/cvi_tdl_utils.h"
#include "cvi_tdl_log.hpp"
// #include "cvi_venc.h"
#include "service/cvi_tdl_service.h"
// #include "vpss_helper.h"

#define FACE_AREA_STANDARD (112 * 112)

#define UPDATE_VALUE_MIN 0.1
// TODO: Use cooldown to avoid too much updating
#define UPDATE_COOLDOWN 3
#define CAPTURE_FACE_LIVE_TIME_EXTEND 5

CVI_S32 _FaceCapture_QuickSetUp(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                                int fd_model_id, int fr_model_id, const char *fd_model_path,
                                const char *fr_model_path, const char *fq_model_path,
                                const char *fl_model_path, const char *fa_model_path) {
  LOGI("_FaceCapture_QuickSetUp");
  if (fd_model_id != CVI_TDL_SUPPORTED_MODEL_RETINAFACE &&
      fd_model_id != CVI_TDL_SUPPORTED_MODEL_FACEMASKDETECTION &&
      fd_model_id != CVI_TDL_SUPPORTED_MODEL_SCRFDFACE) {
    LOGE("invalid face detection model id %d", fd_model_id);
    return CVI_FAILURE;
  }
  if (fr_model_id != CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION &&
      fr_model_id != CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE) {
    LOGE("invalid face recognition model id %d", fr_model_id);
    return CVI_FAILURE;
  }
  CVI_S32 ret = CVI_SUCCESS;
  ret |= CVI_TDL_OpenModel(tdl_handle, fd_model_id, fd_model_path);
  if (ret != CVI_SUCCESS) {
    printf("fd model %s open failed\n", fd_model_path);
    return CVI_FAILURE;
  }
  if (fr_model_path != NULL && strlen(fr_model_path) > 1) {
    ret |= CVI_TDL_OpenModel(tdl_handle, fr_model_id, fr_model_path);
    if (ret == CVI_SUCCESS) {
      printf("fr model %s open sucessfull\n", fr_model_path);
    } else {
      printf("open fr model failed,ret:%d\n", ret);
      return ret;
    }
  }

  if (fa_model_path != NULL) {
    ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, fa_model_path);
    if (ret == CVI_SUCCESS) {
      printf("fa model %s open sucessfull\n", fa_model_path);
    } else {
      printf("open fa model failed,ret:%d\n", ret);
      return ret;
    }
  }

  if (fq_model_path != NULL) {
    ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEQUALITY, fq_model_path);
  }
  if (fl_model_path != NULL) {
    CVI_TDL_SUPPORTED_MODEL_E flmodel =
        CVI_TDL_SUPPORTED_MODEL_FACELANDMARKERDET2;  // CVI_TDL_SUPPORTED_MODEL_LANDMARK_DET3;
    ret |= CVI_TDL_OpenModel(tdl_handle, flmodel, fl_model_path);
    if (ret == CVI_SUCCESS) {
      face_cpt_info->fl_model = flmodel;
    } else {
      printf("open landmark failed,ret:%d\n", ret);
      return ret;
    }
  }

  if (fd_model_id == CVI_TDL_SUPPORTED_MODEL_RETINAFACE) {
    face_cpt_info->fd_inference = CVI_TDL_FaceDetection;
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RETINAFACE, false);
  } else if (fd_model_id == CVI_TDL_SUPPORTED_MODEL_SCRFDFACE) {
    face_cpt_info->fd_inference = CVI_TDL_FaceDetection;
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, false);
  } else {
    face_cpt_info->fd_inference = CVI_TDL_FaceDetection;
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEMASKDETECTION, false);
  }

  face_cpt_info->fd_model = fd_model_id;
  printf("frmodelid:%d\n", (int)fr_model_id);
  face_cpt_info->fr_inference = (fr_model_id == CVI_TDL_SUPPORTED_MODEL_FACERECOGNITION)
                                    ? CVI_TDL_FaceRecognition
                                    : CVI_TDL_FaceAttribute;

  face_cpt_info->fa_inference = CVI_TDL_FaceAttribute_cls;

  if (ret != CVI_TDL_SUCCESS) {
    printf("_FaceCapture_QuickSetUp failed with %#x!\n", ret);
    return ret;
  }

  face_cpt_info->use_FQNet = fq_model_path != NULL;
  if (fr_model_path != NULL) {
    face_cpt_info->fr_flag = 1;
  }
  if (fa_model_path != NULL) {
    face_cpt_info->fa_flag = 1;
  } else {
    face_cpt_info->fa_flag = 0;
  }

  /* Init DeepSORT */
  CVI_TDL_DeepSORT_Init(tdl_handle, false);

  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  ds_conf.ktracker_conf.max_unmatched_num = 20;
  ds_conf.ktracker_conf.accreditation_threshold = 10;
  ds_conf.ktracker_conf.P_beta[2] = 0.1;
  ds_conf.ktracker_conf.P_beta[6] = 2.5e-2;
  ds_conf.kfilter_conf.Q_beta[2] = 0.1;
  ds_conf.kfilter_conf.Q_beta[6] = 2.5e-2;
  ds_conf.kfilter_conf.R_beta[2] = 0.1;
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, false);
  printf("CVI_TDL_DeepSORT_SetConfig done\n");
  return CVI_TDL_SUCCESS;
}

CVI_S32 _FaceCapture_FusePedSetUp(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                                  int ped_model_id, const char *ped_model_path) {
  if (ped_model_id != CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN) {
    return CVI_FAILURE;
  }
  int ret = CVI_TDL_SUCCESS;
  face_cpt_info->od_model = ped_model_id;
  ret |= CVI_TDL_OpenModel(tdl_handle, face_cpt_info->od_model, ped_model_path);
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, face_cpt_info->od_model, false);
  printf("ped model open ok\n");
  return ret;
}

CVI_S32 _FaceCapture_Run(face_capture_t *face_cpt_info, const cvitdl_handle_t tdl_handle,
                         VIDEO_FRAME_INFO_S *frame) {
  if (face_cpt_info == NULL) {
    LOGE("[APP::FaceCapture] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }
  if (frame->stVFrame.u32Length[0] == 0) {
    LOGE("[APP::FaceCapture] got empty frame.\n");
    return CVI_TDL_FAILURE;
  }
  LOGI("[APP::FaceCapture] RUN (MODE: %d, FR: %d, FQ: %d)\n", face_cpt_info->mode,
       face_cpt_info->fr_flag, face_cpt_info->use_FQNet);
  CVI_S32 ret;
  ret = clean_data(face_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::FaceCapture] clean data failed.\n");
    return CVI_TDL_FAILURE;
  }
  CVI_TDL_Free(&face_cpt_info->last_faces);
  CVI_TDL_Free(&face_cpt_info->last_trackers);
  CVI_TDL_Free(&face_cpt_info->last_objects);

  uint32_t ts = get_time_in_ms();
  /* set output signal to 0. */
  for (int i = 0; i < face_cpt_info->size; i++) {
    // do not reset quality,send overall high quality
    if (face_cpt_info->_output[i]) {
      face_cpt_info->data[i].info.face_quality =
          0;  // set fq to zero ,force to update data at next time
      face_cpt_info->data[i].info.pose_score1 = 0;
    }
    face_cpt_info->_output[i] = false;
  }

  if (face_cpt_info->od_model != 0) {
    VIDEO_FRAME_INFO_S *infer_frame = NULL;
    int fd_w = 768;
    int fd_h = 432;
    ret = CVI_TDL_Resize_VideoFrame(tdl_handle, face_cpt_info->fd_model, frame, fd_w, fd_h,
                                    PIXEL_FORMAT_NV21, &infer_frame);
    if (ret != CVI_SUCCESS) {
      printf("resize frame failed\n");
      return CVI_TDL_FAILURE;
    }

    if (CVI_SUCCESS != CVI_TDL_Detection(tdl_handle, infer_frame, face_cpt_info->od_model,
                                         &face_cpt_info->last_objects)) {
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fd_model, infer_frame, true);
      printf("ped detection failed\n");
      return CVI_TDL_FAILURE;
    }
    ret = face_cpt_info->fd_inference(tdl_handle, infer_frame, face_cpt_info->fd_model,
                                      &face_cpt_info->last_faces);
    if (ret != CVI_SUCCESS) {
      printf("fd detection failed\n");
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fd_model, infer_frame, true);
      return CVI_TDL_FAILURE;
    }

    CVI_TDL_RescaleMetaCenterFace(frame, &face_cpt_info->last_faces);
    CVI_TDL_RescaleMetaCenterObj(frame, &face_cpt_info->last_objects);
    if (CVI_TDL_SUCCESS !=
        CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fd_model, infer_frame, true)) {
      printf("release frame failed\n");
      return CVI_TDL_FAILURE;
    }
  } else {
    if (CVI_TDL_SUCCESS != face_cpt_info->fd_inference(tdl_handle, frame, face_cpt_info->fd_model,
                                                       &face_cpt_info->last_faces)) {
      return CVI_TDL_FAILURE;
    }
  }

  if (face_cpt_info->fr_flag == 1) {
    if (CVI_SUCCESS != face_cpt_info->fr_inference(tdl_handle, frame, &face_cpt_info->last_faces)) {
      return CVI_TDL_FAILURE;
    }
  }

  CVI_TDL_Service_FaceAngleForAll(&face_cpt_info->last_faces);
  bool *skip = (bool *)malloc(sizeof(bool) * face_cpt_info->last_faces.size);
  set_skipFQsignal(face_cpt_info, &face_cpt_info->last_faces, skip);
  face_quality_assessment(frame, &face_cpt_info->last_faces, skip, face_cpt_info->cfg.qa_method,
                          face_cpt_info->cfg.thr_laplacian);
  if (face_cpt_info->use_FQNet) {
#ifndef NO_OPENCV
    if (CVI_TDL_SUCCESS !=
        CVI_TDL_FaceQuality(tdl_handle, frame, &face_cpt_info->last_faces, skip)) {
      return CVI_TDL_FAILURE;
    }
#endif
  } else {
    for (uint32_t i = 0; i < face_cpt_info->last_faces.size; i++) {
      // use pose score as face_quality
      face_cpt_info->last_faces.info[i].face_quality = face_cpt_info->last_faces.info[i].pose_score;
    }
  }
  free(skip);

#ifdef DEBUG_TRACK
  for (uint32_t j = 0; j < face_cpt_info->last_faces.size; j++) {
    LOGI("face[%u]: quality[%.4f], pose[%.2f][%.2f][%.2f]\n", j,
         face_cpt_info->last_faces.info[j].face_quality,
         face_cpt_info->last_faces.info[j].head_pose.yaw,
         face_cpt_info->last_faces.info[j].head_pose.pitch,
         face_cpt_info->last_faces.info[j].head_pose.roll);
  }
#endif

  CVI_TDL_DeepSORT_Set_Timestamp(tdl_handle, ts);
  if (face_cpt_info->od_model != 0) {
    CVI_TDL_DeepSORT_FaceFusePed(tdl_handle, &face_cpt_info->last_faces,
                                 &face_cpt_info->last_objects, &face_cpt_info->last_trackers);
  } else {
    CVI_TDL_DeepSORT_Face(tdl_handle, &face_cpt_info->last_faces, &face_cpt_info->last_trackers);
  }
  if (frame->stVFrame.u32Length[0] == 0) {
    LOGE("input frame turn into empty\n");
    return CVI_TDL_FAILURE;
  }

  ret = update_data(tdl_handle, face_cpt_info, frame, &face_cpt_info->last_faces,
                    &face_cpt_info->last_trackers, true);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::FaceCapture] update face failed.\n");
    return CVI_TDL_FAILURE;
  }

  update_output_state(tdl_handle, face_cpt_info);

  for (size_t i = 0; i < face_cpt_info->size; i++) {
    if (face_cpt_info->fa_flag == 1 && face_cpt_info->_output[i]) {
      VIDEO_FRAME_INFO_S image_frame;
      VIDEO_FRAME_INFO_S *face_frame = NULL;

      image_to_video_frame(face_cpt_info, &face_cpt_info->data[i].image, &image_frame);

      CVI_TDL_CropResizeImage(tdl_handle, face_cpt_info->fl_model, &image_frame,
                              &face_cpt_info->data[i].crop_face_box, 112, 112, PIXEL_FORMAT_RGB_888,
                              &face_frame);

      if (CVI_SUCCESS !=
          face_cpt_info->fa_inference(tdl_handle, face_frame, &face_cpt_info->data[i].face_data)) {
        return CVI_TDL_FAILURE;
      }

#ifdef DEBUG_TRACK
      printf("gender: %s  score: %f \n",
             face_cpt_info->data[i].face_data.info[0].gender_score < 0.5 ? "female" : "male",
             face_cpt_info->data[i].face_data.info[0].gender_score);

      printf("age: %d  score: %f \n", (int)(face_cpt_info->data[i].face_data.info[0].age * 100),
             face_cpt_info->data[i].face_data.info[0].age);
      printf("glass: %d  score: %f \n",
             face_cpt_info->data[i].face_data.info[0].glass < 0.5 ? 0 : 1,
             face_cpt_info->data[i].face_data.info[0].glass);
      printf("mask: %d  score: %f\n",
             face_cpt_info->data[i].face_data.info[0].mask_score < 0.5 ? 0 : 1,
             face_cpt_info->data[i].face_data.info[0].mask_score);
#endif
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, &image_frame, 0);
      CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->fl_model, face_frame, 0);
    }
  }

  if (face_cpt_info->fr_flag == 2) {
    // extract face feature from cropped image
    extract_cropped_face(tdl_handle, face_cpt_info);
  }
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::FaceCapture] capture face failed.\n");
    return CVI_TDL_FAILURE;
  }

  /* update timestamp*/
  face_cpt_info->_time =
      (face_cpt_info->_time == 0xffffffffffffffff) ? 0 : face_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}
