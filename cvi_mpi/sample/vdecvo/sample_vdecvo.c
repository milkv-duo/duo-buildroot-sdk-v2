#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <inttypes.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/socket.h>

#include "cvi_buffer.h"
#include "sample_comm.h"
#include "cvi_sys.h"

#define MAX_VDEC_NUM 2

typedef struct _SAMPLE_VDEC_PARAM_S {
	VDEC_CHN        VdecChn;
	VDEC_CHN_ATTR_S stChnAttr;
	CVI_CHAR        decode_file_name[64];
	CVI_BOOL        stop_thread;
	pthread_t       vdec_thread;
	RECT_S          stDispRect;
	CVI_U32         bind_mode;
	VB_SOURCE_E     vdec_vb_source;
	PIXEL_FORMAT_E  vdec_pixel_format;
} SAMPLE_VDEC_PARAM_S;

typedef struct _SAMPLE_VDEC_CONFIG_S {
	CVI_S32 s32ChnNum;
	SAMPLE_VDEC_PARAM_S astVdecParam[MAX_VDEC_NUM];
} SAMPLE_VDEC_CONFIG_S;

static pthread_t send_vo_thread;
static CVI_VOID *s_h264file[MAX_VDEC_NUM];

// #define SHOW_STATISTICS_1
// #define SHOW_STATISTICS_2

