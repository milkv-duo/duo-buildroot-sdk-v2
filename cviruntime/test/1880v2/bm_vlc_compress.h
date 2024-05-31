#ifndef __BM_VLC_COMPRESS_H__
#define __BM_VLC_COMPRESS_H__
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_UNARY_FIELD_SIZE 47
#define MAX_ORDER_K 5

  /**
   * \data_type 0 means 8bit, 1 means 16bit
   */
  static inline size_t get_out_bs_buf_size(u64 in_size, u8 data_type) {
    size_t blk_num = (data_type) ? ((in_size + 31) >> 5) : ((in_size + 15) >> 4);
    size_t in_size_pad = blk_num << (4 + data_type);
    size_t bs_buf_size = in_size_pad + (ceiling_func(blk_num, 16) << 4) + 16;
    return bs_buf_size;
  }

  typedef struct
  {
    bool signedness;
    bool is_bfloat16;
    uint8_t bias0;
    uint8_t bias1;
    bool zero_guard_en;
  } CommandInfo;
  typedef struct
  {
    uint8_t *stream; // stream buffer pointer
    int bit_pos;     // current pointer (in bit)
    int buf_size;    // in byte
  } StreamBuffer;

static inline int8_t two_side_circular_shift(int8_t val, uint8_t bias0, uint8_t bias1);
static inline int8_t inv_two_side_circular_shift(int8_t val, uint8_t bias0, uint8_t bias1);
static inline uint8_t center_shift(uint8_t val, uint8_t bias, bool zero_guard);
static inline uint8_t inv_center_shift(uint8_t val, uint8_t bias, bool zero_guard);

static inline void init_stream(StreamBuffer *bs, const uint8_t *buf, int buf_size, bool read_only);

static inline void bm_vlc_est_weight_bias(const uint8_t *ibuf, size_t isz, bool signedness, bool isBfloat16, CommandInfo *cmd_info);
static inline void bm_vlc_enc_int8(const uint8_t *ibuf, size_t isz, uint8_t *obuf, size_t *osz, CommandInfo *cmd_info);
static inline void bm_vlc_dec_int8(const uint8_t *ibuf, size_t isz, uint8_t *obuf);
static inline void bm_vlc_enc_bf16(const uint16_t *ibuf, size_t isz, uint8_t *obuf, size_t *osz, CommandInfo *cmd_info);
 static inline void bm_vlc_dec_bf16(const uint8_t *ibuf, size_t isz, uint16_t *obuf);

static inline uint8_t get_bit_val(uint8_t *buf, int byte_idx, int bit_idx)
  {
    return (buf[byte_idx] >> bit_idx) & 0x1;
  }

static inline uint8_t sign_to_unsign(uint8_t val)
  {
    uint8_t sign_i = (val >> 7) & 0x1;
    int abs_data_i = abs(((int8_t)val));
    return ((abs_data_i << 1) - sign_i);
  }

static inline int8_t unsign_to_sign(uint8_t val)
  {
    uint8_t sign_i = val & 0x1;
    int abs_data_i = (((int)val) + 1) >> 1;
    return (uint8_t)((sign_i == 1) ? (-abs_data_i) : abs_data_i);
  }

static inline void dispatch_bf16_data(const uint16_t *bf16_in, uint8_t *exp, uint8_t *frac, size_t isz)
{
  for (size_t i = 0; i < isz; i++)
  {
    exp[i] = (uint8_t)((bf16_in[i] >> 7) & 0xFF);
    frac[i] = (uint8_t)(((bf16_in[i] >> 15) << 7) | (bf16_in[i] & 0x7F));
  }
}

static inline void merge_bf16_data(const uint8_t *exp_in, const uint8_t *frac_in, uint16_t *bf16_out, size_t isz)
{
  memset(bf16_out, 0, sizeof(uint16_t));
  for (size_t i = 0; i < isz; i++)
  {
    bf16_out[i] = ((frac_in[i] >> 7) << 15) | (exp_in[i] << 7) | (frac_in[i] & 0x7F);
  }
}

