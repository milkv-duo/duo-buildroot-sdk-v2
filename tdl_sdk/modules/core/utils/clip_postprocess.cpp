#include "clip_postprocess.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

namespace cvitdl {
void normalize_matrix(Eigen::MatrixXf& matrix) {
  for (int i = 0; i < matrix.rows(); ++i) {
    float norm = matrix.row(i).norm();
    if (norm != 0.0f) {
      matrix.row(i) /= norm;
    }
  }
}

int clip_postprocess(Eigen::MatrixXf& text_features, Eigen::MatrixXf& image_features,
                     Eigen::MatrixXf& result) {
  normalize_matrix(image_features);
  normalize_matrix(text_features);
  Eigen::MatrixXf text_features_transposed = text_features.transpose();
  result = 100 * image_features * text_features_transposed;
  // prods = softmax(result);

  return 0;
}
}  // namespace cvitdl