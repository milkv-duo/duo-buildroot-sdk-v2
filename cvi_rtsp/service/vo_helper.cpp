
#include <string.h>
#include <cvi_math.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "cvi_vo.h"
#include "cvi_vpss.h"
#include "cvi_hdmi.h"

#include "vpss_helper.h"
#include "vo_helper.h"

#define APP_PROF_LOG_PRINT(level, fmt, args...) printf(fmt, ##args)

/**************************************************************************
 *                        H E A D E R   F I L E S
 **************************************************************************/

#include <cvi_type.h>
#include <cvi_comm_vo.h>
#include <cvi_comm_video.h>
//#include "mediapipe_sensor.h"

 /**************************************************************************
 *                              M A C R O S
 **************************************************************************/

#define MEDIA_VO_COLOR_RGB_RED RGB_8BIT(0xFF, 0, 0)
#define MEDIA_VO_COLOR_RGB_GREEN RGB_8BIT(0, 0xFF, 0)
#define MEDIA_VO_COLOR_RGB_BLUE RGB_8BIT(0, 0, 0xFF)
#define MEDIA_VO_COLOR_RGB_BLACK RGB_8BIT(0, 0, 0)
#define MEDIA_VO_COLOR_RGB_YELLOW RGB_8BIT(0xFF, 0xFF, 0)
#define MEDIA_VO_COLOR_RGB_CYN RGB_8BIT(0, 0xFF, 0xFF)
#define MEDIA_VO_COLOR_RGB_WHITE RGB_8BIT(0xFF, 0xFF, 0xFF)

#define MEDIA_VO_COLOR_10_RGB_RED RGB(0x3FF, 0, 0)
#define MEDIA_VO_COLOR_10_RGB_GREEN RGB(0, 0x3FF, 0)
#define MEDIA_VO_COLOR_10_RGB_BLUE RGB(0, 0, 0x3FF)
#define MEDIA_VO_COLOR_10_RGB_BLACK RGB(0, 0, 0)
#define MEDIA_VO_COLOR_10_RGB_YELLOW RGB(0x3FF, 0x3FF, 0)
#define MEDIA_VO_COLOR_10_RGB_CYN RGB(0, 0x3FF, 0x3FF)
#define MEDIA_VO_COLOR_10_RGB_WHITE RGB(0x3FF, 0x3FF, 0x3FF)

#define MEDIA_VO_DEV_DHD0 0 /* VO's device HD0 */
#define MEDIA_VO_DEV_DHD1 1 /* VO's device HD1 */
#define MEDIA_VO_DEV_UHD MEDIA_VO_DEV_DHD0 /* VO's ultra HD device:HD0 */
#define MEDIA_VO_DEV_HD MEDIA_VO_DEV_DHD1 /* VO's HD device:HD1 */

 /**************************************************************************
 *                          C O N S T A N T S
 **************************************************************************/

typedef CVI_S32 VO_DEV;
typedef CVI_S32 VO_LAYER;
typedef CVI_S32 VO_CHN;

typedef enum _MEDIA_VO_MODE_E {
    MEDIA_VO_MODE_1MUX,
    MEDIA_VO_MODE_2MUX,
    MEDIA_VO_MODE_4MUX,
    MEDIA_VO_MODE_8MUX,
    MEDIA_VO_MODE_9MUX,
    MEDIA_VO_MODE_16MUX,
    MEDIA_VO_MODE_25MUX,
    MEDIA_VO_MODE_36MUX,
    MEDIA_VO_MODE_49MUX,
    MEDIA_VO_MODE_64MUX,
    MEDIA_VO_MODE_2X4,
    MEDIA_VO_MODE_BUTT
} MEDIA_VO_MODE_E;

/**************************************************************************
 *                          D A T A   T Y P E S
 **************************************************************************/
typedef struct _media_vo_chn_cfg_ {
    VO_CHN VoChn;
    VO_CHN_ATTR_S stChnAttr;
    uint32_t Rotate;
} MEDIA_VO_CHN_CFG_S;

typedef struct _media_vo_dev_cfg_ {
    /* for device */
    VO_DEV VoDev;
    VO_PUB_ATTR_S stVoPubAttr;

    /* for layer */
    VO_LAYER VoLayer;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    CVI_U32 u32DisBufLen;

    /* for chn */
    CVI_U8 u8ChnCnt;
    MEDIA_VO_CHN_CFG_S astChnCfg[VO_MAX_CHN_NUM];
} MEDIA_VO_DEV_CFG_S;

typedef struct _CVI_PARAM_VO_CFG_S {
    CVI_U8 u8DevCnt;
    MEDIA_VO_DEV_CFG_S astDevCfg[VO_MAX_DEV_NUM];
} CVI_PARAM_VO_CFG_S;

 /**************************************************************************
 *              F U N C T I O N   D E C L A R A T I O N S
 **************************************************************************/
