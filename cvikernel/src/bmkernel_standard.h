#ifndef BMKERNEL_STANDARD_H
#define BMKERNEL_STANDARD_H
#include <bmkernel/bm_kernel.h>
#include "kernel_internal.h"
#include <cvikernel/cvikernel.h>

typedef struct bmk_context {
  bmk_info_t info;
  cvk_chip_info_t chip_info;

  ec_t ec;
  mode_manager_t mode_manager;

  uint32_t cmdbuf_ptr;
  uint32_t max_nr_desc;
  uint32_t cur_nr_desc;
  desc_pair_t *desc_pairs;

  uint32_t lmem_ptr;
  uint16_t layer_id;
  void* op; //<! compress used
} bmk_context_t, ctx_t;

#endif // BMKERNEL_STANDARD_H
