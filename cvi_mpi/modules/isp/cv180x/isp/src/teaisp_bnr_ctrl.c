/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_bnr_ctrl.c
 * Description:
 *
 */

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"

#include "isp_mgr_buf.h"
#include "teaisp_bnr_ctrl.h"
#include "teaisp_helper.h"

#include "list.h"

#define ENABLE_PRELOAD_BNR_MODEL 1

typedef struct {
	struct list_head list;
	TEAISP_BNR_MODEL_INFO_S model_info;
	void *model;
} TEAISP_BNR_MODEL_S;

typedef struct {
	struct list_head model_info_head;
	TEAISP_BNR_MODEL_S *pcurr_model;

	CVI_BOOL bnrThreadEnable;
	pthread_t tid;
	sem_t sem;
} TEAISP_BNR_HANDLE_S;

const struct isp_module_ctrl teaisp_bnr_mod = {
	.init = teaisp_bnr_ctrl_init,
	.uninit = teaisp_bnr_ctrl_uninit,
	.suspend = teaisp_bnr_ctrl_suspend,
	.resume = teaisp_bnr_ctrl_resume,
	.ctrl = teaisp_bnr_ctrl_ctrl
};

static CVI_S32 teaisp_bnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_bnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_bnr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 teaisp_bnr_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_bnr_proc_info(VI_PIPE ViPipe);
static CVI_S32 teaisp_bnr_ctrl_check_bnr_attr_valid(const TEAISP_BNR_ATTR_S *pstBNRAttr);

static struct teaisp_bnr_ctrl_runtime  *_get_bnr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 teaisp_bnr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	runtime->is_teaisp_bnr_running = CVI_FALSE;
	runtime->is_teaisp_bnr_enable = CVI_FALSE;

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	// free mem
	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	if (handle != CVI_NULL) {

		ISP_LOG_INFO("destroy and join bnr thread--\n");
		handle->bnrThreadEnable = CVI_FALSE;
		sem_post(&handle->sem);
		pthread_join(handle->tid, NULL);
		sem_destroy(&handle->sem);
		ISP_LOG_INFO("destroy and join bnr thread--\n");

		if (runtime->is_teaisp_bnr_running) {
			teaisp_bnr_set_driver_stop(ViPipe);
		}

		struct list_head *pos, *n;
		TEAISP_BNR_MODEL_S *model;

		list_for_each_safe(pos, n, &handle->model_info_head) {
			model = list_entry(pos, TEAISP_BNR_MODEL_S, list);

			if (model->model != NULL) {
				teaisp_bnr_unload_model(ViPipe, model->model);
				model->model = CVI_NULL;
			}

			list_del(pos);
			ISP_RELEASE_MEMORY(model);
		}

		ISP_RELEASE_MEMORY(handle);
		runtime->handle = CVI_NULL;

		if (runtime->is_teaisp_bnr_running) {
			teaisp_bnr_set_driver_deinit(ViPipe);
			runtime->is_teaisp_bnr_running = CVI_FALSE;
		}
	}

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		teaisp_bnr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassAiBnr;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassAiBnr = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
#ifdef ENABLE_PRELOAD_BNR_MODEL
static void bnr_load_all_bmodel(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	struct list_head *pos, *n;
	TEAISP_BNR_MODEL_S *model;

	list_for_each_safe(pos, n, &handle->model_info_head) {
		model = list_entry(pos, TEAISP_BNR_MODEL_S, list);

		if (model->model == NULL) {
			ret = teaisp_bnr_load_model(ViPipe, model->model_info.path, &model->model);
			if (ret != CVI_SUCCESS) {
				model->model = NULL;
			}
		}
	}
}

static void bnr_unload_all_bmodel(VI_PIPE ViPipe)
{
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	struct list_head *pos, *n;
	TEAISP_BNR_MODEL_S *model;

	list_for_each_safe(pos, n, &handle->model_info_head) {
		model = list_entry(pos, TEAISP_BNR_MODEL_S, list);

		if (model->model != NULL) {
			teaisp_bnr_unload_model(ViPipe, model->model);
			model->model = CVI_NULL;
		}
	}
}
#endif

