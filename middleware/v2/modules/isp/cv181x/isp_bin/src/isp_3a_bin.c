/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_3a_bin.c
 * Description:
 *
 */

#include "isp_3a_bin.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"
#include "3A_internal.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

CVI_S32 isp_3aBinAttr_get_size(VI_PIPE ViPipe, CVI_U32 *size)
{
	CVI_U32 tmpSize = 0;

	tmpSize += sizeof(ISP_WDR_EXPOSURE_ATTR_S);
	tmpSize += sizeof(ISP_EXPOSURE_ATTR_S);
	tmpSize += sizeof(ISP_AE_ROUTE_S);
	tmpSize += sizeof(ISP_AE_ROUTE_EX_S);
	tmpSize += sizeof(ISP_SMART_EXPOSURE_ATTR_S);
	tmpSize += sizeof(ISP_IRIS_ATTR_S);
	tmpSize += sizeof(ISP_DCIRIS_ATTR_S);
	tmpSize += sizeof(ISP_AE_ROUTE_S);
	tmpSize += sizeof(ISP_AE_ROUTE_EX_S);
	tmpSize += sizeof(ISP_WB_ATTR_S);
	tmpSize += sizeof(ISP_AWB_ATTR_EX_S);
	tmpSize += sizeof(ISP_AWB_Calibration_Gain_S);
	tmpSize += sizeof(ISP_AWB_Calibration_Gain_S_EX);
	tmpSize += sizeof(ISP_STATISTICS_CFG_S);

	*size = tmpSize;

	UNUSED(ViPipe);

	return CVI_SUCCESS;
}

CVI_S32 isp_3aBinAttr_get_param(VI_PIPE ViPipe, FILE *fp)
{
	// AE
	ISP_WDR_EXPOSURE_ATTR_S pstWDRExpAttr = { 0 };

	CVI_ISP_GetWDRExposureAttr(ViPipe, &pstWDRExpAttr);
	fwrite(&pstWDRExpAttr, sizeof(ISP_WDR_EXPOSURE_ATTR_S), 1, fp);

	ISP_EXPOSURE_ATTR_S pstExpAttr = { 0 };

	CVI_ISP_GetExposureAttr(ViPipe, &pstExpAttr);
	pstExpAttr.u8DebugMode = 0;
	fwrite(&pstExpAttr, sizeof(ISP_EXPOSURE_ATTR_S), 1, fp);

	ISP_AE_ROUTE_S pstAeRouteAttr = { 0 };

	CVI_ISP_GetAERouteAttr(ViPipe, &pstAeRouteAttr);
	fwrite(&pstAeRouteAttr, sizeof(ISP_AE_ROUTE_S), 1, fp);

	ISP_AE_ROUTE_EX_S pstAeRouteAttrEx = { 0 };

	CVI_ISP_GetAERouteAttrEx(ViPipe, &pstAeRouteAttrEx);
	fwrite(&pstAeRouteAttrEx, sizeof(ISP_AE_ROUTE_EX_S), 1, fp);

	ISP_SMART_EXPOSURE_ATTR_S stAeSmartExposureAttr = { 0 };

	CVI_ISP_GetSmartExposureAttr(ViPipe, &stAeSmartExposureAttr);
	fwrite(&stAeSmartExposureAttr, sizeof(ISP_SMART_EXPOSURE_ATTR_S), 1, fp);

	ISP_IRIS_ATTR_S stAeIrisAttr = { 0 };

	CVI_ISP_GetIrisAttr(ViPipe, &stAeIrisAttr);
	fwrite(&stAeIrisAttr, sizeof(ISP_IRIS_ATTR_S), 1, fp);

	ISP_DCIRIS_ATTR_S stAeDcIrisAttr = { 0 };

	CVI_ISP_GetDcirisAttr(ViPipe, &stAeDcIrisAttr);
	fwrite(&stAeDcIrisAttr, sizeof(ISP_DCIRIS_ATTR_S), 1, fp);

	ISP_AE_ROUTE_S pstAeRouteSFAttr = { 0 };

	CVI_ISP_GetAERouteSFAttr(ViPipe, &pstAeRouteSFAttr);
	fwrite(&pstAeRouteSFAttr, sizeof(ISP_AE_ROUTE_S), 1, fp);

	ISP_AE_ROUTE_EX_S pstAeRouteSFAttrEx = { 0 };

	CVI_ISP_GetAERouteSFAttrEx(ViPipe, &pstAeRouteSFAttrEx);
	fwrite(&pstAeRouteSFAttrEx, sizeof(ISP_AE_ROUTE_EX_S), 1, fp);

	// AWB
	ISP_WB_ATTR_S stWBAttr = {0};

	CVI_ISP_GetWBAttr(ViPipe, &stWBAttr);
	stWBAttr.u8DebugMode = 0;
	fwrite(&stWBAttr, sizeof(ISP_WB_ATTR_S), 1, fp);

	ISP_AWB_ATTR_EX_S stAWBAttrEx = {0};

	CVI_ISP_GetAWBAttrEx(ViPipe, &stAWBAttrEx);
	fwrite(&stAWBAttrEx, sizeof(ISP_AWB_ATTR_EX_S), 1, fp);

	ISP_AWB_Calibration_Gain_S stWBCalib = {0};

	CVI_ISP_GetWBCalibration(ViPipe, &stWBCalib);
	fwrite(&stWBCalib, sizeof(ISP_AWB_Calibration_Gain_S), 1, fp);

	ISP_AWB_Calibration_Gain_S_EX stWBCalibEx = {0};

	CVI_ISP_GetWBCalibrationEx(ViPipe, &stWBCalibEx);
	fwrite(&stWBCalibEx, sizeof(ISP_AWB_Calibration_Gain_S_EX), 1, fp);

	ISP_STATISTICS_CFG_S stStatCfg = {0};

	CVI_ISP_GetStatisticsConfig(ViPipe, &stStatCfg);
	fwrite(&stStatCfg, sizeof(ISP_STATISTICS_CFG_S), 1, fp);

	return CVI_SUCCESS;
}