// -- streaming operation handler --
static inline void init_stream(StreamBuffer *bs, const uint8_t *buf, int buf_size, bool read_only)
{
  bs->bit_pos = 0;
  bs->stream = (uint8_t *)buf;
  bs->buf_size = buf_size;
  if (!read_only)
    memset((uint8_t *)buf, 0, sizeof(uint8_t) * buf_size);
}

static inline void write_stream(StreamBuffer *bs, uint8_t *src, int bit_len)
{
  for (int bit = 0; bit < bit_len; bit++)
  {
    int src_byte_i = bit / 8;
    int src_bit_i = bit % 8;
    int dest_byte_i = (bs->bit_pos + bit) / 8;
    int dest_bit_i = (bs->bit_pos + bit) % 8;
    bs->stream[dest_byte_i] |= (get_bit_val(src, src_byte_i, src_bit_i) << dest_bit_i);
  }
  bs->bit_pos += bit_len;
}

static inline void move_stream_ptr(StreamBuffer *bs, int bit_len)
{
  bs->bit_pos += bit_len;
}

static inline void parse_stream(StreamBuffer *bs, uint8_t *dest, int bit_len)
{
  memset(dest, 0, sizeof(uint8_t) * (bit_len + 7) >> 3);
  for (int bit = 0; bit < bit_len; bit++)
  {
    int dest_byte_i = bit / 8;
    int dest_bit_i = bit % 8;
    int bs_byte_i = (bs->bit_pos + bit) / 8;
    int bs_bit_i = (bs->bit_pos + bit) % 8;
    dest[dest_byte_i] |= (get_bit_val(bs->stream, bs_byte_i, bs_bit_i) << dest_bit_i);
  }
  bs->bit_pos += bit_len;
}

// -- header read/write operation handler --
static inline void vlc_enc_header(StreamBuffer *bs_header, CommandInfo *cmd_info, size_t blk_bs_size)
{
  write_stream(bs_header, (uint8_t *)&blk_bs_size, 24);            // bit[23:0] compressed block stream size
  move_stream_ptr(bs_header, 4);                                   // bit[27:24] reserved
  write_stream(bs_header, (uint8_t *)&cmd_info->signedness, 1);    // bit[28] signedness
  write_stream(bs_header, (uint8_t *)&cmd_info->is_bfloat16, 1);   // bit[29] data type
  move_stream_ptr(bs_header, 2);                                   // bit[31:30] bit depth
  write_stream(bs_header, (uint8_t *)&cmd_info->bias0, 8);         // bit[39:32] bias0 for symbol remapping
  write_stream(bs_header, (uint8_t *)&cmd_info->bias1, 7);         // bit[46:40] bias1 for symbol remapping
  write_stream(bs_header, (uint8_t *)&cmd_info->zero_guard_en, 1); // bit[47] zero guard
}

static inline void vlc_dec_header(StreamBuffer *bs_header, CommandInfo *cmd_info)
{
  move_stream_ptr(bs_header, 28);                                // bit[27:24] reserved
  parse_stream(bs_header, (uint8_t *)&cmd_info->signedness, 1);  // bit[28] signedness
  parse_stream(bs_header, (uint8_t *)&cmd_info->is_bfloat16, 1); // bit[29] data type
  move_stream_ptr(bs_header, 2);
  parse_stream(bs_header, (uint8_t *)&cmd_info->bias0, 8);         // bit[39:32] bias0 for symbol remapping
  parse_stream(bs_header, (uint8_t *)&cmd_info->bias1, 7);         // bit[46:40] bias1 for symbol remapping
  parse_stream(bs_header, (uint8_t *)&cmd_info->zero_guard_en, 1); // bit[47] zero guard
}

// -- symbol remmaping handler --
static inline uint8_t center_shift(uint8_t val, uint8_t bias, bool zero_guard)
{
  if (val == 0 && zero_guard)
    return 0;

  int16_t shift_data_i = val - bias;
  uint8_t range = (bias <= 128) ? bias : 255 - bias;
  if (bias <= 128)
  {
    return (val >= (range << 1)) ? val : sign_to_unsign(shift_data_i) + zero_guard;
  }
  else
  {
    return (val < (bias - range)) ? (range + bias - val + zero_guard) : (sign_to_unsign(shift_data_i) + zero_guard);
  }
}

