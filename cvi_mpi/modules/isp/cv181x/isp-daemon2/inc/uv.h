#ifndef __UV_H__
#define __UV_H__

#include <stdio.h>
#include <sys/time.h>

#define USE_UV_BLOCKING_MODE

#if RTOS_ALIOS
#include "lwip/sockets.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define UV_EOF	(-4095)
#define cvi_uv_strerror cvi_uv_err_name

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef int cvi_uv_os_fd_t;
typedef void cvi_uv_handle_t;
typedef void cvi_uv_stream_t;
typedef void (*cvi_uv_connection_cb)(cvi_uv_stream_t *server, int status);

typedef struct {
	int sockFd;
} cvi_uv_tcp_t;

typedef struct cvi_uv_buf_t {
	char *base;
	size_t len;
} cvi_uv_buf_t;

typedef struct cvi_uv_loop_t {
	cvi_uv_connection_cb conn_cb;
	int bServerSocketInit;
	void *ptObject;

} cvi_uv_loop_t;

typedef enum {
	UV_RUN_DEFAULT = 0,
	UV_RUN_ONCE,
	UV_RUN_NOWAIT
} cvi_uv_run_mode;

typedef void (*cvi_uv_close_cb)(cvi_uv_handle_t *handle);
typedef void (*cvi_uv_alloc_cb)(cvi_uv_handle_t *handle, size_t suggested_size, cvi_uv_buf_t *buf);
typedef void (*cvi_uv_read_cb)(cvi_uv_stream_t *stream, ssize_t nread, const cvi_uv_buf_t *buf);
typedef void (*cvi_uv_walk_cb)(cvi_uv_handle_t *handle, void *arg);

//static void CVI_ISPD2_ES_CB_NewConnection(cvi_uv_stream_t *pUVServerHandle, int iStatus);
void *CVI_ISPD2_ES_CB_NewConnectionEx(void *args);

#define cvi_uv_loop_init(x) ({memset((x), 0, sizeof(*(x))); (x)->conn_cb = CVI_ISPD2_ES_CB_NewConnection; })

#define cvi_uv_loop_close(x) (0)
#define cvi_uv_read_stop(x)

//int cvi_uv_read_stop(cvi_uv_stream_t *stream);
int cvi_uv_accept(cvi_uv_stream_t *server, cvi_uv_stream_t *client);
int cvi_uv_read_start(cvi_uv_stream_t *stream, cvi_uv_alloc_cb alloc_cb, cvi_uv_read_cb read_cb);
int cvi_uv_fileno(const cvi_uv_handle_t *handle, cvi_uv_os_fd_t *fd);
int cvi_uv_tcp_init(cvi_uv_loop_t *loop, cvi_uv_tcp_t *tcp);
//int cvi_uv_loop_init(cvi_uv_loop_t *loop);
int cvi_uv_ip4_addr(const char *ip, int port, struct sockaddr_in *addr);
int cvi_uv_tcp_bind(cvi_uv_tcp_t *tcp, const struct sockaddr *addr, unsigned int flags);
int cvi_uv_listen(cvi_uv_stream_t *stream, int backlog, cvi_uv_connection_cb cb);
int cvi_uv_run(cvi_uv_loop_t *loop, cvi_uv_run_mode mode);
void cvi_uv_close(cvi_uv_handle_t *handle, cvi_uv_close_cb close_cb);
void cvi_uv_walk(cvi_uv_loop_t *loop, cvi_uv_walk_cb walk_cb, void *arg);
//int cvi_uv_loop_close(cvi_uv_loop_t * loop);
void cvi_uv_stop(cvi_uv_loop_t *loop);
const char *cvi_uv_err_name(int err);


#endif
