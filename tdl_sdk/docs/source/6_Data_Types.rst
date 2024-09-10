.. vim: syntax=rst

数据类型
=======================

CVI_TDL_Core
~~~~~~~~~~~~~~~~~~~~~~~~~

CVI_TDL_SUPPORTED_MODEL_E
-------------------------

【描述】

此enum定义TDL SDK中所有Deep Learning Model。下表为每个模型Id和其模型功能说明。

.. list-table::
   :widths: 2 6 3
   :header-rows: 1

   * - 模型Index
     - 模型ID
     - 说明

   * - 0
     - CVI_TDL_SUPPORTED_MODEL
     
       _RETINAFACE
     - 人脸侦测(RetinaFace)

   * - 1
     - CVI_TDL_SUPPORTED_MODEL
     
       _RETINAFACE_IR
     - 红外线人脸侦测(RetinaFace)

   * - 2
     - CVI_TDL_SUPPORTED_MODEL
     
       _RETINAFACE_HARDHAT
     - 安全帽人脸检测(RetinaFace)

   * - 3
     - CVI_TDL_SUPPORTED_MODEL
     
       _SCRFDFACE
     - 人脸侦测(ScrFD Face)

   * - 4
     - CVI_TDL_SUPPORTED_MODEL
     
       _THERMALFACE
     - 热显人脸侦测

   * - 5
     - CVI_TDL_SUPPORTED_MODEL
     
       _THERMALPERSON
     - 热显人体侦测

   * - 6
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACEATTRIBUTE
     - 人脸属性和人脸识别

   * - 7
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACERECOGNITION
     - 人脸识别

   * - 8
     - CVI_TDL_SUPPORTED_MODEL
     
       _MASKFACERECOGNITION
     - 戴口罩人脸识别

   * - 9
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACEQUALITY
     - 人脸质量

   * - 10
     - CVI_TDL_SUPPORTED_MODEL
     
       _MASKCLASSIFICATION
     - 人脸口罩识别

   * - 11
     - CVI_TDL_SUPPORTED_MODEL
     
       _HANDCLASSIFICATION
     - 手势识别

   * - 12
     - CVI_TDL_SUPPORTED_MODEL
     
       _HAND_KEYPOINT
     - 手势关键点侦测

   * - 13
     - CVI_TDL_SUPPORTED_MODEL
     
       _HAND_KEYPOINT_CLASSIFICATION
     - 手势关键点识别

   * - 14
     - CVI_TDL_SUPPORTED_MODEL
     
       _LIVENESS
     - 双目活体识别

   * - 15
     - CVI_TDL_SUPPORTED_MODEL
     
       _HAND_DETECTION
     - 手部侦测

   * - 16
     - CVI_TDL_SUPPORTED_MODEL
     
       _MOBILEDETV2_PERSON_VEHICLE
     - 人形及交通工具侦测

   * - 17
     - CVI_TDL_SUPPORTED_MODEL
     
       _MOBILEDETV2_VEHICLE
     - 交通工具侦测

   * - 18
     - CVI_TDL_SUPPORTED_MODEL
     
       _MOBILEDETV2_PEDESTRIAN
     - 行人侦测

   * - 19
     - CVI_TDL_SUPPORTED_MODEL
     
       _MOBILEDETV2_PERSON_PETS
     - 猫狗及人型侦测

   * - 20
     - CVI_TDL_SUPPORTED_MODEL
     
       _MOBILEDETV2_COCO80
     - 80类对象侦测

   * - 21
     - CVI_TDL_SUPPORTED_MODEL
     
       _YOLOV3
     - 80类对象侦测

   * - 22
     - CVI_TDL_SUPPORTED_MODEL
     
       _YOLOV5
     - 80类对象侦测

   * - 23
     - CVI_TDL_SUPPORTED_MODEL
     
       _YOLOX
     - 80类对象侦测

   * - 24
     - CVI_TDL_SUPPORTED_MODEL
     
       _OSNET
     - 行人重识 - 0别

   * - 25
     - CVI_TDL_SUPPORTED_MODEL
     
       _SOUNDCLASSIFICATION
     - 声音识别

   * - 26
     - CVI_TDL_SUPPORTED_MODEL
     
       _SOUNDCLASSIFICATION_V2
     - 声音识别 V2

   * - 27
     - CVI_TDL_SUPPORTED_MODEL
     
       _WPODNET
     - 车牌侦测

   * - 28
     - CVI_TDL_SUPPORTED_MODEL
     
       _LPRNET_TW
     - 台湾地区车牌识别

   * - 29
     - CVI_TDL_SUPPORTED_MODEL
     
       _LPRNET_CN
     - 大陆地区车牌识别

   * - 30
     - CVI_TDL_SUPPORTED_MODEL
     
       _DEEPLABV3
     - 语意分割

   * - 31
     - CVI_TDL_SUPPORTED_MODEL
     
       _ALPHAPOSE
     - 人体关键点侦测

   * - 32
     - CVI_TDL_SUPPORTED_MODEL
     
       _EYECLASSIFICATION
     - 闭眼识别

   * - 33
     - CVI_TDL_SUPPORTED_MODEL
     
       _YAWNCLASSIFICATION
     - 打哈欠识别

   * - 34
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACELANDMARKER
     - 人脸关键点侦测

   * - 35
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACELANDMARKERDET2
     - 人脸关键点侦测2

   * - 36
     - CVI_TDL_SUPPORTED_MODEL
     
       _INCAROBJECTDETECTION
     - 车内对象识别

   * - 37
     - CVI_TDL_SUPPORTED_MODEL
       
       _SMOKECLASSIFICATION
     - 抽烟识别

   * - 38
     - CVI_TDL_SUPPORTED_MODEL
     
       _FACEMASKDETECTION
     - 口罩人脸侦测

   * - 39
     - CVI_TDL_SUPPORTED_MODEL
     
       _IRLIVENESS
     - 红外线活体侦测

   * - 40
     - CVI_TDL_SUPPORTED_MODEL
     
       _PERSON_PETS_DETECTION
     - 人形及猫狗侦测

   * - 41
     - CVI_TDL_SUPPORTED_MODEL
     
       _PERSON_VEHICLE_DETECTION
     - 人形及车辆侦测

   * - 42
     - CVI_TDL_SUPPORTED_MODEL
     
       _HAND_FACE_PERSON_DETECTION
     - 手部、脸及人型侦测

   * - 43
     - CVI_TDL_SUPPORTED_MODEL
     
       _HEAD_PERSON_DETECTION
     - 手部及人型侦测

   * - 44
     - CVI_TDL_SUPPORTED_MODEL
     
       _YOLOV8POSE
     - 姿态侦测

   * - 45
     - CVI_TDL_SUPPORTED_MODEL
     
       _SIMCC_POSE
     - 姿态侦测

   * - 46
     - CVI_TDL_SUPPORTED_MODEL
     
       _LANDMARK_DET3
     - 人脸关键点侦测

