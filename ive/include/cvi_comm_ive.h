#ifndef _CVI_COMM_IVE_H_
#define _CVI_COMM_IVE_H_
#ifdef CV180X
#include "linux/cvi_type.h"
#else
#include "cvi_type.h"
#endif

#define CVI_IVE2_LENGTH_ALIGN 1

typedef void *IVE_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif
typedef struct CVI_IMG CVI_IMG_S;
#ifdef __cplusplus
}
#endif

typedef enum IVE_DMA_MODE {
  IVE_DMA_MODE_DIRECT_COPY = 0x0,
  IVE_DMA_MODE_INTERVAL_COPY = 0x1,
  IVE_DMA_MODE_SET_3BYTE = 0x2,
  IVE_DMA_MODE_SET_8BYTE = 0x3,
  IVE_DMA_MODE_BUTT
} IVE_DMA_MODE_E;

typedef struct IVE_DMA_CTRL {
  IVE_DMA_MODE_E enMode;
  CVI_U64 u64Val;
  CVI_U8
  u8HorSegSize;
  CVI_U8 u8ElemSize;
  CVI_U8 u8VerSegRows;
} IVE_DMA_CTRL_S;

typedef struct IVE_MEM_INFO {
  CVI_U32 u32PhyAddr;
  CVI_U8 *pu8VirAddr;
  CVI_U32 u32ByteSize;
} IVE_MEM_INFO_S;

typedef IVE_MEM_INFO_S IVE_SRC_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef struct IVE_DATA {
  CVI_U32 u32PhyAddr;
  CVI_U8 *pu8VirAddr;

  CVI_U16 u16Stride;
  CVI_U16 u16Width;
  CVI_U16 u16Height;

  CVI_U16 u16Reserved;
  CVI_IMG_S *tpu_block;
} IVE_DATA_S;

typedef IVE_DATA_S IVE_SRC_DATA_S;
typedef IVE_DATA_S IVE_DST_DATA_S;

typedef enum IVE_IMAGE_TYPE {
  IVE_IMAGE_TYPE_U8C1 = 0x0,
  IVE_IMAGE_TYPE_S8C1 = 0x1,

  IVE_IMAGE_TYPE_YUV420SP = 0x2,
  IVE_IMAGE_TYPE_YUV422SP = 0x3,
  IVE_IMAGE_TYPE_YUV420P = 0x4,
  IVE_IMAGE_TYPE_YUV422P = 0x5,

  IVE_IMAGE_TYPE_S8C2_PACKAGE = 0x6,
  IVE_IMAGE_TYPE_S8C2_PLANAR = 0x7,

  IVE_IMAGE_TYPE_S16C1 = 0x8,
  IVE_IMAGE_TYPE_U16C1 = 0x9,

  IVE_IMAGE_TYPE_U8C3_PACKAGE = 0xa,
  IVE_IMAGE_TYPE_U8C3_PLANAR = 0xb,

  IVE_IMAGE_TYPE_S32C1 = 0xc,
  IVE_IMAGE_TYPE_U32C1 = 0xd,

  IVE_IMAGE_TYPE_S64C1 = 0xe,
  IVE_IMAGE_TYPE_U64C1 = 0xf,

  IVE_IMAGE_TYPE_BF16C1 = 0x10,
  IVE_IMAGE_TYPE_FP32C1 = 0x11,

  IVE_IMAGE_TYPE_S8C3_PACKAGE,
  IVE_IMAGE_TYPE_S8C3_PLANAR,

  IVE_IMAGE_TYPE_BUTT

} IVE_IMAGE_TYPE_E;

typedef struct IVE_IMAGE {
  IVE_IMAGE_TYPE_E enType;

  CVI_U64 u64PhyAddr[3];
  CVI_U8 *pu8VirAddr[3];

  CVI_U16 u16Stride[3];
  CVI_U16 u16Width;
  CVI_U16 u16Height;

  CVI_U16 u16Reserved;
  CVI_IMG_S *tpu_block;
} IVE_IMAGE_S;

typedef IVE_IMAGE_S IVE_SRC_IMAGE_S;
typedef IVE_IMAGE_S IVE_DST_IMAGE_S;

typedef struct IVE_CONVERT_SCALE_ABS_CRTL {
  float alpha;
  float beta;
} IVE_CONVERT_SCALE_ABS_CRTL_S;

typedef enum IVE_ITC_TYPE {
  IVE_ITC_SATURATE = 0x0,
  IVE_ITC_NORMALIZE = 0x1,
} IVE_ITC_TYPE_E;

typedef struct IVE_ITC_CRTL {
  IVE_ITC_TYPE_E enType;
} IVE_ITC_CRTL_S;

