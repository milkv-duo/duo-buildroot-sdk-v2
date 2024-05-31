#include "cvi_media_sdk.h"
#include <inttypes.h>
///
/// SYS
///

static int iproc_grp = 1;

static void _SYS_HandleSig(int nSignal, siginfo_t *si, void *arg) {
    SAMPLE_COMM_SYS_Exit();

    exit(1);
}

static uint32_t get_frame_size(uint32_t w, uint32_t h, PIXEL_FORMAT_E fmt) {
    // try rotate and non-rotate, choose the larger one
    uint32_t sz_0 = COMMON_GetPicBufferSize(w, h, fmt,
                                            DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    uint32_t sz_1 = COMMON_GetPicBufferSize(h, w, fmt,
                                            DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    return (sz_0 > sz_1) ? sz_0 : sz_1;
}

static int COMM_SYS_Init(VB_CONFIG_S *pstVbConfig)
{
    CVI_S32 s32Ret = CVI_FAILURE;

    CVI_SYS_Exit();
    CVI_VB_Exit();

    if (pstVbConfig == NULL) {
        CVI_LOGE("input parameter is null, it is invaild!\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    s32Ret = CVI_VB_SetConfig(pstVbConfig);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_VB_SetConf failed!\n");
        return s32Ret;
    }

    s32Ret = CVI_VB_Init();
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_VB_Init failed!\n");
        return s32Ret;
    }

    s32Ret = CVI_SYS_Init();
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_SYS_Init failed!\n");
        CVI_VB_Exit();
        return s32Ret;
    }

    return CVI_MAPI_SUCCESS;
}

static void COMM_SYS_Exit(void)
{
    //CVI_VB_ExitModCommPool(VB_UID_VDEC);
    CVI_VB_Exit();
    CVI_SYS_Exit();
}

int CVI_MAPI_Media_Init(CVI_MAPI_MEDIA_SYS_ATTR_T *attr) {

    //struct sigaction sa;
    //memset(&sa, 0, sizeof(struct sigaction));
    //sigemptyset(&sa.sa_mask);
    //sa.sa_sigaction = _SYS_HandleSig;
    //sa.sa_flags = SA_SIGINFO|SA_RESETHAND;    // Reset signal handler to system default after signal triggered
    //sigaction(SIGINT, &sa, NULL);
    //sigaction(SIGTERM, &sa, NULL);

    VB_CONFIG_S stVbConf;
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    for (unsigned i = 0; i < attr->vb_pool_num; i++) {
        uint32_t blk_size;
        if (attr->vb_pool[i].is_frame) {
            blk_size = get_frame_size(
                attr->vb_pool[i].vb_blk_size.frame.width,
                attr->vb_pool[i].vb_blk_size.frame.height,
                attr->vb_pool[i].vb_blk_size.frame.fmt);
        } else {
            blk_size = attr->vb_pool[i].vb_blk_size.size;
        }
        uint32_t blk_num = attr->vb_pool[i].vb_blk_num;

        stVbConf.astCommPool[i].u32BlkSize  = blk_size;
        stVbConf.astCommPool[i].u32BlkCnt   = blk_num;
        stVbConf.astCommPool[i].enRemapMode = VB_REMAP_MODE_CACHED;
        CVI_LOGI("VB pool[%d] BlkSize %d BlkCnt %d\n", i, blk_size, blk_num);
    }
    stVbConf.u32MaxPoolCnt = attr->vb_pool_num;

    int ret    = CVI_MAPI_SUCCESS;
    CVI_S32 rc = COMM_SYS_Init(&stVbConf);
    if (rc != CVI_MAPI_SUCCESS) {
        CVI_LOGE("COMM_SYS_Init fail, rc = %#x\n", rc);
        ret = CVI_MAPI_ERR_FAILURE;
        goto error;
    }

    rc = CVI_SYS_SetVIVPSSMode(&attr->stVIVPSSMode);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("CVI_SYS_SetVIVPSSMode failed with %#x\n", rc);
        ret = CVI_MAPI_ERR_FAILURE;
        goto error;
    }

    rc = CVI_SYS_SetVPSSModeEx(&attr->stVPSSMode);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("CVI_SYS_SetVPSSModeEx failed with %#x\n", rc);
        ret = CVI_MAPI_ERR_FAILURE;
        goto error;
    }

    return ret;

error:
    COMM_SYS_Exit();
    return ret;
}

int CVI_MAPI_Media_Deinit(void) {
    COMM_SYS_Exit();
    return CVI_MAPI_SUCCESS;
}

