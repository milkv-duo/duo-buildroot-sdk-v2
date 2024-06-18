/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_sensor.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "cvi_comm_sns.h"
#include "isp_main_local.h"
#include "isp_ioctl.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_sts_ctrl.h"
#include "../../../algo/ae/aealgo.h"

// #include <linux/vi_uapi.h>
// #include <linux/vi_isp.h>
// #include <linux/vi_snsr.h>

typedef struct _ISP_SNS_CTX_S {
	ISP_CMOS_DEFAULT_S stCmosDft;
	ISP_CMOS_BLACK_LEVEL_S stSnsBlackLevel;
	ISP_SNS_SYNC_INFO_S stSnsSyncCfg;
	ISP_SNS_ATTR_INFO_S snsAttr;
	ISP_SENSOR_REGISTER_S snsRegFunc;
} ISP_SNS_CTX_S;

ISP_SNS_CTX_S gSnsCtx[VI_MAX_PIPE_NUM];

#ifdef ARCH_RTOS_CV181X
#define RTOS_I2C_DELAY_MAX 4
#define RTOS_I2C_DELAY_FRM 0
CVI_U32 i2c_widx;
CVI_U32 i2c_ridx;
CVI_U32 i2c_preridx;
ISP_SNS_SYNC_INFO_S gSnsCfg[VI_MAX_PIPE_NUM][RTOS_I2C_DELAY_MAX];
#endif

#define SNS_CTX_GET(pipeIdx, pSnsCtx) (pSnsCtx = &(gSnsCtx[pipeIdx]))

static CVI_S32 isp_sensor_updDefault(VI_PIPE ViPipe);

CVI_S32 isp_sensor_get_crop_info(CVI_S32 ViPipe, ISP_SNS_SYNC_INFO_S *snsCropInfo)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	memcpy(snsCropInfo, &(pSnsInfo->stSnsSyncCfg), sizeof(ISP_SNS_SYNC_INFO_S));

	return 0;
}

CVI_S32 isp_sensor_register(CVI_S32 ViPipe, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo, ISP_SENSOR_REGISTER_S *pstRegister)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);
	if (pstSnsAttrInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	memcpy(&(pSnsInfo->snsAttr), pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));
	memcpy(&(pSnsInfo->snsRegFunc), pstRegister, sizeof(ISP_SENSOR_REGISTER_S));

	if (pstRegister->stSnsExp.pfn_cmos_sensor_global_init != CVI_NULL)
		pstRegister->stSnsExp.pfn_cmos_sensor_global_init(ViPipe);
	else {
		ISP_LOG_ERR("pfn_cmos_sensor_global_init is NULL\n");
		return CVI_FAILURE;
	}
	isp_sensor_updDefault(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 isp_sensor_unRegister(CVI_S32 ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	memset(&(pSnsInfo->snsAttr), 0, sizeof(ISP_SNS_ATTR_INFO_S));
	memset(&(pSnsInfo->snsRegFunc), 0, sizeof(ISP_SENSOR_REGISTER_S));
	return 0;
}

CVI_S32 isp_sensor_init(CVI_S32 ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_sensor_init != CVI_NULL)
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_sensor_init(ViPipe);
	else {
		ISP_LOG_WARNING("sensor init cb not registered\n");
		return CVI_FAILURE;
	}
	return 0;
}

