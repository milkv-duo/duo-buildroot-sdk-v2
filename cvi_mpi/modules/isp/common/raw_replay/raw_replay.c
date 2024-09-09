/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: raw_replay.c
 * Description:
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "inttypes.h"

#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_sys_base.h"

#include "../sample/common/sample_comm.h"
#include "3A_internal.h"


#include "raw_replay.h"

#if defined(__CV181X__) || defined(__CV180X__)
#include "vi_ioctl.h"
#endif

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MINMAX(a, min, max) (MIN(MAX((a), (min)), (max)))

#define _BUFF_LEN  128
#define LOGOUT(fmt, arg...) printf("[RAW_REPLAY] %s,%d: " fmt, __func__, __LINE__, ##arg)

#define ABORT_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("abort: "#EXP"\n"); \
			abort();                   \
		}                              \
	} while (0)

#define RETURN_FAILURE_IF(EXP)         \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			return CVI_FAILURE;        \
		}                              \
	} while (0)

#define GOTO_FAIL_IF(EXP, LABEL)       \
	do {                               \
		if (EXP) {                     \
			LOGOUT("fail: "#EXP"\n");  \
			goto LABEL;                \
		}                              \
	} while (0)

#define WARN_IF(EXP)                   \
	do {                               \
		if (EXP) {                     \
			LOGOUT("warn: "#EXP"\n");  \
		}                              \
	} while (0)

#define ERROR_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("error: "#EXP"\n"); \
		}                              \
	} while (0)

#define SEC_TO_USEC 1000000

#if defined(__CV181X__) || defined(__CV180X__)
#define BLC_MAX_BIT 10
#define FRAME_OUT_MAX 4095
#define RGBMAP_BLOCK_BYTE 6
#define RGBMAP_BLOCK_MIN_UNIT 3

static int BAYER_ORDER[4][2][2] = {
	{ {BB, GB}, {GR, RR} },
	{ {GB, BB}, {RR, GR} },
	{ {GR, RR}, {BB, GB} },
	{ {RR, GR}, {GB, BB} },
};

static CVI_S32 iso_lut[ISP_AUTO_ISO_STRENGTH_NUM] = {
	100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600,
	51200, 102400, 204800, 409600, 819200, 1638400, 3276800
};
#endif

typedef struct {
	CVI_U8 wBit;
	CVI_U8 hBit;
	CVI_U16 gridRow;
	CVI_U16 gridCol;
	CVI_U8 *rgbBuf[2];
	CVI_U32 size;
} RGBMAP_INFO;

#define MAX_GET_YUV_TIMEOUT (1000)

typedef struct {
	CVI_BOOL bEnable;
	CVI_U8 src;
	CVI_S32 pipeOrGrp;
	CVI_S32 chn;
	VIDEO_FRAME_INFO_S videoFrame;
	CVI_S32 yuvIndex;
	pthread_t getYuvTid;
	CVI_BOOL bGetYuvThreadEnabled;
	sem_t sem;
} GET_YUV_CTX_S;

typedef struct {
	VI_PIPE ViPipe;
	RAW_REPLAY_INFO *pRawHeader;
	CVI_U32 u32TotalFrame;
	CVI_U32 u32RawFrameSize;
	RECT_S stRoiRect;
	CVI_U32 u32RoiTotalFrame;
	CVI_U32 u32RoiRawFrameSize;

	VB_POOL PoolID;
	VB_BLK blk;
	CVI_U32 u32BlkSize;
	CVI_U64 u64PhyAddr[2];
	CVI_U8 *pu8VirAddr[2];
	CVI_U64 u64RoiPhyAddr[2];
	CVI_U8 *pu8RoiVirAddr[2];

	pthread_t rawReplayTid;
	CVI_BOOL bRawReplayThreadEnabled;
	CVI_BOOL bRawReplayThreadSuspend;
	CVI_BOOL bUseDMA;
	CVI_TDMA_2D_S stDMAParam[2];
	CVI_U32 u32CtrlFlag;
	CVI_BOOL bSuspend;
	CVI_BOOL isRawReplayReady;

	RGBMAP_INFO *rgbMapInfo;
	CVI_U8 rgbMapIdx;
	struct cvi_vip_memblock drvInfo[2];

	GET_YUV_CTX_S stGetYuvCtx;
	VI_DEV_TIMING_ATTR_S timingAttr;
} RAW_REPLAY_CTX_S;

static RAW_REPLAY_CTX_S *pRawReplayCtx;
static RAWPLAY_OP_MODE op_mode;



static void get_current_awb_info(ISP_MWB_ATTR_S *pstMwbAttr);
static void update_awb_config(const ISP_MWB_ATTR_S *pstMwbAttr, ISP_OP_TYPE_E eType);
static void get_current_ae_info(ISP_EXP_INFO_S *pstExpInfo);
static void update_ae_config(const ISP_EXP_INFO_S *pstExpInfo, ISP_OP_TYPE_E eType);
static void apply_raw_header(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo, CVI_U32 index);

