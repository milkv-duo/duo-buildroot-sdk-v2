/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_defines.h
 * Description:
 *   The common definitions per chip capability.
 */
 /******************************************************************************         */

#ifndef __U_CVI_CV180X_DEFINES_H__
#define __U_CVI_CV180X_DEFINES_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "linux/cvi_base.h"

#define CVI_CHIP_NAME "CV180X"

#ifndef __CV180X__
#define __CV180X__
#endif

#define CVI_CHIP_TEST  0x0

#define CVIU01 0x1
#define CVIU02 0x2

#define CVI_COLDBOOT 0x1
#define CVI_WDTBOOT 0x2
#define CVI_SUSPENDBOOT 0x3
#define CVI_WARMBOOT 0x4

#define MMF_VER_PRIX "_MMF_V"

#define ALIGN_NUM 4

#define LUMA_PHY_ALIGN          16

#define DEFAULT_ALIGN           64
#define MAX_ALIGN               1024
#define SEG_CMP_LENGTH          256

/* For VENC */
#define VENC_MAX_CHN_NUM        16           /* Maximum number of channels */
#define VENC_MAX_ROI_NUM        8            /* Maximum number of roi region */
#define MAX_TILE_NUM            1            /* Maximum number of tile */
#define VENC_ALIGN_W            32           /* VENC width alignment */
#define VENC_ALIGN_H            16           /* VENC height alignment */

/* For RC */
#define RC_TEXTURE_THR_SIZE     16           /* Number of thresholds for texture rate control */

/* For VDEC */
#define VDEC_MAX_CHN_NUM        64           /* vdec max channel number */

#define H264D_ALIGN_W           64           /* h264 decode align width  */
#define H264D_ALIGN_H           64           /* h264 decode align height */
#define H265D_ALIGN_W           64           /* h265 decode align width  */
#define H265D_ALIGN_H           64           /* h265 decode align height */
#define JPEGD_ALIGN_W           64           /* jpeg decode align width  */
#define JPEGD_ALIGN_H           16           /* jpeg decode align height */

#define H264D_ALIGN_FRM          0x1000      /* h264 decode yuv_frame align size */
#define H265D_ALIGN_FRM          0x1000      /* h265 decode yuv_frame align size */
#define JPEGD_ALIGN_FRM          0x1000      /* jpeg decode yuv_frame align size */

#define VEDU_H264D_MAX_WIDTH    2880         /* h264 decode max width  */
#define VEDU_H264D_MAX_HEIGHT   1920         /* h264 decode max height */
#define VEDU_H265D_MAX_WIDTH    2880         /* h265 decode max width  */
#define VEDU_H265D_MAX_HEIGHT   1920         /* h265 decode max height */
#define JPEGD_MAX_WIDTH         2880         /* jpeg decode max width  */
#define JPEGD_MAX_HEIGHT        1920         /* jpeg decode max height */

/* For Region */
#define RGN_MIN_WIDTH             2
#define RGN_MIN_HEIGHT            2

#define RGN_COVER_MAX_WIDTH       2880
#define RGN_COVER_MAX_HEIGHT      4096
#define RGN_COVER_MIN_X           0
#define RGN_COVER_MIN_Y           0
#define RGN_COVER_MAX_X           (RGN_COVER_MAX_WIDTH - RGN_MIN_WIDTH)
#define RGN_COVER_MAX_Y           (RGN_COVER_MAX_HEIGHT - RGN_MIN_HEIGHT)

#define RGN_COVEREX_MAX_NUM       4
#define RGN_COVEREX_MAX_WIDTH     2880
#define RGN_COVEREX_MAX_HEIGHT    4096
#define RGN_COVEREX_MIN_X         0
#define RGN_COVEREX_MIN_Y         0
#define RGN_COVEREX_MAX_X         (RGN_COVEREX_MAX_WIDTH - RGN_MIN_WIDTH)
#define RGN_COVEREX_MAX_Y         (RGN_COVEREX_MAX_HEIGHT - RGN_MIN_HEIGHT)

#define RGN_OVERLAY_MAX_WIDTH     2880
#define RGN_OVERLAY_MAX_HEIGHT    4096
#define RGN_OVERLAY_MIN_X         0
#define RGN_OVERLAY_MIN_Y         0
#define RGN_OVERLAY_MAX_X         (RGN_OVERLAY_MAX_WIDTH - RGN_MIN_WIDTH)
#define RGN_OVERLAY_MAX_Y         (RGN_OVERLAY_MAX_HEIGHT - RGN_MIN_HEIGHT)

