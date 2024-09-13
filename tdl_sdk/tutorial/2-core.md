# Core

## Multi-threading

We don't recommend to use the same ``VPSS_GRP`` in different threads, so we provide a function to change thread id for any model in one handle.

```c
// Auto assign group id
CVI_S32 CVI_TDL_SetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                             const uint32_t thread);
// Manually assign group id
CVI_S32 CVI_TDL_SetVpssThread2(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                              const uint32_t thread, const VPSS_GRP vpssGroupId, const CVI_U8 dev);
```

You can get the current thread id for any model.

```c
CVI_S32 CVI_TDL_GetVpssThread(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config, uint32_t *thread);
```

To get the relationship between thread and the ``VPSS_GRP`` thread uses, you can call ``CVI_TDL_GetVpssGrpIds``. The output array ``groups`` gives all the used ``VPSS_GRP`` in order.

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

## Reading Image

There are two ways to read image from file.

1. The ``CVI_TDL_ReadImage`` function inside module ``core``
2. IVE library

### ``CVI_TDL_ReadImage``

``CVI_TDL_ReadImage`` uses ``ION`` from Middleware SDK. You must release ``ION`` with ``CVI_TDL_ReleaseImage`` to prevent memory leaks.

```c
  const char *path = "hi.jpg";
  VIDEO_FRAME_INFO_S rgb_frame;
  CVI_S32 ret = CVI_TDL_ReadImage(path, &rgb_frame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Read image failed with %#x!\n", ret);
    return ret;
  }

  // ...Do something...

  CVI_TDL_ReleaseImage(&rgb_frame);
```

VI, VPSS, and ``ION`` shares the same memory pool. It is neccesarily to calculate the required space when initializing Middleware, or you can use the helper function provided by TDL SDK.

```c
  const CVI_S32 vpssgrp_width = 1920;
  const CVI_S32 vpssgrp_height = 1080;
  const CVI_S32 image_num = 12;
  // We allocate the pool with size of 24 * (RGB_PACKED_IMAGE in the size of 1920 * 1080)
  // The first four parameters are the input info, and the last four are the output info.
  ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888,
                         image_num, vpssgrp_width, vpssgrp_height,
                         PIXEL_FORMAT_RGB_888, image_num);
  if (ret != CVI_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }
```

### IVE library

IVE library does not occupied spaces in Middleware's memory pool. IVE uses a different image structure, so it must be converted to ``VIDEO_FRAME_INFO_S`` before use.

```c
  const char *path = "hi.jpg";
  IVE_HANDLE ive_handle = CVI_IVE_CreateHandle();
  IVE_IMAGE_S image = CVI_IVE_ReadImage(ive_handle, path, IVE_IMAGE_TYPE_U8C3_PACKAGE);
  if (image.u16Width == 0) {
    printf("Read image failed with %x!\n", ret);
    return ret;
  }
  // Convert to VIDEO_FRAME_INFO_S. IVE_IMAGE_S must be kept to release when not used.
  VIDEO_FRAME_INFO_S frame;
  ret = CVI_IVE_Image2VideoFrameInfo(&image, &frame, false);
  if (ret != CVI_SUCCESS) {
    printf("Convert to video frame failed with %#x!\n", ret);
    return ret;
  }

  // ...Do something...

  // Free image and handles.
  CVI_SYS_FreeI(ive_handle, &image);
  CVI_IVE_DestroyHandle(ive_handle);
```

## Model related settings

### Open model path

The model path must be set before the corresponding inference function is called.

```c
CVI_S32 CVI_TDL_OpenModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                            const char *filepath);
```

### Skip VPSS preprocess

If you already done the "scaling and quantization" step with the Middleware's binding mode, you can skip the VPSS preprocess with the following API for any model.

```c
CVI_S32 CVI_TDL_SetSkipVpssPreprocess(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config, bool skip);
```

### Set model threshold

The threshold for any model can be set any time. If you set the threshold after the inference function, TDL SDK will use the default threshold saved in the model.

```c
CVI_S32 CVI_TDL_SetModelThreshold(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                             float threshold);
```

### Closing models

TDL SDK will close the model for you if you destroy the handle, but we still provide the API to close the all the models or indivitually. This is because a user may run out of memory if a user want to switch between features while runtime, providing this API allows users to free spaces without destroying the handle.

```c
CVI_S32 CVI_TDL_CloseAllModel(cvitdl_handle_t handle);

CVI_S32 CVI_TDL_CloseModel(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config);
```

### Acquiring VPSS_CHN_ATTR_S from built-in models

The model will be loaded when the function is called. Make sure you complete your settings before calling this funciton. If you want to use VPSS binding mode instead of built-in VPSS instance inside TDL SDK, you can get the VPSS_CHN_ATTR_S from the supported built-in models. If the model does not support exporting, ``CVI_TDL_FAILURE`` will return.

```c
CVI_S32 CVI_TDL_GetVpssChnConfig(cvitdl_handle_t handle, CVI_TDL_SUPPORTED_MODEL_E config,
                                const CVI_U32 frameWidth, const CVI_U32 frameHeight,
                                const CVI_U32 idx, cvtdl_vpssconfig_t *chnConfig);
```

### Inference calls

A model will be loaded when the function is called for the first time. The following functions are the example of the API calls.

