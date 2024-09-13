#include "mot_auto_tuning_helper.hpp"

/* Tracker state X apply [xyah] coordinate system */
CVI_S32 GET_PREDEFINED_OPT_1_PARAMS(MOT_GRID_SEARCH_PARAMS_t &params) {
#if 0 /* too much combination */
  params.P_a[0] = {0.05, 0.1, 0.2};
  params.P_a[1] = {0.05, 0.1, 0.2};
  params.P_a[2] = {0.0};
  params.P_a[3] = {0.05, 0.1, 0.2};
  params.P_a[4] = {0.0625, 0.125};
  params.P_a[5] = {0.0625, 0.125};
  params.P_a[6] = {0.0};
  params.P_a[7] = {0.0625, 0.125};

  params.P_b[0] = {0.0};
  params.P_b[1] = {0.0};
  params.P_b[2] = {0.01, 0.1};
  params.P_b[3] = {0.0};
  params.P_b[4] = {0.0};
  params.P_b[5] = {0.0};
  params.P_b[6] = {1e-5, 2.5e-2};
  params.P_b[7] = {0.0};

  params.Q_a[0] = {0.05, 0.1, 0.2};
  params.Q_a[1] = {0.05, 0.1, 0.2};
  params.Q_a[2] = {0.0};
  params.Q_a[3] = {0.05, 0.1, 0.2};
  params.Q_a[4] = {0.0125, 0.0625, 0.125};
  params.Q_a[5] = {0.0125, 0.0625, 0.125};
  params.Q_a[6] = {0.0};
  params.Q_a[7] = {0.0125, 0.0625, 0.125};

  params.Q_b[0] = {0.0};
  params.Q_b[1] = {0.0};
  params.Q_b[2] = {0.01, 0.1};
  params.Q_b[3] = {0.0};
  params.Q_b[4] = {0.0};
  params.Q_b[5] = {0.0};
  params.Q_b[6] = {1e-5, 2.5e-2};
  params.Q_b[7] = {0.0};

  params.R_a[0] = {0.05, 0.1, 0.2};
  params.R_a[1] = {0.05, 0.1, 0.2};
  params.R_a[2] = {0.0};
  params.R_a[3] = {0.05, 0.1, 0.2};

  params.R_b[0] = {0.0};
  params.R_b[1] = {0.0};
  params.R_b[2] = {0.01, 0.1};
  params.R_b[3] = {0.0};
#endif

#if 0
  params.P_a_013 = {0.05, 0.1, 0.2};
  params.P_a_2 = {0.0};
  params.P_a_457 = {0.0625, 0.125};
  params.P_a_6 = {0.0};
  params.P_b_013 = {0.0};
  params.P_b_2 = {0.01, 0.1};
  params.P_b_457 = {0.0};
  params.P_b_6 = {1e-5, 2.5e-2};

  params.Q_a_013 = {0.05, 0.1, 0.2};
  params.Q_a_2 = {0.0};
  params.Q_a_457 = {0.0125, 0.0625, 0.125};
  params.Q_a_6 = {0.0};
  params.Q_b_013 = {0.0};
  params.Q_b_2 = {0.01, 0.1};
  params.Q_b_457 = {0.0};
  params.Q_b_6 = {1e-5, 2.5e-2};

  params.R_a_013 = {0.05, 0.1, 0.2};
  params.R_a_2 = {0.0};
  params.R_b_013 = {0.0};
  params.R_b_2 = {0.01, 0.1};

  params.max_IOU_distance = {0.7, 0.8};
  params.max_unmatched = {10, 20};
  params.thr_accreditation = {5, 10};
#endif
  params.P_a_013 = {0.05, 0.2};
  params.P_a_2 = {0.0};
  params.P_a_457 = {0.0625, 0.125};
  params.P_a_6 = {0.0};
  params.P_b_013 = {0.0};
  params.P_b_2 = {0.01, 0.1};
  params.P_b_457 = {0.0};
  params.P_b_6 = {1e-5, 2.5e-2};

  params.Q_a_013 = {0.1, 0.2};
  params.Q_a_2 = {0.0};
  params.Q_a_457 = {0.0125, 0.0625};
  params.Q_a_6 = {0.0};
  params.Q_b_013 = {0.0};
  params.Q_b_2 = {0.01, 0.1};
  params.Q_b_457 = {0.0};
  params.Q_b_6 = {1e-5, 2.5e-2};

  params.R_a_013 = {0.1, 0.2};
  params.R_a_2 = {0.0};
  params.R_b_013 = {0.0};
  params.R_b_2 = {0.01, 0.1};

  params.max_IOU_distance = {0.7};
  params.max_unmatched = {10};
  params.thr_accreditation = {5};

  return CVI_SUCCESS;
}