static CVI_S32 isp_sensor_updDefault(VI_PIPE ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;
	ISP_CTX_S *pstIspCtx = CVI_NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);
	SNS_CTX_GET(ViPipe, pSnsInfo);
	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_isp_default != CVI_NULL) {
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_isp_default(ViPipe, &pSnsInfo->stCmosDft);
		/*TODO@CF. This value will give from sensor init. Write constant first.*/
		pstIspCtx->u8AEWaitFrame = 8;
	} else {
		ISP_LOG_WARNING("sensor get default not registered\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_sensor_updateBlc(VI_PIPE ViPipe, ISP_BLACK_LEVEL_ATTR_S **ppstSnsBlackLevel)
{
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);
	if (ppstSnsBlackLevel == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_isp_black_level != NULL) {
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_isp_black_level(ViPipe, &pSnsInfo->stSnsBlackLevel);
	} else {
		return CVI_FAILURE;
	}
	*ppstSnsBlackLevel = &(pSnsInfo->stSnsBlackLevel.blcAttr);

	return CVI_SUCCESS;
}

CVI_S32 isp_sensor_default_get(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S **ppstSnsDft)
{
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (ppstSnsDft == CVI_NULL) {
		return CVI_FAILURE;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);
	if (pSnsInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	*ppstSnsDft = &pSnsInfo->stCmosDft;

	return CVI_SUCCESS;
}

CVI_S32 isp_sensor_regCfg_get(VI_PIPE ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);
	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_sns_reg_info != CVI_NULL) {
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_get_sns_reg_info(ViPipe, &pSnsInfo->stSnsSyncCfg);
		pSnsInfo->stSnsSyncCfg.snsCfg.bConfig = CVI_TRUE;
	} else {
		ISP_LOG_WARNING("sensor reg config not registered\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

/*TODO@CF. This function not complete.*/
CVI_S32 isp_sensor_switchMode(CVI_S32 ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo;
	ISP_CMOS_SENSOR_IMAGE_MODE_S mode;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_image_mode != NULL)
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_image_mode(ViPipe, &mode);
	else {
		ISP_LOG_WARNING("sensor switch mode not registered\n");
		return CVI_FAILURE;
	}
	return 0;
}

CVI_S32 isp_sensor_setWdrMode(CVI_S32 ViPipe, WDR_MODE_E wdrMode)
{
	ISP_SNS_CTX_S *pSnsInfo;
	CVI_U8 mode = wdrMode;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_wdr_mode != NULL)
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_wdr_mode(ViPipe, mode);
	else {
		ISP_LOG_WARNING("sensor switch mode not registered\n");
		return CVI_FAILURE;
	}
	return 0;
}

CVI_S32 isp_sensor_exit(CVI_S32 ViPipe)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_sensor_exit != NULL)
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_sensor_exit(ViPipe);
	else {
		ISP_LOG_WARNING("sensor exit not registered\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 isp_sensor_getId(VI_PIPE ViPipe, SENSOR_ID *pSensorId)
{
	ISP_SNS_CTX_S *pSnsInfo;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	*pSensorId = pSnsInfo->snsAttr.eSensorId;

	return CVI_SUCCESS;
}

CVI_S32 isp_snsSync_info_set(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *syncCfg, CVI_U32 be_frm_idx)
{
#ifndef ARCH_RTOS_CV181X
	ISP_CTX_S *pstIspCtx = NULL;
	struct cvi_isp_snr_update snr_update;
	CVI_U32 reg_num = 0, i = 0;
	CVI_U8 u8SnsResponseFrame;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);
	if (syncCfg == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (syncCfg->snsCfg.enSnsType == SNS_I2C_TYPE)
		snr_update.snr_cfg_node.snsr.sns_type = ISP_SNS_I2C_TYPE;
	else {
		ISP_LOG_ERR("Current only support i2c sensor\n");
		return CVI_FAILURE;
	}
	snr_update.raw_num = ViPipe;
	snr_update.snr_cfg_node.snsr.regs_num = reg_num = syncCfg->snsCfg.u32RegNum;
	if (pstIspCtx->stAeResult.u8MeterFramePeriod == 0) {
		pstIspCtx->stAeResult.u8MeterFramePeriod = AE_GetSensorPeriod(ViPipe);
	}
	if (!AE_GetSensorExpGainPeriod(ViPipe))
		u8SnsResponseFrame = pstIspCtx->stAeResult.u8MeterFramePeriod;
	else
		u8SnsResponseFrame = AE_GetSensorExpGainPeriod(ViPipe);

	snr_update.snr_cfg_node.snsr.magic_num = be_frm_idx + 5 - u8SnsResponseFrame;
	snr_update.snr_cfg_node.snsr.magic_num_vblank = be_frm_idx + 5 - u8SnsResponseFrame;

	ISP_LOG_INFO("p:%d, b:%d, m:%d\n", ViPipe, be_frm_idx,
		snr_update.snr_cfg_node.snsr.magic_num);
	for (i = 0; i < reg_num; i++) {
		snr_update.snr_cfg_node.snsr.i2c_data[i].update = syncCfg->snsCfg.astI2cData[i].bUpdate;
		snr_update.snr_cfg_node.snsr.i2c_data[i].dly_frm_num = syncCfg->snsCfg.astI2cData[i].u8DelayFrmNum;
	#if USE_USER_SEN_DRIVER
		snr_update.snr_cfg_node.snsr.i2c_data[i].i2c_dev = syncCfg->snsCfg.unComBus.s8I2cDev;
	#else
		snr_update.snr_cfg_node.snsr.i2c_data[i].i2c_dev = syncCfg->snsCfg.astI2cData[i].u8IntPos;
	#endif
		snr_update.snr_cfg_node.snsr.i2c_data[i].dev_addr = syncCfg->snsCfg.astI2cData[i].u8DevAddr;
		snr_update.snr_cfg_node.snsr.i2c_data[i].reg_addr = syncCfg->snsCfg.astI2cData[i].u32RegAddr;
		snr_update.snr_cfg_node.snsr.i2c_data[i].addr_bytes = syncCfg->snsCfg.astI2cData[i].u32AddrByteNum;
		snr_update.snr_cfg_node.snsr.i2c_data[i].data = syncCfg->snsCfg.astI2cData[i].u32Data;
		snr_update.snr_cfg_node.snsr.i2c_data[i].data_bytes = syncCfg->snsCfg.astI2cData[i].u32DataByteNum;
		snr_update.snr_cfg_node.snsr.i2c_data[i].vblank_update = syncCfg->snsCfg.astI2cData[i].bvblankUpdate;

		snr_update.snr_cfg_node.snsr.i2c_data[i].drop_frame = syncCfg->snsCfg.astI2cData[i].bDropFrm;
		snr_update.snr_cfg_node.snsr.i2c_data[i].drop_frame_cnt = syncCfg->snsCfg.astI2cData[i].u8DropFrmNum;

		if (snr_update.snr_cfg_node.snsr.i2c_data[i].update ||
			snr_update.snr_cfg_node.snsr.i2c_data[i].vblank_update) {
			ISP_LOG_DEBUG("u:%d,%d,da:0x%x,ra:0x%x,d:0x%x,(%d,%d,%d,%d,%d)\n",
				snr_update.snr_cfg_node.snsr.i2c_data[i].update,
				snr_update.snr_cfg_node.snsr.i2c_data[i].vblank_update,
				snr_update.snr_cfg_node.snsr.i2c_data[i].dev_addr,
				snr_update.snr_cfg_node.snsr.i2c_data[i].reg_addr,
				snr_update.snr_cfg_node.snsr.i2c_data[i].data,
				snr_update.snr_cfg_node.snsr.i2c_data[i].dly_frm_num,
				snr_update.snr_cfg_node.snsr.i2c_data[i].addr_bytes,
				snr_update.snr_cfg_node.snsr.i2c_data[i].data_bytes,
				snr_update.snr_cfg_node.snsr.i2c_data[i].drop_frame,
				snr_update.snr_cfg_node.snsr.i2c_data[i].drop_frame_cnt);
		}
	}
	snr_update.snr_cfg_node.snsr.use_snsr_sram = syncCfg->snsCfg.use_snsr_sram;
	snr_update.snr_cfg_node.snsr.need_update = syncCfg->snsCfg.need_update;
	snr_update.snr_cfg_node.isp.need_update = syncCfg->ispCfg.need_update;
	snr_update.snr_cfg_node.isp.wdr.frm_num = syncCfg->ispCfg.frm_num;
	snr_update.snr_cfg_node.isp.dly_frm_num = syncCfg->ispCfg.u8DelayFrmNum;
	//printf("need_update %d %d %d %d\n", snr_update.snr_cfg_node.snsr.need_update,
	//	syncCfg->ispCfg.frm_num, addIdx, pstIspCtx->ispDevFd);
	for (i = 0; i < syncCfg->ispCfg.frm_num; i++) {
		CVI_U32 sensorExpIndex = syncCfg->ispCfg.frm_num - 1 - i;

		snr_update.snr_cfg_node.isp.wdr.img_size[i].max_width =
			syncCfg->ispCfg.img_size[sensorExpIndex].stMaxSize.u32Width;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].max_height =
			syncCfg->ispCfg.img_size[sensorExpIndex].stMaxSize.u32Height;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].width =
			syncCfg->ispCfg.img_size[sensorExpIndex].stSnsSize.u32Width;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].height =
			syncCfg->ispCfg.img_size[sensorExpIndex].stSnsSize.u32Height;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].start_x =
			syncCfg->ispCfg.img_size[sensorExpIndex].stWndRect.s32X;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].start_y =
			syncCfg->ispCfg.img_size[sensorExpIndex].stWndRect.s32Y;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].active_w =
			syncCfg->ispCfg.img_size[sensorExpIndex].stWndRect.u32Width;
		snr_update.snr_cfg_node.isp.wdr.img_size[i].active_h =
			syncCfg->ispCfg.img_size[sensorExpIndex].stWndRect.u32Height;
	}
	snr_update.snr_cfg_node.cif.need_update = syncCfg->cifCfg.need_update;
	snr_update.snr_cfg_node.cif.wdr_manu.devno = syncCfg->cifCfg.wdr_manual.devno;
	snr_update.snr_cfg_node.cif.wdr_manu.attr.manual_en = syncCfg->cifCfg.wdr_manual.manual_en;
	snr_update.snr_cfg_node.cif.wdr_manu.attr.l2s_distance = syncCfg->cifCfg.wdr_manual.l2s_distance;
	snr_update.snr_cfg_node.cif.wdr_manu.attr.lsef_length = syncCfg->cifCfg.wdr_manual.lsef_length;
	snr_update.snr_cfg_node.cif.wdr_manu.attr.discard_padding_lines =
		syncCfg->cifCfg.wdr_manual.discard_padding_lines;
	snr_update.snr_cfg_node.cif.wdr_manu.attr.update = syncCfg->cifCfg.wdr_manual.update;
	snr_update.snr_cfg_node.configed = 1;

	if (snr_update.snr_cfg_node.snsr.need_update == 1)
		S_EXT_CTRLS_PTR(VI_IOCTL_SET_SNR_CFG_NODE, &snr_update);