#define RGN_OVERLAYEX_MAX_WIDTH   2880
#define RGN_OVERLAYEX_MAX_HEIGHT  4096
#define RGN_OVERLAYEX_MIN_X       0
#define RGN_OVERLAYEX_MIN_Y       0
#define RGN_OVERLAYEX_MAX_X       (RGN_OVERLAYEX_MAX_WIDTH - RGN_MIN_WIDTH)
#define RGN_OVERLAYEX_MAX_Y       (RGN_OVERLAYEX_MAX_HEIGHT - RGN_MIN_HEIGHT)

#define RGN_MOSAIC_MAX_NUM        8
#define RGN_MOSAIC_X_ALIGN        4
#define RGN_MOSAIC_Y_ALIGN        2
#define RGN_MOSAIC_WIDTH_ALIGN    4
#define RGN_MOSAIC_HEIGHT_ALIGN   4

#define RGN_MOSAIC_MIN_WIDTH      8
#define RGN_MOSAIC_MIN_HEIGHT     8
#define RGN_MOSAIC_MAX_WIDTH      2880
#define RGN_MOSAIC_MAX_HEIGHT     4096
#define RGN_MOSAIC_MIN_X          0
#define RGN_MOSAIC_MIN_Y          0
#define RGN_MOSAIC_MAX_X          (RGN_MOSAIC_MAX_WIDTH - RGN_MOSAIC_MIN_WIDTH)
#define RGN_MOSAIC_MAX_Y          (RGN_MOSAIC_MAX_HEIGHT - RGN_MOSAIC_MIN_HEIGHT)

// vpss rgn define
#define RGN_MAX_LAYER_VPSS        1
#define RGN_ODEC_LAYER_VPSS       0
#define RGN_NORMAL_LAYER_VPSS     0
#define RGN_MAX_NUM_VPSS          8
#define RGN_EX_MAX_NUM_VPSS       16
#define RGN_EX_MAX_WIDTH          2880

// vo rgn define
#define RGN_MAX_NUM_VO            8

#define RGN_MAX_BUF_NUM           2
#define RGN_MAX_NUM               108

/*************************************/
#define VENC_MAX_SSE_NUM          8
#define CVI_MAX_SENSOR_NUM        2

/* For VI */
/* number of channel and device on video input unit of chip
 * Note! VI_MAX_CHN_NUM is NOT equal to VI_MAX_DEV_NUM
 * multiplied by VI_MAX_CHN_NUM, because all VI devices
 * can't work at mode of 4 channels at the same time.
 */
#define VI_MAX_PHY_DEV_NUM        3        /* max physics device num */
#define VI_MAX_VIR_DEV_NUM        2        /* max virtual device num */
#define VI_MAX_DEV_NUM            (VI_MAX_PHY_DEV_NUM + VI_MAX_VIR_DEV_NUM)        /* max device num */
#define VI_MAX_PHY_PIPE_NUM       4        /* max physics pipe num */
#define VI_MAX_VIR_PIPE_NUM       2        /* max virtual pipe num */
#define VI_MAX_PIPE_NUM           (VI_MAX_PHY_PIPE_NUM + VI_MAX_VIR_PIPE_NUM)        /* max pipe num */

#define VI_MAX_VIR_CHN_NUM          2      /* max virtual channel num */
#define VI_MAX_PHY_CHN_NUM          3      /* max physics channel num */
#define VI_MAX_EXT_CHN_NUM          2      /* max external channel num */
#define VI_MAX_CHN_NUM              (VI_MAX_PHY_CHN_NUM + VI_MAX_VIR_CHN_NUM)        /* max channel num */
#define VI_EXT_CHN_START            VI_MAX_CHN_NUM        /*  extension channel start channel num */
#define VI_MAX_EXTCHN_BIND_PER_CHN  1      /* maximum external channels bind Per Channel*/

#define VI_PIPE1_MAX_WIDTH          4096   /* pipe1 max width */
#define VI_CMP_PARAM_SIZE           152    /* compare parameter size */
#define VI_PIXEL_FORMAT             PIXEL_FORMAT_NV21        /* pixel format */
#define CVI_VI_VPSS_EXTRA_BUF       0      /* vi vpss extra buffer num */

