# Service Module

## Create service handle

In programming, provide create handle api assigns a unique identifier to a resource, and provide destroy api to releases it when no longer needed.

```c
CVI_S32 CVI_TDL_Service_CreateHandle(cvitdl_service_handle_t *handle,
                                    cvitdl_handle_t tdl_handle);

CVI_S32 CVI_TDL_Service_DestroyHandle(cvitdl_service_handle_t handle);
```

## Feature Matching

Service provide feature matching tool to analyze the result from model that generates feature such as Face Attribute, OSNet. First, use ``RegisterFeatureArray`` to register a feature array for output comparison. Currently only supports method ``INNER_PRODUCT``.

```c
CVI_S32 CVI_TDL_Service_RegisterFeatureArray(cvitdl_service_handle_t handle,
                                            const cvtdl_service_feature_array_t featureArray,
                                            const cvtdl_service_feature_matching_e method);
```

Second, use ``FaceInfoMatching`` or ``ObjectInfoMatching`` to match to output result with the feature array. The length of the top ``index`` equals to ``k``.

```c
CVI_S32 CVI_TDL_Service_FaceInfoMatching(cvitdl_service_handle_t handle, const cvtdl_face_t *face,
                                        const uint32_t k, uint32_t **index);

CVI_S32 CVI_TDL_Service_ObjectInfoMatching(cvitdl_service_handle_t handle,
                                          const cvtdl_object_info_t *object_info, const uint32_t k,
                                          uint32_t **index);
```

## Calculate Similarity

Service provide a similarity comparison tool.

```c
CVI_S32 CVI_TDL_Service_CalculateSimilarity(cvitdl_service_handle_t handle,
                                           const cvtdl_feature_t *feature_rhs,
                                           const cvtdl_feature_t *feature_lhs,
                                           float *score); 
```

The tool also provides raw feature comparison with the registered feature array. Currently only supports ``TYPE_INT8`` and ``TYPE_UINT8`` comparison.

```c
CVI_S32 CVI_TDL_Service_RawMatching(cvitdl_service_handle_t handle, const void *feature,
                                   const feature_type_e type, const uint32_t topk,
                                   float threshold, uint32_t *indices, float *sims,
                                   uint32_t *size);
```

## Digital Zoom

Digital Zoom is an effect tool that zooms in to the largest detected bounding box in a frame. Users can create zoom-in effect easily with this tool.

``face_skip_ratio`` or ``obj_skip_ratio`` is a threshold that skips the bounding box area smaller than the ``image_size * face_skip_ratio``. ``trans_ratio`` is a value that controls the zoom-in speed. The recommended initial value for these two parameters are ``0.05f`` and ``0.1f``. The ``padding_ratio`` pads the final union area before zoom if set.

```c
CVI_S32 CVI_TDL_Service_FaceDigitalZoom(cvitdl_service_handle_t handle,
                                       const VIDEO_FRAME_INFO_S *inFrame,
                                       const cvtdl_face_t *meta,
                                       const float face_skip_ratio,
                                       const float trans_ratio,
                                       const float padding_ratio,
                                       VIDEO_FRAME_INFO_S *outFrame);

CVI_S32 CVI_TDL_Service_ObjectDigitalZoom(cvitdl_service_handle_t handle,
                                         const VIDEO_FRAME_INFO_S *inFrame,
                                         const cvtdl_object_t *meta,
                                         const float obj_skip_ratio,
                                         const float trans_ratio,
                                         const float padding_ratio,
                                         VIDEO_FRAME_INFO_S *outFrame);

CVI_S32 CVI_TDL_Service_ObjectDigitalZoomExt(cvitdl_service_handle_t handle, const 
                                            VIDEO_FRAME_INFO_S *inFrame, 
                                            const cvtdl_object_t *meta, const float obj_skip_ratio, 
                                            const float trans_ratio, const float pad_ratio_left,
                                            const float pad_ratio_right, const float pad_ratio_top, 
                                            const float pad_ratio_bottom, VIDEO_FRAME_INFO_S *outFrame);
```

Related sample codes: ``sample_read_dt.c``

## Draw Rect

``DrawRect`` is a function that draws all the bounding boxes and their tag names on the frame.

```c
CVI_S32 CVI_TDL_Service_FaceDrawRect(cvitdl_service_handle_t handle, const cvtdl_face_t *meta, 
                                    VIDEO_FRAME_INFO_S *frame, bool drawText, cvtdl_service_brush_t brush);

CVI_S32 CVI_TDL_Service_FaceDrawRect2(cvitdl_service_handle_t handle,
                                    const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                                    const bool drawText, cvtdl_service_brush_t *brushes);

CVI_S32 CVI_TDL_Service_ObjectDrawRect(cvitdl_service_handle_t handle, 
                                      const cvtdl_object_t *meta, 
                                      VIDEO_FRAME_INFO_S *frame,
                                      bool drawText,
                                      cvtdl_service_brush_t brush);

CVI_S32 CVI_TDL_Service_ObjectDrawRect2(cvitdl_service_handle_t handle,
                                      const cvtdl_object_t *meta,
                                      VIDEO_FRAME_INFO_S *frame, 
                                      const bool drawText,
                                      cvtdl_service_brush_t *brushes);
```