static void raw_replay_use_dma(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void raw_replay_use_vb(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void raw_replay_singlefrmae(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame);
static void *get_yuv_thread(void *param);
static void *raw_replay_thread(void *param);
static void update_video_frame(VIDEO_FRAME_INFO_S *stVideoFrame);

#if defined(__CV181X__) || defined(__CV180X__)
static void write_rgbmap(RAW_REPLAY_CTX_S *pCtx, int index);
static void get_rgbmap_info(RAW_REPLAY_CTX_S *pCtx, RAW_REPLAY_INFO *pRawInfo);
static void output_rgbmap_data(CVI_U8 *buf, int stride, int x, int y, int r, int g, int b);
static int get_raw_pixel_data(CVI_U8 *buf, int stride, int x, int y);
static void rgbmap_proc(CVI_U8 *raw, CVI_U8 *rgbOut, RAW_REPLAY_CTX_S *pCtx, int index);
static void generate_rgbmap_by_frame(RAW_REPLAY_CTX_S *pCtx, int index);
static CVI_S32 get_rgbmap_buf(RAW_REPLAY_CTX_S *pCtx);
static CVI_S32 get_rgbmap_block_wBit(CVI_S32 iso);
static void free_rgbmapbuf(void);
#endif

static void get_raw_replay_yuv_by_index(CVI_S32 index);

static CVI_S32 raw_replay_init(CVI_VOID)
{
	ABORT_IF(pRawReplayCtx != NULL);

	pRawReplayCtx = calloc(1, sizeof(RAW_REPLAY_CTX_S));
	ABORT_IF(pRawReplayCtx == NULL);

	memset(pRawReplayCtx, 0, sizeof(RAW_REPLAY_CTX_S));

	pRawReplayCtx->bRawReplayThreadEnabled = false;
	pRawReplayCtx->bRawReplayThreadSuspend = false;

	pRawReplayCtx->u32RoiTotalFrame = 0;

	pRawReplayCtx->bUseDMA = CVI_FALSE;

	pRawReplayCtx->u32CtrlFlag = 0;

	pRawReplayCtx->isRawReplayReady = false;
	pRawReplayCtx->PoolID = VB_INVALID_POOLID;

	pRawReplayCtx->stGetYuvCtx.bEnable = CVI_FALSE;
	pRawReplayCtx->stGetYuvCtx.yuvIndex = -1;
	sem_init(&pRawReplayCtx->stGetYuvCtx.sem, 0, 0);

	pRawReplayCtx->timingAttr.bEnable = CVI_FALSE;
	pRawReplayCtx->timingAttr.s32FrmRate = 25;

	return CVI_SUCCESS;
}

static CVI_S32 raw_replay_deinit(CVI_VOID)
{
	sem_destroy(&pRawReplayCtx->stGetYuvCtx.sem);
	if (pRawReplayCtx != NULL) {
		free(pRawReplayCtx);
		pRawReplayCtx = NULL;
	}

	return CVI_SUCCESS;
}

void raw_replay_ctrl(CVI_U32 flag)
{
	ABORT_IF(pRawReplayCtx == NULL);

	pRawReplayCtx->u32CtrlFlag = flag;
}

#define ION_TOTAL_MEM "/sys/kernel/debug/ion/cvi_carveout_heap_dump/total_mem"
#define ION_ALLOC_MEM "/sys/kernel/debug/ion/cvi_carveout_heap_dump/alloc_mem"
#define ION_SUMMARY   "/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary"

static void show_ion_debug_info(void)
{
	system("cat "ION_SUMMARY);

	LOGOUT("ION total mem:\n");
	system("cat "ION_TOTAL_MEM);

	LOGOUT("ION alloc mem:\n");
	system("cat "ION_ALLOC_MEM);

	LOGOUT("raw replay frame number = (ION total mem - ION alloc mem) / rawFrameSize\n\n");
}

CVI_S32 set_raw_replay_data(const CVI_VOID *header, const CVI_VOID *data,
							CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	RAW_REPLAY_INFO *pRawInfo = (RAW_REPLAY_INFO *) header;

	if (curFrame == 0) {
		if (pRawReplayCtx == NULL) {
			raw_replay_init();
		} else {
			if (pRawReplayCtx->bRawReplayThreadEnabled) {

				pRawReplayCtx->bRawReplayThreadSuspend = true;

				while (pRawReplayCtx->isRawReplayReady) {
					usleep(10 * 1000);
				}

				LOGOUT("suspend raw replay thread...\n");

				if (pRawReplayCtx->pRawHeader != NULL) {
					free(pRawReplayCtx->pRawHeader);
					pRawReplayCtx->pRawHeader = NULL;
				}
#if defined(__CV181X__) || defined(__CV180X__)
				free_rgbmapbuf();
#endif
				s32Ret = CVI_SYS_Munmap(pRawReplayCtx->pu8VirAddr[0],
					pRawReplayCtx->u32BlkSize);
				ERROR_IF(s32Ret != CVI_SUCCESS);

				s32Ret = CVI_VB_ReleaseBlock(pRawReplayCtx->blk);
				ERROR_IF(s32Ret != CVI_SUCCESS);

				s32Ret = CVI_VB_DestroyPool(pRawReplayCtx->PoolID);
				ERROR_IF(s32Ret != CVI_SUCCESS);
				pRawReplayCtx->PoolID = VB_INVALID_POOLID;
				pRawReplayCtx->u32RoiTotalFrame = 0;
			}
		}

		LOGOUT("totalFrame: %d, rawFrameSize: %d, %d,%d, roiFrameSize: %d, %d,%d,%d,%d,%d OP:%d\n",
			totalFrame, rawFrameSize, pRawInfo->width, pRawInfo->height,
			pRawInfo->roiFrameSize,
			pRawInfo->roiFrameNum,
			pRawInfo->stRoiRect.s32X, pRawInfo->stRoiRect.s32Y,
			pRawInfo->stRoiRect.u32Width, pRawInfo->stRoiRect.u32Height,
			pRawInfo->op_mode);

		pRawReplayCtx->u32TotalFrame = totalFrame;
		pRawReplayCtx->u32RawFrameSize = rawFrameSize;

		if (pRawInfo->roiFrameNum != 0) {
			pRawReplayCtx->u32RoiTotalFrame = pRawInfo->roiFrameNum;
			pRawReplayCtx->stRoiRect = pRawInfo->stRoiRect;
			pRawReplayCtx->u32RoiRawFrameSize = pRawInfo->roiFrameSize;
		}

		if (pRawReplayCtx->pRawHeader == NULL) {
			pRawReplayCtx->pRawHeader = (RAW_REPLAY_INFO *) calloc(
										totalFrame,
										sizeof(RAW_REPLAY_INFO));
		}

		ABORT_IF(pRawReplayCtx->pRawHeader == NULL);

		VB_POOL_CONFIG_S cfg;
		CVI_U64 tmpPhyAddr = 0;

		cfg.u32BlkSize = (pRawReplayCtx->u32TotalFrame - pRawReplayCtx->u32RoiTotalFrame) * \
						 pRawReplayCtx->u32RawFrameSize;
		cfg.u32BlkCnt = 1;
		cfg.enRemapMode = VB_REMAP_MODE_CACHED;

		snprintf(cfg.acName, MAX_VB_POOL_NAME_LEN, "%s", "raw_replay_vb");

		if (pRawReplayCtx->u32RoiTotalFrame != 0) {
			cfg.u32BlkSize += pRawReplayCtx->u32RoiTotalFrame * pRawReplayCtx->u32RoiRawFrameSize;
		}

		pRawReplayCtx->u32BlkSize = cfg.u32BlkSize;
		if (pRawReplayCtx->PoolID == VB_INVALID_POOLID) {
			pRawReplayCtx->PoolID = CVI_VB_CreatePool(&cfg);

			if (pRawReplayCtx->PoolID == VB_INVALID_POOLID) {
				show_ion_debug_info();
			}
			ABORT_IF(pRawReplayCtx->PoolID == VB_INVALID_POOLID);

			pRawReplayCtx->blk = CVI_VB_GetBlock(pRawReplayCtx->PoolID, cfg.u32BlkSize);
			tmpPhyAddr = CVI_VB_Handle2PhysAddr(pRawReplayCtx->blk);
			pRawReplayCtx->u64PhyAddr[0] = tmpPhyAddr;

			pRawReplayCtx->pu8VirAddr[0] = (CVI_U8 *) CVI_SYS_MmapCache(tmpPhyAddr, cfg.u32BlkSize);
			if (pRawInfo->enWDR) {
				pRawReplayCtx->u64PhyAddr[1] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32BlkSize / 2;
				pRawReplayCtx->pu8VirAddr[1] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32BlkSize / 2;
			}
			LOGOUT("create vb pool cnt: %d, blksize: %d phyAddr: %"PRId64"\n", 1,
				cfg.u32BlkSize, pRawReplayCtx->u64PhyAddr[0]);
		}

		if (pRawReplayCtx->u32RoiTotalFrame != 0) {
			pRawReplayCtx->u64RoiPhyAddr[0] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32RawFrameSize;
			pRawReplayCtx->pu8RoiVirAddr[0] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32RawFrameSize;
			if (pRawInfo->enWDR) {
				pRawReplayCtx->u64RoiPhyAddr[0] = pRawReplayCtx->u64PhyAddr[0] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->pu8RoiVirAddr[0] = pRawReplayCtx->pu8VirAddr[0] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->u64RoiPhyAddr[1] = pRawReplayCtx->u64PhyAddr[1] + pRawReplayCtx->u32RawFrameSize / 2;
				pRawReplayCtx->pu8RoiVirAddr[1] = pRawReplayCtx->pu8VirAddr[1] + pRawReplayCtx->u32RawFrameSize / 2;
			}
		}

		if (pRawReplayCtx->u32RoiTotalFrame != 0 &&
			pRawReplayCtx->u32RoiRawFrameSize != pRawReplayCtx->u32RawFrameSize) {

			pRawReplayCtx->bUseDMA = CVI_TRUE;
			CVI_U32 heightSrc = pRawInfo->stRoiRect.u32Height;
			CVI_U32 strideBytesSrc = pRawInfo->roiFrameSize / heightSrc;
			CVI_U32 strideBytesDst =
				pRawReplayCtx->u32RawFrameSize / pRawInfo->height;

			if (pRawInfo->enWDR) {
				strideBytesSrc = strideBytesSrc / 2;
				strideBytesDst = strideBytesDst / 2;
			}

			pRawReplayCtx->stDMAParam[0].stride_bytes_src = strideBytesSrc;
			pRawReplayCtx->stDMAParam[0].stride_bytes_dst = strideBytesDst;
			pRawReplayCtx->stDMAParam[0].h = heightSrc;
			pRawReplayCtx->stDMAParam[0].w_bytes = strideBytesSrc;
			pRawReplayCtx->stDMAParam[0].paddr_src = pRawReplayCtx->u64RoiPhyAddr[0];
			pRawReplayCtx->stDMAParam[0].paddr_dst = pRawReplayCtx->u64PhyAddr[0] +
				strideBytesDst * pRawInfo->stRoiRect.s32Y + pRawInfo->stRoiRect.s32X / 2 * 3;
			if (pRawInfo->enWDR) {
				pRawReplayCtx->stDMAParam[1].stride_bytes_src = strideBytesSrc;
				pRawReplayCtx->stDMAParam[1].stride_bytes_dst = strideBytesDst;
				pRawReplayCtx->stDMAParam[1].h = heightSrc;
				pRawReplayCtx->stDMAParam[1].w_bytes = strideBytesSrc;
				pRawReplayCtx->stDMAParam[1].paddr_src = pRawReplayCtx->u64RoiPhyAddr[1];
				pRawReplayCtx->stDMAParam[1].paddr_dst = pRawReplayCtx->u64PhyAddr[1] +
					strideBytesDst * pRawInfo->stRoiRect.s32Y + pRawInfo->stRoiRect.s32X / 2 * 3;
			}
			LOGOUT("stride_src: %d, stride_dst: %d, h: %d, w_bytes: %d\n",
				strideBytesSrc, strideBytesDst, heightSrc, strideBytesSrc);
		} else {
			pRawReplayCtx->bUseDMA = CVI_FALSE;
		}

#if defined(__CV181X__) || defined(__CV180X__)
		pRawReplayCtx->rgbMapInfo = (RGBMAP_INFO *)calloc(totalFrame, sizeof(RGBMAP_INFO));
		ABORT_IF(pRawReplayCtx->rgbMapInfo == NULL);
		pRawReplayCtx->rgbMapIdx = 0;
#endif

	}

	if (pRawReplayCtx == NULL) {
		LOGOUT("ERROR, please set frame 0\n");
		return CVI_FAILURE;
	}

#if defined(__CV181X__) || defined(__CV180X__)
	get_rgbmap_info(pRawReplayCtx, pRawInfo);
#endif

	if (curFrame < pRawReplayCtx->u32TotalFrame) {
		memcpy((uint8_t *)pRawReplayCtx->pRawHeader + curFrame * sizeof(RAW_REPLAY_INFO),
				header, sizeof(RAW_REPLAY_INFO));
		if (pRawReplayCtx->u32RoiTotalFrame == 0) {
			if (pRawInfo->enWDR) {
				memcpy(pRawReplayCtx->pu8VirAddr[0] + curFrame * pRawReplayCtx->u32RawFrameSize / 2,
					data, pRawReplayCtx->u32RawFrameSize / 2);
				memcpy(pRawReplayCtx->pu8VirAddr[1] + curFrame * pRawReplayCtx->u32RawFrameSize / 2,
					data + pRawReplayCtx->u32RawFrameSize / 2, pRawReplayCtx->u32RawFrameSize / 2);
			} else {
				memcpy(pRawReplayCtx->pu8VirAddr[0] + curFrame * pRawReplayCtx->u32RawFrameSize,
					data, pRawReplayCtx->u32RawFrameSize);
			}
#if defined(__CV181X__) || defined(__CV180X__)
			generate_rgbmap_by_frame(pRawReplayCtx, curFrame);
#endif
		} else {
			if (curFrame == 0) {
				if (pRawInfo->enWDR) {
					memcpy(pRawReplayCtx->pu8VirAddr[0], data, pRawReplayCtx->u32RawFrameSize / 2);
					memcpy(pRawReplayCtx->pu8VirAddr[1], data + pRawReplayCtx->u32RawFrameSize / 2,
							pRawReplayCtx->u32RawFrameSize / 2);
				} else {
					memcpy(pRawReplayCtx->pu8VirAddr[0], data, pRawReplayCtx->u32RawFrameSize);
				}
#if defined(__CV181X__) || defined(__CV180X__)
				generate_rgbmap_by_frame(pRawReplayCtx, curFrame);
#endif
			} else {
				if (pRawInfo->enWDR) {
					memcpy(pRawReplayCtx->pu8RoiVirAddr[0] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize / 2,
						data, pRawReplayCtx->u32RoiRawFrameSize / 2);
					memcpy(pRawReplayCtx->pu8RoiVirAddr[1] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize / 2,
						data + pRawReplayCtx->u32RoiRawFrameSize / 2, pRawReplayCtx->u32RoiRawFrameSize / 2);
				} else {
					memcpy(pRawReplayCtx->pu8RoiVirAddr[0] + (curFrame - 1) * pRawReplayCtx->u32RoiRawFrameSize,
						data, pRawReplayCtx->u32RoiRawFrameSize);
				}
#if defined(__CV181X__) || defined(__CV180X__)
				generate_rgbmap_by_frame(pRawReplayCtx, curFrame);
#endif
			}
		}
	} else {
		return CVI_FAILURE;
	}

	if (curFrame == 0) {

		if (access("/mnt/data/rawTestFlag", F_OK) == 0) {

			LOGOUT("save auto test data......\n");

			//system("mkdir -p /mnt/data/rawTest");
			char path[_BUFF_LEN] = {0};
			char cmd[_BUFF_LEN] = {0};

			const char *prefix = NULL;

			CVI_U32 chip_id = 0;

			CVI_SYS_GetChipId(&chip_id);

			printf("error: unknown chip......\n");

			snprintf(path, _BUFF_LEN, "%s/%s", prefix,
				pRawReplayCtx->pRawHeader[0].enWDR ? "wdr" : "sdr");

			snprintf(cmd, _BUFF_LEN, "mkdir -p %s", path);

			system(cmd);

			snprintf(path, _BUFF_LEN, "%s/%s/header.bin", prefix,
				pRawReplayCtx->pRawHeader[0].enWDR ? "wdr" : "sdr");

			FILE *fp = NULL;

			fp = fopen(path, "wb");
			if (fp == NULL) {
				LOGOUT("fopen %s failed!!!\n", path);
			} else {
				fwrite(pRawReplayCtx->pRawHeader, sizeof(RAW_REPLAY_INFO), 1, fp);
				fclose(fp);
				fp = NULL;
			}

			LOGOUT("size: %d, %d\n", (CVI_U32) sizeof(RAW_REPLAY_INFO), rawFrameSize);

			snprintf(path, _BUFF_LEN, "%s/%s/test.raw", prefix,
				pRawReplayCtx->pRawHeader[0].enWDR ? "wdr" : "sdr");

			fp = fopen(path, "wb");
			if (fp == NULL) {
				LOGOUT("fopen %s failed!!!\n", path);
			} else {
				fwrite(data, rawFrameSize, 1, fp);
				fclose(fp);
				fp = NULL;
			}
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 start_raw_replay(VI_PIPE ViPipe)
{
	RETURN_FAILURE_IF(pRawReplayCtx == NULL);

	CVI_ISP_AESetRawReplayMode(0, CVI_TRUE);

	if (!pRawReplayCtx->bRawReplayThreadEnabled) {

		pRawReplayCtx->ViPipe = ViPipe;

		pRawReplayCtx->bRawReplayThreadEnabled = CVI_TRUE;

		RETURN_FAILURE_IF(pthread_create(&pRawReplayCtx->rawReplayTid,
				NULL, raw_replay_thread, NULL) != 0);
	} else if (pRawReplayCtx->bRawReplayThreadSuspend) {
		pRawReplayCtx->bRawReplayThreadSuspend = false;
	}

	return CVI_SUCCESS;
}

CVI_S32 stop_raw_replay(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	CVI_ISP_AESetRawReplayMode(0, CVI_FALSE);

	if (pRawReplayCtx == NULL)
		return CVI_SUCCESS;

	if (pRawReplayCtx->bRawReplayThreadEnabled) {

		pRawReplayCtx->bRawReplayThreadSuspend = false;
		pRawReplayCtx->bRawReplayThreadEnabled = false;

		pthread_join(pRawReplayCtx->rawReplayTid, NULL);

		if (pRawReplayCtx->pRawHeader != NULL) {
			free(pRawReplayCtx->pRawHeader);
			pRawReplayCtx->pRawHeader = NULL;
		}
#if defined(__CV181X__) || defined(__CV180X__)
		free_rgbmapbuf();
#endif
		s32Ret = CVI_SYS_Munmap(pRawReplayCtx->pu8VirAddr[0],
			pRawReplayCtx->u32BlkSize);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		s32Ret = CVI_VB_ReleaseBlock(pRawReplayCtx->blk);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		s32Ret = CVI_VB_DestroyPool(pRawReplayCtx->PoolID);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		raw_replay_deinit();
	}

	return CVI_SUCCESS;
}

static void get_current_awb_info(ISP_MWB_ATTR_S *pstMwbAttr)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_INFO_S stWBInfo;

	memset(&stWBInfo, 0, sizeof(ISP_WB_INFO_S));

	s32Ret = CVI_ISP_QueryWBInfo(pRawReplayCtx->ViPipe, &stWBInfo);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	pstMwbAttr->u16Rgain = stWBInfo.u16Rgain;
	pstMwbAttr->u16Grgain = stWBInfo.u16Grgain;
	pstMwbAttr->u16Gbgain = stWBInfo.u16Gbgain;
	pstMwbAttr->u16Bgain = stWBInfo.u16Bgain;
}

static void update_awb_config(const ISP_MWB_ATTR_S *pstMwbAttr, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_WB_ATTR_S stWbAttr;

	if ((pRawReplayCtx->u32CtrlFlag & DISABLE_AWB_UPDATE_CTRL) != 0) {
		return;
	}

	memset(&stWbAttr, 0, sizeof(ISP_WB_ATTR_S));

	s32Ret = CVI_ISP_GetWBAttr(pRawReplayCtx->ViPipe, &stWbAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL) {
		stWbAttr.enOpType = OP_TYPE_MANUAL;
		stWbAttr.stManual = *pstMwbAttr;
	} else {
		stWbAttr.enOpType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetWBAttr(pRawReplayCtx->ViPipe, &stWbAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);
}

static void get_current_ae_info(ISP_EXP_INFO_S *pstExpInfo)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_ISP_QueryExposureInfo(pRawReplayCtx->ViPipe, pstExpInfo);
	ERROR_IF(s32Ret != CVI_SUCCESS);
}

static void update_ae_config(const ISP_EXP_INFO_S *pstExpInfo, ISP_OP_TYPE_E eType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	ISP_EXPOSURE_ATTR_S stAEAttr;
	ISP_WDR_EXPOSURE_ATTR_S stWdrExpAttr;

	memset(&stAEAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	memset(&stWdrExpAttr, 0, sizeof(ISP_WDR_EXPOSURE_ATTR_S));

	s32Ret = CVI_ISP_GetExposureAttr(pRawReplayCtx->ViPipe, &stAEAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	s32Ret = CVI_ISP_GetWDRExposureAttr(pRawReplayCtx->ViPipe, &stWdrExpAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL) {
		stAEAttr.enOpType = OP_TYPE_MANUAL;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;

		stAEAttr.stManual.u32ExpTime = pstExpInfo->u32ExpTime;
		stAEAttr.stManual.u32AGain = pstExpInfo->u32AGain;
		stAEAttr.stManual.u32DGain = pstExpInfo->u32DGain;
		stAEAttr.stManual.u32ISPDGain = pstExpInfo->u32ISPDGain;

		if (pRawReplayCtx->u32TotalFrame == 1) {
			stWdrExpAttr.enExpRatioType = OP_TYPE_MANUAL;

			for (uint32_t i = 0; i < WDR_EXP_RATIO_NUM; i++) {
				stWdrExpAttr.au32ExpRatio[i] = pstExpInfo->u32WDRExpRatio;
			}
		}

		memcpy((CVI_U8 *)&stAEAttr.stAuto.au32Reserve[0],
			(CVI_U8 *)&pstExpInfo->fLightValue,
			sizeof(CVI_FLOAT));
	} else {
		stAEAttr.enOpType = OP_TYPE_AUTO;

		stAEAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		stAEAttr.stManual.enGainType = AE_TYPE_GAIN;
		stAEAttr.stManual.enISONumOpType = OP_TYPE_AUTO;

		stWdrExpAttr.enExpRatioType = OP_TYPE_AUTO;
	}

	s32Ret = CVI_ISP_SetExposureAttr(pRawReplayCtx->ViPipe, &stAEAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	s32Ret = CVI_ISP_SetWDRExposureAttr(pRawReplayCtx->ViPipe, &stWdrExpAttr);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	if (eType == OP_TYPE_MANUAL)
		CVI_ISP_AESetRawReplayExposure(pRawReplayCtx->ViPipe, pstExpInfo);
}

static void apply_raw_header(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo, CVI_U32 index)
{
	pstMwbAttr->u16Rgain = pRawReplayCtx->pRawHeader[index].WB_RGain;
	pstMwbAttr->u16Grgain = pRawReplayCtx->pRawHeader[index].WB_GGain;
	pstMwbAttr->u16Bgain = pRawReplayCtx->pRawHeader[index].WB_BGain;

	pstExpInfo->u32ExpTime = pRawReplayCtx->pRawHeader[index].longExposure;
	pstExpInfo->u32AGain = pRawReplayCtx->pRawHeader[index].exposureAGain;
	pstExpInfo->u32DGain = pRawReplayCtx->pRawHeader[index].exposureDGain;
	pstExpInfo->u32ISPDGain = pRawReplayCtx->pRawHeader[index].ispDGain;

	pstExpInfo->u32ShortExpTime = pRawReplayCtx->pRawHeader[index].shortExposure;
	pstExpInfo->u32AGainSF = pRawReplayCtx->pRawHeader[index].AGainSF;
	pstExpInfo->u32DGainSF = pRawReplayCtx->pRawHeader[index].DGainSF;
	pstExpInfo->u32ISPDGainSF = pRawReplayCtx->pRawHeader[index].ispDGainSF;

	pstExpInfo->u32ISO = pRawReplayCtx->pRawHeader[index].ISO;
	pstExpInfo->fLightValue = pRawReplayCtx->pRawHeader[index].lightValue;

	pstExpInfo->u32WDRExpRatio = pRawReplayCtx->pRawHeader[index].exposureRatio;
}

static void raw_replay_use_dma(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 idx = 0, sig = 1;
	CVI_U64 addrLe = 0, addrSe = 0;
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}

#if defined(__CV181X__) || defined(__CV180X__)
	write_rgbmap(pRawReplayCtx, 0);
	pRawReplayCtx->rgbMapIdx = (pRawReplayCtx->rgbMapIdx + 1) % 3;
#endif
	apply_raw_header(pstMwbAttr, pstExpInfo, 0);
	update_ae_config(pstExpInfo, OP_TYPE_MANUAL);
	update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);

	s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
	ERROR_IF(s32Ret != CVI_SUCCESS);

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
		get_raw_replay_yuv_by_index(idx + 1);
		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);
		gettimeofday(&t2, NULL);

		if (pRawReplayCtx->pRawHeader[0].enWDR) {
			addrLe = pRawReplayCtx->u64RoiPhyAddr[0] + idx * pRawReplayCtx->u32RoiRawFrameSize / 2;
			pRawReplayCtx->stDMAParam[0].paddr_src = addrLe;
			s32Ret = CVI_SYS_TDMACopy2D(&pRawReplayCtx->stDMAParam[0]);
			ERROR_IF(s32Ret != CVI_SUCCESS);
			addrSe = pRawReplayCtx->u64RoiPhyAddr[1] + idx * pRawReplayCtx->u32RoiRawFrameSize / 2;
			pRawReplayCtx->stDMAParam[1].paddr_src = addrSe;
			s32Ret = CVI_SYS_TDMACopy2D(&pRawReplayCtx->stDMAParam[1]);
			ERROR_IF(s32Ret != CVI_SUCCESS);

		} else {
			addrLe = pRawReplayCtx->u64RoiPhyAddr[0] + idx * pRawReplayCtx->u32RoiRawFrameSize;
			pRawReplayCtx->stDMAParam[0].paddr_src = addrLe;
			s32Ret = CVI_SYS_TDMACopy2D(&pRawReplayCtx->stDMAParam[0]);
			ERROR_IF(s32Ret != CVI_SUCCESS);
		}
#if defined(__CV181X__) || defined(__CV180X__)
		write_rgbmap(pRawReplayCtx, idx + 1);
		pRawReplayCtx->rgbMapIdx = (pRawReplayCtx->rgbMapIdx + 1) % 3;
#endif
		apply_raw_header(pstMwbAttr, pstExpInfo, idx + 1);
		update_ae_config(pstExpInfo, OP_TYPE_MANUAL);

		if ((idx + sig >= (CVI_S32)pRawReplayCtx->u32RoiTotalFrame) || (idx + sig < 0)) {
			sig = -sig;
		}
		idx += sig;

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}

		gettimeofday(&t1, NULL);
		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);
	}
}

static void raw_replay_use_vb(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE PipeId[] = {0};
	CVI_S32 idx = 0, sig = 1;
	CVI_U64 addrLe = 0, addrSe = 0;
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	addrLe = pRawReplayCtx->u64PhyAddr[0];
	addrSe = pRawReplayCtx->u64PhyAddr[1];
	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}
	apply_raw_header(pstMwbAttr, pstExpInfo, 0);
	update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
		if (pRawReplayCtx->pRawHeader[0].enWDR) {
			pstVideoFrame->stVFrame.u64PhyAddr[0] = addrLe +
				idx * pRawReplayCtx->u32RawFrameSize / 2;
			pstVideoFrame->stVFrame.u64PhyAddr[1] = addrSe +
				idx * pRawReplayCtx->u32RawFrameSize / 2;
		} else {
			pstVideoFrame->stVFrame.u64PhyAddr[0] = addrLe +
				idx * pRawReplayCtx->u32RawFrameSize;
		}
#if defined(__CV181X__) || defined(__CV180X__)
		write_rgbmap(pRawReplayCtx, idx);
		pRawReplayCtx->rgbMapIdx = (pRawReplayCtx->rgbMapIdx + 1) % 3;
#endif
		apply_raw_header(pstMwbAttr, pstExpInfo, idx);
		update_ae_config(pstExpInfo, OP_TYPE_MANUAL);

		gettimeofday(&t1, NULL);

		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);

		get_raw_replay_yuv_by_index(idx);
		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);
		while (pRawReplayCtx->stGetYuvCtx.bEnable) {
			usleep(10 * 1000);
		}

		gettimeofday(&t2, NULL);

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}

		if ((idx + sig >= (CVI_S32)pRawReplayCtx->u32TotalFrame) || (idx + sig < 0)) {
			sig = -sig;
		}
		idx += sig;
	}
}

