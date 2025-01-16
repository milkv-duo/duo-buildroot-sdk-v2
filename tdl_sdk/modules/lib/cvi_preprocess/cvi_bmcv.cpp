
#include "cvi_bmcv.h"
#include <map>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl_log.hpp"
#include "vpss_engine.hpp"

typedef struct _stVpssInfo {
  int vpss_grp;
  int vpss_chn;
} stVpssInfo;
static std::map<VIDEO_FRAME_INFO_S *, stVpssInfo> g_outframe_vpss_info_;
using namespace cvitdl;

template <typename T>
void release_buffer(T *p_buf) {
  if (p_buf == nullptr) {
    return;
  }
  delete p_buf;
}
DLL_EXPORT CVI_S32 CVI_Preprocess_Create_Handle(cvi_pre_handle_t *handle) {
  VpssEngine *vpss_inst = new VpssEngine(-1, 0);
  if (!vpss_inst->isInitialized()) {
    vpss_inst->init();
  }
  *handle = vpss_inst;
  return CVI_SUCCESS;
}
DLL_EXPORT CVI_S32 CVI_Preprocess_Destroy_Handle(cvi_pre_handle_t handle) {
  if (handle == nullptr) {
    LOGW("handle is null\n");
    return CVI_FAILURE;
  }
  VpssEngine *vpss_inst = (VpssEngine *)handle;
  delete vpss_inst;
  return CVI_SUCCESS;
}
VPSS_CROP_INFO_S *generate_crop_chn_attr(bmcv_resize_t resize_param) {
  if (resize_param.in_width == 0 || resize_param.in_height == 0) {
    return nullptr;
  }

  auto p_crop_attr = new VPSS_CROP_INFO_S;
  p_crop_attr->bEnable = true;
  p_crop_attr->stCropRect = {resize_param.start_x, resize_param.start_y,
                             (uint32_t)resize_param.in_width, (uint32_t)resize_param.in_height};

  return p_crop_attr;
}
VPSS_CHN_ATTR_S *generate_vpss_chn_attr(uint32_t src_width, uint32_t src_height, uint8_t index,
                                        bmcv_resize_image *p_crop_resize_param,
                                        bmcv_convert_to_attr *p_cvt_param,
                                        PIXEL_FORMAT_E dst_format) {
  int pad_val[3] = {0, 0, 0};
  int pad_type = 0;
  if (p_crop_resize_param) {
    pad_val[0] = p_crop_resize_param->padding_r;
    pad_val[1] = p_crop_resize_param->padding_g;
    pad_val[2] = p_crop_resize_param->padding_b;
    if (p_crop_resize_param->stretch_fit != 0) {
      pad_type = 1;
    }
  }

  auto p_chn_attr = new VPSS_CHN_ATTR_S;
  float *factor = nullptr;
  float *mean = nullptr;
  uint32_t dst_width = src_width;
  uint32_t dst_height = src_height;
  if (p_crop_resize_param) {
    bmcv_resize_t resize_param = p_crop_resize_param->resize_img_attr[index];
    dst_width = (uint32_t)resize_param.out_width;
    dst_height = (uint32_t)resize_param.out_height;
  }
  if (p_cvt_param) {
    factor = new float[3];
    mean = new float[3];
    factor[0] = p_cvt_param->alpha_0;
    factor[1] = p_cvt_param->alpha_1;
    factor[2] = p_cvt_param->alpha_2;
    // vpss preprocess Y=factor*X-mean
    mean[0] = p_cvt_param->beta_0;
    mean[1] = p_cvt_param->beta_1;
    mean[2] = p_cvt_param->beta_2;

    float factor_limit = 8191.f / 8192;
    for (uint32_t j = 0; j < 3; j++) {
      if (factor[j] > factor_limit) {
        LOGE("factor[%d] is bigger than limit: %f\n", j, factor[j]);
        factor[j] = factor_limit;
      }
    }
  }
  VPSS_CHN_SQ_HELPER_X(p_chn_attr, src_width, src_height, dst_width, dst_height, dst_format, factor,
                       mean, pad_val, pad_type);
  delete[] factor;
  delete[] mean;
  return p_chn_attr;
}

CVI_S32 CVI_Peprocess_Crop(cvi_pre_handle_t handle, int crop_num, bmcv_rect_t *rects,
                           VIDEO_FRAME_INFO_S *input_frame, VIDEO_FRAME_INFO_S **output_frame) {
  VPSS_CROP_INFO_S *p_crop_attr = new VPSS_CROP_INFO_S[crop_num];
  bmcv_resize_image resize_param;
  memset(&resize_param, 0, sizeof(resize_param));
  resize_param.roi_num = crop_num;
  resize_param.resize_img_attr = new bmcv_resize_t[crop_num];
  for (int i = 0; i < crop_num; i++) {
    resize_param.resize_img_attr[i].start_x = rects[i].start_x;
    resize_param.resize_img_attr[i].start_y = rects[i].start_y;
    resize_param.resize_img_attr[i].in_width = rects[i].crop_w;
    resize_param.resize_img_attr[i].in_height = rects[i].crop_h;
    resize_param.resize_img_attr[i].out_width = rects[i].crop_w;
    resize_param.resize_img_attr[i].out_height = rects[i].crop_h;
  }
  int ret = CVI_Preprocess_All(handle, input_frame, &resize_param, nullptr, output_frame);
  delete[] resize_param.resize_img_attr;
  delete[] p_crop_attr;
  return ret;
}

