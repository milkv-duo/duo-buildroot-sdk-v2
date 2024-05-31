// \file mask sample for gt(great than), ge(great equal), eq(equal), lt(less than), le(less equal)

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

void init_input(uint32_t *input_data, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    input_data[i] = ((0x1234 + i) << 16) + 0x5678 + i;
  }
}

static void testbench(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx,
                      cvk_tg_shape_t *fp32_tg_shape) {
  // for calculate size we need in host
  cvk_tl_shape_t ifmap_shape = {fp32_tg_shape->n, fp32_tg_shape->c, fp32_tg_shape->h,
                                fp32_tg_shape->w};

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);

  // unit size is 1 bytes, bf16 takes 2 bytes
  int data_type_size = 2;

  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint64_t ifmap_bytesize_per_fp32 = ifmap_bytesize / 4;  // 4 means float takes 4 bytes

  // init input / output data in ddr
  init_input((uint32_t *)input_data, ifmap_bytesize_per_fp32);

  // send host memory->device memory
  cvk_fmt_t fmt = CVK_FMT_BF16;

  cvk_tg_t *fp32_tg = test_alloc_tg_mem_comp(rt_ctx, cvk_ctx, *fp32_tg_shape, fmt);
  test_put_tg_mem_comp(rt_ctx, fp32_tg, (uint8_t *)input_data);

  cvk_tg_shape_t bf16_tg_shape = *fp32_tg_shape;
  cvk_tg_t *bf16_tg = test_alloc_tg_mem_comp(rt_ctx, cvk_ctx, bf16_tg_shape, fmt);

  // prepare command buffer
  cvm_s2s_fp32_bf16(cvk_ctx, fp32_tg->start_address, fp32_tg->shape, bf16_tg->start_address,
                    bf16_tg->shape, fmt);

  // submit descriptor
  test_submit_comp(rt_ctx, cvk_ctx);

  // get data from tl
  uint8_t *ofmap_data = test_get_tg_mem_comp(rt_ctx, bf16_tg);

  // compare with reference with byte
  uint16_t *ofmap_data_bf16 = (uint16_t *)ofmap_data;
  uint32_t *input_data_i32 = (uint32_t *)input_data;
  for (uint32_t i = 0; i < ifmap_bytesize_per_fp32; i++) {
    uint16_t _input_data_i16 = (input_data_i32[i] >> 16) & 0xffff;
    if (_input_data_i16 != ofmap_data_bf16[i]) {
      fprintf(stderr, "comparing failed output[%u] got %u, ref %u\n", i, ofmap_data_bf16[i],
              _input_data_i16);
      // fail case
      exit(-1);
    }
  }

  // free resource from tpu memory
  test_free_tg_mem_comp(rt_ctx, bf16_tg);
  test_free_tg_mem_comp(rt_ctx, fp32_tg);

  // free resource from host memory
  free(input_data);
  free(ofmap_data);
}

int main() {
  CVI_RT_HANDLE rt_ctx;
  cvk_context_t *cvk_ctx;
  int round_mode;

  // align kerenl rounding mode
  round_mode = set_store_feround();

  // init runtime / kerenl structure
  test_init(&rt_ctx, &cvk_ctx);

  cvk_tg_shape_t fp32_tg_shape = {1, 2, 3, 4};
  {
    // test 1
    printf("test fp32 <%d,%d,%d,%d> to bf16\n", fp32_tg_shape.n, fp32_tg_shape.c, fp32_tg_shape.h,
           fp32_tg_shape.w);
    testbench(&rt_ctx, cvk_ctx, &fp32_tg_shape);
    printf("compare test bf16 to fp32 done\n");
  }

  {
    // test 2
    fp32_tg_shape = {1, 20, 30, 40};
    printf("test fp32 <%d,%d,%d,%d> to bf16\n", fp32_tg_shape.n, fp32_tg_shape.c, fp32_tg_shape.h,
           fp32_tg_shape.w);
    testbench(&rt_ctx, cvk_ctx, &fp32_tg_shape);
    printf("compare test bf16 to fp32 done\n");
  }

  // de-init runtime / kerenl structure
  test_exit(&rt_ctx, cvk_ctx);

  // restore rounding mode
  restore_feround(round_mode);

  return 0;
}
