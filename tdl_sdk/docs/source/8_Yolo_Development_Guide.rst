.. vim: syntax=rst

Yolo通用推理接口使用文档
=========================

目的
---------------

算能端侧提供的集成yolo系列算法C++接口，
用以缩短外部开发者定制化部署yolo系列模型所需的时间。

TDL_SDK内部实现了yolo系列算法封装其前后处理和推理，
提供统一且便捷的编程接口。

目前TDL_SDK包括但不限于
yolov5,yolov6,yolov7,yolov8,yolox,ppyoloe,yolov10
等算法。



通用Yolov5模型部署
------------------


引言
~~~~~~~~~~~~~~~

本文档介绍了如何将yolov5架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolov5模型pytorch版本转换为onnx模型

* onnx模型转换为cvimodel格式

* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~

1.首先可以下载yolov5官方仓库代码，地址如下:
[ultralytics/yolov5: YOLOv5 🚀 in PyTorch > ONNX > CoreML > TFLite]
(https://github.com/ultralytics/yolov5)

.. code-block:: shell

  git clone https://github.com/ultralytics/yolov5.git

2.获取yolov5的.pt格式的模型，例如下载yolov5s模型的地址：
[yolov5s](https://github.com/ultralytics/yolov5/releases/download/v7.0/yolov5s.pt)

3.下载TDL_SDK提供的yolov5 onnx导出脚本：

* 官方导出方式的模型中解码部分不适合量化，因此需要使用TDL_SDK提供的导出方式

TDL_SDK 的导出方式可以直接使用yolo_export中的脚本，通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

下载之后，解压压缩包并将 yolo_export/yolov5_export.py 复制到yolov5仓库目录下

4.然后使用以下命令导出TDK_SDK版本的onnx模型

.. code-block:: shell

  #其中--weights表示权重文件的相对路径，--img-size表示输出尺寸为 640 x 640
  python yolov5_export.py --weights ./weigthts/yolov5s.pt --img-size 640 640
  #生成的onnx模型在当前目录


.. tip::
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，例如

.. code-block:: shell

   python yolov5_export.py --weights ./weights/yolov5s.pt --img-size 384 640

准备转模型环境
~~~~~~~~~~~~~~~

onnx转成cvimodel需要 TPU-MLIR 的发布包。TPU-MLIR 是算能TDL处理器的TPU编译器工程。

【TPU-MLIR工具包下载】
TPU-MLIR代码路径 https://github.com/sophgo/tpu-mlir，感兴趣的可称为开源开发者共同维护开源社区。

而我们仅需要对应的工具包即可，下载地位为算能官网的TPU-MLIR论坛，后面简称为工具链工具包:
(https://developer.sophgo.com/thread/473.html)

TPU-MLIR工程提供了一套完整的工具链, 其可以将不同框架下预训练的神经网络, 转化为可以在算能 TPU 上高效运算的文件。

目前支持onnx和Caffe模型直接转换，其他框架的模型需要转换成onnx模型，再通过TPU-MLIR工具转换。

转换模型需要在指定的docker执行，主要的步骤可以分为两步：

* 第一步是通过model_transform.py将原始模型转换为mlir文件

* 第二步是通过model_deploy.py将mlir文件转换成cvimodel

> 如果需要转换为INT8模型，还需要在第二步之前调用run_calibration.py生成校准表，然后传给model_deploy.py

【Docker配置】

TPU-MLIR需要在Docker环境开发，可以直接下载docker镜像(速度比较慢),参考如下命令：

.. code-block:: shell

  docker pull sophgo/tpuc_dev:latest

或者可以从【TPU工具链工具包】中下载的docker镜像(速度比较快)，然后进行加载docker。

.. code-block:: shell

  docker load -i  docker_tpuc_dev_v2.2.tar.gz

如果是首次使用Docker，可以执行下述命令进行安装和配置（仅首次执行）：

.. code-block:: shell

  sudo apt install docker.io
  sudo systemctl start docker
  sudo systemctl enable docker
  sudo groupadd docker
  sudo usermod -aG docker $USER
  newgrp docker

【进入docker环境】
确保安装包在当前目录，然后在当前目录创建容器如下：

.. code-block:: shell

  docker run --privileged --name myname -v $PWD:/workspace -it sophgo/tpuc_dev:v2.2

后续的步骤假定用户当前处在docker里面的/workspace目录

#### 加载tpu-mlir工具包&准备工作目录

以下操作需要在Docker容器执行

【解压tpu_mlir工具包】
以下文件夹创建主要是为了方便后续管理，也可按照自己喜欢的管理方式进行文件分类

新建一个文件夹tpu_mlir，将新工具链解压到tpu_mlir/目录下，并设置环境变量：

.. code-block:: shell

  ##其中tpu-mlir_xxx.tar.gz的xxx是版本号，根据对应的文件名而决定
  mkdir tpu_mlir && cd tpu_mlir
  cp tpu-mlir_xxx.tar.gz ./
  tar zxf tpu-mlir_xxx.tar.gz
  source tpu_mli_xxx/envsetup.sh

【拷贝onnx模型】
创建一个文件夹，以yolov5s举例，创建一个文件夹yolov5s，并将onnx模型放在yolov5s/onnx/路径下

.. code-block:: shell

  mkdir yolov5s && cd yolov5s
  ##上一节转出来的yolov5 onnx模型拷贝到yolov5s目录下
  cp yolov5s.onnx ./
  ## 拷贝官网的dog.jpg过来做校验。
  cp dog.jpg ./

上述准备工作完成之后，就可以开始转换模型

onnx转MLIR
~~~~~~~~~~~~~~~

如果模型是图片输入, 在转模型之前我们需要了解模型的预处理。

如果模型用预处理后的 npz 文件做输入, 则不需要考虑预处理。

本例子中yolov5的图片是rgb,mean和scale对应为:

* mean:  0.0, 0.0, 0.0
* scale: 0.0039216, 0.0039216, 0.0039216

模型转换的命令如下：

.. code-block:: shell

  model_transform.py \
  --model_name yolov5s \
  --model_def yolov5s.onnx \
  --input_shapes [[1,3,640,640]] \
  --mean 0.0,0.0,0.0 \
  --scale 0.0039216,0.0039216,0.0039216 \
  --keep_aspect_ratio \
  --pixel_format rgb \
  --test_input ./dog.jpg \
  --test_result yolov5s_top_outputs.npz \
  --mlir yolov5s.mlir

其中model_transform.py参数详情, 请参考【tpu_mlir_xxxxx/doc/TPU-MLIR快速入门指南】

转换成mlir文件之后，会生成一个yolov5s_in_f32.npz文件，该文件是模型的输入文件

MLIR转INT8模型
~~~~~~~~~~~~~~~

【生成校准表】

转 INT8 模型前需要跑 calibration，得到校准表；输入数据的数量根据情况准备 100~1000 张 左右。

然后用校准表，生成cvimodel.生成校对表的图片尽可能和训练数据分布相似

.. code-block:: shell

  ## 这个数据集从COCO2017提取100来做校准，用其他图片也是可以的，这里不做强制要求。
  run_calibration.py yolov5s.mlir \
  --dataset COCO2017 \
  --input_num 100 \
  -o yolov5s_cali_table

运行完成之后会生成名为yolov5_cali_table的文件，该文件用于后续编译cvimode模型的输入文件

【生成cvimodel】

然后生成int8对称量化cvimodel模型，执行如下命令：

其中--quant_output参数表示将输出层也量化为int8，不添加该参数则保留输出层为float32。

从后续测试结果来说，将输出层量化为int8，可以减少部分ion，并提高推理速度，
并且模型检测精度基本没有下降，推荐添加--quant_output参数

.. code-block:: shell

  model_deploy.py \
  --mlir yolov5s.mlir \
  --quant_input \
  --quant_output \
  --quantize INT8 \
  --calibration_table yolov5s_cali_table \
  --processor cv181x \
  --test_input yolov5s_in_f32.npz \
  --test_reference yolov5s_top_outputs.npz \
  --tolerance 0.85,0.45 \
  --model yolov5_cv181x_int8_sym.cvimodel

其中model_deploy.py的主要参数参考, 请参考【tpu_mlir_xxxxx/doc/TPU-MLIR快速入门指南】

编译完成后，会生成名为yolov5_cv181x_int8_sym.cvimodel的文件

在上述步骤运行成功之后，编译cvimodel的步骤就完成了，之后就可以使用TDL_SDK调用导出的cvimodel进行yolov5目标检测推理了。

.. caution:: 
  注意运行的对应平台要一一对应！

TDL_SDK接口说明
~~~~~~~~~~~~~~~


集成的yolov5接口开放了预处理的设置，yolov5模型算法的anchor，conf置信度以及nms置信度设置

预处理设置的结构体为YoloPreParam

.. code-block:: c

  /** @struct YoloPreParam
   *  @ingroup core_cvitdlcore
   *  @brief Config the yolov5 detection preprocess.
   *  @var YoloPreParam::factor
   *  Preprocess factor, one dimension matrix, r g b channel
   *  @var YoloPreParam::mean
   *  Preprocess mean, one dimension matrix, r g b channel
   *  @var YoloPreParam::rescale_type
   *  Preprocess config, vpss rescale type config
   *  @var YoloPreParam::keep_aspect_ratio
   *  Preprocess config aspect scale
   *  @var YoloPreParam:: resize_method
   *  Preprocess resize method config
   *  @var YoloPreParam::format
   *  Preprocess pixcel format config
   */
  typedef struct {
    float factor[3];
    float mean[3];
    meta_rescale_type_e rescale_type;
    bool keep_aspect_ratio;
    VPSS_SCALE_COEF_E resize_method;
    PIXEL_FORMAT_E format;
  } YoloPreParam;

以下是一个简单的设置案例:

* 通过CVI_TDL_Get_YOLO_Preparam以及CVI_TDL_Get_YOLO_Algparam分别获取：初始化预处理设置YoloPreParam以及yolov5模型设置YoloAlgParam
* 在设置了预处理参数和模型参数之后，再使用CVI_TDL_Set_YOLO_Preparam和CVI_TDL_Set_YOLO_Algparam传入设置的参数

  * yolov5是 **anchor-based** 的检测算法，为了方便使用，开放了anchor自定义设置，在设置YoloAlgParam中，需要注意anchors和strides的顺序需要一一对应，否则会导致推理结果出现错误

  * 另外支持自定义分类数量修改，如果修改了模型的输出分类数量，需要设置YolovAlgParam.cls为修改后的分类数量

* 再打开模型 CVI_TDL_OpenModel

* 再打开模型之后可以设置对应的置信度和nsm阈值：

  * CVI_TDL_SetModelThreshold 设置置信度阈值，默认0.5

  * CVI_TDL_SetModelNmsThreshold 设置nsm阈值，默认0.5

.. code-block:: c

  // set preprocess and algorithm param for yolov5 detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov5 param \n");
    CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set Yolov5 preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolov5_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5);
    uint32_t *anchors = new uint32_t[18];
    uint32_t p_anchors[18] = {10, 13, 16,  30,  33, 23,  30,  61,  62,
                              45, 59, 119, 116, 90, 156, 198, 373, 326};
    memcpy(anchors, p_anchors, sizeof(p_anchors));
    yolov5_param.anchors = anchors;
    yolov5_param.anchor_len = 18;

    uint32_t *strides = new uint32_t[3];
    uint32_t p_strides[3] = {8, 16, 32};
    memcpy(strides, p_strides, sizeof(p_strides));
    yolov5_param.strides = strides;
    yolov5_param.stride_len = 3;
    yolov5_param.cls = 80;

    printf("setup yolov5 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, yolov5_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set Yolov5 algorithm parameters %#x\n", ret);
      return ret;
    }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);

    printf("yolov5 algorithm parameters setup success!\n");
    return ret;
  }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);

    printf("yolov5 algorithm parameters setup success!\n");
    return ret;
  }

