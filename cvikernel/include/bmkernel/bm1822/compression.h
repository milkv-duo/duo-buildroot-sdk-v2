#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <assert.h>

typedef struct {
  uint32_t compress_md;
  uint32_t bit_length;
  int is_signed;

  uint64_t total_data_num;
  uint32_t non_zero_data_num;

  uint64_t header_bytes;
  uint64_t map_bytes;
  uint64_t data_bytes;
  uint64_t total_bytes;

  int compressed_min;
  int compressed_max;
} compression_info_t;

typedef struct {
  uint64_t header_offset;
  uint64_t header_size;
  uint64_t map_offset;
  uint64_t map_size;
  uint64_t data_offset;
  uint64_t data_size;
  uint64_t total_size;
} compress_addr_info;

static uint64_t compression_map_bytes(uint64_t total_data_num)
{
  uint64_t bit_alignment = 16 * 8;
  uint64_t bits = total_data_num;

  return ceiling_func(bits, bit_alignment)*16;
}

static uint64_t compression_map_clear_bytes(uint64_t total_data_num)
{
  uint64_t bit_alignment = 2 * 8;
  uint64_t bits = total_data_num;

  return ceiling_func(bits, bit_alignment)*2;
}


static uint64_t compression_data_bytes(uint64_t non_zero_data_num, uint32_t bit_length)
{
  if (bit_length == 1)
    return 0;

  uint64_t bit_alignment = 8;
  uint64_t bits = non_zero_data_num * bit_length;

  return ceiling_func(bits, bit_alignment);
}

static inline uint32_t compression_bit_length(uint32_t compress_md)
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
    uint32_t bit_length, int is_signed, int *min, int *max)
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

static inline uint64_t count_non_zero_results(
    uint8_t buf[], uint64_t size, int is_signed, int max, int min)
{
  uint64_t n = 0;

  for (uint64_t i = 0; i < size; i++) {
    int val = is_signed? (int8_t)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0)
      n++;
  }

  return n;
}

static inline void set_map_bit(uint8_t map[], uint64_t i)
{
  uint64_t byte_i = i / 8;
  uint64_t bit_i = i % 8;

  map[byte_i] |= (1 << bit_i);
}

static inline uint8_t read_map_bit(uint8_t map[], uint64_t i)
{
  uint64_t byte_i = i / 8;
  uint64_t bit_i = i % 8;

  return (map[byte_i] >> bit_i) & 1;
}

static inline void parse_header(
    uint32_t header, int *is_signed, uint32_t *compress_md, uint32_t *nz_num)
{
  *is_signed = (header >> 29) & 1;
  *compress_md = (header >> 24) & 0b11;
  *nz_num = header & 0xffffff;
}

static inline void fill_header(uint32_t *hdr, compression_info_t *info)
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

static inline void fill_map(uint8_t map[], uint8_t buf[], compression_info_t *info)
{
  int min = info->compressed_min;
  int max = info->compressed_max;

  uint64_t clear_map = compression_map_clear_bytes(info->total_data_num);
  for (uint64_t i = 0; i < clear_map; i++)
    map[i] = 0;

  for (uint64_t i = 0; i < info->total_data_num; i++) {
    int val = info->is_signed? (int8_t)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0)
      set_map_bit(map, i);
  }
}

static inline void compress_one_data(
    uint8_t data[], uint64_t i, uint8_t val, compression_info_t *info)
{
  uint32_t bit_len = info->bit_length;
  uint32_t data_per_byte = 8 / bit_len;

  uint32_t byte_i = i / data_per_byte;
  uint32_t bit_i = (i % data_per_byte) * bit_len;
  uint8_t mask = (1 << bit_len) - 1;

  data[byte_i] |= (val & mask) << bit_i;
}

static inline uint8_t sign_extend(uint8_t val, uint32_t bit_len)
{
  int shift = 8 - bit_len;
  return (int8_t)(val << shift) >> shift;
}

static inline uint8_t decompress_one_data(
    uint8_t data[], uint64_t i, compression_info_t *info)
{
  uint32_t bit_len = info->bit_length;
  uint32_t data_per_byte = 8 / bit_len;

  uint32_t byte_i = i / data_per_byte;
  uint32_t bit_i = (i % data_per_byte) * bit_len;
  uint8_t mask = (1 << bit_len) - 1;

  uint8_t val = (data[byte_i] >> bit_i) & mask;
  if (info->is_signed)
    val = sign_extend(val, bit_len);

  return val;
}

static inline void fill_data(uint8_t data[], uint8_t buf[], compression_info_t *info)
{
  int min = info->compressed_min;
  int max = info->compressed_max;

  for (uint64_t i = 0; i < info->data_bytes; i++)
    data[i] = 0;

  uint64_t nz_i = 0;
  for (uint64_t i = 0; i < info->total_data_num; i++) {
    int val = info->is_signed? (int8_t)buf[i]: buf[i];
    int res = saturate(val, max, min);
    if (res != 0) {
      compress_one_data(data, nz_i, res, info);
      nz_i++;
    }
  }
}

static inline compression_info_t make_compression_info(
    uint8_t buf[], uint64_t size, uint32_t compress_md, int is_signed)
{
  uint32_t bit_length = compression_bit_length(compress_md);

  int min, max;
  compute_compressed_range(bit_length, is_signed, &min, &max);

  uint32_t nz_num = count_non_zero_results(buf, size, is_signed, max, min);
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
    uint8_t compressed_buf[], uint64_t max_size, uint64_t total_data_num)
{
  uint64_t header_bytes = 16;
  assert(header_bytes <= max_size);

  int is_signed;
  uint32_t compress_md, nz_num;
  parse_header(*(uint32_t *)compressed_buf, &is_signed, &compress_md, &nz_num);

  uint32_t bit_length = compression_bit_length(compress_md);
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

static inline uint8_t * compress(
    uint8_t buf[], uint64_t size, uint32_t compress_md, int is_signed, compress_addr_info *compressed_data)
{
  compression_info_t info =
      make_compression_info(buf, size, compress_md, is_signed);

  assert(info.total_bytes < 0x100000);
  static uint8_t *result = new uint8_t[0x100000];
  uint32_t *hdr = (uint32_t *)result;
  uint8_t *map = &result[info.header_bytes];
  uint8_t *data = &map[info.map_bytes];

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
    uint8_t buf[], uint64_t size, uint8_t compressed_buf[], uint64_t max_size)
{
  compression_info_t info =
      parse_compression_info(compressed_buf, max_size, size);
  assert(info.total_bytes <= max_size);
  assert(info.total_data_num == size);

  uint8_t *map = &compressed_buf[info.header_bytes];
  if (info.bit_length == 1) {
    for (uint64_t i = 0; i < size; i++) {
      uint8_t val = read_map_bit(map, i);
      buf[i] = info.is_signed? sign_extend(val, 1): val;
    }
  } else {
    uint8_t *data = &map[info.map_bytes];
    uint64_t data_i = 0;
    for (uint64_t i = 0; i < size; i++) {
      uint8_t val = read_map_bit(map, i);
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
