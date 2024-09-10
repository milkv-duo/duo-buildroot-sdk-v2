#pragma once
#include "ive.hpp"

class TamperDetectorMD {
 public:
  TamperDetectorMD() = delete;
  // TamperDetectorMD(VIDEO_FRAME_INFO_S *init_frame, float momentum=0.05, int update_interval=10);
  TamperDetectorMD(ive::IVE *ive_instance, VIDEO_FRAME_INFO_S *init_frame, float momentum,
                   int update_interval);
  ~TamperDetectorMD();

  int update(VIDEO_FRAME_INFO_S *frame);
  int detect(VIDEO_FRAME_INFO_S *frame);
  int detect(VIDEO_FRAME_INFO_S *frame, float *moving_score);
  ive::IVEImage &getMean();
  ive::IVEImage &getDiff();
  void printMean();
  void printDiff();
  void free();
  void print_info();

 private:
  ive::IVE *ive_instance = nullptr;
  int nChannels;
  CVI_U32 strideWidth, height, width, area;
  ive::IVEImage mean;
  ive::IVEImage diff;
  float momentum;
  int update_interval;
  int counter;
  // IVE_IMAGE_S MIN_DIFF_M;

  // float alertMovingScore;
  // float alertContourScore;
};