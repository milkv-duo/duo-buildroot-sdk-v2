#include "cvi_ispd2_local.h"
#include "cvi_ispd2_log.h"
#include "cvi_ispd2_event_server.h"
#if RTOS_ALIOS
#include "lwip/sockets.h"
#else
#include <sys/socket.h>
#endif
#include <errno.h>
#include <unistd.h>

#include <sys/prctl.h>

typedef struct {
	cvi_uv_alloc_cb alloc_cb;
	cvi_uv_read_cb read_cb;
	cvi_uv_buf_t pBuf;
	cvi_uv_stream_t *handle;
} uvRdCb;

#define container_of(ptr, type, member) ({\
		const typeof(((type *)0)->member) * __mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); })


void cvi_uv_close(cvi_uv_handle_t *handle, cvi_uv_close_cb close_cb)
{
	cvi_uv_tcp_t *tcp = (cvi_uv_tcp_t *)handle;

	cvi_uv_stream_t_ex	*pUVClientEx = (cvi_uv_stream_t_ex *)handle;

	if (tcp->sockFd != 0) {
		close(tcp->sockFd);
	}

	SAFE_FREE(pUVClientEx);

	UNUSED(close_cb);
}

int cvi_uv_accept(cvi_uv_stream_t *server, cvi_uv_stream_t *client)
{
	cvi_uv_stream_t_ex *pUVServerEx = (cvi_uv_stream_t_ex *)server;

	struct sockaddr_in  connAddr;
	socklen_t addr_len;

	UNUSED(connAddr);

	addr_len = sizeof(struct sockaddr_in);

	cvi_uv_stream_t_ex *pUVClientEx = (cvi_uv_stream_t_ex *)client;

	printf("waiting for connect...\n");

	pUVClientEx->UVTcp.sockFd = accept(pUVServerEx->UVTcp.sockFd, (struct sockaddr *)&connAddr, &addr_len);

	if (pUVClientEx->UVTcp.sockFd == -1) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Accept fail");
		return -1;
	}

	//printf("sockFd: %d\n", pUVClientEx->UVTcp.sockFd);

	return 0;

}

static void *cvi_uv_read(void *arg)
{

	uvRdCb *cb = (uvRdCb *)arg;

	cvi_uv_stream_t_ex *pUVClientEx = (cvi_uv_stream_t_ex *)(cb->handle);
	cvi_uv_stream_t		*pUVClient = (cvi_uv_stream_t *)&(pUVClientEx->UVTcp);

	TISPDaemon2ConnectInfo	*ptConnectObj = (TISPDaemon2ConnectInfo *)pUVClientEx->ptHandle;
	TISPDaemon2Info			*ptObject = ptConnectObj->ptDaemonInfo;

	UNUSED(pUVClient);

	int socket_id = pUVClientEx->UVTcp.sockFd;
	int ret;

	struct timeval tv;
	fd_set readFd;

	prctl(PR_SET_NAME, "cvi_uv_read");

	while (ptObject->bServiceThreadRunning) {
		FD_ZERO(&readFd);
		FD_SET(socket_id, &readFd);
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		ret = select(socket_id + 1, &readFd, NULL, NULL, &tv);

		if (ret > 0) {
			if (FD_ISSET(socket_id, &readFd)) {
				if (cb->alloc_cb) {
					cb->alloc_cb(NULL, 4096, &(cb->pBuf));
				}
				memset(cb->pBuf.base, 0, cb->pBuf.len);

				ret  = recv(socket_id, cb->pBuf.base, cb->pBuf.len, 0);

				if (ret > 0) {
					cb->pBuf.len = ret;
					if (cb->read_cb) {
						cb->read_cb(cb->handle, cb->pBuf.len, &(cb->pBuf));
					}
				} else if (ret == 0) {		// client close
					break;
				} else if (errno == EINTR) {
					ISP_DAEMON2_DEBUG(LOG_DEBUG, "EINTR be caught");
					continue;
				} else {
					ISP_DAEMON2_DEBUG(LOG_DEBUG, "recv fail");
					break;
				}
			}
		} else if (ret < 0) {
			break;
		}
	}

	ISP_DAEMON2_DEBUG(LOG_DEBUG, "Client close");
	ptConnectObj->ptDaemonInfo->u8ClientCount--;
	SAFE_FREE(ptConnectObj->pszRecvBuffer);
	SAFE_FREE(ptConnectObj);


	cvi_uv_close((cvi_uv_handle_t *)pUVClientEx, NULL);

	SAFE_FREE(cb->pBuf.base);
	SAFE_FREE(cb);

	pthread_exit(NULL);
	return 0;
}