CVI_S32 isp_3aBinAttr_get_parambuf(VI_PIPE ViPipe, CVI_U8 *buffer)
{
	// AE
	ISP_WDR_EXPOSURE_ATTR_S stWDRExpAttr = { 0 };

	CVI_ISP_GetWDRExposureAttr(ViPipe, &stWDRExpAttr);
	memcpy(buffer, &stWDRExpAttr, sizeof(ISP_WDR_EXPOSURE_ATTR_S));
	buffer += sizeof(ISP_WDR_EXPOSURE_ATTR_S);

	ISP_EXPOSURE_ATTR_S stExpAttr = { 0 };

	CVI_ISP_GetExposureAttr(ViPipe, &stExpAttr);
	stExpAttr.u8DebugMode = 0;
	memcpy(buffer, &stExpAttr, sizeof(ISP_EXPOSURE_ATTR_S));
	buffer += sizeof(ISP_EXPOSURE_ATTR_S);

	ISP_AE_ROUTE_S stAeRouteAttr = { 0 };

	CVI_ISP_GetAERouteAttr(ViPipe, &stAeRouteAttr);
	memcpy(buffer, &stAeRouteAttr, sizeof(ISP_AE_ROUTE_S));
	buffer += sizeof(ISP_AE_ROUTE_S);

	ISP_AE_ROUTE_EX_S stAeRouteAttrEx = { 0 };

	CVI_ISP_GetAERouteAttrEx(ViPipe, &stAeRouteAttrEx);
	memcpy(buffer, &stAeRouteAttrEx, sizeof(ISP_AE_ROUTE_EX_S));
	buffer += sizeof(ISP_AE_ROUTE_EX_S);

	ISP_SMART_EXPOSURE_ATTR_S stAeSmartExposureAttr = { 0 };

	CVI_ISP_GetSmartExposureAttr(ViPipe, &stAeSmartExposureAttr);
	memcpy(buffer, &stAeSmartExposureAttr, sizeof(ISP_SMART_EXPOSURE_ATTR_S));
	buffer += sizeof(ISP_SMART_EXPOSURE_ATTR_S);

	ISP_IRIS_ATTR_S stAeIrisAttr = { 0 };

	CVI_ISP_GetIrisAttr(ViPipe, &stAeIrisAttr);
	memcpy(buffer, &stAeIrisAttr, sizeof(ISP_IRIS_ATTR_S));
	buffer += sizeof(ISP_IRIS_ATTR_S);

	ISP_DCIRIS_ATTR_S stAeDcIrisAttr = { 0 };

	CVI_ISP_GetDcirisAttr(ViPipe, &stAeDcIrisAttr);
	memcpy(buffer, &stAeDcIrisAttr, sizeof(ISP_DCIRIS_ATTR_S));
	buffer += sizeof(ISP_DCIRIS_ATTR_S);

	ISP_AE_ROUTE_S stAeRouteSFAttr = { 0 };

	CVI_ISP_GetAERouteSFAttr(ViPipe, &stAeRouteSFAttr);
	memcpy(buffer, &stAeRouteSFAttr, sizeof(ISP_AE_ROUTE_S));
	buffer += sizeof(ISP_AE_ROUTE_S);

	ISP_AE_ROUTE_EX_S stAeRouteSFAttrEx = { 0 };

	CVI_ISP_GetAERouteSFAttrEx(ViPipe, &stAeRouteSFAttrEx);
	memcpy(buffer, &stAeRouteSFAttrEx, sizeof(ISP_AE_ROUTE_EX_S));
	buffer += sizeof(ISP_AE_ROUTE_EX_S);

	// AWB
	ISP_WB_ATTR_S stWBAttr = {0};

	CVI_ISP_GetWBAttr(ViPipe, &stWBAttr);
	stWBAttr.u8DebugMode = 0;
	memcpy(buffer, &stWBAttr, sizeof(ISP_WB_ATTR_S));
	buffer += sizeof(ISP_WB_ATTR_S);

	ISP_AWB_ATTR_EX_S stAWBAttrEx = {0};

	CVI_ISP_GetAWBAttrEx(ViPipe, &stAWBAttrEx);
	memcpy(buffer, &stAWBAttrEx, sizeof(ISP_AWB_ATTR_EX_S));
	buffer += sizeof(ISP_AWB_ATTR_EX_S);

	ISP_AWB_Calibration_Gain_S stWBCalib = {0};

	CVI_ISP_GetWBCalibration(ViPipe, &stWBCalib);
	memcpy(buffer, &stWBCalib, sizeof(ISP_AWB_Calibration_Gain_S));
	buffer += sizeof(ISP_AWB_Calibration_Gain_S);

	ISP_AWB_Calibration_Gain_S_EX stWBCalibEx = {0};

	CVI_ISP_GetWBCalibrationEx(ViPipe, &stWBCalibEx);
	memcpy(buffer, &stWBCalibEx, sizeof(ISP_AWB_Calibration_Gain_S_EX));
	buffer += sizeof(ISP_AWB_Calibration_Gain_S_EX);

	ISP_STATISTICS_CFG_S stStatCfg = { 0 };

	CVI_ISP_GetStatisticsConfig(ViPipe, &stStatCfg);
	memcpy(buffer, &stStatCfg, sizeof(ISP_STATISTICS_CFG_S));
	buffer += sizeof(ISP_STATISTICS_CFG_S);

	return CVI_SUCCESS;
}