static inline uint8_t inv_center_shift(uint8_t val, uint8_t bias, bool zero_guard)
{
  if (val == 0 && zero_guard)
    return 0;

  uint8_t unsign_data_i = val - zero_guard;
  uint8_t range = (bias <= 128) ? bias : 255 - bias;
  if (bias <= 128)
  {
    return (val >= (range << 1)) ? val : unsign_to_sign(unsign_data_i) + bias;
  }
  else
  {
    return (unsign_data_i > (range << 1)) ? (range + bias - val + zero_guard) : unsign_to_sign(unsign_data_i) + bias;
  }
}

static inline int8_t two_side_circular_shift(int8_t val, uint8_t bias0, uint8_t bias1)
{
  if (val == 0)
    return 0;

  bool sign = (val < 0) ? true : false;
  int32_t abs_val = abs(val);
  abs_val -= (sign) ? bias1 : bias0;
  abs_val += (abs_val <= 0) ? (127 + sign) : 0;
  return (sign) ? -abs_val : abs_val;
}

static inline int8_t inv_two_side_circular_shift(int8_t val, uint8_t bias0, uint8_t bias1)
{
  if (val == 0)
    return 0;

  bool sign = (val < 0) ? true : false;
  uint32_t abs_val = abs(val);
  abs_val += (sign) ? bias1 : bias0;
  int32_t abs_val_minus = abs_val - (127 + sign);
  uint8_t abs_val_lsb = ((abs_val_minus <= 0)
                             ? abs_val
                             : abs_val_minus) &
                        0xFF;
  return (sign) ? -abs_val_lsb : abs_val_lsb;
}

static inline void symbol_remapping(uint8_t *blk_in, uint8_t *blk_out, uint8_t bias0, uint8_t bias1, bool signedness, bool is_bf16_exp, bool zero_guard)
{
  if (is_bf16_exp == false && signedness == false)
  {
    // remapping bypass
    memcpy(blk_out, blk_in, sizeof(uint8_t) * 16);
    return;
  }

  if (is_bf16_exp == true)
  {
    // center circular shift
    for (int i = 0; i < 16; i++)
    {
      blk_out[i] = center_shift(blk_in[i], bias0, zero_guard);
    }
  }
  else
  {
    // two-side circular shift
    for (int i = 0; i < 16; i++)
    {
      int8_t shift_data_i = two_side_circular_shift((int8_t)blk_in[i], bias0, bias1);
      blk_out[i] = sign_to_unsign(shift_data_i);
    }
  }
}

static inline void inv_symbol_remapping(uint8_t *blk_in, uint8_t *blk_out, uint8_t bias0, uint8_t bias1, bool signedness, bool is_bf16_exp, bool zero_guard)
{
  if (is_bf16_exp == false && signedness == false)
  {
    // remapping bypass
    memcpy(blk_out, blk_in, sizeof(uint8_t) * 16);
    return;
  }

  if (is_bf16_exp == true)
  {
    // center circular shift
    for (int i = 0; i < 16; i++)
    {
      blk_out[i] = inv_center_shift(blk_in[i], bias0, zero_guard);
    }
  }
  else
  {
    // two-side circular shift
    for (int i = 0; i < 16; i++)
    {
      int8_t sign_data_i = unsign_to_sign(blk_in[i]);
      blk_out[i] = (uint8_t)inv_two_side_circular_shift(sign_data_i, bias0, bias1);
    }
  }
}

