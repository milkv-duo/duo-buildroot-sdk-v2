
#include "vi_vo_utils.h"
#include <cvi_venc.h>
#include <inttypes.h>
#include <stdlib.h>
#include "core/utils/vpss_helper.h"

#include <endian.h>
#include <fcntl.h>

#define PIXEL_FORMAT_VO VI_PIXEL_FORMAT
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720

typedef enum {
  CODEC_H264,
  CODEC_H265,
} VencCodec;

CVI_S32 InitVI(SAMPLE_VI_CONFIG_S *pstViConfig, SIZE_S *viSize, SIZE_S tdlSize,
               PIXEL_FORMAT_E tdlFormat);

CVI_S32 InitVPSS(VPSSConfigs *vpssConfigs, const CVI_BOOL isVOOpened);

CVI_S32 InitOutput(OutputType outputType, CVI_S32 frameWidth, CVI_S32 frameHeight,
                   OutputContext *context);

CVI_S32 DestoryOutput(OutputContext *context);

static int low_mem_profile = 1;
static VI_PIPE ViPipe = 0;
static int fps = 25;

#define ION_TOTALMEM "/sys/firmware/devicetree/base/reserved-memory/ion/size"
static void load_ion_totalmem(void) {
  FILE *fp = NULL;
  char mem_buf[16] = "";
  int64_t ion_total_mem = 0;

  fp = fopen(ION_TOTALMEM, "r");
  if (fp == NULL) {
    SAMPLE_PRT("fopen %s fail\n", ION_TOTALMEM);
    return;
  }
  if (fread(mem_buf, 1, sizeof(mem_buf), fp) > 0) {
    memcpy(&ion_total_mem, mem_buf, sizeof(int64_t));
    ion_total_mem = be64toh(ion_total_mem);
    if (ion_total_mem > 0) {
      if (ion_total_mem < 65 * 1024 * 1024) {
        low_mem_profile = 1;
      }
    }
  }
}

CVI_S32 SetFps(uint32_t _fps) {
  fps = _fps;
  ISP_PUB_ATTR_S pubAttr = {0};
  CVI_ISP_GetPubAttr(0, &pubAttr);
  pubAttr.f32FrameRate = fps;
  CVI_ISP_SetPubAttr(0, &pubAttr);
  return 0;
}

CVI_S32 InitVideoSystem(VideoSystemContext *vs_ctx, int _fps) {
  CVI_S32 s32Ret = CVI_SUCCESS;
  PIXEL_FORMAT_E TdlFormat = PIXEL_FORMAT_RGB_888;
  SIZE_S TdlSize = {.u32Width = VIDEO_WIDTH, .u32Height = VIDEO_HEIGHT};
  SIZE_S VoSize = {.u32Width = VIDEO_WIDTH, .u32Height = VIDEO_HEIGHT};
  PIXEL_FORMAT_E Voformat = PIXEL_FORMAT_VO;
  PIXEL_FORMAT_E Viformat = VI_PIXEL_FORMAT;
  int voType = OUTPUT_TYPE_RTSP;

//****************************************************************
// Init VI, VO, Vpss
#ifdef _MIDDLEWARE_V3_
  CVI_MSG_Init();
#endif
  load_ion_totalmem();

  SIZE_S viSize;
  s32Ret = InitVI(&vs_ctx->viConfig, &viSize, TdlSize, TdlFormat);
  if (s32Ret != CVI_SUCCESS) {
    printf("Init video input failed with %d\n", s32Ret);
    return CVI_FAILURE;
  }

  vs_ctx->vpssConfigs.vpssGrp = 0;
  vs_ctx->vpssConfigs.groupFormat = Viformat;
  vs_ctx->vpssConfigs.grpWidth = viSize.u32Width;
  vs_ctx->vpssConfigs.grpHeight = viSize.u32Height;

  // CHN for VO or encoding
  vs_ctx->vpssConfigs.voFormat = Voformat;
  vs_ctx->vpssConfigs.voWidth = VoSize.u32Width;
  vs_ctx->vpssConfigs.voHeight = VoSize.u32Height;
  vs_ctx->vpssConfigs.vpssChnVideoOutput = VPSS_CHN1;

  // CHN for TDL inference
  vs_ctx->vpssConfigs.tdlFormat = TdlFormat;
  vs_ctx->vpssConfigs.tdlFormat = TdlSize.u32Width;
  vs_ctx->vpssConfigs.tdlHeight = TdlSize.u32Height;
  vs_ctx->vpssConfigs.vpssChntdl = VPSS_CHN0;

  SetFps(_fps);

  s32Ret = InitVPSS(&vs_ctx->vpssConfigs, voType != 0);
  if (s32Ret != CVI_SUCCESS) {
    printf("Init video process group 0 failed with %d\n", s32Ret);
    return CVI_FAILURE;
  }

  if (voType) {
    OutputType outputType = voType;
    s32Ret = InitOutput(outputType, vs_ctx->vpssConfigs.voWidth, vs_ctx->vpssConfigs.voHeight,
                        &vs_ctx->outputContext);
    if (s32Ret != CVI_SUCCESS) {
      printf("CVI_Init_Video_Output failed with %d\n", s32Ret);
      return CVI_FAILURE;
    }
  }

  return CVI_SUCCESS;
}

