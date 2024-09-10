.. vim: syntax=rst

应用(APP)
====================

目的
~~~~~~~~~~~~~~

CviTek TDL application，APP 是基于TDL SDK，并针对不同客户端应用，所设计的solution。

APP 整合TDL SDK，提供客户更方便的操作API。

APP code为open source，可以作为客户端开发的参考。

【编译】

1. 下载TDL SDK与其依赖之SDK：MW、TPU、IVE。

2. 移动至TDL SDK的module/app目录

3. 执行以下指令：

.. code-block:: console

   make MW_PATH=<MW_SDK> TPU_PATH=<TPU_SDK> IVE_PATH=<IVE_SDK>

   make install

   make clean

编译完成的lib会放在TDL SDK的tmp_install目录下

API
~~~~~~~~~~~~~~~~~~~

句柄
^^^^^^^^^^^^^^^^^^

【语法】

.. code-block:: c

  typedef struct {

  cvitdl_handle_t tdl_handle;

  IVE_HANDLE ive_handle;

  face_capture_t *face_cpt_info;

  } cvitdl_app_context_t;

  typedef cvitdl_app_context *cvitdl_app_handle_t;

【描述】

cvitdl_app_handle_t为cvitdl_app_context的指针型态，其中包含tdl handle、ive handle与其他应用之数据结构。

CVI_TDL_APP_CreateHandle
------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_CreateHandle(cvitdl_app_handle_t *handle, cvitdl_handle_t tdl_handle);

【描述】

创建使用APP所需的指标。需输入tdl handle。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输出
     - cvitdl_app_handle_t\*
     - handle
     - 输入句柄指标

   * - 输入
     - cvitdl_handle_t
     - a i_handle
     - TDL句柄

CVI_TDL_APP_DestroyHandle
-------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_DestroyHandle(cvitdl_app_handle_t handle);

【描述】

销毁创造的句柄cvitdl_app_handle_t。

只会销毁个别应用程序所使用之数据，不影响tdl handle。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

人脸抓拍
^^^^^^^^^^^^^^^^^^

人脸抓拍 (Face Capture) 结合人脸侦测、多对象追踪、人脸质量检测，功能为抓拍 (或截取) 影像中不同人的脸部照片。

抓拍条件可利用配置文件来调整，例如：抓拍时间点、人脸质量检测算法、人脸角度阀值…。

【配置文件】

.. list-table::
   :widths: 2 1 3
   :header-rows: 1

   * - 参数名称
     - 默认值
     - 说明

   * - Miss_Time_Limit
     - 40
     - 人脸遗失时间限制。当APP连续无法追踪到某个face，会判定此 face已离开。

       [单位：frame]

   * - Threshold_Size_Min
     - 32
     - 最小/最大可接受人脸大小，如果face bbox的任一边小于/大于此阀值，quality会强制设为0。

   * - Threshold_Size_Max
     - 512
     -

   * - Quality_Assessment_Method
     - 0
     - 若人脸评估不使用FQNet时，启用内建质量检测算法

       0: 基于人脸大小与角度

       1: 基于眼睛距离

   * - Threshold_Quality
     - 0.1
     - 人脸质量阀值，若新的face的quality大于此阀值，且比当前截取之face的quality还高，则会截取并更新暂存区face数据。

   * - Threshold_Quality_High
     - 0.95
     - 人脸质量阀值（高），若暂存区某 face的quality高于此阀值，则判定此 face 为高质量，后续不会再进行更新。

       （仅适用于level 2,3）

   * - Threshold_Yaw
     - 0.25
     - 人脸角度阀值，若角度大于此阀值，quality会强制设为0。

       （一单位为90度）

   * - Threshold_Pitch
     - 0.25
     -

   * - Threshold_Roll
     - 0.25
     -

   * - FAST_Mode_Interval
     - 10
     - FAST模式抓拍间隔。

       [单位：frame]

   * - FAST_Mode_Capture_Num
     - 3
     - FAST模式抓拍次数。

   * - CYCLE_Mode_Interval
     - 20
     - CYCLE模式抓拍间隔。

       [单位：frame]

   * - AUTO_Mode_Time_Limit
     - 0
     - AUTO 模式最后输出的时限。

       [单位：frame]

   * - AUTO_Mode_Fast_Cap
     - 1
     - AUTO模式是否输出进行快速抓拍1次。

   * - Capture_Aligned_Face
     - 0
     - 抓拍/截取人脸是否进行校正。

【人脸品质检测算法】

