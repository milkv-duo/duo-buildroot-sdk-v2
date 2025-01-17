#include "motion_detection/md.hpp"
#include "ive_log.hpp"
#include "cvi_module_core.h"
typedef struct {
  MotionDetection *md_model = nullptr;
} ive_context_t;
int CVI_IVE_Set_MotionDetection_Background(ive_handle_t handle, VIDEO_FRAME_INFO_S *frame) 
{
    ive_context_t *ctx = static_cast<ive_context_t *>(handle);
    MotionDetection *md_model = ctx->md_model;
    if (md_model == nullptr) {
        LOGD("Init Motion Detection.\n");
        ctx->md_model = new MotionDetection();
        return ctx->md_model->init(frame);
    }
    return ctx->md_model->update_background(frame);
}
int CVI_IVE_MotionDetection(ive_handle_t handle, VIDEO_FRAME_INFO_S *frame, ive_bbox_t_info_t *bbox, uint8_t threshold, double min_area)
{
  ive_context_t *ctx = static_cast<ive_context_t *>(handle);
  MotionDetection *md_model = ctx->md_model;
  if (md_model == nullptr) {
    LOGE(
        "Failed to do motion detection! Please invoke CVI_IVE_Set_MotionDetection_Background to set "
        "background image first.\n");
    return CVI_FAILURE;
  }
  return ctx->md_model->detect(frame, bbox, threshold, min_area);
}