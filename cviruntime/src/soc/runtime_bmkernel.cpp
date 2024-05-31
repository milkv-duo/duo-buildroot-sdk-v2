#include <stdio.h>
#include <stdlib.h>
#include <bmruntime.h>
#include <bmruntime_bmkernel.h>
#include <runtime/debug.h>

void bmruntime_bmkernel_create(bmctx_t ctx, void **p_bk_ctx) {
  cviruntime_cvikernel_create(ctx, p_bk_ctx);
}

void bmruntime_bmkernel_submit(bmctx_t ctx) {
  cviruntime_cvikernel_submit(ctx);
}

void bmruntime_bmkernel_destroy(bmctx_t ctx) {
  cviruntime_cvikernel_destroy(ctx);
}

void bmruntime_bmkernel_submit_pio(bmctx_t ctx) {
  (void)ctx;
  TPU_ASSERT(0, NULL);
}
