# Appendix 1 Vpss Helper

``vpss_helper.h`` header is a indepentant header which only relies on Middleware SDK. The header provides some common coding pattern templates used in TDL SDK.

## Init Middleware

Currently Middleware can only be init once in the system. If you init after other threads for processes, you'll get message "Init failed.". The following functions are init helper functions.

### MMF_INIT_HELPER

The brainless init helper. Just tell the function the input, output resolution and you're way to go. The default image stacks for input and output images are both 12.

```c
static inline int __attribute__((always_inline))
MMF_INIT_HELPER(uint32_t enSrcWidth, uint32_t enSrcHeight, PIXEL_FORMAT_E enSrcFormat,
                uint32_t enDstWidth, uint32_t enDstHeight, PIXEL_FORMAT_E enDstFormat);
```

### MMF_INIT_HELPER2

A functions similar to ``MMF_INIT_HELPER`` excepts the user can set the number of the image stacks for both input and output.

```c
MMF_INIT_HELPER2(uint32_t enSrcWidth, uint32_t enSrcHeight, PIXEL_FORMAT_E enSrcFormat,
                 const uint32_t inBlkCount, uint32_t enDstWidth, uint32_t enDstHeight,
                 PIXEL_FORMAT_E enDstFormat, const uint32_t outBlkCount);
```

### MMF_INIT

The helper function only helps you register the ``VB_CONFIG_S`` to Middleware SDK and initialize the system.

```c
static inline int __attribute__((always_inline)) MMF_INIT(const VB_CONFIG_S *stVbConf);
```

### Setting Memory Pool

``VB_CONFIG_S`` stores the information of how many pools and how much size of memory should Middleware create when initializing. The unit of ``u32BlkSize`` is byte. Users can create their own configurations according to the scenario.

```c
  COMPRESS_MODE_E enCompressMode = COMPRESS_MODE_NONE;
  // Init SYS and Common VB,
  // Running w/ Vi don't need to do it again. Running Vpss along need init below
  // FIXME: Can only be init once in one pipeline
  VB_CONFIG_S stVbConf;
  memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
  stVbConf.u32MaxPoolCnt = 2;
  CVI_U32 u32BlkSize;
  u32BlkSize = COMMON_GetPicBufferSize(enSrcWidth, enSrcHeight, enSrcFormat, DATA_BITWIDTH_8,
                                       enCompressMode, DEFAULT_ALIGN);
  stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[0].u32BlkCnt = inBlkCount;
  u32BlkSize = COMMON_GetPicBufferSize(enDstWidth, enDstHeight, enDstFormat, DATA_BITWIDTH_8,
                                       enCompressMode, DEFAULT_ALIGN);
  stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[1].u32BlkCnt = outBlkCount;
```

## Init VPSS

This function helps users to initialize the VPSS engine. All the settings for input/ output channel can be change later.

```c
inline int __attribute__((always_inline))
VPSS_INIT_HELPER(CVI_U32 vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
                 PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
                 PIXEL_FORMAT_E enDstFormat, VPSS_MODE_E mode, bool keepAspectRatio);

inline int __attribute__((always_inline))
VPSS_INIT_HELPER2(CVI_U32 vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
                  PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
                  PIXEL_FORMAT_E enDstFormat, uint32_t enabledChannel, bool keepAspectRatio);
```

## Setup VPSS Channel Config

These functions are frequently used inside TDL SDK. Any operation such as resize, and scale with quantization is done by setting the ``VPSS_CHN_ATTR_S`` parameter.

### Resize

We provide two different resize helper, First one is keep aspect ratio with auto mode or don't keep aspect ratio. The second one let users to set the ratio manually.

```c
inline void __attribute__((always_inline))
VPSS_CHN_DEFAULT_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, CVI_U32 dstWidth, CVI_U32 dstHeight,
                        PIXEL_FORMAT_E enDstFormat, CVI_BOOL keepAspectRatio);

inline void __attribute__((always_inline))
VPSS_CHN_RATIO_MANUAL_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, CVI_U32 dstWidth, CVI_U32 dstHeight,
                             PIXEL_FORMAT_E enDstFormat, CVI_U32 ratioX, CVI_U32 ratioY,
                             CVI_U32 ratioWidth, CVI_U32 ratioHeight);
```

### Scale (w/o Quantization)

Due to he hardware limitation of cv1835, the mean is subtracted after the image is padded. If you want to kept the padded area 0, set ``padReverse`` to true. The array length of ``factor`` and ``mean`` must equal to 3.

```c
inline void __attribute__((always_inline))
VPSS_CHN_SQ_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, const CVI_U32 dstWidth,
                   const CVI_U32 dstHeight, const PIXEL_FORMAT_E enDstFormat,
                   const CVI_FLOAT *factor, const CVI_FLOAT *mean, const bool padReverse);

inline void __attribute__((always_inline))
VPSS_CHN_SQ_RB_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, const CVI_U32 srcWidth,
                      const CVI_U32 srcHeight, const CVI_U32 dstWidth, const CVI_U32 dstHeight,
                      const PIXEL_FORMAT_E enDstFormat, const CVI_FLOAT *factor,
                      const CVI_FLOAT *mean, const bool padReverse);
```

## Create frame from BLK

This function creates a ``VIDEO_FRAME_INFO_S`` from suitable memory pool in Middleware SDK. The matching id is stored in ``VB_BLK``. Make sure to call ``CVI_VB_ReleaseBlock`` to avoid memory leak.

```c
inline int __attribute__((always_inline))
CREATE_VBFRAME_HELPER(VB_BLK *blk, VIDEO_FRAME_INFO_S *vbFrame, CVI_U32 srcWidth, CVI_U32 srcHeight,
                      PIXEL_FORMAT_E pixelFormat);
```