CVI_VOID *thread_vdec_send_stream(CVI_VOID *arg)
{
	FILE *fpStrm = NULL;
	SAMPLE_VDEC_PARAM_S *param = (SAMPLE_VDEC_PARAM_S *)arg;
	CVI_BOOL bEndOfStream = CVI_FALSE;
	CVI_S32 s32UsedBytes = 0, s32ReadLen = 0;
	CVI_U8 *pu8Buf = NULL;
	VDEC_STREAM_S stStream;
	CVI_BOOL bFindStart, bFindEnd;
	CVI_U64 u64PTS = 0;
	CVI_U32 u32Len, u32Start;
	CVI_S32 s32Ret, i;
	CVI_S32 bufsize = (param->stChnAttr.u32PicWidth * param->stChnAttr.u32PicHeight * 3) >> 1;
	char strBuf[64];
	int cur_cnt = 0;

	FILE *outFd = NULL;
	outFd = fopen("out.264", "wb");
	CVI_S32 VencChn = 0;
	VENC_STREAM_S stEncStream;
	VENC_CHN_STATUS_S stStat;
#ifdef SHOW_STATISTICS_1
	struct timeval pre_tv, tv1, tv2;
	int pre_cnt = 0;
	int accum_ms = 0;

	pre_tv.tv_usec =
	pre_tv.tv_sec = 0;
#endif

	snprintf(strBuf, sizeof(strBuf), "thread_vdec-%d", param->VdecChn);
	prctl(PR_SET_NAME, strBuf);

    if (param->decode_file_name != 0) {
        fpStrm = fopen(param->decode_file_name, "rb");
        if (fpStrm == NULL) {
            printf("open file err, %s\n", param->decode_file_name);
            return (CVI_VOID *)(CVI_FAILURE);
        }
        pu8Buf = malloc(bufsize);
        if (pu8Buf == NULL) {
            printf("chn %d can't alloc %d in send stream thread!\n",
                    param->VdecChn,
                    bufsize);
            fclose(fpStrm);
            return (CVI_VOID *)(CVI_FAILURE);
        }
    }
    fflush(stdout);

	SAMPLE_PRT("thread_vdec_send_stream %d\n", param->VdecChn);
	u64PTS = 0;
	while (!param->stop_thread) {
		int retry = 0;

		bEndOfStream = CVI_FALSE;
		bFindStart = CVI_FALSE;
		bFindEnd = CVI_FALSE;
		u32Start = 0;
		s32Ret = fseek(fpStrm, s32UsedBytes, SEEK_SET);
		s32ReadLen = fread(pu8Buf, 1, bufsize, fpStrm);
		if (s32ReadLen == 0) {//file end
			memset(&stStream, 0, sizeof(VDEC_STREAM_S));
			stStream.bEndOfStream = CVI_TRUE;
			s32UsedBytes = 0;
			fseek(fpStrm, 0, SEEK_SET);
			s32ReadLen = fread(pu8Buf, 1, bufsize, fpStrm);
		}
		if (param->stChnAttr.enMode == VIDEO_MODE_FRAME &&
				param->stChnAttr.enType == PT_H264) {
			for (i = 0; i < s32ReadLen - 8; i++) {
				int tmp = pu8Buf[i + 3] & 0x1F;

				if (pu8Buf[i] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
				    (((tmp == 0x5 || tmp == 0x1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
				     (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80))) {
					bFindStart = CVI_TRUE;
					i += 8;
					break;
				}
			}

			for (; i < s32ReadLen - 8; i++) {
				int tmp = pu8Buf[i + 3] & 0x1F;

				if (pu8Buf[i] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
				    (tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
				     ((tmp == 5 || tmp == 1) && ((pu8Buf[i + 4] & 0x80) == 0x80)) ||
				     (tmp == 20 && (pu8Buf[i + 7] & 0x80) == 0x80))) {
					bFindEnd = CVI_TRUE;
					break;
				}
			}

			if (i > 0)
				s32ReadLen = i;
			if (bFindStart == CVI_FALSE) {
				CVI_VDEC_TRACE("chn %d can not find H264 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
					    param->VdecChn, s32ReadLen, s32UsedBytes);
			}
			if (bFindEnd == CVI_FALSE) {
				s32ReadLen = i + 8;
			}

		} else if (param->stChnAttr.enMode == VIDEO_MODE_FRAME &&
				param->stChnAttr.enType == PT_H265) {
			CVI_BOOL bNewPic = CVI_FALSE;

			for (i = 0; i < s32ReadLen - 6; i++) {
				CVI_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;

				bNewPic = (pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
					   (tmp <= 21) && ((pu8Buf[i + 5] & 0x80) == 0x80));

				if (bNewPic) {
					bFindStart = CVI_TRUE;
					i += 6;
					break;
				}
			}

			for (; i < s32ReadLen - 6; i++) {
				CVI_U32 tmp = (pu8Buf[i + 3] & 0x7E) >> 1;

				bNewPic = (pu8Buf[i + 0] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
					   (tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 ||
					    ((tmp <= 21) && (pu8Buf[i + 5] & 0x80) == 0x80)));

				if (bNewPic) {
					bFindEnd = CVI_TRUE;
					break;
				}
			}
			if (i > 0)
				s32ReadLen = i;

			if (bFindStart == CVI_FALSE) {
				CVI_VDEC_TRACE("chn 0 can not find H265 start code!s32ReadLen %d, s32UsedBytes %d.!\n",
					    s32ReadLen, s32UsedBytes);
			}
			if (bFindEnd == CVI_FALSE) {
				s32ReadLen = i + 6;
			}

		} else if (param->stChnAttr.enType == PT_MJPEG \
					|| param->stChnAttr.enType == PT_JPEG) {
			for (i = 0; i < s32ReadLen - 1; i++) {
				if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD8) {
					u32Start = i;
					bFindStart = CVI_TRUE;
					i = i + 2;
					break;
				}
			}

			for (; i < s32ReadLen - 3; i++) {
				if ((pu8Buf[i] == 0xFF) && (pu8Buf[i + 1] & 0xF0) == 0xE0) {
					u32Len = (pu8Buf[i + 2] << 8) + pu8Buf[i + 3];
					i += 1 + u32Len;
				} else {
					break;
				}
			}

			for (; i < s32ReadLen - 1; i++) {
				if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD9) {
					bFindEnd = CVI_TRUE;
					break;
				}
			}
			s32ReadLen = i + 2;

			if (bFindStart == CVI_FALSE) {
				CVI_VDEC_TRACE("chn %d can not find JPEG start code!s32ReadLen %d, s32UsedBytes %d.!\n",
					    param->VdecChn, s32ReadLen, s32UsedBytes);
			}
		} else {
			if ((s32ReadLen != 0) && (s32ReadLen < bufsize)) {
				bEndOfStream = CVI_TRUE;
			}
		}

		stStream.u64PTS = u64PTS;
		stStream.pu8Addr = pu8Buf + u32Start;
		stStream.u32Len = s32ReadLen;
		stStream.bEndOfFrame = CVI_TRUE;
		stStream.bEndOfStream = bEndOfStream;
		stStream.bDisplay = 1;
SendAgain:
#ifdef SHOW_STATISTICS_1
		gettimeofday(&tv1, NULL);
#endif
		s32Ret = CVI_VDEC_SendStream(param->VdecChn, &stStream, -1);
		if (s32Ret != CVI_SUCCESS) {
			//printf("%d dec chn CVI_VDEC_SendStream err ret=%d\n",param->vdec_chn,s32Ret);
			retry++;
			if (param->stop_thread)
				break;
			usleep(10000);
			goto SendAgain;
		} else {
			bEndOfStream = CVI_FALSE;
			s32UsedBytes = s32UsedBytes + s32ReadLen + u32Start;
			u64PTS += 1;
			cur_cnt++;
		}
		if (param->bind_mode == VDEC_BIND_VENC) {
			s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
			if (s32Ret != CVI_SUCCESS) {
				CVI_VENC_ERR("CVI_VENC_QueryStatus, Vench = %d, s32Ret = %d\n",
						VencChn, s32Ret);
			}

			if (!stStat.u32CurPacks) {
				CVI_VENC_ERR("u32CurPacks = NULL!\n");
			}
			stEncStream.pstPack =
				(VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
			if (stEncStream.pstPack == NULL) {
				CVI_VENC_ERR("malloc memory failed!\n");
			}

			s32Ret = CVI_VENC_GetStream(VencChn, &stEncStream, -1);
			if (!s32Ret) {
				s32Ret = SAMPLE_COMM_VENC_SaveStream(
						param->stChnAttr.enType,
						outFd,
						&stEncStream);
				if (s32Ret != CVI_SUCCESS) {
					CVI_VENC_ERR("SAMPLE_COMM_VENC_SaveStream, s32Ret = %d\n", s32Ret);
					break;
				}
				CVI_VENC_ReleaseStream(VencChn, &stEncStream);
				free(stEncStream.pstPack);
				stEncStream.pstPack = NULL;
			}
		}
#ifdef SHOW_STATISTICS_1
		gettimeofday(&tv2, NULL);
		int curr_ms =
			(tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec/1000) - (tv1.tv_usec/1000);

		accum_ms += curr_ms;
		if (pre_tv.tv_usec == 0 && pre_tv.tv_sec == 0) {
			pre_tv = tv2;
		} else {
			unsigned long diffus = 0;

			if (pre_tv.tv_sec == tv2.tv_sec) {
				if (tv2.tv_usec > pre_tv.tv_usec) {
					diffus = tv2.tv_usec - pre_tv.tv_usec;
				}
			} else if (tv2.tv_sec > pre_tv.tv_sec) {
				diffus = (tv2.tv_sec - pre_tv.tv_sec) * 1000000;
				diffus = diffus + tv2.tv_usec - pre_tv.tv_usec;
			}

			if (diffus == 0) {
				pre_tv = tv2;
				pre_cnt = cur_cnt;
			} else if (diffus > 1000000) {
				int add_cnt = cur_cnt - pre_cnt;
				double avg_fps = (add_cnt * 1000000.0 / (double) diffus);

				printf("[%d] CVI_VDEC_SendStream Avg. %d ms FPS %.2lf\n"
						, param->VdecChn, accum_ms / add_cnt, avg_fps);
				pre_tv = tv2;
				pre_cnt = cur_cnt;
				accum_ms = 0;
			}
		}
#endif
		usleep(20000);

	}

	/* send the flag of stream end */
	memset(&stStream, 0, sizeof(VDEC_STREAM_S));
	stStream.bEndOfStream = CVI_TRUE;
	CVI_VDEC_SendStream(param->VdecChn, &stStream, -1);

	SAMPLE_PRT("thread_vdec_send_stream %d exit\n", param->VdecChn);

    fflush(stdout);
	if (pu8Buf != CVI_NULL) {
		free(pu8Buf);
	}
	fclose(fpStrm);
	if (param->bind_mode == VDEC_BIND_VENC) {
		fclose(outFd);
	}
	return (CVI_VOID *)CVI_SUCCESS;
}

CVI_S32 set_vpss_AspectRatio(int index, VPSS_GRP VpssGrp, VPSS_CHN VpssChn, RECT_S *rect)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	VPSS_CHN_ATTR_S stChnAttr;

	s32Ret = CVI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VPSS_GetChnAttr is fail\n");
		return s32Ret;
	}
	stChnAttr.stAspectRatio.stVideoRect.s32X = rect->s32X;
	stChnAttr.stAspectRatio.stVideoRect.s32Y = rect->s32Y;
	stChnAttr.stAspectRatio.stVideoRect.u32Width = rect->u32Width;
	stChnAttr.stAspectRatio.stVideoRect.u32Height = rect->u32Height;
	stChnAttr.stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
	if (index == 0) {
		stChnAttr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		stChnAttr.stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
	} else {
		stChnAttr.stAspectRatio.bEnableBgColor = CVI_FALSE;
	}

	s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_SetChnAttr grp %d failed with %#x\n", VpssGrp, s32Ret);
	}

	return s32Ret;
}

// int alarm_sockets[2];
CVI_VOID *thread_send_vo(CVI_VOID *arg)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_BOOL bFirstFrame = CVI_FALSE;
	SAMPLE_VDEC_CONFIG_S *pstVdecCfg = (SAMPLE_VDEC_CONFIG_S *)arg;
	SAMPLE_VDEC_PARAM_S *pstVdecChn;
	VIDEO_FRAME_INFO_S stVdecFrame = {0};
	VIDEO_FRAME_INFO_S stOverlayFrame = {0};
	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = 0;
	VO_LAYER VoLayer = 0;
	VO_CHN VoChn = 0;
	int retry = 0;
