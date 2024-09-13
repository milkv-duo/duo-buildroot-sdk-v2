#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_log.hpp"
#include "cvi_tdl_media.h"
#include "sys_utils.hpp"

bool read_binary_file(const std::string &strf, void *p_buffer, int buffer_len) {
  FILE *fp = fopen(strf.c_str(), "rb");
  if (fp == nullptr) {
    std::cout << "read file failed," << strf << std::endl;
    return false;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (len != buffer_len) {
    std::cout << "size not equal,expect:" << buffer_len << ",has " << len << std::endl;
    return false;
  }
  fread(p_buffer, len, 1, fp);
  fclose(fp);
  return true;
}

bool parse_bin_file(const std::string &bin_path, VIDEO_FRAME_INFO_S *frame,
                    const std::string &dump_path = "/mnt/data/nfsuser_zxh/teaisp_test/a.bin") {
  std::ifstream file(bin_path, std::ios::binary);
  if (!file.is_open()) {
    printf("can not open bin path %s\n", bin_path.c_str());
    return false;
  }
  // printf("open bin path %s success!\n", bin_path.c_str());

  // 读取数组形状信息的 12 个字节
  // int shape[3]; // 假设形状为三维数组
  // file.read(reinterpret_cast<char*>(&shape), sizeof(int) * 3); // 读取 12 字节

  // 输出读取到的形状信息（可选）
  // printf("Shape: (%d, %d, %d)\n", shape[0], shape[1], shape[2]);
  frame->stVFrame.u32Width = 3840;
  frame->stVFrame.u32Height = 1440;
  frame->stVFrame.u32Length[0] = 1440 * 3840;

  // 创建物理地址
  // CVI_U32 u32MapSize = frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] +
  // frame->stVFrame.u32Length[2]; int ret = CVI_SYS_IonAlloc(&frame->stVFrame.u64PhyAddr[0],
  // (CVI_VOID **)&frame->stVFrame.pu8VirAddr[0],
  //                             "cvitdl/image", u32MapSize);
  // if (ret != CVI_SUCCESS) {
  //   syslog(LOG_ERR, "Cannot allocate ion, size: %d, ret=%#x\n", u32MapSize, ret);
  //   return CVI_FAILURE;
  // }

  // 读取数组数据的剩余部分
  file.read(reinterpret_cast<char *>(frame->stVFrame.pu8VirAddr[0]), frame->stVFrame.u32Length[0]);
  // print 10
  // for (int i = 0; i < 10; i++) {
  //   printf("%d ", frame->stVFrame.pu8VirAddr[0][i]);
  // }
  // float qscale = 126.504;
  // for (int i = 0; i < frame->stVFrame.u32Length[0]; i++) {
  //   int val = frame->stVFrame.pu8VirAddr[0][i]*qscale/255;
  //   if(val>=127){
  //     val = 127;
  //   }
  //   frame->stVFrame.pu8VirAddr[0][i] = val;
  // }
  // printf("\n");
  // for (int i = 0; i < 10; i++) {
  //   printf("%d ", frame->stVFrame.pu8VirAddr[0][i]);
  // }

  // ret =
  // CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0],frame->stVFrame.pu8VirAddr[0],u32MapSize);
  // if (ret != CVI_SUCCESS) {
  //   syslog(LOG_ERR, "Cannot flush ion, size: %d, ret=%#x\n", u32MapSize, ret);
  //   return CVI_FAILURE;
  // }

  // dump_frame_result(dump_path, frame);

  file.close();
  return true;
}

void bench_mark_all(std::string bench_path, std::string image_root, std::string res_path,
                    cvitdl_handle_t tdl_handle, VIDEO_FRAME_INFO_S &fdFrame) {
  std::fstream file(bench_path);
  if (!file.is_open()) {
    printf("can not open bench path %s\n", bench_path.c_str());
    return;
  }
  printf("open bench path %s success!\n", bench_path.c_str());
  std::string line;
  std::stringstream res_ss;
  int cnt = 0;

  //   imgprocess_t img_handle;
  //   CVI_TDL_Create_ImageProcessor(&img_handle);

  while (getline(file, line)) {
    if (!line.empty()) {
      stringstream ss(line);
      std::string image_name;
      while (ss >> image_name) {
        cvtdl_class_meta_t meta = {0};
        // VIDEO_FRAME_INFO_S fdFrame;
        // memset(&fdFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        // fdFrame.stVFrame.pu8VirAddr[0] = p_buffer;
        // 创建物理地址
        // CVI_U32 u32MapSize = fdFrame.stVFrame.u32Length[0] + fdFrame.stVFrame.u32Length[1] +
        // fdFrame.stVFrame.u32Length[2];

        if (++cnt % 100 == 0) {
          cout << "processing idx: " << cnt << endl;
        }

        auto name = image_root + image_name;
        // CVI_S32 ret =
        //     CVI_TDL_ReadImage(img_handle, name.c_str(), &fdFrame, BAYER_FORMAT_BG);
        printf("read file %s\n", name.c_str());
        if (!parse_bin_file(name, &fdFrame)) {
          printf("read file failed\n");
          return;
        }
        CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RAW_IMAGE_CLASSIFICATION,
                                      true);
        // printf("run inference for %s\n",image_name.c_str());
        int ret = CVI_TDL_Raw_Image_Classification(tdl_handle, &fdFrame, &meta);
        if (ret != CVI_SUCCESS) {
          CVI_TDL_Free(&meta);
          // CVI_TDL_ReleaseImage(img_handle, &fdFrame);
          printf("infer failed\n");
          break;
        }

        res_ss << image_name << " ";
        for (int i = 0; i < 5; i++) {
          res_ss << " " << meta.cls[i];
        }
        res_ss << "\n";

        CVI_TDL_Free(&meta);
        // CVI_TDL_ReleaseImage(img_handle, &fdFrame);
        break;
      }
    }
  }
  // CVI_TDL_Destroy_ImageProcessor(img_handle);
  std::cout << "write results to file: " << res_path << std::endl;
  FILE *fp = fopen(res_path.c_str(), "w");
  fwrite(res_ss.str().c_str(), res_ss.str().size(), 1, fp);
  fclose(fp);
  std::cout << "write done!" << std::endl;
}