.. list-table::
   :widths: 1 2 3
   :header-rows: 1

   * - #
     - 算法
     - 计算方式

   * - 0
     - 基于人脸大小与角度
     -
       1. Face Area Score

         1. 定义标准人脸大小A_base = 112 * 112

         2. 计算侦测到的人脸面积A_face = 长 * 宽

         3. 计算MIN(1.0, A_face/A_base) 作为分数

       2. Face Pose Score

         1. 分别计算人脸角度 yaw, pitch, roll并取其绝对值

         2. 计算1 - (yaw + pitch + roll) / 3作为分数

       3. Face Quality = Face Area Score * Face Pose Score

   * - 1
     - 基于眼睛距离
     -
       1. 定义标准瞳距 D = 80
       2. 计算双眼距离 d
       3. 计算MIN(1.0, d/D) 当作分数

CVI_TDL_APP_FaceCapture_Init
----------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size);

【描述】

初始化人脸抓拍数据结构。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - uint32_t
     - buffer_size
     - 人脸暂存区大小

CVI_TDL_APP_FaceCapture_QuickSetUp
----------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_QuickSetUp(const cvitdl_app_handle_t handle, int fd_model_id, int fr_model_id, const char *fd_model_path, const char *fr_model_path, const char *fq_model_path, const char *fl_model_path);

【描述】

快速设定人脸抓拍。

【参数】

.. list-table::
   :widths: 1 2 2 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - int
     - fd_model_id
     - 人脸侦测模型ID

   * - 输入
     - int
     - fr_model_id
     - 人脸识别检测模型ID

   * - 输入
     - const char\*
     - fd_model_path
     - 人脸侦测模型路径

   * - 输入
     - const char\*
     - fr_model_path
     - 人脸识别检测模型路径

   * - 输入
     - const char\*
     - fq_model_path
     - 人脸质量检测模型路径

   * - 输入
     - const char\*
     - fl_model_path
     - 人脸座标检测模型路径

CVI_TDL_APP_FaceCapture_FusePedSetup
------------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_FusePedSetup(const cvitdl_app_handle_t handle, int ped_model_id, const char *ped_model_path);

【描述】

快速设定人脸抓拍。

【参数】

.. list-table::
   :widths: 1 2 2 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - int
     - fd_model_id
     - 行人侦测模型ID

   * - 输入
     - const char\*
     - fl_model_path
     - 行人侦测模型路径

CVI_TDL_APP_FaceCapture_GetDefaultConfig
----------------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_GetDefaultConfig(face_capture_config_t *cfg);

【描述】

取得人脸抓拍预设参数。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输出
     - face_capture_config_t\*
     - cfg
     - 人脸抓拍参数

CVI_TDL_APP_FaceCapture_SetConfig
---------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_SetConfig(const cvitdl_app_handle_t handle, face_capture_config_t *cfg);

【描述】

设定人脸抓拍参数。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t 
     - handle
     - 输入句柄指标

   * - 输入
     - face_capture_config_t\*
     - cfg
     - 人脸抓拍参数

CVI_TDL_APP_FaceCapture_FDFR
--------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_FDFR(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *p_face);

【描述】

设定人脸抓拍参数。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t 
     - handle
     - 输入句柄指标

   * - 输入
     - VIDEO_FRAME_INFO_S\*
     - frame
     - 图像

   * - 输入
     - cvtdl_face_t\*
     - p_face
     - 人脸抓拍输出结果

CVI_TDL_APP_FaceCapture_SetMode
-------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_SetMode(const cvitdl_app_handle_t handle, capture_mode_e mode);

【描述】

设定人脸抓拍模式。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - capture_mode_e
     - mode
     - 人脸抓拍模式

CVI_TDL_APP_FaceCapture_Run
---------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame);

【描述】

执行人脸抓拍。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - VIDEO_FRAME_INFO_S\*
     - frame
     - 输入影像

CVI_TDL_APP_FaceCapture_CleanAll
--------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_FaceCapture_CleanAll(const cvitdl_app_handle_t handle);

【描述】

清除所有人脸抓拍暂存区之数据数据。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

人型抓拍
^^^^^^^^^^^^^^^^^^

人型抓拍 (Face Capture) 结合人型侦测、多对象追踪、人脸质量检测，功能为抓拍 (或截取) 影像中不同人的脸部照片。

抓拍条件可利用配置文件来调整，例如：抓拍时间点、人脸质量检测算法、人脸角度阀值…。

【配置文件】

