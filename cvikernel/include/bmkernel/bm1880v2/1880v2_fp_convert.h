#ifndef ATOMIC_FP_H_
#define ATOMIC_FP_H_

#if __arm__
#define __DISABLE_FENV__
#endif

#ifndef __DISABLE_FENV__
#include <fenv.h>
#endif
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t convert_bf16_u8(uint16_t data);
static inline uint8_t _convert_bf16_u8(uint16_t data, int int8_rnd_md);
static inline int8_t _convert_bf16_s8(uint16_t data, int int8_rnd_md);
static inline int8_t convert_bf16_s8(uint16_t data);
static inline uint16_t convert_int8_bf16(uint8_t data, uint8_t sign);
static inline uint32_t convert_fp32_u32(float fp32);
static inline uint32_t convert_fp32_hex(float val);
static inline float convert_hex_fp32(uint32_t hval);

static inline float convert_bf16_fp32(uint16_t bf16);
static inline uint16_t convert_fp32_bf16(float fp32);

static inline void f32_integer(void *if32, void *o_integer, int integer_size, int accumulate, int int8_signed, int int8_rnd_md);
//static inline void f32_integer(void *if32, void *o_integer,
                 // 0 for 32 bit , 1 for 16 bit , 2 for 8 bit
//                 int integer_size, int accumulate = 0, int int8_signed = 1, int int8_rnd_md = 0);

union convert_type_float {
    float fval;
    uint16_t bf16[2];
    uint32_t ival;
};

typedef union convert_type_float convert_int_float;
static const uint16_t NAN_VALUE = 0x7FC0;

//static int round_mode = 0;
static uint8_t float_isnan(const float x) {
    //return isnan(x);
    return x != x;
}

static inline int set_store_feround()
{
#ifndef __DISABLE_FENV__
  int round_mode = fegetround();
  fesetround(FE_TOWARDZERO);
  return round_mode;
#else
  return 0;
#endif
}

static inline void restore_feround(int round_mode)
{
#ifndef __DISABLE_FENV__
  fesetround(round_mode);
#else
  (void)round_mode;
#endif
}

static inline uint8_t _convert_bf16_u8(uint16_t data, int int8_rnd_md)
{
  /* convert bf16 to float32*/
  float fp32;
  convert_int_float convert_val;
  fp32 = convert_bf16_fp32(data);
  /* convert float32 to uint8_t*/
  f32_integer((void*)&fp32, &convert_val.ival, 2, 0, 0, int8_rnd_md);
  return (uint8_t) convert_val.ival;
}

static inline uint8_t convert_bf16_u8(uint16_t data)
{
  return (uint8_t) _convert_bf16_u8(data, 0);
}

static inline int8_t _convert_bf16_s8(uint16_t data, int int8_rnd_md)
{
  /* convert bf16 to float32*/
  float fp32;
  convert_int_float convert_val;
  fp32 = convert_bf16_fp32(data);
  /* convert float32 to uint8_t*/
  f32_integer((void*)&fp32, &convert_val.ival, 2, 0, 1, int8_rnd_md);
  return (int8_t) convert_val.ival;
}

static inline int8_t convert_bf16_s8(uint16_t data)
{
  return (int8_t) _convert_bf16_s8(data, 0);
}

static inline uint16_t convert_int8_bf16(uint8_t data, uint8_t sign)
{
  int32_t val = sign ? (int8_t) data : (uint8_t) data;
  /* need to round to bf16 mode */
  return convert_fp32_bf16((float) val);
}

static inline uint16_t convert_fp32_bf16(float fp32)
{
  if (float_isnan(fp32))
    return NAN_VALUE;
  convert_int_float convert_val;
  convert_val.fval = fp32;
  uint32_t input = convert_val.ival;
  uint32_t lsb = (input >> 16) & 1;
  uint32_t rounding_bias = 0x7fff + lsb;
  input += rounding_bias;
  convert_val.bf16[1] = (uint16_t) (input >> 16);

  /* HW behavior */
  if ((convert_val.bf16[1] & 0x7f80) == 0x7f80) {
    convert_val.bf16[1] = 0x7f7f;
  }
  return convert_val.bf16[1];
}

static inline uint8_t convert_fp32_u8(float fp32)
{
  convert_int_float convert_val;
  f32_integer((void*)&fp32, &convert_val.ival, 2, 0, 0, 0);
  return (uint8_t) convert_val.ival;
}

static inline int8_t convert_fp32_s8(float fp32)
{
  convert_int_float convert_val;
  f32_integer((void*)&fp32, &convert_val.ival, 2, 0, 1, 0);
  return (int8_t) convert_val.ival;
}

static inline uint32_t convert_fp32_u32(float fp32)
{
  convert_int_float convert_val;
  f32_integer((void*)&fp32, &convert_val.ival, 0, 0, 0, 0);
  return (uint32_t) convert_val.ival;
}

