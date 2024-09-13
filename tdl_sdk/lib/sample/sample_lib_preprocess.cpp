#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cvi_bmcv.h"
#include <cstring>
#include "core/utils/vpss_helper.h"
#include <cvi_ive.h>
typedef enum {
  RESZIE = 0,
  CROP,
  MULIT_RESIZE,
  NORMAL,
} BM_PREPROCESS;


void mmap_video_frame(VIDEO_FRAME_INFO_S *frame) {
  CVI_U32 f_frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], f_frame_size);
    if (frame->stVFrame.u32Length[1] != 0) {
      frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    } else {
      frame->stVFrame.pu8VirAddr[1] = NULL;
    }
    if (frame->stVFrame.u32Length[2] != 0) {
      frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    } else {
      frame->stVFrame.pu8VirAddr[1] = NULL;
    }
  }
}
void unmap_video_frame(VIDEO_FRAME_INFO_S *frame) {
  CVI_U32 f_frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], f_frame_size);
  frame->stVFrame.pu8VirAddr[0] = NULL;
  frame->stVFrame.pu8VirAddr[1] = NULL;
  frame->stVFrame.pu8VirAddr[2] = NULL;
}

void dump_img(IVE_HANDLE ive_handle,VIDEO_FRAME_INFO_S *frame, uint8_t index, char *name) {
  char img_name[40];
  IVE_IMAGE_S ive_img;
  memset(&ive_img, 0, sizeof(IVE_IMAGE_S));
  mmap_video_frame(frame);
  CVI_IVE_VideoFrameInfo2Image(frame,&ive_img);
  sprintf(img_name,"resized%d_%s.jpg", index, name);
  CVI_IVE_WriteImage(ive_handle,img_name,&ive_img);
  CVI_SYS_FreeI(ive_handle, &ive_img);
  unmap_video_frame(frame);
}

int test_crop(cvi_pre_handle_t pre_handle,IVE_HANDLE ive_handle,VIDEO_FRAME_INFO_S *input_frame){
  
  uint8_t roi_num = 1;
  auto crop_param = new bmcv_rect_t[roi_num];
  memset(crop_param, 0, sizeof(bmcv_rect_t)*roi_num);
  crop_param->start_x = 100;
  crop_param->start_y = 100;
  crop_param->crop_w = input_frame->stVFrame.u32Width - 100;
  crop_param->crop_h = input_frame->stVFrame.u32Height - 100;
  
  VIDEO_FRAME_INFO_S *out_frame = NULL;
  CVI_S32 ret = CVI_Peprocess_Crop(pre_handle, roi_num, crop_param, input_frame, &out_frame);
  if(ret != CVI_SUCCESS){
    printf("resize failed\n");
    return CVI_FAILURE;
  }

  for(uint8_t i = 0; i<roi_num; i++) {
    dump_img(ive_handle, &out_frame[i], i, "crop");
  }
  if (nullptr != crop_param) {
    delete [] crop_param;
  }

  ret=CVI_Preprocess_ReleaseFrame(pre_handle, out_frame);
  if(ret != CVI_SUCCESS){
    printf("output_frame release failed\n");
    return CVI_FAILURE;
  }
  return ret;
}

