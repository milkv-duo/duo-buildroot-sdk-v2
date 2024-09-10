#ifndef _CVI_TDL_UTILS_H_
#define _CVI_TDL_UTILS_H_
#include "cvi_tdl_core.h"
#include "face/cvtdl_face_types.h"
#include "object/cvtdl_object_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup core_utils TDL Utilities for Preprocessing and Post-processing
 * \ingroup core_cvitdlcore
 */
/**@{*/

/**
 * @brief Dequantize an int8_t output result from NN.
 *
 * @param quantizedData Input quantized data.
 * @param data Output float data.
 * @param bufferSize Size of the buffer.
 * @param dequantizeThreshold Dequantize threshold.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Dequantize(const int8_t *quantizedData, float *data,
                                      const uint32_t bufferSize, const float dequantizeThreshold);

/**
 * @brief Do softmax on buffer.
 *
 * @param inputBuffer Input float buffer.
 * @param outputBuffer Output result.
 * @param bufferSize Size of the buffer.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_SoftMax(const float *inputBuffer, float *outputBuffer,
                                   const uint32_t bufferSize);

/**
 * @brief Do non maximum suppression on cvtdl_face_t.
 *
 * @param face Input cvtdl_face_t.
 * @param faceNMS Output result.
 * @param threshold NMS threshold.
 * @param method Support 'u' and 'm'. (intersection over union and intersection over min area)
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceNMS(const cvtdl_face_t *face, cvtdl_face_t *faceNMS,
                                   const float threshold, const char method);

/**
 * @brief Do non maximum suppression on cvtdl_object_t.
 *
 * @param obj Input cvtdl_object_t.
 * @param objNMS Output result.
 * @param threshold NMS threshold.
 * @param method Support 'u' and 'm'. (intersection over union and intersection over min area)
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_ObjectNMS(const cvtdl_object_t *obj, cvtdl_object_t *objNMS,
                                     const float threshold, const char method);

/**
 * @brief
 *
 * @param inFrame Input frame.
 * @param metaWidth The face meta width used for coordinate recovery.
 * @param metaHeight The face meta height used for coordinate recovery.
 * @param info The face info.
 * @param outFrame Output face align result. Frame must be preallocated.
 * @param enableGDC Enable GDC hardware support.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceAlignment(VIDEO_FRAME_INFO_S *inFrame, const uint32_t metaWidth,
                                         const uint32_t metaHeight, const cvtdl_face_info_t *info,
                                         VIDEO_FRAME_INFO_S *outFrame, const bool enableGDC);

DLL_EXPORT CVI_S32 CVI_TDL_Face_Quality_Laplacian(VIDEO_FRAME_INFO_S *srcFrame,
                                                  cvtdl_face_info_t *face_info, float *score);

/**
 * @brief
 *
 * @param image Output image.
 * @param height The height of output image.
 * @param width The width of output image.
 * @param fmt The pixel format of output image.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_CreateImage(cvtdl_image_t *image, uint32_t height, uint32_t width,
                                       PIXEL_FORMAT_E fmt);

/**
 * @brief
 *
 * @param height The height of the image.
 * @param width The width of the image.
 * @param fmt The pixel format of the image.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_EstimateImageSize(uint64_t *size, uint32_t height, uint32_t width,
                                             PIXEL_FORMAT_E fmt);

DLL_EXPORT CVI_S32 CVI_TDL_LoadBinImage(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                        PIXEL_FORMAT_E format);
DLL_EXPORT CVI_S32 CVI_TDL_StbReadImage(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                                        PIXEL_FORMAT_E format);

DLL_EXPORT CVI_S32 CVI_TDL_DestroyImage(VIDEO_FRAME_INFO_S *frame);
DLL_EXPORT CVI_S32 CVI_TDL_DumpVpssFrame(const char *filepath, VIDEO_FRAME_INFO_S *frame);
DLL_EXPORT CVI_S32 CVI_TDL_DumpImage(const char *filepath, cvtdl_image_t *image);
DLL_EXPORT CVI_S32 CVI_TDL_CreateImageFromVideoFrame(const VIDEO_FRAME_INFO_S *p_src_frame,
                                                     cvtdl_image_t *image);
DLL_EXPORT CVI_S32 CVI_TDL_CreateImageFromVideoFrameSize(const VIDEO_FRAME_INFO_S *p_crop_frame,
                                                         cvtdl_image_t *image, uint32_t img_size);
#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_UTILS_H_
