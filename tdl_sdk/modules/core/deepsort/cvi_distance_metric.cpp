#include "cvi_distance_metric.hpp"

#if 0
  // TODO: Implement functions (Not nessesary)
  void normalize_feature(FEATURE &a);
  float cosine_distance(const FEATURE &a, const FEATURE &b);
  COST_VECTOR cosine_distance(const FEATURE &a, const FEATURES &B);
  COST_MATRIX cosine_distance(const FEATURES &A, const FEATURES &B);
  Eigen::VectorXf get_min_rowwise(Eigen::MatrixXf &M);
#endif

void normalize_feature(FEATURE &a) { a = a / a.norm(); }

COST_MATRIX cosine_distance(const FEATURES &A_norm, const FEATURES &B_norm) {
  COST_MATRIX cost_m = (0.5 * (1 - (A_norm * B_norm.transpose()).array())).matrix();
  return cost_m;
}

COST_VECTOR iou_distance(const BBOX &a, const BBOXES &B) {
  COST_VECTOR cost_v;
  POINT_V a_TL = a.leftCols(2);
  POINT_V a_BR = a_TL + a.rightCols(2);
  float a_area = a(0, 2) * a(0, 3);
  POINTS_M B_TL = B.leftCols(2);
  POINTS_M B_BR = B_TL + B.rightCols(2);
  Eigen::VectorXf B_area = B.rightCols(2).rowwise().prod();

  POINTS_M inter_TL(B.rows(), 2);
  POINTS_M inter_BR(B.rows(), 2);
  for (int i = 0; i < B.rows(); i++) {
    inter_TL.row(i) = B_TL.row(i).array().max(a_TL.array()).matrix();
    inter_BR.row(i) = B_BR.row(i).array().min(a_BR.array()).matrix();
  }
  POINTS_M inter_WH = inter_BR - inter_TL;
  inter_WH = inter_WH.array().max(0.0);
  Eigen::VectorXf inter_area = inter_WH.rowwise().prod();
  Eigen::VectorXf union_area = Eigen::VectorXf::Constant(B.rows(), a_area) + B_area - inter_area;
  cost_v = (1.0 - (inter_area.array() / union_area.array())).matrix();

  return cost_v;
}

void restrict_cost_matrix(COST_MATRIX &M, float upper_bound) {
  for (int i = 0; i < M.rows(); i++) {
    for (int j = 0; j < M.cols(); j++) {
      if (M(i, j) > upper_bound) {
        M(i, j) = upper_bound;
      }
    }
  }
}

ROW_VECTOR get_min_colwise(COST_MATRIX &M) {
  ROW_VECTOR min_v = M.colwise().minCoeff();
  return min_v;
}

COL_VECTOR get_min_rowwise(COST_MATRIX &M) {
  COL_VECTOR min_v = M.rowwise().minCoeff();
  return min_v;
}