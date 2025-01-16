
#ifndef CVI_BMCV_FILE_H
#define CVI_BMCV_FILE_H

#include <cvi_comm_vpss.h>
#include <cvi_type.h>

#define DLL_EXPORT __attribute__((visibility("default")))

typedef void* cvi_pre_handle_t;

typedef struct bmcv_resize_s {
  int start_x;
  int start_y;
  int in_width;
  int in_height;
  int out_width;
  int out_height;
} bmcv_resize_t;

typedef struct bmcv_rect {
  int start_x;
  int start_y;
  int crop_w;
  int crop_h;
} bmcv_rect_t;

typedef struct bmcv_resize_image_s {
  bmcv_resize_t* resize_img_attr;
  int roi_num;
  unsigned char stretch_fit;  // 0:keep aspect ratio,1:stretch
  unsigned char padding_b;
  unsigned char padding_g;
  unsigned char padding_r;
  unsigned int interpolation;
} bmcv_resize_image;

// y = alpha*x + beta
typedef struct bmcv_convert_to_attr_s {
  float alpha_0;
  float beta_0;
  float alpha_1;
  float beta_1;
  float alpha_2;
  float beta_2;
} bmcv_convert_to_attr;

#ifdef __cplusplus
extern "C" {
#endif
DLL_EXPORT CVI_S32 CVI_Preprocess_Create_Handle(cvi_pre_handle_t* handle);
DLL_EXPORT CVI_S32 CVI_Preprocess_Destroy_Handle(cvi_pre_handle_t handle);

DLL_EXPORT CVI_S32 CVI_Peprocess_Crop(cvi_pre_handle_t handle, int crop_num, bmcv_rect_t* rects,
                                      VIDEO_FRAME_INFO_S* input_frame,
                                      VIDEO_FRAME_INFO_S** output_frame);

DLL_EXPORT CVI_S32 CVI_Preprocess_Resize(cvi_pre_handle_t handle, bmcv_resize_image resize_param,
                                         VIDEO_FRAME_INFO_S* input_frame,
                                         VIDEO_FRAME_INFO_S** output_frame);

DLL_EXPORT CVI_S32 CVI_Preprocess_ConvertTo(cvi_pre_handle_t handle,
                                            bmcv_convert_to_attr convert_to_attr,
                                            VIDEO_FRAME_INFO_S* input_frame,
                                            VIDEO_FRAME_INFO_S** output_frame);

DLL_EXPORT CVI_S32 CVI_Preprocess_All(cvi_pre_handle_t handle, VIDEO_FRAME_INFO_S* input_frame,
                                      bmcv_resize_image* p_crop_resize_param,
                                      bmcv_convert_to_attr* p_cvt_param,
                                      VIDEO_FRAME_INFO_S** output_frame);

DLL_EXPORT CVI_S32 CVI_Preprocess_ReleaseFrame(cvi_pre_handle_t handle,
                                               VIDEO_FRAME_INFO_S* output_frame);
#ifdef __cplusplus
}
#endif
#endif