CVI_S32 OPTIMIZE_CONFIG_1(cvitdl_handle_t tdl_handle, const MOT_EVALUATION_ARGS_t &args,
                          const MOT_GRID_SEARCH_PARAMS_t &params,
                          const MOT_PERFORMANCE_CONSTRAINT_t &constraint,
                          cvtdl_deepsort_config_t &config, MOT_Performance_t &performance) {
  std::vector<uint32_t> p_ranges(
      {(uint32_t)params.P_a_013.size(),          (uint32_t)params.P_a_2.size(),
       (uint32_t)params.P_a_457.size(),          (uint32_t)params.P_a_6.size(),
       (uint32_t)params.P_b_013.size(),          (uint32_t)params.P_b_2.size(),
       (uint32_t)params.P_b_457.size(),          (uint32_t)params.P_b_6.size(),
       (uint32_t)params.Q_a_013.size(),          (uint32_t)params.Q_a_2.size(),
       (uint32_t)params.Q_a_457.size(),          (uint32_t)params.Q_a_6.size(),
       (uint32_t)params.Q_b_013.size(),          (uint32_t)params.Q_b_2.size(),
       (uint32_t)params.Q_b_457.size(),          (uint32_t)params.Q_b_6.size(),
       (uint32_t)params.R_a_013.size(),          (uint32_t)params.R_a_2.size(),
       (uint32_t)params.R_b_013.size(),          (uint32_t)params.R_b_2.size(),
       (uint32_t)params.max_IOU_distance.size(), (uint32_t)params.max_unmatched.size(),
       (uint32_t)params.thr_accreditation.size()});
  GridIndexGenerator idx_generator(p_ranges);
  std::vector<uint32_t> idx;
  cvtdl_deepsort_config_t tmp_config;
  MOT_Performance_t tmp_performance;
  while (CVI_SUCCESS == idx_generator.next(idx) /* condition */) {
    GET_CONFIG_BY_PARAMS_1(tmp_config, params, idx);
    CVI_TDL_DeepSORT_Init(tdl_handle, false);
    CVI_TDL_DeepSORT_SetConfig(tdl_handle, &tmp_config, -1, false);
    RUN_MOT_EVALUATION(tdl_handle, args, tmp_performance, false, NULL);
    printf("iter[%u]: score[%.4lf] (coverage rate[%.4lf], stable ids[%u], total entropy[%.4lf])\n",
           idx_generator.counter, tmp_performance.score, tmp_performance.coverage_rate,
           tmp_performance.stable_id_num, tmp_performance.total_entropy);
    if (tmp_performance.coverage_rate < constraint.min_coverage_rate) {
      continue;
    }
    if (tmp_performance.score < performance.score) {
      performance = tmp_performance;
      config = tmp_config;
    }
  }

  return CVI_SUCCESS;
}

