#ifndef _CVI_MEDIA_SDK_H_
#define _CVI_MEDIA_SDK_H_

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "sample_comm.h"
#include "cvi_sys.h"
#include "cvi_type.h"


#define CVI_MAPI_SUCCESS           ((int)(0))
#define CVI_MAPI_ERR_FAILURE       ((int)(-1001))
#define CVI_MAPI_ERR_NOMEM         ((int)(-1002))
#define CVI_MAPI_ERR_TIMEOUT       ((int)(-1003))
#define CVI_MAPI_ERR_INVALID       ((int)(-1004))
#define MAX_VPSS_GRP_NUM    (16)
#define CVI_MAPI_VPROC_MAX_CHN_NUM    (3)

#ifndef SAMPLE_CHECK_RET
#define SAMPLE_CHECK_RET(express)                                                    \
    do {                                                                      \
        int rc = express;                                                     \
        if (rc != 0) {                                                        \
            printf("\nFailed at %s: %d  (rc:0x%#x!)\n",                       \
                    __FILE__, __LINE__, rc);                                  \
            return rc;                                                        \
        }                                                                     \
    } while (0)
#endif


#ifndef UNUSED
# define UNUSED(x) x=x
#endif

#ifndef CVI_LOG_ASSERT
#define CVI_LOG_ASSERT(x, ...)     \
    do {                           \
        if (!(x)) {                \
            printf(__VA_ARGS__);   \
			abort();               \
        }                          \
    } while(0)
#endif
#define CVI_LOGE(...)   printf(__VA_ARGS__)
#define CVI_LOGI(...)   printf(__VA_ARGS__)
#define CVI_LOGV(...)   printf(__VA_ARGS__)
#define CVI_LOGV_MEM(...)   printf(__VA_ARGS__)

#define CVI_MAPI_VB_POOL_MAX_NUM (16)

#define CHECK_VPROC_GRP(grp) do { \
        if (grp >= MAX_VPSS_GRP_NUM) { \
            CVI_LOGE("VprocGrp(%d) exceeds Max(%d)\n", grp, MAX_VPSS_GRP_NUM); \
            return CVI_MAPI_ERR_INVALID; \
        } \
    } while (0)

#define CHECK_VPROC_CHN(chn) do { \
    if (chn >= CVI_MAPI_VPROC_MAX_CHN_NUM) { \
        CVI_LOGE("VprocGrp(%d) exceeds Max(%d)\n", chn, CVI_MAPI_VPROC_MAX_CHN_NUM); \
        return CVI_MAPI_ERR_INVALID; \
    } \
} while (0)

#define VPROC_CHECK_NULL_PTR(ptr) \
        do { \
            if (!(ptr)) { \
                CVI_LOGE(" NULL pointer\n"); \
                return CVI_MAPI_ERR_INVALID; \
            } \
        } while (0)

typedef struct CVI_MAPI_MEDIA_SYS_VB_POOL_S {
    union cvi_vb_blk_size {
        uint32_t                   size;
        struct cvi_vb_blk_frame_s {
            uint32_t             width;
            uint32_t             height;
            PIXEL_FORMAT_E       fmt;
        } frame;
    } vb_blk_size;
    bool                         is_frame;
    uint32_t                     vb_blk_num;
} CVI_MAPI_MEDIA_SYS_VB_POOL_T;


typedef struct CVI_MAPI_MEDIA_SYS_ATTR_S {
    CVI_MAPI_MEDIA_SYS_VB_POOL_T vb_pool[CVI_MAPI_VB_POOL_MAX_NUM];
    uint32_t                     vb_pool_num;
    VI_VPSS_MODE_S stVIVPSSMode;
    VPSS_MODE_S stVPSSMode;
} CVI_MAPI_MEDIA_SYS_ATTR_T;

typedef void * CVI_MAPI_HANDLE_T;


typedef struct CVI_MAPI_PREPROCESS_ATTR_S {
    bool is_rgb;           // default false
    float raw_scale;       // default 255.0 means no raw_scale
    float mean[3];         // in BGR order
    float input_scale[3];  // in BGR order, combined input_scale and std[3]
    float qscale;
} CVI_MAPI_PREPROCESS_ATTR_T;

typedef CVI_MAPI_HANDLE_T CVI_MAPI_VPROC_HANDLE_T;


typedef struct CVI_MAPI_VPROC_ATTR_S {
    VPSS_GRP_ATTR_S   attr_inp;
    int               chn_num;
    VPSS_CHN_ATTR_S   attr_chn[CVI_MAPI_VPROC_MAX_CHN_NUM];
} CVI_MAPI_VPROC_ATTR_T;

typedef int (*PFN_VPROC_FrameDataProc)(uint32_t Grp,
    uint32_t Chn, VIDEO_FRAME_INFO_S *pFrame, void *pPrivateData);

typedef struct CVI_DUMP_FRAME_CALLBACK_FUNC_S {
    PFN_VPROC_FrameDataProc pfunFrameProc;
    void *pPrivateData;
} CVI_DUMP_FRAME_CALLBACK_FUNC_T;

