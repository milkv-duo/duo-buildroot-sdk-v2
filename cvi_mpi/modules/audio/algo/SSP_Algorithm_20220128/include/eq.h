
#ifndef EQ_H
#define EQ_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "struct.h"
#include "tmwtypes.h"

/* Function Declarations */
extern void equalizer_para(cascaded_iir_struct *spk_eq_obj, ssp_para_struct *para, float Fs);
extern void equalizer_init(cascaded_iir_struct *spk_eq_obj, float *spk_eq_state);
extern void equalizer(short *pin, short *pout, cascaded_iir_struct *eq, short frame_size);

#endif


