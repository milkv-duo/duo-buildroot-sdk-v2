// This demo demonstrate how to do tranpose from [nhwc] to [nchw] with cpu and tpu
#include <cassert>
#include <cstring>
#include <ctime>
#include <vector>
#include <iostream>
#include <random>
#include <functional>

#include "add.h"
#include "cviruntime.h"
#include "cvikernel/cvikernel.h"

static constexpr int NPU_NUM = 32;
static constexpr int EU_NUM = 16;
static constexpr int LOCAL_MEM_SIZE = 1 << 15;
#define MAX_TIU_NUM (4096 - 32)

typedef uint16_t bf16_t;

static inline int ceiling_func(int numerator, int denominator)
{
  return (numerator + denominator - 1) / denominator;
}

void MyAddOp::tiling(int64_t total)
{
  tiling_info_t tile;
  memset(&tile, 0, sizeof(tile));
  tile.n = 1;
  tile.c = NPU_NUM;
  tile.w = EU_NUM;
  tile.h = std::min(ceiling_func(total, tile.c * tile.w), MAX_TIU_NUM);
  bool lmem_ok = false;
  tiles.clear();
  while (total > 0)
  {
    int64_t count = tile.n * tile.c * tile.h * tile.w;
    cvk_tl_shape_t tl_shape = {
        .n = tile.n, .c = tile.c, .h = tile.h, .w = tile.w};
    if (lmem_ok == false)
    {
      uint32_t lsize = 2 * ctx->ops->lmem_tensor_to_size(ctx, tl_shape,
                                                         CVK_FMT_BF16, 1);
      lmem_ok = (lsize <= (uint32_t)LOCAL_MEM_SIZE);
    }
    if (count > total || lmem_ok == false)
    {
      if (tile.h > 1)
      {
        tile.h--;
      }
      else if (tile.w > 1)
      {
        tile.w--;
      }
      else if (tile.c > 1)
      {
        tile.c--;
      }
      else
      {
        assert(0 && "lmem is not enough");
      }
    }
    else
    {
      tiles.emplace_back(tile);
      total -= count;
      tile.offset += count * 2;
    }
  }
  assert(total == 0 && "tiling error");
  return;
}

void MyAddOp::codeGenBf16(std::vector<int64_t> shape)
{

  int64_t total = std::accumulate(shape.begin(), shape.end(), 1,
                                  std::multiplies<int64_t>());
  uint64_t ga_input0 = 0;
  uint64_t ga_input1 = total * sizeof(bf16_t);
  uint64_t ga_output = ga_input1 + total * sizeof(bf16_t);
  tiling(total);
  for (auto &tile : tiles)
  {
    cvk_tl_shape_t tl_shape = {
        .n = tile.n, .c = tile.c, .h = tile.h, .w = tile.w};
    auto tl_input0 =
        ctx->ops->lmem_alloc_tensor(ctx, tl_shape, CVK_FMT_BF16, 1);
    auto tl_input1 =
        ctx->ops->lmem_alloc_tensor(ctx, tl_shape, CVK_FMT_BF16, 1);
    // load input 0
    cvk_tg_t tg_i0 = {0};
    tg_i0.fmt = CVK_FMT_BF16;
    tg_i0.start_address = ga_input0 + tile.offset;
    tg_i0.base_reg_index = 2;
    tg_i0.shape = {tile.n, tile.c, tile.h, tile.w};
    tg_i0.stride =
        ctx->ops->tg_default_stride(ctx, tg_i0.shape, CVK_FMT_BF16);

    cvk_tdma_g2l_tensor_copy_param_t p0 = {0};
    p0.src = &tg_i0;
    p0.dst = tl_input0;
    p0.layer_id = 0;
    ctx->ops->tdma_g2l_bf16_tensor_copy(ctx, &p0);

    // load input 1
    cvk_tg_t tg_i1 = {0};
    tg_i1.fmt = CVK_FMT_BF16;
    tg_i1.start_address = ga_input1 + tile.offset;
    tg_i1.base_reg_index = 2;
    tg_i1.shape = {tile.n, tile.c, tile.h, tile.w};
    tg_i1.stride =
        ctx->ops->tg_default_stride(ctx, tg_i1.shape, CVK_FMT_BF16);

    cvk_tdma_g2l_tensor_copy_param_t p1 = {0};
    p1.src = &tg_i1;
    p1.dst = tl_input1;
    p1.layer_id = 0;
    ctx->ops->tdma_g2l_bf16_tensor_copy(ctx, &p1);

    // add input 0 and input 1 => input0
    cvk_tiu_add_param_t p2 = {0};
    p2.res_low = tl_input0;
    p2.a_low = tl_input0;
    p2.b.low = tl_input1;
    p2.layer_id = 0;
    ctx->ops->tiu_add(ctx, &p2);

    // store
    cvk_tg_t tg_dst = {0};
    tg_dst.fmt = CVK_FMT_BF16;
    tg_dst.start_address = ga_output + tile.offset;
    tg_dst.base_reg_index = 2;
    tg_dst.shape = {tile.n, tile.c, tile.h, tile.w};
    tg_dst.stride =
        ctx->ops->tg_default_stride(ctx, tg_dst.shape, CVK_FMT_BF16);

    cvk_tdma_l2g_tensor_copy_param_t p3 = {0};
    p3.src = tl_input0;
    p3.dst = &tg_dst;
    p3.layer_id = 0;
    ctx->ops->tdma_l2g_bf16_tensor_copy(ctx, &p3);

    ctx->ops->lmem_free_tensor(ctx, tl_input1);
    ctx->ops->lmem_free_tensor(ctx, tl_input0);
  }
}