下表为每个模型Id对应的模型档案及推理使用的function：

.. list-table::
   :widths: 5 30 30
   :header-rows: 1

   * - 模型Index
     - Inference  Function
     - 模型档案

   * - 0
     - CVI_TDL_RetinaFace
     - retinaface_mnet0.25_342_608.cvimodel

       retinaface_mnet0.25_608_342.cvimodel

       retinaface_mnet0.25_608.cvimodel

   * - 1
     - CVI_TDL_RetinaFace_IR
     - retinafaceIR_mnet0.25_342_608.cvimodel

       retinafaceIR_mnet0.25_608_342.cvimodel

       retinafaceIR_mnet0.25_608_608.cvimodel

   * - 2
     - CVI_TDL_RetinaFace_Hardhat
     - hardhat_720_1280.cvimodel

   * - 3
     - CVI_TDL_ScrFDFace
     - scrfd_320_256_ir.cvimodel

       scrfd_480_270_int8.cvimodel

       scrfd_480_360_int8.cvimodel

       scrfd_500m_bnkps_432_768.cvimodel

       scrfd_768_432_int8_1x.cvimodel

   * - 4
     - CVI_TDL_ThermalFace
     - thermalfd-v1.cvimodel

   * - 5
     - CVI_TDL_ThermalPerson
     - thermal_person_detection.cvimodel

   * - 6
     - CVI_TDL_FaceAttribute  CVI_TDL_FaceAttributeOne
     - cviface-v3-attribute.cvimodel

   * - 7
     - CVI_TDL_FaceRecognition  CVI_TDL_FaceRecognitionOne
     - cviface-v4.cvimodel

       cviface-v5-m.cvimodel

       cviface-v5-s.cvimodel

       cviface-v6-s.cvimodel

   * - 8
     - CVI_TDL_MaskFaceRecognition
     - masked-fr-v1-m.cvimodel

   * - 9
     - CVI_TDL_FaceQuality
     - fqnet-v5_shufflenetv2-softmax.cvimodel

   * - 10
     - CVI_TDL_MaskClassification
     - mask_classifier.cvimodel

   * - 11
     - CVI_TDL_HandClassification
     - hand_cls_128x128.cvimodel

   * - 12
     - CVI_TDL_HandKeypoint
     - hand_kpt_128x128.cvimodel

   * - 13
     - CVI_TDL_HandKeypointClassification
     - hand_kpt_cls9.cvimodel

   * - 14
     - CVI_TDL_Liveness
     - liveness-rgb-ir.cvimodel

   * - 15
     - CVI_TDL_Hand_Detection
     - hand_det_qat_640x384.cvimodel

   * - 16
     - CVI_TDL_MobileDetV2_Vehicle
     - mobiledetv2-vehicle-d0-ls.cvimodel

   * - 17
     - CVI_TDL_MobileDetV2_Pedestrian
     - mobiledetv2-pedestrian-d0-ls-384.cvimodel

       mobiledetv2-pedestrian-d0-ls-640.cvimodel

       mobiledetv2-pedestrian-d0-ls-768.cvimodel

       mobileDetV2-pedestrian-d1-ls.cvimodel

       mobiledetv2-pedestrian-d1-ls-1024.cvimodel

   * - 18
     - CVI_TDL_MobileDetV2_Person
     
       _Vehicle
     - mobiledetv2-person-vehicle-ls-768.cvimodel

       mobiledetv2-person-vehicle-ls.cvimodel

   * - 19
     - CVI_TDL_MobileDetV2_Person_Pets
     - mobiledetv2-lite-person-pets-ls.cvimodel

   * - 20
     - CVI_TDL_MobileDetV2_COCO80
     - mobiledetv2-d0-ls.cvimodel

       mobiledetv2-d1-ls.cvimodel

       mobiledetv2-d2-ls.cvimodel

   * - 21
     - CVI_TDL_Yolov3
     - yolo_v3_416.cvimodel

   * - 22
     - CVI_TDL_Yolov5
     - yolov5s_3_branch_int8.cvimodel

   * - 23
     - CVI_TDL_YoloX
     - yolox_nano.cvimodel

       yolox_tiny.cvimodel

   * - 24
     - CVI_TDL_OSNet  CVI_TDL_OSNetOne
     - person-reid-v1.cvimodel

   * - 25
     - CVI_TDL_SoundClassification
     - es_classification.cvimodel

       soundcmd_bf16.cvimodel

   * - 26
     - CVI_TDL_SoundClassification_V2
     - c10_lightv2_mse40_mix.cvimodel

   * - 27
     - CVI_TDL_LicensePlateDetection
     - wpodnet_v0_bf16.cvimodel

   * - 28
     - CVI_TDL_LicensePlateRecognition_TW
     - lprnet_v0_tw_bf16.cvimodel

   * - 29
     - CVI_TDL_LicensePlateRecognition_CN
     - lprnet_v1_cn_bf16.cvimodel

   * - 30
     - CVI_TDL_DeeplabV3
     - deeplabv3_mobilenetv2_640x360.cvimodel

   * - 31
     - CVI_TDL_AlphaPose
     - alphapose.cvimodel

   * - 32
     - CVI_TDL_EyeClassification
     - eye_v1_bf16.cvimodel

   * - 33
     - CVI_TDL_YawnClassification
     - yawn_v1_bf16.cvimodel

   * - 34
     - CVI_TDL_FaceLandmarker
     - face_landmark_bf16.cvimodel

   * - 35 
     - CVI_TDL_FaceLandmarkerDet2
     - pipnet_blurness_v5_64_retinaface
     
       _50ep.cvimodel

   * - 36
     - CVI_TDL_IncarObjectDetection
     - incar_od_v0_bf16.cvimodel

   * - 37
     - CVI_TDL_SmokeClassification
     - N/A

   * - 38
     - CVI_TDL_FaceMaskDetection
     - retinaface_yolox_fdmask.cvimodel

   * - 39
     - CVI_TDL_IrLiveness
     - liveness-rgb-ir.cvimodel

       liveness-rgb-ir-3d.cvimodel

   * - 40
     - CVI_TDL_PersonPet_Detection
     - pet_det_640x384.cvimodel

   * - 41
     - CVI_TDL_PersonVehicleDetection
     - yolov8n_384_640_person

       _vehicle.cvimodel

   * - 42
     - CVI_TDL_HandFacePerson_Detection
     - meeting_det_640x384.cvimodel

   * - 43
     - CVI_TDL_HeadPerson_Detection
     - yolov8n_headperson.cvimodel

   * - 44
     - CVI_TDL_Yolov8_Pose
     - yolov8n_pose_384_640.cvimodel

   * - 45
     - CVI_TDL_Simcc_Pose
     - simcc_mv2_pose.cvimodel

   * - 46
     - CVI_TDL_FLDet3
     - onet_int8.cvimodel

