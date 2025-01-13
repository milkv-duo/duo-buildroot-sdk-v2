#include "core/cvi_tdl_types_mem.h"
#include <string.h>
#include <cvi_tdl_log.hpp>
#include "core/cvi_tdl_types_mem_internal.h"
// Free

void CVI_TDL_FreeCpp(cvtdl_feature_t *feature) {
  if (feature->ptr != NULL) {
    free(feature->ptr);
    feature->ptr = NULL;
  }
  feature->size = 0;
  feature->type = TYPE_INT8;
}

void CVI_TDL_FreeCpp(cvtdl_pts_t *pts) {
  if (pts->x != NULL) {
    free(pts->x);
    pts->x = NULL;
  }
  if (pts->y != NULL) {
    free(pts->y);
    pts->y = NULL;
  }
  pts->size = 0;
}

void CVI_TDL_FreeCpp(cvtdl_tracker_t *tracker) {
  if (tracker->info != NULL) {
    free(tracker->info);
    tracker->info = NULL;
  }
  tracker->size = 0;
}

void CVI_TDL_FreeCpp(cvtdl_dms_od_t *dms_od) {
  if (dms_od->info != NULL) {
    free(dms_od->info);
    dms_od->info = NULL;
  }
  dms_od->size = 0;
  dms_od->width = 0;
  dms_od->height = 0;
}

void CVI_TDL_FreeCpp(cvtdl_dms_t *dms) {
  CVI_TDL_FreeCpp(&dms->landmarks_106);
  CVI_TDL_FreeCpp(&dms->landmarks_5);
  CVI_TDL_FreeCpp(&dms->dms_od);
}

void CVI_TDL_FreeCpp(cvtdl_face_info_t *face_info) {
  CVI_TDL_FreeCpp(&face_info->pts);
  CVI_TDL_FreeCpp(&face_info->feature);
}

void CVI_TDL_FreeCpp(cvtdl_face_t *face) {
  if (face->info) {
    for (uint32_t i = 0; i < face->size; i++) {
      CVI_TDL_FreeCpp(&face->info[i]);
    }
    free(face->info);
    face->info = NULL;
  }
  face->size = 0;
  face->width = 0;
  face->height = 0;

  if (face->dms) {
    CVI_TDL_FreeCpp(face->dms);
    face->dms = NULL;
  }
}

void CVI_TDL_FreeCpp(cvtdl_object_info_t *obj_info) {
  CVI_TDL_FreeCpp(&obj_info->feature);
  if (obj_info->vehicle_properity) {
    free(obj_info->vehicle_properity);
    obj_info->vehicle_properity = NULL;
  }

  if (obj_info->pedestrian_properity) {
    free(obj_info->pedestrian_properity);
    obj_info->pedestrian_properity = NULL;
  }

  if (obj_info->mask_properity) {
    if (obj_info->mask_properity->mask) {
      free(obj_info->mask_properity->mask);
      obj_info->mask_properity->mask = NULL;
    }
    if (obj_info->mask_properity->mask_point) {
      free(obj_info->mask_properity->mask_point);
      obj_info->mask_properity->mask_point = NULL;
    }
    free(obj_info->mask_properity);
    obj_info->mask_properity = NULL;
  }
}

void CVI_TDL_FreeCpp(cvtdl_object_t *obj) {
  if (obj->info != NULL) {
    for (uint32_t i = 0; i < obj->size; i++) {
      CVI_TDL_FreeCpp(&obj->info[i]);
    }
    free(obj->info);
    obj->info = NULL;
  }
  obj->size = 0;
  obj->width = 0;
  obj->height = 0;
}

void CVI_TDL_FreeCpp(cvtdl_image_t *image) {
  if (image->pix[0] != NULL) {
    free(image->pix[0]);
  }
  for (int i = 0; i < 3; i++) {
    image->pix[i] = NULL;
    image->stride[i] = 0;
    image->length[i] = 0;
  }
  image->height = 0;
  image->width = 0;
}

void CVI_TDL_FreeCpp(cvtdl_handpose21_meta_ts *handposes) {
  if (handposes->info != NULL) {
    free(handposes->info);
    handposes->info = NULL;
  }
  handposes->size = 0;
  handposes->width = 0;
  handposes->height = 0;
}

