#ifndef _CVI_IVE_MODULE_H_
#define _CVI_IVE_MODULE_H_
/** @typedef ive_handle_t
 * @ingroup core_cvi_ivecore
 * @brief An ive handle
 */
typedef void *ive_handle_t;

typedef struct {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
} ive_bbox;

typedef struct {
  ive_bbox bbox;
} ive_bbox_t;

typedef struct {
  uint32_t size;
  ive_bbox_t *info;
} ive_bbox_t_info_t;

/**
 * @brief Set background frame for motion detection.
 *
 * @param handle An IVE SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * be returned.
 * @return int Return CVI_SUCCESS on success.
 */
int CVI_IVE_Set_MotionDetection_Background(ive_handle_t handle, VIDEO_FRAME_INFO_S *frame);

/**
 * @brief Do Motion Detection with background subtraction method.
 *
 * @param handle An IVE SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * @param bbox ive_bbox_t info
 * @param threshold Threshold of motion detection, the range between 0 and 255.
 * @param min_area Minimal pixel area. The bounding box whose area is larger than this value would
 * @return int Return CVI_SUCCESS on success.
 */
int CVI_IVE_MotionDetection(ive_handle_t handle, VIDEO_FRAME_INFO_S *frame, ive_bbox_t_info_t *bbox,
                            uint8_t threshold, double min_area);
#endif