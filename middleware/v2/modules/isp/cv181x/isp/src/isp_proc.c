/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_proc.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "peri.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_proc_local.h"
#include "isp_version.h"
#include "cvi_comm_3a.h"
#include "isp_ioctl.h"
#include "pthread.h"

#ifdef ENABLE_ISP_PROC_DEBUG
ISP_DEBUGINFO_PROC_S *g_pIspTempProcST[VI_MAX_PIPE_NUM] = { NULL };

static ISP_DEBUGINFO_PROC_S *g_pIspProcST[VI_MAX_PIPE_NUM] = { NULL };
static CVI_CHAR *g_pIspProcBuffer = { NULL };
static ISP_PROC_STATUS_E g_eProcStatus;
static CVI_BOOL g_thread_run;
static CVI_BOOL g_isProcRun;
static CVI_BOOL g_bIsOpenPipe[VI_MAX_PIPE_NUM];
static ISP_GetAAAVer_FUNC g_pfn_getVer[VI_MAX_PIPE_NUM][ISP_AAA_TYPE_NUM] = { NULL };
static pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond;

#define CHECK_FUNC_RET(ret) do { \
	if ((ret) == -1) { \
		return -1; \
	} \
} while (0)
#define UpdateValue(buffer, bufferSize, TotalSize, writeSize) do { \
	(buffer) = (buffer) + (writeSize); \
	(bufferSize) = (bufferSize) - (writeSize); \
	(TotalSize) = (TotalSize) + (writeSize); \
	if (writeSize > bufferSize) { \
		ISP_LOG_ERR("bufferSize[%d] < writeSize[%d]\n", \
			bufferSize, writeSize); \
		return -1; \
	} \
} while (0)
#define PROCBUFFERSIZE 300000
#define DEBUGMODE 0
#if DEBUGMODE
#define PROCFILE "/mnt/data/isp_debugInfo"
#else
#define PROCFILE "/proc/cvitek/isp"
#endif

static CVI_S32 apply_proc_level_from_environment(VI_PIPE ViPipe)
{
#define ENV_PROC_LEVEL		"PROC_LEVEL"
	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;

	ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);
	char *pszEnv = getenv(ENV_PROC_LEVEL);
	char *pszEnd = NULL;

	if (pszEnv == NULL) {
		pstIspProcCtrl->u32ProcLevel = PROC_LEVEL_0;
		return CVI_FAILURE;
	}

	CVI_UL Level = strtol(pszEnv, &pszEnd, 10);

	if ((Level > PROC_LEVEL_3) || (pszEnv == pszEnd)) {
		ISP_LOG_DEBUG("ENV_PROC_LEVEL invalid\n");
		pstIspProcCtrl->u32ProcLevel = PROC_LEVEL_0;
		return CVI_FAILURE;
	}

	pstIspProcCtrl->u32ProcLevel = (CVI_U32)Level;
	return CVI_SUCCESS;
}

static CVI_S32 apply_proc_param_from_environment(VI_PIPE ViPipe)
{
#define ENV_PROC_RATE "PROC_PARAM"
	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;

	ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);
	char *pszEnv = getenv(ENV_PROC_RATE);
	char *pszEnd = NULL;

	if (pszEnv == NULL) {
		pstIspProcCtrl->u32ProcParam = 30;
		return CVI_FAILURE;
	}

	CVI_UL param = strtol(pszEnv, &pszEnd, 10);

	if (pszEnv == pszEnd) {
		ISP_LOG_DEBUG("ENV_PROC_RATE invalid\n");
		pstIspProcCtrl->u32ProcParam = 30;
		return CVI_FAILURE;
	}

	pstIspProcCtrl->u32ProcParam = (CVI_U32)param;
	return CVI_SUCCESS;
}

static CVI_S32 _isp_writeToProc(CVI_CHAR *buffer, CVI_U32 bufferSize)
{
	VI_PIPE ViPipe = 0;
	struct isp_proc_cfg proc_cfg;

	proc_cfg.buffer = (void *)buffer;
	proc_cfg.buffer_size = (size_t)bufferSize;

	S_EXT_CTRLS_PTR(VI_IOCTL_SET_PROC_CONTENT, &proc_cfg);

	return 0;
}

static CVI_S32 _isp_formatPipeTitle(VI_PIPE ViPipe,
				CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	*actualSize = snprintf(buffer, bufferSize,
"------------------------------------------------------------  %s[%d]  ------------------------------\
------------------------------\n",
		"ISP PROC PIPE",
		ViPipe
	);
	if (*actualSize > bufferSize) {
		ISP_LOG_ERR("bufferSize[%d] < actualSize[%d]\n",
			bufferSize, *actualSize);
		return -1;
	}
	return 0;
}

static CVI_S32 _isp_formatModuleTitle(const CVI_CHAR *title,
				CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	*actualSize = snprintf(buffer, bufferSize,
"----|%-20s|----------------------------------------------------------------------------------------------\
--------------------\n",
		title
	);
	if (*actualSize > bufferSize) {
		ISP_LOG_ERR("bufferSize[%d] < actualSize[%d]\n",
			bufferSize, *actualSize);
		return -1;
	}
	return 0;
}

static CVI_S32 _isp_formatModuleVariable(const CVI_CHAR *VariableName,
					CVI_U32 VariableValue, CVI_BOOL isAddEndOfLine,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	writeSize = snprintf(buffer, bufferSize, "%-27s%-8d",
					VariableName, VariableValue);
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	if (isAddEndOfLine) {
		*buffer = '\n';
		UpdateValue(buffer, bufferSize, *actualSize, 1);
	}
	return 0;
}

static CVI_S32 _isp_formatModuleVariables(const CVI_CHAR **VariableName,
					CVI_U32 *VariableValue, CVI_U16 VariableCount,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	for (CVI_U16 i = 0; i < VariableCount; ++i) {
		writeSize = snprintf(buffer, bufferSize, "%-27s%-8d",
						VariableName[i], VariableValue[i]);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		if ((((i+1) % 4) == 0) || (i == (VariableCount - 1))) {
			*buffer = '\n';
			UpdateValue(buffer, bufferSize, *actualSize, 1);
		}
	}
	return 0;
}