void CVI_TDL_FreeCpp(cvtdl_class_meta_t *cls_meta) {
  for (int i = 0; i < 5; i++) {
    cls_meta->cls[i] = 0;
    cls_meta->score[i] = 0;
  }
}

void CVI_TDL_FreeCpp(cvtdl_seg_logits_t *seg_logits) {
  if (seg_logits->float_logits != NULL) {
    delete seg_logits->float_logits;
    seg_logits->float_logits = NULL;
  }
  if (seg_logits->int_logits != NULL) {
    delete[] seg_logits->int_logits;
    seg_logits->int_logits = NULL;
  }
  seg_logits->w = 0;
  seg_logits->h = 0;
  seg_logits->c = 0;
  seg_logits->is_int = false;
  seg_logits->qscale = 0;
}

void CVI_TDL_FreeCpp(cvtdl_lane_t *lane_meta) {
  if (lane_meta->lane != NULL) {
    free(lane_meta->lane);
    lane_meta->lane = NULL;
    lane_meta->size = 0;
  }

  if (lane_meta->feature != NULL) {
    free(lane_meta->feature);
    lane_meta->feature = NULL;
    lane_meta->feature_size = 0;
  }
}

void CVI_TDL_FreeCpp(cvtdl_clip_feature *clip_meta) {
  if (clip_meta->out_feature != NULL) {
    free(clip_meta->out_feature);
  }
  clip_meta->out_feature = NULL;
  clip_meta->feature_dim = 0;
}

void CVI_TDL_FreeFeature(cvtdl_feature_t *feature) { CVI_TDL_FreeCpp(feature); }

void CVI_TDL_FreePts(cvtdl_pts_t *pts) { CVI_TDL_FreeCpp(pts); }

void CVI_TDL_FreeTracker(cvtdl_tracker_t *tracker) { CVI_TDL_FreeCpp(tracker); }

void CVI_TDL_FreeFaceInfo(cvtdl_face_info_t *face_info) { CVI_TDL_FreeCpp(face_info); }

void CVI_TDL_FreeFace(cvtdl_face_t *face) { CVI_TDL_FreeCpp(face); }

void CVI_TDL_FreeObjectInfo(cvtdl_object_info_t *obj_info) { CVI_TDL_FreeCpp(obj_info); }

void CVI_TDL_FreeObject(cvtdl_object_t *obj) { CVI_TDL_FreeCpp(obj); }

void CVI_TDL_FreeImage(cvtdl_image_t *image) { CVI_TDL_FreeCpp(image); }

void CVI_TDL_FreeDMS(cvtdl_dms_t *dms) { CVI_TDL_FreeCpp(dms); }

void CVI_TDL_FreeHandPoses(cvtdl_handpose21_meta_ts *handposes) { CVI_TDL_FreeCpp(handposes); }

void CVI_TDL_FreeClassMeta(cvtdl_class_meta_t *cls_meta) { CVI_TDL_FreeCpp(cls_meta); }

void CVI_TDL_FreeLane(cvtdl_lane_t *lane_meta) { CVI_TDL_FreeCpp(lane_meta); }

void CVI_TDL_FreeClip(cvtdl_clip_feature *clip_meta) { CVI_TDL_FreeCpp(clip_meta); }
// Copy