int cvi_uv_read_start(cvi_uv_stream_t *stream, cvi_uv_alloc_cb alloc_cb, cvi_uv_read_cb read_cb)
{

	pthread_t id;

	UNUSED(id);
	int ret = 0;

	uvRdCb *cb = (uvRdCb *)malloc(sizeof(uvRdCb));

	memset(cb, 0, sizeof(*cb));

	cb->alloc_cb = alloc_cb;
	cb->read_cb	 = read_cb;
	cb->handle	= stream;

	ret = pthread_create(&id, NULL, cvi_uv_read, (void *)cb);

	if (ret != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "create error\n");
		return -1;
	}

	pthread_detach(id);

	return 0;
}

int cvi_uv_tcp_init(cvi_uv_loop_t *loop, cvi_uv_tcp_t *tcp)
{
	tcp->sockFd = 0;
	if (!loop->bServerSocketInit) {
		tcp->sockFd = socket(AF_INET, SOCK_STREAM, 0);
		loop->bServerSocketInit = 1;
	}

	return 0;
}

int cvi_uv_fileno(const cvi_uv_handle_t *handle, cvi_uv_os_fd_t *fd)
{
	cvi_uv_tcp_t *tcp = (cvi_uv_tcp_t *)handle;
	*fd = tcp->sockFd;

	return 0;
}

int cvi_uv_ip4_addr(const char *ip, int port, struct sockaddr_in *addr)
{
	UNUSED(ip);
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = htonl(INADDR_ANY);

	return 0;
}

int cvi_uv_tcp_bind(cvi_uv_tcp_t *tcp, const struct sockaddr *addr, unsigned int flags)
{
	unsigned int addrlen = sizeof(struct sockaddr_in);
	int on = 1;
	int ret;

	UNUSED(flags);

	if (setsockopt(tcp->sockFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))) {
		return -1;
	}

	ret = bind(tcp->sockFd, (struct sockaddr *)addr, addrlen);

	if (ret == -1) {
		printf("bind error\n");
		return -1;
	}

	return 0;
}


int cvi_uv_listen(cvi_uv_stream_t *stream, int backlog, cvi_uv_connection_cb cb)
{
	int ret;
	cvi_uv_stream_t_ex *pUVServerEx = (cvi_uv_stream_t_ex *)stream;
	TISPDaemon2Info *ptObject = container_of(pUVServerEx, TISPDaemon2Info, UVServerEx);

	ptObject->pUVLoop->ptObject = (void *)ptObject;

	UNUSED(cb);

	ret = listen(pUVServerEx->UVTcp.sockFd, backlog);

	return ret;
}

const char *cvi_uv_err_name(int err)
{
	UNUSED(err);
	return "null";
}


void *CVI_ISPD2_ES_CB_NewConnectionEx(void *args)
{
	TISPDaemon2Info	*ptObject = (TISPDaemon2Info *)args;

	ptObject->bServiceThreadRunning = CVI_TRUE;

	ptObject->pUVLoop->conn_cb((cvi_uv_stream_t *)&(ptObject->UVServerEx), 1);

	return 0;
}

int cvi_uv_run(cvi_uv_loop_t *loop, cvi_uv_run_mode mode)
{
	UNUSED(mode);
	TISPDaemon2Info *ptObject = (TISPDaemon2Info *)(loop->ptObject);

	CVI_ISPD2_ES_CB_NewConnectionEx((void *)ptObject);

	return 0;
}

void cvi_uv_walk(cvi_uv_loop_t *loop, cvi_uv_walk_cb walk_cb, void *arg)
{
	UNUSED(loop);
	UNUSED(walk_cb);
	UNUSED(arg);
}

void cvi_uv_stop(cvi_uv_loop_t *loop)
{
	UNUSED(loop);

	shutdown(((TISPDaemon2Info *)(loop->ptObject))->UVServerEx.UVTcp.sockFd, SHUT_RDWR);
}