static inline int vlc_estimate_block_order(uint8_t *blk_in, bool bf16_zvc_en)
{
  int best_k = 0;
  int best_bs_size = 0x7FFFFFFF;

  for (int k = 0; k <= (int)MAX_ORDER_K; k++)
  {
    uint8_t remain_field_size = k << 4;
    int unary_field_len = 0;
    for (int i = 0; i < 16; i++)
    {
      uint8_t group_idx = blk_in[i] >> k;
      unary_field_len += (group_idx + 1);
    }
    int znum_bit = (bf16_zvc_en && k > 0) ? 4 : 0;
    int blk_size = (unary_field_len <= MAX_UNARY_FIELD_SIZE)
                       ? remain_field_size + unary_field_len + znum_bit
                       : 255;
    if (blk_size < best_bs_size)
    {
      best_k = k;
      best_bs_size = blk_size;
    }
  }

  best_k = (best_bs_size > 128) ? -1 : best_k;
  return best_k;
}
// -- vlc block parrelel GR encode/decode --
static inline uint8_t vlc_gr_enc_block_data(uint8_t *blk_in, StreamBuffer *bs, int order_k, bool bf16_zvc_en)
{
  // uncompressed mode
  if (order_k == -1)
  {
    write_stream(bs, blk_in, 128);
    return 128;
  }

  // remain field
  uint8_t remain_field[16] = {0};
  uint8_t unary_field[8] = {0};
  uint8_t sym_end_pos[16] = {0};
  uint8_t unary_field_len = 0;
  int sym_end_pos_accum = -1;

  // bit plane encode for remain field
  for (int k = 0; k < order_k; k++)
  {
    uint8_t bit_plane0 = 0, bit_plane1 = 0;
    for (int i = 0; i < 8; i++)
    {
      bit_plane0 |= (get_bit_val(blk_in, i, k) << i);
      bit_plane1 |= (get_bit_val(blk_in, i + 8, k) << i);
    }
    remain_field[k << 1] = bit_plane0;
    remain_field[(k << 1) + 1] = bit_plane1;
  }
  write_stream(bs, remain_field, order_k << 4);

  if (bf16_zvc_en && order_k > 0)
  {
    int zero_num = 0;
    for (int i = 0; i < 16; i++)
    {
      if (blk_in[i] == 0)
        zero_num++;
    }
    assert(zero_num < 16);
    write_stream(bs, (uint8_t *)&zero_num, 4);
  }

  // unary encode for unary field
  for (int i = 0; i < 16; i++)
  {
    int group_idx = blk_in[i] >> order_k;
    sym_end_pos_accum += (group_idx + 1);
    sym_end_pos[i] = sym_end_pos_accum;
    int byte_idx = sym_end_pos[i] / 8;
    int bit_idx = sym_end_pos[i] % 8;
    unary_field[byte_idx] |= (1 << (bit_idx));
  }
  unary_field_len = sym_end_pos[15] + 1;
  assert(unary_field_len <= MAX_UNARY_FIELD_SIZE);
  uint8_t ulen = (unary_field_len - 16) & 0x1F;
  write_stream(bs, unary_field, unary_field_len);

  return ulen;
}

static inline void vlc_gr_dec_block_data(StreamBuffer *bs, uint8_t bs_size, uint8_t *rec, int order_k, bool bf16_zvc_en)
{
  assert(bs_size <= 128);
  // uncompressed mode
  if (order_k == -1)
  {
    parse_stream(bs, rec, 128);
    return;
  }

  // remain field
  uint8_t remain_data[16] = {0};
  uint8_t remain_bs[16] = {0};
  uint8_t unary_field[8] = {0};
  uint8_t sym_end_pos[16] = {0};
  uint8_t unary_sym[16] = {0};
  uint8_t remain_field_size = order_k << 4;

  parse_stream(bs, remain_bs, remain_field_size);
  // bit plane encode for remain field
  for (int k = 0; k < order_k; k++)
  {
    for (int i = 0; i < 8; i++)
    {
      remain_data[i] |= (get_bit_val(remain_bs, k << 1, i) << k);
      remain_data[i + 8] |= (get_bit_val(remain_bs, (k << 1) + 1, i) << k);
    }
  }

  // zero number info
  int znum_bit = (bf16_zvc_en && order_k > 0) ? 4 : 0;
  uint8_t znum = 0;
  parse_stream(bs, &znum, znum_bit);

  // unary encode for unary field
  uint8_t unary_field_len = bs_size - remain_field_size - znum_bit;
  parse_stream(bs, unary_field, unary_field_len);

  int sym_cnt = 0;
  for (uint8_t ubit_i = 0; ubit_i < unary_field_len; ubit_i++)
  {
    int byte_idx = ubit_i / 8;
    int bit_idx = ubit_i % 8;
    if (get_bit_val(unary_field, byte_idx, bit_idx) == 1)
    {
      sym_end_pos[sym_cnt] = ubit_i;
      sym_cnt++;
    }
  }
  unary_sym[0] = sym_end_pos[0];
  for (int i = 1; i < 16; i++)
  {
    unary_sym[i] = sym_end_pos[i] - sym_end_pos[i - 1] - 1;
  }
  for (int i = 0; i < 16; i++)
  {
    rec[i] = (unary_sym[i] << order_k) + remain_data[i];
  }
}

