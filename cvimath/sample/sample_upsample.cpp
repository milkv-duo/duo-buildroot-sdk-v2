// \file sample for set value by mask, plz refer \cvimath_internal.h for more details

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

static void init_input(uint8_t *input_data, uint64_t ifmap_size) {
  for (uint64_t i = 0; i < ifmap_size; i++) {
    input_data[i] = i;
  }
}

static void init_weight(uint8_t *weight_data, uint64_t weight_size) {
  for (uint64_t i = 0; i < weight_size; i++) {
    weight_data[i] = 1;  // NOTICE: MUST init as 1 under nearest upsample case
  }
}

static int init_ref(uint8_t *input, uint8_t *output, int n, int c, int ih, int iw, int scale_h,
                    int scale_w) {
  int h = ih * scale_h;
  int w = iw * scale_w;
  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          int nwi = wi / scale_w;
          int nhi = hi / scale_h;
          int out_idx = (((ni * c + ci) * h) + hi) * w + wi;
          int in_idx = (((ni * c + ci) * (h / scale_h)) + nhi) * (w / scale_w) + nwi;
          output[out_idx] = input[in_idx];
        }
      }
    }
  }
  return 0;
}

static void testbench(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, cvk_tg_shape_t *tg_shape) {
  // for calculate size we need in host
  cvk_tl_shape_t ifmap_shape = {tg_shape->n, tg_shape->c, tg_shape->h, tg_shape->w};

  // upsample scale, e.g: scale_h = 3,scale_w 2, input h = 4, input w = 5
  // output h is 4 * 3 = 12, output w is 5 * 2 = 10 with nearest

  int scale_h = 3;
  int scale_w = 2;

  // set output shape
  cvk_tl_shape_t ofmap_shape = ifmap_shape;
  ofmap_shape.h = ofmap_shape.h * scale_h;
  ofmap_shape.w = ofmap_shape.w * scale_w;

  cvk_tl_shape_t weight_shape = ifmap_shape;
  weight_shape.h = scale_h;
  weight_shape.w = scale_w;

  uint64_t ifmap_size = tl_shape_size(&ifmap_shape);
  uint64_t ofmap_size = tl_shape_size(&ofmap_shape);
  uint64_t weight_size = tl_shape_size(&weight_shape);

  // unit size is 1 bytes for int/uint 8
  int data_type_size = 1;

  // get input/output size
  uint64_t ifmap_bytesize = ifmap_size * data_type_size;
  uint64_t ofmap_bytesize = ofmap_size * data_type_size;
  uint64_t weight_bytesize = weight_size * data_type_size;

  // alloc on ddr
  // uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *input_data = (uint8_t *)xmalloc(ifmap_bytesize);
  uint8_t *ref_data = (uint8_t *)xmalloc(ofmap_bytesize);
  uint8_t *weight_data = (uint8_t *)xmalloc(weight_bytesize);

  // init input / output data in ddr
  init_input(input_data, ifmap_size);
  init_weight(weight_data, weight_bytesize);  // fix pattern
  init_ref(input_data, ref_data, ifmap_shape.n, ifmap_shape.c, ifmap_shape.h, ifmap_shape.w,
           scale_h, scale_w);

  // alloc on sram
  cvk_fmt_t fmt = CVK_FMT_I8;
  int eu_align = 1;
  cvk_tl_t *tl_ifmap = test_alloc_tl(cvk_ctx, ifmap_shape, fmt, eu_align);
  cvk_tl_t *tl_weight = test_alloc_tl(cvk_ctx, weight_shape, fmt, eu_align);
  cvk_tl_t *tl_ofmap = test_alloc_tl(cvk_ctx, ofmap_shape, fmt, eu_align);

  // send device memory to sram
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_ifmap, input_data);
  test_put_tensor_g2l_comp(rt_ctx, cvk_ctx, tl_weight, weight_data);

  // generate descriptor
  cvm_upsample2d(cvk_ctx, tl_ifmap, tl_weight, tl_ofmap);

  // submit descriptor
  test_submit_comp(rt_ctx, cvk_ctx);

  // get data from tl
  uint8_t *ofmap_data = (uint8_t *)test_get_tensor_l2g_comp(rt_ctx, cvk_ctx, tl_ofmap);

  // compare with reference with byte
  for (uint32_t i = 0; i < ofmap_size; i++) {
    if (ref_data[i] != ofmap_data[i]) {
      fprintf(stderr, "comparing failed output[%u] got %u, ref %u\n", i, ofmap_data[i],
              ref_data[i]);
      // fail case
      fflush(stderr);
      exit(-1);
    }
  }

  // free resource from tpu memory
  free_tl(cvk_ctx, tl_ofmap);
  free_tl(cvk_ctx, tl_weight);
  free_tl(cvk_ctx, tl_ifmap);

  // free resource from host memory
  free(ref_data);
  free(weight_data);
  free(ofmap_data);
  free(input_data);
}

int main() {
  CVI_RT_HANDLE rt_ctx;
  cvk_context_t *cvk_ctx;

  // init runtime / kerenl structure
  test_init(&rt_ctx, &cvk_ctx);

  cvk_tg_shape_t tg_shape = {1, 20, 3, 4};
  // cvk_tg_shape_t tg_shape = {1, 20, 3, 40};

  // run test
  testbench(&rt_ctx, cvk_ctx, &tg_shape);

  // de-init runtime / kerenl structure
  test_exit(&rt_ctx, cvk_ctx);

  printf("pass\n");

  return 0;
}
