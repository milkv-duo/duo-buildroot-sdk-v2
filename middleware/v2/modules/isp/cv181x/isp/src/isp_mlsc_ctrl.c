/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mlsc_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"
#include "isp_mw_compat.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_mlsc_ctrl.h"
#include "isp_mgr_buf.h"

#ifdef __riscv_vector
#include <riscv_vector.h>
#endif
typedef enum _LSC_COLOR_CHANNEL_E {
	LSC_COLOR_CHANNEL_R = 0x0,
	LSC_COLOR_CHANNEL_G = 0x1,
	LSC_COLOR_CHANNEL_B = 0x2,
	LSC_COLOR_CHANNEL_MAX,
} LSC_COLOR_CHANNEL_E;

//#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl mlsc_mod = {
	.init = isp_mlsc_ctrl_init,
	.uninit = isp_mlsc_ctrl_uninit,
	.suspend = isp_mlsc_ctrl_suspend,
	.resume = isp_mlsc_ctrl_resume,
	.ctrl = isp_mlsc_ctrl_ctrl
};

static CVI_S32 isp_mlsc_ctrl_buf_init(VI_PIPE ViPipe);
static CVI_S32 isp_mlsc_ctrl_buf_uninit(VI_PIPE ViPipe);
static CVI_S32 isp_mlsc_ctrl_pre_fe_eof(VI_PIPE ViPipe);
static CVI_S32 isp_mlsc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_mlsc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_mlsc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_mlsc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_mlsc_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_mlsc_ctrl_check_mlsc_attr_valid(const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr);
static CVI_S32 isp_mlsc_ctrl_check_mlsc_lut_attr_valid(
	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr);
static CVI_S32 isp_mlsc_ctrl_packing_tbl(CVI_U8 *lsctable, CVI_U16 *r, CVI_U16 *g, CVI_U16 *b);

static struct isp_mlsc_ctrl_runtime  *_get_mlsc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_mlsc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (!runtime->is_init) {
		for (CVI_U32 i = 0 ; i < CVI_ISP_LSC_GRID_POINTS; i++) {
			runtime->unit_gain_table[i] = (1 << LSC_GAIN_BASE);
		}

		runtime->mapped = CVI_FALSE;

		runtime->preprocess_updated = CVI_TRUE;
		runtime->process_updated = CVI_FALSE;
		runtime->postprocess_updated = CVI_FALSE;
		runtime->is_module_bypass = CVI_FALSE;
	}

#ifndef ARCH_RTOS_CV181X
#ifdef ENABLE_ISP_C906L
	runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
	runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
#else
	runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_MASTER;
#endif
#else
	if (!runtime->is_init) {
		runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_SLAVE;
		runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
		runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
	}
#endif

	runtime->is_init = CVI_TRUE;

	return ret;
}

CVI_S32 isp_mlsc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_mlsc_ctrl_buf_uninit(ViPipe);

	runtime->is_init = CVI_FALSE;

	return ret;
}

CVI_S32 isp_mlsc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_mlsc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_mlsc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_PREFE_EOF:
#ifndef ARCH_RTOS_CV181X
		if (runtime->run_ctrl_mark.pre_sof_run_ctrl == CTRL_RUN_IN_SLAVE) {
			break;
		}
#else
		if (runtime->run_ctrl_mark.pre_sof_run_ctrl == CTRL_RUN_IN_MASTER) {
			break;
		}
#endif
		isp_mlsc_ctrl_pre_fe_eof(ViPipe);
		break;
	case MOD_CMD_POST_EOF:
#ifndef ARCH_RTOS_CV181X
		if (runtime->run_ctrl_mark.post_eof_run_ctrl == CTRL_RUN_IN_SLAVE) {
			break;
		}
#else
		if (runtime->run_ctrl_mark.post_eof_run_ctrl == CTRL_RUN_IN_MASTER) {
			break;
		}
#endif
		isp_mlsc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassMlsc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassMlsc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