.. _cvtdl_obj_class_id_e: 

cvtdl_obj_class_id_e
--------------------

【描述】

此enum定义对象侦测类别。每一类别归属于一个类别群组。

.. list-table::
   :widths: 4 3
   :header-rows: 1

   * - 类别
     - 类别群组

   * - CVI_TDL_DET_TYPE_PERSON
     - CVI_TDL_DET_GROUP_PERSON

   * - CVI_TDL_DET_TYPE_BICYCLE
     - CVI_TDL_DET_GROUP_VEHICLE

   * - CVI_TDL_DET_TYPE_CAR
     -

   * - CVI_TDL_DET_TYPE_MOTORBIKE
     -

   * - CVI_TDL_DET_TYPE_AEROPLANE
     -

   * - CVI_TDL_DET_TYPE_BUS
     -

   * - CVI_TDL_DET_TYPE_TRAIN
     -

   * - CVI_TDL_DET_TYPE_TRUCK
     -

   * - CVI_TDL_DET_TYPE_BOAT
     -

   * - CVI_TDL_DET_TYPE_TRAFFIC_LIGHT

     - CVI_TDL_DET_GROUP
     
       _OUTDOOR

   * - CVI_TDL_DET_TYPE_FIRE_HYDRANT
     -

   * - CVI_TDL_DET_TYPE_STREET_SIGN
     -

   * - CVI_TDL_DET_TYPE_STOP_SIGN
     -

   * - CVI_TDL_DET_TYPE_PARKING_METER

     -

   * - CVI_TDL_DET_TYPE_BENCH
     -

   * - CVI_TDL_DET_TYPE_BIRD
     - CVI_TDL_DET_GROUP_ANIMAL

   * - CVI_TDL_DET_TYPE_CAT
     -

   * - CVI_TDL_DET_TYPE_DOG
     -

   * - CVI_TDL_DET_TYPE_HORSE
     -

   * - CVI_TDL_DET_TYPE_SHEEP
     -

   * - CVI_TDL_DET_TYPE_COW
     -

   * - CVI_TDL_DET_TYPE_ELEPHANT
     -

   * - CVI_TDL_DET_TYPE_BEAR
     -

   * - CVI_TDL_DET_TYPE_ZEBRA
     -

   * - CVI_TDL_DET_TYPE_GIRAFFE
     -

   * - CVI_TDL_DET_TYPE_HAT
     - CVI_TDL_DET_GROUP
     
       _ACCESSORY

   * - CVI_TDL_DET_TYPE_BACKPACK
     -

   * - CVI_TDL_DET_TYPE_UMBRELLA
     -

   * - CVI_TDL_DET_TYPE_SHOE
     -

   * - CVI_TDL_DET_TYPE_EYE_GLASSES
     -

   * - CVI_TDL_DET_TYPE_HANDBAG
     -

   * - CVI_TDL_DET_TYPE_TIE
     -

   * - CVI_TDL_DET_TYPE_SUITCASE
     -

   * - CVI_TDL_DET_TYPE_FRISBEE
     - CVI_TDL_DET_GROUP_SPORTS

   * - CVI_TDL_DET_TYPE_SKIS
     -

   * - CVI_TDL_DET_TYPE_SNOWBOARD
     -

   * - CVI_TDL_DET_TYPE_SPORTS_BALL
     -

   * - CVI_TDL_DET_TYPE_KITE
     -

   * - CVI_TDL_DET_TYPE_BASEBALL_BAT
     -

   * - CVI_TDL_DET_TYPE_BASEBALL_GLOVE
     -

   * - CVI_TDL_DET_TYPE_SKATEBOARD
     -

   * - CVI_TDL_DET_TYPE_SURFBOARD
     -

   * - CVI_TDL_DET_TYPE_TENNIS_RACKET
     -

   * - CVI_TDL_DET_TYPE_BOTTLE
     - CVI_TDL_DET_GROUP_KITCHEN

   * - CVI_TDL_DET_TYPE_PLATE
     -

   * - CVI_TDL_DET_TYPE_WINE_GLASS
     -

   * - CVI_TDL_DET_TYPE_CUP
     -

   * - CVI_TDL_DET_TYPE_FORK
     -

   * - CVI_TDL_DET_TYPE_KNIFE
     -

   * - CVI_TDL_DET_TYPE_SPOON
     -

   * - CVI_TDL_DET_TYPE_BOWL
     -

   * - CVI_TDL_DET_TYPE_BANANA
     - CVI_TDL_DET_GROUP_FOOD

   * - CVI_TDL_DET_TYPE_APPLE
     -

   * - CVI_TDL_DET_TYPE_SANDWICH
     -

   * - CVI_TDL_DET_TYPE_ORANGE
     -

   * - CVI_TDL_DET_TYPE_BROCCOLI
     -

   * - CVI_TDL_DET_TYPE_CARROT
     -

   * - CVI_TDL_DET_TYPE_HOT_DOG
     -

   * - CVI_TDL_DET_TYPE_PIZZA
     -

   * - CVI_TDL_DET_TYPE_DONUT
     -

   * - CVI_TDL_DET_TYPE_CAKE
     -

   * - CVI_TDL_DET_TYPE_CHAIR
     - CVI_TDL_DET_GROUP
     
       _FURNITURE

   * - CVI_TDL_DET_TYPE_SOFA
     -

   * - CVI_TDL_DET_TYPE_POTTED_PLANT
     -

   * - CVI_TDL_DET_TYPE_BED
     -

   * - CVI_TDL_DET_TYPE_MIRROR
     -

   * - CVI_TDL_DET_TYPE_DINING_TABLE
     -

   * - CVI_TDL_DET_TYPE_WINDOW
     -

   * - CVI_TDL_DET_TYPE_DESK
     -

   * - CVI_TDL_DET_TYPE_TOILET
     -

   * - CVI_TDL_DET_TYPE_DOOR
     -

   * - CVI_TDL_DET_TYPE_TV_MONITOR
     - CVI_TDL_DET_GROUP
     
       _ELECTRONIC

   * - CVI_TDL_DET_TYPE_LAPTOP
     -

   * - CVI_TDL_DET_TYPE_MOUSE
     -

   * - CVI_TDL_DET_TYPE_REMOTE
     -

   * - CVI_TDL_DET_TYPE_KEYBOARD
     -

   * - CVI_TDL_DET_TYPE_CELL_PHONE
     -

   * - CVI_TDL_DET_TYPE_MICROWAVE
     - CVI_TDL_DET_GROUP
     
       _APPLIANCE

   * - CVI_TDL_DET_TYPE_OVEN
     -

   * - CVI_TDL_DET_TYPE_TOASTER
     -

   * - CVI_TDL_DET_TYPE_SINK
     -

   * - CVI_TDL_DET_TYPE_REFRIGERATOR
     -

   * - CVI_TDL_DET_TYPE_BLENDER
     -

   * - CVI_TDL_DET_TYPE_BOOK
     - CVI_TDL_DET_GROUP_INDOOR

   * - CVI_TDL_DET_TYPE_CLOCK
     -

   * - CVI_TDL_DET_TYPE_VASE
     -

   * - CVI_TDL_DET_TYPE_SCISSORS
     -

   * - CVI_TDL_DET_TYPE_TEDDY_BEAR
     -

   * - CVI_TDL_DET_TYPE_HAIR_DRIER
     -

   * - CVI_TDL_DET_TYPE_TOOTHBRUSH
     -

   * - CVI_TDL_DET_TYPE_HAIR_BRUSH
     -