static void raw_replay_singlefrmae(ISP_MWB_ATTR_S *pstMwbAttr, ISP_EXP_INFO_S *pstExpInfo,
				VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VI_PIPE PipeId[] = {0};
	const VIDEO_FRAME_INFO_S *pstVideoFrameInfo[1];
	CVI_U32 frameTime = 0;
	CVI_U32 frameRate = pRawReplayCtx->timingAttr.s32FrmRate;
	struct timeval t1, t2;

	pstVideoFrameInfo[0] = pstVideoFrame;
	if (!pRawReplayCtx->timingAttr.bEnable) {
		frameRate = frameRate > 0 ? frameRate : 25;
		frameTime = SEC_TO_USEC / frameRate;
	}

	apply_raw_header(pstMwbAttr, pstExpInfo, 0);
	update_awb_config(pstMwbAttr, OP_TYPE_MANUAL);

	while (pRawReplayCtx->bRawReplayThreadEnabled && !pRawReplayCtx->bRawReplayThreadSuspend) {
#if defined(__CV181X__) || defined(__CV180X__)
		write_rgbmap(pRawReplayCtx, 0);
		pRawReplayCtx->rgbMapIdx = (pRawReplayCtx->rgbMapIdx + 1) % 3;
#endif
		update_ae_config(pstExpInfo, OP_TYPE_MANUAL);

		gettimeofday(&t1, NULL);

		s32Ret = CVI_VI_SendPipeRaw(1, PipeId, pstVideoFrameInfo, 0);
		ERROR_IF(s32Ret != CVI_SUCCESS);
		CVI_ISP_GetVDTimeOut(0, ISP_VD_FE_END, 0);

		gettimeofday(&t2, NULL);

		if (!pRawReplayCtx->timingAttr.bEnable) {
			CVI_U64 interval = (t2.tv_sec - t1.tv_sec) * SEC_TO_USEC + (t2.tv_usec - t1.tv_usec);

			if (frameTime > interval)
				usleep(frameTime - interval);
		}
	}
}

