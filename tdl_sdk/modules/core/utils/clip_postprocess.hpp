#pragma once
#include "Eigen/Core"
#include "unsupported/Eigen/FFT"

namespace cvitdl {
void normalize_matrix(Eigen::MatrixXf& matrix);
Eigen::MatrixXf softmax(const Eigen::MatrixXf& input);
int clip_postprocess(Eigen::MatrixXf& text_features, Eigen::MatrixXf& image_features,
                     Eigen::MatrixXf& prods);
}  // namespace cvitdl