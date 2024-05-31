#include "kernel_internal.h"

void serial_mode_init(serial_mode_t *m, engine_state_t *es, ec_t *ec)
{
  engine_state_copy(&m->engine_state, es);
  m->ec = ec;
}

void serial_mode_record_desc(serial_mode_t *m, ec_desc_t *d)
{
  uint32_t nr_engines = m->engine_state.nr_engines;

  for (uint32_t i = 0; i < nr_engines; i++) {
    ec_desc_t *before = m->engine_state.last_desc[i];
    if (before)
      ec_add_dependency(m->ec, before, d);
  }

  // 1st in mode_manager_record_ec_desc() updates last_desc of mode manager.
  // This one updates last_desc of serial model
  // The only one difference compared to parallel mode.
  engine_state_update(&m->engine_state, d);
}

void serial_mode_destroy(serial_mode_t *m)
{
  engine_state_destroy(&m->engine_state);
}
