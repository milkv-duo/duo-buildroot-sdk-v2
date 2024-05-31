#include <iostream>
#include <vector>
#include <fstream>
#include <string.h>
#include <cviruntime.h>
#include <cviruntime_context.h>
#include <cviruntime_extra.h>

using namespace std;

void check_output(uint8_t *in1, uint8_t *in2, int32_t *output) {
  std::vector<int32_t> out;
  int32_t sum = 0;
#if 0
  for (int i = 0; i < 4096; ++i) {
    int32_t sum = 0;
    for (int j = 0; j < 512; ++j) {
      sum += in1[j] * in2[i * 512 + j];
    }
    out.emplace_back(sum);
  printf("sim without transpose\n");
  }
#else
  for (int i = 0; i < 4096; ++i) {
    int32_t sum = 0;
    for (int j = 0; j < 512; ++j) {
      sum += in1[j] * in2[j * 4096 + i];
    }
    out.emplace_back(sum);
  }
  printf("sim with transpose\n");

#endif

  std::ofstream o_f1("sim.text");
  std::ofstream o_f2("model_out.text");
  for (int i = 0; i < 4096; ++i) {
    o_f1 << out[i] << "\n";
    o_f2 << output[i] << "\n";
    if (out[i] != output[i]) {
      printf("check failed\n");
    }
  }
  o_f1.close();
  o_f2.close();
  printf("check success\n");
}

int main(int argc, char *argv[]) {
  CVI_RT_HANDLE ctx;
  CVI_RT_Init(&ctx);
  int m = 1;
  int k = 512;
  int n = 4096;

  CVI_KFUNC_HANDLE dmabuf = CVI_NN_PrepareMatrixMulKernelFunc(ctx, CVI_FMT_UINT8, 1, 512, 4096);

  CVI_RT_MEM input1 = CVI_RT_MemAlloc(ctx, 512);
  CVI_RT_MEM input2 = CVI_RT_MemAlloc(ctx, 512 * 4096);
  CVI_RT_MEM output = CVI_RT_MemAlloc(ctx, 4096 * 4);

  uint8_t* in_ptr1 = CVI_RT_MemGetVAddr(input1);
  uint8_t* in_ptr2 = CVI_RT_MemGetVAddr(input2);

  for (int i = 0; i < 512; ++i) {
    in_ptr1[i] = 2;
  }
  int value = 0;

#define NOT_T
#ifdef NOT_T
  for (int i = 0; i < 4096; ++i) {
    value++;
    value %= 10; 
    for (int j = 0; j < 512; ++j) {
      in_ptr2[i * 512 + j] = value;
    }
  }
  printf("fill without transpose\n");
#else
  for (int i = 0; i < 4096; ++i) {
    value++;
    value %= 10; 
    for (int j = 0; j < 512; ++j) {
      in_ptr2[j * 4096 + i] = value;
    }
  }
  printf("fill with transpose\n");

#endif
  CVI_RT_MemFlush(ctx, input1);
  CVI_RT_MemFlush(ctx, input2);

  int ret = CVI_NN_RunKernelFunc(dmabuf, 3, CVI_RT_MemGetPAddr(input1), CVI_RT_MemGetPAddr(input2), CVI_RT_MemGetPAddr(output));
  CVI_RT_MemInvld(ctx, output);

  check_output(in_ptr1, in_ptr2, (int32_t *)CVI_RT_MemGetVAddr(output));

  CVI_RT_MemFree(ctx, input1);
  CVI_RT_MemFree(ctx, input2);
  CVI_RT_MemFree(ctx, output);
  CVI_NN_DestroyKernelFunc(dmabuf);
  CVI_RT_DeInit(ctx);
  return 0;

}