CVI_S32 isp_3aBinAttr_set_param(VI_PIPE ViPipe, CVI_U8 **binPtr)
{
	// AE
	ISP_WDR_EXPOSURE_ATTR_S stWDRExpAttr = { 0 };

	memcpy(&stWDRExpAttr, (*binPtr), sizeof(ISP_WDR_EXPOSURE_ATTR_S));
	(*binPtr) += sizeof(ISP_WDR_EXPOSURE_ATTR_S);
	CVI_ISP_SetWDRExposureAttr(ViPipe, &stWDRExpAttr);

	ISP_EXPOSURE_ATTR_S stExpAttr = { 0 };

	memcpy(&stExpAttr, (*binPtr), sizeof(ISP_EXPOSURE_ATTR_S));
	stExpAttr.u8DebugMode = 0;
	(*binPtr) += sizeof(ISP_EXPOSURE_ATTR_S);
	CVI_ISP_SetExposureAttr(ViPipe, &stExpAttr);

	ISP_AE_ROUTE_S stAeRouteAttr = { 0 };

	memcpy(&stAeRouteAttr, (*binPtr), sizeof(ISP_AE_ROUTE_S));
	(*binPtr) += sizeof(ISP_AE_ROUTE_S);
	CVI_ISP_SetAERouteAttr(ViPipe, &stAeRouteAttr);

	ISP_AE_ROUTE_EX_S stAeRouteAttrEx = { 0 };

	memcpy(&stAeRouteAttrEx, (*binPtr), sizeof(ISP_AE_ROUTE_EX_S));
	(*binPtr) += sizeof(ISP_AE_ROUTE_EX_S);
	CVI_ISP_SetAERouteAttrEx(ViPipe, &stAeRouteAttrEx);

	ISP_SMART_EXPOSURE_ATTR_S stAeSmartExposureAttr = { 0 };

	memcpy(&stAeSmartExposureAttr, (*binPtr), sizeof(ISP_SMART_EXPOSURE_ATTR_S));
	(*binPtr) += sizeof(ISP_SMART_EXPOSURE_ATTR_S);
	CVI_ISP_SetSmartExposureAttr(ViPipe, &stAeSmartExposureAttr);

	ISP_IRIS_ATTR_S stAeIrisAttr = { 0 };

	memcpy(&stAeIrisAttr, (*binPtr), sizeof(ISP_IRIS_ATTR_S));
	(*binPtr) += sizeof(ISP_IRIS_ATTR_S);
	CVI_ISP_SetIrisAttr(ViPipe, &stAeIrisAttr);

	ISP_DCIRIS_ATTR_S stAeDcIrisAttr = { 0 };

	memcpy(&stAeDcIrisAttr, (*binPtr), sizeof(ISP_DCIRIS_ATTR_S));
	(*binPtr) += sizeof(ISP_DCIRIS_ATTR_S);
	CVI_ISP_SetDcirisAttr(ViPipe, &stAeDcIrisAttr);

	ISP_AE_ROUTE_S stAeRouteSFAttr = { 0 };

	memcpy(&stAeRouteSFAttr, (*binPtr), sizeof(ISP_AE_ROUTE_S));
	(*binPtr) += sizeof(ISP_AE_ROUTE_S);
	CVI_ISP_SetAERouteSFAttr(ViPipe, &stAeRouteSFAttr);

	ISP_AE_ROUTE_EX_S stAeRouteSFAttrEx = { 0 };

	memcpy(&stAeRouteSFAttrEx, (*binPtr), sizeof(ISP_AE_ROUTE_EX_S));
	(*binPtr) += sizeof(ISP_AE_ROUTE_EX_S);
	CVI_ISP_SetAERouteSFAttrEx(ViPipe, &stAeRouteSFAttrEx);

	// AWB
	ISP_WB_ATTR_S stWBAttr = {0};

	memcpy(&stWBAttr, (*binPtr), sizeof(ISP_WB_ATTR_S));
	stWBAttr.u8DebugMode = 0;
	(*binPtr) += sizeof(ISP_WB_ATTR_S);
	CVI_ISP_SetWBAttr(ViPipe, &stWBAttr);

	ISP_AWB_ATTR_EX_S stAWBAttrEx = {0};

	memcpy(&stAWBAttrEx, (*binPtr), sizeof(ISP_AWB_ATTR_EX_S));
	(*binPtr) += sizeof(ISP_AWB_ATTR_EX_S);
	CVI_ISP_SetAWBAttrEx(ViPipe, &stAWBAttrEx);

	ISP_AWB_Calibration_Gain_S stWBCalib = {0};

	memcpy(&stWBCalib, (*binPtr), sizeof(ISP_AWB_Calibration_Gain_S));
	(*binPtr) += sizeof(ISP_AWB_Calibration_Gain_S);
	CVI_ISP_SetWBCalibration(ViPipe, &stWBCalib);

	ISP_AWB_Calibration_Gain_S_EX stWBCalibEx = {0};

	memcpy(&stWBCalibEx, (*binPtr), sizeof(ISP_AWB_Calibration_Gain_S_EX));
	(*binPtr) += sizeof(ISP_AWB_Calibration_Gain_S_EX);
	CVI_ISP_SetWBCalibrationEx(ViPipe, &stWBCalibEx);

	ISP_STATISTICS_CFG_S stStatCfg = {0};

	memcpy(&stStatCfg, (*binPtr), sizeof(ISP_STATISTICS_CFG_S));
	(*binPtr) += sizeof(ISP_STATISTICS_CFG_S);
	CVI_ISP_SetStatisticsConfig(ViPipe, &stStatCfg);

	return CVI_SUCCESS;
}