CVI_S32 isp_mlsc_ctrl_set_run_ctrl_mark(VI_PIPE ViPipe, const struct isp_module_run_ctrl_mark *mark)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->run_ctrl_mark = *mark;

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_mlsc_ctrl_pre_fe_eof(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	isp_mlsc_ctrl_buf_init(ViPipe);

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_mlsc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_mlsc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_mlsc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_mlsc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_MESH_SHADING_ATTR_S *mlsc_attr = NULL;

	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *mlsc_lut_attr = NULL;

	isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &mlsc_attr);
	isp_mlsc_ctrl_get_mlsc_lut_attr(ViPipe, &mlsc_lut_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(mlsc_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));
	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	 //No need to update parameters if disable. Because its meaningless
	if (!mlsc_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (mlsc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(mlsc_attr, MeshStr);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(mlsc_attr, MeshStr, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	runtime->mlsc_param_in.color_temperature = algoResult->u32ColorTemp;
	runtime->mlsc_param_in.color_temp_size = mlsc_lut_attr->Size;

	for (CVI_U32 i = 0 ; i < ISP_MLSC_COLOR_TEMPERATURE_SIZE ; i++) {
		runtime->mlsc_param_in.color_temp_tbl[i] = mlsc_lut_attr->LscGainLut[i].ColorTemperature;
		runtime->mlsc_param_in.mlsc_lut_r[i] = ISP_PTR_CAST_PTR(mlsc_lut_attr->LscGainLut[i].RGain);
		runtime->mlsc_param_in.mlsc_lut_g[i] = ISP_PTR_CAST_PTR(mlsc_lut_attr->LscGainLut[i].GGain);
		runtime->mlsc_param_in.mlsc_lut_b[i] = ISP_PTR_CAST_PTR(mlsc_lut_attr->LscGainLut[i].BGain);
	}
	runtime->mlsc_param_in.grid_col = CVI_ISP_LSC_GRID_COL;
	runtime->mlsc_param_in.grid_row = CVI_ISP_LSC_GRID_ROW;
	runtime->mlsc_param_in.strength = runtime->mlsc_attr.MeshStr;
	// ParamOut
	runtime->mlsc_param_out.mlsc_lut_r = ISP_PTR_CAST_PTR(runtime->mlsc_lut_attr.RGain);
	runtime->mlsc_param_out.mlsc_lut_g = ISP_PTR_CAST_PTR(runtime->mlsc_lut_attr.GGain);
	runtime->mlsc_param_out.mlsc_lut_b = ISP_PTR_CAST_PTR(runtime->mlsc_lut_attr.BGain);
	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

#ifndef ENABLE_ISP_C906L
	ret = isp_algo_mlsc_main(
		(struct mlsc_param_in *)&runtime->mlsc_param_in,
		(struct mlsc_param_out *)&runtime->mlsc_param_out
	);
#endif
	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_lsc_config *mlsc_cfg =
		(struct cvi_vip_isp_lsc_config *)&(post_addr->tun_cfg[tun_idx].lsc_cfg);

	const ISP_MESH_SHADING_ATTR_S *mlsc_attr = NULL;

	isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &mlsc_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		mlsc_cfg->update = 0;
	} else {
		mlsc_cfg->update = 1;
		mlsc_cfg->enable = mlsc_attr->Enable && !runtime->is_module_bypass;
		if (mlsc_attr->Enable && !runtime->is_module_bypass) {
			mlsc_cfg->debug = 0;
			mlsc_cfg->gain_base = 0;
			mlsc_cfg->strength = 4095;
			mlsc_cfg->gain_3p9_0_4p8_1 = 0;
			mlsc_cfg->renormalize_enable = mlsc_attr->OverflowProtection;
			mlsc_cfg->gain_bicubic_0_bilinear_1 = 1;
			mlsc_cfg->boundary_interpolation_mode = 1;
			mlsc_cfg->boundary_interpolation_lf_range = 1;
			mlsc_cfg->boundary_interpolation_up_range = 1;
			mlsc_cfg->boundary_interpolation_rt_range = 35;
			mlsc_cfg->boundary_interpolation_dn_range = 35;
			mlsc_cfg->bldratio_enable = 0;
			mlsc_cfg->bldratio = 0;
			mlsc_cfg->intp_gain_max = 67108863;
			mlsc_cfg->intp_gain_min = 2097152;


			if (runtime->mlsc_mem_info.size) {
#ifndef ARCH_RTOS_CV181X
				isp_mlsc_ctrl_packing_tbl(
					(CVI_U8 *)(runtime->mlsc_mem_info.vir_addr),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_r),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_g),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_b));
				FLUSH_CACHE_COMPAT(
					runtime->mlsc_mem_info.phy_addr,
					runtime->mlsc_mem_info.vir_addr,
					runtime->mlsc_mem_info.size);
#else
				isp_mlsc_ctrl_packing_tbl(
					(CVI_U8 *)(runtime->mlsc_mem_info.phy_addr),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_r),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_g),
					ISP_PTR_CAST_U16(runtime->mlsc_param_out.mlsc_lut_b));