#ifdef SHOW_STATISTICS_2
	struct timeval tv1, tv2;
#endif

	SAMPLE_PRT("thread_send_vo %d\n", VoChn);
	prctl(PR_SET_NAME, "thread_send_vo");
#ifdef SHOW_STATISTICS_2
	tv1.tv_usec = tv1.tv_sec = 0;
#endif
	while (!pstVdecCfg->astVdecParam[0].stop_thread) {
		bFirstFrame = CVI_TRUE;
		//vdec frame
		for (CVI_S32 i = 0; i < pstVdecCfg->s32ChnNum; i++) {
			pstVdecChn = &pstVdecCfg->astVdecParam[i];

			if (!bFirstFrame) {
				s32Ret = CVI_VPSS_SendChnFrame(VpssGrp, VpssChn, &stOverlayFrame, 1000);
				if (s32Ret != CVI_SUCCESS) {
					CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stOverlayFrame);
					continue;
				}
				CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stOverlayFrame);
			}
RETRY_GET_FRAME:
			s32Ret = CVI_VDEC_GetFrame(pstVdecChn->VdecChn, &stVdecFrame, -1);
			if (s32Ret != CVI_SUCCESS) {
				CVI_TRACE_LOG(CVI_DBG_ERR, "[%d] CVI_VDEC_GetFrame fail\n", pstVdecChn->VdecChn);
				retry++;
				if (s32Ret == CVI_ERR_VDEC_BUSY || s32Ret == CVI_ERR_VDEC_ERR_INVALID_RET) {
					CVI_TRACE_LOG(CVI_DBG_ERR, "get frame timeout ..in overlay ..retry\n");
					goto RETRY_GET_FRAME;
				}
			}
			set_vpss_AspectRatio(i, VpssGrp, VpssChn, &pstVdecChn->stDispRect);
			s32Ret = CVI_VPSS_SendFrame(VpssGrp, &stVdecFrame, 1000);

			if (s32Ret != CVI_SUCCESS) {
				CVI_VDEC_ReleaseFrame(pstVdecChn->VdecChn, &stVdecFrame);
				continue;
			}

			s32Ret = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, &stOverlayFrame, 1000);
			CVI_VDEC_ReleaseFrame(pstVdecChn->VdecChn, &stVdecFrame);
			if (s32Ret != CVI_SUCCESS) {
				CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VPSS_GetChnFrame fail, grp:%d\n", VpssGrp);
				continue;
			}
			bFirstFrame = CVI_FALSE;
		}

		s32Ret = CVI_VO_SendFrame(VoLayer, VoChn, &stOverlayFrame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VO_SendFrame fail\n");
		}

		s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &stOverlayFrame);
			if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VPSS_ReleaseChnFrame fail\n");
		}
#ifdef SHOW_STATISTICS_2
		gettimeofday(&tv2, NULL);
		int curr_ms =
			(tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec/1000) - (tv1.tv_usec/1000);
		SAMPLE_PRT("CVI_VO_SendFrame delta %d ms try %d times\n", curr_ms, retry);
		tv1 = tv2;
#endif
		retry = 0;
	}
	SAMPLE_PRT("thread_send_vo exit\n");
	return (CVI_VOID *)CVI_SUCCESS;
}

VB_POOL g_ahLocalPicVbPool[MAX_VDEC_NUM] = { [0 ...(MAX_VDEC_NUM - 1)] = VB_INVALID_POOLID };

CVI_S32 vdec_init_vb_pool(VDEC_CHN ChnIndex, SAMPLE_VDEC_ATTR *pastSampleVdec, CVI_BOOL is_user)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_U32 u32BlkSize;
	VB_CONFIG_S stVbConf;

	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt = 1;

	u32BlkSize =
		VDEC_GetPicBufferSize(pastSampleVdec->enType, pastSampleVdec->u32Width,
						pastSampleVdec->u32Height,
						pastSampleVdec->enPixelFormat, DATA_BITWIDTH_8,
						COMPRESS_MODE_NONE);
	stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt = pastSampleVdec->u32FrameBufCnt;
	stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_NONE;
	SAMPLE_PRT("VDec Init Pool[VdecChn%d], u32BlkSize = %d, u32BlkCnt = %d\n", ChnIndex,
			stVbConf.astCommPool[0].u32BlkSize, stVbConf.astCommPool[0].u32BlkCnt);

	if (!is_user) {
		if (stVbConf.u32MaxPoolCnt == 0 && stVbConf.astCommPool[0].u32BlkSize == 0) {
			CVI_SYS_Exit();
			s32Ret = CVI_SYS_Init();
			if (s32Ret != CVI_SUCCESS) {
				CVI_VDEC_ERR("CVI_SYS_Init, %d\n", s32Ret);
				return CVI_FAILURE;
			}
		} else {
			s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
			if (s32Ret != CVI_SUCCESS) {
				CVI_VDEC_ERR("SAMPLE_COMM_SYS_Init, %d\n", s32Ret);
				return CVI_FAILURE;
			}
		}
	} else {
		for (CVI_U32 i = 0; i < stVbConf.u32MaxPoolCnt; i++) {
			g_ahLocalPicVbPool[ChnIndex] = CVI_VB_CreatePool(&stVbConf.astCommPool[0]);

			if (g_ahLocalPicVbPool[ChnIndex] == VB_INVALID_POOLID) {
				CVI_VDEC_ERR("CVI_VB_CreatePool Fail\n");
				return CVI_FAILURE;
			}
		}
		SAMPLE_PRT("CVI_VB_CreatePool : %d, u32BlkSize=0x%x, u32BlkCnt=%d\n",
			g_ahLocalPicVbPool[ChnIndex], stVbConf.astCommPool[0].u32BlkSize, stVbConf.astCommPool[0].u32BlkCnt);
	}

	return s32Ret;
}

CVI_VOID vdec_exit_vb_pool(CVI_VOID)
{
	CVI_S32 i, s32Ret;
	VDEC_MOD_PARAM_S stModParam;
	VB_SOURCE_E vb_source;
	CVI_VDEC_GetModParam(&stModParam);
	vb_source = stModParam.enVdecVBSource;
	if (vb_source != VB_SOURCE_USER)
		return;

	for (i = MAX_VDEC_NUM-1; i >= 0; i--) {
		if (g_ahLocalPicVbPool[i] != VB_INVALID_POOLID) {
			CVI_VDEC_TRACE("CVI_VB_DestroyPool : %d\n", g_ahLocalPicVbPool[i]);

			s32Ret =  CVI_VB_DestroyPool(g_ahLocalPicVbPool[i]);
			if (s32Ret != CVI_SUCCESS) {
				CVI_VDEC_ERR("CVI_VB_DestroyPool : %d fail!\n", g_ahLocalPicVbPool[i]);
			}

			g_ahLocalPicVbPool[i] = VB_INVALID_POOLID;
		}
	}
}

