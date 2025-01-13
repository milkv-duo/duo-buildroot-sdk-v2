#include "app_ipcam_websocket.h"
#include "middleware_utils.h"

CVI_S32 SAMPLE_TDL_Init_WM_NO_RTSP(SAMPLE_TDL_MW_CONFIG_S *pstMWConfig,
                                   SAMPLE_TDL_MW_CONTEXT *pstMWContext) {
  MMF_VERSION_S stVersion;
  CVI_SYS_GetVersion(&stVersion);
  printf("MMF Version:%s\n", stVersion.version);

  if (pstMWConfig->stViConfig.s32WorkingViNum <= 0) {
    printf("Invalidate working vi number: %u\n", pstMWConfig->stViConfig.s32WorkingViNum);
    return CVI_FAILURE;
  }

  // Set sensor number
  CVI_VI_SetDevNum(pstMWConfig->stViConfig.s32WorkingViNum);

  // Setup VB
  if (pstMWConfig->stVBPoolConfig.u32VBPoolCount <= 0 ||
      pstMWConfig->stVBPoolConfig.u32VBPoolCount >= VB_MAX_COMM_POOLS) {
    printf("Invalid number of vb pool: %u, which is outside the valid range of 1 to (%u - 1)\n",
           pstMWConfig->stVBPoolConfig.u32VBPoolCount, VB_MAX_COMM_POOLS);
    return CVI_FAILURE;
  }

  VB_CONFIG_S stVbConf;
  memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

  stVbConf.u32MaxPoolCnt = pstMWConfig->stVBPoolConfig.u32VBPoolCount;
  CVI_U32 u32TotalBlkSize = 0;
  for (uint32_t u32VBIndex = 0; u32VBIndex < stVbConf.u32MaxPoolCnt; u32VBIndex++) {
    CVI_U32 u32BlkSize =
        COMMON_GetPicBufferSize(pstMWConfig->stVBPoolConfig.astVBPoolSetup[u32VBIndex].u32Width,
                                pstMWConfig->stVBPoolConfig.astVBPoolSetup[u32VBIndex].u32Height,
                                pstMWConfig->stVBPoolConfig.astVBPoolSetup[u32VBIndex].enFormat,
                                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[u32VBIndex].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[u32VBIndex].u32BlkCnt =
        pstMWConfig->stVBPoolConfig.astVBPoolSetup[u32VBIndex].u32BlkCount;
    CVI_U32 u32PoolSize = (u32BlkSize * stVbConf.astCommPool[u32VBIndex].u32BlkCnt);
    u32TotalBlkSize += u32PoolSize;
    printf("Create VBPool[%u], size: (%u * %u) = %u bytes\n", u32VBIndex, u32BlkSize,
           stVbConf.astCommPool[u32VBIndex].u32BlkCnt, u32PoolSize);
  }
  printf("Total memory of VB pool: %u bytes\n", u32TotalBlkSize);

  CVI_S32 s32Ret;
#ifndef _MIDDLEWARE_V3_
  // Init system & vb pool
  printf("Initialize SYS and VB\n");
  s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
  if (s32Ret != CVI_SUCCESS) {
    printf("system init failed with %#x\n", s32Ret);
    return s32Ret;
  }
#endif

  // Init VI

  printf("Initialize VI\n");
  VI_VPSS_MODE_S stVIVPSSMode;
  stVIVPSSMode.aenMode[0] = VI_OFFLINE_VPSS_ONLINE;
  CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);

  memcpy(&pstMWContext->stViConfig, &pstMWConfig->stViConfig, sizeof(SAMPLE_VI_CONFIG_S));

  s32Ret = SAMPLE_PLAT_VI_INIT(&pstMWConfig->stViConfig);
  if (s32Ret != CVI_SUCCESS) {
    printf("vi init failed. s32Ret: 0x%x !\n", s32Ret);
    goto vi_start_error;
  }

  // Init VPSS
  printf("Initialize VPSS\n");
  memcpy(&pstMWContext->stVPSSPoolConfig, &pstMWConfig->stVPSSPoolConfig,
         sizeof(SAMPLE_TDL_VPSS_POOL_CONFIG_S));
#ifndef __CV186X__
  VPSS_MODE_S stVPSSMode = pstMWConfig->stVPSSPoolConfig.stVpssMode;
  CVI_SYS_SetVPSSModeEx(&stVPSSMode);
#endif

  for (uint32_t u32VpssGrpIndex = 0;
       u32VpssGrpIndex < pstMWConfig->stVPSSPoolConfig.u32VpssGrpCount; u32VpssGrpIndex++) {
    SAMPLE_TDL_VPSS_CONFIG_S *pstVPSSConf =
        &pstMWConfig->stVPSSPoolConfig.astVpssConfig[u32VpssGrpIndex];
    VPSS_GRP_ATTR_S *pstGrpAttr = &pstVPSSConf->stVpssGrpAttr;
    VPSS_CHN_ATTR_S *pastChnAttr = pstVPSSConf->astVpssChnAttr;
    printf("---------VPSS[%u]---------\n", u32VpssGrpIndex);
    printf("Input size: (%ux%u)\n", pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
    printf("Input format: (%d)\n", pstGrpAttr->enPixelFormat);
    printf("VPSS physical device number: %u\n", pstGrpAttr->u8VpssDev);
    printf("Src Frame Rate: %d\n", pstGrpAttr->stFrameRate.s32SrcFrameRate);
    printf("Dst Frame Rate: %d\n", pstGrpAttr->stFrameRate.s32DstFrameRate);

    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM + 1] = {0};
    for (uint32_t u32ChnIndex = 0; u32ChnIndex < pstVPSSConf->u32ChnCount; u32ChnIndex++) {
      abChnEnable[u32ChnIndex] = true;
      printf("    --------CHN[%u]-------\n", u32ChnIndex);
      printf("    Output size: (%ux%u)\n", pastChnAttr[u32ChnIndex].u32Width,
             pastChnAttr[u32ChnIndex].u32Height);
      printf("    Depth: %u\n", pastChnAttr[u32ChnIndex].u32Depth);
      printf("    Do normalization: %d\n", pastChnAttr[u32ChnIndex].stNormalize.bEnable);
      if (pastChnAttr[u32ChnIndex].stNormalize.bEnable) {
        printf("        factor=[%f, %f, %f]\n", pastChnAttr[u32ChnIndex].stNormalize.factor[0],
               pastChnAttr[u32ChnIndex].stNormalize.factor[1],
               pastChnAttr[u32ChnIndex].stNormalize.factor[2]);
        printf("        mean=[%f, %f, %f]\n", pastChnAttr[u32ChnIndex].stNormalize.mean[0],
               pastChnAttr[u32ChnIndex].stNormalize.mean[1],
               pastChnAttr[u32ChnIndex].stNormalize.mean[2]);
        printf("        rounding=%d\n", pastChnAttr[u32ChnIndex].stNormalize.rounding);
      }
      printf("        Src Frame Rate: %d\n", pastChnAttr[u32ChnIndex].stFrameRate.s32SrcFrameRate);
      printf("        Dst Frame Rate: %d\n", pastChnAttr[u32ChnIndex].stFrameRate.s32DstFrameRate);
      printf("    ----------------------\n");
    }
    printf("------------------------\n");

    s32Ret = SAMPLE_COMM_VPSS_Init(u32VpssGrpIndex, abChnEnable, pstGrpAttr, pastChnAttr);
    if (s32Ret != CVI_SUCCESS) {
      printf("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
      goto vpss_start_error;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(u32VpssGrpIndex, abChnEnable, pstGrpAttr, pastChnAttr);
    if (s32Ret != CVI_SUCCESS) {
      printf("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
      goto vpss_start_error;
    }

    if (pstVPSSConf->bBindVI) {
      printf("Bind VI with VPSS Grp(%u), Chn(%u)\n", u32VpssGrpIndex, pstVPSSConf->u32ChnBindVI);
#if (defined _MIDDLEWARE_V2_ || defined _MIDDLEWARE_V3_)
      s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, pstVPSSConf->u32ChnBindVI, u32VpssGrpIndex);
#else
      s32Ret = SAMPLE_COMM_VI_Bind_VPSS(pstVPSSConf->u32ChnBindVI, u32VpssGrpIndex);
#endif
      if (s32Ret != CVI_SUCCESS) {
        printf("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto vpss_start_error;
      }
    }
  }

  // Attach VB to VPSS
  for (CVI_U32 u32VBPoolIndex = 0; u32VBPoolIndex < pstMWConfig->stVBPoolConfig.u32VBPoolCount;
       u32VBPoolIndex++) {
    SAMPLE_TDL_VB_CONFIG_S *pstVBConfig =
        &pstMWConfig->stVBPoolConfig.astVBPoolSetup[u32VBPoolIndex];
    if (pstVBConfig->bBind) {
      printf("Attach VBPool(%u) to VPSS Grp(%u) Chn(%u)\n", u32VBPoolIndex,
             pstVBConfig->u32VpssGrpBinding, pstVBConfig->u32VpssChnBinding);

      s32Ret = CVI_VPSS_AttachVbPool(pstVBConfig->u32VpssGrpBinding, pstVBConfig->u32VpssChnBinding,
                                     (VB_POOL)u32VBPoolIndex);
      if (s32Ret != CVI_SUCCESS) {
        printf("Cannot attach VBPool(%u) to VPSS Grp(%u) Chn(%u): ret=%x\n", u32VBPoolIndex,
               pstVBConfig->u32VpssGrpBinding, pstVBConfig->u32VpssChnBinding, s32Ret);
        goto vpss_start_error;
      }
    }
  }

  // Init VENC
  VENC_GOP_ATTR_S stGopAttr;

  // Always use venc chn0 in sample.
  pstMWContext->u32VencChn = 0;

  SAMPLE_COMM_CHN_INPUT_CONFIG_S *pstInputConfig = &pstMWConfig->stVencConfig.stChnInputCfg;

  printf("Initialize VENC\n");
  s32Ret = SAMPLE_COMM_VENC_GetGopAttr(VENC_GOPMODE_NORMALP, &stGopAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("Venc Get GopAttr for %#x!\n", s32Ret);
    goto venc_start_error;
  }

  printf("venc codec: %s\n", pstInputConfig->codec);
  printf("venc frame size: %ux%u\n", pstMWConfig->stVencConfig.u32FrameWidth,
         pstMWConfig->stVencConfig.u32FrameHeight);

  PAYLOAD_TYPE_E enPayLoad;
  if (!strcmp(pstInputConfig->codec, "h264")) {
    enPayLoad = PT_H264;
  } else if (!strcmp(pstInputConfig->codec, "h265")) {
    enPayLoad = PT_H265;
  } else {
    printf("Unsupported encode format in sample: %s\n", pstInputConfig->codec);
    s32Ret = CVI_FAILURE;
    goto venc_start_error;
  }

  PIC_SIZE_E enPicSize = SAMPLE_TDL_Get_PIC_Size(pstMWConfig->stVencConfig.u32FrameWidth,
                                                 pstMWConfig->stVencConfig.u32FrameHeight);
  if (enPicSize == PIC_BUTT) {
    s32Ret = CVI_FAILURE;
    printf("Cannot get PIC SIZE from VENC frame size: %ux%u\n",
           pstMWConfig->stVencConfig.u32FrameWidth, pstMWConfig->stVencConfig.u32FrameHeight);
    goto venc_start_error;
  }

  s32Ret = SAMPLE_COMM_VENC_Start(pstInputConfig, pstMWContext->u32VencChn, enPayLoad, enPicSize,
                                  pstInputConfig->rcMode, 0, CVI_FALSE, &stGopAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("Venc Start failed for %#x!\n", s32Ret);
    goto venc_start_error;
  }

  return CVI_SUCCESS;

venc_start_error:
vpss_start_error:
  SAMPLE_COMM_VI_DestroyIsp(&pstMWConfig->stViConfig);
  SAMPLE_COMM_VI_DestroyVi(&pstMWConfig->stViConfig);
vi_start_error:
#ifndef _MIDDLEWARE_V3_
  SAMPLE_COMM_SYS_Exit();
#endif

  return s32Ret;
}

CVI_S32 SAMPLE_TDL_Send_Frame_WEB(VIDEO_FRAME_INFO_S *stVencFrame,
                                  SAMPLE_TDL_MW_CONTEXT *pstMWContext) {
  CVI_S32 s32Ret = CVI_SUCCESS;

  CVI_S32 s32SetFrameMilliSec = 20000;
  VENC_STREAM_S stStream;
  VENC_CHN_ATTR_S stVencChnAttr;
  VENC_CHN_STATUS_S stStat;
  VENC_CHN VencChn = pstMWContext->u32VencChn;

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
    goto send_failed;
  }

  s32Ret = app_ipcam_WebSocket_Stream_Send(&stStream, NULL);
  if (s32Ret != CVI_SUCCESS) {
    printf("app_ipcam_WebSocket_Stream_Send, s32Ret = %d\n", s32Ret);
    goto send_failed;
  }

send_failed:
  free(stStream.pstPack);
  CVI_VENC_ReleaseStream(VencChn, &stStream);
  return s32Ret;
}
