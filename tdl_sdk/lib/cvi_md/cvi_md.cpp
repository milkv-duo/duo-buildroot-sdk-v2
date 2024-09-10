#include "cvi_md.h"
#include "md.hpp"
#include "cvi_tdl_log.hpp"


CVI_S32 CVI_MD_Create_Handle(cvi_md_handle_t *handle){
  ive::IVE *ive_handle = new ive::IVE;
  if (ive_handle->init() != CVI_SUCCESS) {
    LOGE("IVE handle init failed.\n");
    return CVI_FAILURE;
  } else {
    LOGI("IVE handle inited\n");
  }
  MotionDetection *p_md_inst = new MotionDetection(ive_handle);
  *handle = p_md_inst;
  LOGI("MD handle inited\n");
  return CVI_SUCCESS;
}

CVI_S32 CVI_MD_Destroy_Handle(cvi_md_handle_t handle){
  if(handle == nullptr){
      LOGE("handle is nullptr");
      return CVI_FAILURE;
  }
  MotionDetection *p_md_inst = (MotionDetection*)handle;
  delete p_md_inst->get_ive_instance();
  delete p_md_inst;
  return CVI_SUCCESS;
}
/**
 * @brief Set background frame for motion detection.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * be returned.
 * @return int Return CVI_SUCCESS on success.
 */
CVI_S32 CVI_MD_Set_Background(const cvi_md_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame){
    if(handle == nullptr){
      LOGE("handle is nullptr");
      return CVI_FAILURE;
    }
    MotionDetection *p_md_inst = (MotionDetection*)handle;
    return p_md_inst->update_background(frame);
}

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

CVI_S32 CVI_MD_Set_ROI(const cvi_md_handle_t handle, MDROI *roi_s){
    if(handle == nullptr){
      LOGE("handle is nullptr");
      return CVI_FAILURE;
    }
    MotionDetection *p_md_inst = (MotionDetection*)handle;
    return p_md_inst->set_roi(reinterpret_cast<MDROI_t*>(roi_s));
}
/**
 * @brief Do Motion Detection with background subtraction method.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame, should be YUV420 format.
 * @param objects Detected object info
 * @param threshold Threshold of motion detection, the range between 0 and 255.
 * @param min_area Minimal pixel area. The bounding box whose area is larger than this value would
 * @return int Return CVI_SUCCESS on success.
 */
CVI_S32 CVI_MD_Detect(const cvi_md_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                            int **pp_boxes,uint32_t *p_num_boxes, uint8_t threshold,
                            double min_area){
    *p_num_boxes = 0;
    if(handle == nullptr){
      LOGE("handle is nullptr");
      return CVI_FAILURE;
    }
    MotionDetection *p_md_inst = (MotionDetection*)handle;
    
    std::vector<std::vector<float>> bboxes;
    CVI_S32 ret = p_md_inst->detect(frame,bboxes,threshold,min_area);
    *p_num_boxes = bboxes.size();
    int *p_box = (int*)malloc(bboxes.size()*4*sizeof(int));
    
    for(size_t i = 0; i < bboxes.size();i++){
      p_box[4*i+0] = bboxes[i][0];
      p_box[4*i+1] = bboxes[i][1];
      p_box[4*i+2] = bboxes[i][2];
      p_box[4*i+3] = bboxes[i][3];
    }
    *pp_boxes = p_box;
    return ret;
}