#else
	UNUSED(ViPipe);
	UNUSED(syncCfg);
	UNUSED(be_frm_idx);
#endif
	return 0;
}

CVI_S32 isp_snsSync_cfg_set(VI_PIPE ViPipe)
{
	CVI_S32 s32Ret = 0;
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *snsSyncInfo = CVI_NULL;


	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SNS_CTX_GET(ViPipe, pSnsInfo);

	s32Ret = isp_sensor_regCfg_get(ViPipe);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_ERR("isp_sensor_regCfg_get fail\n");
		return s32Ret;
	}
	snsSyncInfo = &pSnsInfo->stSnsSyncCfg;

#ifdef ARCH_RTOS_CV181X
	ISP_SNS_SYNC_INFO_S *preSnsSyncInfo = CVI_NULL;

	memcpy(&(gSnsCfg[ViPipe][i2c_widx]), snsSyncInfo, sizeof(ISP_SNS_SYNC_INFO_S));
	snsSyncInfo = &gSnsCfg[ViPipe][i2c_ridx];
	preSnsSyncInfo = &gSnsCfg[ViPipe][i2c_preridx];

	for (CVI_U32 i = 0 ; i < preSnsSyncInfo->snsCfg.u32RegNum ; i++) {
		if (preSnsSyncInfo->snsCfg.astI2cData[i].bUpdate == true) {
			memcpy(&snsSyncInfo->snsCfg.astI2cData[snsSyncInfo->snsCfg.u32RegNum],
				&preSnsSyncInfo->snsCfg.astI2cData[i], sizeof(ISP_I2C_DATA_S));
			preSnsSyncInfo->snsCfg.astI2cData[i].bUpdate = false;
			snsSyncInfo->snsCfg.u32RegNum++;
		}
	}

	if (pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_sns_reg_info != CVI_NULL) {
		pSnsInfo->snsRegFunc.stSnsExp.pfn_cmos_set_sns_reg_info(ViPipe, snsSyncInfo);
		pSnsInfo->stSnsSyncCfg.snsCfg.bConfig = CVI_TRUE;
	} else {
		ISP_LOG_WARNING("sensor reg config not registered\n");
		return CVI_FAILURE;
	}

	i2c_widx = ((i2c_widx + 1) % RTOS_I2C_DELAY_MAX);
	i2c_ridx = ((i2c_ridx + 1) % RTOS_I2C_DELAY_MAX);
	i2c_preridx = ((i2c_preridx + 1) % RTOS_I2C_DELAY_MAX);
