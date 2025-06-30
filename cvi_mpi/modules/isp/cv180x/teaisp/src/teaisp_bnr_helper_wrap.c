/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_helper.c
 * Description:
 *
 */

#include <unistd.h>
#include "cvi_sys.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "teaisp_helper.h"
#include "isp_ioctl.h"

#include "teaisp_bnr_ctrl.h"
#include "teaisp.h"

#include "cviruntime.h"

#include "teaisp_bnr_helper_wrap.h"

#define BNR_IN_INPUT_IMG      (0)
#define BNR_IN_FUSION_IMG     (1)
#define BNR_IN_SIGMA          (2)
#define BNR_IN_COEFF_A        (3)
#define BNR_IN_COEFF_B        (4)
#define BNR_IN_BLEND          (5)
#define BNR_IN_MOTION_STR_2D  (6)
#define BNR_IN_STATIC_STR_2D  (7)
#define BNR_IN_BLACK_LEVEL    (8)
#define BNR_IN_STR_3D         (9)
#define BNR_IN_STR_2D         (10)
#define BNR_IN_NUM            (11)

#define BNR_OUT_INPUT_IMG     (0)
#define BNR_OUT_FUSION_IMG    (1)
#define BNR_OUT_SIGMA         (2)
#define BNR_OUT_NUM           (3)

typedef struct {
	int pipe;
	uint32_t core_id;
	uint32_t tuning_index;
	CVI_MODEL_HANDLE cm_handle;
	const char **net_names;
	void **input_buff;
	CVI_TENSOR *input_tensors;
	CVI_TENSOR *output_tensors;
	int input_num;
	int output_num;
} TEAISP_MODEL_S;

typedef struct {
	uint8_t enable_lauch_thread;
	pthread_t launch_thread_id;

	TEAISP_MODEL_S *cmodel0;
	TEAISP_MODEL_S *cmodel1;
	TEAISP_MODEL_TYPE_E enModelType;
} TEAISP_BNR_CTX_S;

static TEAISP_BNR_CTX_S *bnr_ctx[VI_MAX_PIPE_NUM];

static int teaisp_bnr_get_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw, uint64_t *rgbmap)
{
	uint64_t tmp[3] = {0, 0, 0};

	G_EXT_CTRLS_PTR(VI_IOCTL_GET_AI_ISP_RAW, &tmp);

	*input_raw = tmp[0];
	*output_raw = tmp[1];
	*rgbmap = tmp[2];

	return 0;
}

static int teaisp_bnr_put_raw(VI_PIPE ViPipe, uint64_t *input_raw, uint64_t *output_raw, uint64_t *rgbmap)
{
	uint64_t tmp[3] = {0, 0, 0};

	tmp[0] = *input_raw;
	tmp[1] = *output_raw;
	tmp[2] = *rgbmap;

	S_EXT_CTRLS_PTR(VI_IOCTL_PUT_AI_ISP_RAW, &tmp);

	return 0;
}