CVI_BOOL is_raw_replay_ready(void)
{
	if (pRawReplayCtx != NULL) {
		return pRawReplayCtx->isRawReplayReady;
	} else {
		return false;
	}
}

static void update_video_frame(VIDEO_FRAME_INFO_S *stVideoFrame)
{
	CVI_U8 mode = pRawReplayCtx->pRawHeader[0].enWDR;

	LOGOUT("wdrmode: %d, width: %d, height: %d opm:%d\n", mode, pRawReplayCtx->pRawHeader[0].width,
		pRawReplayCtx->pRawHeader[0].height, op_mode);

	if (mode) {
		stVideoFrame->stVFrame.u32Width = pRawReplayCtx->pRawHeader[0].width >> 1;
	} else {
		stVideoFrame->stVFrame.u32Width = pRawReplayCtx->pRawHeader[0].width;
	}

	stVideoFrame->stVFrame.u32Height = pRawReplayCtx->pRawHeader[0].height;

	stVideoFrame->stVFrame.s16OffsetLeft = stVideoFrame->stVFrame.s16OffsetTop =
		stVideoFrame->stVFrame.s16OffsetRight = stVideoFrame->stVFrame.s16OffsetBottom = 0;

	stVideoFrame->stVFrame.enBayerFormat = (BAYER_FORMAT_E) pRawReplayCtx->pRawHeader[0].bayerID;
	stVideoFrame->stVFrame.enPixelFormat = 0;

	if (mode) {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_HDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayCtx->u64PhyAddr[0];
		stVideoFrame->stVFrame.u64PhyAddr[1] = pRawReplayCtx->u64PhyAddr[1];
	} else {
		stVideoFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR10;
		stVideoFrame->stVFrame.u64PhyAddr[0] = pRawReplayCtx->u64PhyAddr[0];
	}
}

