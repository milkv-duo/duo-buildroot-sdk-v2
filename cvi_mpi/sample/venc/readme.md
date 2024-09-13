# Sample_venc User Guide

## Sample_venc 使用方法
sample_venc -c codec -w width -h height -i src.yuv -o enc

EX.

sample_venc -c 265 -w 1920 -h 1080 -i ReadySteadyGo_1920x1080_600.yuv -o enc

sample_venc --numChn=1 --chn=0 -c 264 --getBsMode=1 --statTime=2 --gop=50 --srcFramerate=25 --framerate=25 --initQp=32 --minIqp=28 --maxIqp=46 --minQp=28 --maxQp=46 --ipQpDelta=0 -w 720 -h 480 -i input_yuv -o output_stream --rcMode=0 -n 100 --bitrate=10000 --initialDelay=100 --thrdLv=4 --maxIprop=10

## 参数及其使用说明

|参数|描述|
|----------------------------------|------------------------------------------------------------------------------------------------------|
|--codec                           |   265 = h.265, jpg = jpeg, mjp = motion jpeg                                                         |
|--width                           |   width of input yuv file                                                                            |
|--height                          |   height of input yuv file                                                                           |
|--input                           |   source yuv file                                                                                    |
|--output                          |   output bitstream                                                                                   |
|--frame_num                       |   number of frame to be encode                                                                       |
|--getBsMode                       |   get-bitstream mode, 0 = query status, 1 = select                                                   |
|--profile                         |   profile, 0 = h264 baseline, 1 = h264 main, 2 = h264 high, Default = 2                              |
|--rcMode                          |   rate control mode, 0 = CBR, 1 = VBR, 2 = AVBR, 4 = FIXQP, 5 = QPMAP, 6 = UBR (User BR), default = 4|
|--iqp                             |   I frame QP. [0, 51]                                                                                |
|--pqp                             |   P frame QP. [0, 51]                                                                                |
|--ipQpDelta                       |   QP Delta between P frame and I frame. [-10, 30]                                                    |
|--bgQpDelta                       |   Smart-P QP delta between P frame and BG (background) frame. [-10, 30], default = 0                 |
|--viQpDelta                       |   Smart-P QP delta between P frame and VI (virtual I) frame. [-10, 30], default = 0                  |
|--gop                             |   The period of one gop                                                                              |
|--gopMode                         |   GOP mode. 0: Normal P, 2: Smart P, Default: 0                                                      |
|--bitrate                         |   The average target bitrate (kbits)                                                                 |
|--initQp                          |   The Start Qp of 1st frame, 63 = default                                                            |
|--minQp                           |   Minimum Qp for one frame                                                                           |
|--maxQp                           |   Maximum Qp for one frame                                                                           |
|--minIqp                          |   Minimum Qp for I frame                                                                             |
|--maxIqp                          |   Maximum Qp for I frame                                                                             |
|--srcFramerate                    |   source frame rate                                                                                  |
|--framerate                       |   destination frame rate                                                                             |
|--vfps                            |   enable variable FPS                                                                                |
|--quality                         |   jpeg encode quality. [0, 99]                                                                       |
|--maxbitrate                      |   Maximum output bit rate (kbits)                                                                    |
|--changePos                       |   Ratio to change Qp. [50, 100]                                                                      |
|--minStillPercent                 |   Percentage of target bitrate in low motion. [5, 100]                                               |
|--maxStillQp                      |   Maximum Qp in low motion                                                                           |
|--motionSense                     |   Motion sensitivity                                                                                 |
|--avbrFrmLostOpen                 |   avbrFrmLostOpen                                                                                    |
|--avbrFrmGap                      |   avbrFrmGap                                                                                         |
|--avbrPureStillThr                |   avbrPureStillThr                                                                                   |
|--bgEnhanceEn                     |   Enable background enhancement                                                                      |
|--bgDeltaQp                       |   background delta qp                                                                                |
|--statTime                        |   statistics time in seconds                                                                         |
|--testMode                        |   samele_venc test mode                                                                              |
|--getstream-timeout               |   samele_venc getstream-timeout   -1:block mode, 0:try_once, >0 timeout in ms                        |
|--sendframe-timeout               |   samele_venc sendframe-timeout   -1:block mode, 0:try_once, >0 timeout in ms                        |
|--ifInitVb                        |   if enable VB pool or not                                                                           |
|--vbMode                          |   if enable VB pool mode. 0 = common, 1 = module, 2 = private, 3 = user                              |
|--yuvFolder                       |   yuv files folder                                                                                   |
|--bindmode                        |   bind mode. 0 = VENC_BIND_DISABLE, 1 = VENC_BIND_VI, 2 = VENC_BIND_VPSS                             |
|--pixel_format                    |   0: 420 planar, 1: 422 planar, 2: NV12, 3: NV21                                                     |
|--posX                            |   x axis of start position, need to be multiple of 16 (used for crop)                                |
|--posY                            |   y axis of start position, need to be multiple of 16 (used for crop)                                |
|--inWidth                         |   width of input frame (used for crop)                                                               |
|--inHeight                        |   height of input frame (used for crop)                                                              |
|--bufSize                         |   bitstream Buffer size                                                                              |
|--single_LumaBuf                  |   0: disable, 1: use single luma buffer for H264                                                     |
|--single_core                     |   0: disable, 1: use single core(h264 or h265 only)                                                  |
|--forceIdr                        |   0: disable, > 0: set force idr at number of frame                                                  |
|--chgNum                          |   frame num to change attr                                                                           |
|--chgBitrate                      |   change bitrate  (kbits)                                                                            |
|--chgFramerate                    |   change dstframerate                                                                                |
|--tempLayer                       |   tempLayer                                                                                          |
|--roiCfgFile                      |   ROI configuration file                                                                             |
|--qpMapCfgFile                    |   Roi-based qpMap file                                                                               |
|--bgInterval                      |   bgInterval                                                                                         |
|--frame_lost                      |   0: disable, 1: use frame lost(h264 or h265 only)                                                   |
|--frame_lost_gap                  |   The gap between 2 frame_lost frames(h264 or h265 only)                                             |
|--frame_lost_thr                  |   frame_lost bsp threshold(h264 or h265 only)                                                        |
|--MCUPerECS                       |   jpeg encode MCUPerECS                                                                              |
|--single_EsBuf                    |   0: disable, 1: use single stream buffer (jpege)                                                    |
|--single_EsBuf_264                |   0: disable, 1: use single stream buffer (h264e)                                                    |
|--single_EsBuf_265                |   0: disable, 1: use single stream buffer (h265e)                                                    |
|--single_EsBufSize                |   single stream buffer size (jpege)                                                                  |
|--single_EsBufSize_264            |   single stream buffer size (h264e)                                                                  |
|--single_EsBufSize_265            |   single stream buffer size (h265e)                                                                  |
|--numChn                          |   number of channels to encode                                                                       |
|--chn                             |   set channel-id to configure the following parameters                                               |
|--viWidth                         |   for VI input width                                                                                 |
|--viHeight                        |   for VI input height                                                                                |
|--vpssWidth                       |   for Vpss output width                                                                              |
|--vpssHeight                      |   for VPss output height                                                                             |
|--vpssSrcPath                     |   source file path for vpss                                                                          |
|--user_data1                      |   user data binary file 1                                                                            |
|--user_data2                      |   user data binary file 2                                                                            |
|--user_data3                      |   user data binary file 3                                                                            |
|--user_data4                      |   user data binary file 4                                                                            |
|--h265RefreshType                 |   0: IDR, 1: CRA, default = 0                                                                        |
|--initialDelay                    |   rc initial delay in ms, default = 1000                                                             |
|--jpegMarkerOrder                 |   0: Cvitek, 1: SOI-JFIF-DQT_MERGE-SOF0-DHT_MERGE-DRI, 2: Cvitek w/ JFIF, default = 0                |
|--intraCost                       |   intraCost, the extra cost of intra mode                                                            |
|--thrdLv                          |   thrdLv, threhold to control block qp. [0, 4]                                                       |
|--h264EntropyMode                 |   0: CAVLC, 1: CABAC, default = 1                                                                    |
|--h264ChromaQpOffset              |   H264 Chroma QP offset [-12, 12], default = 0                                                       |
|--h265CbQpOffset                  |   H265 Cb QP offset [-12, 12], default = 0                                                           |
|--h265CrQpOffset                  |   H265 Cr QP offset [-12, 12], default = 0                                                           |
|--maxIprop                        |   max I frame bitrate ratio to P frame, default = 100                                                |
|--rowQpDelta                      |   rowQpDelta [0, 10], default = 1                                                                    |
|--superFrmMode                    |   superFrmMode, 0 = disable, 3 = encode to IDR, default = 0                                          |
|--superIBitsThr                   |   superIBitsThr [1000, 33554432], default = 4000000                                                  |
|--superPBitsThr                   |   superPBitsThr [1000, 33554432], default = 4000000                                                  |
|--maxReEnc                        |   maxReEnc [0, 3], default = 0                                                                       |
|--aspectRatioInfoPresentFlag      |   aspect ratio info present flag [0, 1], default = 0                                                 |
|--aspectRatioIdc                  |   aspect ratio idc [0, 255], default = 1                                                             |
|--overscanInfoPresentFlag         |   overscan info present flag [0, 1], default = 0                                                     |
|--overscanAppropriateFlag         |   overscan appropriate flag [0, 1], default = 0                                                      |
|--sarWidth                        |   sar width [0, 65535], default = 1                                                                  |
|--sarHeight                       |   sar height [0, 65535], default = 1                                                                 |
|--timingInfoPresentFlag           |   timing info present flag [0, 1], default = 0                                                       |
|--fixedFrameRateFlag              |   fixed frame rate flag [0, 1], default = 0                                                          |
|--numUnitsInTick                  |   num units in tick [0, 4294967295], default = 1                                                     |
|--timeScale                       |   time scale [0, 4294967295], default = 60                                                           |
|--videoSignalTypePresentFlag      |   video signal type present flag [0, 1], default = 0                                                 |
|--videoFormat                     |   video format [0, 7], default = 5                                                                   |
|--videoFullRangeFlag              |   video full range flag [0, 1], default = 0                                                          |
|--colourDescriptionPresentFlag    |   colour description present flag [0, 1], default = 0                                                |
|--colourPrimaries                 |   colour primaries [0, 255], default = 2                                                             |
|--transferCharacteristics         |   transfer characteristics [0, 255], default = 2                                                     |
|--matrixCoefficients              |   matrix coefficients [0, 255], default = 2                                                          |
|--testUbrEn                       |   enable to test ubr [0, 1], default = 0                                                             |
|--frameQp                         |   frameQp [0, 51], default = 38                                                                      |
|--isoSendFrmEn                    |   isoSendFrmEn [0, 1], default = 1                                                                   |
|--sensorEn                        |   sensorEn [0, 1], default = 0                                                                       |
|--sliceSplitCnt                   |   sliceSplitCnt [1, 5], default = 1                                                                  |
|--disabledblk                     |   disabledblk [0, 1], default = 0                                                                    |
|--betaOffset                      |   betaOffset [-6, 6], default = 0                                                                    |
|--alphaoffset                     |   alphaoffset [-6, 6], default = 0                                                                   |
|--intraPred                       |   intraPred [0, 1], default = 0                                                                      |