CVI_S32 start_vdec(SAMPLE_VDEC_PARAM_S *pVdecParam)
{
	VDEC_CHN_PARAM_S stChnParam = {0};
	VDEC_MOD_PARAM_S stModParam;
	CVI_S32 s32Ret = CVI_SUCCESS;
	VDEC_CHN VdecChn = pVdecParam->VdecChn;

    SAMPLE_PRT("VdecChn = %d\n", VdecChn);

	CVI_VDEC_GetModParam(&stModParam);
	stModParam.enVdecVBSource = pVdecParam->vdec_vb_source;
	CVI_VDEC_SetModParam(&stModParam);

	s32Ret = CVI_VDEC_CreateChn(VdecChn, &pVdecParam->stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_CreateChn chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
		return s32Ret;
	}

	if (pVdecParam->vdec_vb_source == VB_SOURCE_USER) {
		VDEC_CHN_POOL_S stPool;

		stPool.hPicVbPool = g_ahLocalPicVbPool[VdecChn];
		stPool.hTmvVbPool = VB_INVALID_POOLID;

		s32Ret = CVI_VDEC_AttachVbPool(VdecChn, &stPool);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VDEC_AttachVbPool chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
			return s32Ret;
		}
	}

	s32Ret = CVI_VDEC_GetChnParam(VdecChn, &stChnParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_GetChnParam chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
		return s32Ret;
	}
	stChnParam.enPixelFormat = pVdecParam->vdec_pixel_format;
	stChnParam.u32DisplayFrameNum =
		(pVdecParam->stChnAttr.enType == PT_JPEG || pVdecParam->stChnAttr.enType == PT_MJPEG) ? 0 : 2;
	s32Ret = CVI_VDEC_SetChnParam(VdecChn, &stChnParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_SetChnParam chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VDEC_StartRecvStream(VdecChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_StartRecvStream chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
		return s32Ret;
	}

	return CVI_SUCCESS;
}


CVI_VOID stop_vdec(SAMPLE_VDEC_PARAM_S *pVdecParam)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_VDEC_StopRecvStream(pVdecParam->VdecChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_StopRecvStream chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
	}

	if (pVdecParam->vdec_vb_source == VB_SOURCE_USER) {
		CVI_VDEC_TRACE("detach in user mode\n");
		s32Ret = CVI_VDEC_DetachVbPool(pVdecParam->VdecChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VDEC_DetachVbPool chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
		}
	}

	s32Ret = CVI_VDEC_ResetChn(pVdecParam->VdecChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_ResetChn chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
	}

	s32Ret = CVI_VDEC_DestroyChn(pVdecParam->VdecChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_DestroyChn chn[%d] failed for %#x!\n", pVdecParam->VdecChn, s32Ret);
	}
}