void CVI_TDL_CopyInfoCpp(const cvtdl_face_info_t *info, cvtdl_face_info_t *infoNew) {
  memcpy(infoNew->name, info->name, sizeof(info->name));
  infoNew->unique_id = info->unique_id;
  infoNew->bbox = info->bbox;
  infoNew->pts.size = info->pts.size;
  if (infoNew->pts.size != 0) {
    uint32_t pts_size = infoNew->pts.size * sizeof(float);
    infoNew->pts.x = (float *)malloc(pts_size);
    infoNew->pts.y = (float *)malloc(pts_size);
    memcpy(infoNew->pts.x, info->pts.x, pts_size);
    memcpy(infoNew->pts.y, info->pts.y, pts_size);
  } else {
    infoNew->pts.x = NULL;
    infoNew->pts.y = NULL;
  }
  infoNew->feature.type = info->feature.type;
  infoNew->feature.size = info->feature.size;
  if (infoNew->feature.size != 0) {
    uint32_t feature_size = infoNew->feature.size * getFeatureTypeSize(infoNew->feature.type);
    infoNew->feature.ptr = (int8_t *)malloc(feature_size);
    memcpy(infoNew->feature.ptr, info->feature.ptr, feature_size);
  } else {
    infoNew->feature.ptr = NULL;
  }
  infoNew->emotion = info->emotion;
  infoNew->gender = info->gender;
  infoNew->race = info->race;
  infoNew->age = info->age;
  infoNew->liveness_score = info->liveness_score;
  infoNew->hardhat_score = info->hardhat_score;
  infoNew->mask_score = info->mask_score;
  infoNew->face_quality = info->face_quality;
  infoNew->head_pose = info->head_pose;
  infoNew->velx = info->velx;
  infoNew->vely = info->vely;
  infoNew->blurness = info->blurness;
}

void CVI_TDL_CopyInfoCpp(const cvtdl_object_info_t *info, cvtdl_object_info_t *infoNew) {
  memcpy(infoNew->name, info->name, sizeof(info->name));
  infoNew->unique_id = info->unique_id;
  infoNew->bbox = info->bbox;
  infoNew->feature.type = info->feature.type;
  infoNew->feature.size = info->feature.size;

  infoNew->adas_properity.dis = info->adas_properity.dis;
  infoNew->adas_properity.speed = info->adas_properity.speed;
  infoNew->adas_properity.state = info->adas_properity.state;

  // infoNew->human_angle = info->human_angle;
  // infoNew->aspect_ratio = info->aspect_ratio;
  // infoNew->speed = info->speed;
  // infoNew->is_moving = info->is_moving;
  // infoNew->status = info->status;

  if (infoNew->feature.size != 0) {
    uint32_t feature_size = infoNew->feature.size * getFeatureTypeSize(infoNew->feature.type);
    infoNew->feature.ptr = (int8_t *)malloc(feature_size);
    if (infoNew->feature.ptr == nullptr) {
      syslog(LOG_ERR, "malloc failed for feature.ptr\n");
      return;
    }
    memcpy(infoNew->feature.ptr, info->feature.ptr, feature_size);
  } else {
    infoNew->feature.ptr = NULL;
  }

  if (info->vehicle_properity) {
    infoNew->vehicle_properity = (cvtdl_vehicle_meta *)malloc(sizeof(cvtdl_vehicle_meta));
    infoNew->vehicle_properity->license_bbox = info->vehicle_properity->license_bbox;
    memcpy(infoNew->vehicle_properity->license_char, info->vehicle_properity->license_char,
           sizeof(info->vehicle_properity->license_char));
    memcpy(infoNew->vehicle_properity->license_pts.x, info->vehicle_properity->license_pts.x,
           4 * sizeof(float));
  }

  if (info->pedestrian_properity) {
    infoNew->pedestrian_properity = (cvtdl_pedestrian_meta *)malloc(sizeof(cvtdl_pedestrian_meta));
    infoNew->pedestrian_properity->fall = info->pedestrian_properity->fall;
    memcpy(infoNew->pedestrian_properity->pose_17.score, info->pedestrian_properity->pose_17.score,
           sizeof(float) * 17);
    memcpy(infoNew->pedestrian_properity->pose_17.x, info->pedestrian_properity->pose_17.x,
           sizeof(float) * 17);
    memcpy(infoNew->pedestrian_properity->pose_17.y, info->pedestrian_properity->pose_17.y,
           sizeof(float) * 17);
  }

  infoNew->classes = info->classes;
}

void CVI_TDL_CopyInfoCpp(const cvtdl_pts_t *info, cvtdl_pts_t *infoNew) {
  infoNew->size = info->size;
  infoNew->score = info->score;

  if (infoNew->size != 0) {
    uint32_t pts_size = infoNew->size * sizeof(float);
    infoNew->x = (float *)malloc(pts_size);
    infoNew->y = (float *)malloc(pts_size);
    memcpy(infoNew->x, info->x, pts_size);
    memcpy(infoNew->y, info->y, pts_size);
  } else {
    infoNew->x = NULL;
    infoNew->y = NULL;
  }
}

