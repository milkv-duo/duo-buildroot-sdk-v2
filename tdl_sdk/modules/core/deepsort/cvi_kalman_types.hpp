#pragma once

#include <vector>
#include "cvi_tracker.hpp"

#define DIM_X 8
#define DIM_Z 4

#define STD_XP_0 (float)(1.0 / 20.0)
#define STD_XP_1 (float)(1.0 / 160.0)

typedef enum { PREDICTED = 0, UPDATED } kalman_state_e;

typedef Eigen::Matrix<float, DIM_X, 1> K_VECTOR;
typedef Eigen::Matrix<float, DIM_Z, 1> K_VECTOR_Z;
typedef Eigen::Matrix<float, DIM_X, DIM_X> K_MATRIX;
typedef Eigen::Matrix<float, DIM_Z, DIM_Z> K_MATRIX_Z_Z;
typedef Eigen::Matrix<float, DIM_X, DIM_Z> K_MATRIX_X_Z;
typedef Eigen::Matrix<float, DIM_Z, DIM_X> K_MATRIX_Z_X;

typedef Eigen::Matrix<float, DIM_Z, 1> K_MEASUREMENT_V;
typedef Eigen::Matrix<float, -1, DIM_Z> K_MEASUREMENT_M;
typedef Eigen::Matrix<float, DIM_X, 1> K_STATE_V;
typedef Eigen::Matrix<float, DIM_X, DIM_X> K_COVARIANCE_M;
typedef Eigen::Matrix<float, DIM_X, DIM_X> K_EXTRAPOLATION_M;
typedef Eigen::Matrix<float, DIM_Z, DIM_X> K_OBSERVATION_M;

typedef Eigen::Matrix<float, DIM_X, DIM_X> K_PROCESS_NOISE_M;
typedef Eigen::Matrix<float, DIM_Z, DIM_Z> K_MEASUREMENT_NOISE_M;
