#pragma once

#include "cvi_deepsort_types_internal.hpp"

typedef Eigen::Matrix<float, 1, -1> ROW_VECTOR;
typedef Eigen::Matrix<float, -1, 1> COL_VECTOR;

typedef Eigen::Matrix<float, -1, 1> COST_VECTOR;
typedef Eigen::Matrix<float, -1, -1> COST_MATRIX;

typedef Eigen::Matrix<float, 1, 2> POINT_V;
typedef Eigen::Matrix<float, -1, 2> POINTS_M;

void normalize_feature(FEATURE &a);
float cosine_distance(const FEATURE &a, const FEATURE &b);
COST_VECTOR cosine_distance(const FEATURE &a, const FEATURES &B);
COST_MATRIX cosine_distance(const FEATURES &A, const FEATURES &B);

COST_VECTOR iou_distance(const BBOX &a, const BBOXES &B);

void restrict_cost_matrix(COST_MATRIX &M, float upper_bound);

ROW_VECTOR get_min_colwise(COST_MATRIX &M);
COL_VECTOR get_min_rowwise(COST_MATRIX &M);
