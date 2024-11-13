/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_test.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cvi_isp.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_vi.h"
#include "cvi_sys.h"
#include "cvi_bin.h"
#include "sample_comm.h"

#include "isp_test.h"

#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_main.h"
#include "isp_peri_ctrl.h"

#include "../common/raw_replay/raw_replay_test.h"

typedef enum {
	ISP_TEST_PQ_BIN = 100,
} ISP_TEST;

ISP_BLACK_LEVEL_ATTR_S setBlcAttr;
ISP_COLOR_TONE_ATTR_S setwbgAttr;
ISP_DRC_ATTR_S setDRCAttr;
ISP_DEMOSAIC_ATTR_S setCFAAttr;
ISP_CROSSTALK_ATTR_S setGEAttr;
ISP_FSWDR_ATTR_S setFusionAttr;
ISP_NR_ATTR_S setBnrAttr;
ISP_DEHAZE_ATTR_S setDehazeAttr;
ISP_CCM_ATTR_S setccmAttr;
ISP_GAMMA_ATTR_S setGammaAttr;
ISP_YNR_ATTR_S setYnrAttr;
ISP_CNR_ATTR_S setCnrAttr;
ISP_DCI_ATTR_S setdciAttr;
ISP_CLUT_ATTR_S set3dlutAttr;
ISP_SHARPEN_ATTR_S setSharpAttr;
ISP_CAC_ATTR_S setPfcAttr;
//ISP_SHADING_LUT_ATTR_S setmslut, getmslut;

#define	RANDOM_INIT_AUTO	0
#define RESULT_PATH			"/mnt/data/res/"
#define EXTENSION_FILE_TYPE	".bin"
#define MAX_WAIT_VD_NUM 10
#define WAIT_VD_TIMEOUT_MSEC 200

#define RANDOM_INIT_ATTR(attr, reg, min, max) do {\
	(attr) = rand() % ((max) - (min) + 1) + (min);\
	ISP_LOG_DEBUG("random init: %s(%s) [%d-%d] = %d (%#08x)\n", #attr, #reg, min, max, attr, (CVI_U32)attr);\
} while (0)

#define MANUAL_INIT_ATTR(attr, reg, min, max) do {\
	int input;\
	printf("input %s [%d-%d]: ", #attr, (min), (max));\
	scanf("%d", &input);\
	(attr) = input;\
} while (0)

#define INIT_ATTR(method, attr, reg, min, max) do {\
	if (method == 1)\
		RANDOM_INIT_ATTR(attr, reg, min, max);\
	else if (method == 2)\
		MANUAL_INIT_ATTR(attr, reg, min, max);\
} while (0)

#define PRINT_ATTR(attr, reg, min, max) ISP_LOG_DEBUG("%s(%s) [%d-%d] = %d\n", #attr, #reg, min, max, attr)

#define PRINT_TEST_RESULT(compareResult, block) do {\
	if (compareResult) {\
		ISP_LOG_ERR("%s FAIL\n", #block);\
	} else {\
		ISP_LOG_DEBUG("%s SUCCESS\n", #block);\
	} \
} while (0)

#define ISP_TEST_RELEASE_MEMORY(x) do { \
	if (x) { \
		free(x); \
		x = 0; \
	} \
} while (0)

static CVI_S32 createFolder(const char *file_name);

static CVI_VOID hexDump(CVI_VOID *addr, CVI_S32 len)
{
	CVI_S32 i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char *)addr;

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];

		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
}

#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
bool hexdiff(void *pointer1, void *pointer2, size_t len)
{
	bool pass = true;
	size_t i;
	char *p1 = (char *)pointer1;
	char *p2 = (char *)pointer2;

	for (i = 0; i < len; i++) {
		if (*p1 == *p2)
			printf("[%zu] %hhu (%#02hhx) ", i, *p1, *p1);
		else {
			printf(RED "[%zu] %hhu!=%hhu (%#02hhx!=%#02hhx) " RESET, i, *p1, *p2, *p1, *p2);
			pass = false;
		}

		if (i != 0 && i % 16 == 0)
			printf("\n");

		p1++;
		p2++;
	}

	if (pass)
		printf(GREEN "\npass" RESET "\n");
	else
		printf(RED "\nfail" RESET "\n");
	return pass;
}

CVI_S32 reloadParameter(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_RandomInitBlackLevelAttr(ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr,  int method)
{
	if (pstBlackLevelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	// non-auto attributes
	INIT_ATTR(method, pstBlackLevelAttr->Enable, reg_blc_enable, 0, 1);
	INIT_ATTR(method, pstBlackLevelAttr->enOpType, reg_na, 0, 1);

	// manual attributes
	INIT_ATTR(method, pstBlackLevelAttr->stManual.OffsetR, reg_Offset_r, 0, 1023);
	INIT_ATTR(method, pstBlackLevelAttr->stManual.OffsetGr, reg_Offset_gr, 0, 1023);
	INIT_ATTR(method, pstBlackLevelAttr->stManual.OffsetGb, reg_Offset_gb, 0, 1023);
	INIT_ATTR(method, pstBlackLevelAttr->stManual.OffsetB, reg_Offset_b, 0, 1023);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstBlackLevelAttr->stAuto.OffsetR[i], reg_Offset_r, 0, 1023);
		INIT_ATTR(method, pstBlackLevelAttr->stAuto.OffsetGr[i], reg_Offset_gr, 0, 1023);
		INIT_ATTR(method, pstBlackLevelAttr->stAuto.OffsetGb[i], reg_Offset_gb, 0, 1023);
		INIT_ATTR(method, pstBlackLevelAttr->stAuto.OffsetB[i], reg_Offset_b, 0, 1023);
	}
	#endif
	return 0;
}

void CVI_ISP_RandomInitNRAttr(ISP_NR_ATTR_S *pstNRAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstNRAttr->Enable, reg_bnr_enable, 0, 1);
	INIT_ATTR(method, pstNRAttr->enOpType, reg_na, 0, 1);

	// manual attributes
	INIT_ATTR(method, pstNRAttr->stManual.WindowType
		, (reg_bnr_weight_intra_0, reg_bnr_weight_intra_1, reg_bnr_weight_intra_2
		, reg_bnr_weight_norm_intra1, reg_bnr_weight_norm_intra2), 0, 11);
	INIT_ATTR(method, pstNRAttr->stManual.DetailSmoothMode, reg_bnr_flag_neighbor_max_weight, 0, 1);
	INIT_ATTR(method, pstNRAttr->stManual.NoiseSuppressStr, reg_bnr_ns_gain, 0, 255);
	INIT_ATTR(method, pstNRAttr->stManual.FilterType, reg_bnr_weight_lut_h, 0, 255);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	method = 1;
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstNRAttr->stAuto.WindowType[i]
			, (reg_bnr_weight_intra_0, reg_bnr_weight_intra_1, reg_bnr_weight_intra_2
			, reg_bnr_weight_norm_intra1, reg_bnr_weight_norm_intra2), 0, 11);
		INIT_ATTR(method, pstNRAttr->stAuto.DetailSmoothMode[i], reg_bnr_flag_neighbor_max_weight, 0, 1);
		INIT_ATTR(method, pstNRAttr->stAuto.NoiseSuppressStr[i], reg_bnr_ns_gain, 0, 255);
		INIT_ATTR(method, pstNRAttr->stAuto.FilterType[i], reg_bnr_weight_lut_h, 0, 255);
	}
	#endif
}

void CVI_ISP_RandomInitNRFilterAttr(ISP_NR_FILTER_ATTR_S *pstNRFilterAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstNRFilterAttr->TuningMode, reg_bnr_out_sel, 0, 15);

	// manual attributes
	INIT_ATTR(method, pstNRFilterAttr->stManual.VarThr, reg_bnr_var_th, 0, 1023);
	INIT_ATTR(method, pstNRFilterAttr->stManual.CoringWgtLF, reg_bnr_res_ratio_k_smooth, 0, 256);
	INIT_ATTR(method, pstNRFilterAttr->stManual.CoringWgtHF, reg_bnr_res_ratio_k_texture, 0, 256);
	INIT_ATTR(method, pstNRFilterAttr->stManual.NonDirFiltStr, reg_bnr_weight_smooth, 0, 31);
	INIT_ATTR(method, pstNRFilterAttr->stManual.VhDirFiltStr, (reg_bnr_weight_v, reg_bnr_weight_h), 0, 31);
	INIT_ATTR(method, pstNRFilterAttr->stManual.AaDirFiltStr
		, (reg_bnr_weight_d35, reg_bnr_weight_d135), 0, 31);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	method = 1;
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstNRFilterAttr->stAuto.VarThr[i], reg_bnr_var_th, 0, 1023);
		INIT_ATTR(method, pstNRFilterAttr->stAuto.CoringWgtLF[i], reg_bnr_res_ratio_k_smooth, 0, 256);
		INIT_ATTR(method, pstNRFilterAttr->stAuto.CoringWgtHF[i], reg_bnr_res_ratio_k_texture, 0, 256);
		INIT_ATTR(method, pstNRFilterAttr->stAuto.NonDirFiltStr[i], reg_bnr_weight_smooth, 0, 31);
		INIT_ATTR(method, pstNRFilterAttr->stAuto.VhDirFiltStr[i], (reg_bnr_weight_v, reg_bnr_weight_h), 0, 31);
		INIT_ATTR(method, pstNRFilterAttr->stAuto.AaDirFiltStr[i]
			, (reg_bnr_weight_d35, reg_bnr_weight_d135), 0, 31);
	}
	#endif
}

//-----------------------------------------------------------------------------
//	YNR
//-----------------------------------------------------------------------------
void CVI_ISP_RandomInitYNRAttr(ISP_YNR_ATTR_S *pstYNRAttr, int method)
{
	CVI_S32 i = 0;

	// non-auto attributes
	INIT_ATTR(method, pstYNRAttr->Enable, reg_ynr_enable, 0, 1);
	pstYNRAttr->TuningMode = 8;

	// manual attributes
	INIT_ATTR(method, pstYNRAttr->stManual.WindowType,
		  (reg_ynr_weight_intra_0, reg_ynr_weight_intra_1, reg_ynr_weight_intra_2, reg_bnr_weight_norm_intra1,
		   reg_bnr_weight_norm_intra2),
		  0, 11);
	INIT_ATTR(method, pstYNRAttr->stManual.DetailSmoothMode, reg_ynr_flag_neighbor_max_weight, 0, 1);
	INIT_ATTR(method, pstYNRAttr->stManual.NoiseSuppressStr, reg_ynr_ns_gain, 0, 255);
	INIT_ATTR(method, pstYNRAttr->stManual.FilterType, reg_ynr_weight_lut_h[256], 0, 255);

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstYNRAttr->stAuto.WindowType[i],
			  (reg_ynr_weight_intra_0, reg_ynr_weight_intra_1, reg_ynr_weight_intra_2,
			   reg_bnr_weight_norm_intra1, reg_bnr_weight_norm_intra2),
			  0, 11);
		INIT_ATTR(method, pstYNRAttr->stAuto.DetailSmoothMode[i], reg_ynr_flag_neighbor_max_weight, 0, 1);
		INIT_ATTR(method, pstYNRAttr->stAuto.NoiseSuppressStr[i], reg_ynr_ns_gain, 0, 255);
		INIT_ATTR(method, pstYNRAttr->stAuto.FilterType[i], reg_ynr_weight_lut_h[256], 0, 255);
	}
}

void CVI_ISP_RandomInitYNRMotionNRAttr(ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	// non-auto attributes

	// manual attributes

	// auto attributes

	(void)pstYNRMotionNRAttr;
}

void CVI_ISP_RandomInitYNRFilterAttr(ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes

	// manual attributes
	pstYNRFilterAttr->stManual.VarThr = rand() % 256;
	pstYNRFilterAttr->stManual.CoringWgtLF = rand() % 257;
	pstYNRFilterAttr->stManual.CoringWgtHF = rand() % 257;
	pstYNRFilterAttr->stManual.NonDirFiltStr = rand() % 32;
	pstYNRFilterAttr->stManual.VhDirFiltStr = rand() % 32;
	pstYNRFilterAttr->stManual.AaDirFiltStr = rand() % 32;
	pstYNRFilterAttr->stManual.FilterMode = rand() % 1024;

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		pstYNRFilterAttr->stAuto.VarThr[i] = rand() % 256;
		pstYNRFilterAttr->stAuto.CoringWgtLF[i] = rand() % 257;
		pstYNRFilterAttr->stAuto.CoringWgtHF[i] = rand() % 257;
		pstYNRFilterAttr->stAuto.NonDirFiltStr[i] = rand() % 32;
		pstYNRFilterAttr->stAuto.VhDirFiltStr[i] = rand() % 32;
		pstYNRFilterAttr->stAuto.AaDirFiltStr[i] = rand() % 32;
		pstYNRFilterAttr->stAuto.FilterMode[i] = rand() % 1024;
	}
}