static inline int32_t convert_fp32_s32(float fp32)
{
  convert_int_float convert_val;
  f32_integer((void*)&fp32, &convert_val.ival, 0, 0, 1, 0);
  return (int32_t) convert_val.ival;
}

/* convert hex to float directly */
static inline float convert_hex_fp32(uint32_t hval)
{
  convert_int_float convert_val;
  convert_val.ival = hval;
  return convert_val.fval;
}
/* convert float to hex directly */
static inline uint32_t convert_fp32_hex(float val)
{
  convert_int_float convert_val;
  convert_val.fval = val;
  return convert_val.ival;
}
static inline float convert_bf16_fp32(uint16_t bf16)
{
  convert_int_float convert_val;
  convert_val.bf16[1] = bf16;
  convert_val.bf16[0] = 0;
  return convert_val.fval;
}

static inline void flt2int_flt(float x, unsigned long long* integer_part, float * sub_part, uint8_t sign)
{
    convert_int_float work_x;
    int  level_code;
    unsigned long tail_code;
    work_x.fval = x;
    level_code = ((work_x.ival >> 23) & 0xff) - 127;

    //if the level code is negaive, the integer part of the float is zero
    if ( level_code < 0 ){
        *integer_part = 0;
        *sub_part = x;
    }
    else {
        tail_code = (work_x.ival) & 0x7fffff;
        tail_code = tail_code | 0x800000;

        if (level_code < 23){
            tail_code >>= (23 - level_code);
            *integer_part = tail_code;
            work_x.ival &= 0xffffffff << (23 - level_code);
            *sub_part = x - work_x.fval;
        }
        else {
            tail_code <<= (level_code - 23);
            *integer_part = tail_code;
            if(level_code>30){
              *integer_part = 0x7fffffff;
              if(sign)*integer_part = 0x800000000;
            }
            *sub_part = 0;
        }
    }
}

inline static int flt2int(float ifval, int int8_rnd_md)
{
    union {
      float floatNum;
      unsigned long intNum;
    } tempIfval;
    tempIfval.floatNum = ifval;
    uint8_t isPositive = ((tempIfval.intNum & 0x80000000UL) == 0x80000000UL) ? 0 : 1 ;
    float abs_fval = (!isPositive) ? -ifval : ifval;
    double sub_part;
    double integer;
    unsigned long long integer_part;
    //uint8_t   sign = !isPositive;
    //flt2int_flt(abs_fval, &integer_part, &sub_part, sign);
    sub_part = modf((double)abs_fval, &integer); 
    integer_part = (unsigned long long)integer;
    if (!isPositive)
    {
        unsigned long long result;
        if(int8_rnd_md == 0) { // round to nearest even
          if ( sub_part > 0.5f )
          {
              result = integer_part + 1;
          }
          else if (sub_part == 0.5f)
          {
              if ( integer_part & 0x1 )
              {
                  result = integer_part + 1;
              }
              else
              {
                  result = integer_part;
              }
          }
          else
          {
              result = integer_part;
          }
        } else { //round to zero
          result = integer_part;
        }
        if ( result > 0x80000000UL )
        {
            result = 0x80000000UL;
        }
        return -result;
    }
    else
    {
        unsigned long long result;
        if(int8_rnd_md == 0) { // round to nearest even
          if ( sub_part > 0.5f )
          {
              result = integer_part + 1;
          }
          else if ( sub_part == 0.5f )
          {
              if ( integer_part & 0x1 )
              {
                  result = integer_part + 1;
              }
              else
              {
                  result = integer_part;
              }
          }
          else
          {
              result = integer_part;
          }
        } else {
          result = integer_part;
        }
        if ( result > 0x7fffffff )
        {
            result = 0x7fffffff;
        }
        return result;
    }
}

static inline void f32_integer(void *if32, void *o_integer, int integer_size, int accumulate, int int8_signed, int int8_rnd_md)
{
  int i_tmp;
  float *f_tmp;
  f_tmp = (float *)if32;
  i_tmp = flt2int(*f_tmp, int8_rnd_md);
  int *o32 = (int *)o_integer;
  int dst_f32 = *o32;
  short *o16 = (short *)o_integer;
  short dst_o16 = *o32;
  char *o8 = (char *)o_integer;
  char dst_o8 = *o8;

  if (integer_size == 0) {
    *o32 = i_tmp;
  } else if (integer_size == 1) {
    *o16 = i_tmp;
  } else{
    *o8 = i_tmp;
    int min = (int8_signed) ? -128 : 0;
    int max = (int8_signed) ? 127 : 255;
    if (i_tmp < min ){
        *o8 = min;
    }
    else if (i_tmp > max){
        *o8 =  max;
    }
    //*o8 = i_tmp;
  }
  if (accumulate) {
    if (integer_size == 0) {
      *o32 += dst_f32;
    } else if (integer_size == 1) {
      *o16 += dst_o16;
    } else
      *o8 += dst_o8;
  }
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMIC_FP_H_ */