## Others Draw

Also, there are some tools available for drawing points and rectangles.

```c
cvtdl_service_brush_t CVI_TDL_Service_GetDefaultBrush();

CVI_S32 CVI_TDL_Service_FaceDraw5Landmark(const cvtdl_face_t *meta,
                                         VIDEO_FRAME_INFO_S *frame);

CVI_S32 CVI_TDL_Service_Incar_ObjectDrawRect(cvitdl_service_handle_t handle,
                                            const cvtdl_dms_od_t *meta,
                                            VIDEO_FRAME_INFO_S *frame,
                                            const bool drawText,
                                            cvtdl_service_brush_t brush);

CVI_S32 CVI_TDL_Service_ObjectWriteText(char *name, int x, int y,
                                       VIDEO_FRAME_INFO_S *frame, float r, float g,
                                       float b);

CVI_S32 CVI_TDL_Service_ObjectDrawPose(const cvtdl_object_t *meta,
                                      VIDEO_FRAME_INFO_S *frame);

CVI_S32 CVI_TDL_Service_FaceDrawPts(cvtdl_pts_t *pts, VIDEO_FRAME_INFO_S *frame);

CVI_S32 CVI_TDL_Service_DrawHandKeypoint(cvitdl_service_handle_t handle,
                                        VIDEO_FRAME_INFO_S *frame,
                                        const cvtdl_handpose21_meta_ts *meta);
```

## Face Angle

Calculate the head pose angle.

```c
CVI_S32 CVI_TDL_Service_FaceAngle(const cvtdl_pts_t *pts, cvtdl_head_pose_t *hp);

CVI_S32 CVI_TDL_Service_FaceAngleForAll(const cvtdl_face_t *meta);
```

Related sample codes: ``sample_vi_fd.c``, ``sample_vi_fq.c``, ``sample_vi_mask_fr.c``

## Polygon tools

```c
CVI_S32 CVI_TDL_Service_DrawPolygon(cvitdl_service_handle_t handle,
                                   VIDEO_FRAME_INFO_S *frame, const cvtdl_pts_t *pts,
                                   cvtdl_service_brush_t brush);
                                   
DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_SetTarget(cvitdl_service_handle_t handle,
                                                    const cvtdl_pts_t *pts);

DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_GetTarget(cvitdl_service_handle_t handle,
                                                    cvtdl_pts_t ***regions_pts, uint32_t *size);

DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_CleanAll(cvitdl_service_handle_t handle);

DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_Intersect(cvitdl_service_handle_t handle,
                                                    const cvtdl_bbox_t *bbox, bool *has_intersect);
```

## Object Intersect

The function provides user to draw a line or a polygon to see if the given sequental path interacts with the set region. If a polygon is given, the status will return if the input point is **on line**, **inside** or **outside** the polygon. If a line is given, the status will return if a vector is **no intersect**, **on line**, **towards negative**, or **towards positive**.

The status enumeration.

```c
typedef enum {
  UNKNOWN = 0,
  NO_INTERSECT,
  ON_LINE,
  CROSS_LINE_POS,
  CROSS_LINE_NEG,
  INSIDE_POLYGON,
  OUTSIDE_POLYGON
} cvtdl_area_detect_e;
```

**Note**

1. In line mode, the first input point of an object will return status **unknown** cause there is no vector.

### Setting Detect Region

The region is set using ``cvtdl_pts_t``. Note that the size of ``pts->size`` must larger than 2. The API will make the lines into a close loop if 3 more more points are found. The coordinate of the points cannot exceed the size of the given frame.

```c
CVI_S32 CVI_TDL_Service_SetIntersect(cvitdl_service_handle_t handle,
                                    const VIDEO_FRAME_INFO_S *frame, const cvtdl_pts_t *pts);
```

### Detect Intersection

The user has to give an object an unique id and a coordinate (x, y). The tracker inside the API will handle the rest.

```c
typedef struct {
  uint64_t unique_id;
  float x;
  float y;
} area_detect_pts_t;
```

The output is an ``cvtdl_area_detect_e`` array. Its size is equal to the ``input_length``. You'll need to free ``status`` after use to prevent memory leaks.

```c
CVI_S32 CVI_TDL_Service_ObjectDetectIntersect(cvitdl_service_handle_t handle,
                                             const VIDEO_FRAME_INFO_S *frame,
                                             const area_detect_pts_t *input,
                                             const uint32_t input_length, cvtdl_area_detect_e **status);
```
