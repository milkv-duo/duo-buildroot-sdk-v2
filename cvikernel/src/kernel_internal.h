#ifndef __INST_INTERNAL_H__
#define __INST_INTERNAL_H__

#define ASSERT(cond)                                    \
  do {                                                  \
    if (cond) {                                         \
      /* To catch warnings in `cond' */;                \
    } else {                                            \
      fprintf(stderr,                                   \
              "error: %s: line %d: function %s: "       \
              "assertion `%s' failed\n",                \
              __FILE__, __LINE__, __func__, #cond);     \
      abort();                                          \
    }                                                   \
  } while (0)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <bmkernel/bm_kernel_legacy.h>
#include <bmkernel/bm_regcpu.h>
#include "engine_conductor.h"
#include "engine_state.h"
#include "mode_manager.h"

#ifdef assert
#error "Please don't use assert. Use ASSERT instead."
#endif

#define KiB (1 << 10)
#define MiB (1 << 20)
#define GiB (1 << 30)

typedef struct {
  cmd_hdr_t *cmd_hdr;
  ec_desc_t *ec_desc;
} desc_pair_t;

static inline void * xmalloc(size_t size)
{
  void *p = malloc(size);
  ASSERT(p);
  return p;
}

static inline int bitsize_of_fmt(fmt_t fmt)
{
  switch (fmt) {
    case FMT_F32:
    case FMT_I32:
      return 32;
    case FMT_F16:
    case FMT_I16:
    case FMT_U16:
    case FMT_BF16:
      return 16;
    case FMT_I8:
    case FMT_U8:
      return 8;
    case FMT_I4:
      return 4;
    case FMT_I2:
      return 2;
    case FMT_I1:
      return 1;
    default:
      ASSERT(0);
      return -1;
  }
}

static inline int ceiling_bytesize_of(int bitsize)
{
  return ceiling_func_shift(bitsize, 3);
}

static inline int ceiling_bytesize_of_array(int data_count, fmt_t fmt)
{
  int bitsize = bitsize_of_fmt(fmt);
  return ceiling_bytesize_of(data_count * bitsize);
}

static inline void replace_u32(uint32_t *data, uint32_t start, uint32_t width, uint32_t value)
{
  ASSERT(start < 32);
  ASSERT(width > 0 && width <= 32);
  ASSERT((start + width) <= 32);

  uint32_t mask = ~(((1 << width) - 1) << start);
  *data = (*data & mask) | (value << start);
}

#endif /* __INST_INTERNAL_H__ */
