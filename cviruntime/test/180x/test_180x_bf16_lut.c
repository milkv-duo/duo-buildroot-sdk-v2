#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static uint32_t channel = -1; //<! 1822 hardcode

static uint64_t shape_size(cvk_tl_shape_t s)
{
  return s.n * s.c * s.h * s.w;
}

static void tl_lut_ref(
    uint16_t *ofmap,
    uint16_t *ifmap,
    uint16_t *table,
    cvk_tl_shape_t ifmap_shape,
    cvk_tl_shape_t table_shape)
{
  int ih, iw;
  int tn, th, tw;

  ih = ifmap_shape.h;
  iw = ifmap_shape.w;
  tn = table_shape.n;
  th = table_shape.h;
  tw = table_shape.w;
  assert(tn == 1);
  assert(th * tw == 256);

  for (uint64_t i = 0; i < shape_size(ifmap_shape); i++) {
    int ici = i / (ih * iw) % 32;
    ofmap[i] = table[ici * (th * tw) + ifmap[i]];
  }
}

static int test_tl_lut(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  cvk_tl_shape_t ifmap_shape = {1, channel, 1, 224};
  cvk_tl_shape_t table_shape = {1, channel, 32, 8};
  cvk_tl_shape_t ofmap_shape = ifmap_shape;

  uint64_t ifmap_size = shape_size(ifmap_shape);
  uint64_t table_size = shape_size(table_shape);
  uint64_t ofmap_size = shape_size(ofmap_shape);

  cvk_fmt_t fmt = CVK_FMT_BF16;

  int data_type_size = fmt_size(fmt);
  uint64_t ifmap_bytesize  =  ifmap_size * data_type_size;
  uint64_t table_bytesize  =  table_size * data_type_size;
  uint64_t ofmap_bytesize  =  ofmap_size * data_type_size;

  uint16_t *ifmap_data = (uint16_t *)malloc(ifmap_bytesize);
  uint16_t *table_data = (uint16_t *)malloc(table_bytesize);
  uint16_t *ref_data = (uint16_t *)malloc(ofmap_bytesize);
  if (!ifmap_data || !table_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint64_t i = 0; i < ifmap_size; i++)
    ifmap_data[i] = 0;
    //ifmap_data[i] = i - 20;

  for (uint64_t i = 0; i < table_size; i++)
    table_data[i] = i + i / 256 * 3;

  tl_lut_ref(ref_data, ifmap_data, table_data, ifmap_shape, table_shape);

  cvk_tl_t *tl_ifmap =
    cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx,ifmap_shape, fmt, 1);
  cvk_tl_t *tl_table =
    cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, table_shape, fmt, /*align*/1);
  cvk_tl_t *tl_ofmap =
    cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx,ofmap_shape, fmt, /*align*/1);
  uint16_t *ofmap_data = NULL;
  if (!tl_ifmap || !tl_table || !tl_ofmap) {
    ret = -1;
    goto fail_exit_2;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_ifmap, (uint8_t *)ifmap_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_table, (uint8_t *)table_data);

  cvk_tiu_lookup_table_param_t p12;
  p12.ofmap = tl_ofmap;
  p12.ifmap = tl_ifmap;
  p12.table = tl_table;
  cvk_ctx->ops->tiu_lookup_table(cvk_ctx, &p12);
  CVI_RT_Submit(cvk_ctx);

  ofmap_data = (uint16_t*)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_ofmap);
  for (uint64_t i = 0; i < ofmap_size; i++) {
    if (ofmap_data[i] != ref_data[i]) {
      fprintf(stderr,
          "comparing failed at ofmap_data[%" PRIu64 "], got %d, exp %d\n",
          i, ofmap_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

fail_exit_2:
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_ofmap);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_table);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_ifmap);
  free(ofmap_data);

fail_exit:
  free(ifmap_data);
  free(table_data);
  free(ref_data);

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  channel = cvk_ctx->info.npu_num;

  ret |= test_tl_lut(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