void CVI_ISP_RandomInitCNRAttr(ISP_CNR_ATTR_S *pstCNRAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstCNRAttr->Enable, reg_cnr_enable, 0, 1);

	// manual attributes
	INIT_ATTR(method, pstCNRAttr->stManual.CnrStr, reg_cnr_strength_mode, 0, 255);
	INIT_ATTR(method, pstCNRAttr->stManual.NoiseSuppressStr, reg_cnr_diff_shift_val, 0, 255);
	INIT_ATTR(method, pstCNRAttr->stManual.NoiseSuppressGain, reg_cnr_diff_gain, 0, 7);
	INIT_ATTR(method, pstCNRAttr->stManual.FilterType, reg_cnr_weight_lut_inter_block, 0, 16);
	INIT_ATTR(method, pstCNRAttr->stManual.MotionNrStr, reg_cnr_ratio, 0, 255);
	INIT_ATTR(method, pstCNRAttr->stManual.LumaWgt, reg_cnr_fusion_intensity_weight, 0, 8);
	INIT_ATTR(method, pstCNRAttr->stManual.DetailSmoothMode, reg_cnr_flag_neighbor_max_weight, 0, 1);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	method = 1;
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstCNRAttr->stAuto.CnrStr[i], reg_cnr_strength_mode, 0, 255);
		INIT_ATTR(method, pstCNRAttr->stAuto.NoiseSuppressStr[i], reg_cnr_diff_shift_val, 0, 255);
		INIT_ATTR(method, pstCNRAttr->stAuto.NoiseSuppressGain[i], reg_cnr_diff_gain, 0, 7);
		INIT_ATTR(method, pstCNRAttr->stAuto.FilterType[i], reg_cnr_weight_lut_inter_block, 0, 16);
		INIT_ATTR(method, pstCNRAttr->stAuto.MotionNrStr[i], reg_cnr_ratio, 0, 255);
		INIT_ATTR(method, pstCNRAttr->stAuto.LumaWgt[i], reg_cnr_fusion_intensity_weight, 0, 8);
		INIT_ATTR(method, pstCNRAttr->stAuto.DetailSmoothMode[i], reg_cnr_flag_neighbor_max_weight, 0, 1);
	}
	#endif
}

//-----------------------------------------------------------------------------
//	DCI
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitDCIAttr(ISP_DCI_ATTR_S *pstDCIAttr)
{
	CVI_S32 i = 0;

#if 0
	// non-auto attributes
	pstDCIAttr->Enable = rand() % 2;
	pstDCIAttr->TuningMode = rand() % 2;

	// manual attributes
	pstDCIAttr->stManual.MapEnable = rand() % 2;
	pstDCIAttr->stManual.ContrastGain = rand() % 1024;
	// pstDCIAttr->stManual.ContrastGain = 1000;	// TODO delete this

	pstDCIAttr->stManual.BlcThr = rand() % 256;
	pstDCIAttr->stManual.WhtThr = rand() % 256;
	pstDCIAttr->stManual.BlcCtrl = rand() % 513;
	pstDCIAttr->stManual.WhtCtrl = rand() % 513;
#else	// debug
	// non-auto attributes
	pstDCIAttr->Enable = rand() % 2;

	// manual attributes
	// pstDCIAttr->stManual.ContrastGain = rand() % 1024;
	pstDCIAttr->stManual.ContrastGain = 1000;	// TODO delete this

	pstDCIAttr->stManual.BlcThr = 0;
	pstDCIAttr->stManual.WhtThr = 255;
	pstDCIAttr->stManual.BlcCtrl = 256;
	pstDCIAttr->stManual.WhtCtrl = 256;
#endif

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		pstDCIAttr->stAuto.ContrastGain[i] = rand() % 1024;

		pstDCIAttr->stAuto.BlcThr[i] = rand() % 256;
		pstDCIAttr->stAuto.WhtThr[i] = rand() % 256;
		pstDCIAttr->stAuto.BlcCtrl[i] = rand() % 513;
		pstDCIAttr->stAuto.WhtCtrl[i] = rand() % 513;
	}
	return CVI_SUCCESS;
}

void CVI_ISP_PrintDCIAttr(const ISP_DCI_ATTR_S *p1, const ISP_DCI_ATTR_S *p2)
{
	CVI_S32 i = 0;

	// non-auto attributes
	printf("Enable = %d %d\n", p1->Enable, p2->Enable);
	printf("TuningMode = %d %d\n", p1->TuningMode, p2->TuningMode);

	// manual attributes
	printf("stManual.ContrastGain = %d %d\n", p1->stManual.ContrastGain, p2->stManual.ContrastGain);
	printf("stManual.BlcThr = %d %d\n", p1->stManual.BlcThr, p2->stManual.BlcThr);
	printf("stManual.WhtThr = %d %d\n", p1->stManual.WhtThr, p2->stManual.WhtThr);
	printf("stManual.BlcCtrl = %d %d\n", p1->stManual.BlcCtrl, p2->stManual.BlcCtrl);
	printf("stManual.WhtCtrl = %d %d\n", p1->stManual.WhtCtrl, p2->stManual.WhtCtrl);

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		printf("stAuto.ContrastGain[%d] = %d %d\n", i, p1->stAuto.ContrastGain[i], p2->stAuto.ContrastGain[i]);
		printf("stAuto.BlcThr[%d] = %d %d\n", i, p1->stAuto.BlcThr[i], p2->stAuto.BlcThr[i]);
		printf("stAuto.WhtThr[%d] = %d %d\n", i, p1->stAuto.WhtThr[i], p2->stAuto.WhtThr[i]);
		printf("stAuto.BlcCtrl[%d] = %d %d\n", i, p1->stAuto.BlcCtrl[i], p2->stAuto.BlcCtrl[i]);
		printf("stAuto.WhtCtrl[%d] = %d %d\n", i, p1->stAuto.WhtCtrl[i], p2->stAuto.WhtCtrl[i]);
	}
}

//-----------------------------------------------------------------------------
//	MLSC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitMeshShadingAttr(ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes
	pstMeshShadingAttr->Enable = rand() % 2;

	// manual attributes
	pstMeshShadingAttr->stManual.MeshStr = rand() % 4096;

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
		pstMeshShadingAttr->stAuto.MeshStr[i] = rand() % 4096;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_RandomInitMeshShadingGainLutAttr(ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	CVI_S32 i = 0;

	if (pstMeshShadingGainLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	// non-auto attributes
	for (CVI_U32 t = 0; t < ISP_MLSC_COLOR_TEMPERATURE_SIZE; t++) {
		ISP_MESH_SHADING_GAIN_LUT_S *lut = &pstMeshShadingGainLutAttr->LscGainLut[t];

		#if 0
		// memset(lut->RGain, 0x0A, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->RGain[0]));
		// memset(lut->GGain, 0x0B, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->GGain[0]));
		// memset(lut->BGain, 0x0C, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->BGain[0]));
		memset(lut->RGain, t*10+1, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->RGain[0]));
		memset(lut->GGain, t*10+2, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->GGain[0]));
		memset(lut->BGain, t*10+3, CVI_ISP_LSC_GRID_POINTS*sizeof(lut->BGain[0]));
		#else
		for (i = 0; i < CVI_ISP_LSC_GRID_POINTS; i++) {
			lut->RGain[i] = rand() % 4096;
			lut->GGain[i] = rand() % 4096;
			lut->BGain[i] = rand() % 4096;
		}
		#endif

		#if 1
		char *p;

		p = (char *)lut->RGain;
		printf("%s r[0]=%x t=%d\n", __func__, p[0], t);
		#endif
	}

	// TODO for long exposure
	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//	RLSC
//-----------------------------------------------------------------------------
//CVI_S32 CVI_ISP_RandomInitRadialShadingAttr(ISP_RADIAL_SHADING_ATTR_S *pstRadialShadingAttr)
//{
//	// non-auto attributes
//	pstRadialShadingAttr->Enable = rand() % 2;
//	pstRadialShadingAttr->CenterX = rand() % 4096;	// TODO should print warning message if this > sensor width
//	pstRadialShadingAttr->CenterY = rand() % 2048;	// TODO should print warning message if this > sensor height
//
//	return CVI_SUCCESS;
//}
//
//CVI_S32 CVI_ISP_RandomInitRadialShadingGainLutAttr(ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S *pstRadialShadingGainLutAttr)
//{
//	CVI_S32 i = 0;
//	CVI_S32 ct = 0;
//
//	// non-auto attributes
//	// TODO interplace by color temperature
//	for (ct = 0; ct < ISP_RLSC_COLOR_TEMPERATURE_SIZE; ct++) {
//		for (i = 0; i < 32; i++) {
//			pstRadialShadingGainLutAttr->RLscGainLut[ct].RGain[i] = rand() % 4096;
//			pstRadialShadingGainLutAttr->RLscGainLut[ct].GGain[i] = rand() % 4096;
//			pstRadialShadingGainLutAttr->RLscGainLut[ct].BGain[i] = rand() % 4096;
//			pstRadialShadingGainLutAttr->RLscGainLut[ct].IrGain[i] = rand() % 4096;
//		}
//	}
//
//	return CVI_SUCCESS;
//}

CVI_S32 CVI_ISP_RandomInitDemosaicAttr(ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstDemosaicAttr->Enable, reg_cfa_enable, 0, 1);
	INIT_ATTR(method, pstDemosaicAttr->TuningMode, reg_cfa_out_sel, 0, 1);

	// manual attributes
	INIT_ATTR(method, pstDemosaicAttr->stManual.CoarseEdgeThr, reg_cfa_edgee_thd, 0, 4095);
	INIT_ATTR(method, pstDemosaicAttr->stManual.CoarseStr, reg_cfa_edge_tol, 0, 4095);
	INIT_ATTR(method, pstDemosaicAttr->stManual.FineEdgeThr, reg_cfa_sige_thd, 0, 4095);
	INIT_ATTR(method, pstDemosaicAttr->stManual.FineStr, (reg_cfa_gsig_tol, reg_cfa_rbsig_tol), 0, 4095);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	method = 1;
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstDemosaicAttr->stAuto.CoarseEdgeThr[i], reg_cfa_edgee_thd, 0, 4095);
		INIT_ATTR(method, pstDemosaicAttr->stAuto.CoarseStr[i], reg_cfa_edge_tol, 0, 4095);
		INIT_ATTR(method, pstDemosaicAttr->stAuto.FineEdgeThr[i], reg_cfa_sige_thd, 0, 4095);
		INIT_ATTR(method, pstDemosaicAttr->stAuto.FineStr[i], (reg_cfa_gsig_tol, reg_cfa_rbsig_tol), 0, 4095);
	}
	#endif

	return 0;
}

CVI_S32 CVI_ISP_RandomInitDemosaicDemoireAttr(ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr,  int method)
{
	// non-auto attributes

	(void)pstDemosaicDemoireAttr;
	(void)method;

	return 0;
}

//-----------------------------------------------------------------------------
//	DEHAZE
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitDehazeAttr(ISP_DEHAZE_ATTR_S *pstDehazeAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstDehazeAttr->Enable, reg_dehaze_enable, 0, 2);
	// pstDehazeAttr->enOpType = OP_TYPE_AUTO;
	// printf("enable %d %d\n", pstDehazeAttr->Enable, pstDehazeAttr->enOpType);
	INIT_ATTR(method, pstDehazeAttr->CumulativeThr, reg_dehaze_cum_th, 0, 16383);
	INIT_ATTR(method, pstDehazeAttr->MinTransMapValue, reg_dehaze_tmap_min, 0, 8192);

	// manual attributes
	INIT_ATTR(method, pstDehazeAttr->stManual.Strength, reg_dehaze_w, 0, 100);

	// auto attributes
	#if RANDOM_INIT_AUTO
	CVI_S32 i = 0;

	method = 1;
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstDehazeAttr->stAuto.Strength[i], reg_dehaze_w, 0, 100);
	}
	#endif
	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//	CCM
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitSaturationAttr(ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes

	// manual attributes
	pstSaturationAttr->stManual.Saturation = rand() % 256;

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
		pstSaturationAttr->stAuto.Saturation[i] = rand() % 256;

	return 0;
}

CVI_S32 CVI_ISP_RandomInitCCMAttr(ISP_CCM_ATTR_S *pstCCMAttr,  int method)
{
	CVI_U8 table = 0;

	// non-auto attributes
	INIT_ATTR(method, pstCCMAttr->Enable, reg_ccm_enable, 0, 1);
	// INIT_ATTR(method, pstCCMAttr->enOpType, reg_na, 0, 1);
	INIT_ATTR(method, pstCCMAttr->enOpType, reg_na, 1, 1);	// manual

	INIT_ATTR(method, pstCCMAttr->stManual.SatEnable, reg_na, 0, 1);

	INIT_ATTR(method, pstCCMAttr->stAuto.ISOActEnable, reg_na, 0, 1);
	INIT_ATTR(method, pstCCMAttr->stAuto.TempActEnable, reg_na, 0, 1);
	//high to low, (D50, TL84, A) or (10K, D65, D50, TL84, A)
	// INIT_ATTR(pstCCMAttr->stAuto.CCMTabNum, 0, 7);
	INIT_ATTR(method, pstCCMAttr->stAuto.CCMTabNum, reg_na, 3, 3);
	INIT_ATTR(method, pstCCMAttr->stAuto.CCMTab[0].ColorTemp, reg_na, 5000, 5000);	//D50
	INIT_ATTR(method, pstCCMAttr->stAuto.CCMTab[1].ColorTemp, reg_na, 4100, 4100);	//TL84
	INIT_ATTR(method, pstCCMAttr->stAuto.CCMTab[2].ColorTemp, reg_na, 2800, 2800);;	//A
	for (table = 0; table < pstCCMAttr->stAuto.CCMTabNum; table++) {
		for (CVI_U8 cell = 0; cell < 9; cell++) {
			if (cell == 0 || cell == 4 || cell == 8)
				INIT_ATTR(method, pstCCMAttr->stAuto.CCMTab[table].CCM[cell], reg_na, 512, 1536);
			else
				INIT_ATTR(method, pstCCMAttr->stAuto.CCMTab[table].CCM[cell], reg_na, -512, 512);
		}
	}

	INIT_ATTR(method, pstCCMAttr->stManual.SatEnable, reg_na, 0, 1);
	for (CVI_U8 cell = 0; cell < 9; cell++)
		if (cell == 0 || cell == 4 || cell == 8)
			INIT_ATTR(method, pstCCMAttr->stManual.CCM[cell], reg_ccm_xx, 512, 1536);
		else
			INIT_ATTR(method, pstCCMAttr->stManual.CCM[cell], reg_ccm_xx, -512, 512);

	// manual attributes

	// auto attributes
	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//	CAC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitCACAttr(ISP_CAC_ATTR_S *pstCACAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes
	pstCACAttr->Enable = rand() % 2;
	pstCACAttr->enOpType = rand() % 2;
	// manual attributes
	pstCACAttr->stManual.DePurpleStr = rand() % 256;
	pstCACAttr->GreenCb = rand() % 256;
	pstCACAttr->GreenCr = rand() % 256;
	pstCACAttr->PurpleCb = rand() % 256;
	pstCACAttr->PurpleCr = rand() % 256;
	pstCACAttr->PurpleDetRange = rand() % 128;
	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		pstCACAttr->stAuto.DePurpleStr[i] = rand() % 256;
	}

	return 0;
}

