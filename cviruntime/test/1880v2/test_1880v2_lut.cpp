#include "1880v2_test_util.h"

static u64 shape_size(tl_shape_t s)
{
  return s.n * s.c * s.h * s.w;
}

static void tl_lut_ref(
    u8 *ofmap,
    u8 *ifmap,
    u8 *table,
    tl_shape_t ifmap_shape,
    tl_shape_t table_shape)
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

  for (u64 i = 0; i < shape_size(ifmap_shape); i++) {
    int ici = i / (ih * iw) % 32;
    ofmap[i] = table[ici * (th * tw) + ifmap[i]];
  }
}

static void test_tl_lut(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  tl_shape_t ifmap_shape = {1, 32, 1, 224};
  tl_shape_t table_shape = {1, 32, 16, 16};
  tl_shape_t ofmap_shape = ifmap_shape;

  u64 ifmap_size = shape_size(ifmap_shape);
  u64 table_size = shape_size(table_shape);
  u64 ofmap_size = shape_size(ofmap_shape);

  u8 *ifmap_data = (u8 *)xmalloc(ifmap_size);
  for (u64 i = 0; i < ifmap_size; i++)
    ifmap_data[i] = i - 20;

  u8 *table_data = (u8 *)xmalloc(table_size);
  for (u64 i = 0; i < table_size; i++)
    table_data[i] = i + i / 256 * 3;

  u8 *ref_data = (u8 *)xmalloc(ofmap_size);
  tl_lut_ref(ref_data, ifmap_data, table_data, ifmap_shape, table_shape);

  tl_t *tl_ifmap =
    alloc_tl(bk_ctx,ifmap_shape, FMT_I8, 1);;
  tl_t *tl_table =
    alloc_tl(bk_ctx, table_shape, FMT_I8, /*align*/1);
  tl_t *tl_ofmap =
    alloc_tl(bk_ctx,ofmap_shape, FMT_I8, /*align*/1);

  put_tensor_g2l(ctx, bk_ctx, tl_ifmap, ifmap_data);
  put_tensor_g2l(ctx, bk_ctx, tl_table, table_data);
  bmk1880v2_tiu_lookup_table_param_t p12;
  memset(&p12, 0, sizeof(p12));
  p12.ofmap = tl_ofmap;
  p12.ifmap = tl_ifmap;
  p12.table = tl_table;
  bmk1880v2_tiu_lookup_table(bk_ctx, &p12);
  test_submit(ctx);
  u8 *ofmap_data = get_tensor_l2g(ctx, bk_ctx, tl_ofmap);
  for (u64 i = 0; i < ofmap_size; i++) {
    if (ofmap_data[i] != ref_data[i]) {
      fprintf(stderr,
          "comparing failed at ofmap_data[%" PRIu64 "], got %d, exp %d\n",
          i, ofmap_data[i], ref_data[i]);
      exit(-1);
    }
  }
  free_tl(bk_ctx, tl_ofmap);
  free_tl(bk_ctx, tl_table);
  free_tl(bk_ctx, tl_ifmap);

  free(ifmap_data);
  free(table_data);
  free(ref_data);
  free(ofmap_data);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_tl_lut(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
