#include <iostream>
#include <vector>
#include <fstream>
#include <string.h>
#include <math.h>

#include <cviruntime.h>
#include <cviruntime_context.h>
#include <cviruntime_extra.h>

#include <euc_backend.hpp>

using namespace std;

void check_output(uint8_t *in1, uint8_t *in2, float *output, int m, int k, int n) {
  std::vector<float> out;
  float sum = 0;
  for (int i = 0; i < n; ++i) {
    float sum = 0;
    for (int j = 0; j < k; ++j) {
      sum += std::pow(in1[j] - in2[i * k + j], 2);
    }
    out.emplace_back(sum);
  }

#ifdef __DUMP__
  std::ofstream o_f1("sim.text");
  std::ofstream o_f2("cmdbuf_out.text");
#endif
  for (int i = 0; i < n; ++i) {
#ifdef __DUMP__
    o_f1 << out[i] << "\n";
    o_f2 << output[i] << "\n";
#endif
    if (out[i] != output[i]) {
      printf("check failed idx:%d [%f vs %f]\n", i, out[i], output[i]);
      //return;
    }
  }
#ifdef __DUMP__
  o_f1.close();
  o_f2.close();
#endif
  printf("check success\n");
}

int main(int argc, char *argv[]) {
  CVI_RT_HANDLE ctx;
  CVI_RT_Init(&ctx);
  CVI_RT_KHANDLE k_ctx = CVI_RT_RegisterKernel(ctx, 200000);
  int m = 1;
  int k = 256;
  int n = 4096;

  CVI_RT_MEM input1 = CVI_RT_MemAlloc(ctx, k);
  CVI_RT_MEM input2 = CVI_RT_MemAlloc(ctx, k * n);
  CVI_RT_MEM output = CVI_RT_MemAlloc(ctx, n * 4);

  uint8_t* in_ptr1 = CVI_RT_MemGetVAddr(input1);
  uint8_t* in_ptr2 = CVI_RT_MemGetVAddr(input2);

  for (int i = 0; i < k; ++i) {
    in_ptr1[i] = 2;
  }
  int value = 0;

  for (int i = 0; i < n; ++i) {
    value++;
    value %= 10; 
    for (int j = 0; j < k; ++j) {
      in_ptr2[i * k + j] = value;
    }
  }
  CVI_RT_MemFlush(ctx, input1);
  CVI_RT_MemFlush(ctx, input2);

  std::vector<uint8_t> cmdbuf;
  #if 1
  runtimeJitEuclideanDistance(k_ctx, n, k, cmdbuf);
  {
    printf("cmdbuf size:%zu\n", cmdbuf.size());
    std::ofstream  o_f("search_256_182x.bin", std::ios::binary);
    o_f.write((char *)cmdbuf.data(), cmdbuf.size());
    o_f.close();
  }
  #else
  {
    std::ifstream  i_f("cmdbuf/search_256_182x.bin", std::ios::binary);
    i_f.seekg(0, i_f.end);
    size_t length = i_f.tellg();
    cmdbuf.resize(length);
    i_f.seekg(0, i_f.beg);
    i_f.read((char *)cmdbuf.data(), cmdbuf.size());
  }
  #endif

  CVI_RT_MEM cmdbuf_mem;
  int ret = CVI_RT_LoadCmdbuf(ctx, (uint8_t*)cmdbuf.data(), cmdbuf.size(), CVI_RT_MemGetPAddr(input1), 0, false, &cmdbuf_mem);

  ret = CVI_RT_RunCmdbuf(ctx, cmdbuf_mem, CVI_RT_MemGetPAddr(input2), CVI_RT_MemGetPAddr(output));
  CVI_RT_MemInvld(ctx, output);

  check_output(in_ptr1, in_ptr2, (float *)CVI_RT_MemGetVAddr(output), m, k, n);

  CVI_RT_MemFree(ctx, input1);
  CVI_RT_MemFree(ctx, input2);
  CVI_RT_MemFree(ctx, output);
  CVI_RT_MemFree(ctx, cmdbuf_mem);
  CVI_RT_UnRegisterKernel(k_ctx);
  CVI_RT_DeInit(ctx);
  return 0;

}
