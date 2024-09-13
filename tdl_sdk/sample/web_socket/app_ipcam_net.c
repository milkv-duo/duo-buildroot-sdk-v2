#include "app_ipcam_net.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "libhttpd.h"
#include "thttpd.h"

extern int terminate;
static pthread_t g_webserver_thread;

pthread_mutex_t g_cgi_mutex = PTHREAD_MUTEX_INITIALIZER;
static CGI_CMD_S g_cgi_list[CGI_REG_MAX];

int CVI_NET_AddCgiResponse(void *param, char *pszfmt, ...) {
  httpd_conn *hc = (httpd_conn *)param;

  if (NULL == hc || NULL == pszfmt) {
    return -1;
  }
  int ret;
  char response[2048];
  va_list stVal;
  va_start(stVal, pszfmt);
  ret = vsnprintf(response, sizeof(response), pszfmt, stVal);
  va_end(stVal);
  add_response(hc, response);
  return ret;
}

static int cvi_cgi_cmd_proc(httpd_conn *hc, char *cgi, char *cmd) {
  printf("enter: %s\n", __func__);
  int i = 0;
  int ret = -1;
  int flag_cmd = 0;
  CGI_CALLBACK pFunCgiCmdProc = NULL;  // find the match command in the global array cgi_cmd[]

  for (i = 0; (i < CGI_REG_MAX) && (NULL != g_cgi_list[i].callback); i++) {
    if (strstr(g_cgi_list[i].cgi, cgi) != NULL) {
      flag_cmd = 1;
      pFunCgiCmdProc = g_cgi_list[i].callback;
      break;
    }
  }

  if (i >= CGI_REG_MAX || flag_cmd != 1) {
    printf("can not find the cgi command[%s]\n", cmd);
    CVI_NET_AddCgiResponse(hc, "can not find cgi: %s?%s\n", cgi, cmd);
    ret = -1;
    goto exit;
  }

  if (pFunCgiCmdProc != NULL) {
    ret = pFunCgiCmdProc(hc, cgi, cmd);
    if (ret != 0) {
      printf("pFunCgiCmdProc fail \n");
      ret = -1;
      goto exit;
    }
  } else {
    CVI_NET_AddCgiResponse(hc, "this cgi proc is NULL: %s\n", cgi);
  }

  return 0;
exit:
  return ret;
}

static int cvi_cgi_handle(httpd_conn *hc) {
  printf("enter: %s\n", __func__);  // int s32Ret = 0;
  char *pCommand = NULL;
  char szCommand[MAX_CGI_INTER] = {0};

  if (hc == NULL) {
    printf("pstRequest & pstReponse can not be null\n\n\n");
    return -1;
  }

  if (hc->origfilename == NULL) {
    printf("origfilename can not be null\n\n\n");
    return -1;
  }

  pCommand = strrchr(hc->origfilename, '/');

  if ((pCommand != NULL) && (strlen(pCommand) < MAX_CGI_INTER)) {
    strncpy(szCommand, pCommand + 1, MAX_CGI_INTER - 1);
    szCommand[MAX_CGI_INTER - 1] = '\0';
  } else {
    printf("the command is invalide:\n%s\n", hc->origfilename);
    return -1;
  }
  printf("pCommand is: %s\n", pCommand);
  printf("szCommand is %s\n", szCommand);
  cvi_cgi_cmd_proc(hc, szCommand, hc->query);

  return 0;
}

int CVI_NET_Init() {
  printf("enter: %s\n", __func__);
  int s32Ret = 0;
  terminate = 0;
  s32Ret = pthread_create(&g_webserver_thread, NULL, thttpd_start_main, NULL);
  CVI_THTTPD_RegisterCgiHandle(cvi_cgi_handle);
  return s32Ret;
}

int CVI_NET_Deinit() {
  printf("enter: %s\n", __func__);
  terminate = 1;
  if (g_webserver_thread != 0) {
    pthread_join(g_webserver_thread, NULL);
    g_webserver_thread = 0;
  }
  printf("exit: %s\n", __func__);
  return 0;
}

int CVI_NET_RegisterCgiCmd(CGI_CMD_S *cgi_cmd) {
  printf("enter: %s\n", __func__);
  int i = 0;
  pthread_mutex_lock(&g_cgi_mutex);
  for (i = 0; i < CGI_REG_MAX; i++) {
    if (NULL == g_cgi_list[i].callback) {
      memcpy(&g_cgi_list[i], cgi_cmd, sizeof(CGI_CMD_S));
      break;
    }
  }

  if (i >= CGI_REG_MAX) {
    pthread_mutex_unlock(&g_cgi_mutex);
    printf("command list is full\n");
    return -1;
  }
  pthread_mutex_unlock(&g_cgi_mutex);

  return 0;
}

int CVI_NET_SetVideoPath(const char *path) {
  set_video_path(path); /* from thttpd.h */

  return 0;
}
