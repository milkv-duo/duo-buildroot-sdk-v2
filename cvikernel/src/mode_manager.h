#ifndef CVIKERNEL_MODE_MANAGER_H
#define CVIKERNEL_MODE_MANAGER_H

//#include "kernel_internal.h"
#include "engine_state.h"

// Basic concept of TIU/TDMA command sequence management:
//   1. Each command buffer allocation
//      Engine conductor allocates one descriptor.
//      For previous command buffer, assign current one as follower.
//
//   2. Runtime retrieve all command buffers
//      Assign unique non-zero command id to itself.
//      Assign its command id to the follower as wait id.
//
// Difference between serial and parallel mode:
//    Serial mode keeps track of each command sequence.
//      serial_mode_record_desc() {
//        engine_state_update()
//      }
//
//    Serial mode always has previous command buffer so it always can assign
//    followers.
//
// Concurrent TDMA and TIU command execution:
//   TDMA command runs without waiting previous TIU command:
//     1. parallel_disable
//     2. parallel_enable
//     3. tiu command
//     4. tdma command (not wait TIU command)
//     5. tdma command (not wait TIU command)
//
//   Since parallel mode does not update last command buffer, the tiu command
//   is not updated and the following tdma command will not wait TIU command.
//
typedef struct {
  engine_state_t engine_state;
  ec_t *ec;
} serial_mode_t;

void serial_mode_init(serial_mode_t *m, engine_state_t *es, ec_t *ec);
void serial_mode_record_desc(serial_mode_t *m, ec_desc_t *d);
void serial_mode_destroy(serial_mode_t *m);

typedef struct {
  engine_state_t engine_state;
  ec_t *ec;
} parallel_mode_t;

void parallel_mode_init(parallel_mode_t *m, engine_state_t *es, ec_t *ec);
void parallel_mode_record_desc(parallel_mode_t *m, ec_desc_t *d);
void parallel_mode_destroy(parallel_mode_t *m);

typedef struct {
  uint32_t nr_streams;
  serial_mode_t *streams;
  serial_mode_t *cur_stream;
} stream_mode_t;

void stream_mode_init(
    stream_mode_t *m,
    engine_state_t *es,
    ec_t *ec,
    uint32_t nr_streams);
void stream_mode_record_desc(stream_mode_t *m, ec_desc_t *d);
void stream_mode_set_stream(stream_mode_t *m, uint32_t i);
void stream_mode_destroy(stream_mode_t *m);

typedef struct {
  engine_state_t engine_state;
  ec_t *ec;

#define BMK_SERIAL_MODE   0
#define BMK_PARALLEL_MODE 1
#define BMK_STREAM_MODE   2
  uint32_t mode;

  serial_mode_t serial_mode;
  parallel_mode_t parallel_mode;
  stream_mode_t stream_mode;
} mode_manager_t;

void mode_manager_init(mode_manager_t *mm, ec_t *ec, uint32_t nr_engines);
void mode_manager_destroy(mode_manager_t *mm);
void mode_manager_reset(mode_manager_t *mm);
void mode_manager_enable_parallel(mode_manager_t *mm);
void mode_manager_disable_parallel(mode_manager_t *mm);
void mode_manager_create_streams(mode_manager_t *mm, uint32_t nr_streams);
void mode_manager_destroy_streams(mode_manager_t *mm);
void mode_manager_set_stream(mode_manager_t *mm, uint32_t i);
void mode_manager_restart_sync_id(mode_manager_t *mm);
void mode_manager_record_ec_desc(mode_manager_t *mm, ec_desc_t *d);

#endif /* CVIKERNEL_MODE_MANAGER_H */