static CVI_S32 _isp_formatModuleVariables_Neg(const CVI_CHAR **VariableName,
					CVI_S32 *VariableValue, CVI_U16 VariableCount,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	for (CVI_U16 i = 0; i < VariableCount; ++i) {
		writeSize = snprintf(buffer, bufferSize, "%-27s%-8d",
						VariableName[i], VariableValue[i]);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		if ((((i+1) % 4) == 0) || (i == (VariableCount - 1))) {
			*buffer = '\n';
			UpdateValue(buffer, bufferSize, *actualSize, 1);
		}
	}
	return 0;
}

static CVI_S32 _isp_formatModuleVariables_Float(const CVI_CHAR **VariableName,
					float *VariableValue, CVI_U16 VariableCount,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	for (CVI_U16 i = 0; i < VariableCount; ++i) {
		writeSize = snprintf(buffer, bufferSize, "%-27s%0.2f",
						VariableName[i], VariableValue[i]);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		if ((((i+1) % 4) == 0) || (i == (VariableCount - 1))) {
			*buffer = '\n';
			UpdateValue(buffer, bufferSize, *actualSize, 1);
		}
	}
	return 0;
}

static CVI_S32 _isp_formatModuleU8Array(CVI_U8 *Array, const CVI_CHAR *ArrayName,
					CVI_U16 ArraySize, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > ArraySize) {
		column = ArraySize;
	}
	if ((column == 0) || (column == ArraySize)) {
		writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%d ",
							Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if (i == (ArraySize - 1)) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
			}
		}
	} else {
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			if (i == 0) {
				writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			}
			writeSize = snprintf(buffer, bufferSize, "%d ",
						Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if ((((i+1) % column) == 0) || (i == (ArraySize - 1))) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
				if (i != (ArraySize - 1)) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
			}
		}
	}
	return 0;
}

static CVI_S32 _isp_formatModuleU16Array(CVI_U16 *Array, const CVI_CHAR *ArrayName,
					CVI_U16 ArraySize, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > ArraySize) {
		column = ArraySize;
	}
	if ((column == 0) || (column == ArraySize)) {
		writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%d ",
							Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if (i == (ArraySize - 1)) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
			}
		}
	} else {
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			if (i == 0) {
				writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			}
			writeSize = snprintf(buffer, bufferSize, "%d ",
						Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if ((((i+1) % column) == 0) || (i == (ArraySize - 1))) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
				if (i != (ArraySize - 1)) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
			}
		}
	}
	return 0;
}
#if 0
static CVI_S32 _isp_formatModuleU32Array(CVI_U32 *Array, const CVI_CHAR *ArrayName,
					CVI_U16 ArraySize, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > ArraySize) {
		column = ArraySize;
	}
	if ((column == 0) || (column == ArraySize)) {
		writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%d ",
							Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if (i == (ArraySize - 1)) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
			}
		}
	} else {
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			if (i == 0) {
				writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			}
			writeSize = snprintf(buffer, bufferSize, "%d ",
						Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if ((((i+1) % column) == 0) || (i == (ArraySize - 1))) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
				if (i != (ArraySize - 1)) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
			}
		}
	}
	return 0;
}
#endif //
static CVI_S32 _isp_formatModuleS16Array(CVI_S16 *Array, const CVI_CHAR *ArrayName,
					CVI_U16 ArraySize, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > ArraySize) {
		column = ArraySize;
	}
	if ((column == 0) || (column == ArraySize)) {
		writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%d ",
							Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if (i == (ArraySize - 1)) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
			}
		}
	} else {
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			if (i == 0) {
				writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			}
			writeSize = snprintf(buffer, bufferSize, "%d ",
						Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if ((((i+1) % column) == 0) || (i == (ArraySize - 1))) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
				if (i != (ArraySize - 1)) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
			}
		}
	}
	return 0;
}

#if 0
static CVI_S32 _isp_formatModuleS32Array(CVI_S32 *Array, const CVI_CHAR *ArrayName,
					CVI_U16 ArraySize, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > ArraySize) {
		column = ArraySize;
	}
	if ((column == 0) || (column == ArraySize)) {
		writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%d ",
							Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if (i == (ArraySize - 1)) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
			}
		}
	} else {
		for (CVI_U16 i = 0; i < ArraySize; ++i) {
			if (i == 0) {
				writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			}
			writeSize = snprintf(buffer, bufferSize, "%d ",
						Array[i]);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			if ((((i+1) % column) == 0) || (i == (ArraySize - 1))) {
				*buffer = '\n';
				UpdateValue(buffer, bufferSize, *actualSize, 1);
				if (i != (ArraySize - 1)) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
						ArrayName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
			}
		}
	}
	return 0;
}
#endif //
#if 0
static CVI_S32 _isp_formatModuleU8Matrix(CVI_U8 *Matrix, const CVI_CHAR *MatrixName,
					CVI_U16 MatrixRow, CVI_U16 MatrixColumn, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > MatrixColumn) {
		column = MatrixColumn;
	}
	if ((column == 0) || (column == MatrixColumn)) {
		for (CVI_U16 i = 0; i < MatrixRow; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			for (CVI_U16 j = 0; j < MatrixColumn; ++j) {
				writeSize = snprintf(buffer, bufferSize, "%d ",
								(Matrix+i)[j]);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				if (j == (MatrixColumn - 1)) {
					*buffer = '\n';
					UpdateValue(buffer, bufferSize, *actualSize, 1);
				}
			}
		}
	} else {
		for (CVI_U16 i = 0; i < MatrixRow; ++i) {
			for (CVI_U16 j = 0; j < MatrixColumn; ++j) {
				if (j == 0) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
				writeSize = snprintf(buffer, bufferSize, "%d ",
							(Matrix+i)[j]);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				if ((((j+1) % column) == 0) || (j == (MatrixColumn - 1))) {
					*buffer = '\n';
					UpdateValue(buffer, bufferSize, *actualSize, 1);
					if (j != (MatrixColumn - 1)) {
						writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
						UpdateValue(buffer, bufferSize, *actualSize, writeSize);
					}
				}
			}
		}
	}
	return 0;
}
#endif //
#if 0
static CVI_S32 _isp_formatModuleU16Matrix(CVI_U16 *Matrix, const CVI_CHAR *MatrixName,
					CVI_U16 MatrixRow, CVI_U16 MatrixColumn, CVI_U16 column,
					CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	CVI_S32 writeSize = 0;
	*actualSize = 0;

	// column value
	//  0:auto
	// >0:How many columns are printed per row
	if (column > MatrixColumn) {
		column = MatrixColumn;
	}
	if ((column == 0) || (column == MatrixColumn)) {
		for (CVI_U16 i = 0; i < MatrixRow; ++i) {
			writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			for (CVI_U16 j = 0; j < MatrixColumn; ++j) {
				writeSize = snprintf(buffer, bufferSize, "%d ",
								(Matrix+i)[j]);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				if (j == (MatrixColumn - 1)) {
					*buffer = '\n';
					UpdateValue(buffer, bufferSize, *actualSize, 1);
				}
			}
		}
	} else {
		for (CVI_U16 i = 0; i < MatrixRow; ++i) {
			for (CVI_U16 j = 0; j < MatrixColumn; ++j) {
				if (j == 0) {
					writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
					UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				}
				writeSize = snprintf(buffer, bufferSize, "%d ",
							(Matrix+i)[j]);
				UpdateValue(buffer, bufferSize, *actualSize, writeSize);
				if ((((j+1) % column) == 0) || (j == (MatrixColumn - 1))) {
					*buffer = '\n';
					UpdateValue(buffer, bufferSize, *actualSize, 1);
					if (j != (MatrixColumn - 1)) {
						writeSize = snprintf(buffer, bufferSize, "%-27s",
							MatrixName);
						UpdateValue(buffer, bufferSize, *actualSize, writeSize);
					}
				}
			}
		}
	}
	return 0;
}
#endif //

