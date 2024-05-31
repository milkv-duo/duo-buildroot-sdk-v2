/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 */
#ifndef MYADD_OP_H_
#define MYADD_OP_H_

#include <cvikernel/cvikernel.h>
#include <vector>

class MyAddOp {
public:
  MyAddOp(cvk_context_t * ctx) : ctx(ctx) {}
  void codeGenBf16(std::vector<int64_t> shape);

private:
  typedef struct tiling_info {
    uint32_t n;
    uint32_t c;
    uint32_t h;
    uint32_t w;
    uint64_t offset; // gmem offset
  } tiling_info_t;
  cvk_context_t * ctx;
  std::vector<tiling_info_t> tiles;
  void tiling(int64_t total);

};

#endif
