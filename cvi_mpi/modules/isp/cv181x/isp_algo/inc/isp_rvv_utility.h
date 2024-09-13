#ifndef _ISP_RVV_UTILITY_H_
#define _ISP_RVV_UTILITY_H_

#ifdef __riscv_vector
#include <stdlib.h>
#include <riscv_vector.h>

void *rvv_memcpy(void *restrict destination, const void *restrict source, size_t n);

void *rvv_memset(void *destination, int ch, size_t n);


/**
 * @name vabs_v_i[SEW]m[LMUL] (a, vl)
 * @note SEW=8,16,32,64 LMUL=f4,f2,1,2,4,8
 *
 * @brief there are no absolute value instructions in RISC-V vector extension,
 *        so we need to implement it by ourselves.
 *
 * @example
 *	vint8m8_t a;
 *	vint8m8_t b = vabs_v_i8m8(a, vl);
 */

#define _RVVINTABS(SEW, LMUL, MLEN, T)				\
extern inline vint##SEW##m##LMUL##_t				\
__attribute__ ((__always_inline__, __gnu_inline__, __artificial__))	\
vabs_v_i##SEW##m##LMUL(vint##SEW##m##LMUL##_t a, word_type vl)		\
{									\
		vsetvl_e##SEW##m##LMUL(vl);						\
		vbool##MLEN##_t mask = vmslt_vx_i##SEW##m##LMUL##_b##MLEN(a, 0, vl); \
		return vneg_v_i##SEW##m##LMUL##_m(mask, a, a, vl);		\
}

_RVV_INT_ITERATOR(_RVVINTABS)

#endif // __riscv_vector
#endif // _ISP_RVV_UTILITY_H_
