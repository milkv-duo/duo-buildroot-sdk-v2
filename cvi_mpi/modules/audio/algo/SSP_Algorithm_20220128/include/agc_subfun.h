
#ifndef AGC_SUBFUN_H
#define AGC_SUBFUN_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "struct.h"

/* Function Declarations */
extern short envelope(short* sin, short frame_size);
extern void update_gain(int diff, agc_struct *agc_obj);

#endif


