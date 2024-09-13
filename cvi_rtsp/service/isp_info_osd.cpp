#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if MW_VER == 2
#include <cvi_type.h>
#include <cvi_comm_video.h>
#else
#include <cvi_type.h>
#include <cvi_comm_region.h>
#endif
#include <cvi_ae.h>
#include <cvi_region.h>
#include <cvi_awb.h>
#include "ctx.h"
#include "osd_font_mod.h"

#define CVI_MEDIA_MAX_INFO_OSD_LEN        (100)
#define FONTMOD_WIDTH           (24)
#define FONTMOD_HEIGHT          (30)
#define OSD_INFO_START_X        (0)
#define OSD_INFO_START_Y        (75)
#define OSD_INFO_FONT_WIDTH     (16)
#define OSD_INFO_FONT_HEIGHT    (24)
#define OSD_NUM (2)

#define IsASCII(a)				(((a) >= 0x00 && (a) <= 0x7F) ? 1 : 0)
#define BYTE_BITS               8
#define NOASCII_CHARACTER_BYTES 2 /*   Number of bytes occupied by each Chinese character   */
#define OSD_LIB_FONT_W                 (g_stOsdFonts.u32FontWidth)
#define OSD_LIB_FONT_H                 (g_stOsdFonts.u32FontHeight)

typedef struct _OSD_FONTS_S {
    uint32_t u32FontWidth;
    uint32_t u32FontHeight;
} OSD_FONTS_S;

typedef struct _OSD_CTX {
    bool init;
    bool infoTskRun;
    pthread_t infoTskId;
} OSD_CTX;

typedef struct _OSD_TASK_PARAM {
    int Dev; // Osd dev
    int RgnHdl;
    int vpipe;
} OSD_TASK_PARAM;

static OSD_FONTS_S g_stOsdFonts = {};
typedef struct {
    RGN_ATTR_S gstRgnAttr[OSD_NUM];
    MMF_CHN_S gstChn[OSD_NUM];
    RGN_CHN_ATTR_S gstChnAttr[OSD_NUM];
    OSD_CTX g_OsdCtx[OSD_NUM];
    OSD_TASK_PARAM gOsdTaskParam[OSD_NUM];
} OSD_DEV;
#define OSD_DEV_MAX_NUM (2)
static OSD_DEV gOsdDev[OSD_DEV_MAX_NUM];

static void osd_dev_init(OSD_DEV* dev) {
    if (!dev) return;
    memset(dev, 0, sizeof(OSD_DEV));
}

static void osd_setattr(int osdidx, RGN_ATTR_S* pstRgnAttr, MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr, int VpssGrp)
{
    SIZE_S stRgnSize = {0};
    stRgnSize.u32Width = CVI_MEDIA_MAX_INFO_OSD_LEN * OSD_INFO_FONT_WIDTH;
    stRgnSize.u32Height = OSD_INFO_FONT_HEIGHT;

    pstRgnAttr->enType = OVERLAY_RGN;
    pstRgnAttr->unAttr.stOverlay.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
    pstRgnAttr->unAttr.stOverlay.u32BgColor = 0x8000;
    pstRgnAttr->unAttr.stOverlay.stSize.u32Width = stRgnSize.u32Width;
    pstRgnAttr->unAttr.stOverlay.stSize.u32Height = stRgnSize.u32Height;
    pstRgnAttr->unAttr.stOverlay.u32CanvasNum = 2;

    pstChn->enModId = CVI_ID_VPSS;
    pstChn->s32DevId = VpssGrp;
    pstChn->s32ChnId = 0;
    pstChnAttr->bShow = CVI_TRUE;
    pstChnAttr->enType = OVERLAY_RGN;
    pstChnAttr->unChnAttr.stOverlayChn.stInvertColor.bInvColEn = CVI_FALSE;
    pstChnAttr->unChnAttr.stOverlayChn.stPoint.s32X = OSD_INFO_START_X;
    pstChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y = OSD_INFO_START_Y+osdidx*OSD_INFO_FONT_HEIGHT;
    pstChnAttr->unChnAttr.stOverlayChn.u32Layer = 0;
}

static void osd_init(void)
{
    g_stOsdFonts.u32FontWidth = FONTMOD_WIDTH;
    g_stOsdFonts.u32FontHeight = FONTMOD_HEIGHT;
    for (int i=0; i<OSD_DEV_MAX_NUM; i++)
        osd_dev_init(&gOsdDev[i]);
}

