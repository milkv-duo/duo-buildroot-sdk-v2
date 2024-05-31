#include "kernel_internal.h"

void stream_mode_init(
    stream_mode_t *m, engine_state_t *es, ec_t *ec, uint32_t nr_streams)
{
  m->nr_streams = nr_streams;
  m->streams = xmalloc(nr_streams * sizeof(*m->streams));
  for (uint32_t i = 0; i < nr_streams; i++)
    serial_mode_init(&m->streams[i], es, ec);
  m->cur_stream = &m->streams[0];
}

void stream_mode_record_desc(stream_mode_t *m, ec_desc_t *d)
{
  serial_mode_record_desc(m->cur_stream, d);
}

void stream_mode_set_stream(stream_mode_t *m, uint32_t i)
{
  ASSERT(i < m->nr_streams);
  m->cur_stream = &m->streams[i];
}

void stream_mode_destroy(stream_mode_t *m)
{
  for (uint32_t i = 0; i < m->nr_streams; i++)
    serial_mode_destroy(&m->streams[i]);

  free(m->streams);
  m->nr_streams = 0;
  m->streams = NULL;
  m->cur_stream = NULL;
}
