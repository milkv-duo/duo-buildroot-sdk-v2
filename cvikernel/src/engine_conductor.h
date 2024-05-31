#ifndef ENGINE_CONDUCTOR_H
#define ENGINE_CONDUCTOR_H

#include <stdint.h>

// #include <bmkernel/bm_kernel.h>

// #define CVK_EC_DEBUG

typedef struct ec_desc {
  uint32_t engine_id;
  struct ec_desc **followers;
  uint16_t *sync_ids;

#ifdef CVK_EC_DEBUG
  // desc, follower and sync_ids are pointers.
  // It is easier to debug from address offset instead of pointers.
  uint32_t desc_offset;
  uint32_t followers_offset;
  uint32_t sync_ids_offset;
#endif

} ec_desc_t;

typedef struct {
  uint32_t nr_engines;

  uint32_t max_nr_desc;
  uint32_t cur_nr_desc;
  ec_desc_t *desc;

  ec_desc_t **follower_buf;
  uint16_t *sync_id_buf;
} ec_t;

void ec_init(ec_t *ec, uint32_t nr_engines, uint32_t max_nr_desc);
void ec_reset(ec_t *ec);
void ec_destroy(ec_t *ec);

ec_desc_t * ec_alloc_desc(ec_t *ec, uint32_t engine_id);

void ec_add_dependency(ec_t *ec, ec_desc_t *before, ec_desc_t *after);
void ec_compute_sync_ids(ec_t *ec);

#endif /* ENGINE_CONDUCTOR_H */