.. _cvtdl_obj_det_group_type_e: 

cvtdl_obj_det_group_type_e
--------------------------

【描述】

此enum定义对象类别群组。

.. list-table::
   :widths: 2 1
   :header-rows: 1

   * - 类别群组
     - 描述

   * - CVI_TDL_DET_GROUP_ALL
     - 全部类别

   * - CVI_TDL_DET_GROUP_PERSON
     - 人形

   * - CVI_TDL_DET_GROUP_VEHICLE
     - 交通工具

   * - CVI_TDL_DET_GROUP_OUTDOOR
     - 户外

   * - CVI_TDL_DET_GROUP_ANIMAL
     - 动物

   * - CVI_TDL_DET_GROUP_ACCESSORY
     - 配件

   * - CVI_TDL_DET_GROUP_SPORTS
     - 运动

   * - CVI_TDL_DET_GROUP_KITCHEN
     - 厨房

   * - CVI_TDL_DET_GROUP_FOOD
     - 食物

   * - CVI_TDL_DET_GROUP_FURNITURE
     - 家具

   * - CVI_TDL_DET_GROUP_ELECTRONIC
     - 电子设备

   * - CVI_TDL_DET_GROUP_APPLIANCE
     - 器具

   * - CVI_TDL_DET_GROUP_INDOOR
     - 室内用品

   * - CVI_TDL_DET_GROUP_MASK_HEAD
     - 自订类别

   * - CVI_TDL_DET_GROUP_MASK_START
     - 自订类别开始

   * - CVI_TDL_DET_GROUP_MASK_END
     - 自订类别结束