**推理以及结果获取**

通过本地或者流获取图片，并通过CVI_TDL_ReadImage函数读取图片，然后调用Yolov5推理接口CVI_TDL_Yolov5。
推理的结果存放在obj_meta结构体中，遍历获取边界框bbox的左上角以及右下角坐标点以及object score(x1, y1, x2, y2, score)，另外还有分类classes

.. code-block:: c

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }

  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV5, 0.5);

  std::cout << "model opened:" << model_path << std::endl;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S fdFrame;
  ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
    // continue;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_YOLOV5, &obj_meta);

  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }


编译说明
~~~~~~~~~~~~~~~

1. 获取交叉编译工具 

  .. code-block:: shell
    
    wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
    tar xvf host-tools.tar.gz
    cd host-tools
    export PATH=$PATH:$(pwd)/gcc/riscv64-linux-musl-x86_64/bin

2. 下载 TDL SDK 

  tdlsdk工具包的下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k。
  我们将cvitek_tdl_sdk_1227.tar.gz下载下来。

3. 编译TDL SDK

  我们进入到cvitek_tdl_sdk下的sample目录下。

  .. code-block:: shell

    chmod 777 compile_sample.sh
    ./compile_sample.sh

4. 编译完成之后，可以连接开发板并执行程序：
   
   * 开发板连接网线，确保开发板和电脑在同一个网关
   * 电脑通过串口连接开发板，波特率设置为115200，电脑端在串口输入ifconfig获取开发板的ip地址
   * 电脑通过 ssh 远程工具连接对应ip地址的开发板，用户名默认为：root，密码默认为：cvitek_tpu_sdk
   * 连接开发板之后，可以通过 mount 挂在sd卡或者电脑的文件夹：
      * 改载sd卡的命令是
      
      .. code-block:: shell

        mount /dev/mmcblk0 /mnt/sd
        # or
        mount /dev/mmcblk0p1 /mnt/sd

      * 挂载电脑的命令是：
        
      .. code-block:: shell

        mount -t nfs 10.80.39.3:/sophgo/nfsuser ./admin1_data -o nolock

      主要修改ip地址为自己电脑的ip，路径同样修改为自己的路径

