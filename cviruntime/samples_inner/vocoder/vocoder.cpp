#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"
#include "vocoder_model.hpp"
#include <fstream>
#include <iostream>

void dump_info(const char *path) {
  std::string line;
  std::ifstream file(path);
  std::cout << "dump " << path << "\n";
  while (getline(file, line )) {
    std::cout << line << "\n";
  }
  file.close();
  std::cout << "=======\n";
}

int main(int argc, char **argv) {
  int ret = 0;
  if (argc < 4) {
    printf("Usage:\n");
    printf("   %s cvimodel, input_npz, ref_npz\n", argv[0]);
    exit(1);
  }
  cnpy::npz_t input_npz = cnpy::npz_load(argv[2]);
  if (input_npz.size() == 0) {
    printf("Failed to load input npz\n");
  }
  auto &input_arr = input_npz["input"];

  cnpy::npz_t ref_npz = cnpy::npz_load(argv[3]);
  if (ref_npz.size() == 0) {
    printf("Failed to load ref npz\n");
  }
  auto &ref_arr = ref_npz["522_Mul_dequant"];

  dump_info("/proc/meminfo");
  dump_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/alloc_mem");
  dump_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary");
  Vocoder vocoder(argv[1]);
  dump_info("/proc/meminfo");
  dump_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/alloc_mem");
  dump_info("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary");

  int32_t out_size;
  float *out = vocoder.run(input_arr.data<float>(),
                           input_arr.num_vals, out_size);
  assert(ref_arr.num_vals == out_size);

  int cnt = 10;
  float *ref = ref_arr.data<float>();
  for (int i = 0; i < (int)out_size; i++) {
    float ref_val = ref[i];
    if (abs(out[i] - ref_val) > 0.0001) {
      printf("compare failed at %d, %f vs %f\n", i, out[i], ref_val);
      if (cnt-- < 0)
      assert(0);
    }
  }
  printf("compare passed\n");
  return 0;
}