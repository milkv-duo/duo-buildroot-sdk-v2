#include "../1822_test_util.h"
#include <random>

static u32 channel = -1; //<! 1822 hardcode
static u32 table_h = 32;
static u32 table_w = 8;
static u32 table_hw = table_h * table_w;

using namespace std;
static void tl_lut_ref(
    u16 *ofmap,
    u16 *ifmap,
    u16 *table,
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

  for (u64 i = 0; i < tl_shape_size(&ifmap_shape); i++) {
    int ici = i / (ih * iw) % 32;
    u8 off = ifmap[i] & 0xff;
    ofmap[i] = table[ici * (th * tw) + off];
  }
}

static void gen_sigmoid(u16 *table_data, u64 table_size) {
  // S(x) = 1 / (1 + (e^-x))
  printf ("table_size is %" PRIu64 "\n", table_size);

  for (u64 i = 0; i < table_hw; i++) {
    int sign = rand() % 2 ? 1 : -1;
    float s = exp(0.001 * i) * sign;
    table_data[i] = convert_fp32_bf16(s);
  }

  for (u32 i = 1; i < channel; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(u16) * table_hw);
  }
}

static bool verify(u16 *ofmap_data, u16 *ref_data, u64 ofmap_size) {
  for (u64 i = 0; i < ofmap_size; i++) {
    if (ofmap_data[i] != ref_data[i]) {
      fprintf(stderr,
          "comparing failed at ofmap_data[%" PRIu64 "], got %d(0x%x), exp %d(0x%x)\n",
          i, ofmap_data[i], ofmap_data[i], ref_data[i], ref_data[i]);
      exit(-1);
    }
  }
  return true;
}

union bf16int8 {
  u16 bf16;
  u8  int8[2];
};

static void test_tl_int8_lut_bf16(bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  // TODO: check more shape / align
  tl_shape_t ifmap_shape = {1, channel, 16, 16};
  tl_shape_t table_shape = {1, channel, 32, 8}; // hard code for hw
  tl_shape_t ofmap_shape = ifmap_shape;

  u64 ifmap_size = tl_shape_size(&ifmap_shape);
  u64 table_size = tl_shape_size(&table_shape);
  u64 ofmap_size = tl_shape_size(&ofmap_shape);

  fmt_t fmt = FMT_BF16;

  int data_type_size = bytesize_of_fmt(fmt);
  u64 ifmap_bytesize  =  ifmap_size * data_type_size;
  u64 table_bytesize  =  table_size * data_type_size;
  u64 ofmap_bytesize  =  ofmap_size * data_type_size;

  u16 *ifmap_data = (u16 *)xmalloc(ifmap_bytesize);
  memset(ifmap_data, 0x00, ifmap_bytesize);
  // hw ONLY support index in int8

  for (u64 i = 0; i < ifmap_size; i++) {
    bf16int8 b;
    b.int8[0] = i % table_hw;
    b.int8[1] = i % table_hw;
    ifmap_data[i] = b.bf16;
  }

  u16 *table_data = (u16 *)xmalloc(table_bytesize);
  gen_sigmoid (table_data, table_size);

  u16 *ref_data = (u16 *)xmalloc(ofmap_bytesize);
  tl_lut_ref(ref_data, ifmap_data, table_data, ifmap_shape, table_shape);

  tl_t *tl_ifmap =
    alloc_tl(bk_ctx,ifmap_shape, fmt, /*align*/1);
  tl_t *tl_table =
    alloc_tl(bk_ctx, table_shape, fmt, /*align*/1);
  tl_t *tl_ofmap =
    alloc_tl(bk_ctx,ofmap_shape, fmt, /*align*/1);

  put_bf16_tensor_g2l(ctx, bk_ctx, tl_ifmap, ifmap_data, fmt);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_table, table_data, fmt);

  bmk1822_tiu_lookup_table_param_t p12;
  p12.ofmap = tl_ofmap;
  p12.ifmap = tl_ifmap;
  p12.table = tl_table;
  bmk1822_tiu_lookup_table(bk_ctx, &p12);
  test_submit(ctx);

  u16 *ofmap_data = (u16*)get_bf16_tensor_l2g(ctx, bk_ctx, tl_ofmap, fmt);
  verify(ofmap_data, ref_data, ofmap_size);

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
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  // get channel info
  bmk1822_chip_info_t chip_info = bmk1822_chip_info();
  channel = chip_info.npu_num;

  test_tl_int8_lut_bf16(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
