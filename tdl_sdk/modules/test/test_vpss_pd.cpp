
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
#include "mapi.hpp"

std::string g_model_root;
cvtdl_bbox_t box;
cvtdl_vpssconfig_t vpssConfig;

int dump_frame_result(const std::string &filepath, VIDEO_FRAME_INFO_S *frame) {
  FILE *fp = fopen(filepath.c_str(), "wb");
  if (fp == nullptr) {
    printf("failed to open: %s.\n", filepath.c_str());
    return CVI_FAILURE;
  }

  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    size_t image_size =
        frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
  }
  for (int c = 0; c < 3; c++) {
    uint8_t *paddr = (uint8_t *)frame->stVFrame.pu8VirAddr[c];
    std::cout << "towrite channel:" << c << ",towritelen:" << frame->stVFrame.u32Length[c]
              << ",addr:" << (void *)paddr << std::endl;
    fwrite(paddr, frame->stVFrame.u32Length[c], 1, fp);
  }
  fclose(fp);
  return CVI_SUCCESS;
}

std::string run_image_person_detection(VIDEO_FRAME_INFO_S *p_frame, cvitdl_handle_t tdl_handle) {
  CVI_S32 ret;
  cvtdl_object_t person_obj;
  memset(&person_obj, 0, sizeof(cvtdl_object_t));
  ret = CVI_TDL_Detection(tdl_handle, p_frame, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                          &person_obj);
  if (ret != CVI_SUCCESS) {
    std::cout << "detect face failed:" << ret << std::endl;
  }

  // generate detection result
  std::stringstream ss;
  for (uint32_t i = 0; i < person_obj.size; i++) {
    box = person_obj.info[i].bbox;
    ss << (person_obj.info[i].classes + 1) << " " << box.score << " " << box.x1 << " " << box.y1
       << " " << box.x2 << " " << box.y2 << "\n";
  }

  CVI_TDL_Free(&person_obj);
  return ss.str();
}

static void get_frame_from_mat(VIDEO_FRAME_INFO_S &in_frame, const cv::Mat &mat) {
  CVI_MAPI_AllocateFrame(&in_frame, mat.cols, mat.rows, PIXEL_FORMAT_BGR_888);
  CVI_MAPI_FrameMmap(&in_frame, true);
  uint8_t *src_ptr = mat.data;
  uint8_t *dst_ptr = in_frame.stVFrame.pu8VirAddr[0];
  for (int h = 0; h < mat.rows; ++h) {
    memcpy(dst_ptr, src_ptr, mat.cols * mat.elemSize());
    src_ptr += mat.step[0];
    dst_ptr += in_frame.stVFrame.u32Stride[0];
  }
  CVI_MAPI_FrameFlushCache(&in_frame);
  CVI_MAPI_FrameMunmap(&in_frame);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("need 3 arg, eg ./test_vpss_pd xxxx.cvimodel xxx.jpg person\n");
    return CVI_TDL_FAILURE;
  }

  CVI_S32 ret = 0;
  g_model_root = std::string(argv[1]);
  std::string image_root(argv[2]);
  std::string process_flag(argv[3]);

  // imread
  cv::Mat image;
  image = cv::imread(image_root);
  if (!image.data) {
    printf("Could not open or find the image\n");
    return -1;
  }

  // init vb
  CVI_MAPI_Media_Init(image.cols, image.rows, 2);

  // init vpss
  PreprocessArg arg;
  arg.width = 448;
  arg.height = 256;
  arg.vpssConfig = vpssConfig;
  init_vpss(image.cols, image.rows, &arg);

  VIDEO_FRAME_INFO_S frame_in;
  VIDEO_FRAME_INFO_S frame_preprocessed;
  memset(&frame_in, 0x00, sizeof(frame_in));
  memset(&frame_preprocessed, 0x00, sizeof(frame_preprocessed));

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }
  std::map<std::string, std::function<std::string(VIDEO_FRAME_INFO_S *, cvitdl_handle_t)>>
      process_funcs = {{"person", run_image_person_detection}};
  if (process_funcs.count(process_flag) == 0) {
    std::cout << "error flag:" << process_flag << std::endl;
    return -1;
  }

  // model need set pixel format PIXEL_FORMAT_BGR_888_PLANAR
  static int model_init = 0;
  if (model_init == 0) {
    std::cout << "to init Person model" << std::endl;
    std::string str_person_model = g_model_root;
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                            str_person_model.c_str());
    if (ret != CVI_SUCCESS) {
      std::cout << "open model failed:" << str_person_model << std::endl;
      return -1;
    }
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, 0.6);
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN, true);
    model_init = 1;
  }

  get_frame_from_mat(frame_in, image);

  memset(&vpssConfig, 0, sizeof(vpssConfig));
  ret = CVI_TDL_GetVpssChnConfig(tdl_handle, CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN,
                                 frame_in.stVFrame.u32Width, frame_in.stVFrame.u32Height, 0,
                                 &vpssConfig);
  if (ret != CVI_SUCCESS) {
    printf("CVI_TDL_GetVpssChnConfig failed!\n");
    return -1;
  }
  CVI_VPSS_SetChnAttr(0, 0, &vpssConfig.chn_attr);

  /* dump_frame_result, support rgb, nv21, nv12*/
  // dump_frame_result("test1.yuv", &frame_in);
  if (CVI_SUCCESS != CVI_VPSS_SendFrame(0, &frame_in, -1)) {
    printf("send frame failed\n");
    return -1;
  }

  if (CVI_SUCCESS != CVI_VPSS_GetChnFrame(0, 0, &frame_preprocessed, 1000)) {
    printf("get frame failed\n");
    return -1;
  }

  CVI_MAPI_ReleaseFrame(&frame_in);
  /* dump_frame_result, support rgb, nv21, nv12*/
  // dump_frame_result("test2.yuv", &frame_preprocessed);
  std::string str_res = process_funcs[process_flag](&frame_preprocessed, tdl_handle);
  if (str_res.size() > 0) {
    FILE *fp = fopen("test.txt", "w");
    fwrite(str_res.c_str(), str_res.size(), 1, fp);
    fclose(fp);
  }

  if (CVI_SUCCESS != CVI_VPSS_ReleaseChnFrame(0, 0, &frame_preprocessed)) {
    printf("release frame failed!\n");
    return -1;
  }

  CVI_TDL_DestroyHandle(tdl_handle);
  return ret;
}
