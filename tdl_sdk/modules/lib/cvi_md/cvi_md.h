#ifndef _CVI_MD_HEAD_
#define _CVI_MD_HEAD_

#include <cvi_comm_vpss.h>
#include <cvi_type.h>

#define DLL_EXPORT __attribute__((visibility("default")))
typedef void *cvi_md_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MDPoint {
  int x1;
  int y1;
  int x2;
  int y2;
} MDPoint;

typedef struct _MDROI {
  uint8_t num;
  MDPoint pnt[4];
} MDROI;

DLL_EXPORT CVI_S32 CVI_MD_Create_Handle(cvi_md_handle_t *handle);
DLL_EXPORT CVI_S32 CVI_MD_Destroy_Handle(cvi_md_handle_t handle);
/**
 * @brief Set background frame for motion detection.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * be returned.
 * @return int Return CVI_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_MD_Set_Background(const cvi_md_handle_t handle, VIDEO_FRAME_INFO_S *frame);

/**
 * @brief Set ROI frame for motion detection.
 *
 * @param handle An TDL SDK handle.
 * @param x1 left x coordinate of roi
 * @param y1 top y coordinate of roi
 * @param x2 right x coordinate of roi
 * @param y2 bottom y coordinate of roi
 *
 * be returned.
 * @return int Return CVI_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_MD_Set_ROI(const cvi_md_handle_t handle, MDROI *_roi_s);
/**
 * @brief Do Motion Detection with background subtraction method.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * @param pp_boxes box list with [x1,y1,x2,y2,x1,y1,x2,y2,...],
 *                  the buffer would be allocated inside,and should be freed outside
 * @param p_num_boxes number of boxes in pp_boxes
 * @param threshold Threshold of motion detection, the range between 0 and 255.
 * @param min_area Minimal pixel area. The bounding box whose area is larger than this value would
 * @return int Return CVI_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_MD_Detect(const cvi_md_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                 int **pp_boxes, uint32_t *p_num_boxes, uint8_t threshold,
                                 double min_area);

#ifdef __cplusplus
}
#endif
#endif