#else

	CVI_U32 be_frm_idx = 0;

	isp_sts_ctrl_get_pre_be_frm_idx(ViPipe, &be_frm_idx);

	s32Ret = isp_snsSync_info_set(ViPipe, snsSyncInfo, be_frm_idx);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_ERR("isp_snsSync_info_set fail\n");
		return s32Ret;
	}
#endif

	return CVI_SUCCESS;
}

CVI_S32 isp_sns_info_init(VI_PIPE ViPipe)
{
#ifndef ARCH_RTOS_CV181X
	CVI_U32 i;
	CVI_S32 s32Ret = 0;
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_SNS_CTX_S *pSnsInfo = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *snsSyncInfo = CVI_NULL;
	// struct vi_ext_control ec1;
#endif

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}
#ifndef ARCH_RTOS_CV181X
	SNS_CTX_GET(ViPipe, pSnsInfo);
	ISP_GET_CTX(ViPipe, pstIspCtx);
#endif
#ifdef ARCH_RTOS_CV181X
	i2c_ridx = 0;
	i2c_widx = i2c_ridx + RTOS_I2C_DELAY_FRM;
	i2c_preridx = (i2c_ridx - 1) % RTOS_I2C_DELAY_MAX;
	memset(gSnsCfg[ViPipe], 0, sizeof(ISP_SNS_SYNC_INFO_S) * RTOS_I2C_DELAY_MAX);
