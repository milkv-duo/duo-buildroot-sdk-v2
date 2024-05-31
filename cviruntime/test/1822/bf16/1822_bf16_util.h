#ifndef INC_1822_BF16_UTIL_H
#define INC_1822_BF16_UTIL_H

#define RAND_SEED_MOD 10
#define COMPARE_PASS 0

u16 corner_val[] = {
  0x0000, // 0 00000000 0000000 = zero
  0x8000, // 1 00000000 0000000 = −zero
  0x7f80, // 0 11111111 0000000 = infinity
  0xff80, // 1 11111111 0000000 = −infinity
  0x4049, // 0 10000000 1001001 = 3.140625 ≈ π ( pi )
  0x3eab, // 0 01111101 0101011 = 0.333984375 ≈ 1/3
  0xffc1, // x 11111111 1000001 => qNaN
  0xff81, // x 11111111 0000001 => sNaN
  0x00ff, // x 00000000 1111111 => denormal
};

u16 generate_bf16_corner_val(float val)
{
  if( rand()%RAND_SEED_MOD == 0 ) {
    return corner_val[ rand() % (sizeof(corner_val)/sizeof(u16)) ];
  } else {
    return convert_fp32_bf16(val);
  }
}

int compare_result( void *ref_x, void *result_x , fmt_t fmt, int stride_size)
{
  u8 *u8result_x = NULL;
  u16 *u16result_x = NULL;
  u8 *u8ref_x = NULL;
  u16 *u16ref_x = NULL;

  if(fmt == FMT_BF16) {
    u16result_x = (u16 *)result_x;
    u16ref_x = (u16 *)ref_x;
    for (int i = 0; i < stride_size; i++) {
      if (u16result_x[i] != u16ref_x[i]) {
        printf("compare failed at result_x[%d], got %d, exp %d\n", 
               i, u16result_x[i], u16ref_x[i]);
        return -1;
      }
    }
  } else {
    u8result_x = (u8 *)result_x;
    u8ref_x = (u8 *)ref_x;
    for (int i = 0; i < stride_size; i++) {
      if (u8result_x[i] != u8ref_x[i]) {
        printf("compare failed at result_x[%d], got %d, exp %d\n", 
               i, u8result_x[i], u8ref_x[i]);
        return -1;
      }
    }
  }

  return 0;
}

#endif /* INC_1822_BF16_UTIL_H */
