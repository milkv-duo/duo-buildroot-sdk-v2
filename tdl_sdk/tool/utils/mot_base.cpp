#include "mot_base.hpp"

CVI_S32 GET_TARGET_TYPE(TARGET_TYPE_e *target_type, char *text) {
  if (!strcmp(text, "face")) {
    *target_type = FACE;
  } else if (!strcmp(text, "person")) {
    *target_type = PERSON;
  } else if (!strcmp(text, "vehicle")) {
    *target_type = VEHICLE;
  } else if (!strcmp(text, "pet")) {
    *target_type = PET;
  } else {
    printf("unknow target type: %s\n", text);
    return CVI_FAILURE;
  }
  return CVI_SUCCESS;
}

CVI_S32 GET_PREDEFINED_CONFIG(cvtdl_deepsort_config_t *ds_conf, TARGET_TYPE_e target_type) {
  CVI_TDL_DeepSORT_GetDefaultConfig(ds_conf);
  switch (target_type) {
    case PERSON:
      /** default config
       *  P_alpha[0] = 2 * 1 / 20.;
       *  P_alpha[1] = 2 * 1 / 20.;
       *  P_alpha[3] = 2 * 1 / 20.;
       *  P_alpha[4] = 10 * 1 / 160.;
       *  P_alpha[5] = 10 * 1 / 160.;
       *  P_alpha[7] = 10 * 1 / 160.;
       *  P_beta[2] = 0.01;
       *  P_beta[6] = 1e-5;
       *  Q_beta[2] = 0.01;
       *  Q_beta[6] = 1e-5;
       *  R_beta[2] = 0.1;
       */
      break;
    case FACE: {
      ds_conf->ktracker_conf.max_unmatched_num = 10;
      ds_conf->ktracker_conf.accreditation_threshold = 10;
      ds_conf->ktracker_conf.P_beta[2] = 0.1;
      ds_conf->ktracker_conf.P_beta[6] = 2.5e-2;
      ds_conf->kfilter_conf.Q_beta[2] = 0.1;
      ds_conf->kfilter_conf.Q_beta[6] = 2.5e-2;
    } break;
    case VEHICLE: {
      ds_conf->ktracker_conf.max_unmatched_num = 20;
    } break;
    case PET: {
      ds_conf->ktracker_conf.max_unmatched_num = 30;
      ds_conf->ktracker_conf.accreditation_threshold = 5;
      ds_conf->ktracker_conf.P_beta[2] = 0.1;
      ds_conf->ktracker_conf.P_beta[6] = 2.5 * 1e-2;
      ds_conf->kfilter_conf.Q_beta[2] = 0.1;
      ds_conf->kfilter_conf.Q_beta[6] = 2.5 * 1e-2;
    } break;
    default:
      return CVI_FAILURE;
  }
  return CVI_SUCCESS;
}