//
// VB Frame helper functions
//
int CVI_MAPI_ReleaseFrame(VIDEO_FRAME_INFO_S *frm) {
    VB_BLK blk = CVI_VB_PhysAddr2Handle(frm->stVFrame.u64PhyAddr[0]);
    CVI_VB_ReleaseBlock(blk);
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_GetFrameFromMemory_YUV(VIDEO_FRAME_INFO_S *frm,
                                    uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt, void *data) {
    CVI_LOG_ASSERT(fmt == PIXEL_FORMAT_YUV_PLANAR_420,
                   "Not support fmt %d yet\n", fmt);
    CVI_LOG_ASSERT(width % 2 == 0, "width not align\n");
    CVI_LOG_ASSERT(height % 2 == 0, "height not align\n");

    VB_BLK blk;
    VB_CAL_CONFIG_S stVbCalConfig;

    COMMON_GetPicBufferConfig(width, height, fmt, DATA_BITWIDTH_8,
                              COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

    frm->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    frm->stVFrame.enPixelFormat  = fmt;
    frm->stVFrame.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    frm->stVFrame.enColorGamut   = COLOR_GAMUT_BT709;
    frm->stVFrame.u32Width       = width;
    frm->stVFrame.u32Height      = height;
    frm->stVFrame.u32Stride[0]   = stVbCalConfig.u32MainStride;
    frm->stVFrame.u32Stride[1]   = stVbCalConfig.u32CStride;
    frm->stVFrame.u32Stride[2]   = stVbCalConfig.u32CStride;
    frm->stVFrame.u32TimeRef     = 0;
    frm->stVFrame.u64PTS         = 0;
    frm->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    blk = CVI_VB_GetBlock(VB_INVALID_POOLID, stVbCalConfig.u32VBSize);
    if (blk == VB_INVALID_HANDLE) {
        SAMPLE_PRT("Can't acquire vb block\n");
        return CVI_FAILURE;
    }

    frm->u32PoolId             = CVI_VB_Handle2PoolId(blk);
    frm->stVFrame.u32Length[0] = ALIGN(stVbCalConfig.u32MainYSize,
                                       stVbCalConfig.u16AddrAlign);
    frm->stVFrame.u32Length[1] = frm->stVFrame.u32Length[2] = ALIGN(stVbCalConfig.u32MainCSize,
                                                                    stVbCalConfig.u16AddrAlign);

    frm->stVFrame.u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
    frm->stVFrame.u64PhyAddr[1] = frm->stVFrame.u64PhyAddr[0] + frm->stVFrame.u32Length[0];
    frm->stVFrame.u64PhyAddr[2] = frm->stVFrame.u64PhyAddr[1] + frm->stVFrame.u32Length[1];
    uint32_t image_size         = frm->stVFrame.u32Length[0] + frm->stVFrame.u32Length[1] + frm->stVFrame.u32Length[2];
    frm->stVFrame.pu8VirAddr[0] = (uint8_t *)CVI_SYS_MmapCache(
        frm->stVFrame.u64PhyAddr[0], image_size);
    frm->stVFrame.pu8VirAddr[1] = frm->stVFrame.pu8VirAddr[0] + frm->stVFrame.u32Length[0];
    frm->stVFrame.pu8VirAddr[2] = frm->stVFrame.pu8VirAddr[1] + frm->stVFrame.u32Length[1];

    uint8_t *data_ptr = (uint8_t *)data;
    for (int i = 0; i < 3; ++i) {
        if (frm->stVFrame.u32Length[i] == 0)
            continue;

        uint32_t height_step = (i == 0) ? frm->stVFrame.u32Height : frm->stVFrame.u32Height / 2;
        uint32_t width_step  = (i == 0) ? frm->stVFrame.u32Width : frm->stVFrame.u32Width / 2;
        uint8_t *frm_ptr     = frm->stVFrame.pu8VirAddr[i];
        for (uint32_t j = 0; j < height_step; ++j) {
            memcpy(frm_ptr, data_ptr, width_step);
            frm_ptr += frm->stVFrame.u32Stride[i];
            data_ptr += width_step;
        }
        CVI_SYS_IonFlushCache(frm->stVFrame.u64PhyAddr[i],
                              frm->stVFrame.pu8VirAddr[i],
                              frm->stVFrame.u32Length[i]);
    }
    CVI_SYS_Munmap(frm->stVFrame.pu8VirAddr[0], image_size);
    for (int i = 0; i < 3; ++i) {
        frm->stVFrame.pu8VirAddr[i] = NULL;
    }

    return CVI_MAPI_SUCCESS;
}

static uint32_t getFrameSize_YUV(uint32_t w, uint32_t h, PIXEL_FORMAT_E fmt) {
    if (fmt == PIXEL_FORMAT_YUV_PLANAR_420) {
        return (w * h * 3) >> 1;
    } else if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
        return (w * h * 2);
    } else {
        CVI_LOG_ASSERT(0, "Unsupported fmt %d\n", fmt);
    }
    return 0;
}

int CVI_MAPI_GetFrameFromFile_YUV(VIDEO_FRAME_INFO_S *frame,
                                  uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt,
                                  const char *filaneme, uint32_t frame_no) {
    FILE *fp = fopen(filaneme, "rb");
    if (fp == NULL) {
        CVI_LOGE("Input file %s open failed !\n", filaneme);
        return CVI_MAPI_ERR_FAILURE;
    }

    uint32_t frame_size = getFrameSize_YUV(width, height, fmt);
    char *data          = (char *)malloc(frame_size);
    if (!data) {
        CVI_LOGE("malloc frame buffer failed\n");
        fclose(fp);
        return CVI_MAPI_ERR_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    unsigned int file_size  = ftell(fp);
    unsigned int num_frames = file_size / frame_size;
    if (num_frames < (frame_no + 1)) {
        CVI_LOGE("file %s size %d to small, frame_size %d, no. %d\n",
                 filaneme, file_size, frame_size, frame_no);
        free(data);
        fclose(fp);
        return CVI_MAPI_ERR_FAILURE;
    }
    rewind(fp);

    fseek(fp, frame_size * frame_no, SEEK_SET);
    fread((void *)data, 1, frame_size, fp);
    int ret = CVI_MAPI_GetFrameFromMemory_YUV(frame,
                                              width, height, fmt, data);

    free(data);
    fclose(fp);
    return ret;
}

/// utils function
int CVI_MAPI_SaveFramePixelData(VIDEO_FRAME_INFO_S *frm, const char *name) {
#define FILENAME_MAX_LEN (128)
    char filename[FILENAME_MAX_LEN] = {0};
    const char *extension           = NULL;
    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
        extension = "yuv";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV12) {
        extension = "nv12";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
        extension = "nv21";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
        extension = "chw";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888) {
        extension = "rgb";
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888) {
        extension = "bgr";
    } else {
        CVI_LOG_ASSERT(0, "Invalid frm pixel format %d\n",
                       frm->stVFrame.enPixelFormat);
    }
    snprintf(filename, FILENAME_MAX_LEN, "%s_%dX%d.%s", name,
             frm->stVFrame.u32Width,
             frm->stVFrame.u32Height,
             extension);

    FILE *output;
    output = fopen(filename, "wb");
    CVI_LOG_ASSERT(output, "file open failed\n");

    CVI_LOGI("Save %s, w*h(%d*%d)\n",
             filename,
             frm->stVFrame.u32Width,
             frm->stVFrame.u32Height);

    //CVI_LOG_ASSERT((frm->stVFrame.pu8VirAddr[0] == NULL), "frame VirAddr failed\n");
    if (CVI_MAPI_FrameMmap(frm, true) != CVI_MAPI_SUCCESS) {
        CVI_LOGE("CVI_MAPI_FrameMmap failed\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
        for (int i = 0; i < 3; ++i) {
            CVI_LOGV("  plane(%d): paddr(0x%" PRIx64 ") vaddr(%p) stride(%d) length(%d)\n",
                     i,
                     frm->stVFrame.u64PhyAddr[i],
                     frm->stVFrame.pu8VirAddr[i],
                     frm->stVFrame.u32Stride[i],
                     frm->stVFrame.u32Length[i]);
            //TODO: test unaligned image
            uint32_t length = (i == 0 ? frm->stVFrame.u32Height : frm->stVFrame.u32Height / 2);
            uint32_t step   = (i == 0 ? frm->stVFrame.u32Width : frm->stVFrame.u32Width / 2);
            uint8_t *ptr    = (uint8_t *)frm->stVFrame.pu8VirAddr[i];
            for (unsigned j = 0; j < length; ++j) {
                fwrite(ptr, step, 1, output);
                ptr += frm->stVFrame.u32Stride[i];
            }
        }
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV12 || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
        for (int i = 0; i < 2; ++i) {
            CVI_LOGV("  plane(%d): paddr(0x%" PRIx64 ") vaddr(%p) stride(%d) length(%d)\n",
                     i,
                     frm->stVFrame.u64PhyAddr[i],
                     frm->stVFrame.pu8VirAddr[i],
                     frm->stVFrame.u32Stride[i],
                     frm->stVFrame.u32Length[i]);
            //TODO: test unaligned image
            uint32_t length = (i == 0 ? frm->stVFrame.u32Height : frm->stVFrame.u32Height / 2);
            uint32_t step   = frm->stVFrame.u32Width;
            uint8_t *ptr    = (uint8_t *)frm->stVFrame.pu8VirAddr[i];
            for (unsigned j = 0; j < length; ++j) {
                fwrite(ptr, step, 1, output);
                ptr += frm->stVFrame.u32Stride[i];
            }
        }
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
        for (int i = 0; i < 3; i++) {
            CVI_LOGV("  plane(%d): paddr(0x%" PRIx64 ") vaddr(%p) stride(%d) length(%d)\n",
                     i,
                     frm->stVFrame.u64PhyAddr[i],
                     frm->stVFrame.pu8VirAddr[i],
                     frm->stVFrame.u32Stride[i],
                     frm->stVFrame.u32Length[i]);
            uint8_t *ptr = frm->stVFrame.pu8VirAddr[i];
            for (unsigned j = 0; j < frm->stVFrame.u32Height; ++j) {
                fwrite(ptr, frm->stVFrame.u32Width, 1, output);
                ptr += frm->stVFrame.u32Stride[i];
            }
        }
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888 || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888) {
        CVI_LOGV("  packed: paddr(0x%" PRIx64 ") vaddr(%p) stride(%d) length(%d)\n",
                 frm->stVFrame.u64PhyAddr[0],
                 frm->stVFrame.pu8VirAddr[0],
                 frm->stVFrame.u32Stride[0],
                 frm->stVFrame.u32Length[0]);
        uint8_t *ptr = frm->stVFrame.pu8VirAddr[0];
        for (unsigned j = 0; j < frm->stVFrame.u32Height; ++j) {
            fwrite(ptr, frm->stVFrame.u32Width * 3, 1, output);
            ptr += frm->stVFrame.u32Stride[0];
        }
    } else {
        CVI_LOG_ASSERT(0, "Invalid frm pixel format %d\n",
                       frm->stVFrame.enPixelFormat);
    }

    CVI_MAPI_FrameMunmap(frm);

    fclose(output);
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_AllocateFrame(VIDEO_FRAME_INFO_S *frm,
                           uint32_t width, uint32_t height, PIXEL_FORMAT_E fmt) {
    VB_BLK blk;
    VB_CAL_CONFIG_S stVbCalConfig;
    COMMON_GetPicBufferConfig(width, height, fmt, DATA_BITWIDTH_8,
                              COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

    memset(frm, 0x00, sizeof(*frm));
    frm->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    frm->stVFrame.enPixelFormat  = fmt;
    frm->stVFrame.enVideoFormat  = VIDEO_FORMAT_LINEAR;
    frm->stVFrame.enColorGamut   = COLOR_GAMUT_BT709;
    frm->stVFrame.u32Width       = width;
    frm->stVFrame.u32Height      = height;
    frm->stVFrame.u32Stride[0]   = stVbCalConfig.u32MainStride;
    frm->stVFrame.u32Stride[1]   = stVbCalConfig.u32CStride;
    frm->stVFrame.u32Stride[2]   = stVbCalConfig.u32CStride;
    frm->stVFrame.u32TimeRef     = 0;
    frm->stVFrame.u64PTS         = 0;
    frm->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

    CVI_LOGV("Allocate VB block with size %d\n", stVbCalConfig.u32VBSize);

    blk = CVI_VB_GetBlock(VB_INVALID_POOLID, stVbCalConfig.u32VBSize);
    if (blk == (unsigned long)CVI_INVALID_HANDLE) {
        SAMPLE_PRT("Can't acquire cv block\n");
        return CVI_FAILURE;
    }

    frm->u32PoolId             = CVI_VB_Handle2PoolId(blk);
    frm->stVFrame.u32Length[0] = ALIGN(stVbCalConfig.u32MainYSize,
                                       stVbCalConfig.u16AddrAlign);
    frm->stVFrame.u32Length[1] = frm->stVFrame.u32Length[2] = ALIGN(stVbCalConfig.u32MainCSize,
                                                                    stVbCalConfig.u16AddrAlign);

    frm->stVFrame.u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
    frm->stVFrame.u64PhyAddr[1] = frm->stVFrame.u64PhyAddr[0] + frm->stVFrame.u32Length[0];
    frm->stVFrame.u64PhyAddr[2] = frm->stVFrame.u64PhyAddr[1] + frm->stVFrame.u32Length[1];

    return CVI_MAPI_SUCCESS;
}

static void get_frame_plane_num_and_mem_size(VIDEO_FRAME_INFO_S *frm,
                                             int *plane_num, size_t *mem_size) {
    if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422 || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420 || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444 || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_HSV_888_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT32_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT32_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT16_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT16_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR || frm->stVFrame.enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR) {
        *plane_num = 3;
        // check phyaddr
        CVI_LOG_ASSERT(frm->stVFrame.u64PhyAddr[1] - frm->stVFrame.u64PhyAddr[0] == frm->stVFrame.u32Length[0],
                       "phy addr not continue 0, fmt = %d\n",
                       frm->stVFrame.enPixelFormat);
        CVI_LOG_ASSERT(frm->stVFrame.u64PhyAddr[2] - frm->stVFrame.u64PhyAddr[1] == frm->stVFrame.u32Length[1],
                       "phy addr not continue 1, fmt = %d\n",
                       frm->stVFrame.enPixelFormat);
    } else if (frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV12 ||
                 frm->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
        *plane_num = 2;
        CVI_LOG_ASSERT(frm->stVFrame.u64PhyAddr[1] - frm->stVFrame.u64PhyAddr[0] == frm->stVFrame.u32Length[0],
                       "phy addr not continue 0, fmt = %d\n",
                       frm->stVFrame.enPixelFormat);
    } else {
        *plane_num = 1;
    }

    *mem_size = 0;
    for (int i = 0; i < *plane_num; ++i) {
        *mem_size += frm->stVFrame.u32Length[i];
    }
}

int CVI_MAPI_FrameMmap(VIDEO_FRAME_INFO_S *frm, bool enable_cache) {
    int plane_num   = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr = NULL;
    if (enable_cache) {
        vir_addr = CVI_SYS_MmapCache(frm->stVFrame.u64PhyAddr[0], mem_size);
    } else {
        vir_addr = CVI_SYS_Mmap(frm->stVFrame.u64PhyAddr[0], mem_size);
    }
    CVI_LOG_ASSERT(vir_addr, "mmap failed\n");

    //CVI_SYS_IonInvalidateCache(frm->stVFrame.u64PhyAddr[0], vir_addr, mem_size);
    uint64_t plane_offset = 0;
    for (int i = 0; i < plane_num; ++i) {
        frm->stVFrame.pu8VirAddr[i] = (uint8_t *)vir_addr + plane_offset;
        plane_offset += frm->stVFrame.u32Length[i];
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_FrameMunmap(VIDEO_FRAME_INFO_S *frm) {
    int plane_num   = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr = (void *)frm->stVFrame.pu8VirAddr[0];
    CVI_SYS_Munmap(vir_addr, mem_size);

    for (int i = 0; i < plane_num; ++i) {
        frm->stVFrame.pu8VirAddr[i] = NULL;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_FrameFlushCache(VIDEO_FRAME_INFO_S *frm) {
    int plane_num   = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr    = (void *)frm->stVFrame.pu8VirAddr[0];
    uint64_t phy_addr = frm->stVFrame.u64PhyAddr[0];

    CVI_SYS_IonFlushCache(phy_addr, vir_addr, mem_size);
    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_FrameInvalidateCache(VIDEO_FRAME_INFO_S *frm) {
    int plane_num   = 0;
    size_t mem_size = 0;
    get_frame_plane_num_and_mem_size(frm, &plane_num, &mem_size);

    void *vir_addr    = (void *)frm->stVFrame.pu8VirAddr[0];
    uint64_t phy_addr = frm->stVFrame.u64PhyAddr[0];

    CVI_SYS_IonInvalidateCache(phy_addr, vir_addr, mem_size);

    return CVI_MAPI_SUCCESS;
}


///
/// Preprocess
///

void CVI_MAPI_PREPROCESS_ENABLE(VPSS_CHN_ATTR_S *attr_chn,
                                CVI_MAPI_PREPROCESS_ATTR_T *preprocess) {
    attr_chn->stNormalize.bEnable = CVI_TRUE;
    if (preprocess->is_rgb) {
        CVI_LOG_ASSERT(attr_chn->enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR,
                       "Preprocess (RGB) enabled, fmt_out needs to be "
                       "PIXEL_FORMAT_RGB_888_PLANAR\n");
    } else {
        CVI_LOG_ASSERT(attr_chn->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR,
                       "Preprocess (BGR) enabled, fmt_out needs to be "
                       "PIXEL_FORMAT_BGR_888_PLANAR\n");
    }

    float quant_scale = preprocess->qscale;
    float factor[3];
    float mean[3];
    for (int i = 0; i < 3; i++) {
        factor[i] = preprocess->raw_scale / 255.0f;
        factor[i] *= preprocess->input_scale[i] * quant_scale;
        if (factor[i] < 1.0f / 8192) {
            factor[i] = 1.0f / 8192;
        }
        if (factor[i] > 8191.0f / 8192) {
            factor[i] = 8191.0f / 8192;
        }

        mean[i] = preprocess->mean[i];
        mean[i] *= preprocess->input_scale[i] * quant_scale;
    }
    // mean and factor are supposed to be in BGR, swap R&B if RGB
    if (preprocess->is_rgb) {
        float tmp;
        tmp       = factor[0];
        factor[0] = factor[2];
        factor[2] = tmp;
        tmp       = mean[0];
        mean[0]   = mean[2];
        mean[2]   = tmp;
    }
    for (int i = 0; i < 3; i++) {
        attr_chn->stNormalize.factor[i] = factor[i];
        attr_chn->stNormalize.mean[i]   = mean[i];
    }
    attr_chn->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
}

///
/// VPROC
///

CVI_MAPI_VPROC_ATTR_T CVI_MAPI_VPROC_DefaultAttr_OneChn(
    uint32_t width_in,
    uint32_t height_in,
    PIXEL_FORMAT_E pixel_format_in,
    uint32_t width_out,
    uint32_t height_out,
    PIXEL_FORMAT_E pixel_format_out) {
    CVI_MAPI_VPROC_ATTR_T attr;
    memset((void *)&attr, 0, sizeof(attr));

    attr.attr_inp.stFrameRate.s32SrcFrameRate = -1;
    attr.attr_inp.stFrameRate.s32DstFrameRate = -1;
    attr.attr_inp.enPixelFormat               = pixel_format_in;
    attr.attr_inp.u32MaxW                     = width_in;
    attr.attr_inp.u32MaxH                     = height_in;
    attr.attr_inp.u8VpssDev                   = 0;    

    attr.chn_num = 1;

    attr.attr_chn[0].u32Width                    = width_out;
    attr.attr_chn[0].u32Height                   = height_out;
    attr.attr_chn[0].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    attr.attr_chn[0].enPixelFormat               = pixel_format_out;
    attr.attr_chn[0].stFrameRate.s32SrcFrameRate = -1;
    attr.attr_chn[0].stFrameRate.s32DstFrameRate = -1;
    attr.attr_chn[0].u32Depth                    = 1;  // output buffer queue size
    attr.attr_chn[0].bMirror                     = CVI_FALSE;
    attr.attr_chn[0].bFlip                       = CVI_FALSE;
    attr.attr_chn[0].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    attr.attr_chn[0].stNormalize.bEnable         = CVI_FALSE;

    return attr;
}

CVI_MAPI_VPROC_ATTR_T CVI_MAPI_VPROC_DefaultAttr_TwoChn(
    uint32_t width_in,
    uint32_t height_in,
    PIXEL_FORMAT_E pixel_format_in,
    uint32_t width_out0,
    uint32_t height_out0,
    PIXEL_FORMAT_E pixel_format_out0,
    uint32_t width_out1,
    uint32_t height_out1,
    PIXEL_FORMAT_E pixel_format_out1) {
    CVI_MAPI_VPROC_ATTR_T attr;
    memset((void *)&attr, 0, sizeof(attr));

    attr.attr_inp.stFrameRate.s32SrcFrameRate = -1;
    attr.attr_inp.stFrameRate.s32DstFrameRate = -1;
    attr.attr_inp.enPixelFormat               = pixel_format_in;
    attr.attr_inp.u32MaxW                     = width_in;
    attr.attr_inp.u32MaxH                     = height_in;
    attr.attr_inp.u8VpssDev                   = 0;    

    attr.chn_num = 2;

    attr.attr_chn[0].u32Width                    = width_out0;
    attr.attr_chn[0].u32Height                   = height_out0;
    attr.attr_chn[0].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    attr.attr_chn[0].enPixelFormat               = pixel_format_out0;
    attr.attr_chn[0].stFrameRate.s32SrcFrameRate = -1;
    attr.attr_chn[0].stFrameRate.s32DstFrameRate = -1;
    attr.attr_chn[0].u32Depth                    = 1;  // output buffer queue size
    attr.attr_chn[0].bMirror                     = CVI_FALSE;
    attr.attr_chn[0].bFlip                       = CVI_FALSE;
    attr.attr_chn[0].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    attr.attr_chn[0].stNormalize.bEnable         = CVI_FALSE;

    attr.attr_chn[1].u32Width                    = width_out1;
    attr.attr_chn[1].u32Height                   = height_out1;
    attr.attr_chn[1].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    attr.attr_chn[1].enPixelFormat               = pixel_format_out1;
    attr.attr_chn[1].stFrameRate.s32SrcFrameRate = -1;
    attr.attr_chn[1].stFrameRate.s32DstFrameRate = -1;
    attr.attr_chn[1].u32Depth                    = 1;  // output buffer queue size
    attr.attr_chn[1].bMirror                     = CVI_FALSE;
    attr.attr_chn[1].bFlip                       = CVI_FALSE;
    attr.attr_chn[1].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    attr.attr_chn[1].stNormalize.bEnable         = CVI_FALSE;

    return attr;
}

static pthread_mutex_t vproc_mutex       = PTHREAD_MUTEX_INITIALIZER;
static bool g_grp_used[MAX_VPSS_GRP_NUM] = {0};
static bool g_vproc_initialized          = false;

CVI_S32 Vproc_Init(VPSS_GRP VpssGrp, CVI_BOOL *pabChnEnable, VPSS_GRP_ATTR_S *pstVpssGrpAttr,
                  VPSS_CHN_ATTR_S *pastVpssChnAttr)
{
    VPSS_CHN VpssChn = 0;
    CVI_S32 s32Ret;

    s32Ret = CVI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return s32Ret;
    }

    s32Ret = CVI_VPSS_ResetGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_ResetGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        goto exit1;
    }

    for (unsigned j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if (pabChnEnable[j]) {
            VpssChn = j;
            s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &pastVpssChnAttr[VpssChn]);
            if (s32Ret != CVI_SUCCESS) {
                CVI_LOGE("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                goto exit2;
            }

            s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
            if (s32Ret != CVI_SUCCESS) {
                CVI_LOGE("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
                goto exit2;
            }
        }
    }

    s32Ret = CVI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
        goto exit2;
    }
    return CVI_SUCCESS;

exit2:
    for(signed j = 0; j < VpssChn; j++){
        if (CVI_VPSS_DisableChn(VpssGrp, j) != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_DisableChn failed!\n");
        }
    }
exit1:
    if (CVI_VPSS_DestroyGrp(VpssGrp) != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_DestroyGrp(grp:%d) failed!\n", VpssGrp);
    }

    return s32Ret;
}

CVI_VOID Vproc_Deinit(VPSS_GRP VpssGrp, CVI_BOOL *pabChnEnable)
{
    CVI_S32 j;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VPSS_CHN VpssChn;

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if (pabChnEnable[j]) {
            VpssChn = j;
            s32Ret = CVI_VPSS_DisableChn(VpssGrp, VpssChn);
            if (s32Ret != CVI_SUCCESS) {
                CVI_LOGE("failed with %#x!\n", s32Ret);
            }
        }
    }
    s32Ret = CVI_VPSS_StopGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("failed with %#x!\n", s32Ret);
    }
    s32Ret = CVI_VPSS_DestroyGrp(VpssGrp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("failed with %#x!\n", s32Ret);
    }
}

int find_valid_vpss_grp(bool ExtChnEn)
{
    int i = 0;
    int grp_id = -1;

    pthread_mutex_lock(&vproc_mutex);
    // find a valid grp slot
    VI_VPSS_MODE_S stVIVPSSMode;
    CVI_SYS_GetVIVPSSMode(&stVIVPSSMode);
    if ((stVIVPSSMode.aenMode[0] == VI_OFFLINE_VPSS_ONLINE) ||
        (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_ONLINE) ||
        (ExtChnEn == true))
        i = 2;

    for (; i < MAX_VPSS_GRP_NUM; i++) {
        if (!g_grp_used[i]) {
            break;
        }
    }
    if (i >= MAX_VPSS_GRP_NUM) {
        CVI_LOGE("no empty grp_id left\n");
        pthread_mutex_unlock(&vproc_mutex);
        return grp_id;
    } else {
        grp_id = i;
    }

    // g_grp_used[grp_id] = true;
    pthread_mutex_unlock(&vproc_mutex);

    return grp_id;
}

int CVI_MAPI_VPROC_Init(CVI_MAPI_VPROC_HANDLE_T *vproc_hdl,
        int grp_id, CVI_MAPI_VPROC_ATTR_T *attr)
{
    VPROC_CHECK_NULL_PTR(attr);
    VPROC_CHECK_NULL_PTR(vproc_hdl);

    if(attr->chn_num > CVI_MAPI_VPROC_MAX_CHN_NUM){
        CVI_LOGE("attr->chn_num = %d, Exceed the maximum %d\n", attr->chn_num, CVI_MAPI_VPROC_MAX_CHN_NUM);
        return CVI_MAPI_ERR_INVALID;
    }

    if (grp_id == -1)
        grp_id = find_valid_vpss_grp(false);

    if (grp_id >= 0 && grp_id < MAX_VPSS_GRP_NUM) {
        pthread_mutex_lock(&vproc_mutex);
        if (g_grp_used[grp_id]) {
            CVI_LOGE("grp_id %d has been used\n", grp_id);
            pthread_mutex_unlock(&vproc_mutex);
            return CVI_MAPI_ERR_INVALID;
        }
        g_grp_used[grp_id] = true;
        pthread_mutex_unlock(&vproc_mutex);
    } else {
        CVI_LOGE("Invalid grp_id %d\n", grp_id);
        return CVI_MAPI_ERR_INVALID;
    }

    CVI_LOGI("Create VPROC with vpss grp id %d chn_num:%d\n", grp_id,attr->chn_num);

    VPSS_GRP VpssGrp = grp_id;
    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    for (int i = 0; i < attr->chn_num; i++) {
        abChnEnable[i] = CVI_TRUE;
    }

    CVI_S32 rc = CVI_MAPI_SUCCESS;
    /*start vpss*/
    rc = Vproc_Init(VpssGrp, abChnEnable, &attr->attr_inp, attr->attr_chn);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("Vproc_Init failed. rc: 0x%x !\n", rc);
        rc = CVI_MAPI_ERR_FAILURE;
        goto err1;
    }

    CVI_MAPI_VPROC_CTX_T *pt;
    pt = (CVI_MAPI_VPROC_CTX_T *)malloc(sizeof(CVI_MAPI_VPROC_CTX_T));
    if (!pt) {
        CVI_LOGE("malloc failed\n");
        rc = CVI_MAPI_ERR_NOMEM;
        goto err2;
    }
    memset(pt, 0, sizeof(CVI_MAPI_VPROC_CTX_T));
    pt->VpssGrp = VpssGrp;
    for (int i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        pt->abChnEnable[i] = abChnEnable[i];
    }
    pt->attr = *attr;

    *vproc_hdl = (CVI_MAPI_VPROC_HANDLE_T)pt;
    return CVI_MAPI_SUCCESS;

err2:
    Vproc_Deinit(VpssGrp, abChnEnable);

err1:
    pthread_mutex_lock(&vproc_mutex);
    g_grp_used[grp_id] = false;
    pthread_mutex_unlock(&vproc_mutex);

    return rc;
}

int CVI_MAPI_VPROC_Deinit(CVI_MAPI_VPROC_HANDLE_T vproc_hdl)
{
    VPROC_CHECK_NULL_PTR(vproc_hdl);
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    CHECK_VPROC_GRP(pt->VpssGrp);
    int grp_id = pt->VpssGrp;

    pthread_mutex_lock(&vproc_mutex);
    g_grp_used[grp_id] = false;
    pthread_mutex_unlock(&vproc_mutex);

    CVI_LOGI("Destroy VPROC with vpss grp id %d\n", grp_id);
    int i = 0;
    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (pt->ExtChn[i].ExtChnEnable == true) {
            MMF_CHN_S stSrcChn;
            MMF_CHN_S stDestChn;

            stSrcChn.enModId = CVI_ID_VPSS;
            stSrcChn.s32DevId = pt->VpssGrp;
            stSrcChn.s32ChnId = pt->ExtChn[i].ExtChnAttr.BindVprocChnId;

            stDestChn.enModId = CVI_ID_VPSS;
            stDestChn.s32DevId = pt->ExtChn[i].ExtChnGrp;
            stDestChn.s32ChnId = 0;

            CVI_S32 rc = CVI_SYS_UnBind(&stSrcChn, &stDestChn);
            if (rc != CVI_SUCCESS) {
                CVI_LOGE("CVI_SYS_UnBind, rc: 0x%x !\n", rc);
                return CVI_MAPI_ERR_FAILURE;
            }

        }
    }

    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (pt->ExtChn[i].ExtChnEnable == true) {
            CVI_BOOL ChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
            ChnEnable[0] = true;
            g_grp_used[pt->ExtChn[i].ExtChnGrp] = false;
            Vproc_Deinit(pt->ExtChn[i].ExtChnGrp, ChnEnable);
        }
    }

    Vproc_Deinit(pt->VpssGrp, pt->abChnEnable);
    free(pt);

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_VPROC_GetGrp(CVI_MAPI_VPROC_HANDLE_T vproc_hdl) {
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    return pt->VpssGrp;
}


int CVI_MAPI_VPROC_SendFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
                             VIDEO_FRAME_INFO_S *frame) {
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    CHECK_VPROC_GRP(pt->VpssGrp);
    VPROC_CHECK_NULL_PTR(frame);

    if (CVI_VPSS_SendFrame(pt->VpssGrp, frame, -1) != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_SendFrame failed\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_VPROC_GetChnFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
                               uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame) {
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    CHECK_VPROC_GRP(pt->VpssGrp);
    CHECK_VPROC_CHN(chn_idx);
    VPROC_CHECK_NULL_PTR(frame);
    if (!g_grp_used[pt->VpssGrp]) {
        CVI_LOGE("vproc grp %d uninitialized\n", pt->VpssGrp);
        return CVI_MAPI_ERR_FAILURE;
    }

    if (CVI_VPSS_GetChnFrame(pt->VpssGrp, chn_idx, frame, -1) != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_GetChnFrame failed\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_VPROC_SendChnFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
                                uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame) {
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    CHECK_VPROC_GRP(pt->VpssGrp);
    CHECK_VPROC_CHN(chn_idx);
    VPROC_CHECK_NULL_PTR(frame);
    if (!g_grp_used[pt->VpssGrp]) {
        CVI_LOGE("vproc grp %d uninitialized\n", pt->VpssGrp);
        return CVI_MAPI_ERR_FAILURE;
    }

    if (CVI_VPSS_SendChnFrame(pt->VpssGrp, chn_idx, frame, -1) != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_SendChnFrame failed\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_VPROC_ReleaseFrame(CVI_MAPI_VPROC_HANDLE_T vproc_hdl,
                                uint32_t chn_idx, VIDEO_FRAME_INFO_S *frame) {
    CVI_MAPI_VPROC_CTX_T *pt = (CVI_MAPI_VPROC_CTX_T *)vproc_hdl;
    CHECK_VPROC_GRP(pt->VpssGrp);
    CHECK_VPROC_CHN(chn_idx);
    VPROC_CHECK_NULL_PTR(frame);
    if (!g_grp_used[pt->VpssGrp]) {
        CVI_LOGE("vproc grp %d uninitialized\n", pt->VpssGrp);
        return CVI_MAPI_ERR_FAILURE;
    }

    if (CVI_VPSS_ReleaseChnFrame(pt->VpssGrp, chn_idx, frame) != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_SendFrame failed\n");
        return CVI_MAPI_ERR_FAILURE;
    }

    return CVI_MAPI_SUCCESS;
}

///
/// IPROC
///

static pthread_mutex_t iproc_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool iproc_initialized      = false;

int CVI_MAPI_IPROC_Resize(VIDEO_FRAME_INFO_S *frame_in,
                          VIDEO_FRAME_INFO_S *frame_out,
                          uint32_t resize_width,
                          uint32_t resize_height,
                          PIXEL_FORMAT_E fmt_out,
                          bool keep_aspect_ratio,
                          CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T *crop_in,
                          CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T *crop_out,
                          CVI_MAPI_PREPROCESS_ATTR_T *preprocess) {
    pthread_mutex_lock(&iproc_mutex);

    if (!iproc_initialized) {
        // always dual mode, grp 0 for vproc, grp 1 for iproc
        CVI_SYS_SetVPSSMode(VPSS_MODE_DUAL);
    }

    VPSS_GRP VpssGrp = iproc_grp;
    VPSS_CHN VpssChn = 0;
    CVI_U8 VpssDev   = 1;

    CVI_MAPI_IPROC_RESIZE_CROP_ATTR_T crop_out_adjusted;
    uint32_t resize_width_adjusted  = resize_width;
    uint32_t resize_height_adjusted = resize_height;
    if (crop_out) {
        uint32_t in_w = frame_in->stVFrame.u32Width;
        uint32_t in_h = frame_in->stVFrame.u32Height;
        if (crop_in) {
            in_w = crop_in->w;
            in_h = crop_in->h;
        }
        double x_scale         = 1.0 * in_w / resize_width;
        double y_scale         = 1.0 * in_h / resize_height;
        crop_out_adjusted.x    = crop_out->x * x_scale;
        crop_out_adjusted.y    = crop_out->y * y_scale;
        crop_out_adjusted.w    = crop_out->w * x_scale;
        crop_out_adjusted.h    = crop_out->h * y_scale;
        resize_width_adjusted  = crop_out->w;
        resize_height_adjusted = crop_out->h;
    }

    CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    abChnEnable[0]                             = CVI_TRUE;
    VPSS_GRP_ATTR_S attr_inp;
    VPSS_CHN_ATTR_S attr_chn[VPSS_MAX_PHY_CHN_NUM];
    memset((void *)&attr_inp, 0, sizeof(attr_inp));
    memset((void *)&attr_chn[0], 0, sizeof(attr_chn));

    attr_inp.stFrameRate.s32SrcFrameRate = -1;
    attr_inp.stFrameRate.s32DstFrameRate = -1;
    attr_inp.enPixelFormat               = frame_in->stVFrame.enPixelFormat;
    attr_inp.u32MaxW                     = frame_in->stVFrame.u32Width;
    attr_inp.u32MaxH                     = frame_in->stVFrame.u32Height;
    attr_inp.u8VpssDev                   = VpssDev;

    attr_chn[0].u32Width                    = resize_width_adjusted;
    attr_chn[0].u32Height                   = resize_height_adjusted;
    attr_chn[0].enVideoFormat               = VIDEO_FORMAT_LINEAR;
    attr_chn[0].enPixelFormat               = fmt_out;
    attr_chn[0].stFrameRate.s32SrcFrameRate = -1;
    attr_chn[0].stFrameRate.s32DstFrameRate = -1;
    attr_chn[0].u32Depth                    = 1;  // chn output queue size
    attr_chn[0].bMirror                     = CVI_FALSE;
    attr_chn[0].bFlip                       = CVI_FALSE;
    attr_chn[0].stAspectRatio.enMode        = keep_aspect_ratio ? ASPECT_RATIO_AUTO : ASPECT_RATIO_NONE;
    if (keep_aspect_ratio) {
        attr_chn[0].stAspectRatio.bEnableBgColor = CVI_TRUE;
        attr_chn[0].stAspectRatio.u32BgColor     = 0x00000000;
    }
    attr_chn[0].stNormalize.bEnable = CVI_FALSE;

    // preprocess
    if (preprocess) {
        CVI_MAPI_PREPROCESS_ENABLE(&attr_chn[0], preprocess);
    }

    CVI_S32 rc;
    /*start vpss*/
    if (!iproc_initialized) {
        rc = Vproc_Init(VpssGrp, abChnEnable, &attr_inp, attr_chn);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("Vproc_Init failed. rc: 0x%x !\n", rc);
            goto err;
        }
    } else {
        rc = CVI_VPSS_SetGrpAttr(VpssGrp, &attr_inp);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetGrpAttr failed. rc: 0x%x !\n", rc);
            goto err;
        }
        for (int i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
            if (abChnEnable[i]) {
                VpssChn = i;
                rc      = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &attr_chn[VpssChn]);
                if (rc != CVI_SUCCESS) {
                    CVI_LOGE("CVI_VPSS_SetChnAttr failed. rc: 0x%x !\n", rc);
                    goto err;
                }
            }
        }
    }
    if (!iproc_initialized) {
        iproc_initialized = true;
    }

    if (crop_in && crop_out) {
        VPSS_CROP_INFO_S stCropInInfo;
        stCropInInfo.bEnable = CVI_TRUE;
        stCropInInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        stCropInInfo.stCropRect.s32X      = crop_in->x;
        stCropInInfo.stCropRect.s32Y      = crop_in->y;
        stCropInInfo.stCropRect.u32Width  = crop_in->w;
        stCropInInfo.stCropRect.u32Height = crop_in->h;
        CVI_LOGV("Crop IN, %d %d %d %d\n",
                stCropInInfo.stCropRect.s32X,
                stCropInInfo.stCropRect.s32Y,
                stCropInInfo.stCropRect.u32Width,
                stCropInInfo.stCropRect.u32Height);
        rc = CVI_VPSS_SetGrpCrop(VpssGrp, &stCropInInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetGrpCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }

        VPSS_CROP_INFO_S stCropOutInfo;
        stCropOutInfo.bEnable = CVI_TRUE;
        stCropOutInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        stCropOutInfo.stCropRect.s32X      = crop_out_adjusted.x;
        stCropOutInfo.stCropRect.s32Y      = crop_out_adjusted.y;
        stCropOutInfo.stCropRect.u32Width  = crop_out_adjusted.w;
        stCropOutInfo.stCropRect.u32Height = crop_out_adjusted.h;
        CVI_LOGV("Crop OUT, %d %d %d %d\n",
                stCropOutInfo.stCropRect.s32X,
                stCropOutInfo.stCropRect.s32Y,
                stCropOutInfo.stCropRect.u32Width,
                stCropOutInfo.stCropRect.u32Height);
        rc = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stCropOutInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetChnCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }
    } else if (crop_in) {
        VPSS_CROP_INFO_S stCropInInfo;
        stCropInInfo.bEnable = CVI_TRUE;
        stCropInInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        stCropInInfo.stCropRect.s32X      = crop_in->x;
        stCropInInfo.stCropRect.s32Y      = crop_in->y;
        stCropInInfo.stCropRect.u32Width  = crop_in->w;
        stCropInInfo.stCropRect.u32Height = crop_in->h;
        CVI_LOGV("Crop IN, %d %d %d %d\n",
                stCropInInfo.stCropRect.s32X,
                stCropInInfo.stCropRect.s32Y,
                stCropInInfo.stCropRect.u32Width,
                stCropInInfo.stCropRect.u32Height);
        rc = CVI_VPSS_SetGrpCrop(VpssGrp, &stCropInInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetGrpCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }

        VPSS_CROP_INFO_S stCropDisableInfo;
        stCropDisableInfo.bEnable = CVI_FALSE;
        rc = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stCropDisableInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetChnCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }

    } else if (crop_out) {
        VPSS_CROP_INFO_S stCropDisableInfo;
        stCropDisableInfo.bEnable = CVI_FALSE;
        rc = CVI_VPSS_SetGrpCrop(VpssGrp, &stCropDisableInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetGrpCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }

        VPSS_CROP_INFO_S stCropOutInfo;
        stCropOutInfo.bEnable = CVI_TRUE;
        stCropOutInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        stCropOutInfo.stCropRect.s32X      = crop_out_adjusted.x;
        stCropOutInfo.stCropRect.s32Y      = crop_out_adjusted.y;
        stCropOutInfo.stCropRect.u32Width  = crop_out_adjusted.w;
        stCropOutInfo.stCropRect.u32Height = crop_out_adjusted.h;
        CVI_LOGV("Crop OUT, %d %d %d %d\n",
                stCropOutInfo.stCropRect.s32X,
                stCropOutInfo.stCropRect.s32Y,
                stCropOutInfo.stCropRect.u32Width,
                stCropOutInfo.stCropRect.u32Height);
        rc = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stCropOutInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetChnCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }

    } else {
        VPSS_CROP_INFO_S stCropDisableInfo;
        stCropDisableInfo.bEnable = CVI_FALSE;
        rc = CVI_VPSS_SetGrpCrop(VpssGrp, &stCropDisableInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetGrpCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }
        rc = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stCropDisableInfo);
        if (rc != CVI_SUCCESS) {
            CVI_LOGE("CVI_VPSS_SetChnCrop failed. rc: 0x%x !\n", rc);
            goto err;
        }
    }

    rc = CVI_VPSS_SendFrame(VpssGrp, frame_in, -1);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_SendFrame failed. rc: 0x%x !\n", rc);
        goto err;
    }

    rc = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, frame_out, -1);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_GetChnFrame failed with %#x\n", rc);
        goto err;
    }

    pthread_mutex_unlock(&iproc_mutex);
    return CVI_MAPI_SUCCESS;
