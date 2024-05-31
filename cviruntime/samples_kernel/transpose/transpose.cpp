// This demo demonstrate how to do tranpose from [nhwc] to [nchw] with cpu and tpu
#include <cassert>
#include <cstring>
#include <ctime>
#include <vector>
#include <iostream>
#include <random>
#include <functional>

#include "cviruntime_context.h"
#include "cvikernel/cvikernel.h"

static constexpr int NPU_NUM = 32;
static constexpr int EU_NUM = 16;
static constexpr int LOCAL_MEM_SIZE = 1 << 15;

static void jit_compile(uint8_t **cmdbuf, uint32_t &size, std::vector<int> &shape) {
  cvk_reg_info_t req_info;

  memset(&req_info, 0, sizeof(cvk_reg_info_t));
  strncpy(req_info.chip_ver_str, "cv183x", sizeof(req_info.chip_ver_str) - 1);
  req_info.cmdbuf_size = 300000;
  req_info.cmdbuf = (uint8_t*)malloc(req_info.cmdbuf_size);
  auto cvk_ctx = cvikernel_register(&req_info);

  int i_base_ga_idx = 0;
  int o_base_ga_idx = 3;
  uint64_t ifmap_ga = 0;
  uint64_t ofmap_ga = 0;

  int n = shape[0];
  int c = shape[1];
  int h = shape[2];
  int w = shape[3];

  cvk_tg_shape_t src_shape = {n, c*h, 1, w};
  cvk_tg_shape_t dst_shape = {n, w, 1, c*h};

  cvk_tg_stride_t src_stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, src_shape, CVK_FMT_I8);
  cvk_tg_stride_t dst_stride = cvk_ctx->ops->tg_default_stride(cvk_ctx, dst_shape, CVK_FMT_I8);

  cvk_tg_stride_t out_stride = {dst_stride.n, dst_stride.w, dst_stride.h, dst_stride.c};

  uint32_t n_step = 1;
  uint32_t c_step = 0;
  uint32_t h_step = 0;

  h_step = h;

  for (; h_step > 0; --h_step) {
    uint32_t total_size;
    for (c_step = c; c_step >= NPU_NUM; --c_step) {
      cvk_tl_shape_t tl_ifmap_shape = {1, c_step * h_step, 1, w};
      uint32_t tl_ifmap_size = cvk_ctx->ops->lmem_tensor_to_size(cvk_ctx, tl_ifmap_shape, CVK_FMT_I8, 0);
      total_size = tl_ifmap_size;
      if (total_size <= LOCAL_MEM_SIZE) {
        break;
      }
    }
    if (total_size <= LOCAL_MEM_SIZE) {
      break;
    }
  }

  std::cout << "tiling: c_step " << c_step << ", h_step " << h_step << "\n";
  assert(c_step && h_step);

  for (uint32_t n_pos = 0; n_pos < n; n_pos += n_step) {
    for (uint32_t c_pos = 0; c_pos < c; c_pos += c_step) {
      uint32_t tiling_c = std::min(c - c_pos, c_step);
      for (uint32_t h_pos = 0; h_pos < h; h_pos += h_step) {
        uint32_t tiling_h = std::min(h - h_pos, h_step);

        cvk_tl_t tl_ifmap;
        cvk_ctx->ops->lmem_init_tensor(cvk_ctx, &tl_ifmap, {1, tiling_c * tiling_h, 1, w}, CVK_FMT_I8, 0);
        tl_ifmap.start_address = 0;

        cvk_tg_t tg_src = {0};
        tg_src.base_reg_index = i_base_ga_idx;
        tg_src.fmt = CVK_FMT_I8;
        tg_src.start_address = ifmap_ga + src_stride.n * n_pos + src_stride.c * c_pos + src_stride.h * h_pos;
        tg_src.shape = {tl_ifmap.shape.n, tl_ifmap.shape.c, tl_ifmap.shape.h, tl_ifmap.shape.w};
        tg_src.stride = src_stride;

        std::cout << "tg offset: " << tg_src.start_address << ", shape: ("
                  << tg_src.shape.n << "," << tg_src.shape.c << ","
                  << tg_src.shape.h << "," << tg_src.shape.w <<")\n";
        std::cout << "tg stride: (" << src_stride.n << "," << src_stride.c << ","
                                    << src_stride.h << "," << src_stride.w << ")\n";

        cvk_tdma_g2l_tensor_copy_param_t p1 = {0};
        p1.src = &tg_src;
        p1.dst = &tl_ifmap;
        cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p1);

        cvk_tg_t tg_dst = {0};
        tg_dst.start_address = ofmap_ga + n_pos * out_stride.n + c_pos * out_stride.c + h_pos * out_stride.h;
        tg_dst.shape = {1, w, 1, tiling_c * tiling_h};
        tg_dst.stride = dst_stride;
        tg_dst.base_reg_index = o_base_ga_idx;
        tg_dst.fmt = CVK_FMT_I8;

        cvk_tdma_l2g_tensor_copy_cw_transposed_param_t p2 = {0};
        p2.src = &tl_ifmap;
        p2.dst = &tg_dst;
        cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, &p2);
      }
    }
  }

  *cmdbuf = cvk_ctx->ops->acquire_cmdbuf(cvk_ctx, &size);
}

