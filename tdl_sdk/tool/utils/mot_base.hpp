#pragma once
#include "cvi_tdl.h"

typedef enum { FACE = 0, PERSON, VEHICLE, PET } TARGET_TYPE_e;

CVI_S32 GET_TARGET_TYPE(TARGET_TYPE_e *target_type, char *text);
CVI_S32 GET_PREDEFINED_CONFIG(cvtdl_deepsort_config_t *ds_conf, TARGET_TYPE_e target_type);