void DestroyVideoSystem(VideoSystemContext *vs_ctx) {
  DestoryOutput(&vs_ctx->outputContext);
#if defined(_MIDDLEWARE_V2_) || defined(_MIDDLEWARE_V3_)
  SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, vs_ctx->vpssConfigs.vpssChntdl, vs_ctx->vpssConfigs.vpssGrp);
#else
  SAMPLE_COMM_VI_UnBind_VPSS(vs_ctx->vpssConfigs.vpssChntdl, vs_ctx->vpssConfigs.vpssGrp);
#endif
  CVI_BOOL ChannelEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
  ChannelEnable[vs_ctx->vpssConfigs.vpssChntdl] = CVI_TRUE;
  ChannelEnable[vs_ctx->vpssConfigs.vpssChnVideoOutput] = CVI_TRUE;
  SAMPLE_COMM_VPSS_Stop(vs_ctx->vpssConfigs.vpssGrp, ChannelEnable);
  SAMPLE_COMM_VI_DestroyVi(&vs_ctx->viConfig);
  SAMPLE_COMM_SYS_Exit();
}

CVI_S32 InitVI(SAMPLE_VI_CONFIG_S *pstViConfig, SIZE_S *viSize, SIZE_S tdlSize,
               PIXEL_FORMAT_E tdlFormat) {
#ifndef __CV186X__
  VPSS_MODE_E enVPSSMode = VPSS_MODE_DUAL;
#endif

#if defined(_MIDDLEWARE_V2_) || defined(_MIDDLEWARE_V3_)
  SAMPLE_INI_CFG_S stIniCfg = {};

  DYNAMIC_RANGE_E enDynamicRange = DYNAMIC_RANGE_SDR8;
  PIXEL_FORMAT_E enPixFormat = VI_PIXEL_FORMAT;
  VIDEO_FORMAT_E enVideoFormat = VIDEO_FORMAT_LINEAR;
  COMPRESS_MODE_E enCompressMode = (low_mem_profile == 1) ? COMPRESS_MODE_TILE : COMPRESS_MODE_NONE;
  VI_VPSS_MODE_E enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;

  memset(&stIniCfg, 0, sizeof(SAMPLE_INI_CFG_S));
  stIniCfg.enSource = VI_PIPE_FRAME_SOURCE_DEV;
  stIniCfg.devNum = 2;
  stIniCfg.enSnsType[0] = SONY_IMX307_MIPI_2M_30FPS_12BIT;
  stIniCfg.enWDRMode[0] = WDR_MODE_NONE;
  stIniCfg.s32BusId[0] = 3;
  stIniCfg.MipiDev[0] = 0xFF;
  stIniCfg.s32BusId[1] = 0;
  stIniCfg.MipiDev[1] = 0xFF;
#ifndef _MIDDLEWARE_V3_
  stIniCfg.enSnsType[1] = SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT;
#endif

  VB_CONFIG_S stVbConf;
  PIC_SIZE_E enPicSize;
  CVI_U32 u32BlkSize;
  CVI_S32 s32Ret = CVI_SUCCESS;

  VI_DEV ViDev = 0;
  CVI_S32 s32WorkSnsId = 0;
  VI_PIPE_ATTR_S stPipeAttr;

  // Get config from ini if found.
  if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
    SAMPLE_PRT("Parse complete\n");
  }

  /************************************************
   * step1:  Config VI
   ************************************************/
  SAMPLE_COMM_VI_GetSensorInfo(pstViConfig);

  s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, pstViConfig);
  if (s32Ret != CVI_SUCCESS) return s32Ret;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enPixFormat = enPixFormat;
  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = enMastPipeMode;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange = enDynamicRange;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat = enVideoFormat;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enCompressMode = enCompressMode;