static void *get_yuv_thread(void *param)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	UNUSED(param);

	while (pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled) {
		if (pRawReplayCtx->stGetYuvCtx.bEnable) {
			LOGOUT("get yuv by index: %d\n", pRawReplayCtx->stGetYuvCtx.yuvIndex);

			memset(&pRawReplayCtx->stGetYuvCtx.videoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

			if (pRawReplayCtx->stGetYuvCtx.src == 0) {
				s32Ret = CVI_VI_GetChnFrame(pRawReplayCtx->stGetYuvCtx.pipeOrGrp,
					pRawReplayCtx->stGetYuvCtx.chn,
					&pRawReplayCtx->stGetYuvCtx.videoFrame,
					MAX_GET_YUV_TIMEOUT);
			} else {
				s32Ret = CVI_VPSS_GetChnFrame(pRawReplayCtx->stGetYuvCtx.pipeOrGrp,
					pRawReplayCtx->stGetYuvCtx.chn,
					&pRawReplayCtx->stGetYuvCtx.videoFrame,
					MAX_GET_YUV_TIMEOUT);
			}

			if (s32Ret != CVI_SUCCESS) {
				LOGOUT("get yuv failed with: %#x!\n", s32Ret);
			}

			pRawReplayCtx->stGetYuvCtx.yuvIndex = -1;
			sem_post(&pRawReplayCtx->stGetYuvCtx.sem);
			pRawReplayCtx->stGetYuvCtx.bEnable = CVI_FALSE;
		}
		usleep(20 * 1000);
	}

	return NULL;
}