static void teaisp_bnr_dump_input(TEAISP_MODEL_S *model, uint64_t input_raw_addr, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/input0%d_img.bin", pipe);
	size = model->input_tensors[0].mem_size;
	addr_tmp = CVI_SYS_MmapCache(input_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/input1%d_fusion.bin", pipe);
	size = model->input_tensors[BNR_IN_FUSION_IMG].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_FUSION_IMG]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input2%d_sigma.bin", pipe);
	size = model->input_tensors[BNR_IN_SIGMA].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_SIGMA]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input3%d_coeff_a.bin", pipe);
	size = model->input_tensors[BNR_IN_COEFF_A].mem_size;
	addr_tmp = model->input_buff[BNR_IN_COEFF_A];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input4%d_coeff_b.bin", pipe);
	size = model->input_tensors[BNR_IN_COEFF_B].mem_size;
	addr_tmp = model->input_buff[BNR_IN_COEFF_B];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input5%d_blend.bin", pipe);
	size = model->input_tensors[BNR_IN_BLEND].mem_size;
	addr_tmp = model->input_buff[BNR_IN_BLEND];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input6%d_motion_str_2d.bin", pipe);
	size = model->input_tensors[BNR_IN_MOTION_STR_2D].mem_size;
	addr_tmp = model->input_buff[BNR_IN_MOTION_STR_2D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input7%d_static_str_2d.bin", pipe);
	size = model->input_tensors[BNR_IN_STATIC_STR_2D].mem_size;
	addr_tmp = model->input_buff[BNR_IN_STATIC_STR_2D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input8%d_blc.bin", pipe);
	size = model->input_tensors[BNR_IN_BLACK_LEVEL].mem_size;
	addr_tmp = model->input_buff[BNR_IN_BLACK_LEVEL];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input9%d_str_3d.bin", pipe);
	size = model->input_tensors[BNR_IN_STR_3D].mem_size;
	addr_tmp = model->input_buff[BNR_IN_STR_3D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/input10%d_str_2d.bin", pipe);
	size = model->input_tensors[BNR_IN_STR_2D].mem_size;
	addr_tmp = model->input_buff[BNR_IN_STR_2D];
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
}

static void teaisp_bnr_dump_output(TEAISP_MODEL_S *model, uint64_t output_raw_addr, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/output0%d_img.bin", pipe);
	size = model->output_tensors[0].mem_size;
	addr_tmp = CVI_SYS_MmapCache(output_raw_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/output1%d_fusion.bin", pipe);
	size = model->output_tensors[BNR_OUT_FUSION_IMG].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_FUSION_IMG]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);

	snprintf(path, sizeof(path), "/tmp/output2%d_sigma.bin", pipe);
	size = model->output_tensors[BNR_OUT_SIGMA].mem_size;
	addr_tmp = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_SIGMA]);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
}

static void *teaisp_bnr_launch_thread(void *param)
{
	int pipe = (int) (uintptr_t) param;
	uint64_t input_raw_addr = 0;
	uint64_t output_raw_addr = 0;
	uint64_t rgbmap_addr = 0;
	uint64_t output_addr = 0;

	//ISP_LOG_INFO("run bnr launch thread, %d\n", pipe);
	printf("run bnr launch thread, %d\n", pipe);

reset_launch:

	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (bnr_ctx[pipe]->cmodel0 == NULL) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	TEAISP_MODEL_S *model = bnr_ctx[pipe]->cmodel0;

	bnr_ctx[pipe]->cmodel1 = bnr_ctx[pipe]->cmodel0;

	// wait param update
	while (bnr_ctx[pipe]->enable_lauch_thread) {
		if (model->tuning_index <= 0) {
			usleep(5 * 1000);
		} else {
			break;
		}
	}

	void *input_fusion_ptr = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_FUSION_IMG]);
	void *input_sigma_ptr = CVI_NN_TensorPtr(&model->input_tensors[BNR_IN_SIGMA]);
	void *output_fusion_ptr = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_FUSION_IMG]);
	void *output_sigma_ptr = CVI_NN_TensorPtr(&model->output_tensors[BNR_OUT_SIGMA]);

	// launch
	while (bnr_ctx[pipe]->enable_lauch_thread) {

		if (bnr_ctx[pipe]->cmodel0 != bnr_ctx[pipe]->cmodel1) {
			//ISP_LOG_INFO("reset bnr launch....\n");
			printf("reset bnr launch....\n");
			goto reset_launch;
		}

		teaisp_bnr_get_raw(pipe, &input_raw_addr, &output_raw_addr, &rgbmap_addr);

		output_addr = output_raw_addr;
		if (bnr_ctx[pipe]->enModelType == TEAISP_MODEL_MOTION) {
			output_addr = rgbmap_addr;
		}

#ifndef ENABLE_BYPASS_TPU
		CVI_NN_SetTensorPhysicalAddr(&model->input_tensors[0], input_raw_addr);
		CVI_NN_SetTensorPhysicalAddr(&model->output_tensors[0], output_addr);

		//get model output to input when model forward
		//CVI_NN_SetTensorPtr(&model->output_tensors[BNR_OUT_FUSION_IMG], input_fusion_ptr);
		//CVI_NN_SetTensorPtr(&model->output_tensors[BNR_OUT_SIGMA], input_sigma_ptr);

		for (int i = 0; i < model->input_num; i++) {
			if (i != BNR_IN_INPUT_IMG && i != BNR_IN_FUSION_IMG && i != BNR_IN_SIGMA) {
				memcpy(CVI_NN_TensorPtr(&model->input_tensors[i]),
								model->input_buff[i],
								model->input_tensors[i].mem_size);
			}
		}

		if (access("/tmp/teaisp_bnr_dump", F_OK) == 0) {
			teaisp_bnr_dump_input(model, input_raw_addr, pipe);
			system("rm /tmp/teaisp_bnr_dump;touch /tmp/teaisp_bnr_dump_output");
		}

		//struct timeval tv1, tv2;
		//gettimeofday(&tv1, NULL);

		CVI_RC ret = CVI_NN_Forward(model->cm_handle, model->input_tensors, model->input_num,
									model->output_tensors, model->output_num);

		if (ret != CVI_RC_SUCCESS) {
			ISP_LOG_ERR("model forward failed.\n");
		}

		if (access("/tmp/teaisp_bnr_dump_output", F_OK) == 0) {
			teaisp_bnr_dump_output(model, output_addr, pipe);
			system("rm /tmp/teaisp_bnr_dump_output");
		}

		//gettimeofday(&tv2, NULL);
		//ISP_LOG_ERR("launch time diff, %d, %d\n",
		//	pipe, ((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)));

#else
		void *src_addr;
		void *dst_addr;

		int size = model->input_tensors[0].mem_size;

		src_addr = CVI_SYS_MmapCache(input_raw_addr, size);
		dst_addr = CVI_SYS_MmapCache(output_raw_addr, size);

		memcpy(dst_addr, src_addr, size);

		usleep(45 * 1000);

		CVI_SYS_IonFlushCache(output_raw_addr, dst_addr, size);

		CVI_SYS_Munmap(src_addr, size);
		CVI_SYS_Munmap(dst_addr, size);
#endif

		if (bnr_ctx[pipe]->enModelType == TEAISP_MODEL_MOTION)
			teaisp_bnr_put_raw(pipe, &output_raw_addr, &input_raw_addr, &rgbmap_addr);
		else
			teaisp_bnr_put_raw(pipe, &input_raw_addr, &output_raw_addr, &rgbmap_addr);

		memcpy(input_fusion_ptr, output_fusion_ptr, model->input_tensors[BNR_IN_FUSION_IMG].mem_size);
		memcpy(input_sigma_ptr, output_sigma_ptr, model->input_tensors[BNR_IN_SIGMA].mem_size);

	}

	//ISP_LOG_INFO("bnr launch thread end, %d\n", pipe);
	printf("bnr launch thread end, %d\n", pipe);

	return NULL;
}

