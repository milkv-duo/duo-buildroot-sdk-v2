
#ifndef POWER_H
#define POWER_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "tmwtypes.h"

/* Function Declarations */
extern void c_power(const float a[], float y[], int N);
extern void d_power(const float a[321], float y[321]);

#endif