//-----------------------------------------------------------------------------
//	Gamma
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitGammaAttr(ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes
	pstGammaAttr->Enable = rand() % 2;
	pstGammaAttr->enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
	for (i = 0; i < GAMMA_NODE_NUM; i++) {
		pstGammaAttr->Table[i] = rand() % 4096;
	}

	// manual attributes

	// auto attributes

	return 0;
}
//-----------------------------------------------------------------------------
//	DPC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitDPDynamicAttr(ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes
	pstDPCDynamicAttr->Enable = rand() % 2;

	// manual attributes
	pstDPCDynamicAttr->stManual.ClusterSize = rand() % 4;
	pstDPCDynamicAttr->stManual.BrightDefectToNormalPixRatio = rand() % 256;
	pstDPCDynamicAttr->stManual.DarkDefectToNormalPixRatio = rand() % 256;
	pstDPCDynamicAttr->stManual.FlatThreR = rand() % 256;
	pstDPCDynamicAttr->stManual.FlatThreG = rand() % 256;
	pstDPCDynamicAttr->stManual.FlatThreB = rand() % 256;
	pstDPCDynamicAttr->stManual.FlatThreMinG = rand() % 256;
	pstDPCDynamicAttr->stManual.FlatThreMinRB = rand() % 256;

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		pstDPCDynamicAttr->stAuto.ClusterSize[i] = rand() % 4;
		pstDPCDynamicAttr->stAuto.BrightDefectToNormalPixRatio[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.DarkDefectToNormalPixRatio[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.FlatThreR[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.FlatThreG[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.FlatThreB[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.FlatThreMinG[i] = rand() % 256;
		pstDPCDynamicAttr->stAuto.FlatThreMinRB[i] = rand() % 256;
	}

	return 0;
}

CVI_S32 CVI_ISP_RandomInitDPStaticAttr(ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	// TODO@Kidd implement
	UNUSED(pstDPStaticAttr);

	return 0;
}

CVI_S32 CVI_ISP_RandomInitDPCalibrate(ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	// TODO@Kidd implement
	UNUSED(pstDPCalibAttr);

	return 0;
}

//-----------------------------------------------------------------------------
//	Crosstalk
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitCrosstalkAttr(ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	CVI_S32 i = 0;

	// non-auto attributes
	pstCrosstalkAttr->Enable = rand() % 2;
	for (i = 0; i < 4; i++) {
		pstCrosstalkAttr->GrGbDiffThreSec[i] = rand() % 4-96;
		pstCrosstalkAttr->FlatThre[i] = rand() % 4096;
	}

	// manual attributes
	pstCrosstalkAttr->stManual.Strength = rand() % 256;

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
		pstCrosstalkAttr->stAuto.Strength[i] = rand() % 256;

	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//	Crosstalk
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitColorToneAttr(ISP_COLOR_TONE_ATTR_S *pstColorToneAttr,  int method)
{
	// non-auto attributes
	INIT_ATTR(method, pstColorToneAttr->u16RedCastGain, reg_wbg_rgain, 0, 4095);
	INIT_ATTR(method, pstColorToneAttr->u16GreenCastGain, reg_wbg_bgain, 0, 4095);
	INIT_ATTR(method, pstColorToneAttr->u16BlueCastGain, reg_wbg_ggain, 0, 4095);

	// manual attributes

	// auto attributes
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_RandomInitTNRAttr(ISP_TNR_ATTR_S *pstTNRAttr, int random)
{
	int i = 0;

	// non-auto attributes
	INIT_ATTR(random, pstTNRAttr->Enable, reg_manr_enable, 0, 1);

	// manual attributes
	INIT_ATTR(random, pstTNRAttr->stManual.TnrStrength0, reg_mmap_0_map_gain, 0, 255);
	INIT_ATTR(random, pstTNRAttr->stManual.MapThdLow0, reg_mmap_0_map_thd_l, 0, 255);
	INIT_ATTR(random, pstTNRAttr->stManual.MapThdHigh0, reg_mmap_0_map_thd_h, 0, 255);

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(random, pstTNRAttr->stAuto.TnrStrength0[i], reg_mmap_0_map_gain, 0, 255);
		INIT_ATTR(random, pstTNRAttr->stAuto.MapThdLow0[i], reg_mmap_0_map_thd_l, 0, 255);
		INIT_ATTR(random, pstTNRAttr->stAuto.MapThdHigh0[i], reg_mmap_0_map_thd_h, 0, 255);
	}
	return CVI_SUCCESS;
}

void CVI_ISP_RandomInitTNRLumaMotionAttr(ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr, int random)
{
	int i = 0, j = 0;

	// manual attributes
	for (i = 0; i < 4; i++) {
		INIT_ATTR(random, pstTNRLumaMotionAttr->stManual.L2mIn0[i],
			  reg_mmap_0_luma_adapt_lut_in_0 - reg_mmap_0_luma_adapt_lut_in_3, 0, 4095);
		INIT_ATTR(random, pstTNRLumaMotionAttr->stManual.L2mOut0[i],
			  reg_mmap_0_luma_adapt_lut_out_0 - reg_mmap_0_luma_adapt_lut_out_3, 0, 255);
		INIT_ATTR(random, pstTNRLumaMotionAttr->stManual.L2mIn1[i],
			  reg_mmap_1_luma_adapt_lut_in_0 - reg_mmap_1_luma_adapt_lut_in_3, 0, 4095);
		INIT_ATTR(random, pstTNRLumaMotionAttr->stManual.L2mOut1[i],
			  reg_mmap_1_luma_adapt_lut_out_0 - reg_mmap_1_luma_adapt_lut_out_3, 0, 255);
	}
	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		for (j = 0; j < 4; j++) {
			INIT_ATTR(random, pstTNRLumaMotionAttr->stAuto.L2mIn0[j][i],
				  reg_mmap_0_luma_adapt_lut_in_0 - reg_mmap_0_luma_adapt_lut_in_3, 0, 4095);
			INIT_ATTR(random, pstTNRLumaMotionAttr->stAuto.L2mOut0[j][i],
				  reg_mmap_0_luma_adapt_lut_out_0 - reg_mmap_0_luma_adapt_lut_out_3, 0, 255);
			INIT_ATTR(random, pstTNRLumaMotionAttr->stAuto.L2mIn1[j][i],
				  reg_mmap_1_luma_adapt_lut_in_0 - reg_mmap_1_luma_adapt_lut_in_3, 0, 4095);
			INIT_ATTR(random, pstTNRLumaMotionAttr->stAuto.L2mOut1[j][i],
				  reg_mmap_1_luma_adapt_lut_out_0 - reg_mmap_1_luma_adapt_lut_out_3, 0, 255);
		}
	}
}

void CVI_ISP_RandomInitTNRNoiseModelAttr(ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr, int random)
{
	int i = 0;

	// manual attributes
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.RNoiseLevel0,
		  reg_mmap_0_ns_slope_r - reg_mmap_0_ns_high_offset_r, 0, 255);
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.GNoiseLevel0,
		  reg_mmap_0_ns_slope_g - reg_mmap_0_ns_high_offset_g, 0, 255);
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.BNoiseLevel0,
		  reg_mmap_0_ns_slope_b - reg_mmap_0_ns_high_offset_b, 0, 255);
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.RNoiseLevel1,
		  reg_mmap_1_ns_slope_r - reg_mmap_1_ns_high_offset_r, 0, 255);
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.GNoiseLevel1,
		  reg_mmap_1_ns_slope_g - reg_mmap_1_ns_high_offset_g, 0, 255);
	INIT_ATTR(random, pstTNRNoiseModelAttr->stManual.BNoiseLevel1,
		  reg_mmap_1_ns_slope_b - reg_mmap_1_ns_high_offset_b, 0, 255);

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.RNoiseLevel0[i],
			  reg_mmap_0_ns_slope_r - reg_mmap_0_ns_high_offset_r, 0, 255);
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.GNoiseLevel0[i],
			  reg_mmap_0_ns_slope_g - reg_mmap_0_ns_high_offset_g, 0, 255);
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.BNoiseLevel0[i],
			  reg_mmap_0_ns_slope_b - reg_mmap_0_ns_high_offset_b, 0, 255);
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.RNoiseLevel1[i],
			  reg_mmap_1_ns_slope_r - reg_mmap_1_ns_high_offset_r, 0, 255);
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.GNoiseLevel1[i],
			  reg_mmap_1_ns_slope_g - reg_mmap_1_ns_high_offset_g, 0, 255);
		INIT_ATTR(random, pstTNRNoiseModelAttr->stAuto.BNoiseLevel1[i],
			  reg_mmap_1_ns_slope_b - reg_mmap_1_ns_high_offset_b, 0, 255);
	}
}

void CVI_ISP_RandomInitTNRGhostAttr(ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr, int random)
{
	int i = 0, j = 0;

	// manual attributes
	for (i = 0; i < 4; i++) {
		INIT_ATTR(random, pstTNRGhostAttr->stManual.PrvMotion0[i],
			  reg_mmap_0_iir_prtct_lut_in_0 - reg_mmap_0_iir_prtct_lut_in_3, 0, 255);
		INIT_ATTR(random, pstTNRGhostAttr->stManual.PrtctWgt0[i],
			  reg_mmap_0_iir_prtct_lut_out_0 - reg_mmap_0_iir_prtct_lut_out_3, 0, 15);
	}
	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		for (j = 0; j < 4; j++) {
			INIT_ATTR(random, pstTNRGhostAttr->stAuto.PrvMotion0[j][i],
				  reg_mmap_0_iir_prtct_lut_in_0 - reg_mmap_0_iir_prtct_lut_in_3, 0, 255);
			INIT_ATTR(random, pstTNRGhostAttr->stAuto.PrtctWgt0[j][i],
				  reg_mmap_0_iir_prtct_lut_out_0 - reg_mmap_0_iir_prtct_lut_out_3, 0, 15);
		}
	}
}