CVI_S32 isp_proc_buf_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("g_pIspTempProcST[%d]:%u g_pIspProcST[%d]:%u\n",
			ViPipe, (CVI_U32)sizeof(ISP_DEBUGINFO_PROC_S),
			ViPipe, (CVI_U32)sizeof(ISP_DEBUGINFO_PROC_S));
	g_pIspTempProcST[ViPipe] = ISP_CALLOC(1, sizeof(ISP_DEBUGINFO_PROC_S));
	if (g_pIspTempProcST[ViPipe] == NULL) {
		ISP_LOG_ERR("can't allocate g_pIspTempProcST[%d], size=%u\n",
			ViPipe, (CVI_U32)sizeof(ISP_DEBUGINFO_PROC_S));
		return -1;
	}
	g_pIspProcST[ViPipe] = ISP_CALLOC(1, sizeof(ISP_DEBUGINFO_PROC_S));
	if (g_pIspProcST[ViPipe] == NULL) {
		ISP_LOG_ERR("can't allocate g_pIspProcST[%d], size=%u\n",
			ViPipe, (CVI_U32)sizeof(ISP_DEBUGINFO_PROC_S));
		return -1;
	}

	apply_proc_param_from_environment(ViPipe);
	apply_proc_level_from_environment(ViPipe);
	g_bIsOpenPipe[ViPipe] = true;
	return 0;
}

CVI_S32 isp_proc_buf_deinit(VI_PIPE ViPipe)
{
	ISP_RELEASE_MEMORY(g_pIspTempProcST[ViPipe]);
	ISP_RELEASE_MEMORY(g_pIspProcST[ViPipe]);
	g_bIsOpenPipe[ViPipe] = false;
	return 0;
}

CVI_S32 isp_proc_setProcStatus(ISP_PROC_STATUS_E eStatus)
{
	ISP_LOG_DEBUG("%d\n", eStatus);

	struct timeval t;

	gettimeofday(&t, NULL);
	ISP_LOG_NOTICE("%d->time:%f\n", eStatus, t.tv_sec + t.tv_usec/1000000.0);
	pthread_mutex_lock(&m_mutex);
	g_eProcStatus = eStatus;
	if (eStatus == PROC_STATUS_UPDATEDATA) {
		pthread_mutex_lock(&cond_mutex);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&cond_mutex);
	}
	pthread_mutex_unlock(&m_mutex);
	return 0;
}

ISP_PROC_STATUS_E isp_proc_getProcStatus(void)
{
	return g_eProcStatus;
}

CVI_S32 isp_proc_updateData2ProcST(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("%s\n", "+");

	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		memcpy(g_pIspProcST[ViPipe], g_pIspTempProcST[ViPipe], sizeof(ISP_DEBUGINFO_PROC_S));
	}
	return 0;
}

CVI_S32 _isp_proc_formatVersion(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
	UNUSED(ViPipe);
	CVI_S32 writeSize = 0;
	ISP_AAA_VERSION aaaVer[ISP_AAA_TYPE_NUM] = {0};
	CVI_CHAR *aaaStr[ISP_AAA_TYPE_NUM] = {
		"AE Version",
		"AWB Version",
		"AF Version"
	};

	writeSize = snprintf(buffer, bufferSize, "%-27s%s\n",
					"ISP Version", ISP_VERSION);
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	for (CVI_U8 i = ISP_AAA_TYPE_FIRST; i < ISP_AAA_TYPE_END; ++i) {
		if (g_pfn_getVer[ViPipe][i] != NULL) {
			g_pfn_getVer[ViPipe][i](&(aaaVer[i].Ver), &(aaaVer[i].SubVer));
			writeSize = snprintf(buffer, bufferSize, "%-27s%d.%d\n",
					aaaStr[i], aaaVer[i].Ver, aaaVer[i].SubVer);
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		}
	}

	return 0;
}

