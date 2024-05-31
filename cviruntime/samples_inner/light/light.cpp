#include <iostream>
#include <random>
#include <functional>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>
#include "cviruntime.h"
#include "cviruntime_extra.h"
#include "cnpy.h"

using system_clock = std::chrono::system_clock;
using duration = std::chrono::duration<double, std::milli>;

static int index_get(int h, int w1, int w2) {
  return h * w1 + w2;
}

static void fill_pad_fmap_int8(
    const int8_t *before, int8_t **pafter, int val,
    int pad_l, int pad_r, int pad_t, int pad_b,
    int ins_h, int ins_w, int ins_h_last, int ins_w_last,
    int h_before, int w_before)
{
  int w_after = (w_before - 1) * (ins_w + 1) + ins_w_last + 1 + pad_l + pad_r;
  int h_after = (h_before - 1) * (ins_h + 1) + ins_h_last + 1 + pad_t + pad_b;
  int8_t *after = *pafter;

  if (!after) {
    after = (int8_t *)malloc(sizeof(int8_t) * w_after * h_after);
    assert(after);
  }

  memset(after, val, w_after * h_after);
  for (int h = 0; h < h_before; h++) {
    for (int w = 0; w < w_before; w++) {
      int i = (h * (ins_h + 1) + pad_t) * w_after + w * (ins_w + 1) + pad_l;
      after[i] = before[h * w_before + w];
    }
  }
  *pafter = after;
}

static void max_pooling(
    const int8_t* i_fmap,
    int8_t* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int output_h, int output_w, int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int input_sign)
{
  const int max_init = input_sign? -128: 0;
  int8_t *i_fmap_pad = NULL;
  for (int nc = 0; nc < input_n * input_c; nc++) {
    fill_pad_fmap_int8(i_fmap, &i_fmap_pad, max_init,
      pad_w_l, pad_w_r, pad_h_t, pad_h_b,
      0, 0, 0, 0, input_h, input_w);

    for (int ph = 0; ph < output_h; ++ph) {
      for (int pw = 0; pw < output_w; ++pw) {
        int hstart = ph * stride_h;
        int wstart = pw * stride_w;
        int pool_index = index_get(ph, output_w, pw);
        int max = max_init;
        for (int h = 0; h < kh; h++) {
          for (int w = 0; w < kw; w++) {
            int index = index_get((hstart + h), (input_w + pad_w_l + pad_w_r),
                            (w + wstart));
            int val = input_sign ? i_fmap_pad[index]: (uint8_t)i_fmap_pad[index];
            max = (val > max)? val: max;
          }
        }
        o_fmap[pool_index] = max;
      }
    }
    i_fmap += input_w * input_h;
    o_fmap += output_w * output_h;
  }
  free(i_fmap_pad);
}

class Light {
public:
  Light(int32_t ih, int32_t iw, int32_t kernel_size)
    : input_h(ih), input_w(iw) {
    CVI_RT_Init(&ctx);
    mem_x = CVI_RT_MemAlloc(ctx, ih * iw);
    mem_y = CVI_RT_MemAlloc(ctx, ih * iw);
    kfn = CVI_NN_PrepareGrayImageLightKernelFunc(ctx, ih, iw, kernel_size);
  }

  ~Light() {
    CVI_RT_MemFree(ctx, mem_x);
    CVI_RT_MemFree(ctx, mem_y);
    CVI_NN_DestroyKernelFunc(kfn);
    CVI_RT_DeInit(ctx);
  }

  uint8_t* run(uint8_t *input) {
    auto vptr_x = (uint8_t *)CVI_RT_MemGetVAddr(mem_x);
    // copy data and flush cache
    memcpy(vptr_x, input, input_h * input_w);
    CVI_RT_MemFlush(ctx, mem_x);

    CVI_NN_RunKernelFunc(kfn, 2,
                        CVI_RT_MemGetPAddr(mem_x),
                        CVI_RT_MemGetPAddr(mem_y));
    // invalidate cpu cache
    CVI_RT_MemInvld(ctx, mem_y);
    // get result pointer
    return (uint8_t *)CVI_RT_MemGetVAddr(mem_y);
  }

private:
  CVI_RT_HANDLE ctx;
  CVI_KFUNC_HANDLE kfn;
  CVI_RT_MEM mem_x;
  CVI_RT_MEM mem_y;
  int32_t input_h;
  int32_t input_w;
};

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s h w k\n", argv[0]);
    return 1;
  }
  srand(100);

  int ih = atoi(argv[1]);
  int iw = atoi(argv[2]);
  int k = atoi(argv[3]);
  int pad = (k - 1) / 2;
  int s = 1;
  int oh = (ih + 2*pad - k) / s + 1;
  int ow = (iw + 2*pad - k) / s + 1;
  assert(ih == oh);
  assert(iw == ow);

  uint8_t *x = (uint8_t *)malloc(ih * iw);
  for (int i = 0; i < (int)(ih * iw); i++) {
    x[i] = rand() % 256;
  }
  uint8_t *bkg = (uint8_t *)malloc(ih * iw);
  uint8_t *ref = (uint8_t *)malloc(ih * iw);

  max_pooling((int8_t *)x, (int8_t *)bkg, 1, 1, ih, iw, oh, ow,
              k, k, pad, pad, pad, pad, s, s, 0);
  for (int i = 0; i < oh * ow; i++) {
    int8_t mask = (x[i] >= bkg[i]) ? 0 : 1;
    ref[i] = mask * (x[i] - bkg[i]) + 255;
  }

  Light light(ih, iw, k);

  auto start = system_clock::now();

  auto y = light.run(x);

  auto end = system_clock::now();
  duration d = end - start;
  std::cout << "run duration: " << d.count() << "(ms)\n";

  // get result and compare with reference
  for (uint32_t i = 0; i < ih * iw; i++) {
    if (y[i] != ref[i]) {
      std::cout << "compare failed [" << i << "] " << (int)y[i]
                << " vs " << (int)ref[i] << "\n";
      assert(0);
    }
  }

  printf("test passed!\n");
  return 0;
}