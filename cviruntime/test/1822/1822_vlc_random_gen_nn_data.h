/**
 * copy from git@gitlab-ai.bitmain.vip:2290/wesley.teng/tpu_compress.git tpu_compress/test_vlc_compress.c
   only include random_gen_nn_data relative function
 */

#ifndef __BM_VLC_COMPRESS_RANDOM_GEN_NN_DATA_H__
#define __BM_VLC_COMPRESS_RANDOM_GEN_NN_DATA_H__
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// --- contrain random test ---
double getGaussianRandomVar(double mean, double std)
{
  double PI = 3.1415926;
  double u0 = (double)rand() / RAND_MAX;
  double u1 = (double)rand() / RAND_MAX;
  double n = sqrt(-2 * log(u0)) * cos(2 * PI * u1);
  return n * std + mean;
}

double getExpRandomVar(double lambda)
{
  double x = (double)rand() / RAND_MAX;
  return log(1 - x) / (-lambda);
}

void random_gen_nn_data(uint8_t *ibuf, size_t in_num, bool signedness, bool data_type, double zero_ratio)
{
  float *random_buf = (float *)malloc(in_num * sizeof(float));
  int zero_thr = (int)(100 * zero_ratio);
  double lambda = getGaussianRandomVar(0, 0.5);
  double mean = getGaussianRandomVar(0, 8);
  bool pdf_sel = ((rand() % 10) < 9); // 9 over 10 choose exponential pdf
  double max_v = 0;
  double eps = 0.0001;
  lambda += (lambda > 0) ? eps : -eps;
  for (size_t i = 0; i < in_num; i++)
  {
    double val = (pdf_sel) ? getExpRandomVar(lambda) : getGaussianRandomVar(mean, lambda);
    val = ((signedness || data_type) && rand() % 2) ? -val : val;
    random_buf[i] = ((rand() % 100) < zero_thr) ? 0 : val;
    max_v = (fabs(random_buf[i]) > max_v) ? fabs(random_buf[i]) : max_v;
  }

  if (data_type == 0) // INT8
  {
    double cali_decay = (signedness) ? (rand() / (double)RAND_MAX) + 1 : 1; // weight dacay by calibration
    uint8_t pruned_thr = (signedness && !data_type && (rand() % 2)) ? rand() % 12 : 0;
    for (size_t i = 0; i < in_num; i++)
    {
      int val = (int)((random_buf[i] * 127) / (max_v * cali_decay));
      ibuf[i] = (abs(val) < pruned_thr)
                    ? 0
                    : (val > 127)
                          ? 127
                          : (val < (-128))
                                ? -128
                                : val;
    }
  }
  else // BFloat16
  {
    uint16_t *bf16_buf = (uint16_t *)random_buf;
    for (size_t i = 0; i < in_num; i++)
    {
      short bf16_val = bf16_buf[(i << 1) + 1];
      // WARNING: set subnormal value to zero since HW do NOT support
      int exp = ((bf16_val >> 7) & 0xFF);
      bf16_val = (exp) ? bf16_val : 0;

      ibuf[i << 1] = (uint8_t)(bf16_val & 0xFF);
      ibuf[(i << 1) + 1] = (uint8_t)(bf16_val >> 8);
    }
  }
  free(random_buf);
}
  #ifdef __cplusplus
}
#endif

#endif /* __BM_VLC_COMPRESS_RANDOM_GEN_NN_DATA_H__ */
