#include "kernel_internal.h"

void engine_state_init(engine_state_t *es, uint32_t nr_engines)
{
  es->nr_engines = nr_engines;
  es->last_desc = xmalloc(nr_engines * sizeof(es->last_desc[0]));
  engine_state_reset(es);
}

void engine_state_reset(engine_state_t *es)
{
  for (uint32_t ei = 0; ei < es->nr_engines; ei++)
    es->last_desc[ei] = NULL;
}

void engine_state_copy(engine_state_t *dst, engine_state_t *src)
{
  engine_state_init(dst, src->nr_engines);
  for (uint32_t ei = 0; ei < src->nr_engines; ei++)
    dst->last_desc[ei] = src->last_desc[ei];
}

void engine_state_update(engine_state_t *es, ec_desc_t *d)
{
  es->last_desc[d->engine_id] = d;
}

void engine_state_destroy(engine_state_t *es)
{
  es->nr_engines = 0;
  free(es->last_desc);
  es->last_desc = NULL;
}
