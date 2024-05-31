#ifndef COMPRESSION_H
#define COMPRESSION_H

typedef struct {
  u32 compress_md;
  u32 bit_length;
  int is_signed;

  u64 total_data_num;
  u32 non_zero_data_num;

  u64 header_bytes;
  u64 map_bytes;
  u64 data_bytes;
  u64 total_bytes;

  int compressed_min;
  int compressed_max;
} compression_info_t;

typedef struct {
  u64 header_offset;
  u64 header_size;
  u64 map_offset;
  u64 map_size;
  u64 data_offset;
  u64 data_size;
  u64 total_size;
} compress_addr_info;

static u64 compression_map_bytes(u64 total_data_num)
{
  u64 bit_alignment = 16 * 8;
  u64 bits = total_data_num;

  return ceiling_func(bits, bit_alignment)*16;
}

static u64 compression_map_clear_bytes(u64 total_data_num)
{
  u64 bit_alignment = 2 * 8;
  u64 bits = total_data_num;

  return ceiling_func(bits, bit_alignment)*2;
}


static u64 compression_data_bytes(u64 non_zero_data_num, u32 bit_length)
{
  if (bit_length == 1)
    return 0;

  u64 bit_alignment = 8;
  u64 bits = non_zero_data_num * bit_length;

  return ceiling_func(bits, bit_alignment);
}

static inline u32 compression_bit_length(u32 compress_md)
{
  switch (compress_md) {
    case 0:
      return 8;
    case 1:
      return 4;
    case 2:
      return 2;
    case 3:
      return 1;
    default:
      assert(0);
  }
}

static inline void compute_compressed_range(
    u32 bit_length, int is_signed, int *min, int *max)
{
  if (is_signed) {
    switch (bit_length) {
      case 1:
        *min = -1;
        *max = 0;
        return;
      case 2:
        *min = -2;
        *max = 1;
        return;
      case 4:
        *min = -8;
        *max = 7;
        return;
      case 8:
        *min = -128;
        *max = 127;
        return;
    }
  } else {
    *min = 0;
    switch (bit_length) {
      case 1:
        *max = 1;
        return;
      case 2:
        *max = 3;
        return;
      case 4:
        *max = 15;
        return;
      case 8:
        *max = 255;
        return;
    }
  }
  assert(0);
}

static inline int saturate(int val, int max, int min)
{
  if (val < min)
    return min;
  else if (val > max)
    return max;
  else
    return val;
}

static inline u64 count_non_zero_results(
    u8 buf[], u64 size, int is_signed, int max, int min)
{
  u64 n = 0;

  for (u64 i = 0; i < size; i++) {
    int val = is_signed? (s8)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0)
      n++;
  }

  return n;
}

static inline void set_map_bit(u8 map[], u64 i)
{
  u64 byte_i = i / 8;
  u64 bit_i = i % 8;

  map[byte_i] |= (1 << bit_i);
}

static inline u8 read_map_bit(u8 map[], u64 i)
{
  u64 byte_i = i / 8;
  u64 bit_i = i % 8;

  return (map[byte_i] >> bit_i) & 1;
}

static inline void parse_header(
    u32 header, int *is_signed, u32 *compress_md, u32 *nz_num)
{
  *is_signed = (header >> 29) & 1;
  *compress_md = (header >> 24) & 0b11;
  *nz_num = header & 0xffffff;
}

static inline void fill_header(u32 *hdr, compression_info_t *info)
{
  if(compression_bit_length(info->compress_md)!=1)
  {
    *hdr = (info->is_signed << 29) | (1 << 28) |
        (info->compress_md << 24) |
        info->non_zero_data_num;
  }else
  {
    *hdr = (info->is_signed << 29) | (1 << 28) |
        (info->compress_md << 24);
  }
}

static inline void fill_map(u8 map[], u8 buf[], compression_info_t *info)
{
  int min = info->compressed_min;
  int max = info->compressed_max;

  u64 clear_map = compression_map_clear_bytes(info->total_data_num);
  for (u64 i = 0; i < clear_map; i++)
    map[i] = 0;

  for (u64 i = 0; i < info->total_data_num; i++) {
    int val = info->is_signed? (s8)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0)
      set_map_bit(map, i);
  }
}

static inline void compress_one_data(
    u8 data[], u64 i, u8 val, compression_info_t *info)
{
  u32 bit_len = info->bit_length;
  u32 data_per_byte = 8 / bit_len;

  u32 byte_i = i / data_per_byte;
  u32 bit_i = (i % data_per_byte) * bit_len;
  u8 mask = (1 << bit_len) - 1;

  data[byte_i] |= (val & mask) << bit_i;
}

static inline u8 sign_extend(u8 val, u32 bit_len)
{
  int shift = 8 - bit_len;
  return (s8)(val << shift) >> shift;
}

