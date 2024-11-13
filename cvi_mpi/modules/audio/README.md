# cviaudio
README file(ver.19.1120)
Thie readme file provides build code instruction and the output execution file, libraries, scripts, etc.
All of the API and headers and are properties of  Crystal Vision Technology.
Please use it carefully with Crystal Vision edge board and don't use it for profitable purpose or further business behavior before contact us.

### Requirements
- Linux linaro version 5.10
#### Platforms
- Embedded development board : cv1180x/cv181x
### Packages
- build-essential gdb vim cmake libtool autoconf autotools-dev automake python-dev python3-dev python-pip python-cffi python-numpy python-scipy gcc-arm-none-eabi libgoogle-glog-dev libboost-all-dev
### Build Commands
Go to the cviaudio folder and trigger shell command:
```sh
$ ./run.sh
```
The output libs locate in :/cviaudio/install/soc_cv180x_asic/lib
The output exe binaries locate in :/cviaudio/install/soc_cv180x_asic/bin
### CVI audio API for application layer
You can check the header files:
  - cvi_audio.h

The API flow reference, you can check:

  - cvi_sample_audio.c

### Audio test execusion program
> Please notice that the audio test program should be place in the embedded board.
> Before executing the binary file, export LD_LIBRARY_PATH = libcviaudio.so and orther so-files are required.

### Output Binary Files
#### *civ_nr*
____
This execusion bin allows you to produce a .pcm files after "Noise Reduction" algorithm. By input a single8k or 16k sample
rates wav format file.
ex:
```sh
$ ./cvi_nr [example].wav
```
The output file will naming as "*after[example].wav.pcm*"

#### *cvi_sample_audio*
____
This execusion bin built from source code: cvi_sample_audio.c  and allows user to test the each sample case, such as,
record audio / play audio/ transcode input audio to file...
ex:
```sh
$ ./cvi_sample_audio [testcase #]
```
[testcase #] range from 0 ~ 6, you can toggle ./cvi_sample_audio --help to check the usage.

#### *cvi_transcode*
____You can transcode from pcm/wav to below codec type:
*722*:
```
./cvi_audiotranscode  input.wav  output.wav 722
```
*726*:
```
./cvi_audiotranscode  input.wav  output.wav 726
```
*723*:
```
./cvi_audiotranscode  input.wav  output.wav 723
```
*711*:
```
./cvi_audiotranscode  input.wav  output.wav 711
```
*729*:
```
./cvi_audiotranscode  input.wav  output.wav 729
```
License & Authors
-----------------
Not approval for commercial usage or bussiness activities.
The code only for testify and research.
Copyright:: 2019 draft/ 19.1120