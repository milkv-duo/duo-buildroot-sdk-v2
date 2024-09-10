#include "bmkernel/bm_kernel.h"
#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file name> \n", argv[0]);
    return CVI_FAILURE;
  }
  const char *filename = argv[1];
  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information. CVI_IVE_ReadImage will do the flush for you.
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, filename, IVE_IMAGE_TYPE_U8C1);
  // int nChannels = 1;
  int width = src.u32Width;
  int height = src.u32Height;

  // These are image buffers for HOG.
  // dstH, dstV are the gradient results of the src image.
  // dstMag, dstAng are the results of the magnitude and angle of the src image.
  IVE_DST_IMAGE_S dstH, dstV;
  CVI_IVE_CreateImage(handle, &dstV, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstH, IVE_IMAGE_TYPE_BF16C1, width, height);
  IVE_DST_IMAGE_S dstMag, dstAng;
  CVI_IVE_CreateImage(handle, &dstMag, IVE_IMAGE_TYPE_BF16C1, width, height);
  CVI_IVE_CreateImage(handle, &dstAng, IVE_IMAGE_TYPE_BF16C1, width, height);

  // The unit of the cell is in pixel. However, our block size unit is "cell", not pixel.
  // The unit of the blkStepX and blkStepY is also "cell".
  const CVI_U32 binNum = 9;
  const CVI_U32 cellSize = 8;
  const CVI_U32 blkSizeInCell = 2;
  const CVI_U32 blkStepX = 1;
  const CVI_U32 blkStepY = 1;

  // Setup HOG parameters
  IVE_HOG_CTRL_S pstHogCtrl;
  pstHogCtrl.u8BinSize = binNum;
  pstHogCtrl.u32CellSize = cellSize;
  pstHogCtrl.u16BlkSizeInCell = blkSizeInCell;
  pstHogCtrl.u16BlkStepX = blkStepX;
  pstHogCtrl.u16BlkStepY = blkStepY;

  // Create space for HOG result.
  IVE_DST_MEM_INFO_S dstHist;
  // CVI_IVE_GET_HOG_SIZE is a helper function that returns the histogram size in byte with given
  // HOG setup.
  CVI_U32 dstHistByteSize = 0;
  CVI_IVE_GET_HOG_SIZE(dstAng.u32Width, dstAng.u32Height, binNum, cellSize, blkSizeInCell, blkStepX,
                       blkStepY, &dstHistByteSize);
  CVI_IVE_CreateMemInfo(handle, &dstHist, dstHistByteSize);

  printf("Run TPU HOG.\n");
  int ret = CVI_IVE_HOG(handle, &src, &dstH, &dstV, &dstMag, &dstAng, &dstHist, &pstHogCtrl, 0);

  // Since the output buffer of HOG is in BF16C1, you can convert the image to U8C1 with
  // CVI_IVE_ImageTypeConvert.
  IVE_DST_IMAGE_S dstH_u8, dstV_u8, dstMag_u8;
  CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);
  CVI_IVE_CreateImage(handle, &dstMag_u8, IVE_IMAGE_TYPE_U8C1, width, height);

  printf("Normalize result to 0-255.\n");
  IVE_ITC_CRTL_S iveItcCtrl;
  iveItcCtrl.enType = IVE_ITC_NORMALIZE;
  CVI_IVE_ImageTypeConvert(handle, &dstV, &dstV_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstH, &dstH_u8, &iveItcCtrl, 0);
  CVI_IVE_ImageTypeConvert(handle, &dstMag, &dstMag_u8, &iveItcCtrl, 0);

  // Refresh CPU cache before CPU use.
  CVI_IVE_BufRequest(handle, &dstH);
  CVI_IVE_BufRequest(handle, &dstV);
  CVI_IVE_BufRequest(handle, &dstMag);

  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "sample_hogDstV.png", &dstV_u8);
  CVI_IVE_WriteImage(handle, "sample_hogDstH.png", &dstH_u8);
  CVI_IVE_WriteImage(handle, "sample_hogMag.png", &dstMag_u8);

  // To get the feature from HOG, you must cast the dstHist.pu8VirAddr to (float *) to get the
  // correct value.
  printf("Output first 10x10 HOG features.\n");
  uint32_t blkUnitLength = blkSizeInCell * blkSizeInCell * binNum;
  for (size_t i = 0; i < 10; i++) {
    printf("\n");
    for (size_t j = 0; j < 10; j++) {
      printf("%.3f ", ((float *)dstHist.pu8VirAddr)[i * blkUnitLength + j]);
    }
  }
  printf("\n");

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH);
  CVI_SYS_FreeI(handle, &dstV);
  CVI_SYS_FreeI(handle, &dstMag);
  CVI_SYS_FreeI(handle, &dstAng);
  CVI_SYS_FreeM(handle, &dstHist);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstMag_u8);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}