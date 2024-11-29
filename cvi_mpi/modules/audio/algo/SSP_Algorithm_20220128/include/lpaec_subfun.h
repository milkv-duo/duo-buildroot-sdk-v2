
#ifndef LPAEC_SUBFUN_H
#define LPAEC_SUBFUN_H

 /* Include files */
#include "define.h"

/* Function Declarations */
extern inline void DC_filter(const short *in, float radius, float *out, int len, float *mem);
extern inline float inner_prod(const float *x, const float *y, int len);
extern inline void bin_power_spectrum(const float *X, float *ps, int N);
extern inline void accum_power_spectrum(const float *X, float *ps, int N);
extern inline void accum_spectral_mul(const float *X, const float *Y, float *acc, int N, int M);
extern inline void weighted_gradient_phi(const float *w, const float p, const float *X, const float *Y, float *prod, int N);
extern inline void aec_adjust_prop(const float *W, int N, int M, float *prop);

#endif


