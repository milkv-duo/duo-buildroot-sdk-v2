#include "kernel_internal.h"

#define ENABLE_UPDATE_TDMA_WAIT_ID

#ifdef CVK_EC_DEBUG
char *get_engine_id_str(uint32_t engine_id)
{
  switch (engine_id) {
    case 0:
      return "TPU";
    case 1:
      return "CPU";
    case 2:
      return "TDMA";
    default:
    break;
  }

  return "UNK";
}

void dump_desc(ec_desc_t *desc)
{
  printf("              engine_id %d(%s)\n", desc->engine_id, get_engine_id_str(desc->engine_id));
  printf("              desc_offset %d\n", desc->desc_offset);
  printf("              followers_offset %d\n", desc->followers_offset);
  printf("              sync_ids_offset %d\n", desc->sync_ids_offset);

  if (desc->sync_ids)
    printf("              sync_ids [TPU] %d, [TDMA] %d\n", desc->sync_ids[0], desc->sync_ids[2]);
}
#endif /* CVK_EC_DEBUG */

static void ec_desc_init(ec_desc_t *d, uint32_t engine_id, uint32_t nr_engines)
{
  d->engine_id = engine_id;

  uint32_t nr_followers = nr_engines - 1;
  for (uint32_t i = 0; i < nr_followers; i++)
    d->followers[i] = NULL;

  for (uint32_t i = 0; i < nr_engines; i++)
    d->sync_ids[i] = 0;
}

static void add_follower(ec_desc_t *d, ec_desc_t *follower, uint32_t nr_engines)
{
  if (d->engine_id == follower->engine_id)
    return;

  uint32_t nr_followers = nr_engines - 1;
  for (uint32_t fi = 0; fi < nr_followers; fi++) {
    ec_desc_t **f = &d->followers[fi];
    if ((*f) == NULL) {
      *f = follower;
      return;
    } else if ((*f)->engine_id == follower->engine_id) {
      if ((*f) > follower)
        (*f) = follower;
      return;
    }
  }
  ASSERT(0 && "desc->followers[] overflowed");
}

static uint32_t assign_sync_ids(ec_desc_t desc[], uint32_t nr_desc, uint32_t nr_engines)
{
  uint32_t ids[nr_engines];
  for (uint32_t i = 0; i < nr_engines; i++)
    ids[i] = 0;

  for (uint32_t di = 0; di < nr_desc; di++) {
    ec_desc_t *d = &desc[di];
    uint32_t ei = d->engine_id; // self engine id

    /*
     * NOTE:
     *   Make sync_id equal to the number of descriptors
     *   to coincide with runtime code.
     */
    d->sync_ids[ei] = ++ids[ei];  // Assign self sequence number

    if (ids[ei] == 0xffff) {
      return di + 1;
    }
  }

  return nr_desc;
}

static void update_followers(ec_desc_t desc[], uint32_t nr_desc, uint32_t nr_engines)
{
  for (uint32_t i = 0; i < nr_desc; i++) {
    ec_desc_t *d = &desc[i];

    uint32_t nr_followers = nr_engines - 1;
    for (uint32_t fi = 0; fi < nr_followers; fi++) {
      ec_desc_t *f = d->followers[fi];
      if (f == NULL)
        break;

      // Follower must after current descriptor and before last descriptor.
      if (f >= desc && f < &desc[nr_desc]) {
        // Assign self id to follower's wait id
        uint32_t ei = d->engine_id;
        f->sync_ids[ei] = d->sync_ids[ei];
      }
    }
  }
}

