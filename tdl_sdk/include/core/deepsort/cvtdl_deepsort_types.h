#ifndef _CVI_DEEPSORT_TYPES_H_
#define _CVI_DEEPSORT_TYPES_H_

typedef enum { L005 = 0, L010, L025, L050, L100 } mahalanobis_confidence_e;

/**
 *  Process Noise, Q, 8x8 Matrix:
 *    Q[i,j] = 0, if i != j
 *    Q[i,i] = pow(alpha[i] * X[x_idx[i]] + beta[i], 2)
 *
 *  Measurement Noise, R, 4x4 Matrix:
 *    R[i,j] = 0, if i != j
 *    R[i,i] = pow(alpha[i] * X[x_idx[i]] + beta[i], 2)
 */
typedef struct {
  bool enable_X_constraint_0;
  bool enable_X_constraint_1;
  float X_constraint_min[8];
  float X_constraint_max[8];
  bool enable_bounding_stay;
  mahalanobis_confidence_e confidence_level;
  float chi2_threshold; /* set by DeepSORT */
  // float P_constraint_min[8];
  // float P_constraint_max[8];
  /* Noise(Uncertainty) Parameter */
  float Q_alpha[8];
  float Q_beta[8];
  int Q_x_idx[8];
  float R_alpha[4];
  float R_beta[4];
  int R_x_idx[4];
} cvtdl_kalman_filter_config_t;

/**
 *  Initial Covariance, P, 8x8 Matrix:
 *    P[i,j] = 0, if i != j
 *    P[i,i] = pow(alpha[i] * X[x_idx[i]] + beta[i], 2)
 */
typedef struct {
  int max_unmatched_num;
  int accreditation_threshold;
  int feature_budget_size;
  int feature_update_interval;
  bool enable_QA_feature_init;
  bool enable_QA_feature_update;
  float feature_init_quality_threshold;
  float feature_update_quality_threshold;
  /* Noise(Uncertainty) Parameter */
  float P_alpha[8];
  float P_beta[8];
  int P_x_idx[8];
} cvtdl_kalman_tracker_config_t;

typedef struct {
  float max_distance_iou;
  float max_distance_consine;
  int max_unmatched_times_for_bbox_matching;
  bool enable_internal_FQ;
  cvtdl_kalman_filter_config_t kfilter_conf;
  cvtdl_kalman_tracker_config_t ktracker_conf;
} cvtdl_deepsort_config_t;

#endif /* _CVI_DEEPSORT_TYPES_H_ */
