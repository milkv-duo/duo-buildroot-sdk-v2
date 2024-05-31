#include "kernel_internal.h"

void parallel_mode_init(parallel_mode_t *m, engine_state_t *es, ec_t *ec)
{
  engine_state_copy(&m->engine_state, es);
  m->ec = ec;
}

void parallel_mode_record_desc(parallel_mode_t *m, ec_desc_t *d)
{
  uint32_t nr_engines = m->engine_state.nr_engines;

  for (uint32_t i = 0; i < nr_engines; i++) {
    ec_desc_t *before = m->engine_state.last_desc[i];
    if (before)
      ec_add_dependency(m->ec, before, d);
  }
}

void parallel_mode_destroy(parallel_mode_t *m)
{
  engine_state_destroy(&m->engine_state);
}
