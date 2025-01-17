#ifndef _IVE_DRAW_H
#define _IVE_DRAW_H
#include "cvi_comm_ive.h"
#ifdef CV180X
#include "linux/cvi_comm_video.h"
#else
#include "cvi_comm_video.h"
#endif
#include "cvi_draw_ive.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw a hollow rectangle on IVE_DST_IMAGE_S.
 *
 * @param pIveHandle Ive instanace handler.
 * @param pstDst The output image.
 * @param pstDrawCtrl Draw paramters.
 * @param bInstant Dummy variable.
 * @return CVI_S32 Return success only if all the rectangle is drawn.
 */
CVI_S32 CVI_IVE_DrawRect(IVE_HANDLE pIveHandle, IVE_DST_IMAGE_S *pstDst,
                         IVE_DRAW_RECT_CTRL_S *pstDrawCtrl, bool bInstant);

#ifdef __cplusplus
}
#endif

#endif  // End of _IVE_DRAW_H
