#include "bmkernel/bm_kernel.h"

#include "bmkernel/bm1880v2/1880v2_fp_convert.h"
#include "cvi_ive.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef __ARM_ARCH
#include "arm_neon.h"
#endif
#define CELL_SIZE 8
#define BLOCK_SIZE 2
#define STEP_X 1
#define STEP_Y 1
#define BIN_NUM 9

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstAng);

typedef unsigned char UNIT8;
UNIT8 g_GammaLUT[256];
// Buildtable()
// fPrecompensation=1/gamma
void BuildTable(float fPrecompensation) {
  int i;
  float f;
  for (i = 0; i < 256; i++) {
    f = (i + 0.5f) / 256;
    f = (float)pow(f, fPrecompensation);
    g_GammaLUT[i] = (UNIT8)(f * 256 - 0.5f);
  }
}
// fGamma = 2.2
void GammaCorrectiom(UNIT8 src[], int iWidth, int iHeight, UNIT8 Dst[], float fGamma) {
  int iCols, iRows, ii;
  BuildTable(1 / fGamma);  // gamma correction LUT

  for (iRows = 0; iRows < iHeight; iRows++) {
    ii = iRows * iWidth;
    for (iCols = 0; iCols < iWidth; iCols++) {
      Dst[ii + iCols] = g_GammaLUT[src[ii + iCols]];
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Usage: %s <fullpath.txt> <number> <out_fea.txt>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *img_path_file = argv[1];
  const char *fea_path_file = argv[3];
  size_t total_run = atoi(argv[2]);  // total number of images

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");
  IVE_DST_IMAGE_S dstH, dstV;
  IVE_DST_IMAGE_S dstH_u8, dstV_u8;
  IVE_DST_IMAGE_S dstMag, dstAng;
  IVE_DST_IMAGE_S dstAng_u8;
  IVE_DST_MEM_INFO_S dstHist;
  CVI_U32 dstHistByteSize = 0;
  IVE_HOG_CTRL_S pstHogCtrl;
  int ret = 1;

  bool binit = false;

  char image_full_path[256];
  FILE *fpPath = fopen(img_path_file, "r");

  FILE *fpFea = fopen(fea_path_file, "w");
  for (int i = 0; i < total_run; i++) {
    fscanf(fpPath, "%s", image_full_path);
    printf("[0]: %s\n", image_full_path);
    // Fetch image information
    IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, image_full_path, IVE_IMAGE_TYPE_U8C1);
    IVE_IMAGE_S src2 = CVI_IVE_ReadImage(handle, image_full_path, IVE_IMAGE_TYPE_U8C1);

    GammaCorrectiom(src.pu8VirAddr[0], src.u32Width, src.u32Height, src2.pu8VirAddr[0], 2.2);

    // int nChannels = 1;
    int width = src.u32Width;
    int height = src.u32Height;
    if (!binit) {
      // IVE_DST_IMAGE_S dstH, dstV;
      CVI_IVE_CreateImage(handle, &dstV, IVE_IMAGE_TYPE_BF16C1, width, height);
      CVI_IVE_CreateImage(handle, &dstH, IVE_IMAGE_TYPE_BF16C1, width, height);

      // IVE_DST_IMAGE_S dstH_u8, dstV_u8;
      CVI_IVE_CreateImage(handle, &dstH_u8, IVE_IMAGE_TYPE_U8C1, width, height);
      CVI_IVE_CreateImage(handle, &dstV_u8, IVE_IMAGE_TYPE_U8C1, width, height);

      // IVE_DST_IMAGE_S dstAng;
      CVI_IVE_CreateImage(handle, &dstMag, IVE_IMAGE_TYPE_BF16C1, width, height);
      CVI_IVE_CreateImage(handle, &dstAng, IVE_IMAGE_TYPE_BF16C1, width, height);

      // IVE_DST_IMAGE_S dstAng_u8;
      CVI_IVE_CreateImage(handle, &dstAng_u8, IVE_IMAGE_TYPE_U8C1, width, height);

      // IVE_DST_MEM_INFO_S dstHist;
      CVI_IVE_GET_HOG_SIZE(dstAng.u32Width, dstAng.u32Height, BIN_NUM, CELL_SIZE, BLOCK_SIZE,
                           STEP_X, STEP_Y, &dstHistByteSize);
      CVI_IVE_CreateMemInfo(handle, &dstHist, dstHistByteSize);

      pstHogCtrl.u8BinSize = BIN_NUM;
      pstHogCtrl.u32CellSize = CELL_SIZE;
      pstHogCtrl.u16BlkSizeInCell = BLOCK_SIZE;
      pstHogCtrl.u16BlkStepX = STEP_X;
      pstHogCtrl.u16BlkStepY = STEP_Y;
      binit = true;
    }
    printf("Run TPU HOG. len: %d\n", dstHistByteSize);
    // IVE_HOG_CTRL_S pstHogCtrl;

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    // for (size_t i = 0; i < total_run; i++)
    {
      CVI_IVE_HOG(handle, &src2, &dstH, &dstV, &dstMag, &dstAng, &dstHist, &pstHogCtrl, 0);
      float *ptr = (float *)dstHist.pu8VirAddr;

      uint32_t blkSize = BLOCK_SIZE * BLOCK_SIZE * BIN_NUM;
      uint32_t blkNum = dstHistByteSize / sizeof(float) / blkSize;
      int reald = blkSize * blkNum;
      if (reald != 3780) {
        break;
      }
      for (int j = 0; j < reald; j++) {
        // printf("%d %d\n", j, ptr[j]  );
        fprintf(fpFea, "%f ", ptr[j]);
      }
      fprintf(fpFea, "\n");
    }
    gettimeofday(&t1, NULL);
    unsigned long elapsed_tpu =
        ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec) / total_run;

    printf("Normalize result to 0-255.\n");
    IVE_ITC_CRTL_S iveItcCtrl;
    iveItcCtrl.enType = IVE_ITC_NORMALIZE;
    CVI_IVE_ImageTypeConvert(handle, &dstV, &dstV_u8, &iveItcCtrl, 0);
    CVI_IVE_ImageTypeConvert(handle, &dstH, &dstH_u8, &iveItcCtrl, 0);
    CVI_IVE_ImageTypeConvert(handle, &dstAng, &dstAng_u8, &iveItcCtrl, 0);

    CVI_IVE_BufRequest(handle, &src);
    CVI_IVE_BufRequest(handle, &src2);
    CVI_IVE_BufRequest(handle, &dstH);
    CVI_IVE_BufRequest(handle, &dstV);
    CVI_IVE_BufRequest(handle, &dstAng);
    // ret = cpu_ref(nChannels, &src, &dstH, &dstV, &dstAng);
    if (total_run == 1) {
      // write result to disk
      printf("Save to image.\n");
      CVI_IVE_WriteImage(handle, "test_sobelV_c.png", &dstV_u8);
      CVI_IVE_WriteImage(handle, "test_sobelH_c.png", &dstH_u8);
      CVI_IVE_WriteImage(handle, "test_ang_c.png", &dstAng_u8);
      printf("Output HOG feature.\n");
      /*
      uint32_t blkSize = BLOCK_SIZE * BLOCK_SIZE * BIN_NUM;
      uint32_t blkNum = dstHistSize / sizeof(float) / blkSize;
      for (size_t i = 0; i < blkNum; i++) {
        //printf("\n");
        for (size_t j = 0; j < blkSize; j++) {
          printf("%3f ", ((float *)dstHist.pu8VirAddr)[i + j]);
        }
      }
      printf("\n");*/

    } else {
      printf("OOO %-10s %10lu %10s %10s\n", "HOG", elapsed_tpu, "NA", "NA");
    }

    CVI_SYS_FreeI(handle, &src);
    CVI_SYS_FreeI(handle, &src2);
  }
  fclose(fpFea);
  fclose(fpPath);
  // Free memory, instance
  // CVI_SYS_FreeI(handle, &src);
  CVI_SYS_FreeI(handle, &dstH);
  CVI_SYS_FreeI(handle, &dstH_u8);
  CVI_SYS_FreeI(handle, &dstV);
  CVI_SYS_FreeI(handle, &dstV_u8);
  CVI_SYS_FreeI(handle, &dstAng);
  CVI_SYS_FreeI(handle, &dstAng_u8);
  CVI_SYS_FreeM(handle, &dstHist);
  CVI_IVE_DestroyHandle(handle);

  return ret;
}