CVI_S32 start_thread(SAMPLE_VDEC_CONFIG_S *pstVdecCfg)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	struct sched_param param;
	pthread_attr_t attr;

	param.sched_priority = 80;
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	for (int i = 0; i < pstVdecCfg->s32ChnNum; i++) {
		s32Ret = pthread_create(&pstVdecCfg->astVdecParam[i].vdec_thread, &attr,
			thread_vdec_send_stream, (CVI_VOID *)&pstVdecCfg->astVdecParam[i]);
		if (s32Ret != 0) {
			return CVI_FAILURE;
		}
		usleep(100000);
	}
	usleep(100000);
	if (pstVdecCfg->astVdecParam[0].bind_mode == VDEC_BIND_DISABLE) {
		s32Ret = pthread_create(&send_vo_thread, &attr, thread_send_vo, (CVI_VOID *)pstVdecCfg);
		if (s32Ret != 0) {
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_VOID stop_thread(SAMPLE_VDEC_CONFIG_S *pstVdecCfg)
{
	pstVdecCfg->astVdecParam[0].stop_thread = CVI_TRUE;
	if (send_vo_thread != 0)
		pthread_join(send_vo_thread, NULL);

	for (int i = 0; i < pstVdecCfg->s32ChnNum; i++) {
		pstVdecCfg->astVdecParam[i].stop_thread = CVI_TRUE;
		if (pstVdecCfg->astVdecParam[i].vdec_thread != 0)
			pthread_join(pstVdecCfg->astVdecParam[i].vdec_thread, NULL);
	}

}

PAYLOAD_TYPE_E find_file_type(const char *filename, int filelen)
{
	if (strcmp(filename + filelen - 3, "265") == 0) {
		return PT_H265;
	} else if (strcmp(filename + filelen - 3, "264") == 0) {
		return PT_H264;
	} else if (strcmp(filename + filelen - 3, "jpg") == 0) {
		return PT_JPEG;
	} else if (strcmp(filename + filelen - 3, "mjp") == 0) {
		return PT_MJPEG;
	} else {
		return PT_BUTT;
	}
}

CVI_S32 SAMPLE_VDEC_SEND_VPSS_SEND_VO(CVI_S32 decoding_file_num)
{
	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;

	VB_CONFIG_S        stVbConf;
	CVI_U32	       u32BlkSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	int filelen;

#ifndef VDEC_WIDTH
#define VDEC_WIDTH 1920
#endif
#ifndef VDEC_HEIGHT
#define VDEC_HEIGHT 1080
#endif
#ifndef VPSS_WIDTH
#define VPSS_WIDTH 1280
#endif
#ifndef VPSS_HEIGHT
#define VPSS_HEIGHT 720
#endif
#ifndef VO_WIDTH
#define VO_WIDTH 720
#endif
#ifndef VO_HEIGHT
#define VO_HEIGHT 1280
#endif

	stSize.u32Width = VDEC_WIDTH;
	stSize.u32Height = VDEC_HEIGHT;

	/************************************************
	 * step1:  Init SYS and common VB
	 ************************************************/
	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt		= 2;

	u32BlkSize = VDEC_GetPicBufferSize(PT_JPEG, stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8
					    , COMPRESS_MODE_NONE);
	stVbConf.astCommPool[0].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt	= 5;
	stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_NONE;

	u32BlkSize = COMMON_GetPicBufferSize(VO_WIDTH, VO_HEIGHT, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8
					    , enCompressMode, DEFAULT_ALIGN);
	stVbConf.astCommPool[1].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[1].u32BlkCnt	= 5;
	stVbConf.astCommPool[1].enRemapMode = VB_REMAP_MODE_NONE;
	SAMPLE_PRT("common pool[0] BlkSize %d\n", u32BlkSize);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("system init failed with %#x\n", s32Ret);
		return -1;
	}

	/************************************************
	 * step2:  Init VPSS
	 ************************************************/
	VPSS_GRP	       VpssGrp	= 0;
	VPSS_GRP_ATTR_S    stVpssGrpAttr = {0};
	VPSS_CHN           VpssChn = VPSS_CHN0;
	CVI_BOOL           abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM] = {0};

	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = PIXEL_FORMAT_YUV_PLANAR_420;
	stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
	stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	astVpssChnAttr[VpssChn].u32Width                    = VPSS_WIDTH;
	astVpssChnAttr[VpssChn].u32Height                   = VPSS_HEIGHT;
	astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
	astVpssChnAttr[VpssChn].enPixelFormat               = SAMPLE_PIXEL_FORMAT;
	astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
	astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
	astVpssChnAttr[VpssChn].u32Depth                    = 1;
	astVpssChnAttr[VpssChn].bMirror                     = CVI_FALSE;
	astVpssChnAttr[VpssChn].bFlip                       = CVI_FALSE;
	astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

	abChnEnable[0] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

    /************************************************
    * step3:  Init VO
    ************************************************/
    SAMPLE_VO_CONFIG_S stVoConfig;
	RECT_S stDefDispRect  = {0, 0, VO_WIDTH, VO_HEIGHT};
	SIZE_S stDefImageSize = {VO_WIDTH, VO_HEIGHT};
	VO_DEV VoDev = 0;
	VO_CHN VoChn = 0;

	s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
		return s32Ret;
	}

	stVoConfig.VoDev		 = VoDev;
	stVoConfig.stVoPubAttr.enIntfType  = VO_INTF_MIPI;
	stVoConfig.stVoPubAttr.enIntfSync  = VO_OUTPUT_720x1280_60;
	stVoConfig.stDispRect	 = stDefDispRect;
	stVoConfig.stImageSize	 = stDefImageSize;
	stVoConfig.enPixFormat	 = SAMPLE_PIXEL_FORMAT;
	stVoConfig.enVoMode		 = VO_MODE_1MUX;

	s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
		return s32Ret;
	}

	CVI_VO_SetChnRotation(VoDev, VoChn, ROTATION_90);

    /************************************************
    * step4:  Init VDEC
    ************************************************/
	SAMPLE_VDEC_CONFIG_S stVdecCfg = {0};
	SAMPLE_VDEC_PARAM_S *pVdecChn[MAX_VDEC_NUM];

	stVdecCfg.s32ChnNum = decoding_file_num;
	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		pVdecChn[i] = &stVdecCfg.astVdecParam[i];
		pVdecChn[i]->VdecChn = i;
		pVdecChn[i]->stop_thread = CVI_FALSE;
		pVdecChn[i]->bind_mode = VDEC_BIND_DISABLE;
		filelen = snprintf(pVdecChn[i]->decode_file_name, 63, "%s", (char *)s_h264file[i]);
		pVdecChn[i]->stChnAttr.enType = find_file_type(pVdecChn[i]->decode_file_name, filelen);
		pVdecChn[i]->stChnAttr.enMode = VIDEO_MODE_FRAME;
		pVdecChn[i]->stChnAttr.u32PicWidth = VDEC_WIDTH;
		pVdecChn[i]->stChnAttr.u32PicHeight = VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32StreamBufSize = VDEC_WIDTH*VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32FrameBufCnt = 4;
		if (pVdecChn[i]->stChnAttr.enType == PT_JPEG \
			|| pVdecChn[i]->stChnAttr.enType == PT_MJPEG) {
			pVdecChn[i]->stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(
					pVdecChn[i]->stChnAttr.enType, pVdecChn[i]->stChnAttr.u32PicWidth, \
					pVdecChn[i]->stChnAttr.u32PicHeight, PIXEL_FORMAT_YUV_PLANAR_444, \
					DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
		}
		pVdecChn[i]->stDispRect.s32X = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1)) * i;
		pVdecChn[i]->stDispRect.s32Y = 0;
		pVdecChn[i]->stDispRect.u32Width = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1));
		pVdecChn[i]->stDispRect.u32Height = 720;
		pVdecChn[i]->vdec_vb_source = VB_SOURCE_COMMON;
		pVdecChn[i]->vdec_pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
	}

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		start_vdec(pVdecChn[i]);
	}

	start_thread(&stVdecCfg);

	PAUSE();

	stop_thread(&stVdecCfg);
	SAMPLE_PRT("stop thread\n");
	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		stop_vdec(pVdecChn[i]);
	}

	SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);

	SAMPLE_COMM_VO_StopVO(&stVoConfig);

	vdec_exit_vb_pool();

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