CVI_S32 _isp_proc_formatCtrlParam(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_PROC_CTRL_VARIABLE_COUNT		(9)

	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;
	ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);

	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_PROC_CTRL_VARIABLE_COUNT] = {
		"ProcParam", "ProcLevel", "AEStatIntvl", "AWBStatIntvl",
		"AFStatIntvl", "UpdatePos", "IntTimeOut", "PwmNumber",
		"PortIntDelay"};
	CVI_U32 VariableValue[PROC_PROC_CTRL_VARIABLE_COUNT] = {
		pstIspProcCtrl->u32ProcParam, pstIspProcCtrl->u32ProcLevel,
		pstIspProcCtrl->u32AEStatIntvl, pstIspProcCtrl->u32AWBStatIntvl,
		pstIspProcCtrl->u32AFStatIntvl, pstIspProcCtrl->u32UpdatePos,
		pstIspProcCtrl->u32IntTimeOut, pstIspProcCtrl->u32PwmNumber,
		pstIspProcCtrl->u32PortIntDelay};

	CHECK_FUNC_RET(_isp_formatPipeTitle(ViPipe,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleTitle("Ctrl param Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_PROC_CTRL_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatFSWDR(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_FSWDR_VARIABLE_COUNT		(7)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_FSWDR_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "CombineSNRAwareEn", "CombineSNRAwareLowThr",
		"CombineSNRAwareHighThr", "CombineSNRAwareSmLevel", "CombineSNRAwareTolLevel"};
	CVI_U32 VariableValue[PROC_FSWDR_VARIABLE_COUNT] = {
		pProc->FSWDREnable, pProc->FSWDRisManualMode,
		pProc->FSWDRWDRCombineSNRAwareEn, pProc->FSWDRWDRCombineSNRAwareLowThr,
		pProc->FSWDRWDRCombineSNRAwareHighThr, pProc->FSWDRWDRCombineSNRAwareSmoothLevel,
		pProc->FSWDRWDRCombineSNRAwareToleranceLevel};

	CHECK_FUNC_RET(_isp_formatModuleTitle("FSWDR Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_FSWDR_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatShading(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_MLSC_VARIABLE_COUNT		(3)
#define PROC_RLSC_VARIABLE_COUNT		(4)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *MeshVariableName[PROC_MLSC_VARIABLE_COUNT] = {
		"MeshEnable", "MeshisManualMode", "MeshLscGainLut Size"};
	CVI_U32 MeshVariableValue[PROC_MLSC_VARIABLE_COUNT] = {
		pProc->MeshShadingEnable, pProc->MeshShadingisManualMode,
		pProc->MeshShadingGLSize};
	const CVI_CHAR *RadialVariableName[PROC_RLSC_VARIABLE_COUNT] = {
		"RadialEnable", "RadialCenterX", "RadialCenterY", "RadialStr"};
	CVI_U32 RadialVariableValue[PROC_RLSC_VARIABLE_COUNT] = {
		pProc->RadialShadingEnable, pProc->RadialShadingCenterX,
		pProc->RadialShadingCenterY, pProc->RadialShadingStr};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Shading Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(MeshVariableName,
						MeshVariableValue, PROC_MLSC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleU16Array(pProc->MeshShadingGLColorTemperature,
						"MeshLscGainLutColorTemp", ISP_MLSC_COLOR_TEMPERATURE_SIZE, 0,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(RadialVariableName,
						RadialVariableValue, PROC_RLSC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatDCI(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_DCI_VARIABLE_COUNT		(9)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_DCI_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "Speed", "DciStrength",
		"ContrastGain", "BlcThr", "WhtThr", "BlcCtrl",
		"WhtCtrl"};
	CVI_U32 VariableValue[PROC_DCI_VARIABLE_COUNT] = {
		pProc->DCIEnable, pProc->DCIisManualMode,
		pProc->DCISpeed, pProc->DCIDciStrength,
		pProc->DCIContrastGain, pProc->DCIBlcThr,
		pProc->DCIWhtThr, pProc->DCIBlcCtrl,
		pProc->DCIWhtCtrl};

	CHECK_FUNC_RET(_isp_formatModuleTitle("DCI Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_DCI_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatLDCI(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_LDCI_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_LDCI_VARIABLE_COUNT] = {
		"Enable"};
	CVI_U32 VariableValue[PROC_LDCI_VARIABLE_COUNT] = {
		pProc->LDCIEnable};

	CHECK_FUNC_RET(_isp_formatModuleTitle("LDCI Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_LDCI_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatDehaze(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_DEHAZE_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_DEHAZE_VARIABLE_COUNT] = {
		"Enable", "isManualMode"};
	CVI_U32 VariableValue[PROC_DEHAZE_VARIABLE_COUNT] = {
		pProc->DehazeEnable, pProc->DehazeisManualMode};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Dehaze Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_DEHAZE_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatBlackLevel(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_BLC_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_BLC_VARIABLE_COUNT] = {
		"Enable", "isManualMode"};
	CVI_U32 VariableValue[PROC_BLC_VARIABLE_COUNT] = {
		pProc->BlackLevelEnable, pProc->BlackLevelisManualMode};

	CHECK_FUNC_RET(_isp_formatModuleTitle("BlackLevel Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_BLC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatDPC(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_DPC_VARIABLE_COUNT		(3)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_DPC_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "ClusterSize"};
	CVI_U32 VariableValue[PROC_DPC_VARIABLE_COUNT] = {
		pProc->DPCEnable, pProc->DPCisManualMode,
		pProc->DPCClusterSize};

	CHECK_FUNC_RET(_isp_formatModuleTitle("DPC Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_DPC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatTNR(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_TNR_VARIABLE_COUNT		(19)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_TNR_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "DeflickerMode", "DeflickerToleranceLevel",
		"MtDetectUnit", "BrightnessNoiseLevelLE", "BrightnessNoiseLevelSE",
		"RNoiseLevel0", "RNoiseHiLevel0", "GNoiseLevel0", "GNoiseHiLevel0",
		"BNoiseLevel0", "BNoiseHiLevel0", "RNoiseLevel1", "RNoiseHiLevel1",
		"GNoiseLevel1", "GNoiseHiLevel1", "BNoiseLevel1", "BNoiseHiLevel1"};
	CVI_U32 VariableValue[PROC_TNR_VARIABLE_COUNT] = {
		pProc->TNREnable, pProc->TNRisManualMode,
		pProc->TNRDeflickerMode, pProc->TNRDeflickerToleranceLevel,
		pProc->TNRMtDetectUnit,
		pProc->TNRBrightnessNoiseLevelLE, pProc->TNRBrightnessNoiseLevelSE,
		pProc->TNRRNoiseLevel0, pProc->TNRRNoiseHiLevel0,
		pProc->TNRGNoiseLevel0, pProc->TNRGNoiseHiLevel0,
		pProc->TNRBNoiseLevel0, pProc->TNRBNoiseHiLevel0,
		pProc->TNRRNoiseLevel1, pProc->TNRRNoiseHiLevel1,
		pProc->TNRGNoiseLevel1, pProc->TNRGNoiseHiLevel1,
		pProc->TNRBNoiseLevel1, pProc->TNRBNoiseHiLevel1};

	CHECK_FUNC_RET(_isp_formatModuleTitle("TNR Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_TNR_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleU8Array(pProc->TNRMtDetectUnitList,
						"MtDetectUnitList", ISP_AUTO_ISO_STRENGTH_NUM, 0,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatCAC(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_CAC_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_CAC_VARIABLE_COUNT] = {
		"Enable"};
	CVI_U32 VariableValue[PROC_CAC_VARIABLE_COUNT] = {
		pProc->CACEnable};

	CHECK_FUNC_RET(_isp_formatModuleTitle("CAC Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_CAC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatRGBCAC(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_RGBCAC_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_RGBCAC_VARIABLE_COUNT] = {
		"Enable"};
	CVI_U32 VariableValue[PROC_RGBCAC_VARIABLE_COUNT] = {
		pProc->RGBCACEnable};

	CHECK_FUNC_RET(_isp_formatModuleTitle("LCAC Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_RGBCAC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatLCAC(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_LCAC_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_LCAC_VARIABLE_COUNT] = {
		"Enable"};
	CVI_U32 VariableValue[PROC_LCAC_VARIABLE_COUNT] = {
		pProc->LCACEnable};

	CHECK_FUNC_RET(_isp_formatModuleTitle("LCAC Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_LCAC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatCNR(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_CNR_VARIABLE_COUNT		(5)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_CNR_VARIABLE_COUNT] = {
		"Enable", "isManualMode",
		"NoiseSuppressGain", "FilterType", "MotionNrStr"};
	CVI_U32 VariableValue[PROC_CNR_VARIABLE_COUNT] = {
		pProc->CNREnable, pProc->CNRisManualMode,
		pProc->CNRNoiseSuppressGain, pProc->CNRFilterType,
		pProc->CNRMotionNrStr};

	CHECK_FUNC_RET(_isp_formatModuleTitle("CNR Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_CNR_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatSharpen(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_SHARPEN_VARIABLE_COUNT		(11)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_SHARPEN_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "LumaAdpCoringEn",
		"WdrCoringCompensationEn", "WdrCoringCompensationMode", "WdrCoringToleranceLevel",
		"WdrCoringHighThr", "WdrCoringLowThr", "EdgeFreq", "TextureFreq",
		"YNoiseLevel"};
	CVI_U32 VariableValue[PROC_SHARPEN_VARIABLE_COUNT] = {
		pProc->SharpenEnable, pProc->SharpenisManualMode,
		pProc->SharpenLumaAdpCoringEn, pProc->SharpenWdrCoringCompensationEn,
		pProc->SharpenWdrCoringCompensationMode, pProc->SharpenWdrCoringToleranceLevel,
		pProc->SharpenWdrCoringHighThr, pProc->SharpenWdrCoringLowThr,
		pProc->SharpenEdgeFreq, pProc->SharpenTextureFreq,
		pProc->SharpenYNoiseLevel};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Sharpen Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_SHARPEN_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

#if 0
CVI_S32 _isp_proc_formatSaturation(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_SATURATION_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_SATURATION_VARIABLE_COUNT] = {
		"isManualMode", "Saturation"};
	CVI_U32 VariableValue[PROC_SATURATION_VARIABLE_COUNT] = {
		pProc->SaturationisManualMode, pProc->SaturationSaturation[0]};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Saturation Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	if (pProc->SaturationisManualMode == 1) {
		CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
							VariableValue, PROC_SATURATION_VARIABLE_COUNT,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		ISP_LOG_DEBUG("actualSize %d\n", *actualSize);
	} else {
		CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
							VariableValue, PROC_SATURATION_VARIABLE_COUNT - 1,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleU8Array(pProc->SaturationSaturation,
						"Saturation", 16, 0,
						buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	}
	return 0;
}
#endif //

CVI_S32 _isp_proc_formatGamma(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_GAMMA_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_GAMMA_VARIABLE_COUNT] = {
		"Enable", "enCurveType"};
	CVI_U32 VariableValue[PROC_GAMMA_VARIABLE_COUNT] = {
		pProc->GammaEnable, pProc->GammaenCurveType};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Gamma Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_GAMMA_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatCCM(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_CCM_MANUAL_VARIABLE_COUNT		(5)
#define PROC_CCM_AUTO_VARIABLE_COUNT		(5)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	if (pProc->CCMisManualMode == 1) {
		const CVI_CHAR *VariableName[PROC_CCM_MANUAL_VARIABLE_COUNT] = {
			"Enable", "isManualMode", "SatEnable",
			"SaturationLE", "SaturationSE"};
		CVI_U32 VariableValue[PROC_CCM_MANUAL_VARIABLE_COUNT] = {
			pProc->CCMEnable, pProc->CCMisManualMode, pProc->CCMSatEnable,
			pProc->SaturationLE[0], pProc->SaturationSE[0]};

		CHECK_FUNC_RET(_isp_formatModuleTitle("CCM Info",
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
							VariableValue, PROC_CCM_MANUAL_VARIABLE_COUNT,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleS16Array(pProc->CCMCCM,
							"CCM", 9, 0,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		ISP_LOG_DEBUG("actualSize %d\n", *actualSize);
	} else {
		const CVI_CHAR *VariableName[PROC_CCM_AUTO_VARIABLE_COUNT] = {
			"Enable", "isManualMode", "ISOActEnable", "TempActEnable",
			"CCMTabNum"};
		CVI_U32 VariableValue[PROC_CCM_AUTO_VARIABLE_COUNT] = {
			pProc->CCMEnable, pProc->CCMisManualMode,
			pProc->CCMISOActEnable, pProc->CCMTempActEnable,
			pProc->CCMTabNum};

		CHECK_FUNC_RET(_isp_formatModuleTitle("CCM Info",
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
							VariableValue, PROC_CCM_AUTO_VARIABLE_COUNT,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleS16Array(pProc->CCMCCM,
							"CCM", 9, 0,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);

		for (CVI_U8 i = 0; i < pProc->CCMTabNum; ++i) {
			CVI_CHAR str1[50];
			CVI_CHAR str2[50];

			snprintf(str1, 50, "CCMTab[%d].ColorTemp", i);
			snprintf(str2, 50, "CCMTab[%d].Tab", i);
			CHECK_FUNC_RET(_isp_formatModuleVariable(str1,
								pProc->CCMTab[i].ColorTemp, false,
								buffer, bufferSize, &writeSize));
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
			CHECK_FUNC_RET(_isp_formatModuleS16Array(pProc->CCMTab[i].CCM,
								str2, 9, 0,
								buffer, bufferSize, &writeSize));
			UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		}
		CHECK_FUNC_RET(_isp_formatModuleU8Array(pProc->SaturationLE,
							"SaturationLE", ISP_AUTO_ISO_STRENGTH_NUM, 0,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		CHECK_FUNC_RET(_isp_formatModuleU8Array(pProc->SaturationSE,
							"SaturationSE", ISP_AUTO_ISO_STRENGTH_NUM, 0,
							buffer, bufferSize, &writeSize));
		UpdateValue(buffer, bufferSize, *actualSize, writeSize);
		ISP_LOG_DEBUG("actualSize %d\n", *actualSize);
	}

	return 0;
}

CVI_S32 _isp_proc_formatYNR(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_YNR_VARIABLE_COUNT		(5)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_YNR_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "CoringParamEnable",
		"WindowType", "FilterType"};
	CVI_U32 VariableValue[PROC_YNR_VARIABLE_COUNT] = {
		pProc->YNREnable, pProc->YNRisManualMode,
		pProc->YNRCoringParamEnable, pProc->YNRWindowType,
		pProc->YNRFilterType};

	CHECK_FUNC_RET(_isp_formatModuleTitle("YNR Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_YNR_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatExposureInfo(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_AE_EXP_INFO_VARIABLE_COUNT			(19)
#define PROC_AE_EXP_INFO_NEG_VARIABLE_COUNT		(1)
#define PROC_AE_EXP_INFO_FLOAT_VARIABLE_COUNT	(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	ISP_EXP_INFO_S *pstExp = &(pProc->tAEExp_Info);
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_AE_EXP_INFO_VARIABLE_COUNT] = {
		"ExpTime", "ShortExpTime", "LongExpTime", "WDRExpRatio",
		"AGain", "DGain", "ISPDGain", "Exposure",
		"ExposureIsMax", "AveLum", "LinesPer500ms", "PirisFno",
		"Fps", "ISO", "ISOCalibrate", "RefExpRatio",
		"WDRShortAveLuma", "LEFrameAvgLuma", "SEFrameAvgLuma"};
	CVI_U32 VariableValue[PROC_AE_EXP_INFO_VARIABLE_COUNT] = {
		pstExp->u32ExpTime, pstExp->u32ShortExpTime,
		pstExp->u32LongExpTime, pstExp->u32WDRExpRatio,
		pstExp->u32AGain, pstExp->u32DGain,
		pstExp->u32ISPDGain, pstExp->u32Exposure,
		(pstExp->bExposureIsMAX ? 1 : 0), pstExp->u8AveLum,
		pstExp->u32LinesPer500ms, pstExp->u32PirisFNO,
		pstExp->u32Fps, pstExp->u32ISO,
		pstExp->u32ISOCalibrate, pstExp->u32RefExpRatio,
		pstExp->u8WDRShortAveLuma, pstExp->u8LEFrameAvgLuma,
		pstExp->u8SEFrameAvgLuma};

	const CVI_CHAR *VariableNameNeg[PROC_AE_EXP_INFO_NEG_VARIABLE_COUNT] = {
		"HistError"};
	CVI_S32 VariableValueNeg[PROC_AE_EXP_INFO_NEG_VARIABLE_COUNT] = {
		pstExp->s16HistError};

	const CVI_CHAR *VariableNameFloat[PROC_AE_EXP_INFO_FLOAT_VARIABLE_COUNT] = {
		"LightValue"};
	float VariableValueFloat[PROC_AE_EXP_INFO_FLOAT_VARIABLE_COUNT] = {
		pstExp->fLightValue};

	CHECK_FUNC_RET(_isp_formatModuleTitle("ExposureInfo Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_AE_EXP_INFO_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables_Neg(VariableNameNeg,
						VariableValueNeg, PROC_AE_EXP_INFO_NEG_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables_Float(VariableNameFloat,
						VariableValueFloat, PROC_AE_EXP_INFO_FLOAT_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatWBInfo(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_WB_INFO_VARIABLE_COUNT			(7)
#define PROC_WB_INFO_NEG_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	ISP_WB_INFO_S *pstWB = &(pProc->tWB_Info);
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_WB_INFO_VARIABLE_COUNT] = {
		"Rgain", "Ggain", "Bgain", "Saturation",
		"ColorTemp", "FirstStableTime", "InOutStatus"};
	CVI_U32 VariableValue[PROC_WB_INFO_VARIABLE_COUNT] = {
		pstWB->u16Rgain, pstWB->u16Grgain,
		pstWB->u16Bgain, pstWB->u16Saturation,
		pstWB->u16ColorTemp, pstWB->u32FirstStableTime,
		pstWB->enInOutStatus};

	const CVI_CHAR *VariableNameNeg[PROC_WB_INFO_NEG_VARIABLE_COUNT] = {
		"Bv"};
	CVI_S32 VariableValueNeg[PROC_WB_INFO_NEG_VARIABLE_COUNT] = {
		pstWB->s16Bv};

	CHECK_FUNC_RET(_isp_formatModuleTitle("WBInfo Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_WB_INFO_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables_Neg(VariableNameNeg,
						VariableValueNeg, PROC_WB_INFO_NEG_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatBNR(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_BNR_VARIABLE_COUNT		(4)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_BNR_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "WindowType", "FilterType"};
	CVI_U32 VariableValue[PROC_BNR_VARIABLE_COUNT] = {
		pProc->BNREnable, pProc->BNRisManualMode,
		pProc->BNRWindowType, pProc->BNRFilterType};

	CHECK_FUNC_RET(_isp_formatModuleTitle("BNR Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_BNR_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatCLut(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_CLUT_VARIABLE_COUNT		(1)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_CLUT_VARIABLE_COUNT] = {
		"Enable"};
	CVI_U32 VariableValue[PROC_CLUT_VARIABLE_COUNT] = {
		pProc->CLutEnable};

	CHECK_FUNC_RET(_isp_formatModuleTitle("CLUT Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_CLUT_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatCrosstalk(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_CROSSTALK_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_CROSSTALK_VARIABLE_COUNT] = {
		"Enable", "isManualMode"};
	CVI_U32 VariableValue[PROC_CROSSTALK_VARIABLE_COUNT] = {
		pProc->CrosstalkEnable, pProc->CrosstalkisManualMode};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Crosstalk Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_CROSSTALK_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatDemosaic(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_DEMOSAIC_VARIABLE_COUNT		(2)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_DEMOSAIC_VARIABLE_COUNT] = {
		"Enable", "isManualMode"};
	CVI_U32 VariableValue[PROC_DEMOSAIC_VARIABLE_COUNT] = {
		pProc->DemosaicEnable, pProc->DemosaicisManualMode};

	CHECK_FUNC_RET(_isp_formatModuleTitle("Demosaic Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_DEMOSAIC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

CVI_S32 _isp_proc_formatDRC(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize)
{
#define PROC_DRC_VARIABLE_COUNT		(5)

	ISP_DEBUGINFO_PROC_S *pProc = g_pIspProcST[ViPipe];
	CVI_S32 writeSize = 0;

	const CVI_CHAR *VariableName[PROC_DRC_VARIABLE_COUNT] = {
		"Enable", "isManualMode", "ToneCurveSmooth",
		"HdrStrength"};
	CVI_U32 VariableValue[PROC_DRC_VARIABLE_COUNT] = {
		pProc->DRCEnable, pProc->DRCisManualMode,
		pProc->DRCToneCurveSmooth, pProc->DRCHdrStrength};

	CHECK_FUNC_RET(_isp_formatModuleTitle("DRC Info",
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	CHECK_FUNC_RET(_isp_formatModuleVariables(VariableName,
						VariableValue, PROC_DRC_VARIABLE_COUNT,
						buffer, bufferSize, &writeSize));
	UpdateValue(buffer, bufferSize, *actualSize, writeSize);
	ISP_LOG_DEBUG("actualSize %d\n", *actualSize);

	return 0;
}

static ISP_PROC_BLOCK ispProcBlock[ISP_PROC_BLOCK_MAX] = {
	[ISP_PROC_BLOCK_VERSION] = {
			.formatFunc = _isp_proc_formatVersion,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CTRLPARAM] = {
			.formatFunc = _isp_proc_formatCtrlParam,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_FSWDR] = {
			.formatFunc = _isp_proc_formatFSWDR,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_SHADING] = {
			.formatFunc = _isp_proc_formatShading,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_DCI] = {
			.formatFunc = _isp_proc_formatDCI,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_LDCI] = {
			.formatFunc = _isp_proc_formatLDCI,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_DEHAZE] = {
			.formatFunc = _isp_proc_formatDehaze,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_BLACKLEVEL] = {
			.formatFunc = _isp_proc_formatBlackLevel,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_DPC] = {
			.formatFunc = _isp_proc_formatDPC,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_TNR] = {
			.formatFunc = _isp_proc_formatTNR,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CAC] = {
			.formatFunc = _isp_proc_formatCAC,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_RGBCAC] = {
			.formatFunc = _isp_proc_formatRGBCAC,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_LCAC] = {
			.formatFunc = _isp_proc_formatLCAC,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CNR] = {
			.formatFunc = _isp_proc_formatCNR,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_SHARPEN] = {
			.formatFunc = _isp_proc_formatSharpen,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_SATURATION] = {
			.formatFunc = NULL,		// _isp_proc_formatSaturation
			.bFormat = false,
		},
	[ISP_PROC_BLOCK_GAMMA] = {
			.formatFunc = _isp_proc_formatGamma,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CCM] = {
			.formatFunc = _isp_proc_formatCCM,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_YNR] = {
			.formatFunc = _isp_proc_formatYNR,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_EXPOSUREINFO] = {
			.formatFunc =  _isp_proc_formatExposureInfo,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_WBINFO] = {
			.formatFunc = _isp_proc_formatWBInfo,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_BNR] = {
			.formatFunc = _isp_proc_formatBNR,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CLUT] = {
			.formatFunc = _isp_proc_formatCLut,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_CROSSTALK] = {
			.formatFunc = _isp_proc_formatCrosstalk,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_DEMOSAIC] = {
			.formatFunc = _isp_proc_formatDemosaic,
			.bFormat = true,
		},
	[ISP_PROC_BLOCK_DRC] = {
			.formatFunc = _isp_proc_formatDRC,
			.bFormat = true,
		},
};

CVI_S32 isp_proc_writeInfoToFile(void)
{
	CVI_S32 moduleWriteSize = 0;
	CVI_S32 ret = 0;
	CVI_U32 moduleTotalSize = 0;
	CVI_U32 procAvailableSize = PROCBUFFERSIZE;
	CVI_CHAR *pTempBuffer = g_pIspProcBuffer;
	CVI_U16 start = 0, end = 0;
	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;

#ifdef PROC_TIME_PROFILE
	struct timeval t1, t2;
	CVI_FLOAT timeuse;

	gettimeofday(&t1, NULL);
#endif // PROC_TIME_PROFILE

	if (pTempBuffer == NULL) {
		ISP_LOG_ERR("g_pIspProcBuffer is null, don't write proc file\n");
		return -1;
	}

	for (CVI_U8 ViPipe = 0; ViPipe < VI_MAX_PIPE_NUM; ViPipe++) {
		ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);
		if (g_bIsOpenPipe[ViPipe]) {
			switch (pstIspProcCtrl->u32ProcLevel) {
			case PROC_LEVEL_1:
				start = ISP_COMMON_START;
				end = ISP_LEVEL1_END;
				break;
			case PROC_LEVEL_2:
				start = ISP_COMMON_START;
				end = ISP_LEVEL2_END;
				break;
			case PROC_LEVEL_3:
				start = ISP_COMMON_START;
				end = ISP_LEVEL3_END;
				break;
			default:
				start = ISP_COMMON_START;
				end = ISP_COMMON_START;
			}
			ISP_LOG_DEBUG("Start write pipe[%d] loglevel[%d]\n",
				ViPipe, pstIspProcCtrl->u32ProcLevel);
			for (CVI_U16 block = start; block < end; ++block) {
				if ((ispProcBlock[block].formatFunc != NULL) &&
					(ispProcBlock[block].bFormat)) {
					ret = ispProcBlock[block].formatFunc(ViPipe, pTempBuffer,
						procAvailableSize, &moduleWriteSize);
					if (ret == -1) {
						ISP_LOG_ERR("%s\n",
							"Not enough buffer to format the output, please check PROCBUFFERSIZE");
						return -1;
					}
					pTempBuffer += moduleWriteSize;
					procAvailableSize -= moduleWriteSize;
					moduleWriteSize = 0;
				}
			}
			//add ending
			*pTempBuffer = '\0';
			procAvailableSize -= 1;
		}
	}
	moduleTotalSize = PROCBUFFERSIZE - procAvailableSize;
	ISP_LOG_DEBUG("moduleTotalSize:%d\n", moduleTotalSize);
	_isp_writeToProc(g_pIspProcBuffer, moduleTotalSize);

#ifdef PROC_TIME_PROFILE
	gettimeofday(&t2, NULL);
	timeuse = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
	ISP_LOG_NOTICE("Use Time:%f\n", timeuse);
#endif // PROC_TIME_PROFILE

	return 0;
}

void *ISP_ProcThread(void *param)
{
	UNUSED(param);
	ISP_LOG_DEBUG("+\n");
	prctl(PR_SET_NAME, "ISP_PROC", 0, 0, 0);
	pthread_detach(pthread_self());
	while (1) {
		pthread_mutex_lock(&cond_mutex);
		pthread_cond_wait(&cond, &cond_mutex);
		pthread_mutex_unlock(&cond_mutex);
		if (!g_thread_run)
			break;
		ISP_LOG_DEBUG("receive proc read event\n");
		isp_proc_writeInfoToFile();
		isp_proc_setProcStatus(PROC_STATUS_READY);
		usleep(1000);
	}
	return NULL;
}

CVI_S32 isp_proc_run(void)
{
	ISP_LOG_DEBUG("+\n");
	if (g_isProcRun == true) {
		return -1;
	}
	g_isProcRun = true;
	//Init
	g_pIspProcBuffer = ISP_CALLOC(1, PROCBUFFERSIZE);
	if (g_pIspProcBuffer == NULL) {
		ISP_LOG_ERR("can't allocate g_pIspProcBuffer, size=%d\n",
			PROCBUFFERSIZE);
		return -1;
	}
	pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_init(&cond_mutex, NULL);
	pthread_cond_init(&cond, 0);
	g_thread_run = true;
	isp_proc_setProcStatus(PROC_STATUS_READY);
	//thread run
	//struct sched_param param;
	pthread_attr_t attr;
	pthread_t id;

	//param.sched_priority = 75;//low priority
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	//pthread_attr_setschedparam(&attr, &param);
	//WARNING!!! If ASAN is enabled,
	//a high priority RR thread creates a low priority RR thread on a single-core CPU.
	//As a result, the CPU loading becomes abnormally high
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&id, &attr, ISP_ProcThread, NULL);
	return 0;
}

CVI_S32 isp_proc_exit(void)
{
	ISP_LOG_DEBUG("+\n");

	if (g_isProcRun == false) {
		return -1;
	}
	g_isProcRun = false;
	//thread destroy
	g_thread_run = false;
	pthread_mutex_lock(&cond_mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&cond_mutex);
	//Deinit
	ISP_RELEASE_MEMORY(g_pIspProcBuffer);
	isp_proc_setProcStatus(PROC_STATUS_START);
	pthread_mutex_destroy(&m_mutex);
	pthread_mutex_destroy(&cond_mutex);
	pthread_cond_destroy(&cond);
	return 0;
}

CVI_S32 isp_proc_collectAeInfo(VI_PIPE ViPipe, const ISP_EXP_INFO_S *pstAeExpInfo)
{
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);

	if ((pstIspCtx->frameCnt % pstIspProcCtrl->u32ProcParam) == 0) {
		if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
			ISP_DEBUGINFO_PROC_S *pProcST = NULL;

			ISP_GET_PROC_INFO(ViPipe, pProcST);

			memcpy(&(pProcST->tAEExp_Info), pstAeExpInfo, sizeof(ISP_EXP_INFO_S));
		}
	}

	return 0;
}

CVI_S32 isp_proc_collectAwbInfo(VI_PIPE ViPipe, const ISP_WB_INFO_S *pstAwbInfo)
{
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_CTRL_PARAM_S *pstIspProcCtrl = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ISP_GET_PROC_CTRL(ViPipe, pstIspProcCtrl);

	if ((pstIspCtx->frameCnt % pstIspProcCtrl->u32ProcParam) == 0) {
		if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
			ISP_DEBUGINFO_PROC_S *pProcST = NULL;

			ISP_GET_PROC_INFO(ViPipe, pProcST);

			memcpy(&(pProcST->tWB_Info), pstAwbInfo, sizeof(ISP_WB_INFO_S));
		}
	}

	return 0;
}

CVI_S32 isp_proc_regAAAGetVerCb(VI_PIPE ViPipe, ISP_AAA_TYPE_E aaaType, ISP_GetAAAVer_FUNC pfn_getVer)
{
	g_pfn_getVer[ViPipe][aaaType] = pfn_getVer;
	return 0;
}

CVI_S32 isp_proc_unRegAAAGetVerCb(VI_PIPE ViPipe, ISP_AAA_TYPE_E aaaType)
{
	if (g_pfn_getVer[ViPipe][aaaType] != NULL)
		g_pfn_getVer[ViPipe][aaaType] = NULL;
	return 0;
}
#endif