5. export 动态依赖库
   
   主要需要的动态依赖库为：

   * ai_sdk目录下的lib
   * tpu_sdk目录下的lib
   * middlewave/v2/lib
   * middleware/v2/3rd
   * ai_sdk目录下的sample/3rd/lib

示例如下：

    .. code-block:: shell

      export LD_LIBRARY_PATH=/tmp/lfh/cvitek_tdl_sdk/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/opencv/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/tpu/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/ive/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/middleware/v2/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/lib:\
                              /tmp/lfh/cvitek_tdl_sdk/sample/3rd/middleware/v2/lib/3rd:

.. caution::
  注意将/tmp/lfh修改为开发版可以访问的路径，如果是用sd卡挂载，可以提前将所有需要的lib目录下的文件拷贝在同一个文件夹，然后export对应在sd卡的路径即可


6. 运行sample程序
   
* 切换到挂载的cvitek_tdl_sdk/bin目录下
* 然后运行以下测试案例
  
.. code-block:: shell

  ./sample_yolov5 /path/to/yolov5s.cvimodel /path/to/test.jpg

上述运行命令注意选择自己的cvimodel以及测试图片的挂载路径

测试结果
~~~~~~~~~~~~~~~

以下是官方yolov5模型转换后在coco2017数据集测试的结果，测试平台为 **CV1811h_wevb_0007a_spinor**

以下测试使用阈值为：

* conf_thresh: 0.001
* nms_thresh: 0.65

输入分辨率均为 640 x 640

yolov5s模型的官方导出导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 56.8
     - 37.4

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 32.61
     - 量化失败
     - 量化失败
  
   * - cv181x
     - 92.8
     - 100.42
     - 16.01
     - 量化失败
     - 量化失败

   * - cv182x
     - 69.89
     - 102.74
     - 16
     - 量化失败
     - 量化失败

   * - cv183x
     - 25.66
     - 73.4
     - 14.44
     - 量化失败
     - 量化失败
  
   * - cv186x
     - 10.50
     - 132.89
     - 23.11
     - 量化失败
     - 量化失败

yolov5s模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 55.4241
     - 36.6361

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 22.02
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 87.76
     - 85.74
     - 15.8
     - 54.204
     - 34.3985

   * - cv182x
     - 65.33
     - 87.99
     - 15.77
     - 54.204
     - 34.3985

   * - cv183x
     - 22.86
     - 58.38
     - 14.22
     - 54.204
     - 34.3985
  
   * - cv186x
     - 5.72
     - 69.48
     - 15.13
     - 52.44
     - 33.37

yolov5m模型的官方导出导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 64.1
     - 45.4

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 80.07
     - 量化失败
     - 量化失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 35.96
     - 量化失败
     - 量化失败

   * - cv182x
     - 180.85
     - 258.41
     - 35.97
     - 量化失败
     - 量化失败

   * - cv183x
     - 59.36
     - 137.86
     - 30.49
     - 量化失败
     - 量化失败

   * - cv186x
     - 24.44
     - 241.48
     - 39.26
     - 量化失败
     - 量化失败

yolov5m模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 62.770
     - 44.4973

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 79.08
     - ion分配失败
     - ion分配失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 35.73
     - ion分配失败
     - ion分配失败

   * - cv182x
     - 176.04
     - 243.62
     - 35.74
     - 61.5907
     - 42.0852

   * - cv183x
     - 56.53
     - 122.9
     - 30.27
     - 61.5907
     - 42.0852

   * - cv186x
     - 23.28
     - 218.11
     - 36.81
     - 61.54
     - 42.00


通用Yolov6模型部署
----------------------


引言
~~~~~~~~~~~~~~~

本文档介绍了如何将yolov6架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolov6模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

下载yolov6官方仓库 [meituan/YOLOv6](https://github.com/meituan/YOLOv6)，下载yolov6权重文件，在yolov6文件夹下新建一个目录weights，并将下载的权重文件放在目录yolov6-main/weights/下

然后将yolo_export/yolov6_eport.py复制到yolov6-main/deploy/onnx目录下

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

通过以下命令导出onnx模型

.. code-block:: shell

  python ./deploy/ONNX/yolov6_export.py \
    --weights ./weights/yolov6n.pt \
    --img-size 640 640 

* weights 为pytorch模型文件的路径
* img-size 为模型输入尺寸

然后得到onnx模型

.. tip::
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，如下所示

.. code-block:: 

  python yolov6_export.py --weights path/to/pt/weights --img-size 384 640



onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

yolov6接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~---------

提供预处理参数以及算法参数设置，其中参数设置：

* YoloPreParam 输入预处理设置

  $y=(x-mean)\times factor$

  * factor 预处理方差的倒数
  * mean 预处理均值
  * format 图片格式

* YoloAlgParam

  * cls 设置yolov6模型的分类

> yolov6是anchor-free的目标检测网络，不需要传入anchor

另外是yolov6的两个参数设置：

* CVI_TDL_SetModelThreshold  设置置信度阈值，默认为0.5
* CVI_TDL_SetModelNmsThreshold 设置nms阈值，默认为0.5

.. code-block:: c

  // set preprocess and algorithm param for yolov6 detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov6 param \n");
    CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov6 preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolov6_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6);
    yolov6_param.cls = 80;

    printf("setup yolov6 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, yolov6_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov6 algorithm parameters %#x\n", ret);
      return ret;
    }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);

    printf("yolov6 algorithm parameters setup success!\n");
    return ret;
  }

推理代码如下：

推理代码如下：

通过本地或者流获取图片，并通过CVI_TDL_ReadImage函数读取图片，然后调用Yolov6推理接口CVI_TDL_Yolov6。推理的结果存放在obj_meta结构体中，遍历获取边界框bbox的左上角以及右下角坐标点以及object score(x1, y1, x2, y2, score)，另外还有分类classes

