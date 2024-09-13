# Using Custom TDL with TDL SDK

To use your custom TDL with TDL SDK, you need to know how TDL SDK fills the result into structures. If you follow the format of the SDK, you'll be able to use your custom TDL with the other modules in TDL SDK.

## Putting Results into TDL Structures

Here we will use ``cvtdl_face_t`` as an example.

```c
typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  meta_rescale_type_e rescale_type;
  cvtdl_face_info_t* info;
  cvtdl_dms_t* dms;
} cvtdl_face_t;
```

This is a structure for face. The length of the ``info`` is stored in ``size``. This is how you malloc a face info array.

```c
cvtdl_face_t faceMeta;
memset(&faceMeta, 0, sizeof(faceMeta));
faceMeta.size = 10;
faceMeta.info = (cvtdl_face_info_t*)malloc(faceMeta.size * sizeof(cvtdl_face_info_t));
```

The ``width`` and the ``height`` in ``cvtdl_face_t`` is the reference image size for ``cvtdl_bbox_t`` and ``cvtdl_pts_t`` in ``cvtdl_face_info_t``.

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
```

The rescale function in TDL SDK will restore the coordinate of the box and the points by calculating the ratio of the ``width``, ``height`` and the new ``width``, ``height`` from ``VIDEO_FRAME_INFO_S``. Make sure to call rescale manually after you fill the data into ``cvtdl_face_t``.

```c
CVI_S32 CVI_TDL_RescaleMetaCenter(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
CVI_S32 CVI_TDL_RescaleMetaRB(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
```

Now let's take a look at ``cvtdl_face_info_t``.

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
```

In TDL SDK, after face detection is finished, the following data will be available.

```c
  char name[128];
  cvtdl_bbox_t bbox;
  cvtdl_pts_t pts;
```

The following is the ``cvtdl_bbox_t`` and ``cvtdl_pts_t`` structures.

```c
typedef struct {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
} cvtdl_bbox_t;

typedef struct {
  float* x;
  float* y;
  uint32_t size;
  float score;
} cvtdl_pts_t;
```

This is how you malloc ``x`` and ``y`` for ``cvtdl_pts_t``.

```c
cvtdl_pts_t pts;
memset(&pts, 0, sizeof(pts));
pts.size = 5;
pts.x = (float*)malloc(pts.size * sizeof(float));
pts.y = (float*)malloc(pts.size * sizeof(float));
```


After face recognition, the following data will be available.

```c
  cvtdl_feature_t feature;
```

``cvtdl_feature_t`` is a structure for storing features. The ``size`` represents the number of features, and the ``feature_type_e`` stores the data type that ``ptr`` should cast to when use.

```c
typedef struct {
  int8_t* ptr;
  uint32_t size;
  feature_type_e type;
} cvtdl_feature_t;
```

This is how you malloc a ``ptr`` for ``cvtdl_feature_t``.

```c
cvtdl_feature_t feature;
memset(&feature, 0, sizeof(feature));
feature.size = 256;
feature.type = TYPE_INT8;
feature.ptr = (int8_t*)malloc(feature.size * sizeof(int8_t));
```

This is how you read or write a feature from ``cvtdl_feature_t``.

```c
if (feature.type == TYPE_INT8) {
  int8_t *ptr = (int8_t*)feature.ptr;
  for (uint32_t i = 0; i < feature.size; i++) {
    ptr[i] = i % 256;
  }
} else {
  // ... do something ...
}
```

The other data will be available if the corresponding API is called.

```c
  cvtdl_face_emotion_e emotion;  // Available if use face attribute.
  cvtdl_face_gender_e gender;  // Available if use face attribute.
  cvtdl_face_race_e race;  // Available if use face attribute.
  float age;  // Available if use face attribute.
  float liveness_score;  // Available if use liveness.
  float mask_score;  // Available if use mask classification.
  cvtdl_face_quality_t face_quality;  // Available if use face quality.
```

To free the structure, just simply call ``CVI_TDL_Free``. It supports the following structures.

```c
#define CVI_TDL_Free(X) _Generic((X),                   \
           cvtdl_feature_t*: CVI_TDL_FreeFeature,        \
           cvtdl_pts_t*: CVI_TDL_FreePts,                \
           cvtdl_tracker_t*: CVI_TDL_FreeTracker,        \
           cvtdl_face_info_t*: CVI_TDL_FreeFaceInfo,     \
           cvtdl_face_t*: CVI_TDL_FreeFace,              \
           cvtdl_object_info_t*: CVI_TDL_FreeObjectInfo, \
           cvtdl_object_t*: CVI_TDL_FreeObject)(X)
```

## Custom TDL framework

In TDL SDK, we provide a framework to let user easily run their own model with hardware support. The functions are defined in ``cvi_tdl_custom.h``. The following example is the simpliest way to use the framework with your own model. If your model needs extra preprocessing, you can do it before ``CVI_TDL_Custom_RunInference``.

```c
  // Initialization
  uint32_t id;
  // Add inference instance. Remember the returned id to access instance.
  CVI_TDL_Custom_AddInference(handle, &id);
  // Set model path.
  CVI_TDL_Custom_SetModelPath(handle, id, filepath);
  // Must set if you want to use VPSS in custom TDL framework.
  const float factor = FACTOR;
  const float mean = MEAN;
  const float threshold = THRESHOLD;
  CVI_TDL_Custom_SetVpssPreprocessParam(handle, id, &factor, &mean, 1, threshold, false);

  // ... Do preprocessing if necessary.

  // Run inference
  CVI_TDL_Custom_RunInference(handle, id, &frame, numOfFrames);
  // Post-processing.
  int8_t *tensor = NULL;
  uint32_t tensorCount = 0;
  uint16_t unitSize = 0;
  CVI_TDL_Custom_GetOutputTensor(handle, id, TENSORNAME, &tensor, &tensorCount, &unitSize);

  // ... Do postprocessing.
```

### Custom TDL Settings

There are two ways to send the data into inference. The option is set using ``use_tensor_input`` in ``CVI_TDL_Custom_SetPreprocessFuncPtr``.

```c
typedef void (*preProcessFunc)(VIDEO_FRAME_INFO_S *stInFrame, VIDEO_FRAME_INFO_S *stOutFrame);

CVI_S32 CVI_TDL_Custom_SetPreprocessFuncPtr(cvitdl_handle_t handle, const uint32_t id,
                                           preProcessFunc func, const bool use_tensor_input,
                                           const bool use_vpss_sq);
```

If ``use_tensor_input`` is set to ``true``, you must pass a function that defines how the ``VIDEO_FRAME_INFO_S`` should copy the data into tensor input. The option ``use_vpss_sq`` will also be ignored. If ``use_tensor_input`` is set to ``false``, ``preProcessFunc`` can be set to ``NULL``. If both ``use_tensor_input`` and ``use_vpss_sq`` are set to ``false``, the ``VIDEO_FRAME_INFO_S`` will be directly passed into TPU inference.

If you set ``use_vpss_sq`` to ``true``, you must set the the factor and mean for VPSS with these functions. The framework will get the quantization threshold from loaded cvimodel. Make sure you select the correct input index for your model.

```c
CVI_S32 CVI_TDL_Custom_SetVpssPreprocessParam(cvitdl_handle_t handle, const uint32_t id,
                                             const uint32_t inputIndex,
                                             const float *factor, const float *mean,
                                             const uint32_t length,
                                             const bool keepAspectRatio);

CVI_S32 CVI_TDL_Custom_SetVpssPreprocessParamRaw(cvitdl_handle_t handle, const uint32_t id,
                                                const uint32_t inputIndex,
                                                const float *qFactor, const float *qMean,
                                                const uint32_t length,
                                                const bool keepAspectRatio);
```

The first function will calculate the quantized factor and quantized mean for you, while the second function will directly pass the values to the vpss. The ``length`` represents the buffer size of factor and mean. If set to 1, all the channels will use the same values. ``length`` can only be 1 or 3. If ``keepAspectRatio`` is set to true, the black will be padded at the right and the bottom.

This function is a little bit different from ``CVI_TDL_CloseModel``. The close model in custom framework will not delete the instance but only close the model.

```c
CVI_S32 CVI_TDL_Custom_CloseModel(cvitdl_handle_t handle, const uint32_t id);
```

Calling ``CVI_TDL_CloseAllModel`` will still close all the models and delete all the instance.

### Custom TDL Inference

You are not able to change the settings after these functions are called unless you called ``CVI_TDL_Custom_CloseModel``. If your preprocessing code needs to know the size of the input tensor, you can get the size with ``CVI_TDL_Custom_GetInputTensorNCHW``. Setting the ``tensorName`` to ``NULL`` will return the first input tensor.

```c
CVI_S32 CVI_TDL_Custom_GetInputTensorNCHW(cvitdl_handle_t handle, const uint32_t id,
                                         const char *tensorName, uint32_t *n,
                                         uint32_t *c, uint32_t *h, uint32_t *w);

CVI_S32 CVI_TDL_Custom_RunInference(cvitdl_handle_t handle, const uint32_t id, VIDEO_FRAME_INFO_S *frame, uint32_t NumOfFrames);

CVI_S32 CVI_TDL_Custom_GetOutputTensor(cvitdl_handle_t handle, const uint32_t id,
                                      const char *tensorName, int8_t **tensor,
                                      uint32_t *tensorCount, uint16_t *unitSize);
```

After ``CVI_TDL_Custom_RunInference`` is called, you can get the inference result with ``CVI_TDL_Custom_GetOutputTensor``. Setting the ``tensorName`` to ``NULL`` will return the first output tensor. Whether the skip postprocessing is set to true or false, the return tensor is stored in ``int8_t``. The number of elements and the size of each element are also returned. You'll need to cast to the correct data type before use. The rest of the functions in ``cvi_tdl_custom.h`` work just like the functions in ``cvi_tdl_core.h``.

```c
CVI_S32 CVI_TDL_Custom_GetModelPath(cvitdl_handle_t handle, const uint32_t id, char **filepath);

CVI_S32 CVI_TDL_Custom_SetVpssThread(cvitdl_handle_t handle, const uint32_t id, const uint32_t thread);

CVI_S32 CVI_TDL_Custom_SetVpssThread2(cvitdl_handle_t handle, const uint32_t id,
                                     const uint32_t thread, const VPSS_GRP vpssGroupId);

CVI_S32 CVI_TDL_Custom_GetVpssThread(cvitdl_handle_t handle, const uint32_t id, uint32_t *thread);
```