typedef struct VPROC_DUMP_CTX_S {
    CVI_BOOL bStart;
    CVI_U32 Grp;
    CVI_S32 Chn;
    CVI_S32 s32Count;
    CVI_DUMP_FRAME_CALLBACK_FUNC_T stCallbackFun;
    pthread_t pthreadDump;
} VPROC_DUMP_CTX_T;

typedef struct CVI_MAPI_EXTCHN_ATTR_S {
    uint32_t        ChnId;
    uint32_t        BindVprocChnId;
    VPSS_CHN_ATTR_S VpssChnAttr;
} CVI_MAPI_EXTCHN_ATTR_T;

typedef struct EXT_VPROC_CHN_CTX_S {
    CVI_U32     ExtChnGrp;
    CVI_BOOL    ExtChnEnable;
    CVI_MAPI_EXTCHN_ATTR_T ExtChnAttr;
} EXT_VPROC_CHN_CTX_T;

typedef struct CVI_MAPI_VPROC_CTX_S {
    VPSS_GRP VpssGrp;
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM];
    CVI_MAPI_VPROC_ATTR_T attr;
    EXT_VPROC_CHN_CTX_T ExtChn[VPSS_MAX_PHY_CHN_NUM];
    VPROC_DUMP_CTX_T stVprocDumpCtx;
} CVI_MAPI_VPROC_CTX_T;

typedef struct CVI_MAPI_IPROC_RECT_S {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} CVI_MAPI_IPROC_RECT_T, CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T;


int CVI_MAPI_Media_Init(CVI_MAPI_MEDIA_SYS_ATTR_T *attr);
int CVI_MAPI_Media_Deinit(void);
int CVI_MAPI_ReleaseFrame(VIDEO_FRAME_INFO_S *frm);
int CVI_MAPI_GetFrameFromMemory_YUV(VIDEO_FRAME_INFO_S *frm,
        uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt, void *data);
int CVI_MAPI_GetFrameFromFile_YUV(VIDEO_FRAME_INFO_S *frame,
        uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt,
        const char *filaneme, uint32_t frame_no);
int CVI_MAPI_FrameMmap(VIDEO_FRAME_INFO_S *frm, bool enable_cache);
int CVI_MAPI_FrameMunmap(VIDEO_FRAME_INFO_S *frm);
int CVI_MAPI_SaveFramePixelData(VIDEO_FRAME_INFO_S *frm, const char *name);
int CVI_MAPI_AllocateFrame(VIDEO_FRAME_INFO_S *frm,
        uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt);
int CVI_MAPI_FrameMmap(VIDEO_FRAME_INFO_S *frm, bool enable_cache);
int CVI_MAPI_FrameMunmap(VIDEO_FRAME_INFO_S *frm);
int CVI_MAPI_FrameFlushCache(VIDEO_FRAME_INFO_S *frm);
int CVI_MAPI_FrameInvalidateCache(VIDEO_FRAME_INFO_S *frm);

void CVI_MAPI_PREPROCESS_ENABLE(VPSS_CHN_ATTR_S *attr_chn,
        CVI_MAPI_PREPROCESS_ATTR_T *preprocess);
CVI_MAPI_VPROC_ATTR_T CVI_MAPI_VPROC_DefaultAttr_OneChn(
    uint32_t          width_in,
    uint32_t          height_in,
    PIXEL_FORMAT_E    pixel_format_in,
    uint32_t          width_out,
    uint32_t          height_out,
    PIXEL_FORMAT_E    pixel_format_out);

CVI_MAPI_VPROC_ATTR_T CVI_MAPI_VPROC_DefaultAttr_TwoChn(
    uint32_t          width_in,
    uint32_t          height_in,
    PIXEL_FORMAT_E    pixel_format_in,
    uint32_t          width_out0,
    uint32_t          height_out0,
    PIXEL_FORMAT_E    pixel_format_out0,
    uint32_t          width_out1,
    uint32_t          height_out1,
    PIXEL_FORMAT_E    pixel_format_out1);

int CVI_MAPI_VPROC_Init(CVI_MAPI_VPROC_HANDLE_T *vproc_hdl,
        int grp_id, CVI_MAPI_VPROC_ATTR_T *attr);
int CVI_MAPI_VPROC_Deinit(CVI_MAPI_VPROC_HANDLE_T vproc_hdl);
int CVI_MAPI_VPROC_GetGrp(CVI_MAPI_VPROC_HANDLE_T vproc_hdl);
int CVI_MAPI_VPROC_SendFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
        VIDEO_FRAME_INFO_S *frame);
int CVI_MAPI_VPROC_GetChnFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
        uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame);
int CVI_MAPI_VPROC_SendChnFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
        uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame);
int CVI_MAPI_VPROC_ReleaseFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
        uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame);
int CVI_MAPI_IPROC_Resize(VIDEO_FRAME_INFO_S *frame_in,
        VIDEO_FRAME_INFO_S *frame_out,
        uint32_t resize_width,
        uint32_t resize_height,
        PIXEL_FORMAT_E fmt_out,
        bool keep_aspect_ratio,
        CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T *crop_in,
        CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T *crop_out,
        CVI_MAPI_PREPROCESS_ATTR_T *preprocess);
int CVI_MAPI_IPROC_ReleaseFrame(VIDEO_FRAME_INFO_S *frm);
int CVI_MAPI_IPROC_Deinit();
#endif