#endif

#ifndef ARCH_RTOS_CV181X
	struct cvi_isp_snr_info cfg;

	// memset(&cfg, 0, sizeof(struct cvi_isp_snr_info));

	s32Ret = isp_sensor_regCfg_get(ViPipe);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_ERR("isp_sensor_regCfg_get fail\n");
		return s32Ret;
	}
	snsSyncInfo = &pSnsInfo->stSnsSyncCfg;

	cfg.raw_num = ViPipe;
	cfg.color_mode = pstIspCtx->enBayer;

	cfg.snr_fmt.frm_num = snsSyncInfo->ispCfg.frm_num;
	for (i = 0; i < snsSyncInfo->ispCfg.frm_num; i++) {
		CVI_U32 snsExpIndex = snsSyncInfo->ispCfg.frm_num - 1 - i;

		cfg.snr_fmt.img_size[i].max_width = snsSyncInfo->ispCfg.img_size[snsExpIndex].stMaxSize.u32Width;
		cfg.snr_fmt.img_size[i].max_height = snsSyncInfo->ispCfg.img_size[snsExpIndex].stMaxSize.u32Height;
		cfg.snr_fmt.img_size[i].start_x = snsSyncInfo->ispCfg.img_size[snsExpIndex].stWndRect.s32X;
		cfg.snr_fmt.img_size[i].start_y = snsSyncInfo->ispCfg.img_size[snsExpIndex].stWndRect.s32Y;
		cfg.snr_fmt.img_size[i].active_w = snsSyncInfo->ispCfg.img_size[snsExpIndex].stWndRect.u32Width;
		cfg.snr_fmt.img_size[i].active_h = snsSyncInfo->ispCfg.img_size[snsExpIndex].stWndRect.u32Height;
		cfg.snr_fmt.img_size[i].width = snsSyncInfo->ispCfg.img_size[snsExpIndex].stSnsSize.u32Width;
		cfg.snr_fmt.img_size[i].height = snsSyncInfo->ispCfg.img_size[snsExpIndex].stSnsSize.u32Height;
	}

	isp_sensor_init(ViPipe);

	S_EXT_CTRLS_PTR(VI_IOCTL_SET_SNR_INFO, &cfg);
#endif

	return CVI_SUCCESS;
}