void transpose_tpu(std::vector<int8_t> &input, std::vector<int> &shape,
                   std::vector<int8_t> &output) {
  int64_t tensor_size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int64_t>());

  // runtime init
  CVI_RT_HANDLE ctx = nullptr;
  CVI_RT_Init(&ctx);

  uint8_t *cmdbuf = nullptr;
  uint32_t cmdbuf_size = 0;

  // generate cmdbuf
  jit_compile(&cmdbuf, cmdbuf_size, shape);

  // Alloc device memory for input + output + cmdbuf
  CVI_RT_MEM shared_mem = CVI_RT_MemAlloc(ctx, tensor_size * 2);
  CVI_RT_MEM input_mem = CVI_RT_MemPreAlloc(shared_mem, 0, tensor_size);
  CVI_RT_MEM output_mem = CVI_RT_MemPreAlloc(shared_mem, tensor_size, tensor_size);
  CVI_RT_MEM cmdbuf_mem = nullptr;
  // Load cmdbuf
  CVI_RT_LoadCmdbuf(ctx, cmdbuf, cmdbuf_size, CVI_RT_MemGetPAddr(shared_mem), 0, false, &cmdbuf_mem);

  // Get input tensor virtual address
  int8_t *input_ptr = (int8_t*)CVI_RT_MemGetVAddr(input_mem);
  // Copy data to device
  for (int i = 0; i < tensor_size; ++i) {
    // input data range (-128, 127)
    input_ptr[i] = (int8_t)input[i];
  }
  // Flush cache
  CVI_RT_MemFlush(ctx, input_mem);

  // Run cmdbuf
  CVI_RC ret = CVI_RT_RunCmdbuf(ctx, cmdbuf_mem, CVI_RT_MemGetPAddr(input_mem), CVI_RT_MemGetPAddr(output_mem));
  assert(ret == 0);

  // Get output tensor virtual address
  int8_t *output_ptr = (int8_t*)CVI_RT_MemGetVAddr(output_mem);
  // Copy data from device
  for (int i = 0; i < tensor_size; ++i) {
    output[i] = (int)output_ptr[i];
  }
  // Flush cache
  CVI_RT_MemFlush(ctx, output_mem);

  // Release device memory
  CVI_RT_MemFree(ctx, cmdbuf_mem);
  CVI_RT_MemFree(ctx, shared_mem);
  CVI_RT_DeInit(ctx);
}

void transpose_ref(std::vector<int8_t> &input, std::vector<int> &shape,
                   std::vector<int8_t> &output_ref) {
  int num = shape[0];
  int channel = shape[1];
  int height = shape[2];
  int width = shape[3];

  // [0, 1, 2, 3] => [0, 3, 1, 2]
  for (int n = 0; n < num; ++n) {
    for (int c = 0; c < channel; ++c) {
      for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
          int32_t in_idx = w + h * width + c * height * width + n * channel * height * width;
          int32_t out_idx = h + c * height + w * channel * height + n * channel  * height * width;
          output_ref[out_idx] = input[in_idx];
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  std::mt19937::result_type seed = std::time(0);
  auto randint = [&](int begin, int end) {
    return std::bind(
        std::uniform_int_distribution<int>(begin, end),
        std::mt19937(seed));
  };

  // generate random shape
  auto int_gen1 = randint(1, 100);
  std::vector<int> shape(4);
  for (int i = 0; i < 4; ++i) {
    shape[i] = int_gen1();
  }

  std::cout << "tensor shape: (" << shape[0] << ", " << shape[1] << ", "
                                << shape[2] << ", " << shape[3] <<")\n";

  int64_t size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int64_t>());
  std::cout << "tesnor size: " << size << "\n";

  // generate random input tensor
  auto int_gen2 = randint(-128, 127);
  std::vector<int8_t> src(size);
  for (int i = 0; i < size; ++i) {
    src[i] = (int8_t)int_gen2();
  }

  // cpu implementation
  std::vector<int8_t> dst_ref(size);
  transpose_ref(src, shape, dst_ref);

  // tpu implementation
  std::vector<int8_t> dst(size);
  transpose_tpu(src, shape, dst);

  // compare result between cpu and tpu
  for (int i = 0; i < size; ++i) {
    if (dst[i] == dst_ref[i]) {
      continue;
    } else {
      printf("compare fail, index: %d, expect: %d, but get %d\n", i, dst_ref[i], dst[i]);
      return -1;
    }
  }

  std::cout << "compare pass!\n";

  return 0;
}
