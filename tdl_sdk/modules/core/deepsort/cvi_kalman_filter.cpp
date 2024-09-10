#include "cvi_kalman_filter.hpp"
#include <math.h>
#include <cassert>

#include <iostream>
#include "cvi_tdl_log.hpp"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define BOUNDING_STAY_ALGO 1
#define BOUNDING_STAY_ALGO_1_THRESHOLD 0.7

#define DEBUG_KALMAN_FILTER_UPDATE 0
#define DEBUG_KALMAN_FILTER_MAHALANOBIS 0

KalmanFilter::KalmanFilter() {
  F_ = Eigen::MatrixXf::Identity(DIM_X, DIM_X);
  F_.topRightCorner(DIM_Z, DIM_Z) = Eigen::MatrixXf::Identity(DIM_Z, DIM_Z);
  H_ = Eigen::MatrixXf::Identity(DIM_Z, DIM_X);
}

int KalmanFilter::predict(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_,
                          const cvtdl_kalman_filter_config_t &kfilter_conf) const {
  if (s_ != kalman_state_e::UPDATED) {
    LOGE("kalman_state_e should be %d, but got %d\n", kalman_state_e::UPDATED, s_);
    return -1;
  }
  s_ = kalman_state_e::PREDICTED;
  /* generate process noise, Q */
  K_PROCESS_NOISE_M Q_ = Eigen::MatrixXf::Zero(DIM_X, DIM_X);
  for (int i = 0; i < DIM_X; i++) {
    float X_base = (kfilter_conf.Q_x_idx[i] == -1) ? 0.0 : x_[kfilter_conf.Q_x_idx[i]];
    Q_(i, i) = pow(kfilter_conf.Q_alpha[i] * X_base + kfilter_conf.Q_beta[i], 2);
  }
  /* predict */
  K_STATE_V next_x_ = F_ * x_;
  P_ = F_ * P_ * F_.transpose() + Q_;
  //   if (kfilter_conf.enable_bounding_stay) {
  // #if BOUNDING_STAY_ALGO == 0
  //     if (next_x_(0) < kfilter_conf.X_constraint_min[0] ||
  //         next_x_(1) < kfilter_conf.X_constraint_min[1] ||
  //         next_x_(0) > kfilter_conf.X_constraint_max[0] ||
  //         next_x_(1) > kfilter_conf.X_constraint_max[1]) {
  //       return kalman_filter_ret_e::KF_PREDICT_OVER_BOUNDING;
  //     }
  // #elif BOUNDING_STAY_ALGO == 1
  //     float w = next_x_(2) * next_x_(3);
  //     float h = next_x_(3);
  //     float area = w * h;
  //     float x1 = MAX(kfilter_conf.X_constraint_min[0], next_x_(0) - 0.5 * w);
  //     float y1 = MAX(kfilter_conf.X_constraint_min[1], next_x_(1) - 0.5 * h);
  //     float x2 = MIN(kfilter_conf.X_constraint_max[0], next_x_(0) + 0.5 * w);
  //     float y2 = MIN(kfilter_conf.X_constraint_max[1], next_x_(1) + 0.5 * h);
  //     float area_clipped = (x2 - x1) * (y2 - y1);
  //     float r = area_clipped / area;
  //     // printf("r[%.4f], area[%.4f], area_clipped[%.4f]\n", r, area, area_clipped);
  //     if (r < BOUNDING_STAY_ALGO_1_THRESHOLD) {
  //       // printf("x  = (%.4f,%.4f,%.4f,%.4f)\n", x_(0), x_(1), x_(2), x_(3));
  //       // printf("x' = (%.4f,%.4f,%.4f,%.4f)\n", x_(4), x_(5), x_(6), x_(7));
  //       return kalman_filter_ret_e::KF_PREDICT_OVER_BOUNDING;
  //     }
  // #else
  // #error "unknown bounding stay algorithm"
  // #endif
  //   }
  x_ = next_x_;
  if (kfilter_conf.enable_X_constraint_0) {
    for (int i = 0; i < DIM_Z; i++) {
      x_(i) = MAX(x_(i), kfilter_conf.X_constraint_min[i]);
      x_(i) = MIN(x_(i), kfilter_conf.X_constraint_max[i]);
    }
  }
  if (kfilter_conf.enable_X_constraint_1) {
    for (int i = DIM_Z; i < DIM_X; i++) {
      x_(i) = MAX(x_(i), kfilter_conf.X_constraint_min[i]);
      x_(i) = MIN(x_(i), kfilter_conf.X_constraint_max[i]);
    }
  }
  return kalman_filter_ret_e::KF_SUCCESS;
}