#ifdef ENABLE_UPDATE_TDMA_WAIT_ID
//
// Case 1:
//   Zero wait_tiu_id in TDMA command:
//     TIU : [wait_tdma_id=65|tiu_id=32]
//     TDMA: [tdma_id=66|wait_tiu_id=32]
//     TDMA: [tdma_id=67|wait_tiu_id=0]     => zero wait_tiu_id
//     TDMA: [tdma_id=68|wait_tiu_id=0]     => zero wait_tiu_id
//     TDMA: [tdma_id=69|wait_tiu_id=0]     => zero wait_tiu_id
//
//   Reuse previous wait_tiu_id:
//     TIU : [wait_tdma_id=65|tiu_id=32]
//     TDMA: [tdma_id=66|wait_tiu_id=32]
//     TDMA: [tdma_id=67|wait_tiu_id=32]   => Reuse previous wait_tiu_id
//     TDMA: [tdma_id=68|wait_tiu_id=32]   => Reuse previous wait_tiu_id
//     TDMA: [tdma_id=69|wait_tiu_id=32]   => Reuse previous wait_tiu_id
//
// Case 2:
//   Zero wait_tiu_id in TDMA command:
//     TDMA: [tdma_id=3|wait_tiu_id=0]
//     TIU : [wait_tdma_id=3|tiu_id=1]
//     TDMA: [tdma_id=4|wait_tiu_id=0]
//     TIU : [wait_tdma_id=4|tiu_id=2]
//     TDMA: [tdma_id=5|wait_tiu_id=1]
//     TDMA: [tdma_id=6|wait_tiu_id=0]   => zero wait_tiu_id
//
//   Reuse previous wait_tiu_id:
//     TDMA: [tdma_id=3|wait_tiu_id=0]
//     TIU : [wait_tdma_id=3|tiu_id=1]
//     TDMA: [tdma_id=4|wait_tiu_id=0]   => Still zero, not wait previous TIU
//     TIU : [wait_tdma_id=4|tiu_id=2]
//     TDMA: [tdma_id=5|wait_tiu_id=1]
//     TDMA: [tdma_id=6|wait_tiu_id=1]   => Reuse previous wait_tiu_id
//
static void update_tdma_wait_id(ec_desc_t desc[], uint32_t nr_desc)
{
  uint32_t prev_wait_tiu_id = 0;

  for (uint32_t i = 0; i < nr_desc; i++) {
    ec_desc_t *d = &desc[i];
    uint32_t ei = d->engine_id;

    // Only handle TDMA
    if (ei != 2)
      continue;

    // Reuse TIU wait id of previous TDMA command.
    if (!d->sync_ids[0] && prev_wait_tiu_id)
      d->sync_ids[0] = prev_wait_tiu_id;

    // Record last wait tpu id in TDMA command.
    // Not tpu id of last TIU command, it forces TDMA to wait TIU.
    prev_wait_tiu_id = d->sync_ids[0];
  }
}
#endif

static void compute_sync_ids(ec_desc_t desc[], uint32_t nr_desc, uint32_t nr_engines)
{
  uint32_t nr_done = 0;
  for (uint32_t i = 0; i < nr_desc; i += nr_done) {
    // Assign command id of each engine (TPU, TDMA)
    nr_done = assign_sync_ids(&desc[i], nr_desc - i, nr_engines);

    // Update wait id (wait_tdma_id in TIU, wait_tpu_id in TDMA)
    update_followers(&desc[i], nr_done, nr_engines);

#ifdef ENABLE_UPDATE_TDMA_WAIT_ID
    // Update wait id (wait_tpu_id in TDMA)
    update_tdma_wait_id(&desc[i], nr_done);
#endif
  }
}

void ec_init(ec_t *ec, uint32_t nr_engines, uint32_t max_nr_desc)
{
  ec->nr_engines = nr_engines;

  ec->max_nr_desc = max_nr_desc;
  ec->cur_nr_desc = 0;
  ec->desc = xmalloc(max_nr_desc * sizeof(ec->desc[0]));

  uint32_t nr_followers = nr_engines - 1;
  uint32_t total_followers = max_nr_desc * nr_followers;
  uint32_t follower_buf_size = total_followers * sizeof(ec->follower_buf[0]);
  ec->follower_buf = xmalloc(follower_buf_size);

  uint32_t total_sync_ids = max_nr_desc * nr_engines;
  uint32_t sync_id_buf_size = total_sync_ids * sizeof(ec->sync_id_buf[0]);
  ec->sync_id_buf = xmalloc(sync_id_buf_size);
}

void ec_reset(ec_t *ec)
{
  ec->cur_nr_desc = 0;
}

void ec_destroy(ec_t *ec)
{
  free(ec->desc);
  free(ec->follower_buf);
  free(ec->sync_id_buf);
}

ec_desc_t * ec_alloc_desc(ec_t *ec, uint32_t engine_id)
{
  ASSERT(engine_id < ec->nr_engines);
  ASSERT(ec->cur_nr_desc < ec->max_nr_desc);

  uint32_t nr_followers = ec->nr_engines - 1;
  uint32_t i = ec->cur_nr_desc++;

  ec_desc_t *d = &ec->desc[i];
  d->followers = &ec->follower_buf[i * nr_followers];
  d->sync_ids = &ec->sync_id_buf[i * ec->nr_engines];
  ec_desc_init(d, engine_id, ec->nr_engines);

#ifdef CVK_EC_DEBUG
  d->desc_offset = i;
  d->followers_offset = i * nr_followers;
  d->sync_ids_offset = i * ec->nr_engines;

  // dump_desc(d);
#endif


  return d;
}

void ec_add_dependency(ec_t *ec, ec_desc_t *before, ec_desc_t *after)
{
  ec_desc_t *start = ec->desc;
  ec_desc_t *end = &ec->desc[ec->cur_nr_desc];

  ASSERT(before >= start && before < end);
  ASSERT(after  >= start && after  < end);

  add_follower(before, after, ec->nr_engines);
}

void ec_compute_sync_ids(ec_t *ec)
{
  compute_sync_ids(ec->desc, ec->cur_nr_desc, ec->nr_engines);
}
