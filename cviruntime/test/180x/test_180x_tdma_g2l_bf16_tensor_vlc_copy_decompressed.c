#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_tensor_copy_decompressed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => fmt(%d) bias0/1/zero is (%u/%u/%u) %s\n",
      tag,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w,
      p->dst->fmt,
      p->src->bias0, p->src->bias1, p->src->zero_guard_en,
      (p->dst->fmt == CVK_FMT_I8)? "signed": "unsigned");
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_tl_shape_t lmem_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 17, 13 }
  },
#if 0 // No enough local memory for 180x
  {
    { 3, 39, 17, 23 }
  },

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
  # if 0  // No enough local memory for 180x 
  {
    { 3, 36, 16,  20 }
  },
  #endif
#endif /* ifndef ENABEL_SIMPLE_VLC_TEST*/
};

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, uint16_t *src_data, CommandInfo* cmd_info)
{
  print_param(stderr, p);
  uint64_t size = tl_shape_size(&p->dst->shape, CVK_FMT_I8);
  uint64_t bytesize = size * fmt_size(p->dst->fmt);
  size_t bs_size = 0;
  int is_signed = (p->dst->fmt == CVK_FMT_I8);
  uint8_t data_type = (p->dst->fmt == CVK_FMT_BF16) ? 1 : 0;
  int ret = 0;

  uint8_t *bsbuf = test_vlc_compress((uint8_t *)src_data, bytesize, is_signed, data_type, &bs_size, cmd_info, NULL);

  uint16_t *ref_data = (uint16_t *)malloc(bytesize);
  cvk_vlc_dec_bf16(bsbuf, bytesize, (uint16_t* )ref_data);

  cmpr_tensor_copy_s2d(rt_handle, p->src, bsbuf);
  cvk_ctx->ops->tdma_g2l_tensor_copy_decompressed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);

  uint16_t *dst_data = (uint16_t* )tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "vlc decompress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
          i, dst_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

  free(bsbuf);
  free(dst_data);
  free(ref_data);
  return ret;
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_cmpr_tensor_dev_mem(rt_handle, p->src);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[] = { CVK_FMT_BF16 };
  uint8_t fmts_sz = sizeof(fmts) / sizeof(fmts[0]);
  int ret = 0;

  for (int dst_align = 0; dst_align < 2; dst_align++) {
    for (int mode = 0; mode < VLC_CMP_MODE_MAX; mode++) {
      for (uint8_t fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
        cvk_fmt_t fmt = fmts[fmt_i];
        param_t p;
        memset(&p, 0, sizeof(p));
        p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->lmem_shape, fmt, dst_align);
        if (!p.dst) {
          fprintf(stderr, "allocate tl shape(%d, %d, %d, %d) failed\n",
                  c->lmem_shape.n, c->lmem_shape.c, c->lmem_shape.h,
                  c->lmem_shape.w);
          continue;
        }

        uint64_t size = tl_shape_size(&p.dst->shape, CVK_FMT_I8);
        uint16_t *src_data = (uint16_t *)malloc(sizeof(uint16_t) * size);
        test_vlc_init_testdata((uint8_t *)src_data, size, fmt == CVK_FMT_I8, fmt == CVK_FMT_BF16);

        CommandInfo cmd_info;
        memset(&cmd_info, 0, sizeof(CommandInfo));
        int is_signed = (fmt == CVK_FMT_I8);
        uint8_t data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;

        cmd_info.signedness = is_signed;
        cmd_info.is_bfloat16 = data_type;

        if (mode == VLC_CMP_MODE_COMPILER) {
          cvk_vlc_est_weight_bias((uint8_t* )src_data, size * sizeof(uint16_t), (bool)is_signed, (bool)data_type, &cmd_info);
        }

        cvk_tg_shape_t tg_shape =
            tg_shape_t4(c->lmem_shape.n, c->lmem_shape.c, c->lmem_shape.h, c->lmem_shape.w);
        p.src = alloc_cmpr_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, fmt, &cmd_info);

        ret |= test_param_g2l(rt_handle, cvk_ctx, &p, src_data, &cmd_info);

        free(src_data);
        destroy_param_g2l(rt_handle, cvk_ctx, &p);
      }
    }
  }

  return ret;
}

int main(int argc, char **argv)
{
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
  int ret = 0;

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
    ret |= test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  printf("tdma g2l bf16 tensor vlc test %s\n", ret ? "fail" : "pass");

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);
  return 0;
}