void CVI_ISP_RandomInitTNRMtPtrAttr(ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr, int random)
{
	UNUSED(pstTNRMtPrtAttr);
	UNUSED(random);
	#if 0
	int i = 0, j = 0;

	// manual attributes
	INIT_ATTR(random, pstTNRMtPrtAttr->LowMtPrtEn, reg_motion_sel, 0, 1);
	INIT_ATTR(random, pstTNRMtPrtAttr->stManual.LowMtPrtLevel, reg_3dnr_beta_max, 0, 255);
	for (i = 0; i < 4; i++) {
		INIT_ATTR(random, pstTNRMtPrtAttr->stManual.LowMtPrtIn[i], reg_3dnr_lut_in_0 - reg_3dnr_lut_in_3, 0,
			  255);
		INIT_ATTR(random, pstTNRMtPrtAttr->stManual.LowMtPrtOut[i], reg_3dnr_lut_out_0 - reg_3dnr_lut_out_3, 0,
			  255);
	}

	// auto attributes
	for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(random, pstTNRMtPrtAttr->stAuto.LowMtPrtLevel[i], reg_3dnr_beta_max, 0, 255);
		for (j = 0; j < 4; j++) {
			INIT_ATTR(random, pstTNRMtPrtAttr->stAuto.LowMtPrtIn[j][i],
				  reg_3dnr_lut_in_0 - reg_3dnr_lut_in_3, 0, 255);
			INIT_ATTR(random, pstTNRMtPrtAttr->stAuto.LowMtPrtOut[j][i],
				  reg_3dnr_lut_out_0 - reg_3dnr_lut_out_3, 0, 255);
		}
		for (j = 0; j < 3; j++) {
			INIT_ATTR(random, pstTNRMtPrtAttr->stAuto.LowMtPrtSlope0[j][i],
				  reg_3dnr_lut_slope_0 - reg_3dnr_lut_slope_3, -2048, 2047);
		}
	}
	#endif
}
//-----------------------------------------------------------------------------
//	FSWDR
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitFSWDRAttr(ISP_FSWDR_ATTR_S *pstFSWDRAttr, int method)
{
	if (pstFSWDRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	// non-auto attributes
	INIT_ATTR(method, pstFSWDRAttr->Enable, reg_fs_enable, 0, 1);
	INIT_ATTR(method, pstFSWDRAttr->MotionCompEnable, reg_fs_mc_enable, 0, 1);
	INIT_ATTR(method, pstFSWDRAttr->TuningMode, reg_fs_out_sel, 0, 3);

	// manual attributes
	INIT_ATTR(method, pstFSWDRAttr->stManual.MergeModeAlpha, reg_mmap_mrg_alph, 0, 1);
	INIT_ATTR(method, pstFSWDRAttr->stManual.WDRCombineShortThr, reg_fs_luma_thd_l, 0, 4095);
	INIT_ATTR(method, pstFSWDRAttr->stManual.WDRCombineLongThr, reg_fs_luma_thd_h, 0, 4095);
	INIT_ATTR(method, pstFSWDRAttr->stManual.WDRCombineMinWeight, reg_fs_wgt_min, 0, 255);
	INIT_ATTR(method, pstFSWDRAttr->stManual.WDRCombineMaxWeight, reg_fs_wgt_max, 0, 255);

	// auto attributes
	for (CVI_U8 i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
		INIT_ATTR(method, pstFSWDRAttr->stAuto.MergeModeAlpha[i], reg_mmap_mrg_alph, 0, 1);
		INIT_ATTR(method, pstFSWDRAttr->stAuto.WDRCombineShortThr[i], reg_fs_luma_thd_l, 0, 4095);
		INIT_ATTR(method, pstFSWDRAttr->stAuto.WDRCombineLongThr[i], reg_fs_luma_thd_h, 0, 4095);
		INIT_ATTR(method, pstFSWDRAttr->stAuto.WDRCombineMinWeight[i], reg_fs_wgt_min, 0, 255);
		INIT_ATTR(method, pstFSWDRAttr->stAuto.WDRCombineMaxWeight[i], reg_fs_wgt_max, 0, 255);
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_RandomInitWDRExposureAttr(ISP_WDR_EXPOSURE_ATTR_S *pstWDRExposureAttr, int method)
{
	if (pstWDRExposureAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	// non-auto attributes
	PRINT_ATTR(pstWDRExposureAttr->au32ExpRatio[0], reg_fs_ls_gain, 64, 16383);

	// manual attributes

	// auto attributes
	for (CVI_U8 i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
	}

	UNUSED(method);
	return 0;
}

//-----------------------------------------------------------------------------
//	DRC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitDRCAttr(ISP_DRC_ATTR_S *pstDRCAttr, int method)
{
	if (pstDRCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}


	// non-auto attributes
	INIT_ATTR(method, pstDRCAttr->Enable, reg_ltm_enable, 0, 1);
	INIT_ATTR(method, pstDRCAttr->TuningMode, reg_ltm_dbg_enable, 0, 4);
	INIT_ATTR(method, pstDRCAttr->ToneCurveSelect, reg_ltm_deflt_lut[769], 0, 1);
	for (CVI_U32 i = 0; i < 769; i++) {
		INIT_ATTR(method, pstDRCAttr->CurveUserDefine[i], reg_na, 0, 4096);
	}

	return 0;
}

//-----------------------------------------------------------------------------
//	Shapren
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_RandomInitSharpenAttr(ISP_SHARPEN_ATTR_S *pstSharpenAttr, int method)
{
	(void)pstSharpenAttr;
	(void)method;

	return 0;
}
//-----------------------------------------------------------------------------
//	MAIN
//-----------------------------------------------------------------------------
int isp_block_mpi_setest(VI_PIPE ViPipe, int blockNum, int method)
{
	ISP_LOG_DEBUG("blockNum %d\n", blockNum);

	srand(time(NULL));

	isp_iqParam_addr_initDefault(ViPipe);

	switch (blockNum) {
	case ISP_IQ_BLOCK_BLC: {
		ISP_BLACK_LEVEL_ATTR_S get = {0};
		ISP_BLACK_LEVEL_ATTR_S set = {0};

		CVI_ISP_RandomInitBlackLevelAttr(&set, method);
		CVI_ISP_SetBlackLevelAttr(ViPipe, &set);
		CVI_ISP_GetBlackLevelAttr(ViPipe, &get);
		PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), BLC);
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_IQ_BLOCK_WBGAIN: {
		ISP_COLOR_TONE_ATTR_S get = {0};
		ISP_COLOR_TONE_ATTR_S set = {0};

		{
			ISP_NR_ATTR_S NR;
			ISP_NR_FILTER_ATTR_S NRFilter;

			CVI_ISP_RandomInitNRAttr(&NR, method);
			CVI_ISP_RandomInitNRFilterAttr(&NRFilter, method);

			CVI_ISP_SetNRAttr(ViPipe, &NR);
			CVI_ISP_SetNRFilterAttr(ViPipe, &NRFilter);
		}

		CVI_ISP_RandomInitColorToneAttr(&set, method);
		CVI_ISP_SetColorToneAttr(ViPipe, &set);
		CVI_ISP_GetColorToneAttr(ViPipe, &get);
		printf("ColorTone result %d\n", memcmp(&set, &get, sizeof(set)));
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_IQ_BLOCK_BNR: {
		// NR
		{
			ISP_NR_ATTR_S get = {0};
			ISP_NR_ATTR_S set = {0};

			CVI_ISP_RandomInitNRAttr(&set, method);
			CVI_ISP_SetNRAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetNRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), BNRAttr);
			hexdiff(&set, &get, sizeof(set));
		}

		//NRFilter
		{
			ISP_NR_FILTER_ATTR_S get = {0};
			ISP_NR_FILTER_ATTR_S set = {0};

			CVI_ISP_RandomInitNRFilterAttr(&set, method);
			CVI_ISP_SetNRFilterAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetNRFilterAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), BNRFilter);
			hexdiff(&set, &get, sizeof(set));
		}

		break;
	}
	case ISP_IQ_BLOCK_YNR: {
		// YNR
		{
			ISP_YNR_ATTR_S get = {0};
			ISP_YNR_ATTR_S set = {0};

			CVI_ISP_RandomInitYNRAttr(&set, method);
			CVI_ISP_SetYNRAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetYNRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), YNR);
			hexdiff(&set, &get, sizeof(set));
		}
		// YNRMotionNR
		{
			ISP_YNR_MOTION_NR_ATTR_S get = {0};
			ISP_YNR_MOTION_NR_ATTR_S set = {0};

			CVI_ISP_RandomInitYNRMotionNRAttr(&set);
			CVI_ISP_SetYNRMotionNRAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetYNRMotionNRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), YNRMition);
			hexdiff(&set, &get, sizeof(set));
		}
		// YNRFilter
		{
			ISP_YNR_FILTER_ATTR_S get = {0};
			ISP_YNR_FILTER_ATTR_S set = {0};

			CVI_ISP_RandomInitYNRFilterAttr(&set);
			CVI_ISP_SetYNRFilterAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetYNRFilterAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), YNRFilter);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_CNR: {
		// CNR
		{
			ISP_CNR_ATTR_S get = {0};
			ISP_CNR_ATTR_S set = {0};

			CVI_ISP_RandomInitCNRAttr(&set, method);
			CVI_ISP_SetCNRAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetCNRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), CNR);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_DCI: {
		// DCI
		{
			ISP_DCI_ATTR_S get = {0};
			ISP_DCI_ATTR_S set = {0};

			CVI_ISP_RandomInitDCIAttr(&set);
			CVI_ISP_SetDCIAttr(ViPipe, &set);
			CVI_ISP_GetDCIAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), DCI);
			CVI_ISP_PrintDCIAttr(&set, &get);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_CAC: {
		// DCI
		{
			ISP_CAC_ATTR_S get = {0};
			ISP_CAC_ATTR_S set = {0};

			CVI_ISP_RandomInitCACAttr(&set);
			CVI_ISP_SetCACAttr(ViPipe, &set);
			CVI_ISP_GetCACAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), CAC);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_DEMOSAIC: {
		// Demosaic
		{
			ISP_DEMOSAIC_ATTR_S set = {0};
			ISP_DEMOSAIC_ATTR_S get = {0};

			// CVI_ISP_RandomInitDemosaicAttr(&set, method);
			CVI_ISP_SetDemosaicAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetDemosaicAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), Demosaic);
			hexdiff(&set, &get, sizeof(set));
		}
		// Demosaic Demorie
		{
			ISP_DEMOSAIC_DEMOIRE_ATTR_S set = {0};
			ISP_DEMOSAIC_DEMOIRE_ATTR_S get = {0};

			CVI_ISP_RandomInitDemosaicDemoireAttr(&set, method);
			CVI_ISP_SetDemosaicDemoireAttr(ViPipe, &set);
			reloadParameter(ViPipe);
			CVI_ISP_GetDemosaicDemoireAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), DemosaicDemoire);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_CCM: {
		// Saturation
		{
			ISP_SATURATION_ATTR_S set = {0};
			ISP_SATURATION_ATTR_S get = {0};

			CVI_ISP_RandomInitSaturationAttr(&set);
			CVI_ISP_SetSaturationAttr(ViPipe, &set);
			CVI_ISP_GetSaturationAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), Saturation);
			hexdiff(&set, &get, sizeof(set));
		}
		// CCM
		{
			ISP_CCM_ATTR_S set = {0};
			ISP_CCM_ATTR_S get = {0};

			CVI_ISP_RandomInitCCMAttr(&set, method);
			CVI_ISP_SetCCMAttr(ViPipe, &set);
			CVI_ISP_GetCCMAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), CCM);
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_DPC:
	{
		// Static
		{
			ISP_DP_STATIC_ATTR_S set = {0};
			ISP_DP_STATIC_ATTR_S get = {0};

			CVI_ISP_RandomInitDPStaticAttr(&set);
			CVI_ISP_SetDPStaticAttr(ViPipe, &set);
			CVI_ISP_GetDPStaticAttr(ViPipe, &get);
			printf("DPStatic result %d\n", memcmp(&set, &get, sizeof(ISP_DP_STATIC_ATTR_S)));
			hexdiff(&set, &get, sizeof(set));
		}
		// Dynamic
		{
			ISP_DP_DYNAMIC_ATTR_S set = {0};
			ISP_DP_DYNAMIC_ATTR_S get = {0};

			CVI_ISP_RandomInitDPDynamicAttr(&set);
			CVI_ISP_SetDPDynamicAttr(ViPipe, &set);
			CVI_ISP_GetDPDynamicAttr(ViPipe, &get);
			printf("DPDynamic result %d\n", memcmp(&set, &get, sizeof(ISP_DP_DYNAMIC_ATTR_S)));
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}

	case ISP_IQ_BLOCK_CROSSTALK:
	{
		ISP_CROSSTALK_ATTR_S set = {0};
		ISP_CROSSTALK_ATTR_S get = {0};

		CVI_ISP_RandomInitCrosstalkAttr(&set);
		CVI_ISP_SetCrosstalkAttr(ViPipe, &set);
		CVI_ISP_GetCrosstalkAttr(ViPipe, &get);
		printf("Crosstalk result %d\n", memcmp(&set, &get, sizeof(ISP_CROSSTALK_ATTR_S)));
		hexdiff(&set, &get, sizeof(set));
		break;
	}

	case ISP_IQ_BLOCK_MLSC: {
		#if 1
		// MLSC
		{
			ISP_MESH_SHADING_ATTR_S set = {0};
			ISP_MESH_SHADING_ATTR_S get = {0};

			CVI_ISP_RandomInitMeshShadingAttr(&set);
			CVI_ISP_SetMeshShadingAttr(ViPipe, &set);
			CVI_ISP_GetMeshShadingAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), MeshShading);
			hexdiff(&set, &get, sizeof(set));
		}
		#endif
		#if 1
		// MLSC LUT
		{
			ISP_MESH_SHADING_GAIN_LUT_ATTR_S set = {0};
			ISP_MESH_SHADING_GAIN_LUT_ATTR_S get = {0};

			CVI_ISP_RandomInitMeshShadingGainLutAttr(&set);
			CVI_ISP_SetMeshShadingGainLutAttr(ViPipe, &set);
			CVI_ISP_GetMeshShadingGainLutAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), MeshShadingLut);
			hexdiff(&set, &get, sizeof(set));
		}
		#endif
		break;
	}

	case ISP_IQ_BLOCK_RLSC: {
		// RLSC
		{
			//ISP_RADIAL_SHADING_ATTR_S set = {0};
			//ISP_RADIAL_SHADING_ATTR_S get = {0};

			//CVI_ISP_RandomInitRadialShadingAttr(&set);
			//CVI_ISP_SetRadialShadingAttr(ViPipe, &set);
			//CVI_ISP_GetRadialShadingAttr(ViPipe, &get);
			//PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), RadialShading);
			//hexdiff(&set, &get, sizeof(set));
		}
		// RLSC LUT
		{
			//ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S set = {0};
			//ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S get = {0};

			//CVI_ISP_RandomInitRadialShadingGainLutAttr(&set);
			//CVI_ISP_SetRadialShadingGainLutAttr(ViPipe, &set);
			//CVI_ISP_GetRadialShadingGainLutAttr(ViPipe, &get);
			//PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), RadialShadingLut);
			//hexdiff(&set, &get, sizeof(set));
		}
		break;
	}
	case ISP_IQ_BLOCK_GAMMA: {
		ISP_GAMMA_ATTR_S get = {0};
		ISP_GAMMA_ATTR_S set = {0};

		CVI_ISP_RandomInitGammaAttr(&set);
		CVI_ISP_SetGammaAttr(ViPipe, &set);
		CVI_ISP_GetGammaAttr(ViPipe, &get);
		PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), GAMMA);
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_IQ_BLOCK_DEHAZE: {
		ISP_DEHAZE_ATTR_S set = {0};
		ISP_DEHAZE_ATTR_S get = {0};

		CVI_ISP_RandomInitDehazeAttr(&set, method);
		CVI_ISP_SetDehazeAttr(ViPipe, &set);
		CVI_ISP_GetDehazeAttr(ViPipe, &get);
		PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), DEHAZE);
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_IQ_BLOCK_3DNR: {
		{
			ISP_TNR_ATTR_S get = { 0 };
			ISP_TNR_ATTR_S set = { 0 };

			CVI_ISP_RandomInitTNRAttr(&set, method);
			//set.tnr_enable = 0;
			//set.enOpType = 0;
			CVI_ISP_SetTNRAttr(ViPipe, &set);
			CVI_ISP_GetTNRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), TNR);
			hexdiff(&set, &get, sizeof(set));
		}

		{
			ISP_TNR_NOISE_MODEL_ATTR_S get = { 0 };
			ISP_TNR_NOISE_MODEL_ATTR_S set = { 0 };

			CVI_ISP_RandomInitTNRNoiseModelAttr(&set, method);
			CVI_ISP_SetTNRNoiseModelAttr(ViPipe, &set);
			CVI_ISP_GetTNRNoiseModelAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), TNRNoiseModel);
			hexdiff(&set, &get, sizeof(set));
		}

		{
			ISP_TNR_LUMA_MOTION_ATTR_S get = { 0 };
			ISP_TNR_LUMA_MOTION_ATTR_S set = { 0 };

			CVI_ISP_RandomInitTNRLumaMotionAttr(&set, method);
			CVI_ISP_SetTNRLumaMotionAttr(ViPipe, &set);
			CVI_ISP_GetTNRLumaMotionAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), TNRLumaMotion);
			hexdiff(&set, &get, sizeof(set));
		}

		{
			ISP_TNR_GHOST_ATTR_S get = { 0 };
			ISP_TNR_GHOST_ATTR_S set = { 0 };

			CVI_ISP_RandomInitTNRGhostAttr(&set, method);
			CVI_ISP_SetTNRGhostAttr(ViPipe, &set);
			CVI_ISP_GetTNRGhostAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), TNRGhost);
			hexdiff(&set, &get, sizeof(set));
		}

		{
			ISP_TNR_MT_PRT_ATTR_S get = { 0 };
			ISP_TNR_MT_PRT_ATTR_S set = { 0 };

			CVI_ISP_RandomInitTNRMtPtrAttr(&set, method);
			CVI_ISP_SetTNRMtPrtAttr(ViPipe, &set);
			CVI_ISP_GetTNRMtPrtAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), TNRMtPrt);
			hexdiff(&set, &get, sizeof(set));
		}

		break;
	}
	case ISP_IQ_BLOCK_FUSION: {
		{
			//FSWDR
			ISP_FSWDR_ATTR_S set = {0};
			ISP_FSWDR_ATTR_S get = {0};

			CVI_ISP_RandomInitFSWDRAttr(&set, method);
			CVI_ISP_SetFSWDRAttr(ViPipe, &set);
			CVI_ISP_GetFSWDRAttr(ViPipe, &get);
			PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), Fusion);
			hexdiff(&set, &get, sizeof(set));
		}
		{
			//WDRExposure
			ISP_WDR_EXPOSURE_ATTR_S set = {0};
			ISP_WDR_EXPOSURE_ATTR_S get = {0};

			CVI_ISP_RandomInitWDRExposureAttr(&set, method);
			CVI_ISP_SetWDRExposureAttr(ViPipe, &set);
			CVI_ISP_GetWDRExposureAttr(ViPipe, &get);
			printf("WDRExposure result %d\n", memcmp(&set, &get, sizeof(get)));
			hexdiff(&set, &get, sizeof(set));
		}
		break;
	}
	case ISP_IQ_BLOCK_DRC: {
		ISP_DRC_ATTR_S set = {0};
		ISP_DRC_ATTR_S get = {0};

		CVI_ISP_RandomInitDRCAttr(&set, method);
		CVI_ISP_SetDRCAttr(ViPipe, &set);
		CVI_ISP_GetDRCAttr(ViPipe, &get);
		PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), DRC);
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_IQ_BLOCK_YEE: {
		ISP_SHARPEN_ATTR_S set = {0};
		ISP_SHARPEN_ATTR_S get = {0};

		CVI_ISP_RandomInitSharpenAttr(&set, method);
		CVI_ISP_SetSharpenAttr(ViPipe, &set);
		CVI_ISP_GetSharpenAttr(ViPipe, &get);
		PRINT_TEST_RESULT(memcmp(&set, &get, sizeof(set)), Sharpen);
		hexdiff(&set, &get, sizeof(set));
		break;
	}
	case ISP_TEST_PQ_BIN: {

		#define TEST_BIN "/mnt/data/bin/isp_test"
		#define GRAY_BIN "/mnt/data/bin/isp_gray"

		createFolder(TEST_BIN);

		// Demosaic Enable should be 1
		{
			ISP_DEMOSAIC_ATTR_S attr = {.Enable = 0};

			CVI_ISP_GetDemosaicAttr(ViPipe, &attr);

			if (attr.Enable == 0) {
				ISP_LOG_ERR(".Enable should be 1\n");
			} else {
				ISP_LOG_DEBUG(".Enable is 1\n");
			}
		}

		// CVI_PQ_BIN_GenerateBinFile
		{
			CVI_BIN_EXTRA_S BinExtra = {
				"kidd",
				"test-default",
				"2020-04-10",
			};

			FILE *fd = fopen(TEST_BIN, "wb");

			if (fd == NULL) {
				ISP_LOG_ERR("Open file failed\n");
			}
			CVI_BIN_HEADER header;
			CVI_U32 u32BinLen = 0, u32TempLen = 0;
			CVI_S32 ret = 0;
			CVI_U8 *pBuffer;

			memset(&header, 0, sizeof(CVI_BIN_HEADER));
			u32BinLen = CVI_BIN_GetBinTotalLen();
			pBuffer = (CVI_U8 *)malloc(u32BinLen);
			if (pBuffer == NULL) {
				ISP_LOG_ERR("malloc err!\n");
				fclose(fd);
				break;
			}
			header.extraInfo = BinExtra;
			memcpy(pBuffer, &header, sizeof(CVI_BIN_HEADER));
			ret = CVI_BIN_ExportBinData(pBuffer, u32BinLen);
			if (ret != CVI_SUCCESS) {
				if (pBuffer != NULL) {
					free(pBuffer);
				}
				ISP_LOG_ERR("CVI_BIN_ImportBinData error! errno(0x%x)\n", ret);
				break;
			}
			u32TempLen = fwrite(pBuffer, 1, u32BinLen, fd);
			if (u32TempLen != u32BinLen) {
				ISP_LOG_ERR("write bin buffer to %s fail\n", TEST_BIN);
				free(pBuffer);
				fclose(fd);
				break;
			}
			if (pBuffer != NULL) {
				free(pBuffer);
			}
			fclose(fd);
		}

		// CVI_PQ_BIN_GetBinExtraAttr
		{
			CVI_BIN_EXTRA_S BinExtraAttr = {};
			FILE *fd = fopen(TEST_BIN, "rb");

			CVI_BIN_GetBinExtraAttr(fd, &BinExtraAttr);

			if (strcmp((CVI_CHAR *)BinExtraAttr.Author, "kidd")) {
				ISP_LOG_ERR(".Author should be kidd, but get %s\n", BinExtraAttr.Author);
			}

			if (strcmp((CVI_CHAR *)BinExtraAttr.Desc, "test-default")) {
				ISP_LOG_ERR(".Desc should be test-default, but get %s\n", BinExtraAttr.Desc);
			}

			if (strcmp((CVI_CHAR *)BinExtraAttr.Time, "2020-04-10")) {
				ISP_LOG_ERR(".Time should be 2020-04-10, but get %s\n", BinExtraAttr.Time);
			}
		}

		// disable demosaic
		{
			ISP_DEMOSAIC_ATTR_S attr = {0};

			CVI_ISP_GetDemosaicAttr(ViPipe, &attr);
			attr.Enable = 0;
			CVI_ISP_SetDemosaicAttr(ViPipe, &attr);
		}

		// CVI_PQ_BIN_GenerateBinFile
		{
			CVI_BIN_EXTRA_S BinExtra = {
				"kidd",
				"test-disable-demosaic",
				"2020-04-10",
			};
			FILE *fd = fopen(GRAY_BIN, "wb");

			CVI_BIN_HEADER header;
			CVI_U32 u32BinLen = 0, u32TempLen = 0;
			CVI_U8 *pBuffer;
			CVI_S32 ret = 0;

			memset(&header, 0, sizeof(CVI_BIN_HEADER));
			u32BinLen = CVI_BIN_GetBinTotalLen();
			pBuffer = (CVI_U8 *)malloc(u32BinLen);
			if (pBuffer == NULL) {
				ISP_LOG_ERR("malloc err!\n");
				fclose(fd);
				break;
			}
			header.extraInfo = BinExtra;
			memcpy(pBuffer, &header, sizeof(CVI_BIN_HEADER));
			ret = CVI_BIN_ExportBinData(pBuffer, u32BinLen);
			if (ret != CVI_SUCCESS) {
				free(pBuffer);
				ISP_LOG_ERR("CVI_BIN_ImportBinData error! errno(0x%x)\n", ret);
				break;
			}
			u32TempLen = fwrite(pBuffer, 1, u32BinLen, fd);
			if (u32TempLen != u32BinLen) {
				ISP_LOG_ERR("write bin buffer to %s fail\n", GRAY_BIN);
				fclose(fd);
				if (pBuffer != NULL) {
					free(pBuffer);
				}
				break;
			}
			if (pBuffer != NULL) {
				free(pBuffer);
			}
			fclose(fd);
		}

		// sleep 10s and preview should be gray level
		{
			ISP_LOG_DEBUG("sleep 1s\n");
			sleep(1);
		}

		// CVI_PQ_BIN_ParseBinData
		{
			CVI_S32 result = CVI_FAILURE;
			CVI_U32 size = 0;
			FILE *fp;

			ISP_LOG_DEBUG("try load default value from bin %s\n", TEST_BIN);

			fp = fopen(TEST_BIN, "rb");
			if (fp == NULL) {
				ISP_LOG_WARNING("Cant find bin(%s)\n", TEST_BIN);
			} else {
				ISP_LOG_DEBUG("Bin exist (%s)\n", TEST_BIN);
			}

			fseek(fp, 0L, SEEK_END);
			size = ftell(fp);
			rewind(fp);
			printf("%d size\n", size);

			if (size > 0) {
				// allocate buffer
				CVI_U8 *binBuffer = malloc(size);

				fread(binBuffer, size, 1, fp);
				result = CVI_BIN_ImportBinData(binBuffer, 0);
				if (result != CVI_SUCCESS) {
					ISP_LOG_ERR("CVI_BIN_ImportBinData error! errno(0x%x)\n", result);
				}

				// free buffer
				free(binBuffer);

				if (result == CVI_SUCCESS) {
					ISP_LOG_DEBUG("load default value from bin %s\n", TEST_BIN);
					// return 0;
				}
			}
		}

		// Demosaic Enable should be 1
		{
			ISP_DEMOSAIC_ATTR_S attr = {.Enable = 0};

			CVI_ISP_GetDemosaicAttr(ViPipe, &attr);

			if (attr.Enable == 0) {
				ISP_LOG_ERR(".Enable should be 1\n");
			}
		}
		break;
	}
	}
	return 0;
}

