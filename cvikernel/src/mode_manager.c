#include "kernel_internal.h"

static void enable_serial(mode_manager_t *mm)
{
  serial_mode_init(&mm->serial_mode, &mm->engine_state, mm->ec);
  mm->mode = BMK_SERIAL_MODE;
}

static void enable_parallel(mode_manager_t *mm)
{
  parallel_mode_init(&mm->parallel_mode, &mm->engine_state, mm->ec);
  mm->mode = BMK_PARALLEL_MODE;
}

static void enable_stream(mode_manager_t *mm, uint32_t nr_streams)
{
  stream_mode_init(&mm->stream_mode, &mm->engine_state, mm->ec, nr_streams);
  mm->mode = BMK_STREAM_MODE;
}

static void destroy_current_mode(mode_manager_t *mm)
{
  switch (mm->mode) {
    case BMK_SERIAL_MODE:
      serial_mode_destroy(&mm->serial_mode);
      break;
    case BMK_PARALLEL_MODE:
      parallel_mode_destroy(&mm->parallel_mode);
      break;
    case BMK_STREAM_MODE:
      stream_mode_destroy(&mm->stream_mode);
      break;
    default:
      ASSERT(0);
  }
}

void mode_manager_init(mode_manager_t *mm, ec_t *ec, uint32_t nr_engines)
{
  engine_state_init(&mm->engine_state, nr_engines);
  mm->ec = ec;

  enable_serial(mm);
}

void mode_manager_destroy(mode_manager_t *mm)
{
  engine_state_destroy(&mm->engine_state);
  destroy_current_mode(mm);
}

void mode_manager_reset(mode_manager_t *mm)
{
  ec_reset(mm->ec);
  engine_state_reset(&mm->engine_state);

  destroy_current_mode(mm);
  enable_serial(mm);
}

void mode_manager_enable_parallel(mode_manager_t *mm)
{
  if (mm->mode == BMK_PARALLEL_MODE)
    return;

  destroy_current_mode(mm);
  enable_parallel(mm);
}

void mode_manager_disable_parallel(mode_manager_t *mm)
{
  if (mm->mode != BMK_PARALLEL_MODE) {
    ASSERT(mm->mode == BMK_SERIAL_MODE);
    return;
  }

  destroy_current_mode(mm);
  enable_serial(mm);
}

void mode_manager_create_streams(mode_manager_t *mm, uint32_t nr_streams)
{
  ASSERT(mm->mode == BMK_SERIAL_MODE);

  destroy_current_mode(mm);
  enable_stream(mm, nr_streams);
}

void mode_manager_destroy_streams(mode_manager_t *mm)
{
  ASSERT(mm->mode == BMK_STREAM_MODE);

  destroy_current_mode(mm);
  enable_serial(mm);
}

void mode_manager_set_stream(mode_manager_t *mm, uint32_t i)
{
  ASSERT(mm->mode == BMK_STREAM_MODE);

  stream_mode_set_stream(&mm->stream_mode, i);
}

void mode_manager_restart_sync_id(mode_manager_t *mm)
{
  ec_reset(mm->ec);
  engine_state_reset(&mm->engine_state);
  destroy_current_mode(mm);

  switch (mm->mode) {
    case BMK_SERIAL_MODE:
      serial_mode_init(&mm->serial_mode, &mm->engine_state, mm->ec);
      break;
    case BMK_PARALLEL_MODE:
      parallel_mode_init(&mm->parallel_mode, &mm->engine_state, mm->ec);
      break;
    case BMK_STREAM_MODE:
      stream_mode_init(&mm->stream_mode, &mm->engine_state, mm->ec,
                       mm->stream_mode.nr_streams);
      break;
    default:
      ASSERT(0);
  }
}

void mode_manager_record_ec_desc(mode_manager_t *mm, ec_desc_t *d)
{
  engine_state_update(&mm->engine_state, d);
  switch (mm->mode) {
    case BMK_SERIAL_MODE:
      serial_mode_record_desc(&mm->serial_mode, d);
      break;
    case BMK_PARALLEL_MODE:
      parallel_mode_record_desc(&mm->parallel_mode, d);
      break;
    case BMK_STREAM_MODE:
      stream_mode_record_desc(&mm->stream_mode, d);
      break;
    default:
      ASSERT(0);
  }
}