CVI_S32 GET_CONFIG_BY_PARAMS_1(cvtdl_deepsort_config_t &config,
                               const MOT_GRID_SEARCH_PARAMS_t &params,
                               const std::vector<uint32_t> &idxes) {
  CVI_TDL_DeepSORT_GetDefaultConfig(&config);
  config.ktracker_conf.P_alpha[0] = params.P_a_013[idxes[0]];
  config.ktracker_conf.P_alpha[1] = params.P_a_013[idxes[0]];
  config.ktracker_conf.P_alpha[2] = params.P_a_2[idxes[1]];
  config.ktracker_conf.P_alpha[3] = params.P_a_013[idxes[0]];
  config.ktracker_conf.P_alpha[4] = params.P_a_013[idxes[2]];
  config.ktracker_conf.P_alpha[5] = params.P_a_013[idxes[2]];
  config.ktracker_conf.P_alpha[6] = params.P_a_2[idxes[3]];
  config.ktracker_conf.P_alpha[7] = params.P_a_013[idxes[2]];
  config.ktracker_conf.P_beta[0] = params.P_b_013[idxes[4]];
  config.ktracker_conf.P_beta[1] = params.P_b_013[idxes[4]];
  config.ktracker_conf.P_beta[2] = params.P_b_2[idxes[5]];
  config.ktracker_conf.P_beta[3] = params.P_b_013[idxes[4]];
  config.ktracker_conf.P_beta[4] = params.P_b_013[idxes[6]];
  config.ktracker_conf.P_beta[5] = params.P_b_013[idxes[6]];
  config.ktracker_conf.P_beta[6] = params.P_b_2[idxes[7]];
  config.ktracker_conf.P_beta[7] = params.P_b_013[idxes[6]];

  config.kfilter_conf.Q_alpha[0] = params.Q_a_013[idxes[8]];
  config.kfilter_conf.Q_alpha[1] = params.Q_a_013[idxes[8]];
  config.kfilter_conf.Q_alpha[2] = params.Q_a_2[idxes[9]];
  config.kfilter_conf.Q_alpha[3] = params.Q_a_013[idxes[8]];
  config.kfilter_conf.Q_alpha[4] = params.Q_a_013[idxes[10]];
  config.kfilter_conf.Q_alpha[5] = params.Q_a_013[idxes[10]];
  config.kfilter_conf.Q_alpha[6] = params.Q_a_2[idxes[11]];
  config.kfilter_conf.Q_alpha[7] = params.Q_a_013[idxes[10]];
  config.kfilter_conf.Q_beta[0] = params.Q_b_013[idxes[12]];
  config.kfilter_conf.Q_beta[1] = params.Q_b_013[idxes[12]];
  config.kfilter_conf.Q_beta[2] = params.Q_b_2[idxes[13]];
  config.kfilter_conf.Q_beta[3] = params.Q_b_013[idxes[12]];
  config.kfilter_conf.Q_beta[4] = params.Q_b_013[idxes[14]];
  config.kfilter_conf.Q_beta[5] = params.Q_b_013[idxes[14]];
  config.kfilter_conf.Q_beta[6] = params.Q_b_2[idxes[15]];
  config.kfilter_conf.Q_beta[7] = params.Q_b_013[idxes[14]];

  config.kfilter_conf.R_alpha[0] = params.R_a_013[idxes[16]];
  config.kfilter_conf.R_alpha[1] = params.R_a_013[idxes[16]];
  config.kfilter_conf.R_alpha[2] = params.R_a_2[idxes[17]];
  config.kfilter_conf.R_alpha[3] = params.R_a_013[idxes[16]];
  config.kfilter_conf.R_beta[0] = params.R_b_013[idxes[18]];
  config.kfilter_conf.R_beta[1] = params.R_b_013[idxes[18]];
  config.kfilter_conf.R_beta[2] = params.R_b_2[idxes[19]];
  config.kfilter_conf.R_beta[3] = params.R_b_013[idxes[18]];

  config.max_distance_iou = params.max_IOU_distance[idxes[20]];
  config.ktracker_conf.max_unmatched_num = params.max_unmatched[idxes[21]];
  config.ktracker_conf.accreditation_threshold = params.thr_accreditation[idxes[22]];

  return CVI_SUCCESS;
}