// -- vlc encode int8 entry funtion --
static inline void bm_vlc_enc_int8(const uint8_t *ibuf, size_t isz, uint8_t *obuf, size_t *osz, CommandInfo *cmd_info)
{
  StreamBuffer bs_header, bs_kmap, bs_data;
  size_t blk_num = (isz + 15) >> 4;
  size_t header_size = 16;
  size_t kmap_size = ceiling_func(blk_num, 16) << 4;
  size_t bs_buf_size = header_size + kmap_size + (blk_num << 4);
  uint8_t *bsbuf = (uint8_t *)calloc(bs_buf_size, sizeof(uint8_t));

  // block encode
  init_stream(&bs_kmap, bsbuf + header_size, kmap_size, false);
  init_stream(&bs_data, bsbuf + header_size + kmap_size, blk_num << 4, false);

  for (size_t blk_idx = 0; blk_idx < blk_num; blk_idx++)
  {
    uint8_t blk_data[16] = {0}, blk_sr_data[16] = {0};
    size_t in_size = (blk_idx == (blk_num - 1)) ? isz - (blk_idx << 4) : 16;
    memcpy(blk_data, &ibuf[blk_idx << 4], sizeof(uint8_t) * in_size);

    symbol_remapping(blk_data, blk_sr_data, cmd_info->bias0, cmd_info->bias1, cmd_info->signedness, false, false);

    int k = vlc_estimate_block_order(blk_sr_data, false);
    uint8_t ulen = vlc_gr_enc_block_data(blk_sr_data, &bs_data, k, false);
    uint8_t k_info = (k == -1) ? 0xE0 : (k << 5) + ulen;
    write_stream(&bs_kmap, &k_info, 8);
  }

  int blk_bs_size = ceiling_func(((bs_data.bit_pos + 7) >> 3), 16) << 4; // 16 byte align
  *osz = header_size + kmap_size + blk_bs_size;

  // write header
  init_stream(&bs_header, bsbuf, header_size, false);
  vlc_enc_header(&bs_header, cmd_info, blk_bs_size);

  memcpy(obuf, bsbuf, (*osz) * sizeof(uint8_t));
  free(bsbuf);
}

// -- vlc decode int8 entry funtion --
static inline void bm_vlc_dec_int8(const uint8_t *ibuf, size_t isz, uint8_t *obuf)
{
  StreamBuffer bs_header, bs_kmap, bs_data;
  CommandInfo cmd_info;
  memset(&cmd_info, 0, sizeof(CommandInfo));

  size_t blk_num = (isz + 15) >> 4;
  int header_size = 16;
  int kmap_size = ceiling_func(blk_num, 16) << 4;

  // parse header
  init_stream(&bs_header, ibuf, header_size, true);
  vlc_dec_header(&bs_header, &cmd_info);

  // block decode
  init_stream(&bs_kmap, ibuf + header_size, kmap_size, true);
  init_stream(&bs_data, ibuf + header_size + kmap_size, blk_num << 4, true);

  for (size_t blk_idx = 0; blk_idx < blk_num; blk_idx++)
  {
    uint8_t blk_data[16] = {0}, blk_sr_data[16] = {0};
    uint8_t k_info = 0;
    parse_stream(&bs_kmap, &k_info, 8);
    uint8_t ulen = k_info & 0x1F;
    int k = (k_info >> 5 == 7) ? -1 : k_info >> 5;
    int blk_bs_size = (k == -1) ? 128 : (k << 4) + ulen + 16;
    vlc_gr_dec_block_data(&bs_data, blk_bs_size, blk_data, k, false);

    inv_symbol_remapping(blk_data, blk_sr_data, cmd_info.bias0, cmd_info.bias1, cmd_info.signedness, false, false);

    int out_size = (blk_idx == (blk_num - 1)) ? isz - (blk_idx << 4) : 16;
    memcpy(&obuf[blk_idx << 4], blk_sr_data, sizeof(uint8_t) * out_size);
  }
}

