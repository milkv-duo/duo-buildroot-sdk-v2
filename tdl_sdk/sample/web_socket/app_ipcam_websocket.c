#include "app_ipcam_websocket.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "app_ipcam_comm.h"
#include "libwebsockets.h"
#define MAX_PAYLOAD_SIZE (1024 * 1024)
#define MAX_BUFFER_SIZE (512 * 1024)

struct lws *g_wsi = NULL;
static unsigned char *s_imgData = NULL;
static unsigned char *s_tdl_Data = NULL;
static int s_fileSize = 0;
static int s_tdlSize = 0;
static volatile int s_terminal = 0;
pthread_mutex_t g_mutexLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_aiMutexLock = PTHREAD_MUTEX_INITIALIZER;
pthread_t g_websocketThread;

struct sessionData_s {
  int msg_count;
  unsigned char buf[MAX_PAYLOAD_SIZE];
  int len;
  int bin;
  int fin;
};

static int ProtocolMyCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
                              void *in, size_t len) {
  struct sessionData_s *data = (struct sessionData_s *)user;
  switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
      printf("Client LWS_CALLBACK_ESTABLISHED!\n");
      g_wsi = wsi;

      break;
    case LWS_CALLBACK_RECEIVE:
      lws_rx_flow_control(wsi, 0);
      char *tmp_data = (char *)malloc(len + 1);
      memset(tmp_data, 0, len + 1);
      memcpy(tmp_data, in, len);
      data->len = len;
      free(tmp_data);
      break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
      pthread_mutex_lock(&g_mutexLock);
      if (s_fileSize != 0) {
        lws_write(wsi, s_imgData + LWS_PRE, s_fileSize,
                  LWS_WRITE_BINARY);  // LWS_WRITE_BINARY LWS_WRITE_TEXT
        s_fileSize = 0;
      }
      pthread_mutex_unlock(&g_mutexLock);

      pthread_mutex_lock(&g_aiMutexLock);

      if (s_tdlSize != 0 && NULL != s_tdl_Data) {
        lws_write(wsi, s_tdl_Data + LWS_PRE, s_tdlSize,
                  LWS_WRITE_BINARY);  // LWS_WRITE_BINARY LWS_WRITE_TEXT
      } else {
        // printf("----- websocket filesize is 0\n");
      }
      if (s_tdl_Data != NULL) {
        free(s_tdl_Data);
        s_tdl_Data = NULL;
      }
      pthread_mutex_unlock(&g_aiMutexLock);

      lws_rx_flow_control(wsi, 1);
      break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      printf("client LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n");
      break;
    case LWS_CALLBACK_WSI_DESTROY:
    case LWS_CALLBACK_CLOSED:
      printf("client close WebSocket\n");
      g_wsi = NULL;
      // CVI_IPC_CleanHumanTraffic();

      pthread_mutex_lock(&g_mutexLock);
      s_fileSize = 0;
      pthread_mutex_unlock(&g_mutexLock);

      pthread_mutex_lock(&g_aiMutexLock);
      if (s_tdl_Data) {
        free(s_tdl_Data);
        s_tdl_Data = NULL;
      }
      s_tdlSize = 0;
      pthread_mutex_unlock(&g_aiMutexLock);
      break;
    default:
      break;
  }
  return 0;
}

struct lws_protocols protocols[] = {
    {"ws",                         /*协议名*/
     ProtocolMyCallback,           /*回调函数*/
     sizeof(struct sessionData_s), /*定义新连接分配的内存,连接断开后释放*/
     MAX_PAYLOAD_SIZE,             /*定义rx 缓冲区大小*/
     0, NULL, 0},
    {NULL, NULL, 0, 0, 0, 0, 0} /*结束标志*/
};

static void *ThreadWebsocket(void *arg) {
  static struct lws_context_creation_info ctx_info = {0};
  ctx_info.port = 8000;
  ctx_info.iface = NULL;  // 在所有网络接口上监听
  ctx_info.protocols = protocols;
  ctx_info.gid = -1;
  ctx_info.uid = -1;
  ctx_info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

  struct lws_context *context = lws_create_context(&ctx_info);
  while (!s_terminal) {
    lws_service(context, 1000);
  }
  lws_context_destroy(context);

  return NULL;
}

int app_ipcam_WebSocket_Init() {
  int s32Ret = 0;

  s_imgData = (unsigned char *)malloc(MAX_BUFFER_SIZE);
  if (!s_imgData) {
    printf("malloc failed\n");
    return -1;
  }

  s32Ret = pthread_create(&g_websocketThread, NULL, ThreadWebsocket, NULL);
  return s32Ret;
}

int app_ipcam_WebSocket_DeInit() {
  s_terminal = 1;
  if (g_websocketThread) {
    pthread_join(g_websocketThread, NULL);
    g_websocketThread = 0;
  }

  if (s_imgData) {
    free(s_imgData);
    s_imgData = NULL;
  }

  return 0;
}

int app_ipcam_WebSocket_Stream_Send(void *pData, void *pArgs) {
  // printf("---%s|%d--- %d\n", __func__, __LINE__, len);
  if (pData == NULL) {
    printf("Web stream send pData is NULL\n");
    return -1;
  }

  if (g_wsi == NULL) {
    return 0;
  }

  VENC_STREAM_S *pstStream = (VENC_STREAM_S *)pData;
  VENC_PACK_S *ppack;
  unsigned char *pAddr = NULL;
  CVI_U32 packSize = 0;
  CVI_U32 total_packSize = 0;

  pthread_mutex_lock(&g_mutexLock);

  for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
    ppack = &pstStream->pstPack[i];
    packSize = ppack->u32Len - ppack->u32Offset;
    total_packSize += packSize;
  }

  CVI_U32 need_size = LWS_PRE + 1 + total_packSize + 1;  // one for type, one for reserve
  if (need_size > MAX_BUFFER_SIZE) {
    printf("buffer size is not ok\n");
    pthread_mutex_unlock(&g_mutexLock);
    return -1;
  }

  memset(s_imgData, 0, total_packSize + LWS_PRE + 1 + 1);
  s_imgData[LWS_PRE] = 0 & 0xff;  // type0: h264 buf

  CVI_U32 len = 0;
  for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
    ppack = &pstStream->pstPack[i];
    pAddr = ppack->pu8Addr + ppack->u32Offset;
    packSize = ppack->u32Len - ppack->u32Offset;
    memcpy(s_imgData + LWS_PRE + 1 + len, pAddr, packSize);
    len += packSize;
  }

  s_fileSize = len + 1;

  if (g_wsi != NULL) {
    lws_callback_on_writable(g_wsi);
  } else {
    printf("WebSocketSend g_wsi is NULL\n");
  }

  pthread_mutex_unlock(&g_mutexLock);

  return 0;
}
