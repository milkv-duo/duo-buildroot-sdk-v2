#pragma once

#include <Eigen/Eigen>
#include <vector>
#include "core/cvi_tdl_core.h"
#include "core/deepsort/cvtdl_deepsort_types.h"
typedef Eigen::Matrix<float, 1, 4> BBOX;
typedef Eigen::Matrix<float, -1, 4> BBOXES;
typedef Eigen::Matrix<float, 1, -1> FEATURE;
typedef Eigen::Matrix<float, -1, -1> FEATURES;

typedef enum {
  Feature_CosineDistance = 0,
  Kalman_MahalanobisDistance,
  BBox_IoUDistance
} cost_matrix_algo_e;

struct stRect {
  float x;
  float y;
  float width;
  float height;
  stRect(float x_, float y_, float w_, float h_) : x(x_), y(y_), width(w_), height(h_){};
  stRect(){};
};

typedef struct _stObjInfo {
  int src_idx;
  cvtdl_bbox_t box;
  uint64_t track_id;
  int classes;
  int pair_obj_id;
  int pair_type;
} stObjInfo;

typedef struct _stCorrelateInfo {
  float offset_scale_x;
  float offset_scale_y;
  float pair_size_scale_x;
  float pair_size_scale_y;
  int votes;
  int time_since_correlated;

} stCorrelateInfo;

enum ObjectType {
  OBJ_CAR,
  OBJ_BUS,
  OBJ_TRUCK,
  OBJ_CARPLATE,
  OBJ_PERSON,
  // OBJ_BIKE,
  // OBJ_MOTOR,
  OBJ_TRICYCLE,  // 5
  OBJ_FACE,
  OBJ_HEAD,         // 7
  OBJ_RIDER,        // 8
  OBJ_NON_VEHICLE,  // 9
  OBJ_RIDER_MOTOR,  //(BIKE OR MOTOR)+RIDER
  OBJ_TYPE_ERROR,   // 11
  OBJ_NONE
};