.. code-block:: c++

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }
  printf("cvimodel open success!\n");
  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV6, 0.5);

  std::cout << "model opened:" << model_path << std::endl;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S fdFrame;
  ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_YOLOV6, &obj_meta);

  printf("detect number: %d\n", obj_meta.size);

  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }


测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

转换了yolov6官方仓库给出的yolov6n以及yolov6s，测试数据集为COCO2017

其中阈值参数设置为：

* conf_threshold: 0.03
* nms_threshold: 0.65

分辨率均为640x640

yolov6n模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 53.1
     - 37.5

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 15.59
     - 量化失败
     - 量化失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 11.58
     - 量化失败
     - 量化失败

   * - cv182x
     - 39.17
     - 47.08
     - 11.56
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 4.74
     - 51.28
     - 14.04
     - 量化失败
     - 量化失败

yolov6n模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 51.6373
     - 36.4384

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 14.74
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 49.11
     - 31.35
     - 8.46
     - 49.8226
     - 34.284

   * - cv182x
     - 34.14
     - 30.53
     - 8.45
     - 49.8226
     - 34.284

   * - cv183x
     - 10.89
     - 21.22
     - 8.49
     - 49.8226
     - 34.284

   * - cv186x
     - 4.02
     - 36.26
     - 12.67
     - 41.53
     - 27.63

yolov6s模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 61.8
     - 45

   * - cv180x
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 27.56
     - 量化失败
     - 量化失败

   * - cv182x
     - 131.1
     - 115.81
     - 27.56
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 14.67
     - 100.02
     - 29.66
     - 量化失败
     - 量化失败

yolov6s模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 60.1657
     - 43.5878

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 45.27
     - ion分配失败
     - ion分配失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 25.33
     - ion分配失败
     - ion分配失败

   * - cv182x
     - 126.04
     - 99.16
     - 25.32
     - 56.2774
     - 40.0781

   * - cv183x
     - 38.55
     - 57.26
     - 23.59
     - 56.2774
     - 40.0781

   * - cv186x
     - 13.89
     - 85.96
     - 29.57
     - 46.02
     - 31.61



.. vim: syntax=rst

通用yolov7模型部署
----------------------

引言
~~~~~~~~~~~~~~~~~~~~~~~~~

本文档介绍了如何将yolov7架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolov7模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

下载官方[yolov7](https://github.com/WongKinYiu/yolov7)仓库代码

.. code-block:: shell

  git clone https://github.com/WongKinYiu/yolov7.git

在上述下载代码的目录中新建一个文件夹weights，然后将需要导出onnx的模型移动到yolov7/weights

.. code-block:: shell

  cd yolov7 & mkdir weights
  cp path/to/onnx ./weights/

然后将yolo_export/yolov7_export.py复制到yolov7目录下

然后使用以下命令导出TDL_SDK形式的yolov7模型

.. code-block:: shell

  python yolov7_export.py --weights ./weights/yolov7-tiny.pt

.. tip::
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，如下命令所示：

.. code-block:: shell

    python yolov7_export.py --weights ./weights/yolov7-tiny.pt --img-size 384 640

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

.. caution:: 
  yolov7官方版本的模型预处理参数，即mean以及scale与yolov5相同，可以复用yolov5转换cvimodel的命令

TDL_SDK接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~

yolov7模型与yolov5模型检测与解码过程基本类似，主要不同是anchor的不同

.. caution::
  **注意修改anchors为yolov7的anchors!!!**
 

  anchors:
   - [12,16, 19,36, 40,28]  *# P3/8*
   - [36,75, 76,55, 72,146]  *# P4/16*
   - [142,110, 192,243, 459,401]  *# P5/32*

预处理接口设置如下代码所示

.. code-block:: c++

  // set preprocess and algorithm param for yolov7 detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov7 param \n");
    CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set Yolov5 preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolov7_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7);
    uint32_t *anchors = new uint32_t[18];
    uint32_t p_anchors[18] = {12, 16, 19,  36,  40,  28,  36,  75,  76,
                              55, 72, 146, 142, 110, 192, 243, 459, 401};
    memcpy(anchors, p_anchors, sizeof(p_anchors));
    yolov7_param.anchors = anchors;

    uint32_t *strides = new uint32_t[3];
    uint32_t p_strides[3] = {8, 16, 32};
    memcpy(strides, p_strides, sizeof(p_strides));
    yolov7_param.strides = strides;
    yolov7_param.cls = 80;

    printf("setup yolov7 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, yolov7_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set Yolov5 algorithm parameters %#x\n", ret);
      return ret;
    }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, 0.5);

    printf("yolov7 algorithm parameters setup success!\n");
    return ret;
  }


推理接口如下所示：

.. code-block:: c++

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }

  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV7, 0.5);

  std::cout << "model opened:" << model_path << std::endl;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S fdFrame;
  ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
    // continue;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_YOLOV7, &obj_meta);

  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }


测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

测试了yolov7-tiny模型各个版本的指标，测试数据为COCO2017，其中阈值设置为：

* conf_threshold: 0.001
* nms_threshold: 0.65

分辨率均为640 x 640

yolov7-tiny模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 56.7
     - 38.7

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 38.97
     - 量化失败
     - 量化失败

   * - cv181x
     - 75.4
     - 85.31
     - 17.54
     - 量化失败
     - 量化失败

   * - cv182x
     - 56.6
     - 85.31
     - 17.54
     - 量化失败
     - 量化失败

   * - cv183x
     - 21.85
     - 71.46
     - 16.15
     - 量化失败
     - 量化失败

   * - cv186x
     - 7.91
     - 137.72
     - 23.87
     - 量化失败
     - 量化失败

yolov7-tiny模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 53.7094
     - 36.438

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 36.81
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 70.41
     - 70.66
     - 15.43
     - 53.3681
     - 32.6277

   * - cv182x
     - 52.01
     - 70.66
     - 15.43
     - 53.3681
     - 32.6277

   * - cv183x
     - 18.95
     - 55.86
     - 14.05
     - 53.3681
     - 32.6277
   
   * - cv186x
     - 6.54
     - 99.41
     - 17.98
     - 53.44
     - 33.08


.. vim: syntax=rst

通用yolov8模型部署
----------------------

引言
~~~~~~~~~~~~~~~~~~~~~~~~~

本文档介绍了如何将yolov8架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolov8模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