CVI_S32 CVI_Preprocess_Resize(cvi_pre_handle_t handle, bmcv_resize_image resize_param,
                              VIDEO_FRAME_INFO_S *input_frame, VIDEO_FRAME_INFO_S **output_frame) {
  int ret = CVI_Preprocess_All(handle, input_frame, &resize_param, nullptr, output_frame);
  return ret;
}
CVI_S32 CVI_Preprocess_ConvertTo(cvi_pre_handle_t handle, bmcv_convert_to_attr convert_to_attr,
                                 VIDEO_FRAME_INFO_S *input_frame,
                                 VIDEO_FRAME_INFO_S **output_frame) {
  int ret = CVI_Preprocess_All(handle, input_frame, nullptr, &convert_to_attr, output_frame);
  return ret;
}
CVI_S32 CVI_Preprocess_All(cvi_pre_handle_t handle, VIDEO_FRAME_INFO_S *input_frame,
                           bmcv_resize_image *p_crop_resize_param,
                           bmcv_convert_to_attr *p_cvt_param, VIDEO_FRAME_INFO_S **output_frame) {
  if (handle == nullptr) {
    LOGE("vpss handle not inited\n");
    return CVI_FAILURE;
  }
  VpssEngine *vpss_inst = (VpssEngine *)handle;

  if (p_crop_resize_param == nullptr && p_cvt_param == nullptr) {
    LOGE("p_crop_resize_param and p_cvt_param should not both be nullptr");
    return CVI_FAILURE;
  }

  int num_chs = 1;
  if (p_crop_resize_param != nullptr) {
    num_chs = p_crop_resize_param->roi_num;
  }

  int ret = CVI_SUCCESS;
  auto _output_frame = new VIDEO_FRAME_INFO_S[num_chs];
  memset(_output_frame, 0, sizeof(VIDEO_FRAME_INFO_S) * num_chs);
  int grp_id = (int)vpss_inst->getGrpId();
  bmcv_resize_t img_crop_attr;
  memset(&img_crop_attr, 0, sizeof(bmcv_resize_t));
  for (int i = 0; i < num_chs; i++) {
    PIXEL_FORMAT_E dst_format = input_frame->stVFrame.enPixelFormat;
    auto vpss_chn_attr =
        generate_vpss_chn_attr(input_frame->stVFrame.u32Width, input_frame->stVFrame.u32Height, i,
                               p_crop_resize_param, p_cvt_param, dst_format);
    if (p_crop_resize_param != nullptr && p_crop_resize_param->resize_img_attr != nullptr) {
      img_crop_attr = p_crop_resize_param->resize_img_attr[i];
    }
    auto crop_attr = generate_crop_chn_attr(img_crop_attr);
    ret = vpss_inst->sendFrameBase(input_frame, nullptr, crop_attr, vpss_chn_attr, nullptr, 1);
    if (ret != 0) {
      LOGE("send frame base failed\n");
      release_buffer(vpss_chn_attr);
      release_buffer(crop_attr);
      return CVI_FAILURE;
    }
    auto o_frame = &_output_frame[i];
    ret = vpss_inst->getFrame(o_frame, 0, 2000);
    if (ret == CVI_SUCCESS) {
      stVpssInfo info;
      info.vpss_grp = grp_id;
      info.vpss_chn = 1;
      g_outframe_vpss_info_[o_frame] = info;
    }
    release_buffer(vpss_chn_attr);
    release_buffer(crop_attr);
  }
  *output_frame = _output_frame;
  return ret;
}
CVI_S32 CVI_Preprocess_ReleaseFrame(cvi_pre_handle_t handle, VIDEO_FRAME_INFO_S *output_frame) {
  if (handle == nullptr) {
    LOGE("vpss handle not inited\n");
    return CVI_FAILURE;
  }
  VpssEngine *vpss_inst = (VpssEngine *)handle;
  auto size = g_outframe_vpss_info_.size();
  for (uint8_t i = 0; i < size; i++) {
    auto frame = &output_frame[i];
    vpss_inst->releaseFrame(frame, 1);
    g_outframe_vpss_info_.erase(frame);
  }
  delete output_frame;
  return CVI_SUCCESS;
}