#ifndef CVIKERNEL_ENGINE_STATE_H
#define CVIKERNEL_ENGINE_STATE_H

//#include "kernel_internal.h"

typedef struct {
  uint32_t nr_engines;
  ec_desc_t **last_desc;
} engine_state_t;

void engine_state_init(engine_state_t *es, uint32_t nr_engines);
void engine_state_update(engine_state_t *es, ec_desc_t *d);
void engine_state_copy(engine_state_t *dst, engine_state_t *src);
void engine_state_reset(engine_state_t *es);
void engine_state_destroy(engine_state_t *es);

#endif /* CVIKERNEL_ENGINE_STATE_H */
