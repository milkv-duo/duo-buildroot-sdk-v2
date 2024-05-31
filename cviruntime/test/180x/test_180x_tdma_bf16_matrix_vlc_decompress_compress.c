#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"


typedef cvk_tdma_g2l_matrix_copy_decompressed_param_t decompress_param_t;
typedef cvk_tdma_l2g_matrix_copy_compressed_param_t compress_param_t;

typedef struct{
  decompress_param_t dec_p;
  compress_param_t com_p;
} param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => %s\n",
      tag,
      p->dec_p.dst->shape.n, p->dec_p.dst->shape.c, p->dec_p.dst->shape.w, p->dec_p.dst->shape.col,
      (p->dec_p.dst->fmt == CVK_FMT_I8)? "signed": "unsigned");
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_mg_shape_t src_shape;
  cvk_ml_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 0, 1 },
    { 0, 1, 1, 1 },
  },
  {
    { 0, 2 },
    { 0, 1, 2, 2 },
  },
  {
    { 0, 2 },
    { 0, 2, 1, 2 },
  },
  {
    { 0, 7 },
    { 0, 1, 7, 7 },
  },
#ifndef ENABEL_SIMPLE_VLC_TEST
  {
    { 0, 7 },
    { 0, 2, 4, 7 },
  },
  {
    { 0, 7 },
    { 0, 7, 1, 7 },
  },
  {
    { 0, 17 },
    { 0, 1, 17, 17 },
  },
  {
    { 0, 17 },
    { 0, 3, 7, 17 },
  },
  {
    { 0, 17 },
    { 0, 17, 1, 17 },
  },
  {
    { 0, 60 },
    { 0, 1, 60, 60 },
  },
  {
    { 0, 60 },
    { 0, 30, 2, 60 },
  },
  {
    { 0, 60 },
    { 0, 60, 1, 60 },
  }
#endif /* ifndef ENABEL_SIMPLE_VLC_TEST*/
};

static void test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p, uint16_t *src_data,
  CommandInfo* cmd_info)
{
  print_param(stderr, p);
  uint64_t size = ml_shape_size(&p->dec_p.dst->shape, CVK_FMT_I8);
  uint64_t bytesize = size * fmt_size(p->dec_p.dst->fmt);
  int is_signed = (p->dec_p.dst->fmt == CVK_FMT_I8);

  uint16_t *gmem_data;
  size_t bs_size;
  size_t data_type = (p->dec_p.dst->fmt == CVK_FMT_BF16) ? 1 : 0;

  gmem_data = (uint16_t* )test_vlc_compress((uint8_t* )src_data, bytesize, is_signed, data_type, &bs_size, cmd_info, NULL);

  //1. send compressed one to gaddr and decompress from gaddr to local
  cmpr_matrix_copy_s2d(rt_handle, p->dec_p.src, (uint8_t* ) gmem_data);
  cvk_ctx->ops->tdma_g2l_matrix_copy_decompressed(cvk_ctx, &p->dec_p);
  CVI_RT_Submit(cvk_ctx);

  //2. decompress from sram
  cvk_ctx->ops->tdma_l2g_matrix_copy_compressed(cvk_ctx, &p->com_p);
  CVI_RT_Submit(cvk_ctx);

  //3. get final data
  uint16_t *dst_data = (uint16_t* )cmpr_matrix_copy_d2s(rt_handle, p->com_p.dst);

  for (uint64_t i = 0; i < bs_size / 2; i++) {
    if (dst_data[i] != gmem_data[i]) {
      fprintf(stderr, "vlc compress comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], gmem_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(gmem_data);
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_cmpr_matrix_dev_mem(rt_handle, p->dec_p.src);
  free_cmpr_matrix_dev_mem(rt_handle, p->com_p.dst);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dec_p.dst);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  cvk_fmt_t fmts[] = { CVK_FMT_BF16 };
  uint8_t fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (uint32_t row = 1; row < 13; row += 2) {
    c->src_shape.row = row;
    c->dst_shape.n = row;
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      for (uint8_t fmt_i = 0; fmt_i < fmts_sz; fmt_i++) {
        cvk_fmt_t fmt = fmts[fmt_i];
        param_t p;
        memset(&p, 0, sizeof(p));
        //put compressed data to gaddr ->decompress to local -> compress to gaddr

        int is_signed = (fmt == CVK_FMT_I8);
        int data_type = (fmt == CVK_FMT_BF16) ? 1 : 0;

        CommandInfo cmd_info;
        memset(&cmd_info, 0, sizeof(CommandInfo));
        cmd_info.signedness = is_signed;
        cmd_info.is_bfloat16 = data_type;
        cmd_info.bias0 = 127;

        // <! not support bias0/1 setting compress by hw
        //get_vlc_compressed_meta(src_data, size, fmt, &bs_size, &cmd_info);

        //1. alloc decompress
        p.dec_p.src = alloc_cmpr_matrix_dev_mem(rt_handle, c->src_shape, fmt, &cmd_info);
        p.dec_p.dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, c->dst_shape, fmt, dst_align);

        uint64_t size = ml_shape_size(&p.dec_p.dst->shape, p.dec_p.dst->fmt);
        uint16_t *src_data = (uint16_t *)malloc(sizeof(uint16_t) * size);
        test_vlc_init_testdata((uint8_t *)src_data, size, fmt == CVK_FMT_I8, fmt == CVK_FMT_BF16);

        assert(p.dec_p.dst);

        //2. alloc compress
        p.com_p.src = p.dec_p.dst; //cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->lmem_shape, fmt, align);
        p.com_p.dst = alloc_cmpr_matrix_dev_mem(rt_handle, c->src_shape, fmt, &cmd_info);

        //3. test: the sequence like below:
        //3.1 put compressed data to gaddr
        //3.2 decompress to local
        //3.3 compress to gaddr
        //printf ("row %u is_align %d fmt %d\n", row, dst_align, fmt);
        test_param_g2l(rt_handle, cvk_ctx, &p, src_data, &cmd_info);
        destroy_param_g2l(rt_handle, cvk_ctx, &p);
        free(src_data);
      }
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
