#include "cvi_tdl.h"

#include <string.h>

int main(void) {
  CVI_S32 ret = CVI_TDL_SUCCESS;
  cvitdl_handle_t handle;
  // Create handle
  if ((ret = CVI_TDL_CreateHandle(&handle)) != CVI_TDL_SUCCESS) {
    printf("Handle create failed\n");
    return ret;
  }

  // Get the used group ids by TDL SDK.
  VPSS_GRP *groups = NULL;
  uint32_t nums = 0;
  if ((ret = CVI_TDL_GetVpssGrpIds(handle, &groups, &nums)) != CVI_TDL_SUCCESS) {
    printf("Get used group id failed.\n");
    return ret;
  }
  printf("Used group id: %u\n", nums);
  for (uint32_t i = 0; i < nums; i++) {
    printf(" |- [%u] group id used: %u ", i, groups[i]);
  }
  printf("\n");
  free(groups);

  // Destroy handle.
  ret = CVI_TDL_DestroyHandle(handle);
  return ret;
}