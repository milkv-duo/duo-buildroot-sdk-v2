

#include "license_plate_keypoint.hpp"
#include "core/utils/vpss_helper.h"
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/face/cvtdl_face_types.h"
#include "rescale_utils.hpp"

#define SCALE (1 / 127.5)
#define MEAN (1)

#define OUTPUT_NAME_PROBABILITY "output_ReduceMean_f32"

namespace cvitdl {

LicensePlateKeypoint::LicensePlateKeypoint() : Core(CVI_MEM_DEVICE) {
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = SCALE;
    m_preprocess_param[0].mean[i] = MEAN;
  }
  m_preprocess_param[0].resize_method = VPSS_SCALE_COEF_BILINEAR;
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = false;
}


int LicensePlateKeypoint::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_BGR_888) {
    LOGE(
        "Error: pixel format not match PIXEL_FORMAT_RGB_888 or PIXEL_FORMAT_BGR_888");
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  for (size_t n = 0; n < vehicle_plate_meta->size; n++) {
    VIDEO_FRAME_INFO_S crop_frame;
    vehicle_plate_meta->info[n].vehicle_properity =
          (cvtdl_vehicle_meta *)malloc(sizeof(cvtdl_vehicle_meta));
    cvtdl_vehicle_meta *v_meta = vehicle_plate_meta->info[n].vehicle_properity;
    cur_obj_info = info_extern_crop_resize_img(
    frame->stVFrame.u32Width, frame->stVFrame.u32Height, &vehicle_plate_meta->info[n], 0.15, 0.35);
    memset(&crop_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    int height = cur_obj_info.bbox.y2 - cur_obj_info.bbox.y1;
    int width = cur_obj_info.bbox.x2 - cur_obj_info.bbox.x1;
    vpssCropImage(frame, &crop_frame, cur_obj_info.bbox, width, height, PIXEL_FORMAT_BGR_888);
    std::vector<VIDEO_FRAME_INFO_S *> frames = {&crop_frame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      mp_vpss_inst->releaseFrame(&crop_frame, 0);
      return ret;
    }
    frame_h = crop_frame.stVFrame.u32Height;
    frame_w = crop_frame.stVFrame.u32Width;
    mp_vpss_inst->releaseFrame(&crop_frame, 0);
    outputParser(v_meta);
    model_timer_.TicToc("post");
  }
  return CVI_SUCCESS;
}

int LicensePlateKeypoint::outputParser(cvtdl_vehicle_meta *v_meta) {
  float *out_pts = getOutputRawPtr<float>(0);
  CVI_SHAPE output_shape = getOutputShape(0);
  for (int i = 0; i < 4; i++) {
    v_meta->license_pts.x[i] = static_cast<float>(cur_obj_info.bbox.x1 + out_pts[2*i] * frame_w);
    v_meta->license_pts.y[i] = static_cast<float>(cur_obj_info.bbox.y1 + out_pts[2*i+1] * frame_h);
  }
  return CVI_SUCCESS;
}
}  // namespace cviai