static int start_launch_thread(VI_PIPE ViPipe)
{
	int pipe = ViPipe;

	// TODO: set priority
	bnr_ctx[pipe]->enable_lauch_thread = 1;
	pthread_create(&bnr_ctx[pipe]->launch_thread_id, NULL,
		teaisp_bnr_launch_thread, (void *) (uintptr_t) pipe);

	return 0;
}

static int stop_launch_thread(VI_PIPE ViPipe)
{
	int pipe = ViPipe;

	bnr_ctx[pipe]->enable_lauch_thread = 0;
	pthread_join(bnr_ctx[pipe]->launch_thread_id, NULL);

	return 0;
}

CVI_S32 teaisp_bnr_load_model_wrap(VI_PIPE ViPipe, const char *path, void **model)
{
	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) ISP_CALLOC(1, sizeof(TEAISP_MODEL_S));

	if (m == NULL) {
		return CVI_FAILURE;
	}

	struct timeval tv1, tv2;

	gettimeofday(&tv1, NULL);

	memset(m, 0, sizeof(TEAISP_MODEL_S));

	m->pipe = ViPipe;

	int maxDev = 0;

	CVI_TEAISP_GetMaxDev(ViPipe, &maxDev);

	if (maxDev > 1) {
		m->core_id = ViPipe % maxDev; // TODO:mason
	} else {
		m->core_id = 0;
	}

	ISP_LOG_INFO("load bmodel++: pipe:%d, core_id: %d, %s\n", ViPipe, m->core_id, path);

	if (bnr_ctx[ViPipe] == NULL) {
		bnr_ctx[ViPipe] = (TEAISP_BNR_CTX_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_CTX_S));

		if (bnr_ctx[ViPipe] == NULL) {
			return CVI_FAILURE;
		}
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;
	}
	bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_BNR;

	CVI_RC ret = CVI_NN_RegisterModel(path, &m->cm_handle);

	if (ret != CVI_RC_SUCCESS) {
		ISP_LOG_ERR("Failed to register and load model %s\n", path);
		goto load_model_fail;
	}

	//CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, m->core_id);
	CVI_NN_SetConfig(model, OPTION_OUTPUT_ALL_TENSORS, true);

	ret = CVI_NN_GetInputOutputTensors(m->cm_handle, &m->input_tensors, &m->input_num,
												&m->output_tensors, &m->output_num);
	if (ret != CVI_RC_SUCCESS) {
		ISP_LOG_ERR("Failed to get inputs & outputs from model\n");
		goto load_model_fail;
	}

	ISP_LOG_INFO("model path: %s, in: %d, on: %d\n", path, m->input_num, m->output_num);

	if (m->input_num != BNR_IN_NUM || m->output_num != BNR_OUT_NUM) {
		ISP_LOG_ASSERT("model param num not match, in: %d, %d, out: %d, %d\n",
			m->input_num, BNR_IN_NUM, m->output_num, BNR_OUT_NUM);
		goto load_model_fail;
	}

	m->input_buff = (void *) ISP_CALLOC(m->input_num, sizeof(void *));
	if (m->input_buff == NULL) {
		goto load_model_fail;
	}

	for (int i = 0; i < m->input_num; i++) {
		memset((void *)CVI_NN_TensorPtr(&m->input_tensors[i]), 0, m->input_tensors[i].mem_size);

		ISP_LOG_ERR("in: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n", i,
			m->input_tensors[i].fmt,
			m->input_tensors[i].shape.dim[0], m->input_tensors[i].shape.dim[1],
			m->input_tensors[i].shape.dim[2], m->input_tensors[i].shape.dim[3],
			m->input_tensors[i].shape.dim_size,
			m->input_tensors[i].paddr,
			m->input_tensors[i].mem_size);

		if (i == BNR_IN_INPUT_IMG || i == BNR_IN_FUSION_IMG || i == BNR_IN_SIGMA) {
			continue;
		} else {
			m->input_buff[i] = (void *) ISP_CALLOC(1, m->input_tensors[i].mem_size);
		}
	}

	for (int i = 0; i < m->output_num; i++) {
		memset((void *)CVI_NN_TensorPtr(&m->output_tensors[i]), 0, m->output_tensors[i].mem_size);

		ISP_LOG_ERR("out: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n", i,
			m->output_tensors[i].fmt,
			m->output_tensors[i].shape.dim[0], m->output_tensors[i].shape.dim[1],
			m->output_tensors[i].shape.dim[2], m->output_tensors[i].shape.dim[3],
			m->output_tensors[i].shape.dim_size,
			m->output_tensors[i].paddr,
			m->output_tensors[i].mem_size);
	}

	*model = m;

	gettimeofday(&tv2, NULL);

	//ISP_LOG_INFO("load bnr model success, %p\n", m);
	printf("load cvimodel success: pipe:%d, core_id: %d, cost time: %ld, %s\n", ViPipe, m->core_id,
		((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)), path);

	int input_size = m->input_tensors[0].mem_size;
	int output_size = m->output_tensors[0].mem_size;

	if (input_size != output_size) {
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_MOTION;
	}

	return CVI_SUCCESS;