static int osd_get_non_asc_num(char* string, int len)
{
    int i;
    int n = 0;
    for (i = 0; i < len; i++) {
        if (string[i] == '\0') {
            break;
        }
        if (!IsASCII(string[i])) {
            i++;
            n++;
        }
    }

    return n;
}

static int osd_get_font_mod(char* Character,uint8_t** FontMod,int* FontModLen)
{
    /* Get Font Mod in GB2312 Fontlib*/
    uint32_t offset = 0;
	uint32_t areacode = 0;
	uint32_t bitcode = 0;
	if(IsASCII(Character[0])) {
		areacode = 3;
		bitcode = (uint32_t)((uint8_t)Character[0]-0x20);
	} else {
		areacode = (uint32_t)((uint8_t)Character[0]-0xA0);
		bitcode = (uint32_t)((uint8_t)Character[1]-0xA0);
	}
	offset = (94*(areacode-1)+(bitcode-1))*(OSD_LIB_FONT_W*OSD_LIB_FONT_H/8);
	*FontMod = (uint8_t*)g_fontLib+offset;
	*FontModLen = OSD_LIB_FONT_W*OSD_LIB_FONT_H/8;
	return CVI_SUCCESS;
}

static int osd_update_text_bitmap(RGN_HANDLE RgnHdl, char *inStr)
{
    int s32Ret;
    uint32_t u32CanvasWidth,u32CanvasHeight,u32BgColor,u32Color;
    SIZE_S stFontSize;
    int s32StrLen = strnlen(inStr, CVI_MEDIA_MAX_INFO_OSD_LEN);
    int NonASCNum = osd_get_non_asc_num(inStr, s32StrLen);

    SIZE_S stMaxOsdSize = {0};
    stMaxOsdSize.u32Width = CVI_MEDIA_MAX_INFO_OSD_LEN * OSD_INFO_FONT_WIDTH;
    stMaxOsdSize.u32Height = OSD_INFO_FONT_HEIGHT;
    u32CanvasWidth = OSD_INFO_FONT_WIDTH * (s32StrLen - NonASCNum * (NOASCII_CHARACTER_BYTES - 1));
    u32CanvasHeight = OSD_INFO_FONT_HEIGHT;
    stFontSize.u32Width = OSD_INFO_FONT_WIDTH;
    stFontSize.u32Height = OSD_INFO_FONT_HEIGHT;
    u32BgColor = 0x8000;
    u32Color = 0xFFFF;

    RGN_CANVAS_INFO_S stCanvasInfo;
    s32Ret = CVI_RGN_GetCanvasInfo(RgnHdl, &stCanvasInfo);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_RGN_GetCanvasInfo fail,RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);
        return s32Ret;
    }

    /*   Generate Bitmap   */
#if (defined CHIP_ARCH_MARS) || (defined CHIP_ARCH_PHOBOS)
    uint16_t *puBmData = (uint16_t *)CVI_SYS_Mmap(stCanvasInfo.u64PhyAddr,
                         stCanvasInfo.u32Stride * stCanvasInfo.stSize.u32Height);
#else
    uint16_t *puBmData = (uint16_t *)stCanvasInfo.pu8VirtAddr;