首先获取yolov8官方仓库代码[ultralytics/ultralytics: NEW - YOLOv8 🚀 in PyTorch > ONNX > OpenVINO > CoreML > TFLite (github.com)](https://github.com/ultralytics/ultralytics)

.. code-block:: shell

  git clone https://github.com/ultralytics/ultralytics.git

再下载对应的yolov8模型文件，以[yolov8n](https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.pt)为例，然后将下载的yolov8n.pt放在ultralytics/weights/目录下，如下命令行所示

.. code-block:: shell

  cd ultralytics & mkdir weights
  cd weights
  wget https://github.com/ultralytics/assets/releases/download/v0.0.0/yolov8n.pt

调整yolov8输出分支，去掉forward函数的解码部分，并将三个不同的feature map的box以及cls分开，得到6个分支，这一步可以直接使用yolo_export的脚本完成

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

将yolo_export/yolov8_export.py代码复制到yolov8仓库下，然后使用以下命令导出分支版本的onnx模型：

.. code-block:: shell

  python yolov8_export.py --weights ./weights/yolov8.pt

运行上述代码之后，可以在./weights/目录下得到yolov8n.onnx文件，之后就是将onnx模型转换为cvimodel模型

.. tip:: 
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，如下：

.. code-block:: shell

    python yolov8_export.py --weights ./weights/yolov8.pt --img-size 384 640


onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

TDL_SDK接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~

yolov8的预处理设置参考如下：

.. code-block:: c

  // set preprocess and algorithm param for yolov8 detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov8 param \n");
    CVI_S32 ret =
        CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov8 preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolov8_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION);
    yolov8_param.cls = 80;

    printf("setup yolov8 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                        yolov8_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov8 algorithm parameters %#x\n", ret);
      return ret;
    }

    // set theshold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);

    printf("yolov8 algorithm parameters setup success!\n");
    return ret;
  }


推理测试代码：

.. code-block:: c++

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, argv[1]);
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }
  printf("---------------------to do detection-----------------------\n");

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S bg;
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
    printf("image read,hidth:%d\n", bg.stVFrame.u32Height);
  }
  std::string str_res;
  cvtdl_object_t obj_meta = {0};
  CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, &obj_meta);

  std::cout << "objnum:" << obj_meta.size << std::endl;
  std::stringstream ss;
  ss << "boxes=[";
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
       << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
       << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << "],";
  }
  ss << "]\n";


测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

转换测试了官网的yolov8n以及yolov8s模型，在COCO2017数据集上进行了测试，其中阈值设置为：

* conf: 0.001
* nms_thresh: 0.6

所有分辨率均为640 x 640

yolov8n模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 53
     - 37.3

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 13.26
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 54.91
     - 44.16
     - 8.64
     - 量化失败
     - 量化失败

   * - cv182x
     - 40.21
     - 44.32
     - 8.62
     - 量化失败
     - 量化失败

   * - cv183x
     - 17.81
     - 40.46
     - 8.3
     - 量化失败
     - 量化失败

   * - cv186x
     - 7.03
     - 55.03
     - 13.92
     - 量化失败
     - 量化失败

yolov8n模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 51.32
     - 36.4577

   * - cv180x
     - 299
     - 78.78
     - 12.75
     - 45.986
     - 31.798

   * - cv181x
     - 45.62
     - 31.56
     - 7.54
     - 51.2207
     - 35.8048

   * - cv182x
     - 32.8
     - 32.8
     - 7.72
     - 51.2207
     - 35.8048

   * - cv183x
     - 12.61
     - 28.64
     - 7.53
     - 51.2207
     - 35.8048

   * - cv186x
     - 5.20
     - 43.06
     - 12.02
     - 51.03
     - 35.61

yolov8s模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 61.8
     - 44.9

   * - cv180x
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败

   * - cv181x
     - 144.72
     - 101.75
     - 17.99
     - 量化失败
     - 量化失败

   * - cv182x
     - 103
     - 101.75
     - 17.99
     - 量化失败
     - 量化失败

   * - cv183x
     - 38.04
     - 38.04
     - 16.99
     - 量化失败
     - 量化失败

   * - cv186x
     - 13.16
     - 95.03
     - 23.44
     - 量化失败
     - 量化失败

yolov8s模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 60.1534
     - 44.034

   * - cv180x
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败
     - 模型转换失败

   * - cv181x
     - 135.55
     - 89.53
     - 18.26
     - 60.2784
     - 43.4908

   * - cv182x
     - 95.95
     - 89.53
     - 18.26
     - 60.2784
     - 43.4908

   * - cv183x
     - 32.88
     - 58.44
     - 16.9
     - 60.2784
     - 43.4908

   * - cv186x
     - 11.37
     - 82.61
     - 21.96
     - 60.27
     - 43.52


通用yolox模型部署
---------------------

引言
~~~~~~~~~~~~~~~~~~~~~~~~~

本文档介绍了如何将yolox架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolox模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

首先可以在github下载yolox的官方代码：[Megvii-BaseDetection/YOLOX: YOLOX is a high-performance anchor-free YOLO, exceeding yolov3~v5 with MegEngine, ONNX, TensorRT, ncnn, and OpenVINO supported. Documentation: https://yolox.readthedocs.io/ (github.com)](https://github.com/Megvii-BaseDetection/YOLOX/tree/main)

使用以下命令从源代码安装YOLOX

.. code-block:: shell

  git clone git@github.com:Megvii-BaseDetection/YOLOX.git
  cd YOLOX
  pip3 install -v -e .  # or  python3 setup.py develop


需要切换到刚刚下载的YOLOX仓库路径，然后创建一个weights目录，将预训练好的.pth文件移动至此

.. code-block:: shell

  cd YOLOX & mkdir weights
  cp path/to/pth ./weigths/

【官方导出onnx】

切换到tools路径

.. code-block:: shell

  cd tools

在onnx中解码的导出方式

.. code-block:: shell

  python \
  export_onnx.py \
  --output-name ../weights/yolox_m_official.onnx \
  -n yolox-m \
  --no-onnxsim \
  -c ../weights/yolox_m.pth \
  --decode_in_inference

相关参数含义如下：

* --output-name 表示导出onnx模型的路径和名称
* -n 表示模型名，可以选择
  * yolox-s, m, l, x
  * yolo-nano
  * yolox-tiny
  * yolov3
* -c 表示预训练的.pth模型文件路径
* --decode_in_inference 表示是否在onnx中解码

【TDL_SDK版本导出onnx】

为了保证量化的精度，需要将YOLOX解码的head分为三个不同的branch输出，而不是官方版本的合并输出

通过以下的脚本和命令导出三个不同branch的head：

将yolo_export/yolox_export.py复制到YOLOX/tools目录下，然后使用以下命令导出分支输出的onnx模型：

.. code-block:: shell

  python \
  yolox_export.py \
  --output-name ../weights/yolox_s_9_branch_384_640.onnx \
  -n yolox-s \
  -c ../weights/yolox_s.pth
  
.. tip:: 
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，如下：

.. code-block:: shell
  
  python \
  yolox_export.py \
  --output-name ../weights/yolox_s_9_branch_384_640.onnx \
  -n yolox-s \
  -c ../weights/yolox_s.pth \
  --img-size 384 640

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

TDL_SDK接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~

### 预处理参数设置

预处理参数设置通过一个结构体传入设置参数

.. code-block:: c++

  typedef struct {
    float factor[3];
    float mean[3];
    meta_rescale_type_e rescale_type;

    PIXEL_FORMAT_E format;
  } YoloPreParam;

而对于YOLOX，需要传入以下四个参数：

* factor 预处理scale参数
* mean 预处理均值参数
* format 图片格式，PIXEL_FORMAT_RGB_888_PLANAR

其中预处理factor以及mean的公式为
$$
y=(x-mean)\times scale
$$

### 算法参数设置

.. code-block:: c++

  typedef struct {
    uint32_t cls;
  } YoloAlgParam;

需要传入分类的数量，例如

.. code-block:: c++

  YoloAlgParam p_yolo_param;
  p_yolo_param.cls = 80;

另外的模型置信度参数设置以及NMS阈值设置如下所示：

.. code-block:: c++

  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, conf_threshold);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, nms_threshold);