#endif
			}
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_buf_init(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_memblock *mem_info = &(runtime->mlsc_mem_info);

	if (runtime->mapped == CVI_TRUE)
		return ret;

	mem_info->raw_num = ViPipe;

#ifndef ARCH_RTOS_CV181X
	G_EXT_CTRLS_PTR(VI_IOCTL_GET_LSC_PHY_BUF, mem_info);

	ISP_LOG_DEBUG("lsc phy addr=0x%llx\n", mem_info->phy_addr);
	ISP_LOG_DEBUG("lsc size=0x%x\n", mem_info->size);

	if (mem_info->size) {
		mem_info->vir_addr = MMAP_CACHE_COMPAT(mem_info->phy_addr, mem_info->size);

		ISP_LOG_DEBUG("runtime->mlsc_mem_info.vir_addr=%p\n"
			, mem_info->vir_addr);
	}
#else
	// TODO: mason.zou
#endif

	runtime->mapped = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_buf_uninit(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifndef ARCH_RTOS_CV181X
	struct cvi_vip_memblock *mem_info = &(runtime->mlsc_mem_info);

	if (mem_info->size) {
		MUNMAP_COMPAT(mem_info->vir_addr, mem_info->size);
	}
#endif
	return ret;
}

// TODO@ST Fix coding style
#define LSC_GAIN_UNIT 2
#define LSC_GAIN_UNIT_SIZE 3
#define LSC_GAIN_ROW_SIZE (((((CVI_ISP_LSC_GRID_COL + LSC_GAIN_UNIT - 1) \
							/ LSC_GAIN_UNIT) * LSC_GAIN_UNIT_SIZE) + 15) >> 4 << 4)