static void *raw_replay_thread(void *param)
{
	VI_DEV_TIMING_ATTR_S stTimingAttr;
	VIDEO_FRAME_INFO_S stVideoFrame;
	CVI_S32 s32Ret = CVI_SUCCESS;
	ISP_MWB_ATTR_S stMWBAttr;
	ISP_EXP_INFO_S stExpInfo;
	param = param;

	memset(&stMWBAttr, 0, sizeof(ISP_MWB_ATTR_S));
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	get_current_awb_info(&stMWBAttr);
	get_current_ae_info(&stExpInfo);
	LOGOUT("wbRGain:%d,wbGGain:%d,wbBGain:%d,exptime:%d,iso:%d,expratio:%d,LV:%f\n",
		pRawReplayCtx->pRawHeader[0].WB_RGain,
		pRawReplayCtx->pRawHeader[0].WB_GGain, pRawReplayCtx->pRawHeader[0].WB_BGain,
		pRawReplayCtx->pRawHeader[0].longExposure, pRawReplayCtx->pRawHeader[0].ISO,
		pRawReplayCtx->pRawHeader[0].exposureRatio, pRawReplayCtx->pRawHeader[0].lightValue);

	s32Ret = CVI_VI_GetDevTimingAttr(0, &stTimingAttr);
	if (s32Ret == CVI_SUCCESS) {
		pRawReplayCtx->timingAttr.bEnable = stTimingAttr.bEnable;
		pRawReplayCtx->timingAttr.s32FrmRate = stTimingAttr.s32FrmRate;
	}
	LOGOUT("\n\nstart raw replay, mode %d (0 manu, 1 auto), framerate %d...\n\n",
		pRawReplayCtx->timingAttr.bEnable, pRawReplayCtx->timingAttr.s32FrmRate);

	pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled = CVI_TRUE;
	pthread_create(&pRawReplayCtx->stGetYuvCtx.getYuvTid, NULL, get_yuv_thread, NULL);

	while (pRawReplayCtx->bRawReplayThreadEnabled) {
		update_video_frame(&stVideoFrame);
#if defined(__CV181X__) || defined(__CV180X__)
		s32Ret = get_rgbmap_buf(pRawReplayCtx);
		ERROR_IF(s32Ret != CVI_SUCCESS);
#endif
		pRawReplayCtx->isRawReplayReady = true;
		if (pRawReplayCtx->u32TotalFrame > 1) {
			if (pRawReplayCtx->bUseDMA) {
				raw_replay_use_dma(&stMWBAttr, &stExpInfo, &stVideoFrame);
			} else {
				raw_replay_use_vb(&stMWBAttr, &stExpInfo, &stVideoFrame);
			}

		} else {
			raw_replay_singlefrmae(&stMWBAttr, &stExpInfo, &stVideoFrame);
		}
		pRawReplayCtx->isRawReplayReady = false;

		while (pRawReplayCtx->bRawReplayThreadSuspend) {
			usleep(500 * 1000);
		}
	}

	pRawReplayCtx->stGetYuvCtx.bGetYuvThreadEnabled = CVI_FALSE;
	pthread_join(pRawReplayCtx->stGetYuvCtx.getYuvTid, NULL);

	LOGOUT("/*** raw replay therad end ***/\n");

	return NULL;
}

#if defined(__CV181X__) || defined(__CV180X__)
static CVI_S32 get_rgbmap_buf(RAW_REPLAY_CTX_S *pCtx)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 fd = -1;

	RETURN_FAILURE_IF(pCtx == NULL);
	fd = get_vi_fd();

	s32Ret = vi_get_rgbmap_le_buf(fd, &pCtx->drvInfo[0]);
	RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);
	pCtx->drvInfo[0].vir_addr = (CVI_U8 *) CVI_SYS_Mmap(pCtx->drvInfo[0].phy_addr,
			pCtx->drvInfo[0].size * 3);
	LOGOUT("rgbmap le vir_addr:%p size:%d\n",
		pCtx->drvInfo[0].vir_addr, pCtx->drvInfo[0].size);

	if (pCtx->pRawHeader[0].enWDR) {
		s32Ret = vi_get_rgbmap_se_buf(fd, &pCtx->drvInfo[1]);
		RETURN_FAILURE_IF(s32Ret != CVI_SUCCESS);
		pCtx->drvInfo[1].vir_addr = (CVI_U8 *) CVI_SYS_Mmap(pCtx->drvInfo[1].phy_addr,
				pCtx->drvInfo[1].size * 3);
		LOGOUT("rgbmap se vir_addr:%p size:%d\n",
			pCtx->drvInfo[1].vir_addr, pCtx->drvInfo[1].size);
	}

	return CVI_SUCCESS;
}

static CVI_S32 get_rgbmap_block_wBit(CVI_S32 iso)
{
	CVI_S32 upper_idx = 0, lower_idx = 0;
	CVI_S32 wBit = 0;
	ISP_TNR_ATTR_S stTNRAttr;

	CVI_ISP_GetTNRAttr(0, &stTNRAttr);
	if (stTNRAttr.enOpType == OP_TYPE_MANUAL) {
		wBit = stTNRAttr.stManual.MtDetectUnit;
	} else {
		for (upper_idx = 0 ; upper_idx < ISP_AUTO_ISO_STRENGTH_NUM - 1 ; upper_idx++) {
			if (iso <= iso_lut[upper_idx])
				break;
		}
		lower_idx = (upper_idx-1 > 0) ? (upper_idx-1) : 0;

		CVI_S32 wBit_l = stTNRAttr.stAuto.MtDetectUnit[lower_idx];
		CVI_S32 wBit_u = stTNRAttr.stAuto.MtDetectUnit[upper_idx];
		CVI_S32 iso_l = iso_lut[lower_idx];
		CVI_S32 iso_u = iso_lut[upper_idx];

		if ((iso <= iso_l) || (lower_idx == upper_idx)) {
			wBit = wBit_l;
		} else if (iso >= iso_u) {
			wBit = wBit_u;
		} else {
			wBit = (wBit_u - wBit_l) * (iso - iso_l) / (iso_u - iso_l) + wBit_l;
		}
	}

	if (wBit < RGBMAP_BLOCK_MIN_UNIT) {
		wBit = RGBMAP_BLOCK_MIN_UNIT;
	}

	return wBit;
}

