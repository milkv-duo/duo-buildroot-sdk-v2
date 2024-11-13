
#ifndef _AE_BUF_H_
#define _AE_BUF_H_

#include "cvi_comm_inc.h"
#include "aealgo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/****************** from aealgo.c *******************/
extern CVI_U8	u8SensorNum;
extern SAE_INFO AeInfo[AE_SENSOR_NUM];
extern SFACE_DETECT_INFO AeFaceDetect[AE_SENSOR_NUM];
void AE_SaveParameterSetting(void);
/*************************************/

extern SAE_BOOT_INFO *AeBootInfo[AE_SENSOR_NUM];

extern ISP_EXP_INFO_S *pstAeMpiExpInfo[AE_SENSOR_NUM];
extern ISP_EXPOSURE_ATTR_S *pstAeMpiExposureAttr[AE_SENSOR_NUM];
extern ISP_EXPOSURE_ATTR_S *pstAeExposureAttrInfo[AE_SENSOR_NUM];

extern ISP_WDR_EXPOSURE_ATTR_S *pstAeMpiWDRExposureAttr[AE_SENSOR_NUM];
extern ISP_WDR_EXPOSURE_ATTR_S *pstAeWDRExposureAttrInfo[AE_SENSOR_NUM];

extern ISP_AE_ROUTE_S *pstAeMpiRoute[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
extern ISP_AE_ROUTE_S *pstAeRouteInfo[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

extern ISP_AE_ROUTE_EX_S *pstAeMpiRouteEx[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];
extern ISP_AE_ROUTE_EX_S *pstAeRouteExInfo[AE_SENSOR_NUM][AE_MAX_WDR_FRAME_NUM];

extern ISP_SMART_EXPOSURE_ATTR_S *pstAeMpiSmartExposureAttr[AE_SENSOR_NUM];
extern ISP_SMART_EXPOSURE_ATTR_S *pstAeSmartExposureAttrInfo[AE_SENSOR_NUM];

extern ISP_AE_STATISTICS_CFG_S	*pstAeStatisticsCfg[AE_SENSOR_NUM];
extern ISP_AE_STATISTICS_CFG_S *pstAeStatisticsCfgInfo[AE_SENSOR_NUM];

extern ISP_IRIS_ATTR_S	*pstAeIrisAttr[AE_SENSOR_NUM];
extern ISP_IRIS_ATTR_S *pstAeIrisAttrInfo[AE_SENSOR_NUM];

extern ISP_DCIRIS_ATTR_S *pstAeDcIrisAttr[AE_SENSOR_NUM];
extern ISP_DCIRIS_ATTR_S *pstAeDcIrisAttrInfo[AE_SENSOR_NUM];

extern CVI_U8 *AE_LogBuf;
extern CVI_BOOL AE_Dump2File;
extern s_AE_DBG_S *pstAeDbg;
extern CVI_U8 *AE_SnapLogBuf;
extern CVI_U32 AE_SnapLogTailIndex;
extern CVI_U8 u8MallocCnt[AE_SENSOR_NUM];

void *AE_Malloc(CVI_U8 sID, size_t size);
void AE_Free(CVI_U8 sID, void *ptr);

void AE_MemoryAlloc(CVI_U8 sID, AE_MEMORY_ITEM item);
void AE_MemoryFree(CVI_U8 sID, AE_MEMORY_ITEM item);

void AE_SetParamUpdateFlag(CVI_U8 sID, AE_PARAMETER_UPDATE flag);
void AE_CheckParamUpdateFlag(CVI_U8 sID);

void AE_SnapLogInit(void);
void AE_SnapLogPrintf(const char *szFmt, ...);

#ifdef ARCH_RTOS_CV181X
void AE_RtosBufInit(CVI_U8 sID);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AE_BUF_H_

