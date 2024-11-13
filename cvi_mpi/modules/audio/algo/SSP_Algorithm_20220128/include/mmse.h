
#ifndef MMSE_H
#define MMSE_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "tmwtypes.h"
#include "struct.h"

/* Function Declarations */
extern void NR(short *x, float initial_noise_pow_time, float *frame, NRState *st, boolean_T *speech_vad, float *y);

#endif


