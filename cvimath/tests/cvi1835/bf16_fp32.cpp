// \file mask sample for gt(great than), ge(great equal), eq(equal), lt(less than), le(less equal)

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

void init_input(uint16_t *input_data, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    input_data[i] = convert_fp32_bf16(i * 1.0);
  }
}

void init_ref(uint16_t *input_data, uint32_t *ref_data, uint64_t ifmap_size) {
  union s {
    uint16_t int16[2];  // big endian
    uint32_t int32;
  };
  union s _s;
  for (uint64_t i = 0; i < ifmap_size; i++) {
    _s.int16[0] = 0;
    _s.int16[1] = input_data[i];
    ref_data[i] = _s.int32;
  }
}

static void testbench(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx,
                      cvk_tg_shape_t *bf16_tg_shape) {
  // for calculate size we need in host
  cvk_tl_shape_t ifmap_shape = {bf16_tg_shape->n, bf16_tg_shape->c, bf16_tg_shape->h,
                                bf16_tg_shape->w};

  // * 2 means fp32 takes twice size of bf16
  cvk_tl_shape_t ofmap_shape = {bf16_tg_shape->n, bf16_tg_shape->c, bf16_tg_shape->h,
                                bf16_tg_shape->w * 2};

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  // unit size is 1 bytes, bf16 takes 2 bytes
  int data_type_size = 2;

  uint64_t ifmap_bytesize = ifmap_size * data_type_size;

  // * 2 means fp32 takes twice size of bf16
  uint64_t ofmap_bytesize = ofmap_size * data_type_size * 2;

  uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *ref_data = (uint8_t *)xmalloc(ofmap_bytesize);

  // init input / output data in ddr
  init_input((uint16_t *)input_data, ifmap_size);
  init_ref((uint16_t *)input_data, (uint32_t *)ref_data, ifmap_size);

  // send host memory->device memory
  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_tg_shape_t fp32_tg_shape;
  fp32_tg_shape = {ofmap_shape.n, ofmap_shape.c, ofmap_shape.h, ofmap_shape.w};

  cvk_tg_t *bf16_tg = test_alloc_tg_mem_comp(rt_ctx, cvk_ctx, *bf16_tg_shape, fmt);
  assert(bf16_tg && "alloc bf16 fail");

  test_put_tg_mem_comp(rt_ctx, bf16_tg, (uint8_t *)input_data);

  cvk_tg_t *fp32_tg = test_alloc_tg_mem_comp(rt_ctx, cvk_ctx, fp32_tg_shape, fmt);
  assert(bf16_tg && "alloc fp32 fail");

  // prepare command buffer
  cvm_bf16_fp32(cvk_ctx, bf16_tg, fp32_tg);

  // submit descriptor
  test_submit_comp(rt_ctx, cvk_ctx);

  // get data from tl
  uint8_t *ofmap_data = test_get_tg_mem_comp(rt_ctx, fp32_tg);

  // compare with reference with byte
  for (uint32_t i = 0; i < ofmap_size; i++) {
    if (ref_data[i] != ofmap_data[i]) {
      fprintf(stderr, "comparing failed output[%u] got %u, ref %u\n", i, ofmap_data[i],
              ref_data[i]);
      // fail case
      exit(-1);
    }
  }

  // free resource from tpu memory
  test_free_tg_mem_comp(rt_ctx, bf16_tg);
  test_free_tg_mem_comp(rt_ctx, fp32_tg);

  // free resource from host memory
  free(input_data);
  free(ref_data);
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

  cvk_tg_shape_t bf16_tg_shape = {1, 2, 3, 4};
  {
    // test 1
    printf("test bf16 <%d,%d,%d,%d> to fp32\n", bf16_tg_shape.n, bf16_tg_shape.c, bf16_tg_shape.h,
           bf16_tg_shape.w);
    testbench(&rt_ctx, cvk_ctx, &bf16_tg_shape);
    printf("compare test bf16 to fp32 done\n");
  }

  {
    // test 2
    bf16_tg_shape = {1, 20, 30, 40};
    printf("test bf16 <%d,%d,%d,%d> to fp32\n", bf16_tg_shape.n, bf16_tg_shape.c, bf16_tg_shape.h,
           bf16_tg_shape.w);
    testbench(&rt_ctx, cvk_ctx, &bf16_tg_shape);
    printf("compare test bf16 to fp32 done\n");
  }

  bf16_tg_shape = {40, 40, 128, 256};
  for (int n = 1; n < (int)bf16_tg_shape.n; n += 10) {
    for (int c = 1; c < (int)bf16_tg_shape.c; c += 10) {
      for (int h = 1; h < (int)bf16_tg_shape.h; h += 100) {
        for (int w = 2; w < (int)bf16_tg_shape.w; w += 100) {
          printf("test bf16 <%d,%d,%d,%d> to fp32\n", bf16_tg_shape.n, bf16_tg_shape.c,
                 bf16_tg_shape.h, bf16_tg_shape.w);
          cvk_tg_shape_t _bf16_tg_shape = {(uint32_t)n, (uint32_t)c, (uint32_t)h, (uint32_t)w};
          testbench(&rt_ctx, cvk_ctx, &_bf16_tg_shape);
          printf("compare test bf16 to fp32 done\n");
        }
      }
    }
  }

  // de-init runtime / kerenl structure
  test_exit(&rt_ctx, cvk_ctx);

  // restore rounding mode
  restore_feround(round_mode);

  return 0;
}
