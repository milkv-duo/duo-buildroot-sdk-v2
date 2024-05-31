#pragma once
#include <signal.h>
#include "cvi_sys.h"
#include "cvi_type.h"
#include "sample_comm.h"

#define CVI_MAPI_SUCCESS ((int)(0))
#define CVI_MAPI_ERR_FAILURE       ((int)(-1001))
#define CVI_MAPI_ERR_NOMEM         ((int)(-1002))
#define CVI_MAPI_ERR_TIMEOUT       ((int)(-1003))
#define CVI_MAPI_ERR_INVALID       ((int)(-1004))

enum YuvType {
  YUV_UNKNOWN = 0,
  YUV420_PLANAR = 1,
  YUV_NV12 = 2,
  YUV_NV21 = 3
};

struct PreprocessArg {
  int width;
  int height;
  YuvType yuv_type;
};


static void _SYS_HandleSig(int nSignal, siginfo_t *si, void *arg)
{
    SAMPLE_COMM_SYS_Exit();
    exit(1);
}

int CVI_MAPI_Media_Init(uint32_t img_w, uint32_t img_h, uint32_t blk_cnt)
{
    SIZE_S stSize;
    stSize.u32Width  = img_w;
    stSize.u32Height = img_h;

    VB_CONFIG_S      stVbConf;
    CVI_U32          u32BlkSize, u32BlkRotSize;
    COMPRESS_MODE_E  enCompressMode = COMPRESS_MODE_NONE;

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = _SYS_HandleSig;
    sa.sa_flags = SA_SIGINFO|SA_RESETHAND;    // Reset signal handler to system default after signal triggered
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt        = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT,
        DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
    u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, SAMPLE_PIXEL_FORMAT,
        DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
    u32BlkSize = u32BlkSize > u32BlkRotSize ? u32BlkSize : u32BlkRotSize;

    stVbConf.astCommPool[0].u32BlkSize    = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt     = blk_cnt;  // 10
    stVbConf.astCommPool[0].enRemapMode   = VB_REMAP_MODE_CACHED;
    printf("common pool[0] BlkSize %d\n", u32BlkSize);

    u32BlkSize = COMMON_GetPicBufferSize(1920, 1080, PIXEL_FORMAT_RGB_888_PLANAR,
        DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);

    stVbConf.astCommPool[1].u32BlkSize    = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt    = 2;
    printf("common pool[1] BlkSize %d\n", u32BlkSize);

    int ret = CVI_MAPI_SUCCESS;
    CVI_S32 rc = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (rc != CVI_SUCCESS) {
        printf("SAMPLE_COMM_SYS_Init fail, rc = %#x\n", rc);
        ret = CVI_MAPI_ERR_FAILURE;
        goto error;
    }

    return ret;

error:
    SAMPLE_COMM_SYS_Exit();
    return ret;
}

