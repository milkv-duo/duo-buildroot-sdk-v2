# Samples extra for CVI TPU SDK

##  Yolo detection series

The sample implementation of yolov3 without post-processing, yolov5 with post-processing and yolox is provided. Please refer to the detector_yolov3_fused_ preprocess, detector_yolov5_fused_Preprocess and detector_yolox_s Implementation under directory

##  Preprocess classification
Support the use of TPU or VPSS for pre-processing. Take the classification model as an example:

1. Refer to classifier for classifier_tpu_preprocess under preprocess directory \
The advantage is to reduce the memory copy of pre-processing, but it will increase the use of ion

2. Refer to classifier for classifier_vpss_preprocess under preprocess directory \
The advantage is that VPSS supports more types of preprocessing. It does not need to use TPU for additional preprocessing, but memory copying is required

##  Complex deployment scenario sample
1. for attitude evaluation, refer to alphapose_fused_preprocess directory
2. for face recognition detection, please refer to insightface_fused_preprocess directory