int test_resize(cvi_pre_handle_t pre_handle,IVE_HANDLE ive_handle,VIDEO_FRAME_INFO_S *input_frame){
  bmcv_resize_image resize_param;
  memset(&resize_param,0,sizeof(resize_param));
  resize_param.roi_num = 1;
  
  resize_param.resize_img_attr = new bmcv_resize_t[resize_param.roi_num];
  resize_param.resize_img_attr[0].start_x = 0;
  resize_param.resize_img_attr[0].start_y = 0;
  resize_param.resize_img_attr[0].in_width = input_frame->stVFrame.u32Width;
  resize_param.resize_img_attr[0].in_height = input_frame->stVFrame.u32Height;
  
  resize_param.resize_img_attr[0].out_width = 500;
  resize_param.resize_img_attr[0].out_height = 500;
  
  VIDEO_FRAME_INFO_S *out_frame = NULL;
  CVI_S32 ret = CVI_Preprocess_Resize(pre_handle,resize_param,input_frame, &out_frame);
  if(ret != CVI_SUCCESS){
    printf("resize failed\n");
    return CVI_FAILURE;
  }

  for(uint8_t i = 0; i<resize_param.roi_num; i++) {
    dump_img(ive_handle, &out_frame[i], i, "resize");
  }

  if (nullptr != resize_param.resize_img_attr) {
    delete [] resize_param.resize_img_attr;
  }

  ret=CVI_Preprocess_ReleaseFrame(pre_handle, out_frame);
  if(ret != CVI_SUCCESS){
    printf("output_frame release failed\n");
    return CVI_FAILURE;
  }
  return ret;
}

int test_mutil_resize(cvi_pre_handle_t pre_handle,IVE_HANDLE ive_handle,VIDEO_FRAME_INFO_S *input_frame){
  bmcv_resize_image resize_param;
  memset(&resize_param,0,sizeof(resize_param));
  resize_param.roi_num = 2;
  
  resize_param.resize_img_attr = new bmcv_resize_t[resize_param.roi_num];
  resize_param.resize_img_attr[0].start_x = 0;
  resize_param.resize_img_attr[0].start_y = 0;
  resize_param.resize_img_attr[0].in_width = input_frame->stVFrame.u32Width;
  resize_param.resize_img_attr[0].in_height = input_frame->stVFrame.u32Height;
  resize_param.resize_img_attr[0].out_width = 500;
  resize_param.resize_img_attr[0].out_height = 500;

  resize_param.resize_img_attr[1].start_x = 0;
  resize_param.resize_img_attr[1].start_y = 0;
  resize_param.resize_img_attr[1].in_width = input_frame->stVFrame.u32Width;
  resize_param.resize_img_attr[1].in_height = input_frame->stVFrame.u32Height;
  resize_param.resize_img_attr[1].out_width = 800;
  resize_param.resize_img_attr[1].out_height = 800;
  
  VIDEO_FRAME_INFO_S *out_frame = NULL;
  CVI_S32 ret = CVI_Preprocess_Resize(pre_handle,resize_param,input_frame, &out_frame);
  if(ret != CVI_SUCCESS){
    printf("resize failed\n");
    return CVI_FAILURE;
  }

  for(uint8_t i = 0; i<resize_param.roi_num; i++) {
    dump_img(ive_handle, &out_frame[i], i, "mutil_resize");
  }
  if (nullptr != resize_param.resize_img_attr) {
    delete [] resize_param.resize_img_attr;
  }

  ret=CVI_Preprocess_ReleaseFrame(pre_handle, out_frame);
  if(ret != CVI_SUCCESS){
    printf("output_frame release failed\n");
    return CVI_FAILURE;
  }
  return ret;
}


