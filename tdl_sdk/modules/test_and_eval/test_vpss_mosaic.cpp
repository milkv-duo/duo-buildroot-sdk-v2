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

typedef struct _RGN_TEST_PARAM {
  // normal rgn
  CVI_S32 u32HdlNum;
  RGN_TYPE_E enType;

  // compressed rgn
  CVI_U32 u32OdecFileSize;
  SIZE_S stOdecSize;

  // vpss settings
  MMF_CHN_S stChn;
  SIZE_S stInputSize;
  SIZE_S stOutputSize;
  PIXEL_FORMAT_E eInputFmt;
  PIXEL_FORMAT_E eOutputFmt;

  // input/output file settings
  CVI_CHAR *inputFile;
  CVI_CHAR *outputFile;
  CVI_U32 u32RepeatCnt;
} RGN_TEST_PARAM;

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

CVI_S32 SAMPLE_COMM_REGION_AttachToChnSelf(CVI_S32 HandleNum, RGN_TYPE_E enType,
                                           MMF_CHN_S *pstChn) {
  CVI_S32 i;
  CVI_S32 s32Ret = CVI_SUCCESS;
  CVI_S32 MinHadle;
  uint32_t MosaicMinHandle = 80;
  RGN_CHN_ATTR_S stChnAttr;

  if (HandleNum <= 0 || HandleNum > 16) {
    return CVI_FAILURE;
  }
  if (enType < OVERLAY_RGN || enType >= RGN_BUTT) {
    return CVI_FAILURE;
  }
  if (pstChn == CVI_NULL) {
    return CVI_FAILURE;
  }
  memset(&stChnAttr, 0, sizeof(stChnAttr));

  /*set the chn config*/
  stChnAttr.bShow = CVI_TRUE;
  MinHadle = MosaicMinHandle;
  stChnAttr.enType = MOSAIC_RGN;
  stChnAttr.unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_8;
  stChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 80;  // 8 pixel align
  stChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 80;
  /*attach to Chn*/

  for (i = MinHadle; i < MinHadle + HandleNum; i++) {
    if (enType == MOSAIC_RGN) {
      stChnAttr.unChnAttr.stMosaicChn.stRect.s32X = 20 + 200 * (i - MosaicMinHandle);
      stChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = 20 + 200 * (i - MosaicMinHandle);
      stChnAttr.unChnAttr.stMosaicChn.u32Layer = i - MosaicMinHandle;
    }
    s32Ret = CVI_RGN_AttachToChn(i, pstChn, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
      break;
    }
  }
  /*detach region from chn */
  if (s32Ret != CVI_SUCCESS && i > 0) {
    i--;
    for (; i >= MinHadle; i--) s32Ret = CVI_RGN_DetachFromChn(i, pstChn);
  }
  return s32Ret;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("need 3 arg, eg ./test_vpss_pd xxxx.cvimodel xxx.jpg vehicle\n");
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
  CVI_MAPI_Media_Init(image.cols, image.rows, 3);
  // init vpss
  PreprocessArg arg;
  arg.width = 1920;
  arg.height = 1080;
  arg.vpssConfig = vpssConfig;
  init_vpss(image.cols, image.rows, &arg);

  VIDEO_FRAME_INFO_S frame_in;
  VIDEO_FRAME_INFO_S frame_preprocessed;
  memset(&frame_in, 0x00, sizeof(frame_in));
  memset(&frame_preprocessed, 0x00, sizeof(frame_preprocessed));

  uint32_t u32HdlNum = 3;
  RGN_TYPE_E enType = MOSAIC_RGN;
  PIXEL_FORMAT_E pixel_type = PIXEL_FORMAT_RGB_888;
  uint32_t s32Ret = 0;
  RGN_HANDLE Handle;
  RGN_CHN_ATTR_S stRgnChnAttr;
  MMF_CHN_S stChn;

  stChn.enModId = CVI_ID_VPSS;
  stChn.s32DevId = 0;          // grp
  stChn.s32ChnId = VPSS_CHN0;  // chn

  s32Ret = SAMPLE_COMM_REGION_Create(u32HdlNum, enType, pixel_type);
  if (s32Ret != CVI_SUCCESS) {
    printf("SAMPLE_COMM_REGION_Create failed!\n");
  }

  s32Ret = SAMPLE_COMM_REGION_AttachToChnSelf(u32HdlNum, enType, &stChn);
  if (s32Ret != CVI_SUCCESS) {
    printf("SAMPLE_COMM_REGION_AttachToChn failed!\n");
  }

  get_frame_from_mat(frame_in, image);

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

  /* dump_frame_result, support rgb, nv21, nv12*/
  dump_frame_result("test2.yuv", &frame_preprocessed);

  SAMPLE_COMM_REGION_DetachFrmChn(u32HdlNum, enType, &stChn);
  SAMPLE_COMM_REGION_Destroy(u32HdlNum, enType);

  CVI_MAPI_ReleaseFrame(&frame_in);
  if (CVI_SUCCESS != CVI_VPSS_ReleaseChnFrame(0, 0, &frame_preprocessed)) {
    printf("release frame failed!\n");
    return -1;
  }
  CVI_MAPI_Media_Deinit();
  return ret;
}
