#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_l2g_tensor_copy_compressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => %d-bit %s\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      p->dst->bit_length,
      (p->src->fmt == CVK_FMT_I8)? "signed": "unsigned");
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_tl_shape_t lmem_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 17, 13 }
  },
  {
    { 3, 39, 17, 23 }
  },
#if 0 // No enough local memory for 1810
  {
    { 5, 39, 17, 23 }
  },
  {
    { 20, 35,  2, 2 }
  },
#endif
#ifndef ENABEL_SIMPLE_VLC_TEST
  {
    { 1, 1, 1, 1 }
  },
  {
    { 1, 1, 1, 2 }
  },
  {
    { 1, 1, 7, 2 }
  },
  {
    { 1, 1, 10, 60 }
  },
  {
    { 1, 2, 1, 1 }
  },
  {
    { 2, 17, 1,  4 }
  },
  {
    { 2, 17, 1,  4 }
  },
  {
    { 3, 16, 1, 1 }
  },
  {
    { 3, 36, 16,  20 }
  },
#endif /* ifndef ENABEL_SIMPLE_VLC_TEST*/
};

static void test_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, CommandInfo* cmd_info, uint16_t *src_data)
{
  print_param(stderr, p);
  uint64_t bytesize = tl_shape_size(&p->src->shape, p->src->fmt);
  int is_signed = (p->src->fmt == CVK_FMT_I8);
  uint8_t data_type = (p->src->fmt == CVK_FMT_BF16) ? 1 : 0;
  size_t bs_size = 0;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, (uint8_t *)src_data);
  cvk_ctx->ops->tdma_l2g_tensor_copy_compressed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);

  uint16_t *dst_data = (uint16_t* )cmpr_tensor_copy_d2s(rt_handle, p->dst);
  uint16_t *ref_data = (uint16_t* )test_vlc_compress((uint8_t *)src_data, bytesize, is_signed, data_type, &bs_size, cmd_info, NULL);

  for (uint64_t i = 0; i < bs_size / 2 ; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "vlc compress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);

      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}

static void destroy_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_cmpr_tensor_dev_mem(rt_handle, p->dst);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[] = { CVK_FMT_BF16 };
  uint8_t fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (int src_align = 0; src_align < 2; src_align++) {
    for (uint8_t fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
      cvk_fmt_t fmt = fmts[fmt_i];
      uint8_t data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;
      param_t p;
      memset(&p, 0, sizeof(p));

      p.src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->lmem_shape, fmt, src_align);
      assert(p.src);

      CommandInfo cmd_info;
      memset(&cmd_info, 0, sizeof(CommandInfo));
      uint64_t in_size = tl_shape_size(&p.src->shape, CVK_FMT_I8);

      uint16_t *src_data = (uint16_t *)malloc(sizeof(uint16_t) * in_size);
      test_vlc_init_testdata((uint8_t *)src_data, in_size, fmt == CVK_FMT_I8, fmt == CVK_FMT_BF16);

      int is_signed = (p.src->fmt == CVK_FMT_I8);
      cmd_info.signedness = is_signed;
      cmd_info.is_bfloat16 = data_type;
      cmd_info.bias0 = 127;

      // <! not support bias0/1 setting compress by hw
      //cvk_vlc_est_weight_bias(src_data, in_size, (bool)is_signed, (bool)data_type, &cmd_info);

      cvk_tg_shape_t tg_shape =
          tg_shape_t4(c->lmem_shape.n, c->lmem_shape.c, c->lmem_shape.h, c->lmem_shape.w);
      p.dst = alloc_cmpr_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, fmt, &cmd_info);
      test_param_l2g(rt_handle, cvk_ctx, &p, &cmd_info, src_data);
      destroy_param_l2g(rt_handle, cvk_ctx, &p);

      free(src_data);
    }
  }
}

int main(int argc, char **argv)
{
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

  uint32_t nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (uint32_t i = 0; i < nr_cases; i++)
    test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
