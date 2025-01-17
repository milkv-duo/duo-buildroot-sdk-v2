#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "ive.h"

#define TEST_W 1920
#define TEST_H 1080

static const uint16_t NAN_VALUE = 0x7FC0;

union convert_type_float {
  float fval;
  uint16_t bf16[2];
  uint32_t ival;
};

typedef union convert_type_float convert_int_float;

// static int round_mode = 0;
static uint8_t float_isnan(const float x) { return x != x; }

uint16_t convert_fp32_bf16(float fp32) {
  if (float_isnan(fp32)) return NAN_VALUE;
  convert_int_float convert_val;
  convert_val.fval = fp32;
  uint32_t input = convert_val.ival;
  uint32_t lsb = (input >> 16) & 1;
  uint32_t rounding_bias = 0x7fff + lsb;
  input += rounding_bias;
  convert_val.bf16[1] = (uint16_t)(input >> 16);

  /* HW behavior */
  if ((convert_val.bf16[1] & 0x7f80) == 0x7f80) {
    convert_val.bf16[1] = 0x7f7f;
  }
  return convert_val.bf16[1];
}

void convert_image_to_bf16(IVE_IMAGE_S *u8c1, IVE_IMAGE_S *bf16) {
  uint32_t stride_bf16 = bf16->u16Stride[0] / 2;
  uint32_t stride_u8c1 = u8c1->u16Stride[0];
  CVI_U16 *bf16_data = (CVI_U16 *)bf16->pu8VirAddr[0];
  CVI_U8 *u8c1_data = u8c1->pu8VirAddr[0];
  for (int j = 0; j < u8c1->u16Height; j++) {
    for (int i = 0; i < u8c1->u16Width; i++) {
      float value = (float)u8c1_data[i + j * stride_u8c1];
      if ((i * j) % 2 == 0) {
        value *= -1.0;
      }
      bf16_data[i + j * stride_bf16] = convert_fp32_bf16(value);
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect parameter. Usage: %s <image>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name1 = argv[1];

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();

  IVE_IMAGE_S src_u8c1 = CVI_IVE_ReadImage(handle, file_name1, IVE_IMAGE_TYPE_U8C1);
  int width = src_u8c1.u16Width;
  int height = src_u8c1.u16Height;

  IVE_IMAGE_S src, dst;
  CVI_IVE_CreateImage(handle, &src, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C1, width, height);

  // convert image to bf16 and multiply -1.0 every two pixels.
  convert_image_to_bf16(&src_u8c1, &src);

  printf("Run TPU convert scale.\n");
  IVE_CONVERT_SCALE_ABS_CRTL_S ctrl;
  ctrl.alpha = 1.1;
  ctrl.beta = 0.0;
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  // saturate_uint8(abs(alpha * I + beta)).
  CVI_S32 ret = CVI_IVE_ConvertScaleAbs(handle, &src, &dst, &ctrl, 0);

  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);

  CVI_IVE_BufRequest(handle, &dst);

  printf("elapsed_tpu: %lu\n", elapsed_tpu);
  CVI_IVE_WriteImage(handle, "sample_convert_scale_abs_source.png", &src_u8c1);
  CVI_IVE_WriteImage(handle, "sample_convert_scale_abs_dest.png", &dst);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src_u8c1);
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dst);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}