feature_type_e
--------------

【enum】

.. list-table::
   :widths: 1 1 2
   :header-rows: 1

   * - 数值
     - 参数名称
     - 描述

   * - 0
     - TYPE_INT8
     - int8_t特征类型

   * - 1
     - TYPE_UINT8
     - uint8_t特征类型

   * - 2
     - TYPE_INT16
     - int16_t特征类型

   * - 3
     - TYPE_UINT16
     - uint16_t特征类型

   * - 4
     - TYPE_INT32
     - int32_t特征类型

   * - 5
     - TYPE_UINT32
     - uint32_t特征类型

   * - 6
     - TYPE_BF16
     - bf16特征类型

   * - 7
     - TYPE_FLOAT
     - float特征类型

meta_rescale_type_e
-------------------

【enum】

.. list-table::
   :widths: 1 2 2
   :header-rows: 1

   * - 数值
     - 参数名称
     - 描述

   * - 0
     - RESCALE_UNKNOWN
     - 未知

   * - 1
     - RESCALE_NOASPECT
     - 不依比例直接调整

   * - 2
     - RESCALE_CENTER
     - 在四周进行padding

   * - 3
     - RESCALE_RB
     - 在右下进行padding

cvtdl_bbox_t
------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - x1
     - 侦测框左上点坐标之 x 值

   * - float
     - y1
     - 侦测框左上点坐标之 y 值

   * - float
     - x2
     - 侦测框右下点坐标之 x 值

   * - float
     - y2
     - 侦测框右下点坐标之 y 值

   * - float
     - score
     - 侦测框之信心程度

.. _cvtdl_feature_t: 

cvtdl_feature_t
---------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - int8_t\*
     - ptr
     - 地址

   * - uint32_t
     - size
     - 特征维度

   * - feature_type_e
     - type
     - 特征型态

.. _cvtdl_pts_t: 

cvtdl_pts_t
-----------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float\*
     - x
     - 坐标x

   * - float\*
     - y
     - 坐标y

   * - uint32_t
     - size
     - 坐标点个数

