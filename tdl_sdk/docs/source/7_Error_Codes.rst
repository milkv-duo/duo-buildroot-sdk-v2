.. vim: syntax=rst

错误码
================

.. list-table::
   :widths: 1 3 1
   :header-rows: 1

   * - 错误代码
     - 宏定义
     - 描述

   * - 0xFFFFFFFF
     - CVI_TDL_FAILURE
     - API 调用失败

   * - 0xC0010101
     - CVI_TDL_ERR_INVALID_MODEL_PATH
     - 不正确的模型路径

   * - 0xC0010102
     - CVI_TDL_ERR_OPEN_MODEL
     - 开启模型失败

   * - 0xC0010103
     - CVI_TDL_ERR_CLOSE_MODEL
     - 关闭模型失败

   * - 0xC0010104
     - CVI_TDL_ERR_GET_VPSS_CHN_CONFIG
     - 取得VPSS CHN设置失败

   * - 0xC0010105
     - CVI_TDL_ERR_INFERENCE
     - 模型推理失败

   * - 0xC0010106
     - CVI_TDL_ERR_INVALID_ARGS
     - 不正确的参数

   * - 0xC0010107
     - CVI_TDL_ERR_INIT_VPSS
     - 初始化VPSS失败

   * - 0xC0010108
     - CVI_TDL_ERR_VPSS_SEND_FRAME
     - 送Frame到VPSS时失败

   * - 0xC0010109
     - CVI_TDL_ERR_VPSS_GET_FRAME
     - 从VPSS取得Frame失败

   * - 0xC001010A
     - CVI_TDL_ERR_MODEL_INITIALIZED
     - 模型未开启

   * - 0xC001010B
     - CVI_TDL_ERR_NOT_YET_INITIALIZED
     - 功能未初始化

   * - 0xC001010C
     - CVI_TDL_ERR_NOT_YET_IMPLEMENTED
     - 功能尚未实现

   * - 0xC001010D
     - CVI_TDL_ERR_ALLOC_ION_FAIL
     - 分配ION内存失败

   * - 0xC0010201
     - CVI_TDL_ERR_MD_OPERATION_FAILED
     - 运行Motion Detection失败