typedef struct IVE_ADD_CTRL_S {
  float aX;
  float bY;
} IVE_ADD_CTRL_S;

typedef struct IVE_BLOCK_CTRL {
  CVI_FLOAT f32ScaleSize;
  CVI_U32 u32CellSize;
} IVE_BLOCK_CTRL_S;

typedef struct IVE_BLEND_CTRL_S {
  CVI_U8 u8Weight;
} IVE_BLEND_CTRL_S;

typedef struct IVE_ELEMENT_STRUCTURE_CTRL {
  CVI_U8 au8Mask[25];
} IVE_ELEMENT_STRUCTURE_CTRL_S;

typedef IVE_ELEMENT_STRUCTURE_CTRL_S IVE_DILATE_CTRL_S;
typedef IVE_ELEMENT_STRUCTURE_CTRL_S IVE_ERODE_CTRL_S;

typedef struct IVE_FILTER_CTRL {
  CVI_U8 u8MaskSize;
  CVI_S8 as8Mask[169];
  CVI_U32 u32Norm;
} IVE_FILTER_CTRL_S;

typedef struct IVE_DOWNSAMPLE_CTRL {
  CVI_U8 u8KnerlSize;
} IVE_DOWNSAMPLE_CTRL_S;

typedef struct IVE_HOG_CTRL {
  CVI_U8 u8BinSize;
  CVI_U32 u32CellSize;
  CVI_U16 u16BlkSizeInCell;
  CVI_U16 u16BlkStepX;
  CVI_U16 u16BlkStepY;
} IVE_HOG_CTRL_S;

typedef enum IVE_MAG_AND_ANG_OUT_CTRL {
  IVE_MAG_AND_ANG_OUT_CTRL_MAG = 0x0,
  IVE_MAG_AND_ANG_OUT_CTRL_ANG = 0x1,
  IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG = 0x2,
  IVE_MAG_AND_ANG_OUT_CTRL_BUTT
} IVE_MAG_AND_ANG_OUT_CTRL_E;

typedef enum IVE_MAG_DIST {
  IVE_MAG_DIST_L1 = 0x0,
  IVE_MAG_DIST_L2 = 0x1,
  IVE_MAG_DIST_BUTT
} IVE_MAG_DIST_E;

/*
 *Magnitude and angle control parameter
 */
typedef struct IVE_MAG_AND_ANG_CTRL {
  IVE_MAG_AND_ANG_OUT_CTRL_E enOutCtrl;
  IVE_MAG_DIST_E enDistCtrl;
} IVE_MAG_AND_ANG_CTRL_S;

typedef enum IVE_NORM_GRAD_OUT_CTRL {
  IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER = 0x0,
  IVE_NORM_GRAD_OUT_CTRL_HOR = 0x1,
  IVE_NORM_GRAD_OUT_CTRL_VER = 0x2,
  IVE_NORM_GRAD_OUT_CTRL_COMBINE = 0x3,

  IVE_NORM_GRAD_OUT_CTRL_BUTT
} IVE_NORM_GRAD_OUT_CTRL_E;

typedef struct IVE_NORM_GRAD_CTRL {
  IVE_NORM_GRAD_OUT_CTRL_E enOutCtrl;
  IVE_MAG_DIST_E enDistCtrl;
  IVE_ITC_TYPE_E enITCType;
  CVI_U8 u8MaskSize;
} IVE_NORM_GRAD_CTRL_S;

typedef enum IVE_ORD_STAT_FILTER_MODE {
  IVE_ORD_STAT_FILTER_MODE_MAX = 0x0,
  IVE_ORD_STAT_FILTER_MODE_MIN = 0x1,

  IVE_ORD_STAT_FILTER_MODE_BUTT
} IVE_ORD_STAT_FILTER_MODE_E;

typedef struct IVE_ORD_STAT_FILTER_CTRL {
  IVE_ORD_STAT_FILTER_MODE_E enMode;
} IVE_ORD_STAT_FILTER_CTRL_S;

/*
 * Sad mode
 */
typedef enum IVE_SAD_MODE {
  IVE_SAD_MODE_MB_4X4 = 0x0,
  IVE_SAD_MODE_MB_8X8 = 0x1,
  IVE_SAD_MODE_MB_16X16 = 0x2,

  IVE_SAD_MODE_BUTT
} IVE_SAD_MODE_E;
/*
 *Sad output ctrl
 */
