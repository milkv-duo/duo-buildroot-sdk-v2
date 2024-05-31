#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/quant.hpp>

namespace cvi {
namespace runtime {

static inline signed char float2int8(float v)
{
    int int32 = std::round(v);
    if (int32 > 127) return 127;
    if (int32 < -128) return -128;
    return (signed char)int32;
}

void QuantFunc::setup(tensor_list_t &inputs,
                      tensor_list_t &outputs,
                      OpParam &param) {
  _bottom = inputs[0];
  _top = outputs[0];
  if (param.has("scale")) {
    _scale = param.get<float>("scale");
  }
  if (param.get<std::string>("to") == "NONE") {
    _dequant = true;
    if (param.has("threshold")) {
      _scale = param.get<float>("threshold") / 128.0f;
    }
  #if __arm__
    work_buf = (int*)aligned_alloc(32, 1024 * sizeof(int));
    assert(work_buf && "failed to allocate buffer for dequant");
  #endif
  } else {
    if (param.has("threshold")) {
      _scale = 128.0f / param.get<float>("threshold");
    }
  }
}

void QuantFunc::run() {
  if (_dequant) {
    dequantToFp32();
  } else {
    quantFromFp32();
  }
}

void QuantFunc::dequantToFp32() {
  auto top_data = _top->cpu_data<float>();
  if (_bottom->fmt == CVI_FMT_INT8) {
    auto bottom_data = _bottom->cpu_data<int8_t>();
    float scale = _scale;
#if (__arm__ || __aarch64__)
    int total = (int)_bottom->count();
#if __aarch64__
    int8_t *ptr = bottom_data;
    float* outptr = top_data;
    // all neuron memory size is aligned to 16 or 32 bytes,
    // it's safe to compute more than needed.
    int nn = (total + 7) / 8;
    if (nn > 0) {
      asm volatile(
          "dup    v2.4s, %w6                   \n" // scale
          "dup    v7.4s, %w7                   \n"
          "0:                                  \n"
          "prfm   pldl1keep, [%1, #128]        \n"
          "ld1    {v8.8b}, [%1], #8            \n"
          "saddl  v9.8h, v8.8b, v7.8b          \n"
          "saddl  v0.4s, v9.4h, v7.4h          \n"
          "saddl2 v1.4s, v9.8h, v7.8h          \n"
          // top_s32 -> top_f32
          "scvtf  v5.4s, v0.4s                 \n"
          "scvtf  v6.4s, v1.4s                 \n"
          // top_f32 = top_f32 * scale_out
          "fmul   v5.4s, v5.4s, v2.4s          \n"
          "fmul   v6.4s, v6.4s, v2.4s          \n"
          // save top_f32
          "st1    {v5.4s, v6.4s}, [%2], #32    \n"
          "subs   %w0, %w0, #1                 \n"
          "bne    0b                           \n"
          : "=r"(nn),         // %0
            "=r"(ptr),     // %1
            "=r"(outptr)         // %2
          : "0"(nn),
            "1"(ptr),
            "2"(outptr),
            "r"(scale),        // %6
            "r"(0)
          : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
      );
    }
#else
    int size = 0;
    for (int offset = 0; offset < total; offset += size) {
      size = (total - offset) < 1024 ?
             (total - offset) : 1024;
      for (int i = 0; i < size; ++i) {
        work_buf[i] = (int)bottom_data[offset + i];
      }
      int *ptr = work_buf;
      float *outptr = top_data + offset;

      // all neuron memory size is aligned to 16 or 32 bytes,
      // it's safe to compute more than needed.
      int nn = (size + 7) / 8;
      if (nn > 0) {
        asm volatile(
            "pld        [%1, #256]          \n"
            "vld1.s32   {d0-d3}, [%1]!      \n" //q0-q1 data
            "vdup.f32   q10, %6             \n" //q10 scale

            "0:                             \n"
            "vcvt.f32.s32 q0, q0            \n"
            "vcvt.f32.s32 q1, q1            \n"

            "vmul.f32   q2,q0,q10           \n"
            "vmul.f32   q3,q1,q10           \n"

            "pld        [%1, #256]          \n"
            "vld1.s32   {d0-d3}, [%1]!      \n"
            "vst1.f32   {d4-d7}, [%2]!      \n"

            "subs       %0, #1              \n"
            "bne        0b                  \n"

            "sub        %1, #32             \n"
            : "=r"(nn),         // %0
              "=r"(ptr),     // %1
              "=r"(outptr)         // %2
            : "0"(nn),
              "1"(ptr),
              "2"(outptr),
              "r"(scale)        // %6
            : "cc", "memory", "q0", "q1", "q2", "q3", "q10", "q12"
        );
      }
    }
#endif
#else
    for (int i = 0; i < (int)_bottom->count(); ++i) {
      top_data[i] = bottom_data[i] * scale;
    }
#endif // (__arm__ || __aarch64__)
  } else if (_bottom->fmt == CVI_FMT_BF16) {
    uint16_t *p = _bottom->cpu_data<uint16_t>();
    uint16_t *q = reinterpret_cast<uint16_t *>(top_data);
    int size = (int)_bottom->count();
    for (; size != 0; p++, q += 2, size--) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      q[0] = *p;
      q[1] = 0;
#else
      q[0] = 0;
      q[1] = *p;
#endif
    }
  } else {
    assert(0);
  }
}

void QuantFunc::quantFromFp32() {
  auto bottom_data = _bottom->cpu_data<float>();
  if (_top->fmt == CVI_FMT_INT8) {
    auto top_data = _top->cpu_data<int8_t>();
    float scale = _scale;
#if (__arm__ || __aarch64__)
    int size = (int)_bottom->count();
    const float* ptr = bottom_data;
    signed char* outptr = top_data;

    int nn = (size + 7) / 8;
    if (nn > 0) {
#if __aarch64__
      asm volatile(
          "dup    v2.4s, %w6                   \n" //scale
          "0:                                  \n"
          "prfm   pldl1keep, [%1, #128]        \n"
          "ld1    {v0.4s, v1.4s}, [%1], #32    \n" //data
          // bottom_f32 = bottom_f32 * scale
          "fmul   v3.4s, v0.4s, v2.4s          \n"
          "fmul   v4.4s, v1.4s, v2.4s          \n"
          // top_f32 -> top_s32
          "fcvtas v5.4s, v3.4s                 \n"
          "fcvtas v6.4s, v4.4s                 \n"
          // top_s32 -> top_s16
          "sqxtn  v7.4h, v5.4s                 \n"
          "sqxtn2 v7.8h, v6.4s                 \n"
          // top_s16 -> top_s8
          "sqxtn  v8.8b, v7.8h                 \n"
          // save top_s8
          "st1    {v8.8b}, [%2], #8            \n"
          "subs   %w0, %w0, #1                 \n"
          "bne    0b                           \n"
          : "=r"(nn),       // %0
            "=r"(ptr),      // %1
            "=r"(outptr)    // %2
          : "0"(nn),
            "1"(ptr),
            "2"(outptr),
            "r"(scale)      // %6
          : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8"
      );
#else
      asm volatile(
          "pld        [%1, #256]          \n"
          "vld1.f32   {d0-d3}, [%1]!      \n"
          "vdup.32    q10, %6             \n"

          "0:                             \n"
          "vmul.f32   q0,q0,q10           \n"
          "vmul.f32   q1,q1,q10           \n"

          "vcvtr.s32.f32 s0,s0            \n"
          "vcvtr.s32.f32 s1,s1            \n"
          "vcvtr.s32.f32 s2,s2            \n"
          "vcvtr.s32.f32 s3,s3            \n"
          "vcvtr.s32.f32 s4,s4            \n"
          "vcvtr.s32.f32 s5,s5            \n"
          "vcvtr.s32.f32 s6,s6            \n"
          "vcvtr.s32.f32 s7,s7            \n"

          "vqmovn.s32 d4,q0               \n"
          "vqmovn.s32 d5,q1               \n"

          "pld        [%1, #256]          \n"
          "vld1.f32   {d0-d3}, [%1]!      \n"

          "vqmovn.s16 d4, q2              \n"
          "vst1.8     {d4}, [%2]!         \n"

          "subs       %0, #1              \n"
          "bne        0b                  \n"

          "sub        %1, #32             \n"
          : "=r"(nn),         // %0
            "=r"(ptr),        // %1
            "=r"(outptr)      // %2
          : "0"(nn),
            "1"(ptr),
            "2"(outptr),
            "r"(scale)        // %6
          : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q10", "q11"
      );
#endif
    }
#else
    for (int i = 0; i < (int)_bottom->count(); ++i) {

      float fval = bottom_data[i] * scale;
      //int ival = (int)(std::floor(fval + 0.5));
      int ival = float2int8(fval);
      if (ival > 127) {
        top_data[i] = 127;
      } else if (ival < -128) {
        top_data[i] = -128;
      } else {
        top_data[i] = (int8_t)ival;
      }
    }
#endif // (__arm__ || __aarch64__)
  } else if (_top->fmt == CVI_FMT_BF16) {
    auto top_data = _top->cpu_data<uint16_t>();
    for (int i = 0; i < (int)_bottom->count(); ++i) {
      float val = bottom_data[i] * 1.001957f;
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      top_data[i] = ((uint16_t *)(&val))[0];
#else
      top_data[i] = ((uint16_t *)(&val))[1];
#endif
    }
  } else {
    assert(0);
  }
}

} // namespace runtime
} // namespace cvi