#define CVI_VI_CHN_0_BUF            (2 + CVI_VI_VPSS_EXTRA_BUF)        /* chn0 buffer num */
#define CVI_VI_CHN_1_BUF            (2 + CVI_VI_VPSS_EXTRA_BUF)        /* chn1 buffer num */
#define CVI_VI_CHN_2_BUF            (2 + CVI_VI_VPSS_EXTRA_BUF)        /* chn2 buffer num */
#define CVI_VI_CHN_3_BUF            (2 + CVI_VI_VPSS_EXTRA_BUF)        /* chn3 buffer num */

/* For VO */
#define VO_MIN_CHN_WIDTH        32      /* channel minimal width */
#define VO_MIN_CHN_HEIGHT       32      /* channel minimal height */
#define VO_MAX_DEV_NUM          1       /* max dev num */
#define VO_MAX_LAYER_NUM        1       /* max layer num */
#define VO_MAX_PRIORITY         1       /* max layer priority */
#define VO_MAX_CHN_NUM          1       /* max chn num */
#define VO_MAX_LAYER_IN_DEV     1       /* max layer num of each dev */
#define VO_MAX_GRAPHICS_LAYER_NUM   1
#define VO_MIN_TOLERATE         1       /* min play toleration 1ms */
#define VO_MAX_TOLERATE         100000  /* max play toleration 100s */

/* For AUDIO */
#define AI_DEV_MAX_NUM          1       /* max dev num of input/output devices */
#define AO_DEV_MAX_NUM          2       /* max dev num of output devices */
#define AENC_MAX_CHN_NUM        3       /* max channel num of audio encoder */
#define ADEC_MAX_CHN_NUM        3       /* max channel num of audio decoder */

#define AI_MAX_CHN_NUM          2       /* max channel num of input device */
#define AO_MAX_CHN_NUM          1       /* max channel num of output device */
#define AO_SYSCHN_CHNID         (AO_MAX_CHN_NUM - 1) /* system channel id */
                                        /* max channel num of input/output device */
#define AIO_MAX_CHN_NUM         ((AO_MAX_CHN_NUM > AI_MAX_CHN_NUM) ? AO_MAX_CHN_NUM:AI_MAX_CHN_NUM)

/* For VPSS */
#define VPSS_IP_NUM              2      /* VPSS IP num */
#define VPSS_MAX_GRP_NUM         16     /* maximum number of VPSS groups */
#define VPSS_ONLINE_NUM          5      /* maximum number of VPSS online groups */
#define VPSS_ONLINE_GRP_0        0      /* online grp 0 */
#define VPSS_ONLINE_GRP_1        1      /* online grp 1 */

#define VPSS_MAX_PHY_CHN_NUM     3      /* sc_d, sc_v1, sc_v2 */
#define SC_D_MAX_LIMIT           1280   /* maximum width of sc_d */
#define SC_V1_MAX_LIMIT          2880   /* maximum width of sc_v1 */
#define SC_V2_MAX_LIMIT          1920   /* maximum width of sc_v2 */

#define VPSS_MAX_CHN_NUM         (VPSS_MAX_PHY_CHN_NUM) /* maximum number of VPSS channels */
#define VPSS_MIN_IMAGE_WIDTH     32     /* minimum width of the VPSS image */
#define VPSS_MIN_IMAGE_HEIGHT    32     /* minimum height of the VPSS image */
#define VPSS_MAX_IMAGE_WIDTH     2880   /* maximum width of the VPSS image */
#define VPSS_MAX_IMAGE_HEIGHT    4096   /* maximum height of the VPSS image */
#define VPSS_MAX_ZOOMIN          32     /* maximum zoom-in factor of VPSS physical channels */
#define VPSS_MAX_ZOOMOUT         32     /* maximum zoom-out factor of VPSS physical channels */

/* For GDC */
#define LDC_ALIGN                64     /* ldc image size alignment constraints*/

#define GDC_IP_NUM               1      /* number of hardware IPs of gdc*/
#define GDC_PROC_JOB_INFO_NUM    (500)  /* maximum number of job to be processed*/
#define GDC_MIN_IMAGE_WIDTH      64     /* minimum width of the GDC image */
#define GDC_MIN_IMAGE_HEIGHT     64     /* minimum height of the GDC image */
#define GDC_MAX_IMAGE_WIDTH      4032   /* maximum width of the GDC image */
#define GDC_MAX_IMAGE_HEIGHT     4032   /* maximum height of the GDC image */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __U_CVI_CV180X_DEFINES_H__ */