typedef enum IVE_SAD_OUT_CTRL {
  IVE_SAD_OUT_CTRL_16BIT_BOTH = 0x0,
  IVE_SAD_OUT_CTRL_8BIT_BOTH = 0x1,
  IVE_SAD_OUT_CTRL_16BIT_SAD = 0x2,
  IVE_SAD_OUT_CTRL_8BIT_SAD = 0x3,
  IVE_SAD_OUT_CTRL_THRESH = 0x4,

  IVE_SAD_OUT_CTRL_BUTT
} IVE_SAD_OUT_CTRL_E;
/*
 * Sad ctrl param
 */
typedef struct IVE_SAD_CTRL {
  IVE_SAD_MODE_E enMode;
  IVE_SAD_OUT_CTRL_E enOutCtrl;
  CVI_U16 u16Thr;
  CVI_U8 u8MinVal;
  CVI_U8 u8MaxVal;
} IVE_SAD_CTRL_S;

typedef enum IVE_SOBEL_OUT_CTRL {
  IVE_SOBEL_OUT_CTRL_BOTH = 0x0,
  IVE_SOBEL_OUT_CTRL_HOR = 0x1,
  IVE_SOBEL_OUT_CTRL_VER = 0x2,
  IVE_SOBEL_OUT_CTRL_BUTT
} IVE_SOBEL_OUT_CTRL_E;

typedef struct IVE_SOBEL_CTRL {
  IVE_SOBEL_OUT_CTRL_E enOutCtrl;
  CVI_U8 u8MaskSize;
  CVI_S8 as8Mask[25];
} IVE_SOBEL_CTRL_S;

typedef enum IVE_SUB_MODE_E {
  IVE_SUB_MODE_NORMAL = 0x0,
  IVE_SUB_MODE_ABS = 0x1,
  IVE_SUB_MODE_SHIFT = 0x2,
  IVE_SUB_MODE_ABS_THRESH = 0x3,
  IVE_SUB_MODE_ABS_CLIP,
  IVE_SUB_MODE_BUTT
} IVE_SUB_MODE_E;

typedef struct IVE_SUB_CTRL {
  IVE_SUB_MODE_E enMode;
} IVE_SUB_CTRL_S;

typedef enum IVE_THRESH_MODE {
  IVE_THRESH_MODE_BINARY,
  IVE_THRESH_MODE_SLOPE,
  IVE_THRESH_MODE_BUTT
} IVE_THRESH_MODE_E;

typedef struct IVE_THRESH_CTRL {
  CVI_U32 enMode;
  CVI_U8 u8MinVal;
  CVI_U8 u8MaxVal;
  CVI_U8 u8LowThr;
} IVE_THRESH_CTRL_S;

typedef enum hiIVE_THRESH_S16_MODE_E {
  IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX = 0x0,
  IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX = 0x1,
  IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX = 0x2,
  IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX = 0x3,

  IVE_THRESH_S16_MODE_BUTT
} IVE_THRESH_S16_MODE_E;

typedef union IVE_8BIT {
  CVI_S8 s8Val;
  CVI_U8 u8Val;
} IVE_8BIT_U;

typedef struct hiIVE_THRESH_S16_CTRL {
  IVE_THRESH_S16_MODE_E enMode;
  CVI_S16 s16LowThr;
  CVI_S16 s16HighThr;
  IVE_8BIT_U un8MinVal;
  IVE_8BIT_U un8MidVal;
  IVE_8BIT_U un8MaxVal;
} IVE_THRESH_S16_CTRL_S;

typedef enum IVE_THRESH_U16_MODE {
  IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX = 0x0,
  IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX = 0x1,

  IVE_THRESH_U16_MODE_BUTT
} IVE_THRESH_U16_MODE_E;

typedef struct IVE_THRESH_U16_CTRL {
  IVE_THRESH_U16_MODE_E enMode;
  CVI_U16 u16LowThr;
  CVI_U16 u16HighThr;
  CVI_U8 u8MinVal;
  CVI_U8 u8MidVal;
  CVI_U8 u8MaxVal;
} IVE_THRESH_U16_CTRL_S;

// for cpu version

typedef enum IVE_CC_DIR { DIRECTION_4 = 0x0, DIRECTION_8 = 0x1 } IVE_CC_DIR_E;

typedef struct IVE_CC_CTRL {
  IVE_CC_DIR_E enMode;
} IVE_CC_CTRL_S;

// integral image
typedef enum cviIVE_INTEG_OUT_CTRL_E {
  IVE_INTEG_OUT_CTRL_COMBINE = 0x0,
  IVE_INTEG_OUT_CTRL_SUM = 0x1,
  IVE_INTEG_OUT_CTRL_SQSUM = 0x2,
  IVE_INTEG_OUT_CTRL_BUTT
} IVE_INTEG_OUT_CTRL_E;

