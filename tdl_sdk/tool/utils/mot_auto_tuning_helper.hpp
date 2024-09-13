#pragma once

#include <vector>
#include "mot_evaluation.hpp"

typedef struct {
  double min_coverage_rate;
  // double max_avg_entropy;
} MOT_PERFORMANCE_CONSTRAINT_t;

typedef struct {
#if 0 /* too much combination */
  std::vector<float> P_a[8];
  std::vector<float> P_b[8];
  std::vector<float> Q_a[8];
  std::vector<float> Q_b[8];
  std::vector<float> R_a[4];
  std::vector<float> R_b[4];
#endif
  std::vector<float> P_a_013;
  std::vector<float> P_a_2;
  std::vector<float> P_a_457;
  std::vector<float> P_a_6;
  std::vector<float> P_b_013;
  std::vector<float> P_b_2;
  std::vector<float> P_b_457;
  std::vector<float> P_b_6;
  std::vector<float> Q_a_013;
  std::vector<float> Q_a_2;
  std::vector<float> Q_a_457;
  std::vector<float> Q_a_6;
  std::vector<float> Q_b_013;
  std::vector<float> Q_b_2;
  std::vector<float> Q_b_457;
  std::vector<float> Q_b_6;
  std::vector<float> R_a_013;
  std::vector<float> R_a_2;
  std::vector<float> R_b_013;
  std::vector<float> R_b_2;
  std::vector<float> max_IOU_distance;
  std::vector<int> max_unmatched;
  std::vector<int> thr_accreditation;
} MOT_GRID_SEARCH_PARAMS_t;

/* helper functions (core) */
CVI_S32 OPTIMIZE_CONFIG_1(cvitdl_handle_t tdl_handle, const MOT_EVALUATION_ARGS_t &args,
                          const MOT_GRID_SEARCH_PARAMS_t &params,
                          const MOT_PERFORMANCE_CONSTRAINT_t &constraint,
                          cvtdl_deepsort_config_t &config, MOT_Performance_t &performance);
// CVI_S32 OPTIMIZE_CONFIG_2(cvitdl_handle_t tdl_handle, const MOT_EVALUATION_ARGS_t &args);

/* helper functions */
CVI_S32 GET_PREDEFINED_OPT_1_PARAMS(MOT_GRID_SEARCH_PARAMS_t &params);
CVI_S32 GET_CONFIG_BY_PARAMS_1(cvtdl_deepsort_config_t &config,
                               const MOT_GRID_SEARCH_PARAMS_t &params,
                               const std::vector<uint32_t> &idxes);