CVI_S32 SAMPLE_VDEC_BIND_VPSS_BIND_VO(CVI_S32 decoding_file_num)
{
	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;
	VB_CONFIG_S        stVbConf;
	CVI_U32	       u32BlkSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	int filelen;

#ifndef VDEC_WIDTH
#define VDEC_WIDTH 1920
#endif
#ifndef VDEC_HEIGHT
#define VDEC_HEIGHT 1080
#endif
#ifndef VPSS_WIDTH
#define VPSS_WIDTH 1280
#endif
#ifndef VPSS_HEIGHT
#define VPSS_HEIGHT 720
#endif
#ifndef VO_WIDTH
#define VO_WIDTH 720
#endif
#ifndef VO_HEIGHT
#define VO_HEIGHT 1280
#endif

	stSize.u32Width = VDEC_WIDTH;
	stSize.u32Height = VDEC_HEIGHT;

	/************************************************
	 * step1:  Init SYS and common VB
	 ************************************************/
	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt		= 1;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8
					    , enCompressMode, DEFAULT_ALIGN);
	stVbConf.astCommPool[0].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt	= 5;
	stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_NONE;
	SAMPLE_PRT("common pool[0] BlkSize %d\n", u32BlkSize);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("system init failed with %#x\n", s32Ret);
		return -1;
	}

	/************************************************
	 * step2:  Init VPSS
	 ************************************************/
	VPSS_GRP	       VpssGrp	= 0;
	VPSS_GRP_ATTR_S    stVpssGrpAttr = {0};
	VPSS_CHN           VpssChn = VPSS_CHN0;
	CVI_BOOL           abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM] = {0};

	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = PIXEL_FORMAT_YUV_PLANAR_420;
	stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
	stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	astVpssChnAttr[VpssChn].u32Width                    = VPSS_WIDTH;
	astVpssChnAttr[VpssChn].u32Height                   = VPSS_HEIGHT;
	astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
	astVpssChnAttr[VpssChn].enPixelFormat               = SAMPLE_PIXEL_FORMAT;
	astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
	astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
	astVpssChnAttr[VpssChn].u32Depth                    = 1;
	astVpssChnAttr[VpssChn].bMirror                     = CVI_FALSE;
	astVpssChnAttr[VpssChn].bFlip                       = CVI_FALSE;
	astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

	abChnEnable[0] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

    /************************************************
    * step3:  Init VO
    ************************************************/
    SAMPLE_VO_CONFIG_S stVoConfig;
	RECT_S stDefDispRect  = {0, 0, VO_WIDTH, VO_HEIGHT};
	SIZE_S stDefImageSize = {VO_WIDTH, VO_HEIGHT};
	VO_DEV VoDev = 0;
	VO_CHN VoChn = 0;

	s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
		return s32Ret;
	}

	stVoConfig.VoDev		 = VoDev;
	stVoConfig.stVoPubAttr.enIntfType  = VO_INTF_MIPI;
	stVoConfig.stVoPubAttr.enIntfSync  = VO_OUTPUT_720x1280_60;
	stVoConfig.stDispRect	 = stDefDispRect;
	stVoConfig.stImageSize	 = stDefImageSize;
	stVoConfig.enPixFormat	 = SAMPLE_PIXEL_FORMAT;
	stVoConfig.enVoMode		 = VO_MODE_1MUX;

	s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
		return s32Ret;
	}

	CVI_VO_SetChnRotation(VoDev, VoChn, ROTATION_90);

    /************************************************
    * step4:  Init VDEC
    ************************************************/
	SAMPLE_VDEC_CONFIG_S stVdecCfg = {0};
	SAMPLE_VDEC_PARAM_S *pVdecChn[MAX_VDEC_NUM];
	stVdecCfg.s32ChnNum = decoding_file_num;

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		pVdecChn[i] = &stVdecCfg.astVdecParam[i];
		pVdecChn[i]->VdecChn = i;
		pVdecChn[i]->stop_thread = CVI_FALSE;
		pVdecChn[i]->bind_mode = VDEC_BIND_VPSS_VO;
		filelen = snprintf(pVdecChn[i]->decode_file_name, 63, "%s", (char *)s_h264file[i]);
		pVdecChn[i]->stChnAttr.enType = find_file_type(pVdecChn[i]->decode_file_name, filelen);
		pVdecChn[i]->stChnAttr.enMode = VIDEO_MODE_FRAME;
		pVdecChn[i]->stChnAttr.u32PicWidth = VDEC_WIDTH;
		pVdecChn[i]->stChnAttr.u32PicHeight = VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32StreamBufSize = VDEC_WIDTH*VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32FrameBufCnt = 1;
		if (pVdecChn[i]->stChnAttr.enType == PT_JPEG \
			|| pVdecChn[i]->stChnAttr.enType == PT_MJPEG) {
			pVdecChn[i]->stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(
					pVdecChn[i]->stChnAttr.enType, pVdecChn[i]->stChnAttr.u32PicWidth, \
					pVdecChn[i]->stChnAttr.u32PicHeight, PIXEL_FORMAT_YUV_PLANAR_444, \
					DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
		}
		pVdecChn[i]->stDispRect.s32X = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1)) * i;
		pVdecChn[i]->stDispRect.s32Y = 0;
		pVdecChn[i]->stDispRect.u32Width = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1));
		pVdecChn[i]->stDispRect.u32Height = 720;
		pVdecChn[i]->vdec_vb_source = VB_SOURCE_USER;
		pVdecChn[i]->vdec_pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
	}

	////////////////////////////////////////////////////
	// init VB(for VDEC)
	////////////////////////////////////////////////////
	SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		astSampleVdec[i].enType = pVdecChn[i]->stChnAttr.enType;
		astSampleVdec[i].u32Width = VDEC_WIDTH;
		astSampleVdec[i].u32Height = VDEC_HEIGHT;

		astSampleVdec[i].enMode = VIDEO_MODE_FRAME;
		astSampleVdec[i].stSampleVdecVideo.enDecMode = VIDEO_DEC_MODE_IP;
		astSampleVdec[i].stSampleVdecVideo.enBitWidth = DATA_BITWIDTH_8;
		astSampleVdec[i].stSampleVdecVideo.u32RefFrameNum = 2;
		astSampleVdec[i].u32DisplayFrameNum =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 0 : 2;
		astSampleVdec[i].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
		astSampleVdec[i].u32FrameBufCnt =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 1 : 4;

		s32Ret = vdec_init_vb_pool(i, &astSampleVdec[i], CVI_TRUE);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VDEC_ERR("SAMPLE_COMM_VDEC_InitVBPool fail\n");
		}
	}

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		start_vdec(pVdecChn[i]);
	}

	SAMPLE_COMM_VDEC_Bind_VPSS(0, VpssGrp);
	SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, VoDev, VoChn);
	start_thread(&stVdecCfg);

	PAUSE();

	stop_thread(&stVdecCfg);
	SAMPLE_PRT("stop thread\n");
	SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, VoDev, VoChn);
	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		SAMPLE_COMM_VDEC_UnBind_VPSS(i, VpssGrp);
		stop_vdec(pVdecChn[i]);
	}
	SAMPLE_COMM_VO_StopVO(&stVoConfig);
	SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);

	vdec_exit_vb_pool();

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