int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;

  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  std::string model_path = argv[1];
  std::string bench_path = argv[2];
  std::string image_root = argv[3];
  std::string res_path = argv[4];

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_RAW_IMAGE_CLASSIFICATION,
                          model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x %s!\n", ret, model_path.c_str());
    return ret;
  }
  std::cout << "model opened:" << model_path << std::endl;

  CVI_U8 buffer[1440 * 3840];

  VIDEO_FRAME_INFO_S fdFrame;
  CVI_U32 u32MapSize = 1440 * 3840;
  printf("try to allocate ion, size: %d\n", u32MapSize);
  ret = CVI_SYS_IonAlloc(&fdFrame.stVFrame.u64PhyAddr[0],
                         (CVI_VOID **)&fdFrame.stVFrame.pu8VirAddr[0], "cvitdl/image", u32MapSize);
  if (ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "Cannot allocate ion, size: %d, ret=%#x\n", u32MapSize, ret);
    printf("allocate ion failed\n");
    return CVI_FAILURE;
  }

  bench_mark_all(bench_path, image_root, res_path, tdl_handle, fdFrame);

  CVI_TDL_DestroyHandle(tdl_handle);

  ret = CVI_SYS_IonFree(fdFrame.stVFrame.u64PhyAddr[0], (CVI_VOID *)fdFrame.stVFrame.pu8VirAddr[0]);
  if (ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "Cannot free ion, size: %d, ret=%#x\n", u32MapSize, ret);
    printf("free ion failed\n");
    return CVI_FAILURE;
  }

  return ret;
}