#else
  SAMPLE_INI_CFG_S stIniCfg = {};
  DYNAMIC_RANGE_E enDynamicRange = DYNAMIC_RANGE_SDR8;
  PIXEL_FORMAT_E enPixFormat = VI_PIXEL_FORMAT;
  VIDEO_FORMAT_E enVideoFormat = VIDEO_FORMAT_LINEAR;
  COMPRESS_MODE_E enCompressMode = (low_mem_profile == 1) ? COMPRESS_MODE_TILE : COMPRESS_MODE_NONE;
  VI_VPSS_MODE_E enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;

  memset(&stIniCfg, 0, sizeof(SAMPLE_INI_CFG_S));
  stIniCfg.enSource = VI_PIPE_FRAME_SOURCE_DEV;
  stIniCfg.devNum = 1;
  stIniCfg.enSnsType = SONY_IMX307_MIPI_2M_30FPS_12BIT;
  stIniCfg.enWDRMode = WDR_MODE_NONE;
  stIniCfg.s32BusId = 3;
  stIniCfg.MipiDev = 0xFF;
  stIniCfg.u8UseDualSns = 0;
  stIniCfg.s32Sns2BusId = 0;
  stIniCfg.Sns2MipiDev = 0xFF;
  stIniCfg.enSns2Type = SONY_IMX307_SLAVE_MIPI_2M_30FPS_12BIT;

  VB_CONFIG_S stVbConf;
  PIC_SIZE_E enPicSize;
  CVI_U32 u32BlkSize;
  CVI_S32 s32Ret = CVI_SUCCESS;

  VI_DEV ViDev = 0;
  VI_CHN ViChn = 0;
  CVI_S32 s32WorkSnsId = 0;
  VI_PIPE_ATTR_S stPipeAttr;

  // Get config from ini if found.
  if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
    SAMPLE_PRT("Parse complete\n");
  }

  /************************************************
   * step1:  Config VI
   ************************************************/
  SAMPLE_COMM_VI_GetSensorInfo(pstViConfig);

  pstViConfig->s32WorkingViNum = 1 + s32WorkSnsId;
  pstViConfig->as32WorkingViId[s32WorkSnsId] = s32WorkSnsId;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.enSnsType =
      (s32WorkSnsId == 0) ? stIniCfg.enSnsType : stIniCfg.enSns2Type;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.MipiDev =
      (s32WorkSnsId == 0) ? stIniCfg.MipiDev : stIniCfg.Sns2MipiDev;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.s32BusId =
      (s32WorkSnsId == 0) ? stIniCfg.s32BusId : stIniCfg.s32Sns2BusId;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.s32SnsI2cAddr =
      (s32WorkSnsId == 0) ? stIniCfg.s32SnsI2cAddr : stIniCfg.s32Sns2I2cAddr;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[0] =
      (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[0] : stIniCfg.as16Sns2LaneId[0];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[1] =
      (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[1] : stIniCfg.as16Sns2LaneId[1];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[2] =
      (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[2] : stIniCfg.as16Sns2LaneId[2];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[3] =
      (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[3] : stIniCfg.as16Sns2LaneId[3];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId[4] =
      (s32WorkSnsId == 0) ? stIniCfg.as16LaneId[4] : stIniCfg.as16Sns2LaneId[4];

  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[0] =
      (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[0] : stIniCfg.as8Sns2PNSwap[0];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[1] =
      (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[1] : stIniCfg.as8Sns2PNSwap[1];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[2] =
      (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[2] : stIniCfg.as8Sns2PNSwap[2];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[3] =
      (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[3] : stIniCfg.as8Sns2PNSwap[3];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap[4] =
      (s32WorkSnsId == 0) ? stIniCfg.as8PNSwap[4] : stIniCfg.as8Sns2PNSwap[4];
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.stMclkAttr.bMclkEn =
      (s32WorkSnsId == 0) ? stIniCfg.stMclkAttr.bMclkEn : stIniCfg.stSns2MclkAttr.bMclkEn;
  pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.stMclkAttr.u8Mclk =
      (s32WorkSnsId == 0) ? stIniCfg.stMclkAttr.u8Mclk : stIniCfg.stSns2MclkAttr.u8Mclk;

  pstViConfig->astViInfo[s32WorkSnsId].stDevInfo.ViDev = 0;
  pstViConfig->astViInfo[s32WorkSnsId].stDevInfo.enWDRMode =
      (s32WorkSnsId == 0) ? stIniCfg.enWDRMode : stIniCfg.enSns2WDRMode;

  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = enMastPipeMode;
  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0] = s32WorkSnsId;
  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1] = -1;
  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2] = -1;
  pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3] = -1;

  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.ViChn = ViChn;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enPixFormat = enPixFormat;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange = enDynamicRange;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat = enVideoFormat;
  pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enCompressMode = enCompressMode;
#endif
  /************************************************
   * step2:  Get input size
   ************************************************/
  s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.enSnsType,
                                          &enPicSize);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n", s32Ret);
    return s32Ret;
  }

  s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, viSize);
  SAMPLE_PRT("viSize=%d, %d\n", viSize->u32Height, viSize->u32Width);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
    return s32Ret;
  }

  /************************************************
   * step3:  Init SYS and common VB
   ************************************************/
