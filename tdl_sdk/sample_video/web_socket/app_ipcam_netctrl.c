#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "app_ipcam_comm.h"
#include "app_ipcam_net.h"
#include "app_ipcam_netctrl.h"
#include "app_ipcam_websocket.h"
#include "cJSON.h"
#include "cvi_ae.h"
#include "cvi_comm.h"
#include "cvi_comm_isp.h"
#include "cvi_isp.h"
#include "cvi_vpss.h"
#include "tdl_type.h"
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define NET_VALUE_LEN_MAX (64)

#define APP_PARAM_SAME_CHK(SRC, DST, FLAG) \
  do {                                     \
    if (SRC != DST) {                      \
      FLAG = CVI_TRUE;                     \
      continue;                            \
    } else if (SRC == DST) {               \
      FLAG = CVI_FALSE;                    \
      continue;                            \
    }                                      \
  } while (0);

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
typedef enum APP_ROTATION_T {
  APP_ENUM_MIRROR,
  APP_ENUM_FLIP,
  APP_ENUM_180,
} APP_ROTATION_E;
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern struct lws *g_wsi;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int IcgiRegister(const char *cmd, const char *val, void *cb) {
  printf("enter: %s ", __func__);

  int ret = 0;
  CGI_CMD_S stCgiCmd;

  if (cmd != NULL) {
    printf("%s %zu  ", cmd, strlen(cmd) + 1);
    snprintf(stCgiCmd.cgi, strlen(cmd) + 1, cmd);
  } else {
    printf("error, cmd is NULL\n");
    ret = -1;
    goto EXIT;
  }

  if (val != NULL) {
    printf("%s %zu", val, strlen(val) + 1);
    snprintf(stCgiCmd.cmd, strlen(val) + 1, val);
  }
  printf("\n");

  if (cb == NULL) {
    ret = -1;
    goto EXIT;
  }
  stCgiCmd.callback = (CGI_CALLBACK)cb;
  CVI_NET_RegisterCgiCmd(&stCgiCmd);

EXIT:
  return ret;
}

static int Hex2Dec(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  } else {
    return -1;
  }
}

static int UrlDecode(const char url[], char *result) {
  int i = 0;
  int len = strlen(url);
  int res_len = 0;
  for (i = 0; i < len; ++i) {
    char c = url[i];
    if (c != '%') {
      result[res_len++] = c;
    } else {
      char c1 = url[++i];
      char c0 = url[++i];
      int num = 0;
      num = Hex2Dec(c1) * 16 + Hex2Dec(c0);
      result[res_len++] = num;
    }
  }
  result[res_len] = '\0';
  return res_len;
}

// /*
// *   preview page start
// */
static int app_ipcam_LocalIP_Get(const char *adapterName, char *ipAddr) {
  struct ifreq ifr;
  struct sockaddr addr;
  int skfd;
  const char *ifname = adapterName;

  skfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (skfd < 0) {
    printf("socket failed! strerror : %s\n", strerror(errno));
    return -1;
  }
  strcpy(ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;
  memset(&addr, 0, sizeof(struct sockaddr));

  if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
    addr = ifr.ifr_addr;
  } else {
    printf("ioctl failed! strerror : %s\n", strerror(errno));
    close(skfd);
    return -1;
  }
  strcpy(ipAddr, inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
  close(skfd);
  return 0;
}

static int GetWsAddrCallBack(void *param, const char *cmd, const char *val) {
  printf("enter: %s [%s|%s]\n", __func__, cmd, val);

  char ip[64] = "0.0.0.0";

  // FIXME: Only one websocket connection is supported.
  if (g_wsi == NULL) {
    if (app_ipcam_LocalIP_Get("eth0", ip) != 0) {
      if (app_ipcam_LocalIP_Get("wlan0", ip) != 0)
        printf("Error: [%s][%d] getlocalIP [eth0/wlan0] failed!\n", __func__, __LINE__);
    }
    printf("getlocalIP [eth0/wlan0] %s\n", ip);
  }

  CVI_NET_AddCgiResponse(param, "ws://%s:8000", ip);

  return 0;
}

static int GetAudioInfoCallBack(void *param, const char *cmd, const char *val) {
  cJSON *cjsonAudioAttr = NULL;
  char *str = NULL;
  SAMPLE_TDL_TYPE tdl_type = ai_param_get();
  cjsonAudioAttr = cJSON_CreateObject();

  printf("enter: %s\n", __func__);

  cJSON_AddNumberToObject(cjsonAudioAttr, "main_enabled", tdl_type);
  str = cJSON_Print(cjsonAudioAttr);
  if (str) {
    CVI_NET_AddCgiResponse(param, "%s", str);
    cJSON_free(str);
  }
  cJSON_Delete(cjsonAudioAttr);

  return 0;
}

static int SetAudioInfoCallBack(void *param, const char *cmd, const char *val) {
  printf("enter: %s [%s|%s]\n", __func__, cmd, val);
  int ret = 0;
  char decode[1024] = {0};
  cJSON *cjsonParser = NULL;
  cJSON *cjsonObj = NULL;

  UrlDecode(val, decode);
  printf("%s\n", decode);

  cjsonParser = cJSON_Parse(decode);
  if (cjsonParser == NULL) {
    printf("parse fail.\n");
    return -1;
  }
  cjsonObj = cJSON_GetObjectItem(cjsonParser, "main_enabled");
  _NULL_POINTER_CHECK_(cjsonObj->valuestring, -1);
  SAMPLE_TDL_TYPE tdl_type = (SAMPLE_TDL_TYPE)atoi(cjsonObj->valuestring);
  ai_param_set(tdl_type);
  CVI_NET_AddCgiResponse(param, "ret: %d\n", ret);
  return 0;
}

// /*
// *  preview page get/set CB list
// */
static int app_ipcam_IcgiRegister_Preview(void) {
  printf("enter: %s\n", __func__);
  IcgiRegister("get_ws_addr.cgi", NULL, (void *)GetWsAddrCallBack);
  return 0;
}

static int app_ipcam_IcgiRegister_Audio(void) {
  printf("enter: %s\n", __func__);
  IcgiRegister("get_audio_cfg.cgi", NULL, (void *)GetAudioInfoCallBack);
  IcgiRegister("set_audio_cfg.cgi", NULL, (void *)SetAudioInfoCallBack);
  return 0;
}

int app_ipcam_NetCtrl_Init() {
  printf("app_ipcam_NetCtrl_Init\n");
  char path[] = "/mnt/nfs/www";
  CVI_NET_SetVideoPath(path);

  app_ipcam_IcgiRegister_Preview();
  app_ipcam_IcgiRegister_Audio();
  CVI_NET_Init();

  return 0;
}

int app_ipcam_NetCtrl_DeInit() {
  CVI_NET_Deinit();

  return 0;
}
