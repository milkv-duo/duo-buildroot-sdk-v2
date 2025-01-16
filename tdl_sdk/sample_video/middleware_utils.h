#ifndef __MIDDLEWARE_UTILS_H__
#define __MIDDLEWARE_UTILS_H__
#include <cvi_comm.h>
#include <pthread.h>
#include <rtsp.h>
#include <sample_comm.h>

/**
 * @brief configuration for a VBPool
 * @var u32Width
 * Width of video block
 * @var u32Height
 * Height of video block
 * @var u32BlkCount
 * Number of video block in this pool
 * @var enFormat
 * Pixel format of video block
 * @var u32VpssGrpBinding
 * Which VPSS group would be binded
 * @var u32VpssChnBinding
 * Which VPSS channel would be binded
 * @var bBind
 * Binding to a specific VPSS Grp
 */

typedef struct {
  CVI_U32 u32Width;
  CVI_U32 u32Height;
  CVI_U32 u32BlkCount;
  PIXEL_FORMAT_E enFormat;
  VPSS_GRP u32VpssGrpBinding;
  VPSS_CHN u32VpssChnBinding;
  CVI_BOOL bBind;
} SAMPLE_TDL_VB_CONFIG_S;

/**
 * @brief structure for VB pool configurations
 * @var astVBPoolSetup
 * VB configurations
 * @var u32VBPoolCount
 * Number of VBPool would be created.
 */
typedef struct {
  SAMPLE_TDL_VB_CONFIG_S astVBPoolSetup[VB_MAX_COMM_POOLS];
  CVI_U32 u32VBPoolCount;
} SAMPLE_TDL_VB_POOL_CONFIG_S;

/**
 * @brief structure for a VPSS Grp configuration
 * @var stVpssGrpAttr
 * VPSS group attributes
 * @var astVpssChnAttr
 * VPSS channel attributes
 * @var u32ChnCount
 * Number of enabled VPSS channel.
 * @var bBindVI
 * Should bind VPSS with VI or not.
 * @var u32ChnBindVI
 * Channel ID to bind with VI.
 */
typedef struct {
  VPSS_GRP_ATTR_S stVpssGrpAttr;
  VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
  CVI_U32 u32ChnCount;
  CVI_BOOL bBindVI;
  CVI_U32 u32ChnBindVI;
} SAMPLE_TDL_VPSS_CONFIG_S;

/**
 * @brief structure for VPSS configurations
 * @var astVpssConfig
 * An array of VPSS configurations.
 * @var u32VpssGrpCount
 * Number of enabled VPSS grp.
 * @var enVpssMode
 * Vpss mode
 */
typedef struct {
  SAMPLE_TDL_VPSS_CONFIG_S astVpssConfig[VPSS_MAX_GRP_NUM];
  CVI_U32 u32VpssGrpCount;
#ifndef __CV186X__
  VPSS_MODE_S stVpssMode;
#endif
} SAMPLE_TDL_VPSS_POOL_CONFIG_S;

typedef chnInputCfg SAMPLE_COMM_CHN_INPUT_CONFIG_S;

/**
 * @brief video encoder configurations
 * @var stChnInputCfg
 * Input channel configuration
 * @var u32FrameHeight
 * Height of frame
 * @var u32FrameWidth
 * Width of frame
 */
typedef struct {
  SAMPLE_COMM_CHN_INPUT_CONFIG_S stChnInputCfg;
  CVI_U32 u32FrameHeight;
  CVI_U32 u32FrameWidth;
} SAMPLE_TDL_VENC_CONFIG_S;

/**
 * @brief rtsp configuration
 * @var stRTSPConfig
 * structure of rtsp config
 * @var onConnect
 * Callback function to be called if client connect to RTSP server.
 * @var onDisconnect
 * A callback function to be called if client disconnect to RTSP server.
 */
typedef struct {
  CVI_RTSP_CONFIG stRTSPConfig;
  struct {
    void (*onConnect)(const char *ip, void *arg);
    void (*onDisconnect)(const char *ip, void *arg);
  } Lisener;
} SAMPLE_TDL_RTSP_CONFIG;

/**
 * @brief structure for middleware configurations
 * @var stViConfig
 * VI configuration
 * @var stVPSSPoolConfig
 * VPSS configurations
 * @var stVBPoolConfig
 * Video block configureations
 * @var stRTSPConfig
 * RTSP configuration
 * @var stVencConfig
 * VENC Video encoder configurations
 */
