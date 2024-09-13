#ifndef _CVI_TDL_MEDIA_H_
#define _CVI_TDL_MEDIA_H_
#include <cvi_comm.h>

#define DLL_EXPORT __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

typedef void *imgprocess_t;
DLL_EXPORT CVI_S32 CVI_TDL_Create_ImageProcessor(imgprocess_t *handle);

DLL_EXPORT CVI_S32 CVI_TDL_ReadImage(imgprocess_t handle, const char *filepath,
                                     VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format);
DLL_EXPORT CVI_S32 CVI_TDL_ReadImage_Resize(imgprocess_t handle, const char *filepath,
                                            VIDEO_FRAME_INFO_S *frame, PIXEL_FORMAT_E format,
                                            uint32_t width, uint32_t height);
DLL_EXPORT CVI_S32 CVI_TDL_ReadImage_CenrerCrop_Resize(imgprocess_t handle, const char *filepath,
                                                       VIDEO_FRAME_INFO_S *frame,
                                                       PIXEL_FORMAT_E format, uint32_t width,
                                                       uint32_t height);
DLL_EXPORT CVI_S32 CVI_TDL_ReleaseImage(imgprocess_t handle, VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_Destroy_ImageProcessor(imgprocess_t handle);
#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_MEDIA_H_