int CVI_MAPI_Media_Deinit(void)
{
    SAMPLE_COMM_SYS_Exit();
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_AllocateFrame(VIDEO_FRAME_INFO_S *frm,
        uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt) {
    VB_BLK blk;
    VB_CAL_CONFIG_S stVbCalConfig;
    COMMON_GetPicBufferConfig(width, height, fmt, DATA_BITWIDTH_8,
                              COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

    frm->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    frm->stVFrame.enPixelFormat = fmt;
    frm->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    frm->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    frm->stVFrame.u32Width = width;
    frm->stVFrame.u32Height = height;
    frm->stVFrame.u32Stride[0] = stVbCalConfig.u32MainStride;
    frm->stVFrame.u32Stride[1] = stVbCalConfig.u32CStride;
    frm->stVFrame.u32Stride[2] = stVbCalConfig.u32CStride;
    frm->stVFrame.u32TimeRef = 0;
    frm->stVFrame.u64PTS = 0;
    frm->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    printf("Allocate VB block with size %d\n", stVbCalConfig.u32VBSize);

    blk = CVI_VB_GetBlock(VB_INVALID_POOLID, stVbCalConfig.u32VBSize);
    if (blk == (unsigned long)CVI_INVALID_HANDLE) {
        printf("Can't acquire VB block for size %d\n",
            stVbCalConfig.u32VBSize);
        return CVI_MAPI_ERR_FAILURE;
    }

    frm->u32PoolId = CVI_VB_Handle2PoolId(blk);
    frm->stVFrame.u32Length[0] = ALIGN(stVbCalConfig.u32MainYSize,
                                       stVbCalConfig.u16AddrAlign);
    frm->stVFrame.u32Length[1] = frm->stVFrame.u32Length[2]
                               = ALIGN(stVbCalConfig.u32MainCSize,
                                       stVbCalConfig.u16AddrAlign);

    frm->stVFrame.u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
    frm->stVFrame.u64PhyAddr[1] = frm->stVFrame.u64PhyAddr[0]
                                  + frm->stVFrame.u32Length[0];
    frm->stVFrame.u64PhyAddr[2] = frm->stVFrame.u64PhyAddr[1]
                                  + frm->stVFrame.u32Length[1];
    for (int i = 0; i < 3; ++i) {
        frm->stVFrame.pu8VirAddr[i] = NULL;
    }

    return CVI_MAPI_SUCCESS;
}

static void get_frame_plane_num_and_mem_size(VIDEO_FRAME_INFO_S *frm,
    int *plane_num, size_t *mem_size)
{
    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_HSV_888_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT32_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT32_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT16_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT16_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR) {
        *plane_num = 3;
        // check phyaddr
        assert(frm->stVFrame.u64PhyAddr[1] - frm->stVFrame.u64PhyAddr[0]
                       == frm->stVFrame.u32Length[0] &&
                       "phy addr not continue 0");
        assert(frm->stVFrame.u64PhyAddr[2] - frm->stVFrame.u64PhyAddr[1]
                       == frm->stVFrame.u32Length[1] &&
                       "phy addr not continue 1");
    } else {
        *plane_num = 1;
    }

    *mem_size = 0;
    for (int i = 0; i < *plane_num; ++i) {
        *mem_size += frm->stVFrame.u32Length[i];
    }
}

int CVI_MAPI_FrameMmap(VIDEO_FRAME_INFO_S *frm, bool enable_cache) {
    int plane_num = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr = NULL;
    if (enable_cache) {
        vir_addr = CVI_SYS_MmapCache(frm->stVFrame.u64PhyAddr[0], mem_size);
    } else {
        vir_addr = CVI_SYS_Mmap(frm->stVFrame.u64PhyAddr[0], mem_size);
    }
    assert(vir_addr && "mmap failed\n");

    //CVI_SYS_IonInvalidateCache(frm->stVFrame.u64PhyAddr[0], vir_addr, mem_size);
    uint64_t plane_offset = 0;
    for (int i = 0; i < plane_num; ++i) {
        frm->stVFrame.pu8VirAddr[i] = (uint8_t *)vir_addr + plane_offset;
        plane_offset += frm->stVFrame.u32Length[i];
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_FrameMunmap(VIDEO_FRAME_INFO_S *frm)
{
    int plane_num = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr = (void *)frm->stVFrame.pu8VirAddr[0];
    CVI_SYS_Munmap(vir_addr, mem_size);

    for (int i = 0; i < plane_num; ++i) {
        frm->stVFrame.pu8VirAddr[i] = NULL;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_FrameFlushCache(VIDEO_FRAME_INFO_S *frm)
{
    int plane_num = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr = (void *)frm->stVFrame.pu8VirAddr[0];
    uint64_t phy_addr = frm->stVFrame.u64PhyAddr[0];

    CVI_SYS_IonFlushCache(phy_addr, vir_addr, mem_size);
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_ReleaseFrame(VIDEO_FRAME_INFO_S *frm)
{
    VB_BLK blk = CVI_VB_PhysAddr2Handle(frm->stVFrame.u64PhyAddr[0]);
    CVI_VB_ReleaseBlock(blk);
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_SaveFramePixelData(VIDEO_FRAME_INFO_S *frm, const char *name)
{
    #define FILENAME_MAX_LEN    (128)
    char filename[FILENAME_MAX_LEN] = {0};
    char const *extension = NULL;
    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
        extension = "yuv";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
        extension = "chw";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888) {
        extension = "rgb";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888) {
        extension = "bgr";
    } else {
        assert(0 && "Invalid frm pixel format");
    }
    snprintf(filename, FILENAME_MAX_LEN, "%s_%dX%d.%s", name,
            frm->stVFrame.u32Width,
            frm->stVFrame.u32Height,
            extension);

    FILE *output;
    output = fopen(filename, "wb");
    assert(output && "file open failed");

    printf("Save %s, w*h(%d*%d)\n",
             filename,
             frm->stVFrame.u32Width,
             frm->stVFrame.u32Height);

    CVI_MAPI_FrameMmap(frm, false);

    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
        for (int i = 0; i < 3; ++i) {
            printf("  plane(%d): paddr(0x%lx) vaddr(%p) stride(%d) length(%d)\n",
                    i,
                    frm->stVFrame.u64PhyAddr[i],
                    frm->stVFrame.pu8VirAddr[i],
                    frm->stVFrame.u32Stride[i],
                    frm->stVFrame.u32Length[i]);
            //TODO: test unaligned image
            uint32_t length = (i == 0 ? frm->stVFrame.u32Height : frm->stVFrame.u32Height / 2);
            uint32_t step = (i == 0 ? frm->stVFrame.u32Width : frm->stVFrame.u32Width / 2);
            uint8_t *ptr = (uint8_t *)frm->stVFrame.pu8VirAddr[i];
            for (int j = 0; j < length; ++j) {
                fwrite(ptr, step, 1, output);
                ptr += frm->stVFrame.u32Stride[i];
            }
        }
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR
            || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
        for (int i = 0; i < 3; i++) {
            printf("  plane(%d): paddr(0x%lx) vaddr(%p) stride(%d) length(%d)\n",
                    i,
                    frm->stVFrame.u64PhyAddr[i],
                    frm->stVFrame.pu8VirAddr[i],
                    frm->stVFrame.u32Stride[i],
                    frm->stVFrame.u32Length[i]);
            unsigned char *ptr = frm->stVFrame.pu8VirAddr[i];
            for (int j = 0; j < frm->stVFrame.u32Height; ++j) {
                fwrite(ptr, frm->stVFrame.u32Width, 1, output);
                ptr += frm->stVFrame.u32Stride[i];
            }
        }
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888
               || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888) {
        printf("  packed: paddr(0x%lx) vaddr(%p) stride(%d) length(%d)\n",
                frm->stVFrame.u64PhyAddr[0],
                frm->stVFrame.pu8VirAddr[0],
                frm->stVFrame.u32Stride[0],
                frm->stVFrame.u32Length[0]);
        uint8_t *ptr = frm->stVFrame.pu8VirAddr[0];
        for (int j = 0; j < frm->stVFrame.u32Height; ++j)
        {
            fwrite(ptr, frm->stVFrame.u32Width * 3, 1, output);
            ptr += frm->stVFrame.u32Stride[0];
        }
    } else {
        printf(0, "Invalid frm pixel format");
    }

    CVI_MAPI_FrameMunmap(frm);

    fclose(output);
    return CVI_MAPI_SUCCESS;
}

static CVI_S32 vproc_init(VPSS_GRP VpssGrp, CVI_BOOL *pabChnEnable, VPSS_GRP_ATTR_S *pstVpssGrpAttr,
                  VPSS_CHN_ATTR_S *pastVpssChnAttr)
{
    VPSS_CHN VpssChn = 0;
    CVI_S32 s32Ret;
    CVI_S32 j;

    s32Ret = CVI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return s32Ret;
    }

    s32Ret = CVI_VPSS_ResetGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VPSS_ResetGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        goto exit1;
    }

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if (pabChnEnable[j]) {
            VpssChn = j;
            s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &pastVpssChnAttr[VpssChn]);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                goto exit2;
            }

            s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
            if (s32Ret != CVI_SUCCESS) {
                printf("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
                goto exit2;
            }
        }
    }

    s32Ret = CVI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        printf("CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
        goto exit2;
    }
    return CVI_SUCCESS;

exit2:
    for(j = 0;j < VpssChn; j++){
        if (CVI_VPSS_DisableChn(VpssGrp, j) != CVI_SUCCESS) {
            printf("CVI_VPSS_DisableChn failed!\n");
        }
    }
exit1:
    if (CVI_VPSS_DestroyGrp(VpssGrp) != CVI_SUCCESS) {
        printf("CVI_VPSS_DestroyGrp(grp:%d) failed!\n", VpssGrp);
    }

    return s32Ret;
}

static CVI_S32 set_vpss_config(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S* stVpssGrpAttr, PreprocessArg *arg)
{
	VPSS_CHN VpssChn = VPSS_CHN0;
	CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = { 0 };
	VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (VpssGrp == 0) {
	//channel0 
		abChnEnable[VpssChn] = CVI_TRUE;
		astVpssChnAttr[VpssChn].u32Width = arg->width;
		astVpssChnAttr[VpssChn].u32Height = arg->height;
		astVpssChnAttr[VpssChn].enVideoFormat = VIDEO_FORMAT_LINEAR;
    if (arg->yuv_type == YUV420_PLANAR) {
		  astVpssChnAttr[VpssChn].enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    } else if (arg->yuv_type == YUV_NV12) {
		  astVpssChnAttr[VpssChn].enPixelFormat = PIXEL_FORMAT_NV12;
    } else {
		  astVpssChnAttr[VpssChn].enPixelFormat = PIXEL_FORMAT_NV21;
    }
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
		astVpssChnAttr[VpssChn].u32Depth = 1;
		astVpssChnAttr[VpssChn].bMirror = false;
		astVpssChnAttr[VpssChn].bFlip = false;

		astVpssChnAttr[VpssChn].stAspectRatio.enMode = ASPECT_RATIO_AUTO;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor  = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable = CVI_FALSE;
	} else {
    return -1;
  }
	CVI_SYS_SetVPSSMode(VPSS_MODE_SINGLE);

	/*start vpss*/
  s32Ret = vproc_init(VpssGrp, abChnEnable, stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}
	return s32Ret;
}

int init_vpss(int in_width, int in_height, PreprocessArg *arg) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	CVI_S32 vpssgrp_width = in_width;
	CVI_S32 vpssgrp_height = in_height;
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
	stVpssGrpAttr.enPixelFormat = PIXEL_FORMAT_BGR_888;
	stVpssGrpAttr.u32MaxW = vpssgrp_width;
	stVpssGrpAttr.u32MaxH = vpssgrp_height;
	// only for test here. u8VpssDev should be decided by VPSS_MODE and usage.
	stVpssGrpAttr.u8VpssDev = 0;
	s32Ret = set_vpss_config(0, &stVpssGrpAttr, arg);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_Init_Video_Process Grp0 failed with %d\n", s32Ret);
		return s32Ret;
	}
	return s32Ret;
}


static CVI_VOID vproc_deinit()
{
    CVI_S32 j;
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = CVI_VPSS_DisableChn(0, 0);
    if (s32Ret != CVI_SUCCESS) {
        printf("failed with %#x!\n", s32Ret);
    }
    s32Ret = CVI_VPSS_StopGrp(0);
    if (s32Ret != CVI_SUCCESS) {
        printf("failed with %#x!\n", s32Ret);
    }
    s32Ret = CVI_VPSS_DestroyGrp(0);
    if (s32Ret != CVI_SUCCESS) {
        printf("failed with %#x!\n", s32Ret);
    }
}
