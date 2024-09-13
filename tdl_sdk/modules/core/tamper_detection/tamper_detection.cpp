#include <stdio.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_log.hpp"
#include "ive.hpp"

#include "tamper_detection.hpp"

using namespace ive;
#define INIT_DIFF 12
#define MIN_DIFF 8

#define DEBUG (0)
#define DEBUG_PRINT_CHANNELS_NUM 1
#define STATIC_CAST_UINT(d) static_cast<uint>(d)

#if DEGUB
void print_IVE_IMAGE_S(IVE_IMAGE_S &ive_image, int c = 3) {
  int nChannels = c;
  int strideWidth = ive_image.u16Stride[0];
  int height = ive_image.u16Height;
  int width = ive_image.u16Width;
  if (width > 64) {
    std::cout << "width " << width << " is too long" << std::endl;
    return;
  }

  for (int cc = 0; cc < nChannels; cc++) {
    for (int ii = 0; ii < height; ii++) {
      for (int jj = 0; jj < width; jj++) {
        std::cout << std::setw(5)
                  << STATIC_CAST_UINT(ive_image.pu8VirAddr[cc][ii * strideWidth + jj]) << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << "-------------------------------------" << std::endl;
  }
}
#endif

#define RETURN_IF_FAILED(x, errmsg) \
  do {                              \
    if ((x) != CVI_TDL_SUCCESS) {   \
      LOGE(errmsg "\n");            \
      return CVI_TDL_ERR_INFERENCE; \
    }                               \
  } while (0)

TamperDetectorMD::TamperDetectorMD(IVE *_ive_instance, VIDEO_FRAME_INFO_S *init_frame,
                                   float momentum, int update_interval) {
  this->ive_instance = _ive_instance;
  this->nChannels = 3;

  IVEImage new_frame;
  CVI_S32 ret = new_frame.fromFrame(init_frame);

  if (ret != CVI_SUCCESS) {
    LOGE("Convert to video frame failed with %#x!\n", ret);
  }
  this->strideWidth = init_frame->stVFrame.u32Stride[0];
  this->height = init_frame->stVFrame.u32Height;
  this->width = init_frame->stVFrame.u32Width;

  mean.create(ive_instance, ImageType::U8C3_PLANAR, width, height);
  ive_instance->dma(&new_frame, &mean);

  diff.create(ive_instance, ImageType::U8C3_PLANAR, width, height);
  ive_instance->fillConst(&diff, INIT_DIFF);

  // this->alertMovingScore = alertMovingScore;
  this->momentum = momentum;
  this->update_interval = update_interval;
  this->counter = 1;

#if DEBUG
  std::cout << "==== this->mean ====" << std::endl;
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);
  std::cout << "==== this->diff ====" << std::endl;
  print_IVE_IMAGE_S(this->diff, DEBUG_PRINT_CHANNELS_NUM);

  // this->update_interval = 1;
#endif
}

TamperDetectorMD::~TamperDetectorMD() { free(); }

int TamperDetectorMD::update(VIDEO_FRAME_INFO_S *frame) {
  IVEImage new_frame;
  RETURN_IF_FAILED(new_frame.fromFrame(frame), "Convert to video frame failed");

#if DEBUG
  std::cout << "==== [update] input frame ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &new_frame);
  print_IVE_IMAGE_S(new_frame, DEBUG_PRINT_CHANNELS_NUM);
#endif
#if DEBUG
  std::cout << "==== this->mean ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &this->mean);
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);
#endif

  IVEImage frame_diff_1, frame_diff_2;
  IVEImage min_diff;

  frame_diff_1.create(ive_instance, U8C3_PLANAR, width, height);
  frame_diff_2.create(ive_instance, U8C3_PLANAR, width, height);
  min_diff.create(ive_instance, U8C3_PLANAR, width, height);

  ive_instance->fillConst(&min_diff, MIN_DIFF);

  RETURN_IF_FAILED(ive_instance->sub(&new_frame, &mean, &frame_diff_1), "error: sub");

#if DEBUG
  std::cout << "==== [update] frame diff (1) ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff_1);
  print_IVE_IMAGE_S(frame_diff_1, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->add(&frame_diff_1, &frame_diff_1, &frame_diff_1, 1.0f, 1.0f),
                   "error: add");

#if DEBUG
  std::cout << "==== [update] frame diff (2) ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff_1);
  print_IVE_IMAGE_S(frame_diff_1, DEBUG_PRINT_CHANNELS_NUM);
#endif

#if DEBUG
  std::cout << "==== momentum ====" << std::endl
            << "aX = " << iveAddCtrl.aX << std::endl
            << "bY = " << iveAddCtrl.bY << std::endl;
  std::cout << "==== [update] (Y) frame ====" << std::endl;
  print_IVE_IMAGE_S(new_frame, DEBUG_PRINT_CHANNELS_NUM);
  std::cout << "==== [update] (X) old mean ====" << std::endl;
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);
#endif
  // Update mean

  RETURN_IF_FAILED(
      ive_instance->add(&mean, &new_frame, &mean, 1.0 - this->momentum, this->momentum),
      "error: add\n");