```c
// Face recognition
CVI_S32 CVI_TDL_FaceAttribute(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                             cvtdl_face_t *faces);
// Object detection
CVI_S32 CVI_TDL_Yolov3(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj,
                      cvtdl_obj_det_type_e det_type);
```

Related sample codes: ``sample_read_fr.c``, ``sample_read_fr2``, ``sample_vi_od.c``

## Model output structures

The output result from a model stores in a apecific structure. Face related data stores in ``cvtdl_face_t``, and object related data stores in ``cvtdl_object_t``. THe code snippet shows how to use the structure.

```c
  cvtdl_face_t face;
  // memset before use.
  memset(&face, 0, sizeof(cvtdl_face_t));

  CVI_TDL_FaceAttribute(handle, frame, &face);

  // Free to avoid memory leaks.
  CVI_TDL_Free(&face);
```

``CVI_TDL_Free`` is defined as a generic type, it supports the following structures.

```c
#ifdef __cplusplus
#define CVI_TDL_Free(X) CVI_TDL_FreeCpp(X)
#else
// clang-format off
#define CVI_TDL_Free(X) _Generic((X),                   \
           cvtdl_feature_t*: CVI_TDL_FreeFeature,        \
           cvtdl_pts_t*: CVI_TDL_FreePts,                \
           cvtdl_face_info_t*: CVI_TDL_FreeFaceInfo,     \
           cvtdl_face_t*: CVI_TDL_FreeFace,              \
           cvtdl_object_info_t*: CVI_TDL_FreeObjectInfo, \
           cvtdl_object_t*: CVI_TDL_FreeObject)(X)
// clang-format on
#endif
```

These structures are defined in ``include/core/face/cvtdl_face_types.h``, and `` include/core/object/cvtdl_object_types.h``. The ``size`` is the length of the variable ``info``. The ``width`` and ``height`` stores in the structure are the reference size used by variable ``info``.

```c
typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  meta_rescale_type_e rescale_type;
  cvtdl_face_info_t* info;
  cvtdl_dms_t* dms;
} cvtdl_face_t;

typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  cvtdl_object_info_t *info;
} cvtdl_object_t;
```

### Reference size

The structure ``cvtdl_bbox_t`` stores in ``cvtdl_face_info_t`` and ``cvtdl_object_info_t`` are the bounding box of the found face or object. The coordinates store in ``cvtdl_bbox_t`` correspond to the ``width`` and ``height`` instead of the size of the input frame. Usually the ``width`` and ``height`` will equal to the size of the output model. The structure ``cvtdl_pts_t`` in ``cvtdl_face_info_t`` also refers to the ``width`` and ``height``.

```c
typedef struct {
  char name[128];
  uint64_t unique_id;
  cvtdl_bbox_t bbox;
  cvtdl_pts_t pts;
  cvtdl_feature_t feature;
  cvtdl_face_emotion_e emotion;
  cvtdl_face_gender_e gender;
  cvtdl_face_race_e race;
  float score;
  float age;
  float liveness_score;
  float hardhat_score;
  float mask_score;
  float recog_score;
  float face_quality;
  float pose_score;
  float pose_score1;
  float sharpness_score;
  float blurness;
  cvtdl_head_pose_t head_pose;
  int track_state;
} cvtdl_face_info_t;

typedef struct {
  char name[128];
  cvtdl_bbox_t bbox;
  int classes;
} cvtdl_object_info_t;
```

## Recovering coordinate

To get the coordinate correspond to the frame size, we provide two generic type APIs.

```c
#ifdef __cplusplus
#define CVI_TDL_RescaleMetaCenter(videoFrame, X) CVI_TDL_RescaleMetaCenterCpp(videoFrame, X)
#define CVI_TDL_RescaleMetaRB(videoFrame, X) CVI_TDL_RescaleMetaRBCpp(videoFrame, X)
#else
#define CVI_TDL_RescaleMetaCenter(videoFrame, X) _Generic((X), \
           cvtdl_face_t*: CVI_TDL_RescaleMetaCenterFace,        \
           cvtdl_object_t*: CVI_TDL_RescaleMetaCenterObj)(videoFrame, X)
#define CVI_TDL_RescaleMetaRB(videoFrame, X) _Generic((X),     \
           cvtdl_face_t*: CVI_TDL_RescaleMetaRBFace,            \
           cvtdl_object_t*: CVI_TDL_RescaleMetaRBObj)(videoFrame, X)
#endif
```

If you use the inference calls from TDL SDK without VPSS binding mode, you can call ``CVI_TDL_RescaleMetaCenter`` to recover the results. However, if you use VPSS binding mode instead of the VPSS inside the inference calls, the function you use depends on the ``stAspectRatio`` settings. If the setting is ``ASPECT_RATIO_AUTO``, you use ``CVI_TDL_RescaleMetaCenter``. If the setting is ``ASPECT_RATIO_MANUAL``, you can call ``CVI_TDL_RescaleMetaRB`` if you only pad the image
 in the direction of right or bottom.

```c
  cvtdl_face_t face;
  // memset before use.
  memset(&face, 0, sizeof(cvtdl_face_t));

  CVI_TDL_FaceAttribute(handle, frame, &face);
  CVI_TDL_RescaleMetaCenter(frame, &face);

  // Free to avoid memory leaks.
  CVI_TDL_Free(&face);
```