typedef struct cviIVE_INTEG_CTRL_S {
  IVE_INTEG_OUT_CTRL_E enOutCtrl;
} IVE_INTEG_CTRL_S;

typedef struct cviIVE_EQUALIZE_HIST_CTRL_S {
  IVE_MEM_INFO_S stMem;
} IVE_EQUALIZE_HIST_CTRL_S;

typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef enum cviIVE_16BIT_TO_8BIT_MODE_E {
  IVE_16BIT_TO_8BIT_MODE_S16_TO_S8 = 0x0,
  IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS = 0x1,
  IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS = 0x2,
  IVE_16BIT_TO_8BIT_MODE_U16_TO_U8 = 0x3,
  IVE_16BIT_TO_8BIT_MODE_BUTT
} IVE_16BIT_TO_8BIT_MODE_E;

typedef struct cviIVE_16BIT_TO_8BIT_CTRL_S {
  IVE_16BIT_TO_8BIT_MODE_E enMode;
  CVI_U16 u16Denominator;
  CVI_U8 u8Numerator;
  CVI_S8 s8Bias;
} IVE_16BIT_TO_8BIT_CTRL_S;

typedef struct cviIVE_NCC_DST_MEM_S {
  CVI_U64 u64Numerator;
  CVI_U64 u64QuadSum1;
  CVI_U64 u64QuadSum2;
  CVI_U64 u8Reserved[8];
} IVE_NCC_DST_MEM_S;

typedef enum cviIVE_LBP_CMP_MODE_E {
  IVE_LBP_CMP_MODE_NORMAL = 0x0, /* P(x)-P(center)>= un8BitThr.s8Val, s(x)=1; else s(x)=0; */
  IVE_LBP_CMP_MODE_ABS = 0x1,    /* abs(P(x)- P(center))>=un8BitThr.u8Val, s(x)=1; else s(x)=0; */
  IVE_LBP_CMP_MODE_BUTT
} IVE_LBP_CMP_MODE_E;

typedef struct cviIVE_LBP_CTRL_S {
  IVE_LBP_CMP_MODE_E enMode;
  IVE_8BIT_U un8BitThr;
} IVE_LBP_CTRL_S;

// csc/resize

typedef enum cviIVE_CSC_MODE_E {
  /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
  // IVE_CSC_MODE_VIDEO_BT601_YUV2RGB = 0x0,
  /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
  // IVE_CSC_MODE_VIDEO_BT709_YUV2RGB = 0x1,
  /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT601_YUV2RGB = 0x2,
  /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT709_YUV2RGB = 0x3,
  /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT601_YUV2HSV = 0x4,
  /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT709_YUV2HSV = 0x5,
  /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT601_YUV2LAB = 0x6,
  /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
  // IVE_CSC_MODE_PIC_BT709_YUV2LAB = 0x7,
  /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
  // IVE_CSC_MODE_VIDEO_BT601_RGB2YUV = 0x8,
  /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
  // IVE_CSC_MODE_VIDEO_BT709_RGB2YUV = 0x9,
  /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
  // IVE_CSC_MODE_PIC_BT601_RGB2YUV = 0xa,
  /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
  // IVE_CSC_MODE_PIC_BT709_RGB2YUV = 0xb,

  IVE_CSC_MODE_PIC_RGB2HSV = 0xb,
  IVE_CSC_MODE_PIC_RGB2GRAY = 0xc,

  // IVE_CSC_MODE_BUTT
} IVE_CSC_MODE_E;

typedef struct cviIVE_CSC_CTRL_S {
  IVE_CSC_MODE_E enMode; /*Working mode*/
} IVE_CSC_CTRL_S;

typedef enum cviIVE_RESIZE_MODE_E {
  IVE_RESIZE_MODE_LINEAR = 0x0, /*Bilinear interpolation*/
  IVE_RESIZE_MODE_AREA = 0x1,   /*Area-based (or super) interpolation*/
  IVE_RESIZE_MODE_BUTT
} IVE_RESIZE_MODE_E;

typedef struct cviIVE_RESIZE_CTRL_S {
  IVE_RESIZE_MODE_E enMode;
  IVE_MEM_INFO_S stMem;
  CVI_U16 u16Num;
} IVE_RESIZE_CTRL_S;

typedef struct cviIVE_FILTER_AND_CSC_CTRL_S {
  IVE_CSC_MODE_E enMode; /*CSC working mode*/
  CVI_S8 as8Mask[25];    /*Template parameter filter coefficient*/
  CVI_U16 u16Norm;       /*Normalization parameter, by right shift*/
} IVE_FILTER_AND_CSC_CTRL_S;

// }
#endif  // End of _CVI_COMM_IVE.h