#if DEBUG
  std::cout << "==== [update] new mean ====" << std::endl;
  CVI_IVE_BufRequest(this->ive_handle, &this->mean);
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);

  std::cout << "==== momentum ====" << std::endl
            << "aX = " << iveAddCtrl.aX << std::endl
            << "bY = " << iveAddCtrl.bY << std::endl;
  std::cout << "==== [update] (Y) diff ====" << std::endl;
  print_IVE_IMAGE_S(frame_diff_1, DEBUG_PRINT_CHANNELS_NUM);
  std::cout << "==== [update] (X) old diff ====" << std::endl;
  print_IVE_IMAGE_S(this->diff, DEBUG_PRINT_CHANNELS_NUM);
#endif
  // Update diff
  RETURN_IF_FAILED(
      ive_instance->add(&diff, &frame_diff_1, &diff, 1.0 - this->momentum, this->momentum),
      "error: add");

#if DEBUG
  std::cout << "==== [update] new diff ====" << std::endl;
  CVI_IVE_BufRequest(this->ive_handle, &this->diff);
  print_IVE_IMAGE_S(this->diff, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->sub(&diff, &min_diff, &frame_diff_1, SubMode::NORMAL),
                   "error: sub");

#if DEBUG
  std::cout << "==== [update] min diff ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &min_diff);
  print_IVE_IMAGE_S(min_diff, DEBUG_PRINT_CHANNELS_NUM);
  std::cout << "==== [update] diff sub (move) ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff_1);
  print_IVE_IMAGE_S(frame_diff_1, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(
      ive_instance->thresh(&frame_diff_1, &frame_diff_1, ThreshMode::BINARY, 0, 0, 0, 0, 255),
      "error: thresh");

#if DEBUG
  std::cout << "==== [update] diff sub (move) (thr) ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff_1);
  print_IVE_IMAGE_S(frame_diff_1, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->fillConst(&frame_diff_2, 255), "error: fill");

  RETURN_IF_FAILED(ive_instance->sub(&frame_diff_2, &frame_diff_1, &frame_diff_2, SubMode::NORMAL),
                   "error: sub");

#if DEBUG
  std::cout << "==== [update] frame_diff_2 ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff_2);
  print_IVE_IMAGE_S(frame_diff_2, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->andImage(&this->diff, &frame_diff_1, &frame_diff_1), "error: and");
  RETURN_IF_FAILED(ive_instance->andImage(&min_diff, &frame_diff_2, &frame_diff_2), "error: and");
  RETURN_IF_FAILED(ive_instance->orImage(&frame_diff_1, &frame_diff_2, &diff), "error: or");

#if DEBUG
  std::cout << "==== [update] final diff ====" << std::endl;
  CVI_IVE_BufRequest(this->ive_handle, &this->diff);
  print_IVE_IMAGE_S(this->diff, DEBUG_PRINT_CHANNELS_NUM);
#endif

  new_frame.free();
  frame_diff_1.free();
  frame_diff_2.free();
  min_diff.free();
  this->counter = 1;
  return CVI_TDL_SUCCESS;
}

int TamperDetectorMD::detect(VIDEO_FRAME_INFO_S *frame, float *moving_score) {
#if DEBUG
  std::cout << "==== [detect] this->mean ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &this->mean);
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);
#endif

  IVEImage new_frame;
  RETURN_IF_FAILED(new_frame.fromFrame(frame), "Convert to video frame failed");