.. list-table::
   :widths: 2 1 3
   :header-rows: 1

   * - 参数名称
     - 说明
     - 说明

   * - Miss_Time_Limit
     - 40
     - 人脸遗失时间限制。当APP连续无法追踪到某个face，会判定此  face已离开。

       [单位：frame]

   * - Threshold_Size_Min
     - 32
     - 最小/最大可接受人脸大小，如果face  bbox的任一边小于      /大于此阀值，quality会强制设为0。

   * - Threshold_Size_Max
     - 512
     -

   * - Quality_Assessment_Method
     - 0
     - 若人脸评估不  使用FQNet时，启用内建质量检测算法

       0: 基于人脸大小与角度

       1: 基于眼睛距离

   * - Threshold_Quality
     - 0.1
     - 人脸质量阀值，若新的face的quality大于此阀值  ，且比当前截取之face的quality还高  ，则会截取并更新暂存区face数据。

   * - Threshold_Quality_High
     - 0.95
     - 人脸质量阀值（高），若暂存区某 face的quality高于此阀值，则判定此  face 为高质量，后  续不会再进行更新。

       （仅适用于level  2,3）

   * - Threshold_Yaw
     - 0.25
     - 人脸角度阀值，若角度大于此阀值，qua  lity会强制设为0。

       （一单位为90度）

   * - Threshold_Pitch
     - 0.25
     -

   * - Threshold_Roll
     - 0.25
     -

   * - FAST_Mode_Interval
     - 10
     - FAST模式抓拍间隔。

       [单位：frame]

   * - FAST_Mode_Capture_Num
     - 3
     - FAST模式抓拍次数。

   * - CYCLE_Mode_Interval
     - 20
     - CYCLE模式抓拍间隔。

       [单位：frame]

   * - AUTO_Mode_Time_Limit
     - 0
     - AUTO 模式最后输出的时限。

       [单位：frame]

   * - AUTO_Mode_Fast_Cap
     - 1
     - AUTO模式是否输出进行快速抓拍1次。

   * - Capture_Aligned_Face
     - 0
     - 抓拍/截取人脸是否进行校正。

【人脸品质检测算法】

.. list-table::
   :widths: 2 1 3
   :header-rows: 1

   * - #
     - 算法
     - 计算方式

   * - 0
     - 基于人脸大小与角度
     -
        1. Face Area Score

            1. 定义标准人脸大小A_base = 112 * 112

            2. 计算侦测到的人脸面积A_face = 长 * 宽

            3. 计算MIN(1.0, A_face/A_base) 作为分数

        2. Face Pose Score

            1. 分别计算人脸角度 yaw, pitch, roll并取其绝对值

            2. 计算1 - (yaw + pitch + roll) /   3作为分数

        3. Face Quality = Face Area Score * Face Pose Score

   * - 1
     - 基于眼睛距离
     -
       1. 定义标准瞳距 D = 80

       2. 计算双眼距离 d

       3. 计算MIN(1.0, d/D) 当作分数

CVI_TDL_APP_PersonCapture_Init
------------------------------

【语法】

.. code-block:: none

  CVI_TDL_APP_PersonCapture_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size);

【描述】

初始化人形抓拍数据结构。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - uint32_t
     - buffer_size
     - 人脸暂存区大小

CVI_TDL_APP_PersonCapture_QuickSetUp
------------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_QuickSetUp(const cvitdl_app_handle_t handle,

  const char *od_model_name,

  const char *od_model_path,

  const char *reid_model_path);

【描述】

快速设定人型抓拍。

【参数】

.. list-table::
   :widths: 1 2 2 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - const char\*
     - od_model_name
     - 人型侦测模型名称

   * - 输入
     - const char\*
     - od_model_path
     - 人型侦测模型路径

   * - 输入
     - const char\*
     - reid_model_path
     - ReID模型路径

CVI_TDL_APP_FaceCapture_GetDefaultConfig
----------------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_GetDefaultConfig(person_capture_config_t *cfg);

【描述】

取得人型抓拍预设参数。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输出
     - person_capture_config_t\*
     - cfg
     - 人型抓拍参数

CVI_TDL_APP_PersonCapture_SetConfig
-----------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_SetConfig(const cvitdl_app_handle_t handle, person_capture_config_t *cfg);

【描述】

设定人型抓拍参数。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - person_capture_config_t\*
     - cfg
     - 人型抓拍参数

CVI_TDL_APP_PersonCapture_SetMode
---------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_SetMode(const cvitdl_app_handle_t handle, capture_mode_e mode);

【描述】

设定人型抓拍模式。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - capture_mode_e
     - mode
     - 人型抓拍模式

CVI_TDL_APP_PersonCapture_Run
-----------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame);

【描述】

执行人型抓拍。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - VIDEO_FRAME_INFO_S\*
     - frame
     - 输入影像

CVI_TDL_APP_ConsumerCounting_Run
--------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_ConsumerCounting_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame);

【描述】

执行人型抓拍。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

   * - 输入
     - VIDEO_FRAME_INFO_S\*
     - frame
     - 输入影像

CVI_TDL_APP_PersonCapture_CleanAll
----------------------------------

【语法】

.. code-block:: c

  CVI_S32 CVI_TDL_APP_PersonCapture_ClanAll(const cvitdl_app_handle_t handle);

【描述】

清除所有人型抓拍暂存区之数据数据。

【参数】

.. list-table::
   :widths: 1 2 1 2
   :header-rows: 1

   * -
     - 数据型态
     - 参数名称
     - 说明

   * - 输入
     - cvitdl_app_handle_t
     - handle
     - 输入句柄指标

