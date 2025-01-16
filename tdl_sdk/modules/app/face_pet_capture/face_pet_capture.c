#include "face_pet_capture.h"
#include "core/cvi_tdl_utils.h"
#include "default_config.h"
#include "face_cap_utils.h"

#include <math.h>
#include <sys/time.h>
// #include "core/cvi_tdl_utils.h"
#include "cvi_tdl_log.hpp"
// #include "cvi_venc.h"
// #include "service/cvi_tdl_service.h"
// #include "vpss_helper.h"

void object_to_face(cvtdl_object_t *obj_meta, cvtdl_face_t *face_meta, int class_id, int dst_size) {
  if (face_meta->info) {
    CVI_TDL_Free(face_meta->info);
  }

  face_meta->size = dst_size;
  face_meta->width = obj_meta->width;
  face_meta->height = obj_meta->height;
  face_meta->rescale_type = obj_meta->rescale_type;

  if (dst_size > 0) {
    face_meta->info = (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * dst_size);
    memset(face_meta->info, 0, sizeof(cvtdl_face_info_t) * dst_size);

    int count = 0;
    for (uint32_t i = 0; i < obj_meta->size; i++) {
      if (obj_meta->info[i].classes == class_id) {
        face_meta->info[count].pts.size = 5;
        face_meta->info[count].pts.x = (float *)malloc(5 * sizeof(float));
        face_meta->info[count].pts.y = (float *)malloc(5 * sizeof(float));
        memcpy(&face_meta->info[count].bbox, &obj_meta->info[i].bbox, sizeof(cvtdl_bbox_t));
        count++;
      }
    }
  }
}

void object_to_object(cvtdl_object_t *obj_meta_src, cvtdl_object_t *obj_meta_dst, int class_id,
                      int dst_size) {
  if (obj_meta_dst->info) {
    CVI_TDL_Free(obj_meta_dst->info);
  }

  memcpy(obj_meta_dst, obj_meta_src, sizeof(cvtdl_object_t));
  obj_meta_dst->size = dst_size;
  obj_meta_dst->info = NULL;

  if (dst_size > 0) {
    obj_meta_dst->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * dst_size);
    memset(obj_meta_dst->info, 0, sizeof(cvtdl_object_info_t) * dst_size);

    int count = 0;
    for (uint32_t i = 0; i < obj_meta_src->size; i++) {
      if (obj_meta_src->info[i].classes == class_id) {
        CVI_TDL_CopyObjectInfo(&obj_meta_src->info[i], &obj_meta_dst->info[count]);
        obj_meta_dst->info[count].classes = 0;
        count++;
      }
    }
  }
}

void assign_object(const cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                   cvtdl_object_t *det_obj_meta) {
  int num_info[4] = {0};  // face, head, person, pet

  for (uint32_t i = 0; i < det_obj_meta->size; i++) {
    num_info[det_obj_meta->info[i].classes] += 1;
  }

  cvtdl_face_t head_meta = {0};

  object_to_face(det_obj_meta, &face_cpt_info->last_faces, 0, num_info[0]);
  object_to_face(det_obj_meta, &head_meta, 1, num_info[1]);

  object_to_object(det_obj_meta, &face_cpt_info->last_objects, 2, num_info[2]);

#ifdef DEBUG_TRACK

  printf("last_objects: %d, last_faces: %d, lhead_meta:%d\n", face_cpt_info->last_objects.size,
         face_cpt_info->last_faces.size, head_meta.size);

  for (uint32_t i = 0; i < face_cpt_info->last_objects.size; i++) {
    printf("last_objects: %.4f, %.4f, %.4f, %.4f, %d, %.4f\n",
           face_cpt_info->last_objects.info[i].bbox.x1, face_cpt_info->last_objects.info[i].bbox.y1,
           face_cpt_info->last_objects.info[i].bbox.x2, face_cpt_info->last_objects.info[i].bbox.y2,
           face_cpt_info->last_objects.info[i].classes,
           face_cpt_info->last_objects.info[i].bbox.score);
  }

  for (uint32_t i = 0; i < face_cpt_info->last_faces.size; i++) {
    printf("last_faces: %.4f, %.4f, %.4f, %.4f, %.4f\n", face_cpt_info->last_faces.info[i].bbox.x1,
           face_cpt_info->last_faces.info[i].bbox.y1, face_cpt_info->last_faces.info[i].bbox.x2,
           face_cpt_info->last_faces.info[i].bbox.y2, face_cpt_info->last_faces.info[i].bbox.score);
  }
  for (uint32_t i = 0; i < head_meta.size; i++) {
    printf("head_meta: %.4f, %.4f, %.4f, %.4f, %.4f\n", head_meta.info[i].bbox.x1,
           head_meta.info[i].bbox.y1, head_meta.info[i].bbox.x2, head_meta.info[i].bbox.y2,
           head_meta.info[i].bbox.score);
  }
#endif

  if (num_info[3] > face_cpt_info->pet_objects.size) {
    object_to_object(det_obj_meta, &face_cpt_info->pet_objects, 3, num_info[3]);
  }

  CVI_TDL_FaceHeadIouScore(tdl_handle, &face_cpt_info->last_faces, &head_meta);
  CVI_TDL_Free(&head_meta);
}

