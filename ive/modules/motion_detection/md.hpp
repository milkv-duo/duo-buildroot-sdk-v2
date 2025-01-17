#ifndef _CVI_IVE_MD_H_
#define _CVI_IVE_MD_H_
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "ive.h"
#include "cvi_module_core.h"
/* ccl */
#define CC_MAX_NUM_LABELS 200
#define MAX_CC_OBJECTS 200
#define CC_SUPER_PIXEL_H 2
#define CC_SUPER_PIXEL_W 2
#define CC_SCAN_WINDOW_H 2
#define CC_SCAN_WINDOW_W 2
#define CC_FG_SUPER_PIX_THD 3
#define BLOCK_SIZE 2
// for cv182x align
#define DEFAULT_ALIGN 64
typedef struct CCTag {
  int maskWidth;
  int maskHeight;
  int ccSuperPixelSize;
  int superPixMapW;
  int superPixMapH;
  int numTotalObj;
  int maxID;
  unsigned char *dataSuperPixMap;
  unsigned char *dataSuperPixFG;
  unsigned char *eqCCLabelArray;
  int numObjects;
  int *boundingBoxesTmp;
  int *boundingBoxes;
} CCLType;
struct Padding {
  uint32_t left;
  uint32_t top;
  uint32_t right;
  uint32_t bottom;
};
class MotionDetection {
 public:
  MotionDetection();
  ~MotionDetection();
  int init(VIDEO_FRAME_INFO_S *init_frame);
  int detect(VIDEO_FRAME_INFO_S *frame, ive_bbox_t_info_t *bbox_info, uint8_t threshold,
                 double min_area);
  int update_background(VIDEO_FRAME_INFO_S *frame);
 private:
  int construct_images(VIDEO_FRAME_INFO_S *init_frame);
  void free_all();
  int copy_image(VIDEO_FRAME_INFO_S *srcframe, IVE_IMAGE_S *dst);
  IVE_HANDLE handle;
  void *p_ccl_instance = NULL;
  IVE_IMAGE_S background_img;
  IVE_IMAGE_S md_output;
  IVE_IMAGE_S tmp_cpy_img_;
  IVE_IMAGE_S tmp_src_img_;
  uint32_t im_width;
  uint32_t im_height;
  Padding m_padding;
};
#endif
