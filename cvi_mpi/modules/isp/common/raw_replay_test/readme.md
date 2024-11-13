
## 一、做什么用

本工具用于在板端回灌在 PC 端使用 pqtool 抓下来的 raw，可以是连续 RAW 或者单张 RAW。

回灌后会生成对应的 yuv、jpg、h264 裸流，用于画质检查。

## 二、如何使用

### 使用步骤

1. 使用 pqtool 抓图，抓单张 RAW 或连续 RAW, 放入 SD 卡，并将 SD 卡挂载到板端；
2. 编写对应 test script file 并一起放入 SD 卡；
3. 运行测试程序 ./raw_replay_test /mnt/sd/test_script_file.txt

### test script file 介绍

test script file 里面可以同时描述多组 RAW 进行测试，使用 test_start 和 test_end 进行区分，每组测试可包含以下几个参数：

1. test_dir 描述 RAW 存放的位置，必要参数，可以是直接存放 RAW 的文件夹，或者该文件夹下面的子文件夹中有 RAW；
2. test_pq_bin 描述进行该组 RAW 测试时需要加载的 pq bin 文件路径，可选参数，如果不设置代表使用默认参数；
3. test_output_dir 描述该组 RAW 测试输出结果的存放路径，可选参数，如果不设置代表输出到和 RAW 文件同级目录；
4. test_roi 用于当板子内存比较小时可以选择在该组 RAW 上进行局部的回灌测试，可选参数，如果不设置代表使用 RAW 的原始大小进行回灌；
5. test_sensor_cfg 用于指定本组测试使用的 sensor cfg 文件，主要用于区别 sdr 和 wdr 的测试资源，可选参数，如果不设置则使用 mnt/data/sensor_cfg.ini；
6. test_md5 用于设置本次测试是否是进行 md5 测试，可选参数，设置 1 表示进行 md5 测试，设置为 0 或者不设置表示不进行 md5 测试。

举例如下：

    test_start
    test_dir = /mnt/sd/raw_data
    test_pq_bin = /mnt/sd/cvi_sdr_01.bin
    test_roi = 0,0,960,540
    test_output_dir = /mnt/sd/raw_data/output_01
    test_end

    test_start
    test_dir = /mnt/sd/raw_data
    test_pq_bin = /mnt/sd/cvi_sdr_02.bin
    test_output_dir = /mnt/sd/raw_data/output_02
    test_end

如上 test script 中描述了对同一组 RAW 进行两次测试，并使用了两个不同的 pq bin 文件，用于比对两个输出结果之间的差异。

## 三、如何同时测试 SDR 和 WDR 的 RAW

当测试资源同时存在 SDR 和 WDR 的 RAW 时，可以通过 test_sensor_cfg 来指定对应的 sensor cfg 来达到同时测试 SDR 和 WDR 的 RAW。

test script 举例如下：

    test_start
    test_dir = /mnt/sd/sdr/raw_data
    test_pq_bin = /mnt/sd/sdr/cvi_sdr.bin
    test_output_dir = /mnt/sd/raw_data/output_01
    test_sensor_cfg = /mnt/sd/sdr/sensor_cfg.ini.sdr
    test_end

    test_start
    test_dir = /mnt/sd/wdr/raw_data
    test_pq_bin = /mnt/sd/wdr/cvi_wdr.bin
    test_output_dir = /mnt/sd/raw_data/output_02
    test_sensor_cfg = /mnt/sd/wdr/sensor_cfg.ini.wdr
    test_end

## 四、如何进行 YUV MD5 的测试

MD5 测试的方法是回灌后抓取 YUV 对比当前 YUV 的 MD5 和之前的 MD5 是不是一样的，用来检验代码的改动前后是否会引起画质的变化，通过 test_md5 参数来使能 MD5 测试。

YUV 的 MD5 结果是存放在对应 RAW 文件夹下的 md5.txt 中，初次测试时由于不存在会新建该文件，同时将第一次的 MD5 结果写入。

MD5 测试时请使用单张 RAW ，同时 test script 里面不能设置 test_output_dir 参数。

在进行 MD5 测试时，不会生成 h264 裸流，只有 YUV 和 jpg。

建议每次上 code 前，如果预期当前修改前后不会对画质产生影响，可以使用改测试来验证。

test script 举例如下：

    test_start
    test_dir = /mnt/sd/md5-test-res/307-sdr
    test_sensor_cfg = /mnt/sd/md5-test-res/307-sdr/sensor_cfg.ini.307.sdr
    test_md5 = 1
    test_end

    test_start
    test_dir = /mnt/sd/md5-test-res/307-wdr
    test_sensor_cfg = /mnt/sd/md5-test-res/307-wdr/sensor_cfg.ini.307.wdr
    test_md5 = 1
    test_end
