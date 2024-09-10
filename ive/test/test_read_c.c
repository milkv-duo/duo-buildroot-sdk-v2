#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Incorrect loop value. Usage: %s <file_name>\n", argv[0]);
    return CVI_FAILURE;
  }
  const char *file_name = argv[1];

  // Create instance
  IVE_HANDLE handle = CVI_IVE_CreateHandle();
  printf("BM Kernel init.\n");

  // Fetch image information
  IVE_IMAGE_S src = CVI_IVE_ReadImage(handle, file_name, IVE_IMAGE_TYPE_U8C3_PLANAR);

  // write result to disk
  printf("Save to image.\n");
  CVI_IVE_WriteImage(handle, "test_read_c.png", &src);

  // Free memory, instance
  CVI_SYS_FreeI(handle, &src);
  CVI_IVE_DestroyHandle(handle);

  return CVI_SUCCESS;
}