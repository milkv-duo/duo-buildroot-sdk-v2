#ifndef _TPU_IVE_H
#define _TPU_IVE_H
#include "cvi_comm_ive.h"
#ifdef CV180X
#include "linux/cvi_comm_video.h"
#else
#include "cvi_comm_video.h"
#endif
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

IVE_HANDLE CVI_TPU_IVE_CreateHandle();
CVI_S32 CVI_TPU_IVE_DestroyHandle(IVE_HANDLE pIveHandle);

/**
 * @brief Blend Y channel of input frame, pstSrc2_dst->y = pstSrc1->y*alpha +
 * (255-alpha)*pstSrc2_dst->y
 *
 * @param pIveHandle Ive instance handler.
 * @param pstSrc1 Input frame1
 * @param pstSrc2_dst Input frame2 and output frame,the blended Y would be written to pstSrc2_dst->y
 * @param pstAlpha Blend weight
 * @return CVI_S32 CVI_S32 Return CVI_SUCCESS if succeed.
 */
CVI_S32 CVI_IVE_Blend_Pixel_Y(IVE_HANDLE pIveHandle, VIDEO_FRAME_INFO_S *pstSrc1,
                              VIDEO_FRAME_INFO_S *pstSrc2_dst, VIDEO_FRAME_INFO_S *pstAlpha);

#ifdef __cplusplus
}
#endif

#endif  // End of _IVE_H
