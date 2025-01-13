# Turnkey Deep Learning (TDL)
## 1. Overview


Cvitek's TDL (Turnkey Deep Learning) integrates algorithms to shorten the time required for application development. This architecture implements the algorithms needed for TDL, including pre-processing and post-processing, and provides a unified and convenient programming interface.

The TDL_SDK is based on Cvitek’s self-developed Middleware and TPU SDK, including two main internal modules (Core and Service), algorithm C interfaces, and algorithm applications (Application).


![TDL SDK System Framework](tutorial/assets/architecture.png)
<div style="text-align: center;">
    Pic1 TDL SDK System Framework
</div>

The Core module provides algorithm-related interfaces, encapsulating complex underlying operations and algorithm details. It performs the necessary pre-processing and post-processing on the models and completes the inference. The Service module provides auxiliary APIs related to algorithms, such as drawing, feature comparison, area intrusion detection, etc. The C interface encapsulates the functionalities of existing algorithm modules and supports both TDL SDK internal models and user-developed models (requiring model conversion according to the documentation). The Application module encapsulates application logic, such as applications that include face capture logic.

Currently, the TDL SDK includes algorithms for [motion detection](./modules/core/motion_detection)， [face detection](./modules/core/face_detection)， [face recognition](./modules/core/mask_face_recognition)， [face landmark detection](./modules/core/face_landmarker)， [fall detection](./modules/core/fall_detection)， [semantic segmentation](./modules/core/segmentation)， [license plate detection](./modules/core/license_plate_detection)， [license plate recognition](./modules/core/license_plate_recognition)， [liveness detection](./modules/core/liveness)，[sound classification](./modules/core/sound_classification)， [human keypoints detection](./modules/core/human_keypoints_detection)， [lane detection](./modules/core/lane_detection)， [object tracking](./modules/core/deepsort)， [gesture detection](./modules/core/hand_classification)， [gesture recognition](./modules/core/hand_keypoint_classification)，[text detection](./modules/core/ocr/ocr_detection)，[text recognition](./modules/core/ocr/ocr_recognition) and so on.




## 2. Repository Directory
**cmake**: Contains the CMake configuration files required for the project.
**docs**: Contains project documentation and files related to its generation.
**doxygen**: Contains Doxygen configuration files for generating API documentation.
**include**: Stores the project's header files, organized by module.
**modules**: Contains the specific implementation of each functional module and core module of the project.
**sample**: Provides multiple sample programs and their build scripts to demonstrate the project's features and usage.
**scripts**: Contains various auxiliary scripts, such as code checking, compilation, and testing scripts.
**tool**: Contains various tools and utility code.
**toolchain**: Provides toolchain configuration files for different platforms.
**tutorial**: Contains tutorial documents for the project to help users quickly get started and use the project.
Intermediate files generated during compilation and third-party library downloads will be located in the **tmp** folder.
## 3. Compilation Process


```
step1:
git clone -b sg200x-evb git@github.com:sophgo/sophpi.git
cd sophpi
./scripts/repo_clone.sh --gitclone scripts/subtree.xml

step2:
source build/cvisetup.sh
defconfig sg2002_wevb_riscv64_sd
clean_all
export TPU_REL=1
build_all

step3: (After modifying the TDL_SDK content, execute this step)
clean_tdl_sdk
build_tdl_sdk

```
## 4. Use Cases
Algorithm Interface
Using face detection as an example
```
/**
 * @brief Detect face with score.
 *
 * @param handle An TDL SDK handle.
 * @param frame Input video frame.
 * @param face_meta Output detect result. The bbox will be given.
 * @param model_index The face detection model id selected to use.
 * @return int Return CVI_TDL_SUCCESS on success.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceDetection(const cvitdl_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         CVI_TDL_SUPPORTED_MODEL_E model_index,
                                         cvtdl_face_t *face_meta);
```
Accessibility Interface
Using face framing as an example
```
/**
 * @brief Draw rect to frame with given face meta with a global brush.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the face.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDrawRect(cvitdl_service_handle_t handle,
                                                const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                                                const bool drawText, cvtdl_service_brush_t brush);
```

A simple usage example:
```
#define _GNU_SOURCE
#include <stdio.h>
#include <iostream>
#include <string>
#include "core/utils/vpss_helper.h"
#include "cvi_tdl.h"
#include "cvi_tdl_media.h"
int main(int argc, char *argv[]) {
  int vpssgrp_width = 1920;
  int vpssgrp_height = 1080;
  CVI_S32 ret = MMF_INIT_HELPER2(vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1,
                                 vpssgrp_width, vpssgrp_height, PIXEL_FORMAT_RGB_888, 1);
  if (ret != CVI_TDL_SUCCESS) {
    printf("Init sys failed with %#x!\n", ret);
    return ret;
  }

  cvitdl_handle_t tdl_handle = NULL;
  ret = CVI_TDL_CreateHandle(&tdl_handle);
  if (ret != CVI_SUCCESS) {
    printf("Create tdl handle failed with %#x!\n", ret);
    return ret;
  }

  std::string strf1(argv[2]);

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, argv[1]);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }

  CVI_TDL_UseMmap(tdl_handle, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, false);

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S bg;
  // printf("toread image:%s\n",argv[1]);
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
  }
  std::string str_res;
  for (int i = 0; i < 1; i++) {
    cvtdl_face_t obj_meta = {0};
    ret = CVI_TDL_FaceDetection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_SCRFDFACE, &obj_meta);
    std::stringstream ss;
    ss << "boxes=[";
    for (uint32_t i = 0; i < obj_meta.size; i++) {
      ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
         << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << "],";
    }
    str_res = ss.str();
    CVI_TDL_Free(&obj_meta);
  }
  std::cout << str_res << std::endl;
  CVI_TDL_ReleaseImage(img_handle, &bg);
  CVI_TDL_DestroyHandle(tdl_handle);
  CVI_TDL_Destroy_ImageProcessor(img_handle);
  return ret;
}
```

<div style="text-align: center;">
    <video width="960" controls>
  <source src="tutorial/assets/人脸检测.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>
</div>
<div style="text-align: center;">
    Face detection performance
</div>

More examples can be found in the [tutorial](tutorial)

## 5. models
Released models：[tdl_models](https://github.com/sophgo/tdl_models)

## 6. Development Guide
For more details, please refer to the [TDL SDK Software Development Guide](https://doc.sophgo.com/cvitek-develop-docs/master/docs_latest_release/CV180x_CV181x/zh/01.software/TPU/TDL_SDK_Software_Development_Guide/build/html/index.html#)

## 7. SDK Issue Feedback
Lastly, if you have any questions or suggestions for improvements regarding the repository, please submit them through Issues. We welcome all forms of contributions, including documentation improvements, bug fixes, new feature additions, and more. By directly participating in the development and maintenance of the project, you can help us continuously improve. With your assistance, we aim to develop this project into a more comprehensive and user-friendly deep learning SDK library.