int cpu_ref(const int channels, IVE_SRC_IMAGE_S *src, IVE_DST_IMAGE_S *dstH, IVE_DST_IMAGE_S *dstV,
            IVE_DST_IMAGE_S *dstAng) {
  int ret = CVI_SUCCESS;
  uint16_t *dstH_ptr = (uint16_t *)dstH->pu8VirAddr[0];
  uint16_t *dstV_ptr = (uint16_t *)dstV->pu8VirAddr[0];
  uint16_t *dstAng_ptr = (uint16_t *)dstAng->pu8VirAddr[0];
  float mul_val = 180.f / M_PI;
  float ang_abs_limit = 1;

  printf("Check Ang:\n");
  for (size_t i = 0; i < channels * src->u32Width * src->u32Height; i++) {
    float dstH_f = convert_bf16_fp32(dstH_ptr[i]);
    float dstV_f = convert_bf16_fp32(dstV_ptr[i]);
    float dstAng_f = convert_bf16_fp32(dstAng_ptr[i]);
    float atan2_res = (float)atan2(dstV_f, dstH_f) * mul_val;
    float error = fabs(atan2_res - dstAng_f);
    if (error > ang_abs_limit) {
      // printf("[%zu] atan2( %f, %f) = TPU %f, CPU %f. eplison = %f\n", i, dstV_f, dstH_f,
      // dstAng_f,
      //       atan2_res, error);
      ret = CVI_FAILURE;
    }
  }
  return ret;
}