cvtdl_4_pts_t
-------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - x[4]
     - 4个坐标点之x坐标值

   * - float
     - y[4]
     - 4个坐标点之y坐标值

cvtdl_vpssconfig_t
------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - VPSS_SCALE_COEF_E
     - chn_coeff
     - Rescale方式

   * - VPSS_CHN_ATTR_S
     - chn_attr
     - VPSS属性数据

.. _cvtdl_tracker_t: 

cvtdl_tracker_t
---------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - size
     - 追踪讯息数量

   * - cvtdl_tracker_info_t\*
     - info
     - 追踪讯息结构

cvtdl_tracker_info_t
--------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - cvtdl_trk_state_type_t
     - state
     - 追踪状态

   * - cvtdl_bbox_t
     - bbox
     - 追踪预测之边界框

cvtdl_trk_state_type_t
----------------------

【enum】

.. list-table::
   :widths: 1 2 2
   :header-rows: 1

   * - 数值
     - 参数名称
     - 描述

   * - 0
     - CVI_TRACKER_NEW
     - 追踪状态为新增

   * - 1
     - CVI_TRACKER_UNSTABLE
     - 追踪状态为不稳定

   * - 2
     - CVI_TRACKER_STABLE
     - 追踪状态为稳定

cvtdl_deepsort_config_t
-----------------------

.. list-table::
   :widths: 2 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - max_distance_iou
     - 进行BBox匹配时最大IOU距离

   * - float
     - ma x_distance_consine
     - 进行Feature匹配时最大consine距离

   * - int
     - max_unmatched_times_for
     
       _bbox_matching
     - 参与BBox匹配的目标最大未匹配次数之数量

   * - bool
     - enable_internal_FQ
     - 启用内部特征品质

   * - cvtdl_kalman_filter_config_t
     - kfilter_conf
     - Kalman Filter设定

   * - cvtdl_kalman_tracker
   
       _config_t
     - ktracker_conf
     - Kalman Tracker 设定

cvtdl_kalman_filter_config_t
----------------------------

.. list-table::
   :widths: 2 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - bool
     - enable_X_constraint_0
     - 启用第 0 个 X 约束

   * - bool
     - enable_X_constraint_1
     - 启用第 1 个 X 约束

   * - float
     - X_constraint_min[8]
     - X 约束下限

   * - float
     - X_constraint_max[8]
     - X 约束上限

   * - bool
     - enable_bounding_stay
     - 保留边界

   * - mahalanobis_confidence_e
     - confidence_level
     - 马氏距离信心度

   * - float
     - chi2_threshold
     - 卡方阈值

   * - float
     - Q_std_alpha[8]
     - Process Noise 参数

   * - float
     - Q_std_beta[8]
     - Process Noise 参数

   * - int
     - Q_std_x_idx[8]
     - Process Noise 参数

   * - float
     - R_std_alpha[4]
     - Measurement Noise 参数

   * - float
     - R_std_beta[4]
     - Measurement Noise 参数

   * - int
     - R_std_x_idx[4]
     - Measurement Noise 参数

【描述】

对于追踪目标运动状态X

Process Nose (运动偏差), Q, 其中

:math:`Q\lbrack i\rbrack = \left( {Alpha}_{Q}\lbrack i\rbrack \bullet X\left\lbrack {Idx}_{Q}\lbrack i\rbrack \right\rbrack + {Beta}_{Q}\lbrack i\rbrack \right)^{2}`

Measurement Nose (量测偏差), R, 同理运动偏差公式

cvtdl_kalman_tracker_config_t
-----------------------------

.. list-table::
   :widths: 1 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - int
     - max_unmatched_num
     - 追踪目标最大遗失数

   * - int
     - accreditation_threshold
     - 追踪状态转为稳定之阀值

   * - int
     - feature_budget_size
     - 保存追踪目标feature之最大数量

   * - int
     - feature_update_interval
     - 更新feature之时间间距

   * - bool
     - enable_QA_feature_init
     - 启用 QA 特征初始化

   * - bool
     - enable_QA_feature_update
     - 启用 QA 特征更新

   * - float
     - feature_init_quality_threshold
     - 特征初始化品质阈值

   * - float
     - feature_update_quality_threshold
     - 特征更新品质阈值

   * - float
     - P_std_alpha[8]
     - Initial Covariance 参数

   * - float
     - P_std_beta[8]
     - Initial Covariance 参数

   * - int
     - P_std_x_idx[8]
     - Initial Covariance 参数

【描述】

Initial Covariance (初始运动状态偏差), P, 同理运动偏差公式

cvtdl_liveness_ir_position_e
----------------------------

【enum】

.. list-table::
   :widths: 1 2 2
   :header-rows: 1

   * - 数值
     - 参数名称
     - 描述

   * - 0
     - LIVENESS_IR_LEFT
     - IR镜头在RGB镜头左侧

   * - 1
     - LIVENESS_IR_RIGHT
     - IR镜头在RGB镜头右侧