static void *bnr_work_thread(void *arg)
{
	CVI_S32 ret = CVI_SUCCESS;
	VI_PIPE ViPipe = (VI_PIPE)(uintptr_t)arg;

	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);
	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	CVI_U32 u32ISO = 0;
	CVI_U32 u32UpdateCnt = 0;

	struct teaisp_bnr_config bnr_cfg;
	const TEAISP_BNR_ATTR_S *bnr_attr = NULL;

	while (handle->bnrThreadEnable) {

		sem_wait(&handle->sem);

		if (!handle->bnrThreadEnable) {
			break;
		}

		u32ISO = runtime->u32CurrentISO;
		teaisp_bnr_ctrl_get_bnr_attr(ViPipe, &bnr_attr);

		TEAISP_BNR_MODEL_S *temp = list_entry(handle->model_info_head.next, TEAISP_BNR_MODEL_S, list);

		if (!bnr_attr->enable || u32ISO < temp->model_info.enterISO) {

			if (runtime->is_teaisp_bnr_running) {

				if (bnr_attr->enable && handle->pcurr_model == temp &&
					u32ISO > (temp->model_info.enterISO - temp->model_info.tolerance)) {
					continue;
				}

				teaisp_bnr_set_driver_stop(ViPipe);
#ifndef ENABLE_PRELOAD_BNR_MODEL
				teaisp_bnr_unload_model(ViPipe, handle->pcurr_model->model);
				handle->pcurr_model->model = CVI_NULL;
#endif
				handle->pcurr_model = CVI_NULL;

				runtime->is_teaisp_bnr_running = CVI_FALSE;
			}

			if (runtime->is_teaisp_bnr_enable) {
#ifdef ENABLE_PRELOAD_BNR_MODEL
				bnr_unload_all_bmodel(ViPipe);
#endif
				ret = teaisp_bnr_set_driver_deinit(ViPipe);

				if (ret != CVI_SUCCESS) {
					ISP_LOG_ERR("driver deinit error...\n");
					continue;
				}

				runtime->is_teaisp_bnr_enable = CVI_FALSE;
			}

			continue;

		} else {
			if (!runtime->is_teaisp_bnr_enable) {
#ifdef ENABLE_PRELOAD_BNR_MODEL
				bnr_load_all_bmodel(ViPipe);
#endif
				ret = teaisp_bnr_set_driver_init(ViPipe);

				if (ret != CVI_SUCCESS) {
					ISP_LOG_ERR("driver init error...\n");
					continue;
				}

				runtime->is_teaisp_bnr_enable = CVI_TRUE;
			}

			struct list_head *pos;

			list_for_each_prev(pos, &handle->model_info_head) {
				temp = list_entry(pos, TEAISP_BNR_MODEL_S, list);
				if (u32ISO >= temp->model_info.enterISO) {
					break;
				}
			}

			if (handle->pcurr_model == NULL) {
				handle->pcurr_model = temp;
#ifndef ENABLE_PRELOAD_BNR_MODEL
				// load model && set api info
				//ISP_LOG_INFO("load model: %s, %d, %d\n",
				printf("load model: %s, %d, %d\n",
					temp->model_info.path, temp->model_info.enterISO, temp->model_info.tolerance);
				teaisp_bnr_load_model(ViPipe, temp->model_info.path, &temp->model);
#else
				printf("enter model: %s, %d, %d\n",
					temp->model_info.path, temp->model_info.enterISO, temp->model_info.tolerance);
#endif
				u32UpdateCnt = 0;
				runtime->bnr_cfg.blend = 0x0;
				bnr_cfg = runtime->bnr_cfg;
				teaisp_bnr_set_api_info((int) ViPipe, temp->model, (void *) &bnr_cfg, 1);
			}

			if (temp != handle->pcurr_model) {
#ifndef ENABLE_PRELOAD_BNR_MODEL
				TEAISP_BNR_MODEL_S *pprev = CVI_NULL;
#endif
				ISP_LOG_INFO("ISO: %d, next model enterISO: %d, curr model enterISO: %d, %d\n",
					runtime->u32CurrentISO, temp->model_info.enterISO,
					handle->pcurr_model->model_info.enterISO,
					handle->pcurr_model->model_info.tolerance);

				if (temp->model_info.enterISO > handle->pcurr_model->model_info.enterISO) {
#ifndef ENABLE_PRELOAD_BNR_MODEL
					pprev = handle->pcurr_model;
#endif
					handle->pcurr_model = temp;
				}

				if (temp->model_info.enterISO < handle->pcurr_model->model_info.enterISO &&
					u32ISO < (handle->pcurr_model->model_info.enterISO -
						handle->pcurr_model->model_info.tolerance)) {
#ifndef ENABLE_PRELOAD_BNR_MODEL
					pprev = handle->pcurr_model;
#endif
					handle->pcurr_model = temp;
				}

				if (temp == handle->pcurr_model) {
#ifndef ENABLE_PRELOAD_BNR_MODEL
					// load model && set api info
					//ISP_LOG_INFO("load model: %s, %d, %d\n",
					printf("load model: %s, %d, %d\n",
						temp->model_info.path, temp->model_info.enterISO,
						temp->model_info.tolerance);
					teaisp_bnr_load_model(ViPipe, temp->model_info.path, &temp->model);
#else
					printf("enter model: %s, %d, %d\n",
						temp->model_info.path, temp->model_info.enterISO, temp->model_info.tolerance);
#endif
					//u32UpdateCnt = 0;
					//runtime->bnr_cfg.blend = 0x0;
					//bnr_cfg = runtime->bnr_cfg;
					//teaisp_bnr_set_api_info((int) ViPipe, temp->model, (void *) &bnr_cfg, 1);
#ifndef ENABLE_PRELOAD_BNR_MODEL
					teaisp_bnr_unload_model(ViPipe, pprev->model);
					pprev->model = CVI_NULL;
#endif
				}
			}

			if (!runtime->is_teaisp_bnr_running) {
				runtime->is_teaisp_bnr_running = CVI_TRUE;
				teaisp_bnr_set_driver_start(ViPipe);
			}

			if (u32UpdateCnt > 0x0 && handle->pcurr_model != CVI_NULL) {
				runtime->bnr_cfg.blend = 0x3F800000; // !!! float 1.0

				if (runtime->bnr_cfg.update &&
					memcmp(&bnr_cfg, &runtime->bnr_cfg, sizeof(struct teaisp_bnr_config)) != 0) {
				//if (runtime->bnr_cfg.update) {
					bnr_cfg = runtime->bnr_cfg;
					teaisp_bnr_set_api_info((int) ViPipe, handle->pcurr_model->model, (void *) &bnr_cfg, 0);
				}
			}

			u32UpdateCnt++;
		}
	}

	ISP_LOG_INFO("bnr work thread end...\n");

	return NULL;
}

