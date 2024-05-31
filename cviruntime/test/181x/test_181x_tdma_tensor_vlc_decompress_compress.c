#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_tensor_copy_decompressed_param_t decompress_param_t;
typedef cvk_tdma_l2g_tensor_copy_compressed_param_t compress_param_t;

typedef struct{
  decompress_param_t dec_p;
  compress_param_t com_p;
} param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => %d-bit %s\n",
      tag,
      p->dec_p.dst->shape.n, p->dec_p.dst->shape.c, p->dec_p.dst->shape.h, p->dec_p.dst->shape.w,
      p->dec_p.src->bit_length,
      (p->dec_p.dst->fmt == CVK_FMT_I8)? "signed": "unsigned");
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
  {
    { 5, 39, 17, 23 }
  },
  {
    { 20, 35,  2, 2 }
  },
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

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, cvk_cmpr_tg_t* dst)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = tl_shape_size(&p->dec_p.dst->shape, p->dec_p.dst->fmt);
  int is_signed = (p->dec_p.dst->fmt == CVK_FMT_I8);
  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  uint8_t *gmem_data = NULL, *dst_data = NULL;
  if (!src_data) {
    ret = -1;
    goto fail_exit;
  }

  test_vlc_init_testdata(src_data, size, p->dec_p.dst->fmt == CVK_FMT_I8, p->dec_p.dst->fmt == CVK_FMT_BF16);

  size_t total_size;
  size_t data_type = (p->dec_p.dst->fmt == CVK_FMT_BF16) ? 1 : 0;
  size_t in_size = size;
  size_t bs_buf_size = get_out_bs_buf_size(size, data_type);
  gmem_data = (uint8_t *) malloc(bs_buf_size * sizeof(uint8_t));
  if (!gmem_data) {
    ret = -1;
    goto fail_exit;
  }

  // command info
  CommandInfo cmd_info;
  memset(&cmd_info, 0, sizeof(CommandInfo));
  cmd_info.signedness = is_signed;

  // <! not support bias0/1 setting compress by hw
  cvk_vlc_enc_int8(src_data, in_size, gmem_data, &total_size, &cmd_info);

  cmpr_tensor_copy_s2d(rt_handle, p->dec_p.src, gmem_data);
  cvk_ctx->ops->tdma_g2l_tensor_copy_decompressed(cvk_ctx, &p->dec_p);
  CVI_RT_Submit(cvk_ctx);

  dst->zero_guard_en = cmd_info.zero_guard_en;
  dst->bias0 = cmd_info.bias0;
  dst->bias1 = cmd_info.bias1;
  p->com_p.dst = dst;
  cvk_ctx->ops->tdma_l2g_tensor_copy_compressed(cvk_ctx, &p->com_p);
  CVI_RT_Submit(cvk_ctx);

  dst_data = cmpr_tensor_copy_d2s(rt_handle, p->com_p.dst);

  for (uint64_t i = 0; i < total_size ; i++) {
    if (dst_data[i] != gmem_data[i]) {
      fprintf(stderr, "vlc compress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], gmem_data[i]);
      ret = -1;
      break;
    }
  }

fail_exit:
  free(src_data);
  free(dst_data);
  free(gmem_data);

  return ret;
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_cmpr_tensor_dev_mem(rt_handle, p->dec_p.src);
  free_cmpr_tensor_dev_mem(rt_handle, p->com_p.dst);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dec_p.dst);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[2] = { CVK_FMT_I8, CVK_FMT_U8 };
  int ret = 0;

  for (int align = 0; align < 2; align++) {
    for (uint8_t fmt_i = 0; fmt_i < 2; fmt_i++) {
      cvk_fmt_t fmt = fmts[fmt_i];

      param_t p;
      memset(&p, 0, sizeof(p));
      cvk_tg_shape_t tg_shape =
          tg_shape_t4(c->lmem_shape.n, c->lmem_shape.c, c->lmem_shape.h, c->lmem_shape.w);
      p.dec_p.src = alloc_cmpr_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, fmt, NULL);
      p.dec_p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->lmem_shape, fmt, align);
      assert(p.dec_p.dst);

      p.com_p.src = p.dec_p.dst; //cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->lmem_shape, fmt, align);
      assert(p.com_p.src);
      cvk_cmpr_tg_t* dst = alloc_cmpr_tensor_dev_mem(rt_handle, cvk_ctx, tg_shape, fmt, NULL);

      ret |= test_param_g2l(rt_handle, cvk_ctx, &p, dst);
      destroy_param_g2l(rt_handle, cvk_ctx, &p);
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

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