#define MAX_FOLDER_HIERARCHY 16
static CVI_S32 createFolder(const char *file_name)
{
	CVI_S32 ret = CVI_SUCCESS;
	char tmp[BIN_FILE_LENGTH];
	char subFolder[MAX_FOLDER_HIERARCHY][BIN_FILE_LENGTH] = {0};
	int subFolderCnt = 0;
	struct stat status;

	char *token;

	if (file_name == NULL)
		return CVI_FAILURE;

	// Get folder hierarchy cnt
	{
		strcpy(tmp, file_name);

		/* get the first token */
		token = strtok(tmp, "/");

		while (token != NULL) {
			subFolderCnt++;
			token = strtok(NULL, "/");
		}
	}

	// Create folder recursively
	{
		strcpy(tmp, file_name);

		token = strtok(tmp, "/");
		for (int i = 0 ; i < subFolderCnt-1 ; i++) {
			for (int j = i ; j < subFolderCnt-1 ; j++) {
				sprintf(&subFolder[j][strlen(subFolder[j])], "/%s", token);
			}
			token = strtok(NULL, "/");
		}
		for (int i = 0 ; i < subFolderCnt-1 ; i++) {
			if (stat(subFolder[i], &status) == -1) {
				ISP_LOG_DEBUG("Folder path not exist! (%s)\n", subFolder[i]);
				mkdir(subFolder[i], 0666);
			}
		}
	}

	return ret;
}