int KalmanFilter::update(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_,
                         const K_MEASUREMENT_V &z_,
                         const cvtdl_kalman_filter_config_t &kfilter_conf) const {
  if (s_ != kalman_state_e::PREDICTED) {
    LOGE("kalman_state_e should be %d, but got %d\n", kalman_state_e::PREDICTED, s_);
    return -1;
  }
  /* Compute the Kalman Gain : K = P * H^t * (H * P * H^t + R)^(-1) */
  /* generate measurement noise, R */
  K_MATRIX_Z_Z R_ = Eigen::MatrixXf::Zero(DIM_Z, DIM_Z);
  for (int i = 0; i < DIM_Z; i++) {
    float X_base = (kfilter_conf.R_x_idx[i] == -1) ? 0.0 : x_[kfilter_conf.R_x_idx[i]];
    R_(i, i) = pow(kfilter_conf.R_alpha[i] * X_base + kfilter_conf.R_beta[i], 2);
  }
  // K_MATRIX_Z_Z HPHt_ = H_ * P_ * H_.transpose();
  K_MATRIX_Z_Z HPHt_ = P_.block(0, 0, DIM_Z, DIM_Z);
  HPHt_ = HPHt_ + R_;

  // K_MATRIX_X_Z PHt_ = P_ * H_.transpose();
  K_MATRIX_X_Z PHt_ = P_.block(0, 0, DIM_X, DIM_Z);

  // K_MATRIX_X_Z K_ = PHt_ * HPHt_.inverse();
  K_MATRIX_X_Z K_ = HPHt_.llt().solve(PHt_.transpose()).transpose();

  /* Update estimate with measurement : x = x + K * (z - H * x) */
  // K_MEASUREMENT_V Hx_ = H_ * x_;
  K_MEASUREMENT_V Hx_ = x_.block(0, 0, DIM_Z, 1);
  x_ = x_ + K_ * (z_ - Hx_);

  /* Update the estimate uncertainty : P = (I - K * H) * P */
  P_ = P_ - K_ * P_.block(0, 0, DIM_Z, DIM_X).matrix();

  s_ = kalman_state_e::UPDATED;
  return 0;
}

ROW_VECTOR KalmanFilter::mahalanobis(const kalman_state_e &s_, const K_STATE_V &x_,
                                     const K_COVARIANCE_M &P_, const K_MEASUREMENT_M &Z_,
                                     const cvtdl_kalman_filter_config_t &kfilter_conf) const {
  K_MATRIX_Z_Z Cov_ = P_.block(0, 0, DIM_Z, DIM_Z);
  K_MATRIX_Z_Z R_ = Eigen::MatrixXf::Zero(DIM_Z, DIM_Z);
  for (int i = 0; i < DIM_Z; i++) {
    float X_base = (kfilter_conf.R_x_idx[i] == -1) ? 0.0 : x_[kfilter_conf.R_x_idx[i]];
    R_(i, i) = pow(kfilter_conf.R_alpha[i] * X_base + kfilter_conf.R_beta[i], 2);
  }
  Cov_ += R_;

  K_MEASUREMENT_V Hx_ = x_.block(0, 0, DIM_Z, 1);

  K_MEASUREMENT_M Diff_ = Z_.rowwise() - Hx_.transpose();

  /*
    d = x^t * S^(-1) * x
      = x^t * (L * L^t)^(-1) * x
      = x^t * (L^t)^(-1) * L^(-1) * x
      = (L^(-1) * x)^t * (L^(-1) * x)
      = M^t * M
  */
  Eigen::Matrix<float, -1, -1> L_ = Cov_.llt().matrixL();
  Eigen::Matrix<float, -1, -1> M_ =
      L_.triangularView<Eigen::Lower>().solve<Eigen::OnTheRight>(Diff_).transpose();

  auto M_2_ = ((M_.array()) * (M_.array())).matrix();
  auto maha2_distance = M_2_.colwise().sum();

  return maha2_distance;
}
