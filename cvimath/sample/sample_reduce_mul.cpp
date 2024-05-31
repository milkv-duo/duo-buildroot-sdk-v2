// \file mask sample for gt(great than), ge(great equal), eq(equal), lt(less than), le(less equal)

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

void init_input(uint8_t *input_data, uint64_t ifmap_bytesize, cvk_fmt_t fmt) {
  uint32_t fmt_size = cvm_bytesize_of_fmt(fmt);
  uint64_t sz = ifmap_bytesize / fmt_size;
  int round = 4;  // random
  for (uint64_t i = 0; i < sz; i++) {
    uint8_t r[2];
    r[0] = i % round;
    if (r[0] == 0) {
      r[0] = 1;  // prevent mul to 0
    }

    if (fmt_size == 2) {
      // bf16
      uint16_t bf16 = convert_fp32_bf16((float)r[0]);
      memcpy(r, &bf16, fmt_size);
    }
    memcpy(&input_data[i * fmt_size], r, fmt_size);
  }
}

void init_ref(uint8_t *input_data, uint8_t *ref_data, cvk_tl_shape_t *ifmap_shape, cvk_fmt_t fmt) {
  uint32_t fmt_size = cvm_bytesize_of_fmt(fmt);
  int ref_idx = 0;

  // reduce ONLY hw
  for (uint32_t n = 0; n < ifmap_shape->n; n++) {
    for (uint32_t c = 0; c < ifmap_shape->c; c++) {
      float tmp = 1;
      for (uint32_t h = 0; h < ifmap_shape->h; h++) {
        for (uint32_t w = 0; w < ifmap_shape->w; w++) {
          uint32_t off = (n * ifmap_shape->c * ifmap_shape->h * ifmap_shape->w +
                          c * ifmap_shape->h * ifmap_shape->w + h * ifmap_shape->w + w) *
                         fmt_size;
          float v;
          if (fmt_size == 2) {
            // bf16 case
            uint16_t bf16;
            memcpy(&bf16, &input_data[off], fmt_size);
            v = convert_bf16_fp32(bf16);
          } else {
            v = input_data[off];
          }
          tmp = v * tmp;
        }
      }
      uint8_t r[2];
      if (fmt_size == 2) {
        // bf16 case
        uint16_t bf16 = convert_fp32_bf16(tmp);
        memcpy(r, (void *)&bf16, fmt_size);
      } else {
        r[0] = tmp;
      }
      memcpy(&ref_data[ref_idx * fmt_size], r, fmt_size);
      ref_idx++;
    }
  }
}

static void testbench(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, cvk_fmt_t fmt) {
  // alloc shape, align with \len
  uint32_t input_n = 1;
  uint32_t input_c = 3;
  uint32_t input_h = 2;
  uint32_t input_w = 2;

  cvk_tl_shape_t ifmap_shape = {input_n, input_c, input_h, input_w};
  // NOTICE: ONLY reduce hw for performance
  cvk_tl_shape_t ofmap_shape = {input_n, input_c, 1, 1};

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);

  // unit size is 1 bytes, bf16 takes 2 bytes
  int data_type_size = 1;
  if (fmt == CVK_FMT_BF16) {
    data_type_size = 2;
  }

  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;

  // alloc input/output tl
  cvk_tl_t *tl_ifmap = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, CTRL_AL);

  // alloc data from ddr
  uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *ref_data = (uint8_t *)xmalloc(ofmap_bytesize);

  // init input / output data in ddr
  init_input(input_data, ifmap_bytesize, fmt);
  init_ref(input_data, ref_data, &ifmap_shape, fmt);

  // send host memory->device memory->tpu_memory
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_ifmap, (uint8_t *)input_data);

  // prepare command buffer
  cvm_reduce_hw_mul(cvk_ctx, tl_ifmap);

  // submit descriptor
  test_submit_comp(rt_ctx, cvk_ctx);

  // reshape for reduce result
  tl_ifmap->shape = {tl_ifmap->shape.n, tl_ifmap->shape.c, 1, 1};
  tl_ifmap->stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_ifmap->shape, tl_ifmap->fmt, 1);

  // get data from tl
  uint8_t *ofmap_data = test_get_tensor_l2g_comp(rt_ctx, cvk_ctx, tl_ifmap);

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
  free_tl(cvk_ctx, tl_ifmap);

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

  printf("test reduce mul int8\n");
  testbench(&rt_ctx, cvk_ctx, CVK_FMT_I8);

  printf("test reduce mul bf16\n");
  testbench(&rt_ctx, cvk_ctx, CVK_FMT_BF16);

  // de-init runtime / kerenl structure
  test_exit(&rt_ctx, cvk_ctx);

  // restore rounding mode
  restore_feround(round_mode);

  return 0;
}
