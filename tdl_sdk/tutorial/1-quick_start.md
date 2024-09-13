# Quick Start

These are the files you may need to use the TDL SDK.

``` shell
# Include header
cvi_tdl.h
cvi_tdl_app.h

# Static library
libcvi_tdl.a
libcvi_tdl_app.a

# Dynamic library
libcvi_tdl.so
libcvi_tdl_app.so
```

Include the headers in your ``*.cpp`` file.

```c
#include "cvi_tdl.h"

int main(void) {

  return 0;
}
```

Link the libraries to your binary.

| Function            | Linked libraries                      |
|---------------------|---------------------------------------|
| CVI_TDL_*            | libcvi_tdl.so                           |
| CVI_TDL_Service_*    | libcvi_tdl.so                           |
| CVI_TDL_Eval_*       | libcvi_tdl_evaluation.so                |
| CVI_TDL_APP_*        | libcvi_tdl_app.so                       |

THe following snippet is a cmake example.

```cmake
project(sample_binary)
set(CVI_TDL_SDK_ROOT "/path-to-the-directory")
include_directories(${CVI_TDL_SDK_ROOT}/include)
add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} ${CVI_TDL_SDK_ROOT}/lib/libcvi_tdl.so)
```

## Basic

TDL SDK is a C style SDK with prefix ``CVI_TDL_``. Let's take a quick example of creating an TDL SDK handle. The handle is defined as ``void *`` just like typical C library.

```c
typedef void *cvitdl_handle_t;
```

Make sure to destroy the handle using ``CVI_TDL_DestroyHandle`` to prevent memory leak.

```c
  cvitdl_handle_t handle;
  // Create handle
  if ((ret = CVI_TDL_CreateHandle(&handle))!= CVI_TDL_SUCCESS) {
    printf("Handle create failed\n");
    return ret;
  }

  // ...Do something...

  // Destroy handle.
  ret = CVI_TDL_DestroyHandle(handle);
  return ret;
```

Now we know how to create a handle, let's take a look at ``sample_init.c``. When setting a path to handle, it does not actually load the model, it only saves the path to the correspoding network list. You can also get the path you set from the handle.

```c
  const char fake_path[] = "face_quality.cvimodel";
  if ((ret = CVI_TDL_OpenModel(handle, CVI_TDL_SUPPORTED_MODEL_FACEQUALITY, fake_path)) !=
      CVI_TDL_SUCCESS) {
    printf("Set model path failed.\n");
    return ret;
  }
  const char *path = CVI_TDL_GetModelPath(handle, CVI_TDL_SUPPORTED_MODEL_FACEQUALITY);

  // Check if the two path are the same.
  if (strcmp(path, fake_path) != 0) {
    ret = CVI_FAILURE;
  }
```

TDL SDK use Vpss hardware to speed up the calculating time on images. Vpss API is a part of Middleware SDK. The SDK doesn't use handle system, but use IDs instead. One TDL SDK handle can use multiple Vpss IDs, thus the return value of ``CVI_TDL_GetVpssGrpIds`` is an array. Remember to free the array to prevent memory leak.

```c
  // Get the used group ids by TDL SDK.
  VPSS_GRP *groups = NULL;
  uint32_t nums = 0;
  if ((ret = CVI_TDL_GetVpssGrpIds(handle, &groups, &nums)) != CVI_TDL_SUCCESS) {
    printf("Get used group id failed.\n");
    return ret;
  }
  printf("Used group id = %u:\n", nums);
  for (uint32_t i = 0; i < nums; i++) {
    printf("%u ", groups[i]);
  }
  printf("\n");
  free(groups);
```

You can also manually assign a group id to TDL SDK when creating a handle.

```c
  VPSS_GRP groupId = 2;
  CVI_U8 vpssDev = 0;
  cvitdl_handle_t handle;
  // Create handle
  if ((ret = CVI_TDL_CreateHandle2(&handle, groupId, vpssDev))!= CVI_TDL_SUCCESS) {
    printf("Handle create failed\n");
    return ret;
  }
```