#ifndef _MIDDLEWARE_V3_
  memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
  stVbConf.u32MaxPoolCnt = 3;

  u32BlkSize = COMMON_GetPicBufferSize(viSize->u32Width, viSize->u32Height, SAMPLE_PIXEL_FORMAT,
                                       DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
  stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[0].u32BlkCnt = low_mem_profile ? 3 : 7;

  u32BlkSize = COMMON_GetPicBufferSize(tdlSize.u32Width, tdlSize.u32Height, tdlFormat,
                                       DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
  stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[1].u32BlkCnt = 3;

  u32BlkSize = COMMON_GetPicBufferSize(VIDEO_WIDTH, VIDEO_HEIGHT, PIXEL_FORMAT_VO, DATA_BITWIDTH_8,
                                       COMPRESS_MODE_NONE, DEFAULT_ALIGN);
  stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[2].u32BlkCnt = 3;

  for (uint32_t poolId = 0; poolId < stVbConf.u32MaxPoolCnt; poolId++) {
    SAMPLE_PRT("common pool[%d] BlkSize %dx%d\n", poolId, stVbConf.astCommPool[poolId].u32BlkSize,
               stVbConf.astCommPool[poolId].u32BlkCnt);
  }

  s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("system init failed with %#x\n", s32Ret);
    return s32Ret;
  }
#ifndef __CV186X__
  s32Ret = CVI_SYS_SetVPSSMode(enVPSSMode);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("system init failed with %#x\n", s32Ret);
    return s32Ret;
  }
#endif
#endif

  /************************************************
   * step4:  Init VI ISP
   ************************************************/
  s32Ret = SAMPLE_COMM_VI_StartSensor(pstViConfig);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("system start sensor failed with %#x\n", s32Ret);
    return s32Ret;
  }
  s32Ret = SAMPLE_COMM_VI_StartDev(&pstViConfig->astViInfo[ViDev]);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("VI_StartDev failed with %#x!\n", s32Ret);
    return s32Ret;
  }
  s32Ret = SAMPLE_COMM_VI_StartMIPI(pstViConfig);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("system start MIPI failed with %#x\n", s32Ret);
    return s32Ret;
  }

  stPipeAttr.bYuvSkip = CVI_FALSE;
  stPipeAttr.u32MaxW = viSize->u32Width;
  stPipeAttr.u32MaxH = viSize->u32Height;
  stPipeAttr.enPixFmt = PIXEL_FORMAT_RGB_BAYER_12BPP;
  stPipeAttr.enBitWidth = DATA_BITWIDTH_12;
  stPipeAttr.stFrameRate.s32SrcFrameRate = -1;
  stPipeAttr.stFrameRate.s32DstFrameRate = -1;
  stPipeAttr.bNrEn = CVI_TRUE;
  stPipeAttr.bYuvBypassPath = CVI_FALSE;
  stPipeAttr.enCompressMode = enCompressMode;
  s32Ret = CVI_VI_CreatePipe(ViPipe, &stPipeAttr);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("CVI_VI_CreatePipe failed with %#x!\n", s32Ret);
    return s32Ret;
  }

  s32Ret = CVI_VI_StartPipe(ViPipe);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("CVI_VI_StartPipe failed with %#x!\n", s32Ret);
    return s32Ret;
  }

  s32Ret = CVI_VI_GetPipeAttr(ViPipe, &stPipeAttr);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("CVI_VI_StartPipe failed with %#x!\n", s32Ret);
    return s32Ret;
  }

  s32Ret = SAMPLE_COMM_VI_CreateIsp(pstViConfig);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("VI_CreateIsp failed with %#x!\n", s32Ret);
    return s32Ret;
  }

  s32Ret = SAMPLE_COMM_VI_StartViChn(pstViConfig);
  if (s32Ret != CVI_SUCCESS) {
    SAMPLE_PRT("StartViChn failed with %#x!\n", s32Ret);
    return s32Ret;
  }
  return s32Ret;
}