CVI_S32 _FacePetCapture_QuickSetUp(cvitdl_handle_t tdl_handle, face_capture_t *face_cpt_info,
                                   int od_model_id, int fr_model_id, const char *od_model_path,
                                   const char *fr_model_path, const char *fl_model_path,
                                   const char *fa_model_path) {
  LOGI("_FacePetCapture_QuickSetUp");

  if (od_model_id != CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION) {
    LOGE("invalid object detection model id %d", od_model_id);
    return CVI_FAILURE;
  }

  CVI_S32 ret = CVI_SUCCESS;

  // setup yolo algorithm preprocess
  cvtdl_det_algo_param_t yolov8_param = CVI_TDL_GetDetectionAlgoParam(tdl_handle, od_model_id);
  yolov8_param.cls = 4;

  printf("setup yolov8 algorithm param \n");
  ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, od_model_id, yolov8_param);
  if (ret != CVI_SUCCESS) {
    printf("Can not set yolov8 algorithm parameters %#x\n", ret);
    return ret;
  }

  ret |= CVI_TDL_OpenModel(tdl_handle, od_model_id, od_model_path);
  if (ret == CVI_SUCCESS) {
    printf("od model %s open sucessfull\n", od_model_path);
  }

  if (fa_model_path != NULL) {
    ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_FACEATTRIBUTE_CLS, fa_model_path);
    if (ret == CVI_SUCCESS) {
      printf("fa model %s open sucessfull\n", fa_model_path);
    }
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

  if (fr_model_path != NULL && strlen(fr_model_path) > 1) {
    ret |= CVI_TDL_OpenModel(tdl_handle, fr_model_id, fr_model_path);
    if (ret == CVI_SUCCESS) {
      printf("fr model %s open sucessfull\n", fr_model_path);
    }
  }

  face_cpt_info->od_model = od_model_id;
  face_cpt_info->od_inference = CVI_TDL_Detection;

  face_cpt_info->fa_inference = CVI_TDL_FaceAttribute_cls;

  if (ret != CVI_TDL_SUCCESS) {
    printf("_FacePetCapture_QuickSetUp failed with %#x!\n", ret);
    return ret;
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

CVI_S32 _FacePetCapture_Run(face_capture_t *face_cpt_info, const cvitdl_handle_t tdl_handle,
                            VIDEO_FRAME_INFO_S *frame) {
  if (face_cpt_info == NULL) {
    LOGE("[APP::FacePetCapture] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }
  if (frame->stVFrame.u32Length[0] == 0) {
    LOGE("[APP::FacePetCapture] got empty frame.\n");
    return CVI_TDL_FAILURE;
  }
  LOGI("[APP::FacePetCapture] RUN (MODE: %d, FR: %d, FQ: %d)\n", face_cpt_info->mode,
       face_cpt_info->fr_flag, face_cpt_info->use_FQNet);
  CVI_S32 ret;
  ret = clean_data(face_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::FacePetCapture] clean data failed.\n");
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

  cvtdl_object_t obj_meta = {0};
  if (CVI_SUCCESS !=
      face_cpt_info->od_inference(tdl_handle, frame, face_cpt_info->od_model, &obj_meta)) {
    CVI_TDL_Release_VideoFrame(tdl_handle, face_cpt_info->od_model, frame, true);
    printf("obj detection failed\n");
    return CVI_TDL_FAILURE;
  }

  assign_object(tdl_handle, face_cpt_info, &obj_meta);

  if (face_cpt_info->fr_flag == 1) {
    if (CVI_SUCCESS != face_cpt_info->fr_inference(tdl_handle, frame, &face_cpt_info->last_faces)) {
      return CVI_TDL_FAILURE;
    }
  }

  for (uint32_t i = 0; i < face_cpt_info->last_faces.size; i++) {
    // use pose score as face_quality
#ifdef DEBUG_TRACK
    printf("face_cpt_info->last_faces.info[i].pose_score: %f\n",
           face_cpt_info->last_faces.info[i].pose_score);
#endif
    face_cpt_info->last_faces.info[i].face_quality = face_cpt_info->last_faces.info[i].pose_score;
  }

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
  if (face_cpt_info->od_model) {
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
                    &face_cpt_info->last_trackers, false);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::FacePetCapture] update face failed.\n");
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
    LOGE("[APP::FacePetCapture] capture face failed.\n");
    return CVI_TDL_FAILURE;
  }

  /* update timestamp*/
  face_cpt_info->_time =
      (face_cpt_info->_time == 0xffffffffffffffff) ? 0 : face_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}