load_model_fail:

	ISP_LOG_ERR("load bnr model error...\n");

	teaisp_bnr_unload_model_wrap(ViPipe, (void *) m);

	return CVI_FAILURE;
}

CVI_S32 teaisp_bnr_unload_model_wrap(VI_PIPE ViPipe, void *model)
{
	UNUSED(ViPipe);

	if (model == NULL) {
		return CVI_SUCCESS;
	}

	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) model;

	ISP_LOG_INFO("unload model++, %p\n", m);
	ISP_LOG_INFO("unload model wait..., %p, %p, %d\n", m,
		bnr_ctx[ViPipe]->cmodel1,
		bnr_ctx[ViPipe]->enable_lauch_thread);

	while (m == bnr_ctx[ViPipe]->cmodel1 &&
		 bnr_ctx[ViPipe]->cmodel1 != NULL &&
		 bnr_ctx[ViPipe]->enable_lauch_thread) {
		ISP_LOG_INFO("unload model wait..., %p, %p, %d\n", m,
			bnr_ctx[ViPipe]->cmodel1,
			bnr_ctx[ViPipe]->enable_lauch_thread);
		usleep(5 * 1000);
	}

	if (m->net_names) {
		free(m->net_names);
		m->net_names = NULL;
	}

	if (m->cm_handle) {
		CVI_NN_CleanupModel(m->cm_handle);
	}

	for (int i = 0; i < m->input_num; i++) {
		if (m->input_buff[i]) {
			free(m->input_buff[i]);
		}
	}

	ISP_RELEASE_MEMORY(m);

	bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_init_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver init, %d\n", ViPipe);

	uint32_t swap_buf_index[2] = {0x0};
	ai_isp_bnr_cfg_t bnr_cfg;

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_INIT;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	memset(&bnr_cfg, 0, sizeof(ai_isp_bnr_cfg_t));

	swap_buf_index[0] = ((BNR_OUT_FUSION_IMG << 16) | BNR_IN_FUSION_IMG);
	swap_buf_index[1] = ((BNR_OUT_SIGMA << 16) | BNR_IN_SIGMA);

	bnr_cfg.swap_buf_index = (uint64_t)(uintptr_t) swap_buf_index;
	bnr_cfg.swap_buf_count = 2;
	bnr_cfg.ai_rgbmap = false;

	//if you want this work, you must set ENABLE_PRELOAD_BNR_MODEL to 1 in teaisp_bnr_ctrl.c
	if (bnr_ctx[ViPipe] && bnr_ctx[ViPipe]->enModelType == TEAISP_MODEL_MOTION) {
		bnr_cfg.ai_rgbmap = true;
	}

	ISP_LOG_ERR("set ai_rgbmap: %d\n", bnr_cfg.ai_rgbmap);

	cfg.param_addr = (uint64_t)(uintptr_t) &bnr_cfg;
	cfg.param_size = sizeof(ai_isp_bnr_cfg_t);

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	if (bnr_ctx[ViPipe] == NULL) {
		bnr_ctx[ViPipe] = (TEAISP_BNR_CTX_S *) ISP_CALLOC(1, sizeof(TEAISP_BNR_CTX_S));

		if (bnr_ctx[ViPipe] == NULL) {
			return CVI_FAILURE;
		}
		bnr_ctx[ViPipe]->enModelType = TEAISP_MODEL_NONE;
	}

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_deinit_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver deinit, %d\n", ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_DEINIT;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_start_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver start, %d\n", ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_ENABLE;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	start_launch_thread(ViPipe);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_driver_stop_wrap(VI_PIPE ViPipe)
{
	ISP_LOG_INFO("set driver stop, %d\n", ViPipe);

	stop_launch_thread(ViPipe);

	ai_isp_cfg_t cfg;

	memset(&cfg, 0, sizeof(ai_isp_cfg_t));

	cfg.vi_pipe = ViPipe;
	cfg.ai_isp_cfg_type = AI_ISP_CFG_DISABLE;
	cfg.ai_isp_type = AI_ISP_TYPE_BNR;

	S_EXT_CTRLS_PTR(VI_IOCTL_AI_ISP_CFG, &cfg);

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_set_api_info_wrap(VI_PIPE ViPipe, void *model, void *param, int is_new)
{
	TEAISP_MODEL_S *m = (TEAISP_MODEL_S *) model;

	ISP_LOG_INFO("bnr set api info, %d, %d, %p\n", ViPipe, m->core_id, model);

	struct teaisp_bnr_config *bnr_cfg = (struct teaisp_bnr_config *) param;

	m->tuning_index++;

	memcpy(m->input_buff[BNR_IN_COEFF_A], &bnr_cfg->coeff_a,
		m->input_tensors[BNR_IN_COEFF_A].mem_size);

	memcpy(m->input_buff[BNR_IN_COEFF_B], &bnr_cfg->coeff_b,
		m->input_tensors[BNR_IN_COEFF_B].mem_size);

	memcpy(m->input_buff[BNR_IN_BLEND], &bnr_cfg->blend,
		m->input_tensors[BNR_IN_BLEND].mem_size);

	memcpy(m->input_buff[BNR_IN_MOTION_STR_2D], &bnr_cfg->filter_motion_str_2d,
		m->input_tensors[BNR_IN_MOTION_STR_2D].mem_size);

	memcpy(m->input_buff[BNR_IN_STATIC_STR_2D], &bnr_cfg->filter_static_str_2d,
		m->input_tensors[BNR_IN_STATIC_STR_2D].mem_size);

	memcpy(m->input_buff[BNR_IN_STR_3D], &bnr_cfg->filter_str_3d,
		m->input_tensors[BNR_IN_STR_3D].mem_size);

	memcpy(m->input_buff[BNR_IN_STR_2D], &bnr_cfg->filter_str_2d,
		m->input_tensors[BNR_IN_STR_2D].mem_size);

	if ((int) m->input_tensors[BNR_IN_BLACK_LEVEL].mem_size == (int) sizeof(float)) {
		memcpy(m->input_buff[BNR_IN_BLACK_LEVEL], &bnr_cfg->blc,
			m->input_tensors[BNR_IN_BLACK_LEVEL].mem_size);
	} else {
		int lblc_cnt = 0;
		float *lblc = (float *) m->input_buff[BNR_IN_BLACK_LEVEL];

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetR[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetGr[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetB[i];
			lblc_cnt++;
		}

		for (int i = 0; i < ISP_LBLC_GRID_POINTS; i++) {
			*lblc++ = (float) bnr_cfg->lblcOffsetGb[i];
			lblc_cnt++;
		}

		if ((int) (lblc_cnt * sizeof(float)) !=
			(int) m->input_tensors[BNR_IN_BLACK_LEVEL].mem_size) {
			ISP_LOG_ERR("fill lblc cnt error, %d, %d\n",
				lblc_cnt, m->input_tensors[BNR_IN_BLACK_LEVEL].mem_size);
		}
	}

	ISP_LOG_INFO("int bnr param: %d, %d, %d, %d, %d, %d, %d, %d\n",
		*((uint32_t *) m->input_buff[BNR_IN_COEFF_A]),
		*((uint32_t *) m->input_buff[BNR_IN_COEFF_B]),
		*((uint32_t *) m->input_buff[BNR_IN_BLEND]),
		*((uint32_t *) m->input_buff[BNR_IN_MOTION_STR_2D]),
		*((uint32_t *) m->input_buff[BNR_IN_STATIC_STR_2D]),
		*((uint32_t *) m->input_buff[BNR_IN_BLACK_LEVEL]),
		*((uint32_t *) m->input_buff[BNR_IN_STR_3D]),
		*((uint32_t *) m->input_buff[BNR_IN_STR_2D]));

	ISP_LOG_ERR("float bnr param: %.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f, %.5f\n",
		*((float *) m->input_buff[BNR_IN_COEFF_A]),
		*((float *) m->input_buff[BNR_IN_COEFF_B]),
		*((float *) m->input_buff[BNR_IN_BLEND]),
		*((float *) m->input_buff[BNR_IN_MOTION_STR_2D]),
		*((float *) m->input_buff[BNR_IN_STATIC_STR_2D]),
		*((float *) m->input_buff[BNR_IN_BLACK_LEVEL]),
		*((float *) m->input_buff[BNR_IN_STR_3D]),
		*((float *) m->input_buff[BNR_IN_STR_2D]));

	UNUSED(is_new);

	bnr_ctx[ViPipe]->cmodel0 = m;

	return CVI_SUCCESS;
}

CVI_S32 teaisp_bnr_get_model_type_wrap(VI_PIPE ViPipe, void *model_type)
{
	if (model_type == NULL) {
		return CVI_SUCCESS;
	}

	TEAISP_MODEL_TYPE_E *m_type = (TEAISP_MODEL_TYPE_E *) model_type;

	*m_type = TEAISP_MODEL_NONE;

	if (bnr_ctx[ViPipe])
		*m_type = bnr_ctx[ViPipe]->enModelType;

	return CVI_SUCCESS;
}