static void jit_compile(uint8_t **cmdbuf, uint32_t &size, std::vector<int64_t> &shape)
{
  cvk_reg_info_t req_info;

  memset(&req_info, 0, sizeof(cvk_reg_info_t));
  strncpy(req_info.chip_ver_str, "cv183x", sizeof(req_info.chip_ver_str) - 1);
  req_info.cmdbuf_size = 300000;
  req_info.cmdbuf = (uint8_t *)malloc(req_info.cmdbuf_size);
  auto cvk_ctx = cvikernel_register(&req_info);
  MyAddOp add(cvk_ctx);
  add.codeGenBf16(shape);
  *cmdbuf = cvk_ctx->ops->acquire_cmdbuf(cvk_ctx, &size);
}

void add_by_tpu(bf16_t *input0, bf16_t *input1, bf16_t *output, std::vector<int64_t> &shape)
{
  int64_t total = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int64_t>());
  int64_t tensor_size = total * sizeof(bf16_t);
  // runtime init
  CVI_RT_HANDLE ctx = nullptr;
  CVI_RT_Init(&ctx);

  uint8_t *cmdbuf = nullptr;
  uint32_t cmdbuf_size = 0;

  // generate cmdbuf
  jit_compile(&cmdbuf, cmdbuf_size, shape);

  // Alloc device memory for input + output + cmdbuf
  CVI_RT_MEM shared_mem = CVI_RT_MemAlloc(ctx, tensor_size * 3);
  CVI_RT_MEM input0_mem = CVI_RT_MemPreAlloc(shared_mem, 0, tensor_size);
  CVI_RT_MEM input1_mem = CVI_RT_MemPreAlloc(shared_mem, tensor_size, tensor_size);
  CVI_RT_MEM output_mem = CVI_RT_MemPreAlloc(shared_mem, tensor_size * 2, tensor_size);

  CVI_RT_MEM cmdbuf_mem = nullptr;
  // Load cmdbuf
  CVI_RT_LoadCmdbuf(ctx, cmdbuf, cmdbuf_size, CVI_RT_MemGetPAddr(shared_mem), 0, false, &cmdbuf_mem);

  // Get input tensor virtual address
  bf16_t *input0_ptr = (bf16_t *)CVI_RT_MemGetVAddr(input0_mem);
  bf16_t *input1_ptr = (bf16_t *)CVI_RT_MemGetVAddr(input1_mem);
  memcpy(input0_ptr, input0, tensor_size);
  memcpy(input1_ptr, input1, tensor_size);
  // Flush cache
  CVI_RT_MemFlush(ctx, input0_mem);
  CVI_RT_MemFlush(ctx, input1_mem);

  // Run cmdbuf
  CVI_RC ret = CVI_RT_RunCmdbuf(ctx, cmdbuf_mem, CVI_RT_MemGetPAddr(shared_mem), 0);
  assert(ret == 0);
  // Flush cache
  CVI_RT_MemInvld(ctx, output_mem);

  // Get output tensor virtual address
  bf16_t *output_ptr = (bf16_t *)CVI_RT_MemGetVAddr(output_mem);
  memcpy(output, output_ptr, tensor_size);

  // Release device memory
  CVI_RT_MemFree(ctx, cmdbuf_mem);
  CVI_RT_MemFree(ctx, output_mem);
  CVI_RT_MemFree(ctx, input1_mem);
  CVI_RT_MemFree(ctx, input0_mem);
  CVI_RT_MemFree(ctx, shared_mem);
  CVI_RT_DeInit(ctx);
}

static inline bf16_t BF16(const float &data)
{
  uint16_t *p_data_bf16 = (uint16_t *)(&data);
  return p_data_bf16[1];
}

static inline float FP32(const bf16_t &data)
{
  float data_f32 = 0.0f;
  uint16_t *p_data_bf16 = (uint16_t *)(&data_f32);
  p_data_bf16[1] = data;
  return data_f32;
}

int main(int argc, char *argv[])
{
  std::vector<int64_t> shape = {1, 3, 1600, 2500};
  int64_t total = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int64_t>());
  std::vector<bf16_t> input0(total);
  std::vector<bf16_t> input1(total);
  std::vector<bf16_t> output(total);
  std::vector<float> output_cpu(total);
  for (int64_t i = 0; i < total; i++)
  {
    float data0 = (float)(i % 255);
    float data1 = (float)((i + 128) % 255);
    input0[i] = BF16(data0);
    input1[i] = BF16(data1);
    output_cpu[i] = data0 + data1;
  }

  add_by_tpu(input0.data(), input1.data(), output.data(), shape);
  printf(">> cpu output: ");
  for (int64_t i = 0; i < 16; i++)
  {
    printf("%f ", output_cpu[i]);
  }

  printf("\n>> tpu output: ");
  for (int64_t i = 0; i < 16; i++)
  {
    printf("%f ", FP32(output[i]));
  }
  printf("\n");

  return 0;
}
