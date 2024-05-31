#include "kernel_internal.h"

shape_t shape_t4(int n, int c, int h, int w)
{
  shape_t s;
  s.n = n;
  s.c = c;
  s.h = h;
  s.w = w;
  s.dim = 4;
  return s;
}

shape_t shape_t3(int c, int h, int w)
{
  shape_t s;
  s.n = 1;
  s.c = c;
  s.h = h;
  s.w = w;
  s.dim = 3;
  return s;
}

shape_t shape_t2(int row, int col)
{
  shape_t s;
  s.n = 1;
  s.c = 1;
  s.h = row;
  s.w = col;
  s.dim = 2;
  return s;
}

shape_t shape_t1(int len)
{
  int row = 1, col = len;
  while (col >= 65536) {
    ASSERT(col % 2 == 0);
    col /= 2;
    row *= 2;
  }
  shape_t s = {
    .dim = 2,
    .n = 1,
    .c = 1,
    .h = row,
    .w = col,
  };
  return s;
}

uint8_t shape_equal(shape_t s1, shape_t s2)
{
  return (s1.dim == s2.dim) &&
      (s1.n == s2.n) &&
      (s1.c == s2.c) &&
      (s1.h == s2.h) &&
      (s1.w == s2.w);
}

void tl_reshape(tensor_lmem *tlp, shape_t shape)
{
  ASSERT(tlp);
  tlp->shape = shape;
}