err:
    pthread_mutex_unlock(&iproc_mutex);
    return CVI_MAPI_ERR_FAILURE;

}

int CVI_MAPI_IPROC_ReleaseFrame(VIDEO_FRAME_INFO_S *frm) {
    // IPROC always use grp 0
    VPSS_GRP VpssGrp = iproc_grp;
    VPSS_CHN VpssChn = 0;

    CVI_S32 rc;
    rc = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, frm);
    if (rc != CVI_SUCCESS) {
        CVI_LOGE("CVI_VPSS_ReleaseChnFrame failed with %#x\n", rc);
        return CVI_MAPI_ERR_FAILURE;
    }

    return CVI_MAPI_SUCCESS;
}

int CVI_MAPI_IPROC_Deinit()
{
    CVI_LOGI("Destroy IPROC with vpss grp id %d\n", iproc_grp);
    int i = 0;

    CVI_S32 j;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VPSS_CHN VpssChn;

    s32Ret = CVI_VPSS_DisableChn(iproc_grp, 0);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("failed with %#x!\n", s32Ret);
    }
    s32Ret = CVI_VPSS_StopGrp(iproc_grp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("failed with %#x!\n", s32Ret);
    }
    s32Ret = CVI_VPSS_DestroyGrp(iproc_grp);
    if (s32Ret != CVI_SUCCESS) {
        CVI_LOGE("failed with %#x!\n", s32Ret);
    }
    return CVI_MAPI_SUCCESS;
}
