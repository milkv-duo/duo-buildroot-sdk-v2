#ifndef _IVE_H
#define _IVE_H
#include "cvi_comm_ive.h"
#ifdef CV180X
#include "linux/cvi_comm_video.h"
#else
#include "cvi_comm_video.h"
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

inline CVI_U16 CVI_CalcStride(CVI_U16 width, CVI_U16 align)
{
    CVI_U16 stride = (CVI_U16)(width / align) * align;
    if (stride < width)
    {
        stride += align;
    }
    return stride;
}

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum _EN_IVE_ERR_CODE_E
    {
        ERR_IVE_SYS_TIMEOUT = 0x40,   /* IVE process timeout */
        ERR_IVE_QUERY_TIMEOUT = 0x41, /* IVE query timeout */
        ERR_IVE_OPEN_FILE = 0x42,     /* IVE open file error */
        ERR_IVE_READ_FILE = 0x43,     /* IVE read file error */
        ERR_IVE_WRITE_FILE = 0x44,    /* IVE write file error */

        ERR_IVE_BUTT
    } EN_IVE_ERR_CODE_E;

    /**
     * @brief Create an IVE instance handler.
     *
     * @return IVE_HANDLE An IVE instance handle.
     */
    IVE_HANDLE CVI_IVE_CreateHandle();

    /**
     * @brief Destroy an Ive instance handler.
     *
     * @param pIveHandle Ive instanace handler.
     * @return CVI_S32 Return CVI_SUCCESS if instance is successfully destroyed.
     */
    CVI_S32 CVI_IVE_DestroyHandle(IVE_HANDLE pIveHandle);

    /**
     * @brief Flush cache data to RAM. Call this after IVE_IMAGE_S VAddr operations.
     *
     * @param pIveHandle Ive instanace handler.
     * @param pstImg Image to be flushed.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeeded.
     */
    CVI_S32 CVI_IVE_BufFlush(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg);

    /**
     * @brief Update cache from RAM. Call this function before using data from VAddr \
     *        in CPU.
     *
     * @param pIveHandle Ive instanace handler.
     * @param pstImg Cache image to be updated.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeeded.
     */
    CVI_S32 CVI_IVE_BufRequest(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg);

    /**
     * @brief Create a IVE_MEM_INFO_S.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstMemInfo The input mem info structure.
     * @param u32ByteSize The size of the mem info in 1d (unit: byte).
     * @return CVI_S32
     */
    CVI_S32 CVI_IVE_CreateMemInfo(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMemInfo,
                                  CVI_U32 u32ByteSize);

    /**
     * @brief Create an IVE_IMAGE_S.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstImg The input image stucture.
     * @param enType The image type. e.g. IVE_IMAGE_TYPE_U8C1.
     * @param u16Width The image width.
     * @param u16Height The image height.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeed.
     */
    CVI_S32 CVI_IVE_CreateImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType,
                                CVI_U16 u16Width, CVI_U16 u16Height);

    /**
     * @brief Create an IVE_IMAGE_S with a given buffer from another IVE_IMAGE_S.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstImg The input image stucture.
     * @param enType The image type. e.g. IVE_IMAGE_TYPE_U8C1.
     * @param u16Width The image width.
     * @param u16Height The image height.
     * @param pstBuffer Use another IVE_IMAGE_S as buffer.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeed.
     */
    CVI_S32 CVI_IVE_CreateImage2(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType,
                                 CVI_U16 u16Width, CVI_U16 u16Height, IVE_IMAGE_S *pstBuffer);
    /**
     * @brief Get the sub image from an image with the given coordiantes. The data is shared without
     * copy.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc The input image.
     * @param pstDst The output sub image.
     * @param u16X1 The X1 coordinate from original image.
     * @param u16Y1 The Y1 coordinate from original image.
     * @param u16X2 The X2 coordinate from original image.
     * @param u16Y2 The Y2 coordinate from original image.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeed.
     */
    CVI_S32 CVI_IVE_SubImage(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                             CVI_U16 u16X1, CVI_U16 u16Y1, CVI_U16 u16X2, CVI_U16 u16Y2);

    /**
     * @brief Convert IVE_IMAGE_S to VIDEO_FRAME_INFO_S. The virtual address is available when
     * converted.
     *
     * @param pstIISrc IVE_IMAGE_S input.
     * @param pstVFIDst VIDEO_FRAME_INFO_S output.
     * @param invertPackage Give hint to the function to tell IVE_IMAGE_S stores RGB package in inverse
     * order.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeeded.
     */
    CVI_S32 CVI_IVE_Image2VideoFrameInfo(IVE_IMAGE_S *pstIISrc, VIDEO_FRAME_INFO_S *pstVFIDst,
                                         CVI_BOOL invertPackage);

    /**
     * @brief Convert VIDEO_FRAME_INFO_S to IVE_IMAGE_S. Note that this function does not map or unmap
     * for you. IVE also does not care about the order of RGB_888 or BGR_888 package images.
     *
     * @param pstVFISrc VIDEO_FRAME_INFO_S input.
     * @param pstIIDst IVE_IMAGE_S output.
     * @return CVI_S32 Return CVI_SUCCESS if operation succeeded.
     */
    CVI_S32 CVI_IVE_VideoFrameInfo2Image(VIDEO_FRAME_INFO_S *pstVFISrc, IVE_IMAGE_S *pstIIDst);

    /**
     * @brief Read an image from file system. Default is in the order of RGB.
     *
     * @param pIveHandle Ive instance handler.
     * @param filename File path to the image.
     * @param enType Type of the destination image.
     * @return IVE_IMAGE_S Return IVE_IMAGE_S
     */
    IVE_IMAGE_S CVI_IVE_ReadImage(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_TYPE_E enType);

    /**
     * @brief Read an image from file system. Default is in the order of RGB.
     *
     * @param pIveHandle Ive instance handler.
     * @param filename File path to the image.
     * @param enType Type of the destination image.
     * @param invertPackage Invert the order of RGB package image to BGR.
     * @return IVE_IMAGE_S Return IVE_IMAGE_S
     */
    IVE_IMAGE_S CVI_IVE_ReadImage2(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_TYPE_E enType,
                                   CVI_BOOL invertPackage);

    /**
     * @brief Read an image from file system. for yuv.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstImg pointer of src1.
     * @param filename File path to the image.
     * @param enType Type of the destination image.
     * @param u32Width Yuv w
     * @param u32Width Yuv h
     * @return IVE_IMAGE_S Return IVE_IMAGE_S
     */
    CVI_S32 CVI_IVE_ReadRawImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, const char *filename,
                                 IVE_IMAGE_TYPE_E enType, CVI_U16 u32Width, CVI_U16 u32Height);

    /**
     * @brief Read an image from file system. for yuv.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstImg pointer of src1.
     * @param pBuffer File save buffer.
     * @param enType Type of the destination image.
     * @param u32Width Yuv w
     * @param u32Width Yuv h
     * @return IVE_IMAGE_S Return IVE_IMAGE_S
     */
    CVI_S32 CVI_IVE_ReadImageArray(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, char *pBuffer,
                                   IVE_IMAGE_TYPE_E enType, CVI_U16 u32Width, CVI_U16 u32Height);

    /**
     * @brief Write an IVE_IMAGE_S to file system.
     *
     * @param pIveHandle Ive instance handler.
     * @param filename Save file path.
     * @param pstImg Input image.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_WriteImage(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_S *pstImg);

    /**
     * @brief Free Allocated IVE_MEM_INFO_S.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstMemInfo Allocated IVE_MEM_INFO_S.
     * @return CVI_S32 Return CVI_SUCCESS.
     */
    CVI_S32 CVI_SYS_FreeM(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMemInfo);

    /**
     * @brief Free Allocated IVE_IMAGE_S.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstMemInfo Allocated IVE_IMAGE_S.
     * @return CVI_S32 Return CVI_SUCCESS.
     */
    CVI_S32 CVI_SYS_FreeI(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg);

    /**
     * @brief Copy data between image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDst Output image.
     * @param pstDmaCtrl Dma control parameters.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_DMA(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                        IVE_DMA_CTRL_S *pstDmaCtrl, bool bInstant);

    /**
     * @brief Convert image to different image type.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDst Outpu image.
     * @param pstItcCtrl Convert parameters.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_ImageTypeConvert(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                     IVE_DST_IMAGE_S *pstDst, IVE_ITC_CRTL_S *pstItcCtrl,
                                     bool bInstant);

    /**
     * @brief Fill whole image with a single value.
     *
     * @param pIveHandle Ive instance handler.
     * @param value Fill value. Note if output is U8 value must between 0-255.
     * @param pstDst Output result.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_ConstFill(IVE_HANDLE pIveHandle, const CVI_FLOAT value, IVE_DST_IMAGE_S *pstDst,
                              bool bInstant);

    /**
     * @brief Scales, calculates absolute values, and converts the result to 8-bit:
     * dst = saturate_uint8(abs(src * alpha + beta)).
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image, should be BF16C1.
     * @param pstDst Output result, should be U8C1.
     * @param ctrl control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_ConvertScaleAbs(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                    IVE_DST_IMAGE_S *pstDst,
                                    IVE_CONVERT_SCALE_ABS_CRTL_S *pstConvertCtrl, bool bInstant);

    /**
     * @brief Add two image and output the result.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstDst Output result.
     * @param ctrl Add control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Add(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_IMAGE_S *pstDst, IVE_ADD_CTRL_S *ctrl, bool bInstant);

    /**
     * @brief AND two images and output the result.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstDst Output result.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_And(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_IMAGE_S *pstDst, bool bInstant);

    /**
     * @brief Calculate the average of the sliced cells of an image. The output image size will be \
     *        (input_w / u32CellSize, output_h / u32CellSize)
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDst Output result.
     * @param pstBlkCtrl Block control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_BLOCK(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                          IVE_BLOCK_CTRL_S *pstBlkCtrl, bool bInstant);

    /**
     * @brief Dilate a gray scale image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDst Outpu result.
     * @param pstDilateCtrl Dilate control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Dilate(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_DILATE_CTRL_S *pstDilateCtrl, bool bInstant);

    /**
     * @brief Erode a gray scale image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDst Output result.
     * @param pstErodeCtrl Erode control variable.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Erode(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                          IVE_ERODE_CTRL_S *pstErodeCtrl, bool bInstant);

    /**
     * @brief Alpha Blending for two images.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstSrc2 Input image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstDst Output result.
     * @param IVE_BLEND_CTRL_S blend control variable.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Blend(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                          IVE_DST_IMAGE_S *pstDst, IVE_BLEND_CTRL_S *pstBlendCtrl, bool bInstant);

    /**
     * @brief Pixel-wise alpha blending for two images.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstSrc2 Input image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstAlpha alpha image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstDst Output result.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Blend_Pixel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstAlpha,
                                IVE_DST_IMAGE_S *pstDst, bool bInstant);
    /**
     * @brief Pixel-wise alpha blending for two images.clip(s8_a*u8_w + s8_b*(255-u8_w),-128,127)
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image. Both S8C3_PLANAR and S8C1 format are accepted.
     * @param pstSrc2 Input image. Both S8C3_PLANAR and S8C1 format are accepted.
     * @param pstAlpha alpha image. Both U8C3_PLANAR and U8C1 format are accepted.
     * @param pstDst Output result.Both S8C3_PLANAR and S8C1 format are accepted.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Blend_Pixel_S8_CLIP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                        IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstAlpha,
                                        IVE_DST_IMAGE_S *pstDst);
    /**
     * @brief Pixel-wise alpha blending for two images.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image. U8C3_PLANAR,U8C1,YUV420P format are accepted.
     * @param pstSrc2 Input image. U8C3_PLANAR,U8C1,YUV420P format are accepted.
     * @param pstWa alpha image. U8C3_PLANAR,U8C1,YUV420P format are accepted.
     * @param pstWb alpha image. U8C3_PLANAR,U8C1,YUV420P format are accepted.
     * @param pstDst Output result.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Blend_Pixel_U8_AB(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                      IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstWa,
                                      IVE_SRC_IMAGE_S *pstWb, IVE_DST_IMAGE_S *pstDst);
    /**
     * @brief Apply a filter to an image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDst Output result.
     * @param pstFltCtrl Filter control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Filter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_FILTER_CTRL_S *pstFltCtrl, bool bInstant);

    /**
     * @brief Get size of the HOG histogram.
     *
     * @param u16Width Input image width.
     * @param u16Height Input image height.
     * @param u8BinSize Bin size.
     * @param u16CellSize Cell size.
     * @param u16BlkSizeInCell  Block size.
     * @param u16BlkStepX Block step in X dimension.
     * @param u16BlkStepY Block step in Y dimension.
     * @param u32HogSize Output HOG size (length * sizeof(uint32_t)).
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_GET_HOG_SIZE(CVI_U16 u16Width, CVI_U16 u16Height, CVI_U8 u8BinSize,
                                 CVI_U16 u16CellSize, CVI_U16 u16BlkSizeInCell, CVI_U16 u16BlkStepX,
                                 CVI_U16 u16BlkStepY, CVI_U32 *u32HogSize);

    /**
     * @brief Calculate the HOG of an image. The gradient calculation uses Sobel gradient.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDstH Output horizontal gradient result.
     * @param pstDstV Output vertical gradient result.
     * @param pstDstMag Output L2 Norm magnitude result from Gradient V, H.
     * @param pstDstAng Output atan2 angular result from Gradient V / Gradient H.
     * @param pstDstHist HOG histogram. result.
     * @param pstHogCtrl HOG control parameter.
     * @param bInstant DUmmy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_HOG(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                        IVE_DST_IMAGE_S *pstDstV, IVE_DST_IMAGE_S *pstDstMag,
                        IVE_DST_IMAGE_S *pstDstAng, IVE_DST_MEM_INFO_S *pstDstHist,
                        IVE_HOG_CTRL_S *pstHogCtrl, bool bInstant);

    /**
     * @brief Calculate the Magnitude and Nagular result from given horizontal and vertical gradients.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrcH Input horizontal gradient.
     * @param pstSrcV Input vertical gradient.
     * @param pstDstMag Output L2 norm magnitude result.
     * @param pstDstAng Output atan2 angular result.
     * @param pstMaaCtrl Magnitude and angular control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_MagAndAng(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrcH, IVE_SRC_IMAGE_S *pstSrcV,
                              IVE_DST_IMAGE_S *pstDstMag, IVE_DST_IMAGE_S *pstDstAng,
                              IVE_MAG_AND_ANG_CTRL_S *pstMaaCtrl, bool bInstant);

    /**
     * @brief Map src image to dst image with a given table.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstMap Mapping table. (length 256.)
     * @param pstDst Output image.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Map(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_MEM_INFO_S *pstMap,
                        IVE_DST_IMAGE_S *pstDst, bool bInstant);

    /**
     * @brief Merge two image with given mask.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstMask Mask, can be single channel. Pixels set to zero will mask the pixel in image 1.
     * @param pstDst Output image.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Mask(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                         IVE_SRC_IMAGE_S *pstMask, IVE_DST_IMAGE_S *pstDst, bool bInstant);

    /**
     * @brief Calculate the normalized gradient of an image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDstH Output horizontal gradient result. Accepts U8C1, S8C1.
     * @param pstDstV Output vertical gradient result. Accepts U8C1, S8C1.
     * @param pstDstHV Output combined L2 norm gradient result.
     * @param pstNormGradCtrl Norm gradient control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_NormGrad(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                             IVE_DST_IMAGE_S *pstDstV, IVE_DST_IMAGE_S *pstDstHV,
                             IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl, bool bInstant);

    /**
     * @brief Or two images and output the result.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstDst Output result.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Or(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                       IVE_DST_IMAGE_S *pstDst, bool bInstant);

    /**
     * @brief Calculate average value of image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param average average value of image;
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Average(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, float *average,
                            bool bInstant);

    /**
     * @brief Find min max of an image with a 3x3 size kernel.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDst Output image, width, height should be (input_length - ((kernel - 1)/2)).
     * @param pstOrdStatFltCtrl OrdStatFilter control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_OrdStatFilter(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                  IVE_DST_IMAGE_S *pstDst,
                                  IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl, bool bInstant);

    /**
     * @brief Run sigmoid of an input image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image.
     * @param pstDst Output image.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Sigmoid(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                            bool bInstant);

    /**
     * @brief Calculate SAD result with two same size input image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstSad Output SAD result.
     * @param pstThr Output thresholded SAD result.
     * @param pstSadCtrl SAD control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_SAD(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_IMAGE_S *pstSad, IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstSadCtrl,
                        bool bInstant);

    /**
     * @brief Calculate horizontal and vertical gradient using Sobel kernel.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDstH Output horizontal gradient result.
     * @param pstDstV Output vertical gradient result.
     * @param pstSobelCtrl Sobel control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Sobel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                          IVE_DST_IMAGE_S *pstDstV, IVE_SOBEL_CTRL_S *pstSobelCtrl, bool bInstant);

    /**
     * @brief Subtract two images and output the result.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstDst Output result.
     * @param ctrl Subtract control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Sub(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_IMAGE_S *pstDst, IVE_SUB_CTRL_S *ctrl, bool bInstant);

    /**
     * @brief Calculate the threshold result of an image.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input image. Only accepts U8C1.
     * @param pstDst Output result.
     * @param ctrl Threshold control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Thresh(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_THRESH_CTRL_S *ctrl, bool bInstant);

    /**
     * @brief Threshold an S16 image with high low threshold to U8 or S8.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input S16 image.
     * @param pstDst Output U8/ S8 image.
     * @param pstThrS16Ctrl S16 threshold control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Thresh_S16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                               IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl, bool bInstant);

    /**
     * @brief Threshold an U16 image with high low threshold to U8.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input U16 image.
     * @param pstDst Output U8 image.
     * @param pstThrU16Ctrl U16 threshold control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Thresh_U16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                               IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl, bool bInstant);

    /**
     * @brief XOR two images and output the result.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image 1.
     * @param pstSrc2 Input image 2.
     * @param pstDst Output result.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_Xor(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_IMAGE_S *pstDst, bool bInstant);

    // for cpu version

    /**
     * @brief Calculate number of island and label them. Input must be a binary image. Note that for
     * speed the source file will be modified.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc THe input binary image.
     * @param pstDst The labeled image.
     * @param numOfComponents Number of components found.
     * @param pstCCCtrl Connect component control parameter.
     * @param bInstant Dummy variable.
     * @return CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_CC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                       int *numOfComponents, IVE_CC_CTRL_S *pstCCCtrl, bool bInstant);

    /**
     * @brief INTEG make a integral image with one gray image
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc Input gray image.
     * @param pstDst Output int image.
     * @param bInstant Dummy variable.
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */

    CVI_S32 CVI_IVE_Integ(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_MEM_INFO_S *pstDst,
                          IVE_INTEG_CTRL_S *ctrl, bool bInstant);

    CVI_S32 CVI_IVE_Hist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_MEM_INFO_S *pstDst,
                         bool bInstant);

    CVI_S32 CVI_IVE_EqualizeHist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                 IVE_DST_IMAGE_S *pstDst, IVE_EQUALIZE_HIST_CTRL_S *ctrl,
                                 bool bInstant);

    CVI_S32 CVI_IVE_16BitTo8Bit(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                                IVE_16BIT_TO_8BIT_CTRL_S *ctrl, bool bInstant);

    CVI_S32 CVI_IVE_NCC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                        IVE_DST_MEM_INFO_S *pstDst, bool bInstant);

    CVI_S32 CVI_IVE_LBP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                        IVE_LBP_CTRL_S *ctrl, bool bInstant);

    CVI_S32 set_fmt_ex(CVI_S32 fd, CVI_S32 width, CVI_S32 height, // enum v4l2_buf_type type,
                       CVI_U32 pxlfmt, CVI_U32 csc, CVI_U32 quant);

    CVI_S32 CVI_IVE_CSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                        IVE_CSC_CTRL_S *ctrl, bool bInstant);

    CVI_S32 CVI_IVE_Resize(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_RESIZE_CTRL_S *ctrl, bool bInstant);

    CVI_S32 CVI_IVE_FilterAndCSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                 IVE_SRC_IMAGE_S *pstSrcBuf, IVE_DST_IMAGE_S *pstDst,
                                 IVE_FILTER_AND_CSC_CTRL_S *ctrl, bool bInstant);

    /**
     * @brief Pixel-wise compare ,abs(pstSrc1)>abs(pstSrc2)?255:0.
     *
     * @param pIveHandle Ive instance handler.
     * @param pstSrc1 Input image. only S8C1 accepted.
     * @param pstSrc2 Input image. only S8C1 accepted.
     * @param pstDst Output result. U8C1 type
     * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
     */
    CVI_S32 CVI_IVE_CMP_S8_BINARY(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                  IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst);

#ifdef __cplusplus
}
#endif

#endif // End of _IVE_H
