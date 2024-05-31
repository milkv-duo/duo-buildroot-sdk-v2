#ifndef __BM_NEURON_DUMP_HPP_
#define __BM_NEURON_DUMP_HPP_

#include <stdint.h>

typedef uint32_t u32;

#define DEBUG_NEURON_DUMP 0

#if (DEBUG_NEURON_DUMP)
template <typename dtype>
static void neuron_dump(
    const char str[], u32 n, u32 c, u32 h, u32 w, dtype* data) {
  printf("%s, shape = (%d, %d, %d, %d)\n", str, n, c, h, w);
  for (u32 ni = 0; ni < n; ni ++) {
    for (u32 ci = 0; ci < c; ci ++) {
      printf("n = %d, c = %d\n", ni, ci);
      for(u32 hi = 0; hi < h; hi++) {
        printf("\t| ");
        for(u32 wi = 0; wi < w; wi ++) {
          u32 n_stride = c * h * w;
          u32 c_stride = h * w;
          u32 h_stride = w;
          u32 index = ni * n_stride + ci * c_stride + hi * h_stride  + wi;
          printf("%4d ", data[index]);
        }
        printf("| \n");
      }
    }
  }
}

#else
template <typename dtype>
static void neuron_dump(const char str[], u32 n, u32 c, u32 h, u32 w, dtype data[]) {
  (void)str;
  (void)n;
  (void)c;
  (void)h;
  (void)w;
  (void)data;
  return;
}

#endif //DEBUG_NEURON_DUMP
#endif //__BM_NEURON_DUMP_HPP_