static CVI_S32 teaisp_bnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = teaisp_bnr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = teaisp_bnr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = teaisp_bnr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_bnr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 teaisp_bnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const TEAISP_BNR_ATTR_S *bnr_attr = NULL;

	teaisp_bnr_ctrl_get_bnr_attr(ViPipe, &bnr_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(bnr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	runtime->u32CurrentISO = algoResult->u32PreBlcIso;

	// No need to update parameters if disable. Because its meaningless
	if (!bnr_attr->enable || runtime->is_module_bypass) {
		return ret;
	}

	if (bnr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(bnr_attr, FilterMotionStr2D);
		MANUAL(bnr_attr, FilterStaticStr2D);
		MANUAL(bnr_attr, FilterStr3D);
		MANUAL(bnr_attr, FilterStr2D);
		MANUAL(bnr_attr, NoiseLevel);
		MANUAL(bnr_attr, NoiseHiLevel);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(bnr_attr, FilterMotionStr2D, INTPLT_POST_ISO);
		AUTO(bnr_attr, FilterStaticStr2D, INTPLT_POST_ISO);
		AUTO(bnr_attr, FilterStr3D, INTPLT_POST_ISO);
		AUTO(bnr_attr, FilterStr2D, INTPLT_POST_ISO);
		AUTO(bnr_attr, NoiseLevel, INTPLT_POST_ISO);
		AUTO(bnr_attr, NoiseHiLevel, INTPLT_POST_ISO);

		#undef AUTO
	}

	const TEAISP_BNR_NP_S *p = CVI_NULL;
	teaisp_bnr_ctrl_get_np_attr(ViPipe, &p);
	runtime->bnr_param_in.np = (ISP_VOID_PTR)(uintptr_t) p;
	runtime->bnr_param_in.iso = algoResult->u32PreBlcIso;
	runtime->bnr_param_in.NoiseLevel = runtime->bnr_attr.NoiseLevel;
	runtime->bnr_param_in.NoiseHiLevel = runtime->bnr_attr.NoiseHiLevel;
	runtime->afAEEVRatio = algoResult->afAEEVRatio[ISP_CHANNEL_LE];

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 teaisp_bnr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_teaisp_bnr_main(
		(struct teaisp_bnr_param_in *)&runtime->bnr_param_in,
		(struct teaisp_bnr_param_out *)&runtime->bnr_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 teaisp_bnr_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S * pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstIspCtx->enModelType = TEAISP_MODEL_NONE;
	teaisp_bnr_get_model_type(ViPipe, &pstIspCtx->enModelType);

	struct cvi_vip_isp_fe_cfg *pre_fe_addr = get_pre_fe_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_blc_config *pre_fe_blc_cfg[2] = {
		(struct cvi_vip_isp_blc_config *)&(pre_fe_addr->tun_cfg[tun_idx].blc_cfg[0]),
		(struct cvi_vip_isp_blc_config *)&(pre_fe_addr->tun_cfg[tun_idx].blc_cfg[1])
	};

	struct teaisp_bnr_config *bnr_cfg = &(runtime->bnr_cfg);

	CVI_BOOL is_postprocess_update = (runtime->postprocess_updated == CVI_TRUE);

	if (is_postprocess_update == CVI_FALSE) {
		bnr_cfg->update = 0;
	} else {
		bnr_cfg->update = 1;

		volatile CVI_U32 *temp = NULL;
		CVI_FLOAT temp_f = 0;

		temp = (volatile CVI_U32 *) &temp_f;

		temp_f = (CVI_FLOAT) (pre_fe_blc_cfg[0]->groffset + pre_fe_blc_cfg[0]->gboffset) / 2.0;
		bnr_cfg->blc = *temp;

		temp_f = runtime->bnr_param_out.slope;
		bnr_cfg->coeff_a = *temp;

		if (pstIspCtx->enModelType == TEAISP_MODEL_MOTION) {
			temp_f = runtime->afAEEVRatio;
		} else {
			temp_f = runtime->bnr_param_out.intercept;
		}
		bnr_cfg->coeff_b = *temp;

		temp_f = (CVI_FLOAT) (255 - runtime->bnr_attr.FilterMotionStr2D) / 255.0;
		bnr_cfg->filter_motion_str_2d = *temp;

		temp_f = (CVI_FLOAT) (255 - runtime->bnr_attr.FilterStaticStr2D) / 255.0;
		bnr_cfg->filter_static_str_2d = *temp;

		temp_f = (CVI_FLOAT) (255 - runtime->bnr_attr.FilterStr3D) / 255.0;
		bnr_cfg->filter_str_3d = *temp;

		temp_f = (CVI_FLOAT) (255 - runtime->bnr_attr.FilterStr2D) / 255.0;
		bnr_cfg->filter_str_2d = *temp;

		struct lblc_info lblcInfo;

		isp_lblc_ctrl_get_lblc_info(ViPipe, &lblcInfo);
		memcpy(bnr_cfg->lblcOffsetR, lblcInfo.lblcOffsetR, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memcpy(bnr_cfg->lblcOffsetGr, lblcInfo.lblcOffsetGr, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memcpy(bnr_cfg->lblcOffsetGb, lblcInfo.lblcOffsetGb, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memcpy(bnr_cfg->lblcOffsetB, lblcInfo.lblcOffsetB, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
	}

	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	if (handle != NULL) {
		sem_post(&handle->sem);
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_bnr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

static struct teaisp_bnr_ctrl_runtime  *_get_bnr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct teaisp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_BNR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 teaisp_bnr_ctrl_check_bnr_attr_valid(const TEAISP_BNR_ATTR_S *pstBNRAttr)
{
	if (pstBNRAttr->stManual.FilterMotionStr2D < pstBNRAttr->stManual.FilterStaticStr2D) {
		ISP_LOG_ERR("FilterMoitonStr2D must >= FilterStaticStr2D\n");
		return CVI_FAILURE;
	}

	for (int i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		if (pstBNRAttr->stAuto.FilterMotionStr2D[i] < pstBNRAttr->stAuto.FilterStaticStr2D[i]) {
			ISP_LOG_ERR("FilterMotionStr2D must >= FilterStaticStr2D\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
// call after cvi_isp_init, before cvi_isp_run
CVI_S32 teaisp_bnr_ctrl_set_model(VI_PIPE ViPipe, const TEAISP_BNR_MODEL_INFO_S *pstModelInfo)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_LOG_INFO("path: %s, enterISO: %d, tolerance: %d\n",
		pstModelInfo->path, pstModelInfo->enterISO, pstModelInfo->tolerance);

	if (access(pstModelInfo->path, F_OK) != 0) {
		ISP_LOG_ERR("model file not exist\n");
		return CVI_FAILURE;
	}

	TEAISP_BNR_HANDLE_S *handle = (TEAISP_BNR_HANDLE_S *)(uintptr_t) runtime->handle;

	if (handle == CVI_NULL) {

		handle = (TEAISP_BNR_HANDLE_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_HANDLE_S));

		if (handle == CVI_NULL) {
			return CVI_FAILURE;
		}

		INIT_LIST_HEAD(&handle->model_info_head);
		handle->pcurr_model = CVI_NULL;

		handle->bnrThreadEnable = CVI_TRUE;
		sem_init(&handle->sem, 0, 0);
		pthread_create(&handle->tid, NULL, bnr_work_thread, (void *)(uintptr_t)ViPipe);
	}

	runtime->handle = (ISP_VOID_PTR)(uintptr_t) handle;

	// please keep add in order
	if (!list_empty(&handle->model_info_head)) {

		TEAISP_BNR_MODEL_S *prev = list_entry(handle->model_info_head.prev, TEAISP_BNR_MODEL_S, list);

		if (prev->model_info.enterISO >= pstModelInfo->enterISO) {
			ISP_LOG_ASSERT("prev enterISO: %d, curr enterISO: %d, please keep add in order\n",
				prev->model_info.enterISO, pstModelInfo->enterISO);
		}
	}

	TEAISP_BNR_MODEL_S *info = (TEAISP_BNR_MODEL_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_MODEL_S));

	if (info == NULL) {
		return CVI_FAILURE;
	}

	info->model_info = *pstModelInfo;

#ifdef ENABLE_PRELOAD_BNR_MODEL
	ret = teaisp_bnr_load_model(ViPipe, info->model_info.path, &info->model);
	if (ret != CVI_SUCCESS) {
		return CVI_FAILURE;
	}
#endif

	list_add_tail(&info->list, &handle->model_info_head);

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_get_bnr_attr(VI_PIPE ViPipe, const TEAISP_BNR_ATTR_S **pstBNRAttr)
{
	if (pstBNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_BNR, (CVI_VOID *) &shared_buffer);

	*pstBNRAttr = &shared_buffer->stBNRAttr;

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_set_bnr_attr(VI_PIPE ViPipe, const TEAISP_BNR_ATTR_S *pstBNRAttr)
{
	if (pstBNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = teaisp_bnr_ctrl_check_bnr_attr_valid(pstBNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const TEAISP_BNR_ATTR_S *p = CVI_NULL;

	teaisp_bnr_ctrl_get_bnr_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstBNRAttr, sizeof(*pstBNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_get_np_attr(VI_PIPE ViPipe, const TEAISP_BNR_NP_S **np)
{
	if (np == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct teaisp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_BNR, (CVI_VOID *) &shared_buffer);

	*np = (TEAISP_BNR_NP_S *) &shared_buffer->stNPAttr;

	return ret;
}

CVI_S32 teaisp_bnr_ctrl_set_np_attr(VI_PIPE ViPipe, const TEAISP_BNR_NP_S *np)
{
	if (np == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const TEAISP_BNR_NP_S *p = CVI_NULL;

	teaisp_bnr_ctrl_get_np_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, np, sizeof(*np));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}