void CVI_TDL_CopyInfoCpp(const cvtdl_dms_t *info, cvtdl_dms_t *infoNew) {
  infoNew->reye_score = info->reye_score;
  infoNew->leye_score = info->leye_score;
  infoNew->yawn_score = info->yawn_score;
  infoNew->phone_score = info->phone_score;
  infoNew->smoke_score = info->smoke_score;

  infoNew->head_pose = info->head_pose;

  CVI_TDL_CopyInfoCpp(&info->landmarks_106, &infoNew->landmarks_106);
  CVI_TDL_CopyInfoCpp(&info->landmarks_5, &infoNew->landmarks_5);

  infoNew->dms_od.size = info->dms_od.size;
  infoNew->dms_od.width = info->dms_od.width;
  infoNew->dms_od.height = info->dms_od.height;
  infoNew->dms_od.rescale_type = info->dms_od.rescale_type;
  if (info->dms_od.info) {
    CVI_TDL_CopyInfoCpp(info->dms_od.info, infoNew->dms_od.info);
  }
}

void CVI_TDL_CopyInfoCpp(const cvtdl_dms_od_info_t *info, cvtdl_dms_od_info_t *infoNew) {
  memcpy(infoNew->name, info->name, sizeof(info->name));
  infoNew->bbox = info->bbox;
  infoNew->classes = info->classes;
}
void CVI_TDL_CopyFaceInfo(const cvtdl_face_info_t *info, cvtdl_face_info_t *infoNew) {
  CVI_TDL_CopyInfoCpp(info, infoNew);
}

void CVI_TDL_CopyObjectInfo(const cvtdl_object_info_t *info, cvtdl_object_info_t *infoNew) {
  CVI_TDL_CopyInfoCpp(info, infoNew);
}

void CVI_TDL_CopyFaceMeta(const cvtdl_face_t *src, cvtdl_face_t *dest) {
  CVI_TDL_FreeCpp(dest);
  memset(dest, 0, sizeof(cvtdl_face_t));
  if (src->size > 0) {
    dest->size = src->size;
    dest->width = src->width;
    dest->height = src->height;
    dest->rescale_type = src->rescale_type;
    if (src->info) {
      dest->info = (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * src->size);
      memset(dest->info, 0, sizeof(cvtdl_face_info_t) * src->size);
      for (uint32_t fid = 0; fid < src->size; fid++) {
        CVI_TDL_CopyFaceInfo(&src->info[fid], &dest->info[fid]);
      }
    }

    if (src->dms) {
      dest->dms = (cvtdl_dms_t *)malloc(sizeof(cvtdl_dms_t));
      memset(dest->dms, 0, sizeof(cvtdl_dms_t));
      CVI_TDL_CopyInfoCpp(src->dms, dest->dms);
    }
  }
}

void CVI_TDL_CopyObjectMeta(const cvtdl_object_t *src, cvtdl_object_t *dest) {
  CVI_TDL_FreeCpp(dest);
  memset(dest, 0, sizeof(cvtdl_object_t));
  if (src->size > 0) {
    dest->size = src->size;
    dest->width = src->width;
    dest->height = src->height;
    dest->rescale_type = src->rescale_type;
    if (src->info) {
      dest->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * src->size);
      memset(dest->info, 0, sizeof(cvtdl_object_info_t) * src->size);
      for (uint32_t fid = 0; fid < src->size; fid++) {
        CVI_TDL_CopyObjectInfo(&src->info[fid], &dest->info[fid]);
      }
    }
  }
}

void CVI_TDL_CopyLaneMeta(cvtdl_lane_t *src, cvtdl_lane_t *dst) {
  CVI_TDL_FreeCpp(dst);
  memset(dst, 0, sizeof(cvtdl_lane_t));
  if (src->size > 0) {
    dst->size = src->size;
    dst->width = src->width;
    dst->height = src->height;
    dst->rescale_type = src->rescale_type;
    dst->lane_state = src->lane_state;
    dst->lane = (cvtdl_lane_point_t *)malloc(sizeof(cvtdl_lane_point_t) * src->size);
    memcpy(dst->lane, src->lane, sizeof(cvtdl_lane_point_t) * src->size);
  }
}