static CVI_S32 isp_mlsc_ctrl_packing_tbl(CVI_U8 *lsctable, CVI_U16 *r, CVI_U16 *g, CVI_U16 *b)
{
	if (lsctable == CVI_NULL)
		return CVI_FAILURE;
	if (r == CVI_NULL)
		return CVI_FAILURE;
	if (b == CVI_NULL)
		return CVI_FAILURE;
	if (g == CVI_NULL)
		return CVI_FAILURE;

	CVI_U8 *p;
	CVI_U16 * channel[LSC_COLOR_CHANNEL_MAX] = { r, g, b };
	CVI_S32 i, j, k;

	#ifndef __riscv_vector
	for (j = 0; j < CVI_ISP_LSC_GRID_COL; j++) {
		for (k = 0; k < LSC_COLOR_CHANNEL_MAX; k++) {
			p = &lsctable[LSC_GAIN_ROW_SIZE * (j * LSC_COLOR_CHANNEL_MAX + k)];
			for (i = 0; i < CVI_ISP_LSC_GRID_ROW; i += LSC_GAIN_UNIT) {
				p[0] = (channel[k][j * CVI_ISP_LSC_GRID_ROW + i] & 0xFF0) >> 4;
				p[1] = (channel[k][j * CVI_ISP_LSC_GRID_ROW + i] & 0xF) << 4;
				if (i + 1 < CVI_ISP_LSC_GRID_COL) {
					p[1] |= (channel[k][j * CVI_ISP_LSC_GRID_ROW + i + 1] & 0xF00) >> 8;
					p[2] = (channel[k][j * CVI_ISP_LSC_GRID_ROW + i + 1] & 0x0FF);
				}
				p += LSC_GAIN_UNIT_SIZE;
			}
		}
	}

	#else
	CVI_S32 col;
	size_t vl;
	vuint16m8_t vu16C0, vu16C1, vu16Val;
	vuint8m4_t  vu8Val;
	const ptrdiff_t src_offset = CVI_ISP_LSC_GRID_ROW * sizeof(uint16_t);
	const ptrdiff_t tbl_offset = LSC_COLOR_CHANNEL_MAX * LSC_GAIN_ROW_SIZE * sizeof(CVI_U8);

	for (i = 0, col = 0; i < CVI_ISP_LSC_GRID_ROW; i += LSC_GAIN_UNIT, col += LSC_GAIN_UNIT_SIZE) {
		for (k = 0; k < LSC_COLOR_CHANNEL_MAX; k++) {
			p = &lsctable[col + k * LSC_GAIN_ROW_SIZE];
			for (j = 0; j < CVI_ISP_LSC_GRID_COL; j += vl) {
				vl = vsetvl_e16m8(CVI_ISP_LSC_GRID_COL - j);
				vu16C0 = vlse16_v_u16m8(&channel[k][j * CVI_ISP_LSC_GRID_ROW + i], src_offset, vl);
				vu16C1 = vlse16_v_u16m8(&channel[k][j * CVI_ISP_LSC_GRID_ROW + i + 1], src_offset, vl);

				// p[0]
				vu8Val = vnsrl_wx_u8m4(vu16C0, 4, vl);
				vsse8_v_u8m4(p, tbl_offset, vu8Val, vl);
				// p[1]
				vu16C0 = vsll_vx_u16m8(vand_vx_u16m8(vu16C0, 0x0F, vl), 4, vl);
				vu8Val = vncvt_x_x_w_u8m4(vu16C0, vl);
				if (i + 1 < CVI_ISP_LSC_GRID_COL) {
					vu16Val = vand_vx_u16m8(vu16C1, 0x0F00, vl);
					vu8Val = vor_vv_u8m4(vu8Val, vnsrl_wx_u8m4(vu16Val, (uint8_t)8, vl), vl);
				}
				vsse8_v_u8m4(p+1, tbl_offset, vu8Val, vl);
				// p[2]
				if (i + 1 < CVI_ISP_LSC_GRID_COL) {
					vu8Val = vncvt_x_x_w_u8m4(vu16C1, vl);
					vsse8_v_u8m4(p+2, tbl_offset, vu8Val, vl);
				}
				p += LSC_COLOR_CHANNEL_MAX * LSC_GAIN_ROW_SIZE * vl;
			}
		}
	}

	#endif // __riscv_vector
	return 0;
}