typedef struct {
  SAMPLE_VI_CONFIG_S stViConfig;
  SAMPLE_TDL_VPSS_POOL_CONFIG_S stVPSSPoolConfig;
  SAMPLE_TDL_VB_POOL_CONFIG_S stVBPoolConfig;
  SAMPLE_TDL_RTSP_CONFIG stRTSPConfig;
  SAMPLE_TDL_VENC_CONFIG_S stVencConfig;
} SAMPLE_TDL_MW_CONFIG_S;

/**
 * @brief A context structure for middleware
 * @var pstRtspContext
 * pointer to RTSP context
 * @var pstSession
 * pointer to RTSP session
 * @var stViConfig
 * VI configuration
 * @var u32VencChn
 * Video encoder channal number
 * @var stVPSSPoolConfig
 */
typedef struct {
  CVI_RTSP_CTX *pstRtspContext;
  CVI_RTSP_SESSION *pstSession;
  SAMPLE_VI_CONFIG_S stViConfig;
  CVI_U32 u32VencChn;
  SAMPLE_TDL_VPSS_POOL_CONFIG_S stVPSSPoolConfig;
} SAMPLE_TDL_MW_CONTEXT;

/**
 * @brief get vi configurations from ini file (/mnt/data/sensor_cfg.ini).
 * @param pstViConfig vi config.
 * @return CV_S32 return code
 */
CVI_S32 SAMPLE_TDL_Get_VI_Config(SAMPLE_VI_CONFIG_S *pstViConfig);

/**
 * @brief Get default VENC input configuration
 *
 * @param pstInCfg output, pointer to config structure.
 */
void SAMPLE_TDL_Get_Input_Config(SAMPLE_COMM_CHN_INPUT_CONFIG_S *pstInCfg);

/**
 * @brief Get default RTSP configurations
 * @param pstRTSPConfig rtsp config
 */
void SAMPLE_TDL_Get_RTSP_Config(CVI_RTSP_CONFIG *pstRTSPConfig);

/**
 * @brief Convert width and height to PIC_SIZE_E
 *
 * @param width width
 * @param height height
 * @return PIC_SIZE_E enum of PIC size
 */
PIC_SIZE_E SAMPLE_TDL_Get_PIC_Size(CVI_S32 width, CVI_S32 height);

/**
 * @brief initialize cvitek middleware.
 * @param pstMWConfig input middleware configurations.
 * @param pstMWContext output context of middleware stuff.
 * @return CV_S32 return CVI_SUCCESS if initialize middleware successfully, otherwise return error
 * code instead.
 */
CVI_S32 SAMPLE_TDL_Init_WM(SAMPLE_TDL_MW_CONFIG_S *pstMWConfig,
                           SAMPLE_TDL_MW_CONTEXT *pstMWContext);

/**
 * @brief initialize cvitek middleware.
 * @param pstMWConfig input middleware configurations.
 * @param pstMWContext output context of middleware stuff.
 * @return CV_S32 return CVI_SUCCESS if initialize middleware successfully, otherwise return error
 * code instead.
 */
CVI_S32 SAMPLE_TDL_Init_WM_NO_RTSP(SAMPLE_TDL_MW_CONFIG_S *pstMWConfig,
                                   SAMPLE_TDL_MW_CONTEXT *pstMWContext);

/**
 * @brief Send video frame to RTSP and Venc.
 *
 * @param stVencFrame frame to send
 * @param pstMWContext middleware context
 * @return CVI_S32 CVI_SUCCESS if operation is success, otherwise return CVI_FAILURE
 */
CVI_S32 SAMPLE_TDL_Send_Frame_RTSP(VIDEO_FRAME_INFO_S *stVencFrame,
                                   SAMPLE_TDL_MW_CONTEXT *pstMWContext);

/**
 * @brief Send video frame to RTSP and Venc.
 *
 * @param stVencFrame frame to send
 * @param pstMWContext middleware context
 * @return CVI_S32 CVI_SUCCESS if operation is success, otherwise return CVI_FAILURE
 */
CVI_S32 SAMPLE_TDL_Send_Frame_WEB(VIDEO_FRAME_INFO_S *stVencFrame,
                                  SAMPLE_TDL_MW_CONTEXT *pstMWContext);

/**
 * @brief Destroy middleware
 *
 * @param pstMWContext middleware context
 */
void SAMPLE_TDL_Destroy_MW(SAMPLE_TDL_MW_CONTEXT *pstMWContext);

/**
 * @brief Destroy middleware
 *
 * @param pstMWContext middleware context
 */
void SAMPLE_TDL_Destroy_MW_NO_RTSP(SAMPLE_TDL_MW_CONTEXT *pstMWContext);
#endif