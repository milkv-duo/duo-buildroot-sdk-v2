/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_algo_interface.h
 * Description: basic audio algorithm interface
 */
#ifndef CVI_ALGO_INTERFACE_H
#define CVI_ALGO_INTERFACE_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include <linux/cvi_type.h>
#include "rtwtypes.h"
#include "mmse_types.h"
#include "mmse_emxutil.h"
#include "mmse_init.h"
#include "mmse.h"
#include "define.h"
#include "agc_init.h"
#include "struct.h"
#include "agc.h"

/* old declaration of nr algorithm */
CVI_VOID speech_nr_algo_init(CVI_FLOAT fsamplerate);
CVI_VOID speech_nr_algo_deinit(void);
CVI_VOID speech_nr_algo(CVI_CHAR **Wpt, FILE *fp_input, FILE *fp_output);
/* algo_ver20200110 do not use this api */
/* CVI_VOID speech_nr_algo2(CVI_CHAR **Wpt, FILE *fp_input, FILE *fp_output, CVI_S32 size); */
/* new declaration of nr, agc algorithm */
void speech_algo_init(CVI_FLOAT fsamplerate, para_struct *agc_config, CVI_FLOAT *initial_noise_pow_time);
void speech_algo_agc_init(agc_struct *agc_obj,
	para_struct *agc_config,
	CVI_FLOAT fsamplerate);
CVI_VOID speech_algo(CVI_CHAR **Wpt, FILE *fp_input, FILE *fp_output,
	CVI_S32 size, CVI_S32 iNrOn, CVI_S32 iAgcOn, agc_struct *pagc_obj,
	CVI_FLOAT initial_noise_pow_time);
CVI_VOID speech_algo_deinit(void);

#endif