CVI_S32 isp_3aJsonAttr_set_param(VI_PIPE ViPipe, ISP_3A_Parameter_Structures *pstPtr)
{
	// AE
	CVI_ISP_SetWDRExposureAttr(ViPipe, &pstPtr->WDRExpAttr);
	pstPtr->ExpAttr.u8DebugMode = 0;
	CVI_ISP_SetExposureAttr(ViPipe, &pstPtr->ExpAttr);
	CVI_ISP_SetAERouteAttr(ViPipe, &pstPtr->AeRouteAttr);
	CVI_ISP_SetAERouteAttrEx(ViPipe, &pstPtr->AeRouteAttrEx);
	CVI_ISP_SetSmartExposureAttr(ViPipe, &pstPtr->AeSmartExposureAttr);
	CVI_ISP_SetIrisAttr(ViPipe, &pstPtr->AeIrisAttr);
	CVI_ISP_SetDcirisAttr(ViPipe, &pstPtr->AeDcirisAttr);
	CVI_ISP_SetAERouteSFAttr(ViPipe, &pstPtr->AeRouteSFAttr);
	CVI_ISP_SetAERouteSFAttrEx(ViPipe, &pstPtr->AeRouteSFAttrEx);

	// AWB
	pstPtr->WBAttr.u8DebugMode = 0;
	CVI_ISP_SetWBAttr(ViPipe, &pstPtr->WBAttr);
	CVI_ISP_SetAWBAttrEx(ViPipe, &pstPtr->AWBAttrEx);
	CVI_ISP_SetWBCalibration(ViPipe, &pstPtr->WBCalib);
	CVI_ISP_SetWBCalibrationEx(ViPipe, &pstPtr->WBCalibEx);
	CVI_ISP_SetStatisticsConfig(ViPipe, &pstPtr->StatCfg);

	return CVI_SUCCESS;
}

