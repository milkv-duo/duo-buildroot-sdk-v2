#ifndef __APP_DBG_H__
#define __APP_DBG_H__

#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

/* Make this header file easier to include in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

#define APP_IPCAM_MAX_FILE_LEN 128
#define APP_IPCAM_MAX_STR_LEN 64

#define APP_IPCAM_SUCCESS ((int)(0))
#define APP_IPCAM_ERR_FAILURE ((int)(-1001))
#define APP_IPCAM_ERR_NOMEM ((int)(-1002))
#define APP_IPCAM_ERR_TIMEOUT ((int)(-1003))
#define APP_IPCAM_ERR_INVALID ((int)(-1004))

#define APP_CHK_RET(express, name)                                                        \
  do {                                                                                    \
    int rc = express;                                                                     \
    if (rc != CVI_SUCCESS) {                                                              \
      printf("\033[40;31m%s failed at %s line:%d with %#x!\033[0m\n", name, __FUNCTION__, \
             __LINE__, rc);                                                               \
      return rc;                                                                          \
    }                                                                                     \
  } while (0)

#define APP_IPCAM_LOGE(...) printf(__VA_ARGS__)
#define APP_IPCAM_CHECK_RET(ret, fmt...)              \
  do {                                                \
    if (ret != CVI_SUCCESS) {                         \
      APP_IPCAM_LOGE(fmt);                            \
      APP_IPCAM_LOGE("fail and return:[%#x]\n", ret); \
      return ret;                                     \
    }                                                 \
  } while (0)

#ifndef _NULL_POINTER_CHECK_
#define _NULL_POINTER_CHECK_(p, errcode)   \
  do {                                     \
    if (!(p)) {                            \
      printf("pointer[%s] is NULL\n", #p); \
      return errcode;                      \
    }                                      \
  } while (0)
#endif

/* 	END debug API
 *	easy debug for app-development
 */

typedef void *(*pfp_task_entry)(void *param);

typedef struct _RUN_THREAD_PARAM {
  int bRun_flag;
  pthread_t mRun_PID;
} RUN_THREAD_PARAM;

unsigned int GetCurTimeInMsec(void);

#ifdef __cplusplus
}
#endif

#endif