#if DEBUG
  std::cout << "==== [detect] input frame ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &new_frame);
  print_IVE_IMAGE_S(new_frame, DEBUG_PRINT_CHANNELS_NUM);
#endif
#if DEBUG
  std::cout << "==== [detect] this->mean ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &this->mean);
  print_IVE_IMAGE_S(this->mean, DEBUG_PRINT_CHANNELS_NUM);
#endif

  IVEImage frame_diff, frame_move;
  frame_diff.create(ive_instance, ImageType::U8C3_PLANAR, width, height);
  frame_move.create(ive_instance, ImageType::U8C3_PLANAR, width, height);

  // Run IVE sub.
  RETURN_IF_FAILED(ive_instance->sub(&new_frame, &this->mean, &frame_diff, SubMode::ABS),
                   "error: sub");

#if DEBUG
  std::cout << "==== [detect] frame_diff ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff);
  print_IVE_IMAGE_S(frame_diff, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->sub(&frame_diff, &this->diff, &frame_move, SubMode::NORMAL),
                   "error: sub");
#if DEBUG
  std::cout << "==== [detect] move ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_move);
  print_IVE_IMAGE_S(frame_move, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(
      ive_instance->thresh(&frame_move, &frame_move, ThreshMode::BINARY, 0, 0, 0, 0, 255),
      "error: thresh");

#if DEBUG
  std::cout << "==== [detect] move (thr) ====" << std::endl;
  CVI_IVE_BufRequest(this->ive_handle, &frame_move);
  print_IVE_IMAGE_S(frame_move, DEBUG_PRINT_CHANNELS_NUM);
#endif

  RETURN_IF_FAILED(ive_instance->andImage(&frame_diff, &frame_move, &frame_diff), "error: and");
#if DEBUG
  std::cout << "==== [detect] frame_diff (move) ====" << std::endl;
  CVI_IVE_BufFlush(this->ive_handle, &frame_diff);
  print_IVE_IMAGE_S(frame_diff, DEBUG_PRINT_CHANNELS_NUM);
#endif

  // Refresh CPU cache before CPU use.
  frame_diff.bufRequest(ive_instance);

  std::vector<CVI_U8 *> vaddr = frame_diff.getVAddr();
  int diff_sum = 0;
  for (uint32_t c = 0; c < 3; c++) {
    for (uint32_t i = 0; i < height; i++) {
      for (uint32_t j = 0; j < width; j++) {
        diff_sum += vaddr[c][i * strideWidth + j];
      }
    }
  }
#if DEBUG
  std::cout << "diff_sum = " << diff_sum << std::endl;
  std::cout << "height * width = " << height << " * " << width << " = " << height * width
            << std::endl;
  std::cout << "moving_score = " << diff_sum / (height * width) << std::endl;
#endif
  *moving_score = static_cast<float>(diff_sum) / (height * width);

  if (this->update_interval > 0 && this->counter % this->update_interval == 0) {
    this->update(frame);
    this->counter = 1;
  } else {
    this->counter = (this->counter + 1) % this->update_interval;
  }

  new_frame.free();
  frame_diff.free();
  frame_move.free();

  return CVI_TDL_SUCCESS;
}

int TamperDetectorMD::detect(VIDEO_FRAME_INFO_S *frame) {
  // float moving_score, contour_score;
  float moving_score;
  return this->detect(frame, &moving_score);
}

IVEImage &TamperDetectorMD::getMean() { return this->mean; }

IVEImage &TamperDetectorMD::getDiff() { return this->diff; }

void TamperDetectorMD::free() {
  mean.free();
  diff.free();
}

void TamperDetectorMD::print_info() {
  std::cout << "TamperDetectorMD.nChannels   = " << this->nChannels << std::endl
            << "TamperDetectorMD.strideWidth = " << this->strideWidth << std::endl
            << "TamperDetectorMD.height      = " << this->height << std::endl
            << "TamperDetectorMD.width       = " << this->width << std::endl
            << "TamperDetectorMD.momentum        = " << this->momentum << std::endl
            << "TamperDetectorMD.update_interval = " << this->update_interval << std::endl;
}
