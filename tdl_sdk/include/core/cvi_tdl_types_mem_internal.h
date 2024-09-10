#ifndef _CVI_TDL_TYPES_MEM_INTERNAL_H_
#define _CVI_TDL_TYPES_MEM_INTERNAL_H_
#include <stdio.h>
#include <string.h>
#include "core/cvi_tdl_types_mem.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

inline void CVI_TDL_MemAlloc(const uint32_t unit_len, const uint32_t size,
                             const feature_type_e type, cvtdl_feature_t *feature) {
  if (feature->size != size || feature->type != type) {
    free(feature->ptr);
    feature->ptr = (int8_t *)malloc(unit_len * size);
    feature->size = size;
    feature->type = type;
  }
}

inline void CVI_TDL_MemAlloc(const uint32_t size, cvtdl_pts_t *pts) {
  if (pts->size != size) {
    free(pts->x);
    free(pts->y);
    pts->x = (float *)malloc(sizeof(float) * size);
    pts->y = (float *)malloc(sizeof(float) * size);
    pts->size = size;
  }
}

inline void CVI_TDL_MemAlloc(const uint32_t size, cvtdl_tracker_t *tracker) {
  if (tracker->size != size) {
    free(tracker->info);
    tracker->info = (cvtdl_tracker_info_t *)malloc(size * sizeof(cvtdl_tracker_info_t));
    tracker->size = size;
  }
}

inline void CVI_TDL_MemAlloc(const uint32_t size, cvtdl_face_t *meta) {
  if (meta->size != size) {
    for (uint32_t i = 0; i < meta->size; i++) {
      CVI_TDL_FreeCpp(&meta->info[i]);
      free(meta->info);
    }
    meta->size = size;
    meta->info = (cvtdl_face_info_t *)malloc(sizeof(cvtdl_face_info_t) * meta->size);
    meta->dms = NULL;
  }
}

inline void CVI_TDL_MemAlloc(const uint32_t size, cvtdl_object_t *meta) {
  if (meta->size != size) {
    for (uint32_t i = 0; i < meta->size; i++) {
      CVI_TDL_FreeCpp(&meta->info[i]);
      free(meta->info);
    }
    meta->size = size;
    meta->info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * meta->size);
  }
}

inline void CVI_TDL_MemAllocInit(const uint32_t size, cvtdl_object_t *meta) {
  CVI_TDL_MemAlloc(size, meta);
  for (uint32_t i = 0; i < meta->size; ++i) {
    memset(&meta->info[i], 0, sizeof(cvtdl_object_info_t));
    meta->info[i].bbox.x1 = -1;
    meta->info[i].bbox.x2 = -1;
    meta->info[i].bbox.y1 = -1;
    meta->info[i].bbox.y2 = -1;

    meta->info[i].name[0] = '\0';
    meta->info[i].classes = -1;
  }
}

inline void CVI_TDL_MemAllocInit(const uint32_t size, const uint32_t pts_num, cvtdl_face_t *meta) {
  CVI_TDL_MemAlloc(size, meta);
  for (uint32_t i = 0; i < meta->size; ++i) {
    meta->info[i].bbox.x1 = -1;
    meta->info[i].bbox.x2 = -1;
    meta->info[i].bbox.y1 = -1;
    meta->info[i].bbox.y2 = -1;

    meta->info[i].name[0] = '\0';
    meta->info[i].emotion = EMOTION_UNKNOWN;
    meta->info[i].gender = GENDER_UNKNOWN;
    meta->info[i].race = RACE_UNKNOWN;
    meta->info[i].age = -1;
    meta->info[i].liveness_score = -1;
    meta->info[i].mask_score = -1;
    meta->info[i].hardhat_score = -1;
    meta->info[i].face_quality = 0;
    meta->info[i].head_pose.yaw = 0;
    meta->info[i].head_pose.pitch = 0;
    meta->info[i].head_pose.roll = 0;
    memset(&meta->info[i].head_pose.facialUnitNormalVector, 0, sizeof(float) * 3);

    memset(&meta->info[i].feature, 0, sizeof(cvtdl_feature_t));
    if (pts_num > 0) {
      meta->info[i].pts.x = (float *)malloc(sizeof(float) * pts_num);
      meta->info[i].pts.y = (float *)malloc(sizeof(float) * pts_num);
      meta->info[i].pts.size = pts_num;
      for (uint32_t j = 0; j < meta->info[i].pts.size; ++j) {
        meta->info[i].pts.x[j] = -1;
        meta->info[i].pts.y[j] = -1;
      }
    } else {
      memset(&meta->info[i].pts, 0, sizeof(meta->info[i].pts));
    }
  }
}

inline void CVI_TDL_MemAllocInit(const uint32_t size, cvtdl_lane_t *meta) {
  if (meta->size != size) {
    // for (uint32_t i = 0; i < meta->size; i++) {
    free(meta->lane);
    // }
    meta->size = size;
    meta->lane = (cvtdl_lane_point_t *)malloc(sizeof(cvtdl_lane_point_t) * meta->size);
  }
}

inline void __attribute__((always_inline))
featurePtrConvert2Float(cvtdl_feature_t *feature, float *output) {
  switch (feature->type) {
    case TYPE_INT8: {
      int8_t *ptr = (int8_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_UINT8: {
      uint8_t *ptr = (uint8_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_INT16: {
      int16_t *ptr = (int16_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_UINT16: {
      uint16_t *ptr = (uint16_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_INT32: {
      int32_t *ptr = (int32_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_UINT32: {
      uint32_t *ptr = (uint32_t *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    case TYPE_BF16: {
      uint16_t *ptr = (uint16_t *)(feature->ptr);
      union {
        float a;
        uint16_t b[2];
      } bfb;
      bfb.b[0] = 0;
      for (uint32_t i = 0; i < feature->size; ++i) {
        bfb.b[1] = ptr[i];
        output[i] = bfb.a;
      }
    } break;
    case TYPE_FLOAT: {
      float *ptr = (float *)(feature->ptr);
      for (uint32_t i = 0; i < feature->size; ++i) {
        output[i] = ptr[i];
      }
    } break;
    default:
      printf("Unsupported type: %u.\n", feature->type);
      break;
  }
  return;
}

inline void __attribute__((always_inline)) floatToBF16(float *input, uint16_t *output) {
  uint16_t *p = reinterpret_cast<uint16_t *>(input);
  (*output) = p[1];
}

#endif  // End of _CVI_TDL_TYPES_MEM_INTERNAL_H_
