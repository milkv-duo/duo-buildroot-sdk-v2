#ifndef VI_VO_UTILS_H_
#define VI_VO_UTILS_H_

#include <cvi_sys.h>
#include <cvi_vi.h>
#include <pthread.h>
#include <rtsp.h>

#include "sample_comm.h"

typedef enum { OUTPUT_TYPE_PANEL, OUTPUT_TYPE_RTSP } OutputType;

typedef struct {
  VPSS_GRP vpssGrp;
  CVI_U32 grpHeight;
  CVI_U32 grpWidth;
  PIXEL_FORMAT_E groupFormat;

  VPSS_CHN vpssChnVideoOutput;
  CVI_U32 voWidth;
  CVI_U32 voHeight;
  PIXEL_FORMAT_E voFormat;

  VPSS_CHN vpssChntdl;
  CVI_U32 tdlWidth;
  CVI_U32 tdlHeight;
  PIXEL_FORMAT_E tdlFormat;

  VI_PIPE viPipe;
} VPSSConfigs;

typedef struct {
  OutputType type;
  union {
    struct {
      CVI_S32 voLayer;
      CVI_S32 voChn;
    };

    struct {
      CVI_RTSP_STATE_LISTENER listener;
      CVI_RTSP_CTX *rtsp_context;
      CVI_RTSP_SESSION *session;
      chnInputCfg input_cfg;
    };
  };
} OutputContext;

typedef struct {
  VPSSConfigs vpssConfigs;
  OutputContext outputContext;
  SAMPLE_VI_CONFIG_S viConfig;

  struct {
    CVI_U32 DevNum;
    VI_PIPE ViPipe;
  } ViPipe;

} VideoSystemContext;

#ifdef __cplusplus
extern "C" {
#endif

CVI_S32 InitVideoSystem(VideoSystemContext *vsCtx, int fps);
void DestroyVideoSystem(VideoSystemContext *vsCtx);
CVI_S32 SendOutputFrame(VIDEO_FRAME_INFO_S *stVencFrame, OutputContext *context);

#ifdef __cplusplus
}
#endif

#endif