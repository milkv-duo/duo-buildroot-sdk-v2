/*
 *
 * File Name: isp_algo_vec_test.h
 * Description: test utility for riscv_vector
 *
 */

#ifndef _ISP_ALGO_VEC_TEST_
#define _ISP_ALGO_VEC_TEST_

#ifdef __riscv_vector
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <riscv_vector.h>

/**
 * @name TEST_REPORT_RUN_TIME(enable, desc, code_block)
 * @brief report run time of a code block

 * @example
 *	TEST_REPORT_RUN_TIME(true, "test run time", {
 *		for (int i = 0; i < 1000; i++) {
 *			printf("i = %d\n", i);
 *		}
 *	});
 */

#define TEST_REPORT_RUN_TIME(enable, desc, code_block) do { \
	clock_t start_time = clock(); \
	do { \
		code_block \
	} while (0); \
	if (enable) { \
		clock_t end_time = clock(); \
		static double time_taken; \
		static int time_cnt; \
		time_taken += (double)(end_time - start_time) / CLOCKS_PER_SEC * 1e6; \
		time_cnt += 1; \
		printf("[TIME] takes %.1lf us \t %s\n", time_taken / time_cnt, desc); \
	} \
} while (0)

#define DEAD_LOOP do { \
	printf("dead looping...\n"); \
	while (1) \
		;	\
} while (0)


#define TEST_EQ_VALUE(enable, desc, golden, rvv) do { \
	if ((golden) == (rvv)) { \
		if (enable) \
			printf("[TEST] <%s> \t golden: %ld, rvv: %ld, success\n", desc, (long)golden, (long)rvv); \
	} else { \
		if (enable) \
			printf("[TEST] <%s> \t golden: %ld, rvv: %ld, failed\n", desc, (long)golden, (long)rvv); \
		DEAD_LOOP; \
	} \
} while (0)


#define TEST_EQ_ARRAY(enable, desc, g, r, len) do { \
	if (memcmp((g), (r), (len) * sizeof((g)[0])) == 0) { \
		if (enable) \
			printf("[TEST] <%s> \t array golden = rvv, len: %ld, success\n", desc, (long)len); \
	} else { \
		if (enable) \
			printf("[TEST] <%s> \t array golden != rvv, len: %ld, failed\n", desc, (long)len); \
		DEAD_LOOP; \
	} \
} while (0)


#define TEST_EQ_ARRAY3(enable, desc, g1, g2, g3, r1, r2, r3, len) do { \
	if (memcmp((g1), (r1), (len) * sizeof((g1)[0])) == 0 \
	  && memcmp((g2), (r2), (len) * sizeof((g2)[0])) == 0 \
	  && memcmp((g3), (r3), (len) * sizeof((g3)[0])) == 0) { \
		if (enable) \
			printf("[TEST] <%s> \t arrays golden = rvv, len: %ld, success\n", desc, (long)len); \
	} else { \
		if (enable) \
			printf("[TEST] <%s> \t arrays golden != rvv, len: %ld, failed\n", desc, (long)len); \
		DEAD_LOOP; \
	} \
} while (0)


#define TEST_EQ_OR_LT1_ARRAY3(enable, desc, g1, g2, g3, r1, r2, r3, len) do { \
	for (int _i = 0; _i < (len); _i++) { \
		if (((g1)[_i] != (r1)[_i] && (g1)[_i] != (r1)[_i]-1) \
			&& ((g2)[_i] != (r2)[_i] && (g2)[_i] != (r2)[_i]-1) \
			&& ((g3)[_i] != (r3)[_i] && (g3)[_i] != (r3)[_i]-1)) { \
			if (enable) \
				printf("[TEST] <%s> \t golden = %d, rvv = %d\n", desc, (g1)[_i], (r1)[_i]); \
			if (enable) \
				printf("[TEST] <%s> \t arrays golden != rvv, len: %ld, failed\n", desc, (long)len); \
			DEAD_LOOP; \
		} \
	} \
	if (enable) \
		printf("[TEST] <%s> \t arrays golden = rvv or golden = rvv-1, len: %ld, success\n", desc, (long)len); \
} while (0)


/**
 * @name TEST_PRINT_i[SEW]m[LMUL] (desc, v, vl)
 * @name TEST_PRINT_u[SEW]m[LMUL] (desc, v, vl)
 * @note SEW=8,16,32,64 LMUL=f4,f2,1,2,4,8
 *
 * @brief print vector to screen
 *
 * @example
 *	vint8m8_t v;
 *	TEST_PRINT_i8m8("desc for v", v, vl);
 *	vuint32m4_t v;
 *	TEST_PRINT_u32m4("desc for v", v, vl);
 */

#define _TEST_RVVINT_PRINT(SEW, LMUL, MLEN, T)				\
extern inline void			\
__attribute__ ((__always_inline__, __gnu_inline__, __artificial__))	\
TEST_PRINT_i##SEW##m##LMUL(char *desc, vint##SEW##m##LMUL##_t v, size_t vl)	\
{									\
	int##SEW##_t arr[vl];						\
	vse##SEW##_v_i##SEW##m##LMUL(arr, v, vl);				\
	printf("printing %s, len: %ld\n", desc, vl);						\
	for (size_t i = 0; i < vl; i++)					\
		printf("%ld ", (int64_t)arr[i]);						\
	printf("\n");							\
}					\
extern inline void			\
__attribute__ ((__always_inline__, __gnu_inline__, __artificial__))	\
TEST_PRINT_u##SEW##m##LMUL(char *desc, vuint##SEW##m##LMUL##_t v, size_t vl)	\
{									\
	uint##SEW##_t arr[vl];						\
	vse##SEW##_v_u##SEW##m##LMUL(arr, v, vl);				\
	printf("printing %s, len: %ld\n", desc, vl);						\
	for (size_t i = 0; i < vl; i++)					\
		printf("%ld ", (uint64_t)arr[i]);						\
	printf("\n");							\
}

_RVV_INT_ITERATOR(_TEST_RVVINT_PRINT)
#endif // __riscv_vector
#endif // _ISP_ALGO_VEC_TEST_
