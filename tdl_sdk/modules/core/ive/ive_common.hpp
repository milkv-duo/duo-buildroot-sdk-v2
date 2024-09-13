#pragma once

namespace ive {

enum ImageType {
  U8C1 = 0x0,
  S8C1 = 0x1,

  YUV420SP = 0x2,
  YUV422SP = 0x3,
  YUV420P = 0x4,
  YUV422P = 0x5,

  S8C2_PACKAGE = 0x6,
  S8C2_PLANAR = 0x7,

  S16C1 = 0x8,
  U16C1 = 0x9,

  U8C3_PACKAGE = 0xa,
  U8C3_PLANAR = 0xb,

  S32C1 = 0xc,
  U32C1 = 0xd,

  S64C1 = 0xe,
  U64C1 = 0xf,

  BF16C1 = 0x10,
  FP32C1 = 0x11,
};

enum SubMode {
  NORMAL = 0x0,
  ABS = 0x1,
  SHIFT = 0x2,
  ABS_THRESH = 0x3,
};

enum ThreshMode {
  BINARY = 0x0,
  TRUNC = 0x1,
  TO_MINVAL = 0x2,
  MIN_MID_MAX = 0x3,
  ORI_MID_MAX = 0x4,
  MIN_MID_ORI = 0x5,
  MIN_ORI_MAX = 0x6,
  ORI_MID_ORI = 0x7,
  SLOPE = 0x8,
};

enum DMAMode {
  DIRECT_COPY = 0x0,
  INTERVAL_COPY = 0x1,
  SET_3BYTE = 0x2,
  SET_8BYTE = 0x3,
};

}  // namespace ive