static CVI_S32 set_mlsc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_MESH_SHADING_ATTR_S *mlsc_attr = NULL;

		const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *mlsc_lut_attr = NULL;

		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &mlsc_attr);
		isp_mlsc_ctrl_get_mlsc_lut_attr(ViPipe, &mlsc_lut_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->MeshShadingEnable = mlsc_attr->Enable;
		pProcST->MeshShadingisManualMode = mlsc_attr->enOpType;
		pProcST->MeshShadingGLSize = mlsc_lut_attr->Size;

		//manual or auto
		for (CVI_U32 u32TempIdx = 0; u32TempIdx < ISP_MLSC_COLOR_TEMPERATURE_SIZE; ++u32TempIdx) {
			pProcST->MeshShadingGLColorTemperature[u32TempIdx]
				= mlsc_lut_attr->LscGainLut[u32TempIdx].ColorTemperature;
		}
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
//#endif // ENABLE_ISP_C906L

static struct isp_mlsc_ctrl_runtime  *_get_mlsc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_mlsc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MLSC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_mlsc_ctrl_check_mlsc_attr_valid(const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstMeshShadingAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstMeshShadingAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstMeshShadingAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstMeshShadingAttr, OverflowProtection, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_1D(pstMeshShadingAttr, MeshStr, 0x0, 0xfff);

	return ret;
}

static CVI_S32 isp_mlsc_ctrl_check_mlsc_lut_attr_valid(
	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S * pstMeshShadingGainLutAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstMeshShadingGainLutAttr, Size, 0x1, 0x7);

	for (CVI_U32 tempIdx = 0; tempIdx < pstMeshShadingGainLutAttr->Size; ++tempIdx) {
		const ISP_MESH_SHADING_GAIN_LUT_S *pstMeshShadingGainLut
			= &(pstMeshShadingGainLutAttr->LscGainLut[tempIdx]);

		CHECK_VALID_CONST(pstMeshShadingGainLut, ColorTemperature, 0x0, 0x7530);
		CHECK_VALID_ARRAY_1D(pstMeshShadingGainLut, RGain, CVI_ISP_LSC_GRID_POINTS, 0x0, 0xfff);
		CHECK_VALID_ARRAY_1D(pstMeshShadingGainLut, GGain, CVI_ISP_LSC_GRID_POINTS, 0x0, 0xfff);
		CHECK_VALID_ARRAY_1D(pstMeshShadingGainLut, BGain, CVI_ISP_LSC_GRID_POINTS, 0x0, 0xfff);
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_mlsc_ctrl_get_mlsc_attr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S **pstMeshShadingAttr)
{
	if (pstMeshShadingAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MLSC, (CVI_VOID *) &shared_buffer);

	*pstMeshShadingAttr = &shared_buffer->mlsc;

	return ret;
}

CVI_S32 isp_mlsc_ctrl_set_mlsc_attr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	if (pstMeshShadingAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_mlsc_ctrl_check_mlsc_attr_valid(pstMeshShadingAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_MESH_SHADING_ATTR_S *p = CVI_NULL;

	isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstMeshShadingAttr, sizeof(*pstMeshShadingAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_mlsc_ctrl_get_mlsc_lut_attr(VI_PIPE ViPipe,
			const ISP_MESH_SHADING_GAIN_LUT_ATTR_S **pstMeshShadingGainLutAttr)
{
	if (pstMeshShadingGainLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MLSC, (CVI_VOID *) &shared_buffer);

	*pstMeshShadingGainLutAttr = &shared_buffer->mlscLUT;

	return ret;
}

CVI_S32 isp_mlsc_ctrl_set_mlsc_lut_attr(VI_PIPE ViPipe
	, const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	if (pstMeshShadingGainLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_mlsc_ctrl_check_mlsc_lut_attr_valid(pstMeshShadingGainLutAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *p = CVI_NULL;

	isp_mlsc_ctrl_get_mlsc_lut_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstMeshShadingGainLutAttr, sizeof(*pstMeshShadingGainLutAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_mlsc_ctrl_get_mlsc_info(VI_PIPE ViPipe, struct mlsc_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mlsc_ctrl_runtime *runtime = _get_mlsc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_MESH_SHADING_ATTR_S *mlsc_attr = NULL;

	isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &mlsc_attr);

	if (mlsc_attr->Enable && !runtime->is_module_bypass) {
		info->lut_r = runtime->mlsc_lut_attr.RGain;
		info->lut_g = runtime->mlsc_lut_attr.GGain;
		info->lut_b = runtime->mlsc_lut_attr.BGain;
		info->mlsc_compensate_gain = runtime->mlsc_param_out.mlsc_compensate_gain;
	} else {
		info->lut_r = runtime->unit_gain_table;
		info->lut_g = runtime->unit_gain_table;
		info->lut_b = runtime->unit_gain_table;
		info->mlsc_compensate_gain = 1.0;
	}

	UNUSED(ViPipe);

	return ret;
}


