#ifndef _APP_IPCAM_NET_H_
#define _APP_IPCAM_NET_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define CGI_CMD_MAX_LEN (32)
#define CGI_VAL_MAX_LEN (32)
#define CGI_REG_MAX (64)
#define MAX_CGI_INTER (64)

typedef int (*CGI_CALLBACK)(void *param, const char *cmd, const char *val);

typedef struct _CGI_CMD_S {
  char cgi[CGI_CMD_MAX_LEN];
  char cmd[CGI_VAL_MAX_LEN];
  CGI_CALLBACK callback;
} CGI_CMD_S;

int CVI_NET_Init();
int CVI_NET_Deinit();
int CVI_NET_RegisterCgiCmd(CGI_CMD_S *cgi_cmd);
int CVI_NET_AddCgiResponse(void *param, char *pszfmt, ...);
int CVI_NET_SetVideoPath(const char *path);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _CVI_NET_H_ */