#endif

    uint32_t u32BmRow, u32BmCol; /*   Bitmap Row/Col Index   */
    for (u32BmRow = 0; u32BmRow < u32CanvasHeight; ++u32BmRow) {
        int NonASCShow = 0;
        for (u32BmCol = 0; u32BmCol < u32CanvasWidth; ++u32BmCol) {
            /*   Bitmap Data Offset for the point   */
            int s32BmDataIdx = u32BmRow * stCanvasInfo.u32Stride / 2 + u32BmCol;
            /*   Character Index in Text String   */
            int s32CharIdx = u32BmCol / stFontSize.u32Width;
            int s32StringIdx = s32CharIdx + NonASCShow * (NOASCII_CHARACTER_BYTES - 1);
            if (NonASCNum > 0 && s32CharIdx > 0) {
                NonASCShow = osd_get_non_asc_num(inStr, s32StringIdx);
                s32StringIdx = s32CharIdx + NonASCShow * (NOASCII_CHARACTER_BYTES - 1);
            }
            /*   Point Row/Col Index in Character   */
            int s32CharCol = (u32BmCol - (stFontSize.u32Width * s32CharIdx)) * OSD_LIB_FONT_W /
                                stFontSize.u32Width;
            int s32CharRow = u32BmRow * OSD_LIB_FONT_H / stFontSize.u32Height;
            int s32HexOffset = s32CharRow * OSD_LIB_FONT_W / BYTE_BITS + s32CharCol / BYTE_BITS;
            int s32BitOffset = s32CharCol % BYTE_BITS;
            uint8_t *FontMod = NULL;
            int FontModLen = 0;
            if (CVI_SUCCESS == osd_get_font_mod(&inStr[s32StringIdx], &FontMod, &FontModLen)) {
                if (FontMod != NULL && s32HexOffset < FontModLen) {
                    uint8_t temp = FontMod[s32HexOffset];
                    if ((temp >> ((BYTE_BITS - 1) - s32BitOffset)) & 0x1) {
                        puBmData[s32BmDataIdx] = (uint16_t)u32Color;
                    } else {
                        puBmData[s32BmDataIdx] = (uint16_t)u32BgColor;
                    }
                    continue;
                }
            }
            printf("GetFontMod Fail\n");
#if (defined CHIP_ARCH_MARS) || (defined CHIP_ARCH_PHOBOS)
            CVI_SYS_Munmap((void *)puBmData, stCanvasInfo.u32Stride * stCanvasInfo.stSize.u32Height);
#endif
            return CVI_FAILURE;
        }

        for (u32BmCol = u32CanvasWidth; u32BmCol < stMaxOsdSize.u32Width; ++u32BmCol) {
            int s32BmDataIdx = u32BmRow * stCanvasInfo.u32Stride / 2 + u32BmCol;
            puBmData[s32BmDataIdx] = (uint16_t)u32BgColor;
        }
    }

    for (u32BmRow = u32CanvasHeight; u32BmRow < stMaxOsdSize.u32Height; ++u32BmRow) {
        for (u32BmCol = 0; u32BmCol < stMaxOsdSize.u32Width; ++u32BmCol) {
            int s32BmDataIdx = u32BmRow * stCanvasInfo.u32Stride / 2 + u32BmCol;
            puBmData[s32BmDataIdx] = (uint16_t)u32BgColor;
        }
    }
    stCanvasInfo.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
    stCanvasInfo.stSize.u32Width = u32CanvasWidth;
    stCanvasInfo.stSize.u32Height = u32CanvasHeight;

    s32Ret = CVI_RGN_UpdateCanvas(RgnHdl);

    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_RGN_UpdateCanvas fail,RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);
#if (defined CHIP_ARCH_MARS) || (defined CHIP_ARCH_PHOBOS)
        CVI_SYS_Munmap((void *)puBmData, stCanvasInfo.u32Stride * stCanvasInfo.stSize.u32Height);
#endif
        return s32Ret;
    }
#if (defined CHIP_ARCH_MARS) || (defined CHIP_ARCH_PHOBOS)
    CVI_SYS_Munmap((void *)puBmData, stCanvasInfo.u32Stride * stCanvasInfo.stSize.u32Height);
#endif
    return s32Ret;
}

static void osd_get_isp_info(VI_PIPE viPipe, char* str, uint32_t strLen, int line)
{
    ISP_EXP_INFO_S expInfo;
    ISP_WB_INFO_S wbInfo;
    memset(&expInfo,0,sizeof(ISP_EXP_INFO_S));
    memset(&wbInfo,0,sizeof(ISP_WB_INFO_S));
    CVI_ISP_QueryExposureInfo(viPipe, &expInfo);
    CVI_ISP_QueryWBInfo(viPipe, &wbInfo);
    if(line == 0) {
        snprintf(str, strLen, "#AE ExpT:%u SExpT:%u LExpT:%u AG:%u DG:%u IG:%u Exp:%u ExpIsMax:%d AveLum:%d",
                expInfo.u32ExpTime, expInfo.u32ShortExpTime, expInfo.u32LongExpTime, expInfo.u32AGain,\
                expInfo.u32DGain, expInfo.u32ISPDGain, expInfo.u32Exposure, expInfo.bExposureIsMAX, \
                expInfo.u8AveLum);
    } else if (line == 1){
        struct tm* ptime;
        struct timeval tv;
        char str_time[24];
        gettimeofday(&tv, NULL);
        ptime = localtime(&tv.tv_sec);
        strftime(str_time, sizeof(str_time), "%Y-%m-%d %I:%M:%S", ptime);

        snprintf(str, strLen, "%s:%03u PIrisFno:%d Fps:%u ISO:%u #AWB RG:%d BG:%d CT:%d",
                str_time, (uint32_t)(tv.tv_usec / 1000), expInfo.u32PirisFNO, expInfo.u32Fps, expInfo.u32ISO,
                wbInfo.u16Rgain, wbInfo.u16Bgain, wbInfo.u16ColorTemp);
    }
}