static inline u8 decompress_one_data(
    u8 data[], u64 i, compression_info_t *info)
{
  u32 bit_len = info->bit_length;
  u32 data_per_byte = 8 / bit_len;

  u32 byte_i = i / data_per_byte;
  u32 bit_i = (i % data_per_byte) * bit_len;
  u8 mask = (1 << bit_len) - 1;

  u8 val = (data[byte_i] >> bit_i) & mask;
  if (info->is_signed)
    val = sign_extend(val, bit_len);

  return val;
}

static inline void fill_data(u8 data[], u8 buf[], compression_info_t *info)
{
  int min = info->compressed_min;
  int max = info->compressed_max;

  for (u64 i = 0; i < info->data_bytes; i++)
    data[i] = 0;

  u64 nz_i = 0;
  for (u64 i = 0; i < info->total_data_num; i++) {
    int val = info->is_signed? (s8)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0) {
      compress_one_data(data, nz_i, res, info);
      nz_i++;
    }
  }
}

static inline compression_info_t make_compression_info(
    u8 buf[], u64 size, u32 compress_md, int is_signed)
{
  u32 bit_length = compression_bit_length(compress_md);

  int min, max;
  compute_compressed_range(bit_length, is_signed, &min, &max);

  u32 nz_num = count_non_zero_results(buf, size, is_signed, max, min);
  assert(nz_num <= 0xffffff);

  compression_info_t info;
  info.compress_md = compress_md;
  info.bit_length = bit_length;
  info.is_signed = is_signed;
  info.total_data_num = size;
  info.non_zero_data_num = nz_num;
  info.header_bytes = 16;
  info.map_bytes = compression_map_bytes(size);
  info.data_bytes = compression_data_bytes(nz_num, bit_length);
  info.total_bytes = info.header_bytes + info.map_bytes + info.data_bytes;
  info.compressed_min = min;
  info.compressed_max = max;
  return info;
}

static inline compression_info_t parse_compression_info(
    u8 compressed_buf[], u64 max_size, u64 total_data_num)
{
  u64 header_bytes = 16;
  assert(header_bytes <= max_size);

  int is_signed;
  u32 compress_md, nz_num;
  parse_header(*(u32 *)compressed_buf, &is_signed, &compress_md, &nz_num);

  u32 bit_length = compression_bit_length(compress_md);
  int min, max;
  compute_compressed_range(bit_length, is_signed, &min, &max);

  compression_info_t info;
  info.compress_md = compress_md;
  info.bit_length = compression_bit_length(compress_md);
  info.is_signed = is_signed;
  info.total_data_num = total_data_num;
  info.non_zero_data_num = nz_num;
  info.header_bytes = header_bytes;
  info.map_bytes = compression_map_bytes(total_data_num);
  info.data_bytes = compression_data_bytes(nz_num, info.bit_length);
  info.total_bytes = header_bytes + info.map_bytes + info.data_bytes;
  info.compressed_min = min;
  info.compressed_max = max;

  assert(info.total_bytes <= max_size);

  return info;
}

static inline u8 * compress(
    u8 buf[], u64 size, u32 compress_md, int is_signed, compress_addr_info *compressed_data)
{
  compression_info_t info =
      make_compression_info(buf, size, compress_md, is_signed);

  assert(info.total_bytes < 0x100000);
  static u8 *result = (u8 *)malloc(sizeof(u8) * 0x100000);
  u32 *hdr = (u32 *)result;
  u8 *map = &result[info.header_bytes];
  u8 *data = &map[info.map_bytes];

  fill_header(hdr, &info);
  fill_map(map, buf, &info);
  if (info.bit_length != 1)
    fill_data(data, buf, &info);

  compressed_data->header_offset = 0;
  compressed_data->header_size = 4;
  compressed_data->map_offset = info.header_bytes;
  compressed_data->map_size = compression_map_clear_bytes(info.total_data_num);
  compressed_data->data_offset = info.map_bytes + info.header_bytes;
  compressed_data->data_size = info.data_bytes;
  compressed_data->total_size = info.total_bytes;

  return result;
}

static inline void decompress(
    u8 buf[], u64 size, u8 compressed_buf[], u64 max_size)
{
  compression_info_t info =
      parse_compression_info(compressed_buf, max_size, size);
  assert(info.total_bytes <= max_size);
  assert(info.total_data_num == size);

  u8 *map = &compressed_buf[info.header_bytes];
  if (info.bit_length == 1) {
    for (u64 i = 0; i < size; i++) {
      u8 val = read_map_bit(map, i);
      buf[i] = info.is_signed? sign_extend(val, 1): val;
    }
  } else {
    u8 *data = &map[info.map_bytes];
    u64 data_i = 0;
    for (u64 i = 0; i < size; i++) {
      u8 val = read_map_bit(map, i);
      if (val == 0) {
        buf[i] = 0;
      } else {
        buf[i] = decompress_one_data(data, data_i, &info);
        data_i++;
      }
    }
  }
}

#endif /* COMPRESSION_H */