cvtdl_head_pose_t
-----------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - yaw
     - 偏摆角

   * - float
     - pitch
     - 俯仰角

   * - float
     - roll
     - 翻滚角

   * - float
     - facialUnitNormalVector[3]
     - 脸部之面向方位

.. _cvtdl_face_info_t: 

cvtdl_face_info_t
-----------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - char
     - name[128]
     - 人脸名

   * - uint64_t
     - unique_id
     - 人脸ID

   * - cvtdl_bbox_t
     - bbox
     - 人脸侦测框

   * - cvtdl_pts_t
     - pts
     - 人脸特征点

   * - cvtdl_feature_t
     - feature
     - 人脸特征

   * - cvtdl_face_emotion_e
     - emotion
     - 表情

   * - cvtdl_face_gender_e
     - gender
     - 性别

   * - cvtdl_face_race_e
     - race
     - 种族

   * - float
     - score
     - 分数

   * - float
     - age
     - 年龄

   * - float
     - liveness_score
     - 活体机率值

   * - float
     - hardhat_score
     - 安全帽机率值

   * - float
     - mask_score
     - 人脸戴口罩机率值

   * - float
     - recog_score
     - 识别分数

   * - float
     - face_quality
     - 人脸品质

   * - float
     - pose_score
     - 姿势分数

   * - float
     - pose_score1
     - 姿势分数

   * - float
     - sharpness_score
     - 清晰度分数

   * - float
     - blurness
     - 模糊性

   * - cvtdl_head_pose_t
     - head_pose
     - 人脸角度信息

   * - int
     - track_state
     - 追踪状态

.. _cvtdl_face_t: 

cvtdl_face_t
------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - size
     - 人脸个数

   * - uint32_t
     - width
     - 原始图片之宽

   * - uint32_t
     - height
     - 原始图片之高

   * - meta_rescale_type_e\*
     - rescale_type
     - rescale的形态

   * - cvtdl_face_info_t\*
     - info
     - 人脸综合信息

   * - cvtdl_dms_t\*
     - dms
     - 駕駛综合信息

cvtdl_pose17_meta_t
-------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - x[17]
     - 17个骨骼关键点的x坐标

   * - float
     - y[17]
     - 17个骨骼关键点的y坐标

   * - float
     - score[17]
     - 17个骨骼关键点的预测信心值

cvtdl_vehicle_meta
------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - cvtdl_4_pts_t
     - license_pts
     - 车牌4个角坐标

   * - cvtdl_bbox_t
     - license_bbox
     - 车牌边界框

   * - char[125]
     - license_char
     - 车牌号码

【描述】

车牌4个角坐标依序为左上、右上、右下至左下。

cvtdl_class_filter_t
--------------------

.. list-table::
   :widths: 1 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t\*
     - preserved_class_ids
     - 要保留的类别id

   * - uint32_t
     - num_preserved_classes
     - 要保留的类别id个数

cvtdl_dms_t
-----------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - reye_score
     - 右眼开合分数

   * - float
     - leye_score
     - 左眼开合分数

   * - float
     - yawn_score
     - 嘴巴闭合分数

   * - float
     - phone_score
     - 讲电话分数

   * - float
     - smoke_score
     - 抽烟分数

   * - cvtdl_pts_t
     - landmarks_106
     - 106个特征点

   * - cvtdl_pts_t
     - landmarks_5
     - 5个特征点

   * - cvtdl_head_pose_t
     - head_pose
     - 透过106个特征点算出来的人脸角度

   * - cvtdl_dms_od_t
     - dms_od
     - 车内的物件侦测结果

cvtdl_dms_od_t
--------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - size
     - 有几个物件

   * - uint32_t
     - width
     - 宽度

   * - uint32_t
     - height
     - 长度

   * - meta_rescale_type_e
     - rescale_type
     - rescale的形态

   * - cvtdl_dms_od_info_t\*
     - info
     - 物件的资讯

cvtdl_dms_od_info_t
-------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - char[128]
     - name
     - 物体名称

   * - int
     - classes
     - 物体类别

   * - cvtdl_bbox_t
     - bbox
     - 物体边界框

cvtdl_face_emotion_e
--------------------

【描述】

人脸表情

.. list-table::
   :widths: 2 1
   :header-rows: 1

   * - 表情
     - 描述

   * - EMOTION_UNKNOWN
     - 未知

   * - EMOTION_HAPPY
     - 高兴

   * - EMOTION_SURPRISE
     - 惊讶

   * - EMOTION_FEAR
     - 恐惧

   * - EMOTION_DISGUST
     - 厌恶

   * - EMOTION_SAD
     - 伤心

   * - EMOTION_ANGER
     - 生气

   * - EMOTION_NEUTRAL
     - 自然

cvtdl_face_race_e
-----------------

.. list-table::
   :widths: 2 1
   :header-rows: 1

   * - 种族
     - 描述

   * - RACE_UNKNOWN
     - 未知

   * - RACE_CAUCASIAN
     - 高加索人

   * - RACE_BLACK
     - 黑人

   * - RACE_ASIAN
     - 亚洲人