static CVI_S32 isp_test_apply_wait(CVI_VOID)
{
	CVI_U32 waitVdNum;
	CVI_S32 s32Ret = 0;
	// wait some frame for setting apply.
	for (waitVdNum = 0; waitVdNum < MAX_WAIT_VD_NUM; waitVdNum++) {
		s32Ret = CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_START, 500);
		if (s32Ret != CVI_SUCCESS) {
			printf("Wait VD fail with %#x\n", s32Ret);
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

/*static CVI_S32 isp_test_answer_compare(char *filename, CVI_VOID *resultBuf,
 *					CVI_U32 bufferSize, AAA_LIB_TYPE_E aaaType)
{
#define AE_TOLERANCE 30
#define AWB_TOLERANCE 30
#define AF_TOLERANCE 30
	//We decouple auto test from the camera, so we add tolerance
	//We decouple auto test from compressMode, so we add tolerance
	FILE *fp;
	CVI_U8 *buffer = calloc(1, bufferSize);
	CVI_U32 length = 0;
	CVI_S32 s32Ret = CVI_SUCCESS;

	fp = fopen(filename, "r");
	if (fp == CVI_NULL) {
		printf("open data file %s error\n", filename);
		return CVI_FAILURE;
	}

	length = fread(buffer, bufferSize, 1, fp);
	if (length <= 0) {
		printf("fread data error\n");
		s32Ret = CVI_FAILURE;
		goto EXIT;
	}

	if (aaaType == AAA_TYPE_AE) {
		ISP_AE_STATISTICS_S *pAeStsRef = (ISP_AE_STATISTICS_S *)buffer;
		ISP_AE_STATISTICS_S *pAeSts = (ISP_AE_STATISTICS_S *)resultBuf;

		for (CVI_U8 i = 0; i < BAYER_PATTERN_NUM; ++i) {
			if (ABS((CVI_S32)(pAeStsRef->au16FEGlobalAvg[ISP_CHANNEL_LE][0][i] -
					pAeSts->au16FEGlobalAvg[ISP_CHANNEL_LE][0][i])) > AE_TOLERANCE) {
				printf("Not compare GlobalAvg%d [Ref:%d]<->[Get:%d]\n", i,
					pAeStsRef->au16FEGlobalAvg[ISP_CHANNEL_LE][0][i],
					pAeSts->au16FEGlobalAvg[ISP_CHANNEL_LE][0][i]);
				s32Ret = CVI_FAILURE;
				goto EXIT;
			}
		}
	} else if (aaaType == AAA_TYPE_AWB) {
		ISP_WB_STATISTICS_S *pAwbStsRef = (ISP_WB_STATISTICS_S *)buffer;
		ISP_WB_STATISTICS_S *pAwbSts = (ISP_WB_STATISTICS_S *)resultBuf;

		if (ABS((CVI_S32)(pAwbStsRef->u16GlobalR - pAwbSts->u16GlobalR)) > AWB_TOLERANCE) {
			printf("Not compare GlobalR [Ref:%d]<->[Get:%d]\n",
					pAwbStsRef->u16GlobalR, pAwbSts->u16GlobalR);
			s32Ret = CVI_FAILURE;
			goto EXIT;
		}

		if (ABS((CVI_S32)(pAwbStsRef->u16GlobalG - pAwbSts->u16GlobalG)) > AWB_TOLERANCE) {
			printf("Not compare GlobalG [Ref:%d]<->[Get:%d]\n",
					pAwbStsRef->u16GlobalG, pAwbSts->u16GlobalG);
			s32Ret = CVI_FAILURE;
			goto EXIT;
		}

		if (ABS((CVI_S32)(pAwbStsRef->u16GlobalB - pAwbSts->u16GlobalB)) > AWB_TOLERANCE) {
			printf("Not compare GlobalB [Ref:%d]<->[Get:%d]\n",
					pAwbStsRef->u16GlobalB, pAwbSts->u16GlobalB);
			s32Ret = CVI_FAILURE;
			goto EXIT;
		}

		for (CVI_U16 i = 0; i < AWB_ZONE_NUM; ++i) {
			if (ABS((CVI_S32)(pAwbStsRef->au16ZoneAvgR[i] - pAwbStsRef->au16ZoneAvgR[i]))) {
				printf("Not compare ZoneAvgR%d [Ref:%d]<->[Get:%d]\n", i,
					pAwbStsRef->au16ZoneAvgR[i], pAwbSts->au16ZoneAvgR[i]);
				s32Ret = CVI_FAILURE;
				goto EXIT;
			}
			if (ABS((CVI_S32)(pAwbStsRef->au16ZoneAvgG[i] - pAwbStsRef->au16ZoneAvgG[i]))) {
				printf("Not compare ZoneAvgG%d [Ref:%d]<->[Get:%d]\n", i,
					pAwbStsRef->au16ZoneAvgG[i], pAwbSts->au16ZoneAvgG[i]);
				s32Ret = CVI_FAILURE;
				goto EXIT;
			}
			if (ABS((CVI_S32)(pAwbStsRef->au16ZoneAvgB[i] - pAwbStsRef->au16ZoneAvgB[i]))) {
				printf("Not compare ZoneAvgB%d [Ref:%d]<->[Get:%d]\n", i,
					pAwbStsRef->au16ZoneAvgB[i], pAwbSts->au16ZoneAvgB[i]);
				s32Ret = CVI_FAILURE;
				goto EXIT;
			}
		}

	} else if (aaaType == AAA_TYPE_AF) {
		ISP_AF_STATISTICS_S *pAfStsRef = (ISP_AF_STATISTICS_S *)buffer;
		ISP_AF_STATISTICS_S *pAfSts = (ISP_AF_STATISTICS_S *)resultBuf;

		for (CVI_U8 row = 0; row < AF_ZONE_ROW; ++row) {
			for (CVI_U8 col = 0; col < AF_ZONE_COLUMN; ++col) {
				if (ABS((CVI_S64)(pAfStsRef->stFEAFStat.stZoneMetrics[row][col].u64h0 -
						pAfSts->stFEAFStat.stZoneMetrics[row][col].u64h0)) > AF_TOLERANCE) {
					printf("Not compare h0%d-%d [Ref:%" PRIu64 "]<->[Get:%" PRIu64 "\n", row, col,
						pAfStsRef->stFEAFStat.stZoneMetrics[row][col].u64h0,
						pAfSts->stFEAFStat.stZoneMetrics[row][col].u64h0);
				}
				if (ABS((CVI_S64)(pAfStsRef->stFEAFStat.stZoneMetrics[row][col].u64h1 -
						pAfSts->stFEAFStat.stZoneMetrics[row][col].u64h1)) > AF_TOLERANCE) {
					printf("Not compare h1%d-%d [Ref:%" PRIu64 "]<->[Get:%" PRIu64 "]\n", row, col,
						pAfStsRef->stFEAFStat.stZoneMetrics[row][col].u64h1,
						pAfSts->stFEAFStat.stZoneMetrics[row][col].u64h1);
				}
			}
		}
	} else {
		printf("Not support type\n");
		s32Ret = CVI_FAILURE;
	}

EXIT:
	fclose(fp);
	ISP_TEST_RELEASE_MEMORY(buffer);
	return s32Ret;
}

static CVI_S32 isp_test_result_save(char *filename, CVI_VOID *resultBuf, CVI_U32 bufferSize)
{
	FILE *fp;
	CVI_S32 s32Ret = CVI_SUCCESS;

	fp = fopen(filename, "w");
	if (fp == CVI_NULL) {
		printf("open data file error\n");
		return CVI_FAILURE;
	}

	fwrite(resultBuf, 1, bufferSize, fp);
	fclose(fp);
	return s32Ret;
}

static CVI_S32 isp_test_yuv_get(CVI_U8 chn, CVI_U8 **yuvBuf, CVI_U32 *yuvSize)
{
	VIDEO_FRAME_INFO_S stVideoFrame;
	VI_CROP_INFO_S crop_info = {0};
	CVI_U8 *tmpAddr = CVI_NULL;
	CVI_U32 tmpSize = 0;

	if (CVI_VI_GetChnFrame(0, chn, &stVideoFrame, 1000) == 0) {
		size_t image_size = stVideoFrame.stVFrame.u32Length[0] + stVideoFrame.stVFrame.u32Length[1]
				  + stVideoFrame.stVFrame.u32Length[2];
		CVI_VOID *vir_addr;
		CVI_U32 plane_offset, u32LumaSize, u32ChromaSize;

		if (yuvBuf == CVI_NULL) {
			ISP_LOG_WARNING("yuvBuf pointer is NULL\n");
			return CVI_FAILURE;
		}
		*yuvBuf = tmpAddr = calloc(1, image_size);
		if (*yuvBuf == CVI_NULL) {
			ISP_LOG_WARNING("yuvBuf allocate fail. Size =  %zu\n", image_size);
			return CVI_FAILURE;
		}
		ISP_LOG_WARNING("width: %d, height: %d, total_buf_length: %zu\n",
			   stVideoFrame.stVFrame.u32Width,
			   stVideoFrame.stVFrame.u32Height, image_size);

		u32LumaSize =  stVideoFrame.stVFrame.u32Stride[0] * stVideoFrame.stVFrame.u32Height;
		u32ChromaSize =  stVideoFrame.stVFrame.u32Stride[1] * stVideoFrame.stVFrame.u32Height / 2;
		CVI_VI_GetChnCrop(0, chn, &crop_info);
		if (crop_info.bEnable) {
			u32LumaSize = ALIGN((crop_info.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN) *
				ALIGN(crop_info.stCropRect.u32Height, 2);
			u32ChromaSize = (ALIGN(((crop_info.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN) *
				ALIGN(crop_info.stCropRect.u32Height, 2)) >> 1;
		}
		vir_addr = CVI_SYS_Mmap(stVideoFrame.stVFrame.u64PhyAddr[0], image_size);
		CVI_SYS_IonInvalidateCache(stVideoFrame.stVFrame.u64PhyAddr[0], vir_addr, image_size);
		plane_offset = 0;
		for (int i = 0; i < 3; i++) {
			stVideoFrame.stVFrame.pu8VirAddr[i] = vir_addr + plane_offset;
			plane_offset += stVideoFrame.stVFrame.u32Length[i];
#if 0
			ISP_LOG_WARNING("plane(%d): paddr(%lu) vaddr(%p) stride(%d) length(%d)\n",
				   i, stVideoFrame.stVFrame.u64PhyAddr[i],
				   stVideoFrame.stVFrame.pu8VirAddr[i],
				   stVideoFrame.stVFrame.u32Stride[i],
				   stVideoFrame.stVFrame.u32Length[i]);
#endif
			memcpy(tmpAddr, (void *)stVideoFrame.stVFrame.pu8VirAddr[i]
				, (i == 0) ? u32LumaSize : u32ChromaSize);
			tmpAddr += (i == 0) ? u32LumaSize : u32ChromaSize;
			tmpSize += (i == 0) ? u32LumaSize : u32ChromaSize;
		}
		CVI_SYS_Munmap(vir_addr, image_size);

		if (CVI_VI_ReleaseChnFrame(0, chn, &stVideoFrame) != 0)
			printf("CVI_VI_ReleaseChnFrame NG\n");

		if (yuvSize == CVI_NULL) {
			printf("yuv size pointer is NULL.\n");
			return CVI_FAILURE;
		}
		*yuvSize = tmpSize;

		return CVI_SUCCESS;
	}
	printf("CVI_VI_GetChnFrame NG\n");
	return CVI_FAILURE;
}
*/

CVI_S32 isp_statistic_test(CVI_VOID)
{
#if 0 //TODO@ST Fix auto test
#define MAX_TEST_CASE 2
	CVI_U32 i;
	CVI_S32 s32Ret = CVI_SUCCESS, finalRet = CVI_SUCCESS;
	ISP_AE_STATISTICS_S stAEStat[MAX_TEST_CASE] = {0};
	ISP_WB_STATISTICS_S stAwbStat[MAX_TEST_CASE] = {0};
	ISP_AF_STATISTICS_S stAFStat[MAX_TEST_CASE] = {0};
	ISP_STATISTICS_CFG_S stStatCfg, getStatCfg, orgStatCfg;
	ISP_DRC_ATTR_S stDRCAttr;
	CVI_CHAR fileName[64];
	ISP_CTRL_PARAM_S stCtrlParam = {0};

	// Current only test modify some param.
	const CVI_U32 awbWinThrTest[MAX_TEST_CASE][2] = { {0, 4095}, {2000, 3000} };
	const CVI_U8 afWinNumTest[MAX_TEST_CASE][2] = {{17, 15}, {8, 6} };

	//judge af sts isn't get
	CVI_ISP_GetCtrlParam(0, &stCtrlParam);
	stCtrlParam.u32AFStatIntvl = 1;
	CVI_ISP_SetCtrlParam(0, &stCtrlParam);

	// First close AE / AWB RLSC compensate.
	CVI_ISP_GetDRCAttr(0, &stDRCAttr);
	stDRCAttr.DRCMu[ISP_TEST_PARAM_AE_LSCR_ENABLE] = stDRCAttr.DRCMu[ISP_TEST_PARAM_AWB_LSCR_ENABLE] = 0;
	CVI_ISP_SetDRCAttr(0, &stDRCAttr);

	CVI_ISP_GetStatisticsConfig(0, &orgStatCfg);
	for (i = 0; i < MAX_TEST_CASE; i++) {
		// Modify statistic setting.
		CVI_ISP_GetStatisticsConfig(0, &stStatCfg);
		stStatCfg.stFocusCfg.stConfig.u16Hwnd = afWinNumTest[i][0];
		stStatCfg.stFocusCfg.stConfig.u16Vwnd = afWinNumTest[i][1];
		stStatCfg.stWBCfg.u16BlackLevel = awbWinThrTest[i][0];
		stStatCfg.stWBCfg.u16WhiteLevel = awbWinThrTest[i][1];
		CVI_ISP_SetStatisticsConfig(0, &stStatCfg);
		// wait some frame for setting apply.
		isp_test_apply_wait();
		// ae
		// ae statistics sometimes a little different. Still not find root cause, Close first
		CVI_ISP_GetAEStatistics(0, &stAEStat[i]);
		sprintf(fileName, "%s%s%d%s", RESULT_PATH, "aeSts_", i, EXTENSION_FILE_TYPE);
		s32Ret = isp_test_answer_compare(fileName, &stAEStat[i], sizeof(ISP_AE_STATISTICS_S), AAA_TYPE_AE);
		finalRet |= s32Ret;
		if (s32Ret != 0) {
			printf("AE Statistic check NG\n");
			sprintf(fileName, "%s%s%d%s", RESULT_PATH, "aeSts_ng", i, EXTENSION_FILE_TYPE);
			isp_test_result_save(fileName, &stAEStat, sizeof(ISP_AE_STATISTICS_S));
			s32Ret = CVI_FAILURE;
		}

		// awb
		CVI_ISP_GetWBStatistics(0, &stAwbStat[i]);
		sprintf(fileName, "%s%s%d%s", RESULT_PATH, "awbSts_", i, EXTENSION_FILE_TYPE);
		s32Ret = isp_test_answer_compare(fileName, &stAwbStat[i], sizeof(ISP_WB_STATISTICS_S), AAA_TYPE_AWB);
		finalRet |= s32Ret;
		if (s32Ret != 0) {
			printf("AWB Statistic check NG\n");
			sprintf(fileName, "%s%s%d%s", RESULT_PATH, "awbSts_ng", i, EXTENSION_FILE_TYPE);
			isp_test_result_save(fileName, &stAwbStat, sizeof(ISP_WB_STATISTICS_S));
		}
		// af
		if (stCtrlParam.u32AFStatIntvl == 0) {
			s32Ret = CVI_SUCCESS;
			printf("AF sts are not available in current software, so not compare\n");
		} else {
			CVI_ISP_GetFocusStatistics(0, &stAFStat[i]);
			sprintf(fileName, "%s%s%d%s", RESULT_PATH, "afSts_", i, EXTENSION_FILE_TYPE)
			s32Ret = isp_test_answer_compare(fileName, &stAFStat[i],
							sizeof(ISP_AF_STATISTICS_S), AAA_TYPE_AF);
			finalRet |= s32Ret;
			if (s32Ret != 0) {
				printf("AF Statistic check NG\n");
				sprintf(fileName, "%s%s%d%s", RESULT_PATH, "afSts_ng", i, EXTENSION_FILE_TYPE);
				isp_test_result_save(fileName, &stAFStat[i], sizeof(ISP_AF_STATISTICS_S));
			}
			// Get statistic for compare.
			CVI_ISP_GetStatisticsConfig(0, &getStatCfg);
			if (memcmp(&getStatCfg, &stStatCfg, sizeof(ISP_STATISTICS_CFG_S)) != 0) {
				printf("Statistic Cfg get NG\n");
				s32Ret = CVI_FAILURE;
			}
		}
		finalRet |= s32Ret;
	}

	CVI_ISP_SetStatisticsConfig(0, &orgStatCfg);
	return finalRet;
#else
	return CVI_SUCCESS;
#endif
}

// Module control test.
// Use module test API set all module close.
// Compare with close all yuv to check close all action correct or not.
// return to original condition.
CVI_S32 isp_module_ctrl_test(CVI_VOID)
{
	return CVI_SUCCESS; //TODO@ST Fix auto test

	ISP_MODULE_CTRL_U setModCtrl, getModCtrl, orgModeCtrl;
//	ISP_YCONTRAST_ATTR_S yContrastAttr = {0};
	CVI_S32 s32Ret = CVI_SUCCESS;
	//CVI_U8 *yuv = CVI_NULL;
	//CVI_U32 yuvSize = 0;
	//CVI_CHAR fileName[64];
	VI_PIPE_ATTR_S stPipeAttr = {0};

	// 0. judge compress isn't open
	CVI_VI_GetPipeAttr(0, &stPipeAttr);

	// 1. Get current module control status.
	CVI_ISP_GetModuleControl(0, &orgModeCtrl);
	setModCtrl.u64Key = getModCtrl.u64Key = 0;
	// 2. Test module Set all module bypass.
	setModCtrl.bitBypassBlc = 1;
	setModCtrl.bitBypassRlsc = 1;
	setModCtrl.bitBypassFpn = 1;
	setModCtrl.bitBypassDpc = 1;
	setModCtrl.bitBypassCrosstalk = 1;
	setModCtrl.bitBypassWBGain = 1;
	setModCtrl.bitBypassDis = 1;
	setModCtrl.bitBypassBnr = 1;
	setModCtrl.bitBypassDemosaic = 1;
	setModCtrl.bitBypassRgbcac = 1;
	setModCtrl.bitBypassMlsc = 1;
	setModCtrl.bitBypassCcm = 1;
	setModCtrl.bitBypassFusion = 1;
	setModCtrl.bitBypassDrc = 1;
	setModCtrl.bitBypassGamma = 1;
	setModCtrl.bitBypassDehaze = 1;
	setModCtrl.bitBypassClut = 1;
	setModCtrl.bitBypassCsc = 1;
	setModCtrl.bitBypassDci = 1;
	setModCtrl.bitBypassCa = 1;
	setModCtrl.bitBypassPreyee = 1;
	setModCtrl.bitBypassMotion = 1;
	setModCtrl.bitBypass3dnr = 1;
	setModCtrl.bitBypassYnr = 1;
	setModCtrl.bitBypassCnr = 1;
	setModCtrl.bitBypassCac = 1;
	setModCtrl.bitBypassCa2 = 1;
	setModCtrl.bitBypassYee = 1;
	setModCtrl.bitBypassYcontrast = 1;
	setModCtrl.bitBypassMono = 1;

	// 3. Set Module test API and check with result YUV match answer or not.
//	CVI_ISP_SetYContrastAttr(0, &yContrastAttr);
	CVI_ISP_SetModuleControl(0, &setModCtrl);
	isp_test_apply_wait();
	//We decouple auto test from the camera
	//The YUV size of each sensor is different, Not to confirm YUV
	/*
	if (stPipeAttr.enCompressMode == COMPRESS_MODE_TILE) {
		sprintf(fileName, "%s%s.yuv", RESULT_PATH, "moduletest_dpcm");
	} else {
		sprintf(fileName, "%s%s.yuv", RESULT_PATH, "moduletest");
	}
	isp_test_yuv_get(0, &yuv, &yuvSize);

	s32Ret = isp_test_answer_compare(fileName, yuv, yuvSize);
	if (s32Ret != 0) {
		printf("Modultest close all NG. yuvSize %d\n", yuvSize);
		if (stPipeAttr.enCompressMode == COMPRESS_MODE_TILE) {
			sprintf(fileName, "%s%s.yuv", RESULT_PATH, "moduletest_dpcm_ng");
		} else {
			sprintf(fileName, "%s%s.yuv", RESULT_PATH, "moduletest_ng");
		}
		isp_test_result_save(fileName, yuv, yuvSize);
		s32Ret = CVI_FAILURE;
	}
	*/
	// 3.1. Check Get module API correct or not.
	CVI_ISP_GetModuleControl(0, &getModCtrl);
	if (memcmp(&getModCtrl.u64Key, &setModCtrl.u64Key, sizeof(setModCtrl.u64Key))) {
		s32Ret = CVI_FAILURE;
	}
	// Restore
	// 4. Reset to original Module Control setting.
	CVI_ISP_SetModuleControl(0, &orgModeCtrl);
	isp_test_apply_wait();
	//ISP_TEST_RELEASE_MEMORY(yuv);

	return s32Ret;
}

// fw state test.
// Use fmw state API to set freeze/Run case.
// set blc setting at run/freeze state and check final status..
CVI_S32 isp_fw_state_test(CVI_VOID)
{
#if 0 //TODO@ST Fix auto test
	ISP_FMW_STATE_E setFwState = ISP_FMW_STATE_RUN;
	ISP_FMW_STATE_E getFwState = ISP_FMW_STATE_RUN;
	ISP_BLACKLEVEL_ATTR_S tmpBlcAttr = {0}, orgBlcAttr = {0};
	ISP_INNER_STATE_INFO_S innerInfo;
	CVI_U32 blcROffset, blcGrOffset, blcGbOffset, blcBOffset;
	CVI_S32 s32Ret = CVI_SUCCESS;

	// 1. Get original BLC setting.
	CVI_ISP_GetBlackLevelAttr(0, &orgBlcAttr);
	for (; setFwState < ISP_FMW_STATE_BUTT; setFwState++) {
		// 2. Start to set fw state value.
		CVI_ISP_SetFMWState(0, setFwState);
		// 3. start to modify blc status.
		CVI_ISP_GetBlackLevelAttr(0, &tmpBlcAttr);
		blcROffset = tmpBlcAttr.stAuto.OffsetR[0] += 20;
		blcGrOffset = tmpBlcAttr.stAuto.OffsetGr[0] += 20;
		blcGbOffset = tmpBlcAttr.stAuto.OffsetGb[0] += 20;
		blcBOffset = tmpBlcAttr.stAuto.OffsetB[0] += 20;
		CVI_ISP_SetBlackLevelAttr(0, &tmpBlcAttr);
		isp_test_apply_wait();
		// 3.1 Check current blc value status.
		CVI_ISP_QueryInnerStateInfo(0, &innerInfo);
		if (setFwState == ISP_FMW_STATE_RUN) {
			// 3.2 If state is run. But Blc value is not the same with set value.
			// means flow is freeze. Return Error.
			if ((innerInfo.blcOffsetR != blcROffset || innerInfo.blcOffsetGr != blcGrOffset)
				|| (innerInfo.blcOffsetGb != blcGbOffset || innerInfo.blcOffsetB != blcBOffset)) {
				printf("FMW Run State Fail.\n");
				s32Ret = CVI_FAILURE;
			}
		} else {
			// 3.2 If state is freeze. But Blc value is the same with set value.
			// means flow is still run. Return Error.
			if ((innerInfo.blcOffsetR == blcROffset || innerInfo.blcOffsetGr == blcGrOffset)
				|| (innerInfo.blcOffsetGb == blcGbOffset || innerInfo.blcOffsetB == blcBOffset)) {
				printf("FMW freeze State Fail.\n");
				s32Ret = CVI_FAILURE;
			}
		}
		// 4. Get fmw status to check get is correct.
		CVI_ISP_GetFMWState(0, &getFwState);
		if (getFwState != setFwState) {
			printf("Get freeze State Fail.\n");
			s32Ret = CVI_FAILURE;
		}
	}
	// Restore
	// 5. Restore BLC and fw state to original setting.
	CVI_ISP_SetFMWState(0, ISP_FMW_STATE_RUN);
	CVI_ISP_SetBlackLevelAttr(0, &orgBlcAttr);
	isp_test_apply_wait();
	return s32Ret;
#else
	return CVI_SUCCESS;
#endif
}

CVI_U32 aeBindTest[MAX_REGISTER_ALG_LIB_NUM] = {0};
CVI_S32 aeTestRun1(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo,
	   ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv)
{
	aeBindTest[1] = 1;
	pstAeResult->u32Again = pstAeResult->u32Dgain = pstAeResult->u32IspDgain = 1024;
	pstAeResult->u32Iso = 100;
	pstAeResult->enFSWDRMode = ISP_FSWDR_NORMAL_MODE;
	pstAeResult->u32ExpRatio = 64;
	pstAeResult->s16CurrentLV = 100;
	pstAeResult->u8MeterFramePeriod = 5;

	UNUSED(ViPipe);
	UNUSED(pstAeInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

CVI_S32 aeTestRun2(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo,
	   ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv)
{
	aeBindTest[2] = 1;
	pstAeResult->u32Again = pstAeResult->u32Dgain = pstAeResult->u32IspDgain = 1024;
	pstAeResult->u32Iso = 100;
	pstAeResult->enFSWDRMode = ISP_FSWDR_NORMAL_MODE;
	pstAeResult->u32ExpRatio = 64;
	pstAeResult->s16CurrentLV = 100;
	pstAeResult->u8MeterFramePeriod = 5;

	UNUSED(ViPipe);
	UNUSED(pstAeInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

CVI_S32 aeTestRun3(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo,
	   ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv)
{
	aeBindTest[3] = 1;
	pstAeResult->u32Again = pstAeResult->u32Dgain = pstAeResult->u32IspDgain = 1024;
	pstAeResult->u32Iso = 100;
	pstAeResult->enFSWDRMode = ISP_FSWDR_NORMAL_MODE;
	pstAeResult->u32ExpRatio = 64;
	pstAeResult->s16CurrentLV = 100;
	pstAeResult->u8MeterFramePeriod = 5;

	UNUSED(ViPipe);
	UNUSED(pstAeInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

ISP_AE_REGISTER_S aeBindTestRegFunc[MAX_REGISTER_ALG_LIB_NUM] = {
	[0] = {
		.stAeExpFunc.pfn_ae_init = CVI_NULL,
		.stAeExpFunc.pfn_ae_run = CVI_NULL,
		.stAeExpFunc.pfn_ae_ctrl = CVI_NULL,
		.stAeExpFunc.pfn_ae_exit = CVI_NULL,
	},
	[1] = {
		.stAeExpFunc.pfn_ae_init = CVI_NULL,
		.stAeExpFunc.pfn_ae_run = aeTestRun1,
		.stAeExpFunc.pfn_ae_ctrl = CVI_NULL,
		.stAeExpFunc.pfn_ae_exit = CVI_NULL,
	},
	[2] = {
		.stAeExpFunc.pfn_ae_init = CVI_NULL,
		.stAeExpFunc.pfn_ae_run = aeTestRun2,
		.stAeExpFunc.pfn_ae_ctrl = CVI_NULL,
		.stAeExpFunc.pfn_ae_exit = CVI_NULL,
	},
	[3] = {
		.stAeExpFunc.pfn_ae_init = CVI_NULL,
		.stAeExpFunc.pfn_ae_run = aeTestRun3,
		.stAeExpFunc.pfn_ae_ctrl = CVI_NULL,
		.stAeExpFunc.pfn_ae_exit = CVI_NULL,
	}
};

CVI_U32 awbBindTest[MAX_REGISTER_ALG_LIB_NUM] = {0};
CVI_S32 awbTestRun1(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo,
	   ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv)
{
	awbBindTest[1] = 1;
	pstAwbResult->au32WhiteBalanceGain[0] = pstAwbResult->au32WhiteBalanceGain[1] =
		pstAwbResult->au32WhiteBalanceGain[2] = pstAwbResult->au32WhiteBalanceGain[3] = 1024;
	pstAwbResult->u32ColorTemp = 6500;

	UNUSED(ViPipe);
	UNUSED(pstAwbInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

CVI_S32 awbTestRun2(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo,
	   ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv)
{
	awbBindTest[2] = 1;
	pstAwbResult->au32WhiteBalanceGain[0] = pstAwbResult->au32WhiteBalanceGain[1] =
		pstAwbResult->au32WhiteBalanceGain[2] = pstAwbResult->au32WhiteBalanceGain[3] = 1024;
	pstAwbResult->u32ColorTemp = 6500;

	UNUSED(ViPipe);
	UNUSED(pstAwbInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

CVI_S32 awbTestRun3(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo,
	   ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv)
{
	awbBindTest[3] = 1;
	pstAwbResult->au32WhiteBalanceGain[0] = pstAwbResult->au32WhiteBalanceGain[1] =
		pstAwbResult->au32WhiteBalanceGain[2] = pstAwbResult->au32WhiteBalanceGain[3] = 1024;
	pstAwbResult->u32ColorTemp = 6500;

	UNUSED(ViPipe);
	UNUSED(pstAwbInfo);
	UNUSED(s32Rsv);
	return CVI_SUCCESS;
}

ISP_AWB_REGISTER_S awbBindTestRegFunc[MAX_REGISTER_ALG_LIB_NUM] = {
	[0] = {
		.stAwbExpFunc.pfn_awb_init = CVI_NULL,
		.stAwbExpFunc.pfn_awb_run = CVI_NULL,
		.stAwbExpFunc.pfn_awb_ctrl = CVI_NULL,
		.stAwbExpFunc.pfn_awb_exit = CVI_NULL,
	},
	[1] = {
		.stAwbExpFunc.pfn_awb_init = CVI_NULL,
		.stAwbExpFunc.pfn_awb_run = awbTestRun1,
		.stAwbExpFunc.pfn_awb_ctrl = CVI_NULL,
		.stAwbExpFunc.pfn_awb_exit = CVI_NULL,
	},
	[2] = {
		.stAwbExpFunc.pfn_awb_init = CVI_NULL,
		.stAwbExpFunc.pfn_awb_run = awbTestRun2,
		.stAwbExpFunc.pfn_awb_ctrl = CVI_NULL,
		.stAwbExpFunc.pfn_awb_exit = CVI_NULL,
	},
	[3] = {
		.stAwbExpFunc.pfn_awb_init = CVI_NULL,
		.stAwbExpFunc.pfn_awb_run = awbTestRun3,
		.stAwbExpFunc.pfn_awb_ctrl = CVI_NULL,
		.stAwbExpFunc.pfn_awb_exit = CVI_NULL,
	}
};
// 3a lib bind test.
// Use bind attr API to check bind lib is correct or not.
// Register some algo and check 3a lib run correct algo function or not.
// return to original condition.
CVI_S32 isp_aaa_bind_test(CVI_VOID)
{
	return CVI_SUCCESS; //TODO@ST Fix auto test

#define CVI_LIB_NAME "cvi_lib_"
#define LIB_NAME(fileName, libNum, libName) sprintf(fileName, "%s%s_%d", CVI_LIB_NAME, libName, libNum)
	CVI_S32 s32Ret;
	VI_PIPE ViPipe = 0;
	ALG_LIB_S algLib[AAA_TYPE_MAX][MAX_REGISTER_ALG_LIB_NUM + 1];

	ISP_BIND_ATTR_S stBindAttr, stGetBindAttr, stOrgBindAttr;
	AAA_LIB_TYPE_E type;
	const CVI_CHAR algoName[AAA_TYPE_MAX][10] = {"ae", "awb", "af"};

	stBindAttr.sensorId = 0;
	// 1. Get original registered function.
	CVI_ISP_GetBindAttr(ViPipe, &stOrgBindAttr);
	// 2. register some aaa library.
	for (CVI_U32 i = 1; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		for (type = AAA_TYPE_AE; type < AAA_TYPE_AF; type++) {
			memset(&algLib[type][i], 0, sizeof(ALG_LIB_S));
			algLib[type][i].s32Id = ViPipe;
			LIB_NAME(algLib[type][i].acLibName, i, algoName[type]);
		}
		CVI_ISP_AELibRegCallBack(ViPipe, &algLib[AAA_TYPE_AE][i], &aeBindTestRegFunc[i]);
		CVI_ISP_AWBLibRegCallBack(ViPipe, &algLib[AAA_TYPE_AWB][i], &awbBindTestRegFunc[i]);
	}
	// 3. Change bind aaa lib and check 3a run at correct function.
	for (CVI_U32 i = 1; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		memset(&stBindAttr, 0, sizeof(ISP_BIND_ATTR_S));
		memset(&stGetBindAttr, 0, sizeof(ISP_BIND_ATTR_S));

		snprintf(stBindAttr.stAeLib.acLibName, sizeof(algLib[AAA_TYPE_AE][i].acLibName), "%s",
				algLib[AAA_TYPE_AE][i].acLibName);
		stBindAttr.stAeLib.s32Id = ViPipe;
		snprintf(stBindAttr.stAwbLib.acLibName, sizeof(algLib[AAA_TYPE_AWB][i].acLibName), "%s",
				algLib[AAA_TYPE_AWB][i].acLibName);
		stBindAttr.stAwbLib.s32Id = ViPipe;
		// 3.1 change bind attr here.
		s32Ret = CVI_ISP_SetBindAttr(ViPipe, &stBindAttr);
		isp_test_apply_wait();
		if (s32Ret != CVI_SUCCESS) {
			printf("Bind Algo failed with %#x!\n", s32Ret);
			s32Ret = CVI_FAILURE;
		}
		// 3.2 Check get bind attr is the same or not.
		CVI_ISP_GetBindAttr(ViPipe, &stGetBindAttr);
		if (memcmp(&stGetBindAttr, &stBindAttr, sizeof(ISP_BIND_ATTR_S)) != 0) {
			printf("%d Get Bind Algo Compare failed %#x!\n", i, s32Ret);
			hexDump(&stBindAttr, sizeof(ISP_BIND_ATTR_S));
			hexDump(&stGetBindAttr, sizeof(ISP_BIND_ATTR_S));
			s32Ret = CVI_FAILURE;
		}
		// 3.3 Check excecute correct function or not.
		if (aeBindTest[i] == 0) {
			printf("ae %d Bind Algo failed %#x!\n", i, aeBindTest[i]);
			s32Ret = CVI_FAILURE;
		}
		if (awbBindTest[i] == 0) {
			printf("awb %d Bind Algo failed %#x!\n", i, awbBindTest[i]);
			s32Ret = CVI_FAILURE;
		}
	}
	// Restore
	// 4. Set bind to original setting.
	CVI_ISP_SetBindAttr(ViPipe, &stOrgBindAttr);
	isp_test_apply_wait();
	// 5. unregistered 3a lib.
	for (CVI_U32 i = 1; i < MAX_REGISTER_ALG_LIB_NUM; i++) {
		CVI_ISP_AELibUnRegCallBack(ViPipe, &algLib[AAA_TYPE_AE][i]);
		CVI_ISP_AWBLibUnRegCallBack(ViPipe, &algLib[AAA_TYPE_AWB][i]);
	}

	return s32Ret;
}

// ret set test.
// Use set register to check reg control is correct or not.
// Set 3 registers to check.
// Reset to original setting when.
CVI_S32 isp_reg_set_test(CVI_VOID)
{
#if 0 //TODO@ST Fix auto test
#define REG_MAX_TEST_CASE 3
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE ViPipe = 0;
	CVI_U32 orgValue, getValue, setValue;
	// test case. [YNR slope. LTM slope, AF offset]
	CVI_U32 regAddr[REG_MAX_TEST_CASE] = {0xA057004, 0xA039028, 0xA00C03C};
	// 1. stop flow for test write register.
	CVI_ISP_SetFMWState(0, ISP_FMW_STATE_FREEZE);

	for (CVI_U32 i = 0; i < REG_MAX_TEST_CASE; i++) {
		// 1. Get original register value.
		CVI_ISP_GetRegister(ViPipe, regAddr[i], &orgValue);
		setValue = orgValue + 5;
		// 2. Modify original value and set to register. Then get once again to check.
		CVI_ISP_SetRegister(ViPipe, regAddr[i], setValue);
		isp_test_apply_wait();
		// 3. Get register to check reg be changed or not.
		CVI_ISP_GetRegister(ViPipe, regAddr[i], &getValue);

		if (getValue != setValue || getValue == orgValue) {
			printf("Set/get register error orgValue %d %d %d\n",
				orgValue, setValue, getValue);
			s32Ret = CVI_FAILURE;
		}
		// 4. Set to original value.
		CVI_ISP_SetRegister(ViPipe, regAddr[i], orgValue);
	}
	// 5. set flow to run again..
	CVI_ISP_SetFMWState(0, ISP_FMW_STATE_RUN);
	return s32Ret;
#else
	return CVI_SUCCESS;
#endif
}

CVI_S32 isp_raw_replay_test(CVI_VOID)
{
	return CVI_SUCCESS;
}

CVI_VOID isp_gpio_ctrl(CVI_VOID)
{
	uint32_t gpio_num = 0, gpio_val = 0;

	printf("GPIO num (404~511):\n");
	scanf("%d", &gpio_num);

	printf("0) LOW, 1) HIGH\n");
	scanf("%d", &gpio_val);

	bool is_gpio_num_valid = ((gpio_num >= 404) && (gpio_num <= 511));
	bool is_gpio_val_valid = (gpio_val <= 1);

	if ((!is_gpio_num_valid) || (!is_gpio_val_valid)) {
		printf("Wrong input num(%d), val(%d)\n", gpio_num, gpio_val);
		goto GPIO_EXIT;
	}

	isp_peri_gpio_set(gpio_num, gpio_val);

GPIO_EXIT:
	return;
}

CVI_S32 isp_test_set_debug_level(CVI_VOID)
{
	int level = 0;

	printf("set isp debug level:\n");
	printf("	LOG_EMERG	0	/* system is unusable */\n");
	printf("	LOG_ALERT	1	/* action must be taken immediately */\n");
	printf("	LOG_CRIT	2	/* critical conditions */\n");
	printf("	LOG_ERR		3	/* error conditions */\n");
	printf("	LOG_WARNING	4	/* warning conditions */\n");
	printf("	LOG_NOTICE	5	/* normal but significant condition */\n");
	printf("	LOG_INFO	6	/* informational */\n");
	printf("	LOG_DEBUG	7	/* debug-level messages */\n");

	scanf("%d", &level);
	printf("isp log level=%d\n", level);
	CVI_DEBUG_SetDebugLevel(level);
	return 0;
}

struct isp_test_ops isp_api_test = {
	.statistic_test = isp_statistic_test,
	.module_ctrl_test = isp_module_ctrl_test,
	.fw_state_test = isp_fw_state_test,
	.aaa_bind_test = isp_aaa_bind_test,
	.reg_set_test = isp_reg_set_test,
	.dcf_info_test = isp_reg_set_test, //TODO@ST Fix auto test
	.raw_replay_test = isp_raw_replay_test,
};

// static CVI_S32 CVI_ISP_PrintBlackLevelAttr(const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr);

CVI_VOID isp_test_mpi_test(CVI_VOID)
{
	printf("%s\n", __func__);

	CVI_U32 id = 0;
	CVI_U32 ViPipe = 0;
	CVI_U32 max, step;

	switch (id) {
	case 0: {
		ISP_BLACK_LEVEL_ATTR_S attr_back_up;

		CVI_ISP_GetBlackLevelAttr(ViPipe, &attr_back_up);

		ISP_BLACK_LEVEL_ATTR_S attr = {0};

		max = 4095;
		step = max >> 7;
		for (CVI_U32 i = 0 ; i < max ; i += step) {
			CVI_ISP_GetBlackLevelAttr(ViPipe, &attr);
			attr.enOpType = 1;
			attr.stManual.OffsetR = i;
			attr.stManual.OffsetGr = i;
			attr.stManual.OffsetGb = i;
			attr.stManual.OffsetB = i;
			attr.stManual.OffsetR2 = i;
			attr.stManual.OffsetGr2 = i;
			attr.stManual.OffsetGb2 = i;
			attr.stManual.OffsetB2 = i;
			CVI_ISP_SetBlackLevelAttr(ViPipe, &attr);
			// mdelay(40);
			usleep(40 * 1000);
		}

		CVI_ISP_SetBlackLevelAttr(ViPipe, &attr_back_up);

#if 0
		CVI_U32 str_size = offsetof(ISP_BLACK_LEVEL_ATTR_S, stAuto) / 4;

		for (CVI_U32 i = 0 ; i < str_size ; i++) {
			ptr = (((CVI_U32 *)&attr)+i);

			printf("0x%08x, ", *ptr);
			if (!((i+1) % 5))
				printf("\n");
		}
		printf("\n");

		CVI_U32 reg_size = sizeof(struct _ISP_REG_ISP_BLC_T);
		struct _ISP_REG_ISP_BLC_T *vaddr = CVI_SYS_Mmap(0x0A012000, reg_size);

		for (CVI_U32 i = 0 ; i < reg_size / 4; i++) {
			ptr = (((CVI_U32 *)vaddr)+i);

			printf("0x%08x, ", *ptr);
			if (!((i+1) % 5))
				printf("\n");
		}
		printf("\n");

		CVI_SYS_Munmap(vaddr, sizeof(sizeof(struct _ISP_REG_ISP_BLC_T)));
#endif
		break;
	}
	}
}