static void* osd_task(void* param)
{
    OSD_TASK_PARAM *pstParam = (OSD_TASK_PARAM*) param;
    OSD_DEV *pOsdDev = &gOsdDev[pstParam->Dev];
    char ispInfoStr[CVI_MEDIA_MAX_INFO_OSD_LEN] = {0};

    int curIdx=pstParam->RgnHdl%2;
    while(pOsdDev->g_OsdCtx[curIdx].infoTskRun) {
        memset(ispInfoStr, 0, CVI_MEDIA_MAX_INFO_OSD_LEN);
        osd_get_isp_info(pstParam->vpipe, ispInfoStr, CVI_MEDIA_MAX_INFO_OSD_LEN, curIdx);
        osd_update_text_bitmap(pstParam->RgnHdl, ispInfoStr);
        usleep(1*100*1000);
    }

    return NULL;
}

int isp_info_osd_start(SERVICE_CTX *ctx)
{
    int s32Ret = 0;
    int RgnHdl=0;

    osd_init();

    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        OSD_DEV *pOsdDev = &gOsdDev[idx];

        if (!ent->enableIspInfoOsd) continue;

        for (int osd_idx=0; osd_idx < OSD_NUM; osd_idx++, RgnHdl++) {
            pOsdDev->gOsdTaskParam[osd_idx].vpipe = ent->ViChn;
            pOsdDev->gOsdTaskParam[osd_idx].RgnHdl = RgnHdl;
            pOsdDev->gOsdTaskParam[osd_idx].Dev = idx;
            if (!pOsdDev->g_OsdCtx[osd_idx].init) {
                pOsdDev->g_OsdCtx[osd_idx].init = true;
            }

            osd_setattr(osd_idx, &pOsdDev->gstRgnAttr[osd_idx], &pOsdDev->gstChn[osd_idx], &pOsdDev->gstChnAttr[osd_idx], ent->VpssGrp);
            s32Ret = CVI_RGN_Create(RgnHdl, &pOsdDev->gstRgnAttr[osd_idx]);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_RGN_Create fail,RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);
                return s32Ret;
            }

            s32Ret = CVI_RGN_AttachToChn(RgnHdl, &pOsdDev->gstChn[osd_idx], &pOsdDev->gstChnAttr[osd_idx]);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_RGN_AttachToChn fail,RgnHdl[%d] stChn[%d,%d,%d] Error Code: [0x%08X]\n",
                            RgnHdl, pOsdDev->gstChn[osd_idx].enModId, pOsdDev->gstChn[osd_idx].s32DevId, pOsdDev->gstChn[osd_idx].s32ChnId, s32Ret);
                return s32Ret;
            }

            pOsdDev->g_OsdCtx[osd_idx].infoTskRun = true;
            s32Ret = pthread_create(&pOsdDev->g_OsdCtx[osd_idx].infoTskId, NULL, osd_task, (void*)&pOsdDev->gOsdTaskParam[osd_idx]);
            if (s32Ret != 0) {
                printf("create osd_task for RgnHdl[%d] failed %x\n", RgnHdl, s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

int isp_info_osd_stop(SERVICE_CTX *ctx)
{
    int s32Ret = CVI_SUCCESS;
    for (int idx=0; idx<ctx->dev_num; idx++) {
        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];
        OSD_DEV *pOsdDev = &gOsdDev[idx];
        if (!ent->enableIspInfoOsd) continue;

        for(int osd_idx = 0; osd_idx<OSD_NUM; osd_idx++) {
            int RgnHdl = pOsdDev->gOsdTaskParam->RgnHdl;
            pOsdDev->g_OsdCtx[osd_idx].infoTskRun = false;
            pthread_join(pOsdDev->g_OsdCtx[osd_idx].infoTskId, NULL);

            s32Ret = CVI_RGN_DetachFromChn(RgnHdl, &pOsdDev->gstChn[osd_idx]);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_RGN_DetachFromChn #%d failed with %#x!\n", RgnHdl, s32Ret);
                return CVI_FAILURE;
            }

            s32Ret = CVI_RGN_Destroy(RgnHdl);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_RGN_Destroy #%d failed with %#x!\n", RgnHdl, s32Ret);
                return CVI_FAILURE;
            }
        }
    }

	return s32Ret;
}