static CVI_S32 CheckInputCfg(chnInputCfg *pIc) {
  if (!strcmp(pIc->codec, "264") || !strcmp(pIc->codec, "265")) {
    if (pIc->gop < 1) {
      if (!strcmp(pIc->codec, "264"))
        pIc->gop = DEF_264_GOP;
      else
        pIc->gop = DEF_GOP;
    }

    if (!strcmp(pIc->codec, "265")) {
      if (pIc->single_LumaBuf > 0) {
        SAMPLE_PRT("single_LumaBuf only supports H.264\n");
        pIc->single_LumaBuf = 0;
      }
    } else if (low_mem_profile == 1) {
      pIc->single_LumaBuf = 1;
    }
    pIc->iqp = (pIc->iqp >= 0) ? pIc->iqp : DEF_IQP;
    pIc->pqp = (pIc->pqp >= 0) ? pIc->pqp : DEF_PQP;
#ifdef CVI_H26X_MAX_I_PROP_DEFAULT
    pIc->maxIprop = CVI_H26X_MAX_I_PROP_DEFAULT;
    pIc->minIprop = CVI_H26X_MIN_I_PROP_DEFAULT;
#endif

    if (pIc->rcMode == -1) {
      pIc->rcMode = SAMPLE_RC_FIXQP;
    }

    if (pIc->rcMode == SAMPLE_RC_CBR || pIc->rcMode == SAMPLE_RC_VBR) {
      if (pIc->rcMode == SAMPLE_RC_CBR) {
        if (pIc->bitrate <= 0) {
          SAMPLE_PRT("CBR bitrate must be not less than 0");
          return -1;
        }
      } else if (pIc->rcMode == SAMPLE_RC_VBR) {
        if (pIc->maxbitrate <= 0) {
          SAMPLE_PRT("VBR must be not less than 0");
          return -1;
        }
      }

      pIc->firstFrmstartQp =
          (pIc->firstFrmstartQp < 0 || pIc->firstFrmstartQp > 51) ? 63 : pIc->firstFrmstartQp;

      pIc->maxIqp = (pIc->maxIqp >= 0) ? pIc->maxIqp : DEF_264_MAXIQP;
      pIc->minIqp = (pIc->minIqp >= 0) ? pIc->minIqp : DEF_264_MINIQP;
      pIc->maxQp = (pIc->maxQp >= 0) ? pIc->maxQp : DEF_264_MAXQP;
      pIc->minQp = (pIc->minQp >= 0) ? pIc->minQp : DEF_264_MINQP;

      if (pIc->statTime == 0) {
        pIc->statTime = DEF_STAT_TIME;
      }
    } else if (pIc->rcMode == SAMPLE_RC_FIXQP) {
      if (pIc->firstFrmstartQp != -1) {
        pIc->firstFrmstartQp = -1;
      }

      pIc->bitrate = 0;
    } else {
      SAMPLE_PRT("codec = %s, rcMode = %d, not supported RC mode\n", pIc->codec, pIc->rcMode);
      return -1;
    }
  } else if (!strcmp(pIc->codec, "mjp") || !strcmp(pIc->codec, "jpg")) {
    if (pIc->rcMode == -1) {
      pIc->rcMode = SAMPLE_RC_FIXQP;
      pIc->quality = (pIc->quality != -1) ? pIc->quality : 0;
    } else if (pIc->rcMode == SAMPLE_RC_FIXQP) {
      if (pIc->quality < 0)
        pIc->quality = 0;
      else if (pIc->quality >= 100)
        pIc->quality = 99;
      else if (pIc->quality == 0)
        pIc->quality = 1;
    }
  } else {
    SAMPLE_PRT("codec = %s\n", pIc->codec);
    return -1;
  }

  return 0;
}
#ifndef __CV180X__
static CVI_S32 InitVO(const CVI_U32 width, const CVI_U32 height, SAMPLE_VO_CONFIG_S *stVoConfig) {
  CVI_S32 s32Ret = CVI_SUCCESS;
  s32Ret = SAMPLE_COMM_VO_GetDefConfig(stVoConfig);
  if (s32Ret != CVI_SUCCESS) {
    printf("SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
    return s32Ret;
  }
  RECT_S dispRect = {0, 0, height, width};
  SIZE_S imgSize = {height, width};

  stVoConfig->VoDev = 0;
  stVoConfig->stVoPubAttr.enIntfType = VO_INTF_MIPI;
  stVoConfig->stVoPubAttr.enIntfSync = VO_OUTPUT_720x1280_60;
  stVoConfig->stDispRect = dispRect;
  stVoConfig->stImageSize = imgSize;
  stVoConfig->enPixFormat = VI_PIXEL_FORMAT;
  stVoConfig->enVoMode = VO_MODE_1MUX;

  s32Ret = SAMPLE_COMM_VO_StartVO(stVoConfig);
  if (s32Ret != CVI_SUCCESS) {
    printf("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
  }
  CVI_VO_SetChnRotation(0, 0, ROTATION_270);
  printf("SAMPLE_COMM_VO_StartVO done\n");
  return s32Ret;
}
#endif

CVI_S32 InitVPSS(VPSSConfigs *vpssConfigs, const CVI_BOOL isVOOpened) {
  CVI_S32 s32Ret = CVI_SUCCESS;
  VPSS_GRP_ATTR_S stVpssGrpAttr;
  CVI_BOOL ChannelEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
  VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

  ChannelEnable[vpssConfigs->vpssChntdl] = CVI_TRUE;
  VPSS_CHN_DEFAULT_HELPER(&stVpssChnAttr[vpssConfigs->vpssChntdl], vpssConfigs->tdlWidth,
                          vpssConfigs->tdlHeight, vpssConfigs->tdlFormat, true);
  stVpssChnAttr[vpssConfigs->vpssChntdl].u32Depth = 1;

  if (isVOOpened) {
    ChannelEnable[vpssConfigs->vpssChnVideoOutput] = CVI_TRUE;
    VPSS_CHN_DEFAULT_HELPER(&stVpssChnAttr[vpssConfigs->vpssChnVideoOutput], vpssConfigs->voWidth,
                            vpssConfigs->voHeight, PIXEL_FORMAT_VO, true);
  }
  stVpssChnAttr[vpssConfigs->vpssChnVideoOutput].u32Depth = 1;

  VPSS_GRP_DEFAULT_HELPER2(&stVpssGrpAttr, vpssConfigs->grpWidth, vpssConfigs->grpHeight,
                           vpssConfigs->groupFormat, 1);

  /*start vpss*/
  s32Ret =
      SAMPLE_COMM_VPSS_Init(vpssConfigs->vpssGrp, ChannelEnable, &stVpssGrpAttr, stVpssChnAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
    return s32Ret;
  }

  s32Ret =
      SAMPLE_COMM_VPSS_Start(vpssConfigs->vpssGrp, ChannelEnable, &stVpssGrpAttr, stVpssChnAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
    return s32Ret;
  }
#if defined(_MIDDLEWARE_V2_) || defined(_MIDDLEWARE_V3_)
  s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, vpssConfigs->vpssChntdl, vpssConfigs->vpssGrp);
#else
  s32Ret = SAMPLE_COMM_VI_Bind_VPSS(vpssConfigs->vpssChntdl, vpssConfigs->vpssGrp);
#endif
  if (s32Ret != CVI_SUCCESS) {
    printf("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
    return s32Ret;
  }

  return s32Ret;
}

static void _initInputCfg(chnInputCfg *ipIc) {
  strcpy(ipIc->codec, "h264");
  ipIc->initialDelay = CVI_INITIAL_DELAY_DEFAULT;
  ipIc->width = 3840;
  ipIc->height = 2160;
  ipIc->vpssGrp = 1;
  ipIc->vpssChn = 0;
  ipIc->num_frames = -1;
  ipIc->bsMode = 0;
  ipIc->rcMode = 0;
  ipIc->iqp = 30;
  ipIc->pqp = 30;
  ipIc->gop = 60;
  ipIc->bitrate = 8000;
  ipIc->firstFrmstartQp = 30;
  ipIc->minIqp = -1;
  ipIc->maxIqp = -1;
  ipIc->minQp = -1;
  ipIc->maxQp = -1;
  ipIc->srcFramerate = fps;
  ipIc->framerate = fps;
  ipIc->bVariFpsEn = 0;
  ipIc->maxbitrate = -1;
  ipIc->statTime = -1;
  ipIc->chgNum = -1;
  ipIc->quality = -1;
  ipIc->pixel_format = 0;
  ipIc->bitstreamBufSize = 0;
  ipIc->single_LumaBuf = 0;
  ipIc->single_core = 0;
  ipIc->forceIdr = -1;
  ipIc->tempLayer = 0;
  ipIc->testRoi = 0;
  ipIc->bgInterval = 0;
}

static void rtsp_connect(const char *ip, void *arg) { printf("connect: %s\n", ip); }

static void rtsp_disconnect(const char *ip, void *arg) { printf("disconnect: %s\n", ip); }

static PIC_SIZE_E get_output_size(CVI_S32 width, CVI_S32 height) {
  if (width == 1280 && height == 720) {
    return PIC_720P;
  } else if (width == 1920 && height == 1080) {
    return PIC_1080P;
  } else {
    return PIC_BUTT;
  }
}

static CVI_S32 InitRTSP(VencCodec codec, CVI_S32 frameWidth, CVI_S32 frameHeight,
                        OutputContext *context) {
  CVI_S32 s32Ret = CVI_SUCCESS;
  VENC_CHN VencChn[] = {0};
  PAYLOAD_TYPE_E enPayLoad = codec == CODEC_H264 ? PT_H264 : PT_H265;
  PIC_SIZE_E enSize = get_output_size(frameWidth, frameHeight);
  if (enSize == PIC_BUTT) {
    printf("Unsupported resolution: (%#x, %#x)", frameWidth, frameHeight);
    return CVI_FAILURE;
  }

  VENC_GOP_MODE_E enGopMode = VENC_GOPMODE_NORMALP;
  VENC_GOP_ATTR_S stGopAttr;
  SAMPLE_RC_E enRcMode;
  CVI_U32 u32Profile = 0;

  _initInputCfg(&context->input_cfg);

  CheckInputCfg(&context->input_cfg);

  enRcMode = (SAMPLE_RC_E)context->input_cfg.rcMode;

  s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("[Err]Venc Get GopAttr for %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  s32Ret = SAMPLE_COMM_VENC_Start(&context->input_cfg, VencChn[0], enPayLoad, enSize, enRcMode,
                                  u32Profile, CVI_FALSE, &stGopAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("[Err]Venc Start failed for %#x!\n", s32Ret);
    return CVI_FAILURE;
  }

  CVI_RTSP_CONFIG config = {0};
  config.port = 554;

  context->rtsp_context = NULL;
  if (0 > CVI_RTSP_Create(&context->rtsp_context, &config)) {
    printf("fail to create rtsp contex\n");
    return CVI_FAILURE;
  }

  context->session = NULL;
  CVI_RTSP_SESSION_ATTR attr = {0};
  attr.video.codec = codec == CODEC_H264 ? RTSP_VIDEO_H264 : RTSP_VIDEO_H265;
  // attr.video.bitrate = 20000;    // add this to fix rtsp mosaic if needed

  snprintf(attr.name, sizeof(attr.name), "%s", codec == CODEC_H264 ? "h264" : "h265");

  CVI_RTSP_CreateSession(context->rtsp_context, &attr, &context->session);

  // set listener
  context->listener.onConnect = rtsp_connect;
  context->listener.argConn = context->rtsp_context;
  context->listener.onDisconnect = rtsp_disconnect;

  CVI_RTSP_SetListener(context->rtsp_context, &context->listener);

  if (0 > CVI_RTSP_Start(context->rtsp_context)) {
    printf("fail to start\n");
    return CVI_FAILURE;
  }
  printf("init done\n");
  return s32Ret;
}

CVI_S32 InitOutput(OutputType outputType, CVI_S32 frameWidth, CVI_S32 frameHeight,
                   OutputContext *context) {
  context->type = outputType;
  switch (outputType) {
#ifndef __CV180X__
    case OUTPUT_TYPE_PANEL: {
      CVI_S32 s32Ret = CVI_SUCCESS;
      printf("Init panel\n");
      context->voChn = 0;
      context->voLayer = 0;
      SAMPLE_VO_CONFIG_S voConfig;
      s32Ret = InitVO(frameWidth, frameHeight, &voConfig);
      if (s32Ret != CVI_SUCCESS) {
        printf("CVI_Init_Video_Output failed with %d\n", s32Ret);
        return s32Ret;
      }
      CVI_VO_HideChn(context->voLayer, context->voChn);
      return CVI_SUCCESS;
    }
#endif
    case OUTPUT_TYPE_RTSP: {
      printf("Init rtsp\n");
      return InitRTSP(CODEC_H264, frameWidth, frameHeight, context);
    }
    default:
      printf("Unsupported output typed: %x\n", outputType);
      return CVI_FAILURE;
  };

  return CVI_SUCCESS;
}

static CVI_S32 panel_send_frame(VIDEO_FRAME_INFO_S *stVencFrame, OutputContext *context) {
#ifdef __CV180X__
  return 0;
#else
  CVI_S32 s32Ret = CVI_VO_SendFrame(context->voLayer, context->voChn, stVencFrame, -1);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VO_SendFrame failed with %#x\n", s32Ret);
  }
  CVI_VO_ShowChn(context->voLayer, context->voChn);
  return s32Ret;
  return 0;
#endif
}

static CVI_S32 rtsp_send_frame(VIDEO_FRAME_INFO_S *stVencFrame, OutputContext *context) {
  CVI_S32 s32Ret = CVI_SUCCESS;
  CVI_RTSP_SESSION *session = context->session;

  CVI_S32 s32SetFrameMilliSec = 20000;
  VENC_STREAM_S stStream;
  VENC_CHN_ATTR_S stVencChnAttr;
  VENC_CHN_STATUS_S stStat;
  VENC_CHN VencChn = 0;
  s32Ret = CVI_VENC_SendFrame(VencChn, stVencFrame, s32SetFrameMilliSec);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VENC_SendFrame failed! %d\n", s32Ret);
    return s32Ret;
  }

  s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VENC_GetChnAttr, VencChn[%d], s32Ret = %d\n", VencChn, s32Ret);
    return s32Ret;
  }

  s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VENC_QueryStatus failed with %#x!\n", s32Ret);
    return s32Ret;
  }
  if (!stStat.u32CurPacks) {
    printf("NOTE: Current frame is NULL!\n");
    return s32Ret;
  }

  stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
  if (stStream.pstPack == NULL) {
    printf("malloc memory failed!\n");
    return s32Ret;
  }

  s32Ret = CVI_VENC_GetStream(VencChn, &stStream, -1);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VENC_GetStream failed with %#x!\n", s32Ret);
    free(stStream.pstPack);
    stStream.pstPack = NULL;
    return s32Ret;
  }

  VENC_PACK_S *ppack;
  CVI_RTSP_DATA data = {0};
  memset(&data, 0, sizeof(CVI_RTSP_DATA));

  data.blockCnt = stStream.u32PackCount;
  for (unsigned int i = 0; i < stStream.u32PackCount; i++) {
    ppack = &stStream.pstPack[i];
    data.dataPtr[i] = ppack->pu8Addr + ppack->u32Offset;
    data.dataLen[i] = ppack->u32Len - ppack->u32Offset;
  }

  CVI_RTSP_WriteFrame(context->rtsp_context, session->video, &data);

  s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
  if (s32Ret != CVI_SUCCESS) {
    printf("CVI_VENC_ReleaseStream, s32Ret = %d\n", s32Ret);
    free(stStream.pstPack);
    stStream.pstPack = NULL;
    return s32Ret;
  }

  free(stStream.pstPack);
  stStream.pstPack = NULL;
  return s32Ret;
}

CVI_S32 SendOutputFrame(VIDEO_FRAME_INFO_S *stVencFrame, OutputContext *context) {
  if (context->type == OUTPUT_TYPE_PANEL) {
    return panel_send_frame(stVencFrame, context);
  } else if (context->type == OUTPUT_TYPE_RTSP) {
    return rtsp_send_frame(stVencFrame, context);
  } else {
    printf("Failed to send output frame: Wrong output type(%x)\n", context->type);
    return CVI_FAILURE;
  }
}

CVI_S32 DestoryOutput(OutputContext *context) {
  if (context->type == OUTPUT_TYPE_RTSP) {
    CVI_RTSP_Stop(context->rtsp_context);
    CVI_RTSP_DestroySession(context->rtsp_context, context->session);
    CVI_RTSP_Destroy(&context->rtsp_context);
    SAMPLE_COMM_VENC_Stop(0);
  }
  return CVI_SUCCESS;
}