其中conf_threshold为置信度阈值；nms_threshold为 nms 阈值

### 测试代码

.. code-block:: c++

   #ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
  #include <stdio.h>
  #include <stdlib.h>
  #include <time.h>
  #include <chrono>
  #include <fstream>
  #include <functional>
  #include <iostream>
  #include <map>
  #include <sstream>
  #include <string>
  #include <vector>
  #include "core/cvi_tdl_types_mem_internal.h"
  #include "core/utils/vpss_helper.h"
  #include "cvi_tdl.h"
  #include "cvi_tdl_media.h"

  // set preprocess and algorithm param for yolox detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg = CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 1.0;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolox param \n");
    CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolox preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolox_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX);
    yolox_param.cls = 80;

    printf("setup yolox algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, yolox_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolox algorithm parameters %#x\n", ret);
      return ret;
    }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, 0.5);

    printf("yolox algorithm parameters setup success!\n");
    return ret;
  }

  int main(int argc, char* argv[]) {
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

    std::string model_path = argv[1];
    std::string str_src_dir = argv[2];

    float conf_threshold = 0.5;
    float nms_threshold = 0.5;
    if (argc > 3) {
      conf_threshold = std::stof(argv[3]);
    }

    if (argc > 4) {
      nms_threshold = std::stof(argv[4]);
    }

    // change param of yolox
    ret = init_param(tdl_handle);

    printf("start open cvimodel...\n");
    ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, model_path.c_str());
    if (ret != CVI_SUCCESS) {
      printf("open model failed %#x!\n", ret);
      return ret;
    }
    printf("cvimodel open success!\n");
    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, conf_threshold);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOX, nms_threshold);
    std::cout << "model opened:" << model_path << std::endl;

    imgprocess_t img_handle;
    CVI_TDL_Create_ImageProcessor(&img_handle);

    VIDEO_FRAME_INFO_S fdFrame;
    ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
    if (ret != CVI_SUCCESS) {
      std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
                << std::endl;
    }

    cvtdl_object_t obj_meta = {0};

    CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_YOLOX, &obj_meta);

    printf("detect number: %d\n", obj_meta.size);
    for (uint32_t i = 0; i < obj_meta.size; i++) {
      printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
            obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
            obj_meta.info[i].classes);
    }

    CVI_TDL_ReleaseImage(img_handle, &fdFrame);
    CVI_TDL_Free(&obj_meta);
    CVI_TDL_DestroyHandle(tdl_handle);
    CVI_TDL_Destroy_ImageProcessor(img_handle);
    return ret;
  }


测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

测试了yolox模型onnx以及在cv181x/2x/3x各个平台的性能指标，其中参数设置：

* conf: 0.001
* nms: 0.65
* 分辨率：640 x 640

yolox-s模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 59.3
     - 40.5

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 37.41
     - 量化失败
     - 量化失败

   * - cv181x
     - 131.95
     - 104.46
     - 16.43
     - 量化失败
     - 量化失败

   * - cv182x
     - 95.75
     - 104.85
     - 16.41
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 12.39
     - 89.47
     - 19.56
     - 量化失败
     - 量化失败

yolox-s模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 53.1767
     - 36.4747

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 35.21
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 127.91
     - 95.44
     - 16.24
     - 52.4016
     - 35.4241

   * - cv182x
     - 91.67
     - 95.83
     - 16.22
     - 52.4016
     - 35.4241

   * - cv183x
     - 30.6
     - 65.25
     - 14.93
     - 52.4016
     - 35.4241

   * - cv186x
     - 11.39
     - 63.17
     - 19.48
     - 52.61
     - 35.49

yolox-m模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 65.6
     - 46.9

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 92.41
     - ion分配失败
     - ion分配失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 39.18
     - 量化失败
     - 量化失败

   * - cv182x
     - 246.1
     - 306.31
     - 39.16
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 30.55
     - 178.98
     - 38.72
     - 量化失败
     - 量化失败

yolox-m模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 2 2 2 1 1 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 59.9411
     - 43.0057

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 92.28
     - ion分配失败
     - ion分配失败

   * - cv181x
     - ion分配失败
     - ion分配失败
     - 38.95
     - N/A
     - N/A

   * - cv182x
     - 297.5
     - 242.65
     - 38.93
     - 59.3559
     - 42.1688

   * - cv183x
     - 75.8
     - 144.97
     - 33.5
     - 59.3559
     - 42.1688

   * - cv186x
     - 33.05
     - 173.20
     - 38.64
     - 59.34
     - 42.05


通用pp-yoloe模型部署
-------------------------

引言
~~~~~~~~~~~~~~~~~~~~~~~~~

本文档介绍了如何将ppyoloe架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* ppyoloe模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