CVI_S32 media_vo_start_vo(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_stop_vo(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_start_vo(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_stop_vo(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_start(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_stop();
CVI_S32 media_vo_init(CVI_PARAM_VO_CFG_S *pstVOCfg);
CVI_S32 media_vo_uninit(CVI_PARAM_VO_CFG_S *pstVOCfg);



static CVI_BOOL g_bVoInited[VO_MAX_DEV_NUM] = {CVI_FALSE, CVI_FALSE};
static CVI_BOOL g_bHdmiStarted = CVI_FALSE;
static CVI_BOOL g_bHdmiEventProFlg = CVI_FALSE;
static pthread_t g_hdmiEventProThd = CVI_NULL;
static pthread_cond_t g_hdmiEventProCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_hdmiEventProLock = PTHREAD_MUTEX_INITIALIZER;

//static pthread_t g_sendFrmAOChnThd = CVI_NULL;
static CVI_S32 g_hdmiEvent = 0;
static VO_INTF_SYNC_E g_enIdealIntfSync;

static CVI_S32 media_vo_start_dev(MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_start_layer(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_start_chn(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_stop_dev(MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_stop_layer(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_stop_chn(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg);
static CVI_S32 media_vo_init_hdmi(MEDIA_VO_DEV_CFG_S *pstVoDevCfg);

/******************************************************************************
 * origin : SAMPLE_COMM_VO_FillIntfAttr
 * function : media_vo_fill_intf_attr
 ******************************************************************************/
static CVI_S32 media_vo_fill_intf_attr(VO_PUB_ATTR_S *pstPubAttr)
{
    pstPubAttr = pstPubAttr;
#if 0
    extern VO_I80_CFG_S stI80Cfg;
   // extern VO_HW_MCU_CFG_S st7789v3Cfg;
   //extern VO_BT_ATTR_S stTP2803Cfg;
    //extern VO_LVDS_ATTR_S lvds_lcm185x56_cfg;

    if (pstPubAttr == NULL) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "Error:argument can not be NULL\n");
        return CVI_FAILURE;
    }

    switch (pstPubAttr->enIntfType) {
    case VO_INTF_I80:
        //pstPubAttr->sti80Cfg = stI80Cfg;
        break;
    //case VO_INTF_HW_MCU:
    //  pstPubAttr->stMcuCfg = st7789v3Cfg;
    //  break;
    case VO_INTF_CVBS:
    case VO_INTF_YPBPR:
    case VO_INTF_VGA:
    case VO_INTF_BT656:
        //pstPubAttr->stBtAttr = stTP2803Cfg;
        break;
    case VO_INTF_BT1120:
    case VO_INTF_LCD:
    case VO_INTF_LCD_18BIT:
    case VO_INTF_LCD_24BIT:
        pstPubAttr->stLvdsAttr = lvds_lcm185x56_cfg;
    case VO_INTF_LCD_30BIT:
    case VO_INTF_HDMI:
        break;
    case VO_INTF_MIPI:
    case VO_INTF_MIPI_SLAVE:
        //no need, MIPI-DSI is setup by mipi-tx
        break;
    default:
        break;
    }
#endif
    return CVI_SUCCESS;
}

CVI_S32 media_vo_get_wh(VO_INTF_SYNC_E enIntfSync, CVI_U32 *pu32W, CVI_U32 *pu32H, CVI_U32 *pu32Frm)
{
    switch (enIntfSync) {
    case VO_OUTPUT_PAL:
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 25;
        break;
    case VO_OUTPUT_NTSC:
        *pu32W = 720;
        *pu32H = 480;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_1080P24:
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 24;
        break;
    case VO_OUTPUT_1080P25:
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 25;
        break;
    case VO_OUTPUT_1080P30:
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_720P50:
        *pu32W = 1280;
        *pu32H = 720;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_720P60:
        *pu32W = 1280;
        *pu32H = 720;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1080P50:
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_1080P60:
        *pu32W = 1920;
        *pu32H = 1080;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_576P50:
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 50;
        break;
    case VO_OUTPUT_480P60:
        *pu32W = 720;
        *pu32H = 480;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_800x600_60:
        *pu32W = 800;
        *pu32H = 600;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1024x768_60:
        *pu32W = 1024;
        *pu32H = 768;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1280x1024_60:
        *pu32W = 1280;
        *pu32H = 1024;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1366x768_60:
        *pu32W = 1366;
        *pu32H = 768;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1440x900_60:
        *pu32W = 1440;
        *pu32H = 900;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1280x800_60:
        *pu32W = 1280;
        *pu32H = 800;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1600x1200_60:
        *pu32W = 1600;
        *pu32H = 1200;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1680x1050_60:
        *pu32W = 1680;
        *pu32H = 1050;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1920x1200_60:
        *pu32W = 1920;
        *pu32H = 1200;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_640x480_60:
        *pu32W = 640;
        *pu32H = 480;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_720x1280_60:
        *pu32W = 720;
        *pu32H = 1280;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1080x1920_60:
        *pu32W = 1080;
        *pu32H = 1920;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_480x800_60:
        *pu32W = 480;
        *pu32H = 800;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_1440P60:
        *pu32W = 2560;
        *pu32H = 1440;
        *pu32Frm = 60;
        break;
    case VO_OUTPUT_2160P30:
        *pu32W = 3840;
        *pu32H = 2160;
        *pu32Frm = 30;
        break;
    case VO_OUTPUT_USER:
        *pu32W = 720;
        *pu32H = 576;
        *pu32Frm = 25;
        break;
    default:
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "vo enIntfSync %d not support!\n", enIntfSync);
        return CVI_FAILURE;
    }

    return CVI_SUCCESS;
}

CVI_VOID vo_default_param(MEDIA_VO_DEV_CFG_S *pstDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U8 j;
    SIZE_S size;
    CVI_U32 u32Frm;
    CVI_U32 u32Width, u32Height;
    CVI_U32 u32Square = 0;

    s32Ret = media_vo_get_wh(pstDevCfg->stVoPubAttr.enIntfSync, &size.u32Width,
                &size.u32Height, &u32Frm);
    if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_get_wh failed!\n");
            return;
    }

    pstDevCfg->stLayerAttr.stDispRect.u32Width = size.u32Width;
    pstDevCfg->stLayerAttr.stDispRect.u32Height = size.u32Height;
    pstDevCfg->stLayerAttr.stImageSize.u32Width = size.u32Width;
    pstDevCfg->stLayerAttr.stImageSize.u32Height = size.u32Height;
    pstDevCfg->stLayerAttr.u32DispFrmRt = u32Frm;
    u32Width = pstDevCfg->stLayerAttr.stImageSize.u32Width;
    u32Height = pstDevCfg->stLayerAttr.stImageSize.u32Height;

    if (pstDevCfg->u8ChnCnt <= 1) {
        u32Square = 1;
    } else if (pstDevCfg->u8ChnCnt <= 2) {
        u32Square = 2;
    } else if (pstDevCfg->u8ChnCnt <= 4) {
        u32Square = 2;
    } else if (pstDevCfg->u8ChnCnt <= 8) {
        u32Square = 3;
    } else if (pstDevCfg->u8ChnCnt <= 9) {
        u32Square = 3;
    } else if (pstDevCfg->u8ChnCnt <= 16) {
        u32Square = 4;
    } else if (pstDevCfg->u8ChnCnt <= 25) {
        u32Square = 5;
    } else if (pstDevCfg->u8ChnCnt <= 36) {
        u32Square = 6;
    } else if (pstDevCfg->u8ChnCnt <= 49) {
        u32Square = 7;
    } else if (pstDevCfg->u8ChnCnt <= 64) {
        u32Square = 8;
    }

    for (j = 0; j < pstDevCfg->u8ChnCnt; j++) {
        pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32X =
            ALIGN_DOWN((u32Width / u32Square) * (j % u32Square), 2);
        pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32Y =
            ALIGN_DOWN((u32Height / u32Square) * (j / u32Square), 2);
        pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Width =
            ALIGN_DOWN(u32Width / u32Square, 2);
        pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Height =
            ALIGN_DOWN(u32Height / u32Square, 2);
    }

}

CVI_VOID prepare_param(CVI_PARAM_VO_CFG_S *pstVOCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U8 i, j;
    SIZE_S size;
    CVI_U32 u32Frm;
    CVI_U32 u32Width, u32Height;
    MEDIA_VO_DEV_CFG_S *pstDevCfg;
    CVI_U32 u32Square = 0;

    for (i = 0; i < pstVOCfg->u8DevCnt; i++) {
        pstDevCfg = &pstVOCfg->astDevCfg[i];

        s32Ret = media_vo_get_wh(pstDevCfg->stVoPubAttr.enIntfSync, &size.u32Width,
                    &size.u32Height, &u32Frm);
        if (s32Ret != CVI_SUCCESS) {
                APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_get_wh failed!\n");
                return;
        }

        if ((pstDevCfg->stLayerAttr.stDispRect.u32Width == 0) ||
            (pstDevCfg->stLayerAttr.stDispRect.u32Height == 0)) {
            pstDevCfg->stLayerAttr.stDispRect.u32Width = size.u32Width;
            pstDevCfg->stLayerAttr.stDispRect.u32Height = size.u32Height;
            pstDevCfg->stLayerAttr.stImageSize.u32Width = size.u32Width;
            pstDevCfg->stLayerAttr.stImageSize.u32Height = size.u32Height;
        }
        pstDevCfg->stLayerAttr.u32DispFrmRt = u32Frm;
        u32Width = pstDevCfg->stLayerAttr.stImageSize.u32Width;
        u32Height = pstDevCfg->stLayerAttr.stImageSize.u32Height;

        if (pstDevCfg->u8ChnCnt <= 1) {
            u32Square = 1;
        } else if (pstDevCfg->u8ChnCnt <= 2) {
            u32Square = 2;
        } else if (pstDevCfg->u8ChnCnt <= 4) {
            u32Square = 2;
        } else if (pstDevCfg->u8ChnCnt <= 8) {
            u32Square = 3;
        } else if (pstDevCfg->u8ChnCnt <= 9) {
            u32Square = 3;
        } else if (pstDevCfg->u8ChnCnt <= 16) {
            u32Square = 4;
        } else if (pstDevCfg->u8ChnCnt <= 25) {
            u32Square = 5;
        } else if (pstDevCfg->u8ChnCnt <= 36) {
            u32Square = 6;
        } else if (pstDevCfg->u8ChnCnt <= 49) {
            u32Square = 7;
        } else if (pstDevCfg->u8ChnCnt <= 64) {
            u32Square = 8;
        }

        for (j = 0; j < pstDevCfg->u8ChnCnt; j++) {
            if ((pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Width == 0) ||
                (pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Height == 0)) {
                pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32X =
                    ALIGN_DOWN((u32Width / u32Square) * (j % u32Square), 2);
                pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32Y =
                    ALIGN_DOWN((u32Height / u32Square) * (j / u32Square), 2);
                pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Width =
                    ALIGN_DOWN(u32Width / u32Square, 2);
                pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Height =
                    ALIGN_DOWN(u32Height / u32Square, 2);
            }
        }

    }
}

static CVI_U32 hdmi_format_sync_to_pixel_clk(CVI_HDMI_VIDEO_FORMAT hdmi_format)
{
    switch (hdmi_format) {
    case CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60:
        return 148500;
    case CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p30:
        return 74250;
    case CVI_HDMI_VIDEO_FORMAT_CVT_RB_2560X1440p60:
        return 241500;
    case CVI_HDMI_VIDEO_FORMAT_CEA861_3840x2160p30:
        return 297000;
    default:
        return 148500;
    }

    return 148500;
}

static CVI_HDMI_VIDEO_FORMAT vo_sync_to_hdmi_format(VO_INTF_SYNC_E enIntfSync)
{
    switch (enIntfSync) {
    case VO_OUTPUT_1080P60:
        return CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60;
    case VO_OUTPUT_1080P30:
        return CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p30;
    case VO_OUTPUT_1440P60:
        return CVI_HDMI_VIDEO_FORMAT_CVT_RB_2560X1440p60;
    case VO_OUTPUT_2160P30:
        return CVI_HDMI_VIDEO_FORMAT_CEA861_3840x2160p30;
    default:
        return CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60;
    }

    return CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60;
}

static VO_INTF_SYNC_E hdmi_format_to_vo_sync(CVI_HDMI_VIDEO_FORMAT hdmi_format)
{
    switch (hdmi_format) {
    case CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60:
        return VO_OUTPUT_1080P60;
    case CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p30:
        return VO_OUTPUT_1080P30;
    case CVI_HDMI_VIDEO_FORMAT_CVT_RB_2560X1440p60:
        return VO_OUTPUT_1440P60;
    case CVI_HDMI_VIDEO_FORMAT_CEA861_3840x2160p30:
        return VO_OUTPUT_2160P30;
    default:
        return VO_OUTPUT_1080P60;
    }

    return VO_OUTPUT_1080P60;
}

CVI_VOID hdmi_callback(CVI_HDMI_EVENT_TYPE event, CVI_VOID *private_data)
{
    private_data = private_data;

    switch (event) {
    case CVI_HDMI_EVENT_HOTPLUG:
        printf("\033[0;32mhdmi HOTPLUG event! \033[0;39m\n");

        g_hdmiEvent = CVI_HDMI_EVENT_HOTPLUG;
        break;
    case CVI_HDMI_EVENT_NO_PLUG:
        printf("\033[0;31mhdmi NO_PLUG event! \033[0;39m\n");

        g_hdmiEvent = CVI_HDMI_EVENT_NO_PLUG;
        break;
    case CVI_HDMI_EVENT_EDID_FAIL:
        printf("\033[0;31mHDMI edid fail! \033[0;39m\n");

        g_hdmiEvent = CVI_HDMI_EVENT_EDID_FAIL;
        break;
    default:
        break;
    }

    pthread_mutex_lock(&g_hdmiEventProLock);
    pthread_cond_signal(&g_hdmiEventProCond);
    pthread_mutex_unlock(&g_hdmiEventProLock);
}

CVI_VOID *HdmiEventProThread(CVI_VOID *args)
{
    CVI_S32 s32Ret;
    struct timespec ts;
    MEDIA_VO_DEV_CFG_S *pstVoDevCfg = (MEDIA_VO_DEV_CFG_S *)args;
    VO_DEV VoDev = pstVoDevCfg->VoDev;

    prctl(PR_SET_NAME, (unsigned long)"hdmi_event_Thread", 0, 0, 0);

    while (g_bHdmiEventProFlg) {
        while (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
            continue;
        ts.tv_nsec += 100 * 1000 * 1000; //100ms
        ts.tv_sec += ts.tv_nsec / 1000000000;
        ts.tv_nsec %= 1000000000;

        pthread_mutex_lock(&g_hdmiEventProLock);
        s32Ret = pthread_cond_timedwait(&g_hdmiEventProCond, &g_hdmiEventProLock, &ts);
        if (s32Ret) {
            pthread_mutex_unlock(&g_hdmiEventProLock);
            continue;
        }
        pthread_mutex_unlock(&g_hdmiEventProLock);

        switch (g_hdmiEvent) {
        case CVI_HDMI_EVENT_HOTPLUG:
        {
            if (g_bHdmiStarted) {
                CVI_HDMI_Stop();
                CVI_HDMI_DeInit();
                g_bHdmiStarted = CVI_FALSE;
            }

            if (g_bVoInited[VoDev]) {
                media_vo_stop_chn(pstVoDevCfg);
                media_vo_stop_layer(pstVoDevCfg);
                CVI_VO_Disable(pstVoDevCfg->VoDev);
                g_bVoInited[VoDev] = CVI_FALSE;
            }

            if (!g_bHdmiStarted) {
                media_vo_init_hdmi(pstVoDevCfg);
            }

            media_vo_start_dev(pstVoDevCfg);
            media_vo_start_layer(pstVoDevCfg);
            media_vo_start_chn(pstVoDevCfg);
            g_bVoInited[VoDev] = CVI_TRUE;

            printf("reset done\n");
            break;
        }
        case CVI_HDMI_EVENT_NO_PLUG:
        {
            printf("no plug...\n");
            if (g_bHdmiStarted) {
                CVI_HDMI_Stop();
                CVI_HDMI_DeInit();
                g_bHdmiStarted = CVI_FALSE;
            }
            if (g_bVoInited[VoDev]) {
                media_vo_stop_chn(pstVoDevCfg);
                media_vo_stop_layer(pstVoDevCfg);
                CVI_VO_Disable(pstVoDevCfg->VoDev);
                g_bVoInited[VoDev] = CVI_FALSE;
            }

            printf("HDMI stop...\n");
            break;
        }
        case CVI_HDMI_EVENT_EDID_FAIL:
            break;
        default:
            break;
        }

        g_hdmiEvent = 0;
    }

    return NULL;
}

static CVI_S32 media_vo_init_hdmi(MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_HDMI_ATTR stHdmiAttr;
    CVI_HDMI_CALLBACK_FUNC fnHdmiCb = {hdmi_callback, pstVoDevCfg};
    CVI_HDMI_SINK_CAPABILITY stCapability = {0};
    CVI_HDMI_VIDEO_FORMAT enHdmiVideoFormat;

    if (!g_bHdmiEventProFlg) {
        g_bHdmiEventProFlg = CVI_TRUE;
        s32Ret = pthread_create(&g_hdmiEventProThd, CVI_NULL,
            HdmiEventProThread, (CVI_VOID *)pstVoDevCfg);
        if (s32Ret != CVI_SUCCESS) {
            g_bHdmiEventProFlg = CVI_FALSE;
        }

        s32Ret =  CVI_HDMI_RegisterCallback(&fnHdmiCb);
        if(s32Ret != CVI_SUCCESS){
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI RegisterCallback failed with %#x!\n", s32Ret);
            return CVI_FAILURE;
        }
    }

    s32Ret =  CVI_HDMI_Init();
    if(s32Ret != CVI_SUCCESS){
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "HPD disconnected ! HDMI Init failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    CVI_HDMI_GetSinkCapability(&stCapability);
    printf("[HDMI]is_connected:%d,power on:%d, support_hdmi(%d %d) native_video_format:%d\n",
        stCapability.is_connected,
        stCapability.is_sink_power_on,
        stCapability.support_hdmi,
        stCapability.support_hdmi_2_0,
        stCapability.native_video_format);

    enHdmiVideoFormat = vo_sync_to_hdmi_format(g_enIdealIntfSync); // !!!

    if (stCapability.is_connected) {

        bool bFind = false;
        int mcode = 0;

        while (stCapability.support_video_format[mcode].mcode) {
            if (stCapability.support_video_format[mcode].mcode == enHdmiVideoFormat) {
                bFind = true;
            }
            mcode++;
        }

        if (!bFind) {

            printf("HDMI warn, not find format: %d, use: %d\n",
                enHdmiVideoFormat, CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60);

            enHdmiVideoFormat = CVI_HDMI_VIDEO_FORMAT_CEA861_1920x1080p60;
        }
    }

    memset(&stHdmiAttr, 0, sizeof(stHdmiAttr));
    stHdmiAttr.hdmi_en = CVI_TRUE;
    stHdmiAttr.audio_en = CVI_FALSE;
    stHdmiAttr.hdcp14_en = CVI_FALSE;
    stHdmiAttr.bit_depth = CVI_HDMI_BIT_DEPTH_24;
    stHdmiAttr.video_format = enHdmiVideoFormat;
    stHdmiAttr.pix_clk = hdmi_format_sync_to_pixel_clk(stHdmiAttr.video_format);
    stHdmiAttr.deep_color_mode = CVI_HDMI_DEEP_COLOR_24BIT;

    s32Ret =  CVI_HDMI_SetAttr(&stHdmiAttr);
    if(s32Ret != CVI_SUCCESS){
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI SetAttr failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }
    if (stCapability.is_connected) {
        pstVoDevCfg->stVoPubAttr.enIntfSync = hdmi_format_to_vo_sync(stHdmiAttr.video_format);
        pstVoDevCfg->stLayerAttr.stDispRect.u32Width = 0;
        pstVoDevCfg->stLayerAttr.stDispRect.u32Height = 0;
        vo_default_param(pstVoDevCfg);
    }

    s32Ret =  CVI_HDMI_Start();
    if(s32Ret != CVI_SUCCESS){
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI Start failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }
    g_bHdmiStarted = CVI_TRUE;

    printf("HDMI start...\n");

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StartDev
 * function : media_vo_start_dev
 ******************************************************************************/
static CVI_S32 media_vo_start_dev(MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_DEV VoDev = pstVoDevCfg->VoDev;

    if (pstVoDevCfg->stVoPubAttr.enIntfType == VO_INTF_HDMI) {
        media_vo_init_hdmi(pstVoDevCfg);
    }

    s32Ret = media_vo_fill_intf_attr(&pstVoDevCfg->stVoPubAttr);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    s32Ret = CVI_VO_SetPubAttr(VoDev, &pstVoDevCfg->stVoPubAttr);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    s32Ret = CVI_VO_Enable(VoDev);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    return s32Ret;
}


/******************************************************************************
 * origin : SAMPLE_COMM_VO_StartLayer
 * function : media_vo_start_layer
 ******************************************************************************/
static CVI_S32 media_vo_start_layer(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_LAYER VoLayer = pstVoDevCfg->VoLayer;

    s32Ret = CVI_VO_SetVideoLayerAttr(VoLayer, &pstVoDevCfg->stLayerAttr);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    if (pstVoDevCfg->u32DisBufLen) {
        s32Ret = CVI_VO_SetDisplayBufLen(VoLayer, pstVoDevCfg->u32DisBufLen);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "CVI_VO_SetDisplayBufLen failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }

    s32Ret = CVI_VO_EnableVideoLayer(VoLayer);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StartChn
 * function : media_vo_start_chn
 ******************************************************************************/
static CVI_S32 media_vo_start_chn(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    VO_CHN VoChn;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_LAYER VoLayer = pstVoDevCfg->VoLayer;

    for (VoChn = 0; VoChn < pstVoDevCfg->u8ChnCnt; VoChn++) {
        s32Ret = CVI_VO_SetChnAttr(VoLayer, VoChn, &pstVoDevCfg->astChnCfg[VoChn].stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
            return CVI_FAILURE;
        }

        if (pstVoDevCfg->astChnCfg[VoChn].Rotate > 0) {
            s32Ret = CVI_VO_SetChnRotation(VoLayer, VoChn, (ROTATION_E) pstVoDevCfg->astChnCfg[VoChn].Rotate);
            if (s32Ret != CVI_SUCCESS) {
                APP_PROF_LOG_PRINT(LEVEL_ERROR, "CVI_VO_SetChnRotation failed with %#x!\n", s32Ret);
                return s32Ret;
            }
        }
        s32Ret = CVI_VO_EnableChn(VoLayer, VoChn);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
            return CVI_FAILURE;
        }
    }

    return CVI_SUCCESS;
}

static CVI_S32 media_vo_deinit_hdmi(MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_HDMI_CALLBACK_FUNC fnHdmiCb = {hdmi_callback, pstVoDevCfg};

    if (g_bHdmiEventProFlg) {
        g_bHdmiEventProFlg = CVI_FALSE;
        pthread_join(g_hdmiEventProThd, CVI_NULL);

        s32Ret = CVI_HDMI_UnRegisterCallback(&fnHdmiCb);
        if(s32Ret != CVI_SUCCESS){
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI UnRegisterCallback failed with %#x!\n", s32Ret);
            return CVI_FAILURE;
        }
    }

    s32Ret =  CVI_HDMI_Stop();
    if(s32Ret != CVI_SUCCESS){
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI Init failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    s32Ret =  CVI_HDMI_DeInit();
    if(s32Ret != CVI_SUCCESS){
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "HDMI Deinit failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    g_bHdmiStarted = CVI_FALSE;
    printf("HDMI stop...\n");

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StopDev
 * function : media_vo_stop_dev
 ******************************************************************************/
static CVI_S32 media_vo_stop_dev(MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_DEV VoDev = pstVoDevCfg->VoDev;

    s32Ret = CVI_VO_Disable(VoDev);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    if (pstVoDevCfg->stVoPubAttr.enIntfType == VO_INTF_HDMI) {
        media_vo_deinit_hdmi(pstVoDevCfg);
    }

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StopLayer
 * function : media_vo_stop_layer
 ******************************************************************************/
static CVI_S32 media_vo_stop_layer(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_LAYER VoLayer = pstVoDevCfg->VoLayer;

    s32Ret = CVI_VO_DisableVideoLayer(VoLayer);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
        return CVI_FAILURE;
    }

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StopChn
 * function : media_vo_stop_chn
 ******************************************************************************/
static CVI_S32 media_vo_stop_chn(const MEDIA_VO_DEV_CFG_S *pstVoDevCfg)
{
    VO_CHN VoChn;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VO_LAYER VoLayer = pstVoDevCfg->VoLayer;

    for (VoChn = 0; VoChn < pstVoDevCfg->u8ChnCnt; VoChn++) {
        s32Ret = CVI_VO_DisableChn(VoLayer, VoChn);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "failed with %#x!\n", s32Ret);
            return CVI_FAILURE;
        }
    }

    return s32Ret;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StartVO
 * function : media_vo_start
 ******************************************************************************/
CVI_S32 media_vo_start(CVI_PARAM_VO_CFG_S *pstVOCfg)
{
    /*******************************************
     * VO device VoDev# information declaration.
     *******************************************/
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U8 i;

    if (pstVOCfg == NULL) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "Error:argument can not be NULL\n");
        return CVI_FAILURE;
    }

    for (i = 0; i < pstVOCfg->u8DevCnt; i++) {
        /********************************
         * Set and start VO device VoDev#.
         ********************************/
        s32Ret = media_vo_start_dev(&pstVOCfg->astDevCfg[i]);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_start_dev failed!\n");
            return s32Ret;
        }

        /******************************
         * Set and start layer.
         ********************************/
        s32Ret = media_vo_start_layer(&pstVOCfg->astDevCfg[i]);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_start_layer video layer failed!\n");
            media_vo_stop_dev(&pstVOCfg->astDevCfg[i]);
            return s32Ret;
        }

        /******************************
         * start vo channels.
         ********************************/
        s32Ret = media_vo_start_chn(&pstVOCfg->astDevCfg[i]);
        if (s32Ret != CVI_SUCCESS) {
            APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_start_chn failed!\n");
            media_vo_stop_layer(&pstVOCfg->astDevCfg[i]);
            media_vo_stop_dev(&pstVOCfg->astDevCfg[i]);
            return s32Ret;
        }
        g_bVoInited[pstVOCfg->astDevCfg[i].VoDev] = CVI_TRUE;
    }


    return CVI_SUCCESS;
}

/******************************************************************************
 * origin : SAMPLE_COMM_VO_StopVO
 * function : media_vo_stop
 ******************************************************************************/
CVI_S32 media_vo_stop(CVI_PARAM_VO_CFG_S *pstVOCfg)
{
    CVI_U8 i;

    if (pstVOCfg == NULL) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "Error:argument can not be NULL\n");
        return CVI_FAILURE;
    }

    for (i = 0; i < pstVOCfg->u8DevCnt; i++) {
        media_vo_stop_chn(&pstVOCfg->astDevCfg[i]);
        media_vo_stop_layer(&pstVOCfg->astDevCfg[i]);
        media_vo_stop_dev(&pstVOCfg->astDevCfg[i]);
        g_bVoInited[pstVOCfg->astDevCfg[i].VoDev] = CVI_FALSE;
    }

    return CVI_SUCCESS;
}


CVI_S32 media_vo_init(CVI_PARAM_VO_CFG_S *pstVOCfg)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (pstVOCfg->u8DevCnt == 0) return s32Ret;
    /************************************************
     * step:  Init VO
     ************************************************/
     prepare_param(pstVOCfg);

    s32Ret = media_vo_start(pstVOCfg);
    if (s32Ret != CVI_SUCCESS) {
        APP_PROF_LOG_PRINT(LEVEL_ERROR, "media_vo_start_vo failed with %#x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

CVI_S32 media_vo_uninit(CVI_PARAM_VO_CFG_S *pstVOCfg)
{
    if (pstVOCfg->u8DevCnt == 0) return CVI_SUCCESS;


    return media_vo_stop(pstVOCfg);
}

static VO_INTF_SYNC_E get_vo_format_by_src(int src_width, int src_height)
{
#define __SRC_WH(w, h) (((w) << 16) | (h))

    uint32_t src_wh = __SRC_WH(src_width, src_height);

    switch (src_wh) {
    case __SRC_WH(2560, 1440):
        return VO_OUTPUT_1440P60;
    case __SRC_WH(3840, 2160):
        return VO_OUTPUT_2160P30;
    default:
        return VO_OUTPUT_1080P60;
    }

    return VO_OUTPUT_1080P60;
}

static int init_vo_cfg_param(SERVICE_CTX_ENTITY *ent, CVI_PARAM_VO_CFG_S *pstVOCfg) // !!!
{
    int i = 0, j;
    int vo_cnt = 0;
    int ini_value;
    MEDIA_VO_DEV_CFG_S *pstDevCfg;

    vo_cnt = 1;
    pstVOCfg->u8DevCnt = vo_cnt;

    for (i = 0; i < vo_cnt; i++) {
        pstDevCfg = &pstVOCfg->astDevCfg[i];

        pstDevCfg->VoDev = 1; // HDMI must be 1
        pstDevCfg->stVoPubAttr.u32BgColor = MEDIA_VO_COLOR_10_RGB_BLACK;

        ini_value = 15; // HDMI
        pstDevCfg->stVoPubAttr.enIntfType = (VO_INTF_TYPE_E)(0x01 << ini_value);

        VO_INTF_SYNC_E enIntfSync = get_vo_format_by_src(ent->src_width, ent->src_height);

        ini_value = (int) enIntfSync;
        pstDevCfg->stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E) ini_value;

        pstDevCfg->VoLayer = 1;
        pstDevCfg->u32DisBufLen = 0;
        pstDevCfg->stLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
        pstDevCfg->stLayerAttr.stDispRect.s32X = 0;
        pstDevCfg->stLayerAttr.stDispRect.s32Y = 0;
        pstDevCfg->stLayerAttr.stDispRect.u32Width = 0;
        pstDevCfg->stLayerAttr.stDispRect.u32Height = 0;

        pstDevCfg->stLayerAttr.stImageSize.u32Width = pstDevCfg->stLayerAttr.stDispRect.u32Width;
        pstDevCfg->stLayerAttr.stImageSize.u32Height = pstDevCfg->stLayerAttr.stDispRect.u32Height;

        pstDevCfg->u8ChnCnt = 1;

        for (j = 0; j < pstDevCfg->u8ChnCnt; j++) {
            pstDevCfg->astChnCfg[j].VoChn = j;
            pstDevCfg->astChnCfg[j].Rotate = 0;
            pstDevCfg->astChnCfg[j].stChnAttr.u32Priority = 0;
            pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32X = 0;
            pstDevCfg->astChnCfg[j].stChnAttr.stRect.s32Y = 0;
            pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Width = 0;
            pstDevCfg->astChnCfg[j].stChnAttr.stRect.u32Height = 0;
        }
    }

    return 0;
}

static bool g_bvoInit = false;
static CVI_PARAM_VO_CFG_S g_voCfg;
static MMF_CHN_S g_srcDev;
static MMF_CHN_S g_destDev;

static int vo_bind(SERVICE_CTX_ENTITY *ent)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    memset(&g_srcDev, 0, sizeof(MMF_CHN_S));
    memset(&g_destDev, 0, sizeof(MMF_CHN_S));

    g_srcDev.enModId = CVI_ID_VPSS;
    g_srcDev.s32DevId = ent->VpssGrp;
    g_srcDev.s32ChnId = VO_BIND_VPSS_CHN;

    g_destDev.enModId = CVI_ID_VO;
    g_destDev.s32DevId = 1; // HDMI must be 1
    g_destDev.s32ChnId = 0;

    s32Ret = CVI_SYS_Bind(&g_srcDev, &g_destDev);
    if (s32Ret != CVI_SUCCESS) {
        printf("vo bind failed, %d\n", s32Ret);
    }

    return s32Ret;
}

static int vo_unbind(void)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = CVI_SYS_UnBind(&g_srcDev, &g_destDev);
    if (s32Ret != CVI_SUCCESS) {
        printf("vo unbind failed, %d\n", s32Ret);
    }

    return s32Ret;
}

int init_vo(SERVICE_CTX *ctx)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    int dev_idx = 0;

    VPSS_CHN_ATTR_S vpss_chn_attr = {};

    for (int idx = 0; idx < ctx->dev_num; idx++) {

        SERVICE_CTX_ENTITY *ent = &ctx->entity[idx];

        if (g_bvoInit) {
            break;
        }

        if (!ent->enableHdmi) {
            continue;
        }

        g_bvoInit = true;
        dev_idx = idx;

        int chn_width = ent->src_width;
        int chn_height = ent->src_height;

        g_enIdealIntfSync = get_vo_format_by_src(chn_width, chn_height);

        if (g_enIdealIntfSync == VO_OUTPUT_1080P60) {
            chn_width = 1920;
            chn_height = 1080;
        }

        // init vpss chn
        vpss_chn_attr_default(&vpss_chn_attr, chn_width, chn_height, SAMPLE_PIXEL_FORMAT, false);

        printf("Dev: %d, Enable HDMI VPSS CHN %d, VPSS_CHN_ATTR_S: %d x %d\n",
            idx, VO_BIND_VPSS_CHN, vpss_chn_attr.u32Width, vpss_chn_attr.u32Height);

        s32Ret = CVI_VPSS_SetChnAttr(ent->VpssGrp, VO_BIND_VPSS_CHN, &vpss_chn_attr);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VPSS_SetChnAttr Grp: %d, Chn: %d Failed, s32Ret: 0x%x\n",
                ent->VpssGrp, VO_BIND_VPSS_CHN, s32Ret);
            return -1;
        }

        s32Ret = CVI_VPSS_EnableChn(ent->VpssGrp, VO_BIND_VPSS_CHN);
        if (s32Ret != CVI_SUCCESS) {
            printf("CVI_VPSS_EnableChn Grp: %d, Chn: %d Failed, s32Ret: 0x%x\n",
                ent->VpssGrp, VO_BIND_VPSS_CHN, s32Ret);
            return -1;
        }

        memset(&g_voCfg, 0, sizeof(CVI_PARAM_VO_CFG_S));

        init_vo_cfg_param(&ctx->entity[dev_idx], &g_voCfg);
        media_vo_init(&g_voCfg);

        vo_bind(&ctx->entity[dev_idx]);
    }

    return 0;
}

int deinit_vo(SERVICE_CTX *ctx)
{
    (void) ctx;

    if (g_bvoInit) {
        vo_unbind();
        media_vo_uninit(&g_voCfg);
    }

    return 0;
}