// -- vlc encode bfloat16 entry funtion --
static inline void bm_vlc_enc_bf16(const uint16_t *ibuf, size_t isz, uint8_t *obuf, size_t *osz, CommandInfo *cmd_info)
{
  StreamBuffer bs_header, bs_kmap, bs_data;
  size_t blk_num = (isz + 31) >> 5; // 32 bytes per blok
  size_t header_size = 16;
  size_t kmap_size = ceiling_func(blk_num, 16) << 4;
  size_t bs_buf_size = header_size + kmap_size + (blk_num << 5);
  uint8_t *bsbuf = (uint8_t *)calloc(bs_buf_size, sizeof(uint8_t));

  // block encode
  init_stream(&bs_kmap, bsbuf + header_size, kmap_size, false);
  init_stream(&bs_data, bsbuf + header_size + kmap_size, blk_num << 5, false);

  for (size_t blk_idx = 0; blk_idx < blk_num; blk_idx++)
  {
    uint8_t blk_data[16] = {0}, blk_sr_data[16] = {0}, blk_data_frac[16] = {0};
    size_t in_num = (blk_idx == (blk_num - 1)) ? ((isz >> 1) - (blk_idx << 4)) : 16;
    dispatch_bf16_data(&ibuf[blk_idx << 4], blk_data, blk_data_frac, in_num);

    // exp: BGR encode
    symbol_remapping(blk_data, blk_sr_data, cmd_info->bias0, cmd_info->bias1, false, true, cmd_info->zero_guard_en);

    int k = vlc_estimate_block_order(blk_sr_data, cmd_info->zero_guard_en);
    uint8_t ulen = vlc_gr_enc_block_data(blk_sr_data, &bs_data, k, cmd_info->zero_guard_en);
    uint8_t k_info = (k == -1) ? 0xE0 : (k << 5) + ulen;
    write_stream(&bs_kmap, &k_info, 8);

    // frac: implicit zero compression
    for (size_t i = 0; i < 16; i++)
    {
      if (!cmd_info->zero_guard_en || blk_data[i] != 0)
      {
        write_stream(&bs_data, &blk_data_frac[i], 8);
      }
    }
  }

  int blk_bs_size = ceiling_func(((bs_data.bit_pos + 7) >> 3), 16) << 4; // 16 byte align
  *osz = header_size + kmap_size + blk_bs_size;

  // write header
  init_stream(&bs_header, bsbuf, header_size, false);
  vlc_enc_header(&bs_header, cmd_info, blk_bs_size);

  memcpy(obuf, bsbuf, (*osz) * sizeof(uint8_t));
  free(bsbuf);
}

