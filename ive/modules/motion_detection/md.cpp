#include "md.hpp"
#include "ive_log.hpp"
inline void *create_connect_instance() {
  CCLType *p_inst = new CCLType();
  memset(p_inst, 0, sizeof(CCLType));
  return p_inst;
}
inline void init_connected_component(CCLType *ccGst, int width, int height) {
  int superPixMapW, superPixMapH;
  ccGst->maskWidth = width;
  ccGst->maskHeight = height;
  ccGst->superPixMapW = width / CC_SUPER_PIXEL_W;
  ccGst->superPixMapH = height / CC_SUPER_PIXEL_H;
  superPixMapW = ccGst->superPixMapW;
  superPixMapH = ccGst->superPixMapH;
  ccGst->dataSuperPixMap = new unsigned char[superPixMapW * superPixMapH];
  memset(ccGst->dataSuperPixMap, 0, superPixMapH * superPixMapW);
  ccGst->dataSuperPixFG = new unsigned char[superPixMapH * superPixMapW];
  memset(ccGst->dataSuperPixFG, 0, superPixMapH * superPixMapW);
  ccGst->eqCCLabelArray = new unsigned char[CC_MAX_NUM_LABELS + 1];
  ccGst->boundingBoxesTmp = new int[MAX_CC_OBJECTS * 5];
  ccGst->boundingBoxes = new int[MAX_CC_OBJECTS * 5];
}
inline void release_connected_component(CCLType *ccGst) {
  if (ccGst->maskWidth > 0 && ccGst->maskHeight > 0) {
    delete[] ccGst->dataSuperPixMap;
    delete[] ccGst->dataSuperPixFG;
    delete[] ccGst->eqCCLabelArray;
    delete[] ccGst->boundingBoxesTmp;
    delete[] ccGst->boundingBoxes;
  }
  memset(ccGst, 0, sizeof(CCLType));
}
bool cluster_box(int *p_boxes, int ci, int i) {
  // check if intersect
  int inter_R0 = std::max(p_boxes[5 * i + 1], p_boxes[5 * ci + 1]);
  int inter_C0 = std::max(p_boxes[5 * i + 2], p_boxes[5 * ci + 2]);
  int inter_R1 = std::min(p_boxes[5 * i + 3], p_boxes[5 * ci + 3]);
  int inter_C1 = std::min(p_boxes[5 * i + 4], p_boxes[5 * ci + 4]);
  int interw = inter_C1 - inter_C0;
  int interh = inter_R1 - inter_R0;
  if (interw <= 0 || interh <= 0) return false;
  int out_R0 = std::min(p_boxes[5 * i + 1], p_boxes[5 * ci + 1]);
  int out_C0 = std::min(p_boxes[5 * i + 2], p_boxes[5 * ci + 2]);
  int out_R1 = std::max(p_boxes[5 * i + 3], p_boxes[5 * ci + 3]);
  int out_C1 = std::max(p_boxes[5 * i + 4], p_boxes[5 * ci + 4]);
  p_boxes[5 * ci + 1] = out_R0;
  p_boxes[5 * ci + 2] = out_C0;
  p_boxes[5 * ci + 3] = out_R1;
  p_boxes[5 * ci + 4] = out_C1;
  return true;
}
int filter_inside_boxes(int *p_boxes, int num_src) {
  // sort with box area
  struct area_info {
    int area;
    int index;
  };
  std::vector<area_info> box_areas;
  for (int i = 0; i < num_src; i++) {
    int R0 = p_boxes[5 * i + 1];
    int C0 = p_boxes[5 * i + 2];
    int R1 = p_boxes[5 * i + 3];
    int C1 = p_boxes[5 * i + 4];
    int area = (R1 - R0) * (C1 - C0);
    area_info a_info;
    a_info.area = area;
    a_info.index = i;
    box_areas.push_back(a_info);
  }
  std::sort(box_areas.begin(), box_areas.end(),
            [](const area_info &a, const area_info &b) { return a.area > b.area; });
  for (size_t i = 1; i < box_areas.size(); i++) {
    int indi = box_areas[i].index;
    // std::cout<<"indi:"<<indi<<",area:"<<box_areas[i].area<<std::endl;
    for (size_t c = 0; c < i; c++) {
      int indc = box_areas[c].index;
      if (p_boxes[5 * indc] < 0) continue;
      if (cluster_box(p_boxes, indc, indi)) {
        p_boxes[5 * indi] = -1;
        break;
      }
    }
  }
  int ctFinal = 0;
  for (int i = 0; i < num_src; i++) {
    if (p_boxes[5 * i] > 0) {
      memcpy(p_boxes + 5 * ctFinal, p_boxes + 5 * i, 5 * sizeof(int));
      ctFinal++;
    }
  }
  return ctFinal;
}
int dump_ive_image_frame(const std::string &filepath, uint8_t *ptr_img, int w, int h, int wstride) {
  FILE *fp = fopen(filepath.c_str(), "wb");
  if (fp == nullptr) {
    return -1;
  }
  fwrite(&w, sizeof(uint32_t), 1, fp);
  fwrite(&h, sizeof(uint32_t), 1, fp);
  std::cout << "width:" << w << ",height:" << h << ",stride:" << wstride << std::endl;
  fwrite(ptr_img, w * h, 1, fp);
  // for(int j = 0 ; j < h; j++){
  //     uint8_t *ptrj = ptr + j*wstride;
  //     std::cout<<"write r:"<<j<<std::endl;
  //     fwrite(ptrj,w,1,fp);
  // }
  std::cout << "toclose:" << filepath << std::endl;
  fclose(fp);
  std::cout << "closed:" << filepath << std::endl;
  return 0;
}
int *extract_connected_component(unsigned char *p_fg_mask, int width, int height, int wstride,
                                 int area_thresh, void *p_cc_inst, int *p_num_boxes) {
  int r, c, i, j, rBlk, cBlk;
  int imgW, xA, xB, xC, xD;
  int cumulatedBlkSum, superPixMapW, superPixMapH;
  int ctFGPixels, imgDataWStep;
  int lbVal, currLabel, R0, C0, R1, C1, objectSize;
  int tmpMaxLabel;
  unsigned char *ptrImgData;
  unsigned char *ptrImgData0;
  unsigned char *ptrSuperPixMap;
  unsigned char *ptrSuperPixMap0;
  unsigned char *ptrSuperPixFG;
  unsigned char *ptrSuperPixFG0;
  unsigned char *ptrLabelMap;
  unsigned char *ptrLabelMap0;
  unsigned char *eqLabelArray;
  unsigned char *ptrUCHAR;
  int *boundingBoxes;
  int *boundingBoxesFinal;
  int ctFinal;
  CCLType *ccGst = (CCLType *)p_cc_inst;
  if (ccGst->maskWidth != width || ccGst->maskHeight != height) {
    printf("allocate ccl,w:%d,h:%d\n", width, height);
    release_connected_component(ccGst);
    init_connected_component(ccGst, width, height);
    // std::cout<<"to allocate
    // ccl,w:"<<width<<",height:"<<height<<",maskw:"<<ccGst->maskWidth<<std::endl;
  }
  imgW = ccGst->maskWidth;
  superPixMapW = ccGst->superPixMapW;
  superPixMapH = ccGst->superPixMapH;
  ptrSuperPixMap0 = ccGst->dataSuperPixMap;
  ptrSuperPixFG0 = ccGst->dataSuperPixFG;
  eqLabelArray = ccGst->eqCCLabelArray;
  ptrImgData0 = p_fg_mask;
  tmpMaxLabel = 0;
  /* reset foreground super pixels.*/
  memset(ccGst->dataSuperPixMap, 0, superPixMapH * superPixMapW * sizeof(unsigned char));
  memset(ccGst->dataSuperPixFG, 0, superPixMapH * superPixMapW * sizeof(unsigned char));
  ptrUCHAR = ccGst->eqCCLabelArray;
  for (i = 0; i < (CC_MAX_NUM_LABELS + 1); i += 1) {
    *ptrUCHAR = i;
    ptrUCHAR += 1;
  }
  ccGst->numTotalObj = 0;
  /***********************************************/
  /*    Sum up foreground count within suer pixels.*/
  imgDataWStep = (CC_SUPER_PIXEL_H * imgW);
  for (rBlk = 0; rBlk < superPixMapH; rBlk += 1) {
    for (cBlk = 0; cBlk < superPixMapW; cBlk += 1) {
      ptrImgData = ptrImgData0 + (rBlk * imgDataWStep) + (cBlk * CC_SUPER_PIXEL_W);
      cumulatedBlkSum = 0;
      for (r = 0; r < CC_SUPER_PIXEL_H; r += 1) {
        for (c = 0; c < CC_SUPER_PIXEL_W; c += 1) {
          if ((*ptrImgData) > 0) cumulatedBlkSum += 1;
          ptrImgData += 1;
        } /* c */
        ptrImgData += (imgW - CC_SUPER_PIXEL_W);
      } /* r */
      *(ptrSuperPixFG0 + (rBlk * superPixMapW) + cBlk) = cumulatedBlkSum;
    } /*end of: for ( cBlk )*/
  }   /*end of: for ( rBlk )*/
  /**************************/
  /* Scan image for objects */
  /**************************/
  for (r = 0; r < (superPixMapH - CC_SCAN_WINDOW_H); r += 1) {
    for (c = 0; c < (superPixMapW - CC_SCAN_WINDOW_W); c += 1) {
      /* sum up pixel in scanning window.*/
      ctFGPixels = 0;
      ptrSuperPixFG = ptrSuperPixFG0 + (r * superPixMapW) + c;
      for (i = 0; i < CC_SCAN_WINDOW_H; i += 1) {
        for (j = 0; j < CC_SCAN_WINDOW_W; j += 1) {
          ctFGPixels += (*ptrSuperPixFG);
          ptrSuperPixFG += 1;
        } /* col: j */
        ptrSuperPixFG += (superPixMapW - CC_SCAN_WINDOW_W);
      } /* row: i */
      /* mark foreground.*/
      if (ctFGPixels > CC_FG_SUPER_PIX_THD) {
        ptrSuperPixMap = ptrSuperPixMap0 + (r * superPixMapW) + c;
        for (i = 0; i < CC_SCAN_WINDOW_H; i += 1) {
          for (j = 0; j < CC_SCAN_WINDOW_W; j += 1) {
            *(ptrSuperPixMap) = 255;
            ptrSuperPixMap += 1;
          }
          ptrSuperPixMap += (superPixMapW - CC_SCAN_WINDOW_W);
        }
      } /*end of: mark foreground.*/
    } /*end of: for ( c )*/
  }   /*end of: for ( r )*/
  // now the map is saved in ptrSuperPixMap
  /*    Connected component labeling.*/
  currLabel = 1;
  // dump_ive_image_frame("/mnt/data/admin1_data/alios_test/md/fg.bin",ptrSuperPixMap0,ccGst->superPixMapW,ccGst->superPixMapH,ccGst->superPixMapW);
  /* put label in [*ptrSuperPixFG].*/
  memset(ccGst->dataSuperPixFG, 0, superPixMapH * superPixMapW * sizeof(unsigned char));
  ptrLabelMap0 = ccGst->dataSuperPixFG;
  /* the 1st pass for setting label and flooding label.*/
  for (r = 1; r < superPixMapH; r += 1) {
    ptrSuperPixMap = ptrSuperPixMap0 + (r * superPixMapW) + 1;
    ptrLabelMap = ptrLabelMap0 + (r * superPixMapW) + 1;
    for (c = 1; c < superPixMapW; c += 1) {
      if ((*ptrSuperPixMap) > 0) {
        /*    xB    xC    xD    */
        /*    xA    x        */
        xA = *(ptrLabelMap - 1);
        xB = *(ptrLabelMap - 1 - superPixMapW);
        xC = *(ptrLabelMap - superPixMapW);
        xD = *(ptrLabelMap - superPixMapW + 1);
        lbVal = CC_MAX_NUM_LABELS;
        if (xA > 0) lbVal = xA;
        if ((xB > 0) && (xB < lbVal)) lbVal = xB;
        if ((xC > 0) && (xC < lbVal)) lbVal = xC;
        if ((xD > 0) && (xD < lbVal)) lbVal = xD;
        if (lbVal < CC_MAX_NUM_LABELS) {
          /* make connection and flood label.*/
          if (xA > lbVal) {
            *(eqLabelArray + xA) = *(eqLabelArray + lbVal);
          }
          if (xB > lbVal) {
            *(eqLabelArray + xB) = *(eqLabelArray + lbVal);
          }
          if (xC > lbVal) {
            *(eqLabelArray + xC) = *(eqLabelArray + lbVal);
          }
          if (xD > lbVal) {
            *(eqLabelArray + xD) = *(eqLabelArray + lbVal);
          }
        } else {
          /* new label.*/
          lbVal = currLabel;
          if (currLabel <
              (CC_MAX_NUM_LABELS - 1)) {  // the maximum label would be CC_MAX_NUM_LABELS-1
            currLabel += 1;
          }
        }
        /* save label for this pixel.*/
        *(ptrLabelMap) = lbVal;
      } /*end of: if foreground.*/
      ptrSuperPixMap += 1;
      ptrLabelMap += 1;
    } /* col */
  }   /* row */
  /* the 2nd pass to flush with equivalent labels.*/
  for (r = 1; r < superPixMapH; r += 1) {
    ptrLabelMap = ptrLabelMap0 + (r * superPixMapW) + 1;
    for (c = 1; c < superPixMapW; c += 1) {
      lbVal = (*ptrLabelMap);
      if (lbVal > 0) {
        /* equivalent label.*/
        lbVal = *(eqLabelArray + lbVal);
        *ptrLabelMap = lbVal;
        // find the maximal label.
        if (lbVal > tmpMaxLabel) {
          tmpMaxLabel = lbVal;
        }
        // display only:
        *(ccGst->dataSuperPixMap + (r * superPixMapW) + c) = lbVal;
      } /*end of: if foreground.*/
      ptrLabelMap += 1;
    } /* col */
  }   /* row */
  // this would not happen
  if (tmpMaxLabel > CC_MAX_NUM_LABELS) {
    printf("ccl error maxlable:%d\n", tmpMaxLabel);
    *p_num_boxes = 0;
    return ccGst->boundingBoxes;
  }
  ccGst->maxID = tmpMaxLabel;
  if (tmpMaxLabel > 0) {
    tmpMaxLabel = tmpMaxLabel;
  }
  // determine the bounding boxes of each object
  boundingBoxes = ccGst->boundingBoxesTmp;
  for (i = 0; i <= ccGst->maxID; i++) {
    boundingBoxes[5 * i + 0] = 0;
    boundingBoxes[5 * i + 1] = 10000;
    boundingBoxes[5 * i + 2] = 10000;
    boundingBoxes[5 * i + 3] = -10000;
    boundingBoxes[5 * i + 4] = -10000;
  }
  for (r = 1; r < superPixMapH; r += 1) {
    ptrLabelMap = ccGst->dataSuperPixMap + (r * superPixMapW) + 1;
    for (c = 1; c < superPixMapW; c += 1) {
      lbVal = *ptrLabelMap;
      ptrLabelMap++;
      if (lbVal >= 1) {
        if (lbVal > (CC_MAX_NUM_LABELS - 1)) lbVal = CC_MAX_NUM_LABELS - 1;
        lbVal = lbVal - 1;
        boundingBoxes[5 * lbVal] = boundingBoxes[5 * lbVal] + 1;
        if (r < boundingBoxes[5 * lbVal + 1]) boundingBoxes[5 * lbVal + 1] = r;
        if (c < boundingBoxes[5 * lbVal + 2]) boundingBoxes[5 * lbVal + 2] = c;
        if (r > boundingBoxes[5 * lbVal + 3]) boundingBoxes[5 * lbVal + 3] = r;
        if (c > boundingBoxes[5 * lbVal + 4]) boundingBoxes[5 * lbVal + 4] = c;
      }
    }
  }
  // clean up: remove small ones and remove overlap
  boundingBoxes = ccGst->boundingBoxesTmp;
  boundingBoxesFinal = ccGst->boundingBoxes;
  ctFinal = 0;
  // remove overlapped boxes
  for (i = 0; i < tmpMaxLabel; i++) {
    objectSize = boundingBoxes[5 * i + 0];
    if (objectSize > 0) {
      R0 = boundingBoxes[5 * i + 1] * BLOCK_SIZE;
      C0 = boundingBoxes[5 * i + 2] * BLOCK_SIZE;
      R1 = boundingBoxes[5 * i + 3] * BLOCK_SIZE;
      C1 = boundingBoxes[5 * i + 4] * BLOCK_SIZE;
      int area = (C1 - C0) * (R1 - R0);
      if (area < area_thresh) continue;
      boundingBoxesFinal[5 * ctFinal + 0] = objectSize;
      boundingBoxesFinal[5 * ctFinal + 1] = R0;
      boundingBoxesFinal[5 * ctFinal + 2] = C0;
      boundingBoxesFinal[5 * ctFinal + 3] = R1;
      boundingBoxesFinal[5 * ctFinal + 4] = C1;
      ctFinal++;
    }
  }
  ctFinal = filter_inside_boxes(boundingBoxesFinal, ctFinal);
  ccGst->maxID = ctFinal;
  ccGst->numObjects = ctFinal;
  *p_num_boxes = ccGst->numObjects;
  return ccGst->boundingBoxes;
} /*end of: void extract_connected_component() | connected component labeling.*/
inline void destroy_connected_component(void *ccGst) {
  if (ccGst == NULL) return;
  CCLType *p_inst = (CCLType *)ccGst;
  release_connected_component(p_inst);
  delete p_inst;
}
inline CVI_U32 getWidthAlign() { return DEFAULT_ALIGN; }
uint32_t getAlignedWidth(uint32_t width) {
  uint32_t align = getWidthAlign();
  uint32_t stride = (uint32_t)(width / align) * align;
  if (stride < width) {
    stride += align;
  }
  return stride;
}
inline void CVI_MemAllocInit(const uint32_t size, ive_bbox_t_info_t *meta) {
  meta->size = size;
  meta->info = (ive_bbox_t *)malloc(sizeof(ive_bbox_t) * size);
  for (uint32_t i = 0; i < size; ++i) {
    memset(&meta->info[i], 0, sizeof(ive_bbox_t));
    meta->info[i].bbox.x1 = -1;
    meta->info[i].bbox.x2 = -1;
    meta->info[i].bbox.y1 = -1;
    meta->info[i].bbox.y2 = -1;
  }
}
inline std::vector<CVI_U32> getStride(IVE_SRC_IMAGE_S *img) {
  return std::vector<CVI_U32>(std::begin(img->u16Stride), std::end(img->u16Stride));
}
inline std::vector<CVI_U8 *> getVAddr(IVE_SRC_IMAGE_S *img) {
  return std::vector<CVI_U8 *>(std::begin(img->pu8VirAddr), std::end(img->pu8VirAddr));
}
MotionDetection::MotionDetection() {
    handle = CVI_IVE_CreateHandle();
    p_ccl_instance = create_connect_instance();
}
MotionDetection::~MotionDetection() {
  CVI_IVE_DestroyHandle(handle);
  CVI_SYS_FreeI(handle, &md_output);
  CVI_SYS_FreeI(handle, &background_img);
  CVI_SYS_FreeI(handle, &tmp_cpy_img_);
  CVI_SYS_FreeI(handle, &tmp_src_img_);
  destroy_connected_component(p_ccl_instance);
}
int MotionDetection::init(VIDEO_FRAME_INFO_S *init_frame) {
  memset(&background_img, 0 ,sizeof(background_img));
  memset(&md_output, 0 ,sizeof(md_output));
  memset(&tmp_cpy_img_, 0 ,sizeof(tmp_cpy_img_));
  memset(&tmp_src_img_, 0 ,sizeof(tmp_src_img_));
  CVI_S32 ret = construct_images(init_frame);
  if (ret == CVI_SUCCESS) {
    ret = copy_image(init_frame, &background_img);
  }
  p_ccl_instance = create_connect_instance();
  return ret;
}
int MotionDetection::construct_images(VIDEO_FRAME_INFO_S *init_frame) {
  im_width = init_frame->stVFrame.u32Width;
  im_height = init_frame->stVFrame.u32Height;
  uint32_t voWidth = init_frame->stVFrame.u32Width;
  uint32_t voHeight = init_frame->stVFrame.u32Height;
  // only phobos do not need padding because use custom ccl
  memset(&md_output, 0, sizeof(md_output));
  memset(&background_img, 0, sizeof(background_img));
  CVI_IVE_CreateImage(handle, &md_output, IVE_IMAGE_TYPE_U8C1, voWidth,
                      voHeight);
  CVI_IVE_CreateImage(handle, &background_img, IVE_IMAGE_TYPE_U8C1, voWidth,
                      voHeight);
  return CVI_SUCCESS;
}
void MotionDetection::free_all() {
  CVI_SYS_FreeI(handle, &md_output);
  CVI_SYS_FreeI(handle, &background_img);
}
int MotionDetection::copy_image(VIDEO_FRAME_INFO_S *srcframe, IVE_IMAGE_S *dst) {
  if (srcframe->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_400) {
    return CVI_FAILURE;
  }
  int ret = CVI_SUCCESS;
  CVI_IVE_VideoFrameInfo2Image(srcframe, &tmp_cpy_img_);
  //dma tmp_img to dst img
  IVE_DMA_CTRL_S ctrl;
  ctrl.enMode = IVE_DMA_MODE_DIRECT_COPY;
  ctrl.u64Val = 0;
  ctrl.u8HorSegSize = 0;
  ctrl.u8ElemSize = 0;
  ctrl.u8VerSegRows = 0;
  ret = CVI_IVE_DMA(handle, &tmp_cpy_img_, dst, &ctrl, false);
  return ret;
}
int MotionDetection::update_background(VIDEO_FRAME_INFO_S *frame) {
  if (frame->stVFrame.u32Width != background_img.u16Width ||
      frame->stVFrame.u32Height != background_img.u16Height) {
    free_all();
    if (construct_images(frame) != CVI_SUCCESS) {
      return CVI_FAILURE;
    }
  }
  CVI_S32 ret = copy_image(frame, &background_img);
  return ret;
}
/*bbox_info will alloc memory, must remember release*/
int MotionDetection::detect(VIDEO_FRAME_INFO_S *srcframe, ive_bbox_t_info_t *bbox_info, uint8_t threshold, double min_area) {
  if (srcframe->stVFrame.u32Height != im_height || srcframe->stVFrame.u32Width != im_width) {
    LOGE("Height and width of frame isn't equal to background image in MotionDetection\n");
    return CVI_FAILURE;
  }
  memset(bbox_info, 0, sizeof(ive_bbox_t_info_t));
  CVI_S32 ret = CVI_SUCCESS;
  if (srcframe->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_400) {
    return CVI_FAILURE;
  }
  if (srcframe->stVFrame.pu8VirAddr[0] == NULL) {
    LOGE("VIDEO_FRAME need mmap\n");
    return CVI_FAILURE;
  }
  
  ret = CVI_IVE_VideoFrameInfo2Image(srcframe, &tmp_src_img_);
  if (ret != CVI_SUCCESS) {
    LOGE("Convert frame to IVE_IMAGE_S fail %x\n", ret);
    return CVI_FAILURE;
  }
  IVE_SUB_CTRL_S ctrl;
  ctrl.enMode = IVE_SUB_MODE_NORMAL;
  ret |= CVI_IVE_Sub(handle, &tmp_src_img_, &background_img, &md_output, &ctrl, false);
  IVE_THRESH_CTRL_S ctrl_thresh;
  ctrl_thresh.enMode = IVE_THRESH_MODE_BINARY;
  ctrl_thresh.u8MinVal = 0;
  ctrl_thresh.u8MaxVal = 255;
  ctrl_thresh.u8LowThr = 30;
  ret |= CVI_IVE_Thresh(handle, &md_output, &md_output, &ctrl_thresh, false);
  if (ret != CVI_SUCCESS) {
    LOGE("failed to do frame difference ret=%d\n", ret);
    return CVI_FAILURE;
  }
  CVI_IVE_BufRequest(handle, &md_output);
  int num_boxes = 0;
  int wstride = getStride(&md_output)[0];
  void *p_ccl_instance = create_connect_instance();
  int *p_boxes = extract_connected_component(getVAddr(&md_output)[0], im_width, im_height, wstride,
                                             min_area, p_ccl_instance, &num_boxes);
  CVI_MemAllocInit(num_boxes, bbox_info);
  memset(bbox_info->info, 0, sizeof(ive_bbox_t_info_t) * num_boxes);
  for (uint32_t i = 0; i < (uint32_t)num_boxes; ++i) {
    bbox_info->info[i].bbox.x1 = p_boxes[i * 5 + 2];
    bbox_info->info[i].bbox.y1 = p_boxes[i * 5 + 1];
    bbox_info->info[i].bbox.x2 = p_boxes[i * 5 + 4];
    bbox_info->info[i].bbox.y2 = p_boxes[i * 5 + 3];
  }
  return CVI_SUCCESS;
}