cvtdl_pedestrian_meta
---------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - cvtdl_pose17_meta_t
     - pose17
     - 人体17关键点

   * - bool
     - fall
     - 受否跌倒

.. _cvtdl_object_info_t: 

cvtdl_object_info_t
-------------------

.. list-table::
   :widths: 2 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - char
     - name
     - 对象类别名

   * - uint64_t
     - unique_id
     - 唯一 id

   * - cvtdl_box_t
     - bbox
     - 框的边界讯息

   * - cvtdl_feature_t
     - feature
     - 对象特征

   * - int
     - classes
     - 类别ID

   * - cvtdl_vehicle_meta
     - vehicle_property
     - 车辆属性

   * - cvtdl_pedestrian_meta
     - pedestrian_property
     - 行人属性

   * - int
     - track_state
     - 追踪状态

.. _cvtdl_object_t:

cvtdl_object_t
--------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - size
     - info所含物件个数

   * - uint32_t
     - width
     - 原始图片之宽

   * - uint32_t
     - height
     - 原始图片之高

   * - uint32_t
     - entry_num
     - entry数量

   * - uint32_t
     - miss_num
     - miss数量

   * - meta_rescale_type_e
     - rescale_type
     - 模型前处理采用的resize方式

   * - cvtdl_object_info_t\*
     - info
     - 物件信息

cvtdl_handpose21_meta_t
-----------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - xn[21]
     - 归一化 x 点

   * - float
     - x[21]
     - x 点

   * - float
     - yn[21]
     - 归一化 y 点

   * - float
     - y[21]
     - y 点

   * - float
     - bbox_x
     - 框的x 座标

   * - float
     - bbox_y
     - 框的y 座标

   * - float
     - bbox_w
     - 框的宽

   * - float
     - bbox_h
     - 框的高

   * - int
     - label
     - 手势类别

   * - float
     - score
     - 手势分数

cvtdl_handpose21_meta_ts
------------------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - size
     - 侦测到手的数量

   * - uint32_t
     - width
     - 图片宽

   * - uint32_t
     - height
     - 图片高

   * - cvtdl_handpose21_meta_t\*
     - info
     - 手部关键点

Yolov5PreParam
--------------

.. list-table::
   :widths: 2 2 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - float
     - factor[3]
     - 缩放因子

   * - float
     - mean[3]
     - 图像均值

   * - meta_rescale_type_e
     - rescale_type
     - 缩放模式

   * - bool\*
     - keep_aspect_ratio
     - 保持宽高比例缩放

   * - bool\*
     - use_crop
     - 裁剪调整图像大小

   * - VPSS_SCALE_COEF_E\*
     - resize_method
     - 缩放方法

   * - PIXEL_FORMAT_E\*
     - format
     - 图像格式

YOLOV5AlgParam
--------------

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - uint32_t
     - anchors[3][3][2]
     - 模型錨點

   * - float
     - conf_thresh
     - 信心度阀值

   * - float
     - nms_thresh
     - 均方根阀值

CVI_TDL_Service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cvtdl_service_feature_matching_e
--------------------------------

【描述】

特征比对计算方法，目前仅支持Cosine Similarity。

【定义】

.. list-table::
   :widths: 2 1
   :header-rows: 1

   * - 参数名称
     - 描述

   * - COS_SIMILARITY
     - Cosine similarity

cvtdl_service_feature_array_t
-----------------------------

【描述】

特征数组，此结构包含了特征数组指针, 长度, 特征个数, 及特征类型等信息。在注册特征库时需要传入此结构。

【定义】

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - int8_t\*
     - ptr
     - 特征数组指针

   * - uint32_t
     - feature_length
     - 单一特征长度

   * - uint32_t
     - data_num
     - 特征个数

   * - feature_type_e
     - type
     - 特征类型

cvtdl_service_brush_t
---------------------

【描述】

绘图笔刷结构，可指定欲使用之RGB及笔刷大小。

【定义】

.. list-table::
   :widths: 2 1 2
   :header-rows: 1

   * - 数据类型
     - 参数名称
     - 描述

   * - Inner structure
     - color
     - 欲使用的RGB值

   * - uint32_t
     - size
     - 笔刷大小

cvtdl_area_detect_e
-------------------

【enum】

.. list-table::
   :widths: 1 3 3
   :header-rows: 1

   * - 数值
     - 参数名称
     - 描述

   * - 0
     - UNKNOWN
     - int8_t特征类型

   * - 1
     - NO_INTERSECT
     - 不相交

   * - 2
     - ON_LINE
     - 在线上

   * - 3
     - CROSS_LINE_POS
     - 正向交叉

   * - 4
     - CROSS_LINE_NEG
     - 负向交叉

   * - 5
     - INSIDE_POLYGON
     - 在多边形内部

   * - 6
     - OUTSIDE_POLYGON
     - 在多边形外部