static void free_rgbmapbuf(void)
{
	if (pRawReplayCtx->rgbMapInfo != NULL) {
		for (CVI_U8 i = 0; i < pRawReplayCtx->u32TotalFrame; i++) {
			RGBMAP_INFO *tmpAddr = pRawReplayCtx->rgbMapInfo + i;

			if (tmpAddr->rgbBuf[0] != NULL) {
				free(tmpAddr->rgbBuf[0]);
				tmpAddr->rgbBuf[0] = NULL;
			}
			if (tmpAddr->rgbBuf[1] != NULL) {
				free(tmpAddr->rgbBuf[1]);
				tmpAddr->rgbBuf[1] = NULL;
			}
		}
		free(pRawReplayCtx->rgbMapInfo);
		pRawReplayCtx->rgbMapInfo = NULL;
	}

	if (!pRawReplayCtx->bRawReplayThreadEnabled) {
		if (pRawReplayCtx->drvInfo[0].vir_addr != NULL) {
			CVI_SYS_Munmap(pRawReplayCtx->drvInfo[0].vir_addr, pRawReplayCtx->drvInfo[0].size * 3);
			pRawReplayCtx->drvInfo[0].vir_addr = NULL;
		}

		if (pRawReplayCtx->drvInfo[1].vir_addr != NULL) {
			CVI_SYS_Munmap(pRawReplayCtx->drvInfo[1].vir_addr, pRawReplayCtx->drvInfo[1].size * 3);
			pRawReplayCtx->drvInfo[1].vir_addr = NULL;
		}
	}
}

static void get_rgbmap_info(RAW_REPLAY_CTX_S *pCtx, RAW_REPLAY_INFO *pRawInfo)
{
	int iWBit = get_rgbmap_block_wBit(pRawInfo->ISO);
	int iHBit = iWBit;
	int iBlkW = 1 << iWBit;
	int iBlkH = 1 << iHBit;
	int mode = pRawInfo->enWDR;
	int width = mode ? pRawInfo->width / 2 : pRawInfo->width;
	int height = pRawInfo->height;
	bool row_duplicate = ((height % iBlkH) != 0);
	bool col_duplicate = ((width  % iBlkW) != 0);
	int gridCol = col_duplicate ? (width / iBlkW + 1) : (width / iBlkW);
	int gridRow = row_duplicate ? (height / iBlkH + 1) : (height / iBlkH);
	int mapSize = gridCol * gridRow * RGBMAP_BLOCK_BYTE;

	RGBMAP_INFO *tmpAddr = pCtx->rgbMapInfo + pRawInfo->curFrame;

	tmpAddr->wBit = iWBit;
	tmpAddr->hBit = iHBit;
	tmpAddr->gridRow = gridRow;
	tmpAddr->gridCol = gridCol;
	tmpAddr->size = mapSize;

	if (mode) {
		tmpAddr->rgbBuf[0] = (CVI_U8 *)calloc(1, mapSize);
		tmpAddr->rgbBuf[1] = (CVI_U8 *)calloc(1, mapSize);
	} else {
		tmpAddr->rgbBuf[0] = (CVI_U8 *)calloc(1, mapSize);
	}

	LOGOUT("Raw Index:%d, ISO:%d, rgbMap info wBit: %d, hBit: %d, gridRow: %d, gridCol: %d, size: %d\n",
		pRawInfo->curFrame, pRawInfo->ISO, tmpAddr->wBit, tmpAddr->hBit, tmpAddr->gridRow,
		tmpAddr->gridCol, tmpAddr->size);
}

static void write_rgbmap(RAW_REPLAY_CTX_S *pCtx, int index)
{
	RGBMAP_INFO *tmpAddr = pCtx->rgbMapInfo + index;

	if (pCtx->pRawHeader[0].enWDR) {
		CVI_U8 *dstLe = pCtx->drvInfo[0].vir_addr + pCtx->rgbMapIdx * pCtx->drvInfo[0].size;
		CVI_U8 *dstSe = pCtx->drvInfo[1].vir_addr + pCtx->rgbMapIdx * pCtx->drvInfo[1].size;
		CVI_U8 *rgbBufLe = tmpAddr->rgbBuf[0];
		CVI_U8 *rgbBufSe = tmpAddr->rgbBuf[1];

		memcpy(dstLe, rgbBufLe, tmpAddr->size);
		memcpy(dstSe, rgbBufSe, tmpAddr->size);
	} else {
		CVI_U8 *dstLe = pCtx->drvInfo[0].vir_addr + pCtx->rgbMapIdx * pCtx->drvInfo[0].size;
		CVI_U8 *rgbBufLe = tmpAddr->rgbBuf[0];

		memcpy(dstLe, rgbBufLe, tmpAddr->size);
	}
}

static void generate_rgbmap_by_frame(RAW_REPLAY_CTX_S *pCtx, int index)
{
	int wdrMode = pCtx->pRawHeader[0].enWDR;
	bool roiMode = pCtx->u32RoiTotalFrame == 0 ? false : true;
	RGBMAP_INFO *tmpAddr = pCtx->rgbMapInfo + index;
	CVI_U8 *rgbBufLe = tmpAddr->rgbBuf[0];
	CVI_U8 *rgbBufSe = tmpAddr->rgbBuf[1];

	if (roiMode && index != 0) {
		if (wdrMode) {
			CVI_U8 *tmpAddrLe = pCtx->pu8RoiVirAddr[0] + (index - 1) * pCtx->u32RoiRawFrameSize / 2;
			CVI_U8 *tmpAddrSe = pCtx->pu8RoiVirAddr[1] + (index - 1) * pCtx->u32RoiRawFrameSize / 2;

			if (pRawReplayCtx->u32RawFrameSize != pCtx->u32RoiRawFrameSize) {
				int lineSize = pCtx->u32RoiRawFrameSize / pCtx->stRoiRect.u32Height / 2;
				CVI_U8 *fullRawBuf = (CVI_U8 *)calloc(1, pRawReplayCtx->u32RawFrameSize / 2);
				int fullLineSize = pCtx->u32RawFrameSize / pCtx->pRawHeader[0].height / 2;
				CVI_U8 *dst = fullRawBuf + pCtx->stRoiRect.s32Y * fullLineSize + pCtx->stRoiRect.s32X / 2 * 3;

				memcpy(fullRawBuf, pCtx->pu8VirAddr[0], pCtx->u32RawFrameSize / 2);
				for (CVI_U32 i = 0; i < pCtx->stRoiRect.u32Height; i++) {
					memcpy(dst + i*fullLineSize, tmpAddrLe + i*lineSize, lineSize);
				}
				rgbmap_proc(fullRawBuf, rgbBufLe, pCtx, index);

				memcpy(fullRawBuf, pCtx->pu8VirAddr[1], pCtx->u32RawFrameSize / 2);
				for (CVI_U32 i = 0; i < pCtx->stRoiRect.u32Height; i++) {
					memcpy(dst + i*fullLineSize, tmpAddrSe + i*lineSize, lineSize);
				}
				rgbmap_proc(fullRawBuf, rgbBufSe, pCtx, index);

				free(fullRawBuf);
			} else {
				rgbmap_proc(tmpAddrLe, rgbBufLe, pCtx, index);
				rgbmap_proc(tmpAddrSe, rgbBufSe, pCtx, index);
			}
		} else {
			CVI_U8 *tmpAddrLe = pCtx->pu8RoiVirAddr[0] + (index - 1) * pCtx->u32RoiRawFrameSize;
			int lineSize = pCtx->u32RoiRawFrameSize / pCtx->stRoiRect.u32Height;

			if (pRawReplayCtx->u32RawFrameSize != pCtx->u32RoiRawFrameSize) {
				CVI_U8 *fullRawBuf = (CVI_U8 *)calloc(1, pRawReplayCtx->u32RawFrameSize);
				int fullLineSize = pCtx->u32RawFrameSize / pCtx->pRawHeader[0].height;
				CVI_U8 *dst = fullRawBuf + pCtx->stRoiRect.s32Y * fullLineSize + pCtx->stRoiRect.s32X / 2 * 3;

				memcpy(fullRawBuf, pCtx->pu8VirAddr[0], pCtx->u32RawFrameSize);
				for (CVI_U32 i = 0; i < pCtx->stRoiRect.u32Height; i++) {
					memcpy(dst + i*fullLineSize, tmpAddrLe + i*lineSize, lineSize);
				}
				rgbmap_proc(fullRawBuf, rgbBufLe, pCtx, index);

				free(fullRawBuf);
			} else {
				rgbmap_proc(tmpAddrLe, rgbBufLe, pCtx, index);
			}
		}
	} else {
		if (wdrMode) {
			CVI_U8 *tmpAddrLe = pCtx->pu8VirAddr[0] + index * pCtx->u32RawFrameSize / 2;
			CVI_U8 *tmpAddrSe = pCtx->pu8VirAddr[1] + index * pCtx->u32RawFrameSize / 2;

			rgbmap_proc(tmpAddrLe, rgbBufLe, pCtx, index);
			rgbmap_proc(tmpAddrSe, rgbBufSe, pCtx, index);
		} else {
			CVI_U8 *tmpAddrLe = pCtx->pu8VirAddr[0] + index * pCtx->u32RawFrameSize;

			rgbmap_proc(tmpAddrLe, rgbBufLe, pCtx, index);
		}
	}
}

