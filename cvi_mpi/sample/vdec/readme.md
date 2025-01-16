# Sample_vdec User Guide

## Sample_vdec 使用方法
sample_vdec --numChn=num-all-chnnels --chn=currChnId -c codec -i bs -o out.yuv

EX.

sample_vdec --numChn=1 --chn=0 -c 264 -i in.264 -o out.yuv

sample_vdec --numChn=2 --chn=0 -c 264 -i in0.264 -o out0.yuv --chn=1 -c 264 -i in1.264 -o out1.yuv

## 参数及其使用说明

|参数|描述|
|-----------------------|--------------------------------------------------------------------------------|
|--numChn               |number of channels to decode                                                    |
|--chn                  |set channel-id to configure the following parameters                            |
|--codec                |264 = h.264, 265 = h.265, jpg = jpeg, mjp = motion jpeg                         |
|--input                |source bitstream                                                                |
|--output               |output yuv file                                                                 |
|--dump                 |dump yuv for md5sum check, 1 = dump yuv, 0 = md5 check                          |
|--bufwidth             |set max width for alloc frame buffer                                            |
|--bufheight            |set max height for alloc frame buffer                                           |
|--maxframe             |set max frame buffer number                                                     |
|--vbMode               |set vb mode for alloc frame buffer                                              |
|--testMode             |samele_vdec test mode                                                           |
|--bitStreamFolder      |bitstream files folder                                                          |
|--getframe-timeout     |samele_vdec getframe-timeout   -1:block mode, 0:try_once, >0 timeout in ms      |
|--sendstream-timeout   |samele_vdec sendstream-timeout   -1:block mode, 0:try_once, >0 timeout in ms    |
|--bindmode             |bind mode. 0 = VDEC_BIND_DISABLE, 1 = VDEC_BIND_VPSS_VO, 2 = VDEC_BIND_VPSS_VENC|
|--pixel_format         |output pixel format. 0: do not specify, 1: NV12, 2: NV21                        |
|--help                 |help                                                                            |