PP-YOLOE是基于PP-Yolov2的Anchor-free模型，官方仓库在[PaddleDetection](https://github.com/PaddlePaddle/PaddleDetection)

获取官方仓库代码并安装：

.. code-block:: shell

  git clone https://github.com/PaddlePaddle/PaddleDetection.git

  # CUDA10.2
  python -m pip install paddlepaddle-gpu==2.3.2 -i https://mirror.baidu.com/pypi/simple

其他版本参照官方安装文档[开始使用_飞桨-源于产业实践的开源深度学习平台 (paddlepaddle.org.cn)](https://www.paddlepaddle.org.cn/install/quick?docurl=/documentation/docs/zh/install/pip/linux-pip.html)

onnx导出可以参考官方文档[PaddleDetection/deploy/EXPORT_ONNX_MODEL.md at release/2.4 · PaddlePaddle/PaddleDetection (github.com)](https://github.com/PaddlePaddle/PaddleDetection/blob/release/2.4/deploy/EXPORT_ONNX_MODEL.md)

本文档提供官方版本直接导出方式以及算能版本导出onnx，算能版本导出的方式需要去掉检测头的解码部分，方便后续量化，解码部分交给TDL_SDK实现

【官方版本导出】

可以使用PaddleDetection/tools/export_model.py导出官方版本的onnx模型

使用以下命令可以实现自动导出onnx模型，导出的onnx模型路径在output_inference_official/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco_official.onnx

.. code-block:: shell

  cd PaddleDetection
  python \
  tools/export_model_official.py \
  -c configs/ppyoloe/ppyoloe_crn_s_300e_coco.yml \
  -o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_crn_s_300e_coco.pdparams

  paddle2onnx \
  --model_dir \
  output_inference/ppyoloe_crn_s_300e_coco \
  --model_filename model.pdmodel \
  --params_filename model.pdiparams \
  --opset_version 11 \
  --save_file output_inference_official/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco_official.onnx

参数说明：

* -c 模型配置文件
* -o paddle模型权重
* --model_dir 模型导出目录
* --model_filename paddle模型的名称
* --params_filename paddle模型配置
* --opset_version opset版本配置
* --save_file 导出onnx模型的相对路径

【算能版本导出】

为了更好地进行模型量化，需要将检测头解码的部分去掉，再导出onnx模型，使用以下方式导出不解码的onnx模型

将yolo_export/pp_yolo_export.py复制到tools/目录下，然后使用如下命令导出不解码的pp-yoloe的onnx模型

.. code-block:: shell

  python \
  tools/export_model_no_decode.py \
  -c configs/ppyoloe/ppyoloe_crn_s_300e_coco.yml \
  -o weights=https://paddledet.bj.bcebos.com/models/ppyoloe_crn_s_300e_coco.pdparams

  paddle2onnx \
  --model_dir \
  output_inference/ppyoloe_crn_s_300e_coco \
  --model_filename model.pdmodel \
  --params_filename model.pdiparams \
  --opset_version 11 \
  --save_file output_inference/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco.onnx

参数参考官方版本导出的参数设置

.. tip::
  如果需要修改模型的输入尺寸，可以在上述导出的onnx模型进行修改，例如改为384x640的输入尺寸，使用以下命令进行修改:

.. code-block:: shell

  python -m paddle2onnx.optimize \
  --input_model ./output_inference/ppyoloe_crn_s_300e_coco/ppyoloe_crn_s_300e_coco.onnx \
  --output_model ./output_inference/ppyoloe_crn_s_300e_coco/ppyoloe_384.onnx \
  --input_shape_dict "{'x':[1,3,384,640]}"

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

TDL_SDK接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~

### 预处理参数设置

预处理的设置接口如下所示

.. code-block:: c++

  // set preprocess and algorithm param for ppyoloe detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup ppyoloe param \n");
    CVI_S32 ret = CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set ppyoloe preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t ppyoloe_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE);
    ppyoloe_param.cls = 80;

    printf("setup ppyoloe algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, ppyoloe_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set ppyoloe algorithm parameters %#x\n", ret);
      return ret;
    }

    // set thershold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, 0.5);
    CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, 0.5);

    printf("ppyoloe algorithm parameters setup success!\n");
    return ret;
  }

推理代码如下：

.. code-block:: c++

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, model_path.c_str());
  if (ret != CVI_SUCCESS) {
    printf("open model failed %#x!\n", ret);
    return ret;
  }
  printf("cvimodel open success!\n");
  // set thershold
  CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, conf_threshold);
  CVI_TDL_SetModelNmsThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, nms_threshold);

  std::cout << "model opened:" << model_path << std::endl;

  VIDEO_FRAME_INFO_S fdFrame;
  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  ret = CVI_TDL_ReadImage(img_handle, str_src_dir.c_str(), &fdFrame, PIXEL_FORMAT_RGB_888);
  if (ret != CVI_SUCCESS) {
    std::cout << "Convert out video frame failed with :" << ret << ".file:" << str_src_dir
              << std::endl;
  }

  cvtdl_object_t obj_meta = {0};

  CVI_TDL_Detection(tdl_handle, &fdFrame, CVI_TDL_SUPPORTED_MODEL_PPYOLOE, &obj_meta);

  printf("detect number: %d\n", obj_meta.size);
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    printf("detect res: %f %f %f %f %f %d\n", obj_meta.info[i].bbox.x1, obj_meta.info[i].bbox.y1,
           obj_meta.info[i].bbox.x2, obj_meta.info[i].bbox.y2, obj_meta.info[i].bbox.score,
           obj_meta.info[i].classes);
  }
  

测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

测试了ppyoloe_crn_s_300e_coco模型onnx以及cvimodel在cv181x平台的性能对比，其中阈值参数为：

* conf: 0.01
* nms: 0.7
* 输入分辨率：640 x 640

ppyoloe_crn_s_300e_coco模型官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - 60.5
     - 43.1

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 29.59
     - 量化失败
     - 量化失败

   * - cv181x
     - 103.62
     - 110.59
     - 14.68
     - 量化失败
     - 量化失败

   * - cv182x
     - 77.58
     - 111.18
     - 14.68
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 12.35
     - 101.83
     - 18.93
     - 量化失败
     - 量化失败

ppyoloe_crn_s_300e_coco模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 2 1 1 1 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 55.9497
     - 39.8568

   * - cv180x
     - ion分配失败
     - ion分配失败
     - 29.47
     - ion分配失败
     - ion分配失败

   * - cv181x
     - 101.15
     - 103.8
     - 14.55
     - 55.36
     - 39.1982

   * - cv182x
     - 75.03
     - 104.95
     - 14.55
     - 55.36
     - 39.1982

   * - cv183x
     - 30.96
     - 80.43
     - 13.8
     - 55.36
     - 39.1982

   * - cv186x
     - 12.29
     - 100.21
     - 19.99
     - 38.67
     - 27.32



通用yolov10模型部署
----------------------

引言
~~~~~~~~~~~~~~~~~~~~~~~~~

本文档介绍了如何将yolov10架构的模型部署在cv181x开发板的操作流程，主要的操作步骤包括：

* yolov10模型pytorch版本转换为onnx模型
* onnx模型转换为cvimodel格式
* 最后编写调用接口获取推理结果

pt模型转换为onnx
~~~~~~~~~~~~~~~~~~~~~~~~~