static CVI_U32 blc_proc(CVI_S32 val, CVI_S32 offset, CVI_S32 gain, CVI_S32 offset2)
{
	val = MAX(val - offset, 0);
	val = val * gain;
	val = MINMAX((val + (1 << (BLC_MAX_BIT - 1))) >> BLC_MAX_BIT, 0, FRAME_OUT_MAX);
	val = MAX(val - offset2, 0);

	return val;
}

static void rgbmap_proc(CVI_U8 *raw, CVI_U8 *rgbOut, RAW_REPLAY_CTX_S *pCtx, int index)
{
	int width = pCtx->pRawHeader[0].enWDR ? pCtx->pRawHeader[0].width / 2 : pCtx->pRawHeader[0].width;
	int height = pCtx->pRawHeader[0].height;
	int bayerId = pCtx->pRawHeader[0].bayerID;
	RGBMAP_INFO *tmpAddr = pCtx->rgbMapInfo + index;
	int wBit = tmpAddr->wBit;
	int hBit = tmpAddr->hBit;
	int gridRow = tmpAddr->gridRow;
	int gridCol = tmpAddr->gridCol;
	int blkW = 1 << wBit;
	int blkH = 1 << hBit;
	bool row_duplicate = ((height % blkH) != 0);
	bool col_duplicate = ((width  % blkW) != 0);
	int last_row = gridRow - 1;
	int last_col = gridCol - 1;

	for (int by = 0; by < gridRow; by++) {
		for (int bx = 0; bx < gridCol; bx++) {

			int iSumR = 0;
			int iSumG = 0;
			int iSumB = 0;

			// Boundary condition
			int bySel = (row_duplicate && (by == last_row)) ? (by-1) : by;
			int bxSel = (col_duplicate && (bx == last_col)) ? (bx-1) : bx;

			int offset_y = bySel * blkH;
			int offset_x = bxSel * blkW;

			for (int y = 0; y < blkH; y++) {
				for (int x = 0; x < blkW; x++) {

					int iPosY = offset_y + y;
					int iPosX = offset_x + x;

					int eColorID = BAYER_ORDER[bayerId][iPosY % 2][iPosX % 2];

					CVI_U32 val = get_raw_pixel_data(raw, width, iPosX, iPosY);

					switch (eColorID) {
					case RR:
						val = blc_proc(val, pCtx->pRawHeader[index].BLC_Offset[0],
							pCtx->pRawHeader[index].BLC_Gain[0], 0);
						iSumR += val;
						break;
					case BB:
						val = blc_proc(val, pCtx->pRawHeader[index].BLC_Offset[3],
							pCtx->pRawHeader[index].BLC_Gain[3], 0);
						iSumB += val;
						break;
					case GB:
						val = blc_proc(val, pCtx->pRawHeader[index].BLC_Offset[2],
							pCtx->pRawHeader[index].BLC_Gain[2], 0);
						iSumG += val;
						break;
					case GR:
						val = blc_proc(val, pCtx->pRawHeader[index].BLC_Offset[1],
							pCtx->pRawHeader[index].BLC_Gain[1], 0);
						iSumG += val;
						break;
					}
				}
			}

			// Calculate #pixels of a channel in one block
			int iBlkR = (iSumR + (1 << (wBit + hBit - 3))) >> (wBit + hBit - 2);
			int iBlkG = (iSumG + (1 << (wBit + hBit - 2))) >> (wBit + hBit - 1);
			int iBlkB = (iSumB + (1 << (wBit + hBit - 3))) >> (wBit + hBit - 2);

			iBlkR = (iBlkR < FRAME_OUT_MAX) ? iBlkR : FRAME_OUT_MAX;
			iBlkG = (iBlkG < FRAME_OUT_MAX) ? iBlkG : FRAME_OUT_MAX;
			iBlkB = (iBlkB < FRAME_OUT_MAX) ? iBlkB : FRAME_OUT_MAX;

			output_rgbmap_data(rgbOut, gridCol, bx, by, iBlkR, iBlkG, iBlkB);
		}
	}
}

static void output_rgbmap_data(CVI_U8 *buf, int stride, int x, int y, int r, int g, int b)
{
	CVI_U8 *tmpAddr = buf + y * stride * RGBMAP_BLOCK_BYTE + x * RGBMAP_BLOCK_BYTE;

	*tmpAddr++ = b;
	*tmpAddr++ = ((b >> 8) & 0x0f) | ((g & 0x0f) << 4);
	*tmpAddr++ = g >> 4;
	*tmpAddr++ = r;
	*tmpAddr++ = (r >> 8) & 0x0f;
	*tmpAddr++ = 0;
}

static int get_raw_pixel_data(CVI_U8 *buf, int stride, int x, int y)
{
	int value;
	CVI_U8 *tmpAddr = buf + y * stride / 2 * 3 + x / 2 * 3;

	int v0 = *tmpAddr++;
	int v1 = *tmpAddr++;
	int v2 = *tmpAddr++;

	if (x % 2 == 0) {
		value = (v0 << 4) | (v2 & 0x0f);
	} else {
		value = (v1 << 4) | ((v2 >> 4) & 0x0f);
	}

	return value;
}
#endif

static void get_raw_replay_yuv_by_index(CVI_S32 index)
{
	if (index != pRawReplayCtx->stGetYuvCtx.yuvIndex) {
		return;
	}

	pRawReplayCtx->stGetYuvCtx.bEnable = CVI_TRUE;
}

CVI_S32 get_raw_replay_yuv(CVI_U8 yuvIndex, CVI_U8 src, CVI_S32 pipeOrGrp, CVI_S32 chn,
							VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	if (pRawReplayCtx == NULL || pstFrameInfo == NULL) {
		return CVI_FAILURE;
	}

	if (yuvIndex <= 0) {
		LOGOUT("Single frame mode don't support!\n");
		return CVI_FAILURE;
	}

	pRawReplayCtx->stGetYuvCtx.src = src;
	pRawReplayCtx->stGetYuvCtx.pipeOrGrp = pipeOrGrp;
	pRawReplayCtx->stGetYuvCtx.chn = chn;
	pRawReplayCtx->stGetYuvCtx.yuvIndex = yuvIndex;

	LOGOUT("get raw replay yuv+++, src: %d, pipeOrGrp: %d, chn: %d, yuvIndex: %d\n",
		src, pipeOrGrp, chn, yuvIndex);

	sem_wait(&pRawReplayCtx->stGetYuvCtx.sem);

	*pstFrameInfo = pRawReplayCtx->stGetYuvCtx.videoFrame;

	LOGOUT("get raw replay yuv---\n");

	return CVI_SUCCESS;
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif
