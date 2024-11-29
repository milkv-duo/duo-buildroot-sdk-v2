#ifndef _AF_BUF_H_
#define _AF_BUF_H_

#include "cvi_comm_inc.h"
#include "afalgo.h"
#include "malloc.h"
#include "pthread.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

extern ISP_FOCUS_ATTR_S *pstFocusMpiAttr[AF_SENSOR_NUM];
extern ISP_FOCUS_ATTR_S *stFocusAttrInfo[AF_SENSOR_NUM];
extern ISP_FOCUS_Q_INFO_S *pstFocusQInfo[AF_SENSOR_NUM];
extern CVI_U8 isUpdateAttr[AF_SENSOR_NUM][AF_UPDATE_TOTAL];
extern AF_CTX_S *g_astAfCtx[AF_SENSOR_NUM];
extern ISP_FOCUS_STATISTICS_CFG_S *pstAfStatisticsCfg[AF_SENSOR_NUM];
extern ISP_FOCUS_STATISTICS_CFG_S *pstAfStatisticsCfgInfo[AF_SENSOR_NUM];
extern ISP_AF_MOTOR_FUNC_S g_stAfMotorCb[AF_SENSOR_NUM];
extern VI_PIPE g_vipipe;

extern pthread_mutex_t g_afLock[AF_SENSOR_NUM];
extern pthread_t g_afTid[AF_SENSOR_NUM];
extern pthread_attr_t g_attr;
extern struct sched_param g_param;

void *AF_Malloc(size_t nsize);
void AF_Free(void *ptr);
void AF_CheckMemFree(void);

void AF_SetParamUpdateFlag(CVI_U8 sID, AF_PARAMETER_UPDATE flag);
void AF_CheckParamUpdateFlag(CVI_U8 sID);

CVI_S32 af_buf_init(CVI_U8 sID);
CVI_S32 af_buf_deinit(CVI_U8 sID);

#ifdef ARCH_RTOS_CV181X
void AF_RtosBufInit(CVI_U8 sID);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AF_BUF_H_