int test_normalize(cvi_pre_handle_t pre_handle,IVE_HANDLE ive_handle,VIDEO_FRAME_INFO_S *input_frame){
  bmcv_convert_to_attr normal_param;
  memset(&normal_param,0,sizeof(normal_param));
  normal_param.alpha_0 = 0.5;
  normal_param.alpha_1 = 0.5;
  normal_param.alpha_2 = 0.5;
  normal_param.beta_0 = 10;
  normal_param.beta_1 = 20;
  normal_param.beta_2 = 30;
  
  VIDEO_FRAME_INFO_S *out_frame = NULL;
  CVI_S32 ret = CVI_Preprocess_ConvertTo(pre_handle, normal_param, input_frame, &out_frame);
  if(ret != CVI_SUCCESS){
    printf("resize failed\n");
    return CVI_FAILURE;
  }
  mmap_video_frame(input_frame);
  printf("normalize_input0 %d %d %d\n", input_frame->stVFrame.pu8VirAddr[0][0],
                       input_frame->stVFrame.pu8VirAddr[0][1],
                       input_frame->stVFrame.pu8VirAddr[0][2]);
  printf("normalize_input1 %d %d %d\n", input_frame->stVFrame.pu8VirAddr[1][0],
                       input_frame->stVFrame.pu8VirAddr[1][1],
                       input_frame->stVFrame.pu8VirAddr[1][2]);
  printf("normalize_input2 %d %d %d\n", input_frame->stVFrame.pu8VirAddr[2][0],
                       input_frame->stVFrame.pu8VirAddr[2][1],
                       input_frame->stVFrame.pu8VirAddr[2][2]);
  unmap_video_frame(input_frame);

  mmap_video_frame(out_frame);
  printf("normalize_output0 %d %d %d\n", out_frame->stVFrame.pu8VirAddr[0][0],
                       out_frame->stVFrame.pu8VirAddr[0][1],
                       out_frame->stVFrame.pu8VirAddr[0][2]);
  printf("normalize_output1 %d %d %d\n", out_frame->stVFrame.pu8VirAddr[1][0],
                       out_frame->stVFrame.pu8VirAddr[1][1],
                       out_frame->stVFrame.pu8VirAddr[1][2]);
  printf("normalize_output2 %d %d %d\n", out_frame->stVFrame.pu8VirAddr[2][0],
                       out_frame->stVFrame.pu8VirAddr[2][1],
                       out_frame->stVFrame.pu8VirAddr[2][2]);
  unmap_video_frame(out_frame);


  ret=CVI_Preprocess_ReleaseFrame(pre_handle, out_frame);
  if(ret != CVI_SUCCESS){
    printf("output_frame release failed\n");
    return CVI_FAILURE;
  }
  return ret;
}

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  int ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3, 
                            vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 3);
  if (ret != 0) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  IVE_HANDLE ive_handle = CVI_IVE_CreateHandle();
  cvi_pre_handle_t pre_handle;
  ret = CVI_Preprocess_Create_Handle(&pre_handle);

  const char *strf1 = argv[1];
  int bm_preprocess_case = atoi(argv[2]);

  VIDEO_FRAME_INFO_S frame;
  IVE_IMAGE_S image1 = CVI_IVE_ReadImage(ive_handle, strf1, IVE_IMAGE_TYPE_U8C3_PLANAR);
  int imgw = image1.u32Width;
  if (imgw == 0) {
    printf("Read image failed with %x!\n", ret);
    return CVI_FAILURE;
  } else {
    printf("Read image ok with %d!\n", imgw);
  }
  ret = CVI_IVE_Image2VideoFrameInfo(&image1, &frame);
  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }

  switch (bm_preprocess_case) {
    case RESZIE:
      for (int i=0; i<10; i++){
        printf("resize i = %d\n",i);
        test_resize(pre_handle,ive_handle,&frame);
      }
      break;
    case CROP:
      for (int i=0; i<10; i++){
        printf("crop i = %d\n",i);
        test_crop(pre_handle,ive_handle,&frame);
      }
      break;
    case MULIT_RESIZE:
      for (int i=0; i<10; i++){
        printf("mutil_resize i = %d\n",i);
        test_mutil_resize(pre_handle,ive_handle,&frame);
      }
      break;
    case NORMAL:
      // for (int i=0; i<10; i++)
      {
        // printf("normalize i = %d\n",i);
        test_normalize(pre_handle,ive_handle,&frame);
      }
      break;
    default:
      printf("Unknown error.\n");
      break;
  };

  CVI_SYS_FreeI(ive_handle, &image1); 
  CVI_Preprocess_Destroy_Handle(pre_handle);
  CVI_IVE_DestroyHandle(ive_handle);
  CVI_SYS_Exit();
  CVI_VB_Exit();
  return ret;
}