首先获取yolov10官方仓库代码 [THU-MIG/yolov10](https://github.com/THU-MIG/yolov10)

.. code-block:: shell

  git clone https://github.com/THU-MIG/yolov10.git

再下载对应的 yolov10 模型文件，以[yolov10n](https://github.com/THU-MIG/yolov10.git)为例，将下载的yolov10n.pt放在./weights/目录下，如下命令行所示

.. code-block:: shell

  mkdir weights
  cd weights
  wget https://github.com/THU-MIG/yolov10.git

调整yolov10输出分支，去掉forward函数的解码部分，并将三个不同的feature map的box以及cls分开，得到6个分支，这一步可以直接使用yolo_export的脚本完成

yolo_export中的脚本可以通过SFTP获取：下载站台:sftp://218.17.249.213 帐号:cvitek_mlir_2023 密码:7&2Wd%cu5k

通过SFTP找到下图对应的文件夹：

.. image:: /folder_example/yolo_export_sftp.png
  :scale: 50%
  :align: center
  :alt: /home/公版深度学习SDK/yolo_export.zip

将yolo_export/yolov10_export.py代码复制到yolov10仓库根目录下，然后使用以下命令导出分支版本的onnx模型：

.. code-block:: shell

  python yolov10_export.py --weights ./weights/yolov10.pt

运行上述代码之后，可以在./weights/目录下得到 yolov10n.onnx 文件，之后就是将onnx模型转换为cvimodel模型

.. tip:: 
  如果输入为1080p的视频流，建议将模型输入尺寸改为384x640，可以减少冗余计算，提高推理速度，如下：

.. code-block:: shell

    python yolov10_export.py --weights ./weights/yolov10.pt --img-size 384 640


onnx模型转换cvimodel
~~~~~~~~~~~~~~~~~~~~~~~~~---------

cvimodel转换操作可以参考cvimodel转换操作可以参考yolo-v5移植章节的onnx模型转换cvimodel部分。

TDL_SDK接口说明
~~~~~~~~~~~~~~~~~~~~~~~~~

yolov10的预处理设置参考如下：

.. code-block:: c

  // set preprocess and algorithm param for yolov8 detection
  // if use official model, no need to change param
  CVI_S32 init_param(const cvitdl_handle_t tdl_handle) {
    // setup preprocess
    InputPreParam preprocess_cfg =
        CVI_TDL_GetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION);

    for (int i = 0; i < 3; i++) {
      printf("asign val %d \n", i);
      preprocess_cfg.factor[i] = 0.003922;
      preprocess_cfg.mean[i] = 0.0;
    }
    preprocess_cfg.format = PIXEL_FORMAT_RGB_888_PLANAR;

    printf("setup yolov10 param \n");
    CVI_S32 ret =
        CVI_TDL_SetPreParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION, preprocess_cfg);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov10 preprocess parameters %#x\n", ret);
      return ret;
    }

    // setup yolo algorithm preprocess
    cvtdl_det_algo_param_t yolov10_param =
        CVI_TDL_GetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION);
    yolov10_param.cls = 80;
    yolov10_param.max_det = 300;

    printf("setup yolov10 algorithm param \n");
    ret = CVI_TDL_SetDetectionAlgoParam(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION,
                                        yolov10_param);
    if (ret != CVI_SUCCESS) {
      printf("Can not set yolov10 algorithm parameters %#x\n", ret);
      return ret;
    }

    // set theshold
    CVI_TDL_SetModelThreshold(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION, 0.5);

    printf("yolov10 algorithm parameters setup success!\n");
    return ret;
  }


推理测试代码：

.. code-block:: c++

  ret = CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION, argv[1]);

  if (ret != CVI_SUCCESS) {
    printf("open model failed with %#x!\n", ret);
    return ret;
  }
  printf("---------------------to do detection-----------------------\n");

  imgprocess_t img_handle;
  CVI_TDL_Create_ImageProcessor(&img_handle);

  VIDEO_FRAME_INFO_S bg;
  ret = CVI_TDL_ReadImage(img_handle, strf1.c_str(), &bg, PIXEL_FORMAT_RGB_888_PLANAR);
  if (ret != CVI_SUCCESS) {
    printf("open img failed with %#x!\n", ret);
    return ret;
  } else {
    printf("image read,width:%d\n", bg.stVFrame.u32Width);
    printf("image read,hidth:%d\n", bg.stVFrame.u32Height);
  }
  std::string str_res;
  cvtdl_object_t obj_meta = {0};
  CVI_TDL_Detection(tdl_handle, &bg, CVI_TDL_SUPPORTED_MODEL_YOLOV10_DETECTION, &obj_meta);

  std::cout << "objnum:" << obj_meta.size << std::endl;
  std::stringstream ss;
  ss << "boxes=[";
  for (uint32_t i = 0; i < obj_meta.size; i++) {
    ss << "[" << obj_meta.info[i].bbox.x1 << "," << obj_meta.info[i].bbox.y1 << ","
       << obj_meta.info[i].bbox.x2 << "," << obj_meta.info[i].bbox.y2 << ","
       << obj_meta.info[i].classes << "," << obj_meta.info[i].bbox.score << "],";
  }
  ss << "]\n";


测试结果
~~~~~~~~~~~~~~~~~~~~~~~~~

转换测试了官网的yolov10n模型，在COCO2017数据集上进行了测试，其中阈值设置为：

* conf: 0.001
* max_det: 100

所有分辨率均为640 x 640

yolov10n模型的官方导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - pytorch
     - N/A
     - N/A
     - N/A
     - N/A
     - 38.5

   * - cv180x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv181x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv182x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv183x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败

   * - cv186x
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败
     - 量化失败


目前 yolov10n 官方导出版本中存在 mod 算子，tpu-mlir工具链暂时不支持该算子

yolov10n模型的TDL_SDK导出方式性能：

.. list-table::
   :widths: 1 2 2 2 2 2
   :header-rows: 1

   * - 测试平台
     - 推理耗时 (ms)
     - 带宽 (MB)
     - ION(MB)
     - MAP 0.5
     - MAP 0.5-0.95

   * - onnx
     - N/A
     - N/A
     - N/A
     - 52.9
     - 38.0

   * - cv180x
     - 245
     - 83.98
     - 11.47
     - 40.126
     - 29.118

   * - cv181x
     - 45.3
     - 42.86
     - 7.08
     - 48.7
     - 35.2

   * - cv182x
     - 20.6
     - 41.86
     - 7.72
     - 48.7
     - 48.7

   * - cv183x
     - N/A
     - N/A
     - 6.77
     - N/A
     - N/A

   * - cv186x
     - 5.362
     - 49.38
     - 11.20
     - 44.9
     - 33.0
