#pragma once

#include "core/deepsort/cvtdl_deepsort_types.h"
#include "cvi_kalman_types.hpp"

typedef Eigen::Matrix<float, 1, -1> ROW_VECTOR;

typedef enum { KF_SUCCESS = 0, KF_PREDICT_OVER_BOUNDING } kalman_filter_ret_e;

class KalmanFilter {
 public:
  KalmanFilter();
#if 0 /* deprecated */
  int predict(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_) const;
  int update(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_, const K_MEASUREMENT_V &z_) const;
  ROW_VECTOR mahalanobis(const kalman_state_e &s_, const K_STATE_V &x_, const K_COVARIANCE_M &P_,
                         const K_MEASUREMENT_M &Z_) const;
#endif
  int predict(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_,
              const cvtdl_kalman_filter_config_t &kfilter_conf) const;
  int update(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_, const K_MEASUREMENT_V &z_,
             const cvtdl_kalman_filter_config_t &kfilter_conf) const;
  ROW_VECTOR mahalanobis(const kalman_state_e &s_, const K_STATE_V &x_, const K_COVARIANCE_M &P_,
                         const K_MEASUREMENT_M &Z_,
                         const cvtdl_kalman_filter_config_t &kfilter_conf) const;
#if 0
  float mahalanobis(kalman_state_e &s_, K_STATE_V &x_, K_COVARIANCE_M &P_,
                    const K_MEASUREMENT_V &z_) const;
#endif

 private:
  K_EXTRAPOLATION_M F_;
  K_OBSERVATION_M H_;
};