CVI_S32 SAMPLE_VDEC_BIND_VENC(CVI_S32 decoding_file_num)
{

	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;
	VB_CONFIG_S        stVbConf;
	CVI_U32	       u32BlkSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	int filelen;
	CVI_S32 VencChn = 0;

#ifndef VDEC_WIDTH
#define VDEC_WIDTH 1920
#endif
#ifndef VDEC_HEIGHT
#define VDEC_HEIGHT 1080
#endif


	stSize.u32Width = VDEC_WIDTH;
	stSize.u32Height = VDEC_HEIGHT;

	/************************************************
	 * step1:  Init SYS and common VB
	 ************************************************/
	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt		= 1;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8
					    , enCompressMode, DEFAULT_ALIGN);
	stVbConf.astCommPool[0].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt	= 1;
	stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_NONE;
	SAMPLE_PRT("common pool[0] BlkSize %d\n", u32BlkSize);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("system init failed with %#x\n", s32Ret);
		return -1;
	}

    /************************************************
    * step2:  Init VDEC
    ************************************************/
	SAMPLE_VDEC_CONFIG_S stVdecCfg = {0};
	SAMPLE_VDEC_PARAM_S *pVdecChn[MAX_VDEC_NUM];
	stVdecCfg.s32ChnNum = decoding_file_num;

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		pVdecChn[i] = &stVdecCfg.astVdecParam[i];
		pVdecChn[i]->VdecChn = i;
		pVdecChn[i]->stop_thread = CVI_FALSE;
		pVdecChn[i]->bind_mode = VDEC_BIND_VENC;
		filelen = snprintf(pVdecChn[i]->decode_file_name, 63, "%s", (char *)s_h264file[i]);
		pVdecChn[i]->stChnAttr.enType = find_file_type(pVdecChn[i]->decode_file_name, filelen);
		pVdecChn[i]->stChnAttr.enMode = VIDEO_MODE_FRAME;
		pVdecChn[i]->stChnAttr.u32PicWidth = VDEC_WIDTH;
		pVdecChn[i]->stChnAttr.u32PicHeight = VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32StreamBufSize = VDEC_WIDTH*VDEC_HEIGHT;
		pVdecChn[i]->stChnAttr.u32FrameBufCnt = 1;
		if (pVdecChn[i]->stChnAttr.enType == PT_JPEG \
			|| pVdecChn[i]->stChnAttr.enType == PT_MJPEG) {
			pVdecChn[i]->stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(
					pVdecChn[i]->stChnAttr.enType, pVdecChn[i]->stChnAttr.u32PicWidth, \
					pVdecChn[i]->stChnAttr.u32PicHeight, PIXEL_FORMAT_YUV_PLANAR_444, \
					DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
		}
		pVdecChn[i]->stDispRect.s32X = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1)) * i;
		pVdecChn[i]->stDispRect.s32Y = 0;
		pVdecChn[i]->stDispRect.u32Width = (VPSS_WIDTH >> (stVdecCfg.s32ChnNum - 1));
		pVdecChn[i]->stDispRect.u32Height = 720;
		pVdecChn[i]->vdec_vb_source = VB_SOURCE_USER;
		pVdecChn[i]->vdec_pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
	}
	/************************************************
    * step3:  Init VENC
    ************************************************/
	VENC_CHN_ATTR_S stVencChnAttr, *pstVencChnAttr = &stVencChnAttr;
	memset(pstVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	pstVencChnAttr->stVencAttr.enType = PT_H264;
	pstVencChnAttr->stVencAttr.u32MaxPicWidth = VDEC_WIDTH;
	pstVencChnAttr->stVencAttr.u32MaxPicHeight = VDEC_HEIGHT;
	pstVencChnAttr->stVencAttr.u32PicWidth = VDEC_WIDTH;
	pstVencChnAttr->stVencAttr.u32PicHeight = VDEC_HEIGHT;
	pstVencChnAttr->stVencAttr.u32BufSize = 0x30000;
	pstVencChnAttr->stVencAttr.bByFrame = CVI_TRUE;
	pstVencChnAttr->stVencAttr.bEsBufQueueEn = CVI_TRUE;
	pstVencChnAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	pstVencChnAttr->stGopAttr.stNormalP.s32IPQpDelta = 2;

	VENC_H264_CBR_S *pstH264Cbr = &pstVencChnAttr->stRcAttr.stH264Cbr;

	pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
	pstH264Cbr->u32Gop = 30;
	pstH264Cbr->u32StatTime = 0;
	pstH264Cbr->u32SrcFrameRate = 25;
	pstH264Cbr->fr32DstFrameRate = 25;
	pstH264Cbr->bVariFpsEn = 0;
	pstH264Cbr->u32BitRate = 2000;
	s32Ret = CVI_VENC_CreateChn(VencChn, &stVencChnAttr);

	VENC_PARAM_MOD_S stModParam;
	CVI_VENC_GetModParam(&stModParam);
	stModParam.stH264eModParam.enH264eVBSource = VB_SOURCE_COMMON;
	stModParam.stH264eModParam.bSingleEsBuf = true;
	stModParam.stH264eModParam.u32SingleEsBufSize = 0x30000;
	stModParam.stH264eModParam.u32UserDataMaxLen = 3072;
	CVI_VENC_SetModParam(&stModParam);

	////////////////////////////////////////////////////
	// init VB(for VDEC)
	////////////////////////////////////////////////////
	SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		astSampleVdec[i].enType = pVdecChn[i]->stChnAttr.enType;
		astSampleVdec[i].u32Width = VDEC_WIDTH;
		astSampleVdec[i].u32Height = VDEC_HEIGHT;

		astSampleVdec[i].enMode = VIDEO_MODE_FRAME;
		astSampleVdec[i].stSampleVdecVideo.enDecMode = VIDEO_DEC_MODE_IP;
		astSampleVdec[i].stSampleVdecVideo.enBitWidth = DATA_BITWIDTH_8;
		astSampleVdec[i].stSampleVdecVideo.u32RefFrameNum = 2;
		astSampleVdec[i].u32DisplayFrameNum =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 0 : 2;
		astSampleVdec[i].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
		astSampleVdec[i].u32FrameBufCnt =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 1 : 4;

		s32Ret = vdec_init_vb_pool(i, &astSampleVdec[i], CVI_FALSE);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VDEC_ERR("SAMPLE_COMM_VDEC_InitVBPool fail\n");
		}
	}

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		start_vdec(pVdecChn[i]);
		SAMPLE_COMM_VDEC_Bind_VENC(i, VencChn);
	}


	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	CVI_VENC_StartRecvFrame(VencChn, &stRecvParam);
	start_thread(&stVdecCfg);

	PAUSE();

	stop_thread(&stVdecCfg);
	SAMPLE_PRT("stop thread\n");



	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		SAMPLE_COMM_VDEC_UnBind_VENC(i, VencChn);
		stop_vdec(pVdecChn[i]);
	}

	CVI_VENC_StopRecvFrame(VencChn);
	CVI_VENC_ResetChn(VencChn);
	CVI_VENC_DestroyChn(VencChn);

	vdec_exit_vb_pool();

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

