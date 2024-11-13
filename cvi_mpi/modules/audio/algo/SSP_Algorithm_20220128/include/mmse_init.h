
#ifndef MMSE_INIT_H
#define MMSE_INIT_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "tmwtypes.h"
#include "struct.h"

/* Function Declarations */
extern void NR_para(ssp_para_struct *para, float *aa, float *mu);
extern NRState *NR_init(int frame_size, float fs, float aa, float mu);
extern void NR_free(NRState *st);

#endif