// -- vlc decode bfloat16 entry funtion --
static inline void bm_vlc_dec_bf16(const uint8_t *ibuf, size_t isz, uint16_t *obuf)
{
  StreamBuffer bs_header, bs_kmap, bs_data;
  CommandInfo cmd_info;
  memset(&cmd_info, 0, sizeof(CommandInfo));

  size_t blk_num = (isz + 31) >> 5; // 32 bytes per blok
  int header_size = 16;
  int kmap_size = ceiling_func(blk_num, 16) << 4;

  // parse header
  init_stream(&bs_header, ibuf, header_size, true);
  vlc_dec_header(&bs_header, &cmd_info);

  // block decode
  init_stream(&bs_kmap, ibuf + header_size, kmap_size, true);
  init_stream(&bs_data, ibuf + header_size + kmap_size, blk_num << 5, true);

  for (size_t blk_idx = 0; blk_idx < blk_num; blk_idx++)
  {
    uint8_t blk_data[16] = {0}, blk_sr_data[16] = {0}, blk_data_frac[16] = {0};
    uint8_t k_info = 0;
    parse_stream(&bs_kmap, &k_info, 8);
    uint8_t ulen = k_info & 0x1F;
    int k = (k_info >> 5 == 7) ? -1 : k_info >> 5;
    int znum_bit = (cmd_info.zero_guard_en && k > 0) ? 4 : 0;
    uint8_t blk_bs_size = (k == -1) ? 128 : (k << 4) + ulen + 16 + znum_bit;

    // exp: BGR decode
    vlc_gr_dec_block_data(&bs_data, blk_bs_size, blk_data, k, cmd_info.zero_guard_en);

    inv_symbol_remapping(blk_data, blk_sr_data, cmd_info.bias0, cmd_info.bias1, false, true, cmd_info.zero_guard_en);

    size_t out_num = (blk_idx == (blk_num - 1)) ? ((isz >> 1) - (blk_idx << 4)) : 16;

    // frac: implicit zero compression
    for (size_t i = 0; i < out_num; i++)
    {
      if (!cmd_info.zero_guard_en || blk_sr_data[i] != 0)
      {
        parse_stream(&bs_data, &blk_data_frac[i], 8);
      }
    }
    merge_bf16_data(blk_sr_data, blk_data_frac, &obuf[blk_idx << 4], out_num);
  }
}

// -- offline estimate model weight params --
static inline void bm_vlc_est_weight_bias(const uint8_t *ibuf, size_t isz, bool signedness, bool isBfloat16, CommandInfo *cmd_info)
{
  assert(!(isBfloat16 && signedness)); // WARNING: signedness MUST be 0 as isBfloat16==True

  cmd_info->is_bfloat16 = isBfloat16;
  if (isBfloat16 == false && signedness == true)
  {
    // two-side circular shift
    int hist[256] = {0};
    for (size_t i = 0; i < isz; i++)
    {
      hist[ibuf[i]]++;
    }

    int8_t pos_v = 1;
    //while (pos_v < 128)
    // comparison is always   true due to limited range of data type [-Werror=type-limits]
    while (true)
    {
      if (hist[((uint8_t)pos_v)] == 0)
      {
        pos_v++;
      }
      else
      {
        break;
      }
    }
    //cmd_info->bias0 = (pos_v > 1 && pos_v < 128) ? (pos_v - 1) : 0;
    // comparison is always   true due to limited range of data type [-Werror=type-limits]
    cmd_info->bias0 = (pos_v > 1) ? (pos_v - 1) : 0;
    int8_t neg_v = -1;
    //while (neg_v >= (-128)) // comparison is always   true due to limited range of data type [-Werror=type-limits]
    while (true)
    {
      if (hist[(uint8_t)neg_v] == 0)
      {
        neg_v--;
      }
      else
      {
        break;
      }
    }
    //cmd_info->bias1 = (neg_v < -1 && neg_v >= -128) ? abs(neg_v + 1) : 0;
    // comparison is always   true due to limited range of data type [-Werror=type-limits]
    cmd_info->bias1 = (neg_v < -1) ? abs(neg_v + 1) : 0;
    cmd_info->signedness = true;
  }

  if (isBfloat16 == true)
  {
    // center shift
    int64_t exp_accum = 0;
    uint16_t *bf16_in = (uint16_t *)ibuf;
    size_t inum = (isz >> 1), cnt = 0;
    for (size_t i = 0; i < inum; i++)
    {
      uint8_t exp = ((bf16_in[i] >> 7) & 0xFF);
      if (exp != 0)
      {
        exp_accum += exp;
        cnt++;
      }
    }
    if (cnt > 0)
    {
      cmd_info->bias0 = (uint8_t)((exp_accum / (float)cnt) + 0.5);
    }
    cmd_info->zero_guard_en = (inum == cnt) ? false : true;
    cmd_info->signedness = false;
  }
}
  #ifdef __cplusplus
}
#endif

#endif /* __BM_VLC_COMPRESS_H__ */