CVI_S32 SAMPLE_VDEC_BIND_VO(CVI_S32 decoding_file_num)
{
	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;
	VB_CONFIG_S        stVbConf;
	CVI_U32	       u32BlkSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	int filelen;

#ifndef VO_WIDTH
#define VO_WIDTH 720
#endif
#ifndef VO_HEIGHT
#define VO_HEIGHT 1280
#endif

	stSize.u32Width = VDEC_WIDTH;
	stSize.u32Height = VDEC_HEIGHT;

	/************************************************
	 * step1:  Init SYS and common VB
	 ************************************************/
	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt		= 1;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8
					    , enCompressMode, DEFAULT_ALIGN);
	stVbConf.astCommPool[0].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt	= 5;
	stVbConf.astCommPool[0].enRemapMode = VB_REMAP_MODE_NONE;
	SAMPLE_PRT("common pool[0] BlkSize %d\n", u32BlkSize);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("system init failed with %#x\n", s32Ret);
		return -1;
	}

    /************************************************
    * step2:  Init VO
    ************************************************/
    SAMPLE_VO_CONFIG_S stVoConfig;
	RECT_S stDefDispRect  = {0, 0, VO_WIDTH, VO_HEIGHT};
	SIZE_S stDefImageSize = {VO_WIDTH, VO_HEIGHT};
	VO_DEV VoDev = 0;
	VO_CHN VoChn = 0;

	s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
		return s32Ret;
	}

	stVoConfig.VoDev		 = VoDev;
	stVoConfig.stVoPubAttr.enIntfType  = VO_INTF_MIPI;
	stVoConfig.stVoPubAttr.enIntfSync  = VO_OUTPUT_720x1280_60;
	stVoConfig.stDispRect	 = stDefDispRect;
	stVoConfig.stImageSize	 = stDefImageSize;
	stVoConfig.enPixFormat	 = SAMPLE_PIXEL_FORMAT;
	stVoConfig.enVoMode		 = VO_MODE_1MUX;

	s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
		return s32Ret;
	}

	CVI_VO_SetChnRotation(VoDev, VoChn, ROTATION_90);

    /************************************************
    * step3:  Init VDEC
    ************************************************/
	SAMPLE_VDEC_CONFIG_S stVdecCfg = {0};
	SAMPLE_VDEC_PARAM_S *pVdecChn[MAX_VDEC_NUM];
	stVdecCfg.s32ChnNum = decoding_file_num;

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		pVdecChn[i] = &stVdecCfg.astVdecParam[i];
		pVdecChn[i]->VdecChn = i;
		pVdecChn[i]->stop_thread = CVI_FALSE;
		pVdecChn[i]->bind_mode = VDEC_BIND_VO;
		filelen = snprintf(pVdecChn[i]->decode_file_name, 63, "%s", (char *)s_h264file[i]);
		pVdecChn[i]->stChnAttr.enType = find_file_type(pVdecChn[i]->decode_file_name, filelen);
		pVdecChn[i]->stChnAttr.enMode = VIDEO_MODE_FRAME;
		pVdecChn[i]->stChnAttr.u32PicWidth = VO_WIDTH;
		pVdecChn[i]->stChnAttr.u32PicHeight = VO_HEIGHT;
		pVdecChn[i]->stChnAttr.u32StreamBufSize = VO_WIDTH*VO_HEIGHT;
		pVdecChn[i]->stChnAttr.u32FrameBufCnt = 1;
		if (pVdecChn[i]->stChnAttr.enType == PT_JPEG \
			|| pVdecChn[i]->stChnAttr.enType == PT_MJPEG) {
			pVdecChn[i]->stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(
					pVdecChn[i]->stChnAttr.enType, pVdecChn[i]->stChnAttr.u32PicWidth, \
					pVdecChn[i]->stChnAttr.u32PicHeight, PIXEL_FORMAT_YUV_PLANAR_444, \
					DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
		}
		pVdecChn[i]->stDispRect.s32X = (VO_WIDTH >> (stVdecCfg.s32ChnNum - 1)) * i;
		pVdecChn[i]->stDispRect.s32Y = 0;
		pVdecChn[i]->stDispRect.u32Width = (VO_WIDTH >> (stVdecCfg.s32ChnNum - 1));
		pVdecChn[i]->stDispRect.u32Height = VO_HEIGHT;
		pVdecChn[i]->vdec_vb_source = VB_SOURCE_USER;
		pVdecChn[i]->vdec_pixel_format = PIXEL_FORMAT_NV21;
	}

	////////////////////////////////////////////////////
	// init VB(for VDEC)
	////////////////////////////////////////////////////
	SAMPLE_VDEC_ATTR astSampleVdec[VDEC_MAX_CHN_NUM];

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		astSampleVdec[i].enType = pVdecChn[i]->stChnAttr.enType;
		astSampleVdec[i].u32Width = VO_WIDTH;
		astSampleVdec[i].u32Height = VO_HEIGHT;

		astSampleVdec[i].enMode = VIDEO_MODE_FRAME;
		astSampleVdec[i].stSampleVdecVideo.enDecMode = VIDEO_DEC_MODE_IP;
		astSampleVdec[i].stSampleVdecVideo.enBitWidth = DATA_BITWIDTH_8;
		astSampleVdec[i].stSampleVdecVideo.u32RefFrameNum = 2;
		astSampleVdec[i].u32DisplayFrameNum =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 0 : 2;
		astSampleVdec[i].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
		astSampleVdec[i].u32FrameBufCnt =
			(astSampleVdec[i].enType == PT_JPEG || astSampleVdec[i].enType == PT_MJPEG) ? 1 : 7;

		s32Ret = vdec_init_vb_pool(i, &astSampleVdec[i], CVI_TRUE);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VDEC_ERR("SAMPLE_COMM_VDEC_InitVBPool fail\n");
		}
	}

	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		start_vdec(pVdecChn[i]);
		SAMPLE_COMM_VDEC_Bind_VO(i, VoDev, VoChn);
	}

	start_thread(&stVdecCfg);

	PAUSE();

	stop_thread(&stVdecCfg);
	SAMPLE_PRT("stop thread\n");
	for (int i = 0; i < stVdecCfg.s32ChnNum; i++) {
		SAMPLE_COMM_VDEC_UnBind_VO(i, VoDev, VoChn);
		stop_vdec(pVdecChn[i]);
	}
	SAMPLE_COMM_VO_StopVO(&stVoConfig);

	vdec_exit_vb_pool();

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

CVI_VOID SAMPLE_VDECVO_HandleSig(CVI_S32 signo)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if (SIGINT == signo || SIGTERM == signo) {
		//todo for release
		SAMPLE_PRT("Program termination abnormally\n");
	}
    exit(1);
}

CVI_VOID SAMPLE_VDECVO_Usage(CVI_CHAR *sPrgNm)
{
	printf("Usage : %s <case_number> <file1> <file2>\n", sPrgNm);
	printf("\n");
	printf("Note: support h264 stream.\n");
	printf("Note: support jpeg & mjpeg stream.\n");
	printf("Note: run sample_panel first to init panel.\n");
	printf("\n");
	printf("Example: ./sample_vdecvo 1 res/enc-1080p.264 res/enc-1080p.264\n");
	printf("\n");
	printf("case_number:\n");
	printf("\t 0)  VDEC chn bind VPSS bind VO\n");
	printf("\t 1)  VDEC chns send to VPSS send to VO\n");
	printf("\t 2)  VDEC chn bind VENC\n");
	printf("\t 3)  VDEC chn bind VO\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	CVI_S32 s32Ret = CVI_FAILURE;
	CVI_S32 s32CaseNumber;
	CVI_S32 decoding_file_num;

	decoding_file_num = argc - 2;
	if (decoding_file_num < 1 || decoding_file_num > MAX_VDEC_NUM) {
		SAMPLE_VDECVO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (!strncmp(argv[1], "-h", 2)) {
		SAMPLE_VDECVO_Usage(argv[0]);
		return CVI_SUCCESS;
	}

	s32CaseNumber = atoi(argv[1]);
	s_h264file[0] = argv[2];
	s_h264file[1] = argv[3];
	signal(SIGINT, SAMPLE_VDECVO_HandleSig);
	signal(SIGTERM, SAMPLE_VDECVO_HandleSig);

	switch (s32CaseNumber) {
	case 0:
		s32Ret = SAMPLE_VDEC_BIND_VPSS_BIND_VO(decoding_file_num);
		break;
	case 1:
		s32Ret = SAMPLE_VDEC_SEND_VPSS_SEND_VO(decoding_file_num);
		break;
	case 2:
		s32Ret = SAMPLE_VDEC_BIND_VENC(decoding_file_num);
		break;
	case 3:
		s32Ret = SAMPLE_VDEC_BIND_VO(decoding_file_num);
		break;
	default:
		SAMPLE_PRT("the case number %d is invaild!\n", s32CaseNumber);
		SAMPLE_VDECVO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (s32Ret == CVI_SUCCESS)
		SAMPLE_PRT("sample_vdecvo exit success!\n");
	else
		SAMPLE_PRT("sample_vdecvo exit abnormally!\n");

	return s32Ret;
}