void CVI_TDL_CopyHandPoses(const cvtdl_handpose21_meta_ts *src, cvtdl_handpose21_meta_ts *dest) {
  CVI_TDL_FreeCpp(dest);
  memset(dest, 0, sizeof(cvtdl_handpose21_meta_ts));
  dest->width = src->width;
  dest->height = src->height;
  dest->size = src->size;
  if (src->size > 0) {
    dest->info = (cvtdl_handpose21_meta_t *)malloc(sizeof(cvtdl_handpose21_meta_t) * src->size);
    memset(dest->info, 0, sizeof(cvtdl_handpose21_meta_t) * src->size);
    memcpy(dest->info, src->info, sizeof(cvtdl_handpose21_meta_t) * src->size);
  }
}

void CVI_TDL_CopyTrackerMeta(const cvtdl_tracker_t *src, cvtdl_tracker_t *dst) {
  if (src->size != dst->size) {
    CVI_TDL_FreeCpp(dst);
  }
  dst->size = src->size;
  if (dst->size != 0) {
    dst->info = (cvtdl_tracker_info_t *)malloc(sizeof(cvtdl_tracker_info_t) * src->size);
    memcpy(dst->info, src->info, sizeof(cvtdl_tracker_info_t) * src->size);
  } else {
    dst->info = NULL;
  }
}

void CVI_TDL_CopyImage(const cvtdl_image_t *src_image, cvtdl_image_t *dst_image) {
  if (dst_image->pix[0] != NULL) {
    printf("There are already data in destination image. (release them ...)");
    CVI_TDL_FreeCpp(dst_image);
  }
  dst_image->pix_format = src_image->pix_format;
  dst_image->height = src_image->height;
  dst_image->width = src_image->width;

  // copy crop image to dst image
  uint32_t image_size = src_image->length[0] + src_image->length[1] + src_image->length[2];
  dst_image->pix[0] = (uint8_t *)malloc(image_size);
  memcpy(dst_image->pix[0], src_image->pix[0], image_size);
  // copy full image to dst image
  dst_image->full_length = src_image->full_length;
  dst_image->full_img = (uint8_t *)malloc(src_image->full_length);
  memcpy(dst_image->full_img, src_image->full_img, src_image->full_length);
  for (int i = 0; i < 3; i++) {
    dst_image->stride[i] = src_image->stride[i];
    dst_image->length[i] = src_image->length[i];
    if (i != 0 && dst_image->length[i] != 0) {
      dst_image->pix[i] = dst_image->pix[i - 1] + dst_image->length[i - 1];
    }
  }
}

void CVI_TDL_MapImage(VIDEO_FRAME_INFO_S *frame, bool *p_is_mapped) {
  *p_is_mapped = false;
  CVI_U32 frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    *p_is_mapped = true;
  }
}
void CVI_TDL_UnMapImage(VIDEO_FRAME_INFO_S *frame, bool do_unmap) {
  CVI_U32 frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
}
CVI_S32 CVI_TDL_CopyVpssImage(VIDEO_FRAME_INFO_S *src_frame, cvtdl_image_t *dst_image) {
  if (src_frame->stVFrame.enPixelFormat != dst_image->pix_format) {
    printf("pixel format type not match,src:%d,dst:%d\n", (int)src_frame->stVFrame.enPixelFormat,
           (int)dst_image->pix_format);
    return CVI_FAILURE;
  }
  bool unmap = false;
  CVI_TDL_MapImage(src_frame, &unmap);
  CVI_U32 frame_size = src_frame->stVFrame.u32Length[0] + src_frame->stVFrame.u32Length[1] +
                       src_frame->stVFrame.u32Length[2];
  memcpy(dst_image->pix[0], src_frame->stVFrame.pu8VirAddr[0], frame_size);
  CVI_TDL_UnMapImage(src_frame, unmap);
  return CVI_SUCCESS;
}