CVI_S32 isp_3aJsonAttr_get_param(VI_PIPE ViPipe, ISP_3A_Parameter_Structures *pstPtr)
{
	memset(pstPtr, 0, sizeof(ISP_3A_Parameter_Structures));

	// AE
	CVI_ISP_GetWDRExposureAttr(ViPipe, &pstPtr->WDRExpAttr);
	CVI_ISP_GetExposureAttr(ViPipe, &pstPtr->ExpAttr);
	pstPtr->ExpAttr.u8DebugMode = 0;
	CVI_ISP_GetAERouteAttr(ViPipe, &pstPtr->AeRouteAttr);
	CVI_ISP_GetAERouteAttrEx(ViPipe, &pstPtr->AeRouteAttrEx);
	CVI_ISP_GetSmartExposureAttr(ViPipe, &pstPtr->AeSmartExposureAttr);
	CVI_ISP_GetIrisAttr(ViPipe, &pstPtr->AeIrisAttr);
	CVI_ISP_GetDcirisAttr(ViPipe, &pstPtr->AeDcirisAttr);
	CVI_ISP_GetAERouteSFAttr(ViPipe, &pstPtr->AeRouteSFAttr);
	CVI_ISP_GetAERouteSFAttrEx(ViPipe, &pstPtr->AeRouteSFAttrEx);

	// AWB
	CVI_ISP_GetWBAttr(ViPipe, &pstPtr->WBAttr);
	pstPtr->WBAttr.u8DebugMode = 0;
	CVI_ISP_GetAWBAttrEx(ViPipe, &pstPtr->AWBAttrEx);
	CVI_ISP_GetWBCalibration(ViPipe, &pstPtr->WBCalib);
	CVI_ISP_GetWBCalibrationEx(ViPipe, &pstPtr->WBCalibEx);
	CVI_ISP_GetStatisticsConfig(ViPipe, &pstPtr->StatCfg);

	return CVI_SUCCESS;
}
