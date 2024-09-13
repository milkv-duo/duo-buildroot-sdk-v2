Audio instructions
=============

In order to process audio on the development board, we can use sample_audio or ffmpeg or build alsa tools from source code.

# Use samle_audio

## Usage:

./sample_audio <index> --list
-r [sample_rate] -R [Chnsample_rate]
-c [channel] -p [preiod_size][*aac enc must 1024]
-C [codec 0:g726 1:g711A 2:g711Mu 3: adpcm 4.AAC]
-V [bVqeOn] -F [In/Out filename] -T [record time]

## Index

0 : Ai bind Aenc, save as file
1 : Ai unbind Aenc, save as file
2 : Adec bind ao, save as file
3 : Adec unbind ao, save as file
4 : recording frame by frame
5 : playing audio frame by frame
6 : SetVolume db test
7 : Audio Version
8 : GetVolume db test
9 : ioctl test
10: Aec test

## Examples

Aenc    eg : ./sample_audio 0 --list -r 8000 -R 8000 -c 2 -p 320 -C 1 -V 0 -F Cvi_8k_2chn.g711a -T 10
           : ./sample_audio 0 --list -r 8000 -R 8000 -c 2 -p 1024 -C 4 -V 0 -F Cvi_8k_2chn.aac -T 10
Adec    eg : ./sample_audio 2 --list -r 8000 -R 8000 -c 2 -p 320 -C 1 -V 0 -F Cvi_8k_2chn.g711a -T 10
Ai      eg : ./sample_audio 4 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 0 -F Cvi_8k_2chn.raw -T 10
Ao      eg : ./sample_audio 5 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 0 -F Cvi_8k_2chn.raw -T 10
SetVol  eg : ./sample_audio 6
GetVol  eg : ./sample_audio 8
AECtest eg : ./sample_audio 10 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 1 -F play.wav -T 10


# Use FFmpeg

## Command

  ffmpeg -f alsa <input_options> -i <input_device> ... output.wav

  ffmpeg -i input.wav -f alsa <output_options> <output_device>

To see the list of cards currently recognized by your system check the files /proc/asound/cards and /proc/asound/devices.

## Examples

To capture with ffmpeg from an ALSA device with card id 0, you may run the command:

  ffmpeg -f alsa -i hw:0 output.wav

To play a file on soundcard 1, audio device 7, you may run the command:

  ffmpeg -i input.wav -f alsa hw:1,7

* For more details, you can visit https://ffmpeg.org/documentation.html


# Build from source code

## Download source code

Download the source code here: https://www.alsa-project.org/wiki/Download#Detailed_package_descriptions

```
# tar -xvjf alsa-lib-1.2.9.tar.bz2
# tar -xvjf alsa-utils-1.2.9.tar.bz2
```

## Build alsa-lib

```
# cd alsa-lib-1.2.9/
# ./configure --prefix=<path_to_build_directory>/alsa-lib --host=aarch64-linux-gnu --with-configdir=/usr/share/alsa-arm
# make
# sudo make install
```

## Build alsa-utils

```
# cd alsa-utils-1.2.9/
# ./configure --prefix=<path_to_build_directory>/alsa-utils --host=aarch64-linux-gnu --with-alsa-inc-prefix=<path_to_build_directory>/alsa-lib/include/ --with-alsa-prefix=<path_to_build_directory>/alsa-lib/lib/ --disable-alsamixer --disable-xmlto
# make
# sudo make install
```

## Configure the board

### Copy files from the build directory to the root directory of the board.

```
# <path_to_build_directory>/alsa-lib/lib/*      -->  /usr/lib
# <path_to_build_directory>/alsa-utils/bin/*    -->  /usr/bin
# <path_to_build_directory>/alsa-utils/sbin/*   -->  /usr/bin
# <path_to_build_directory>/alsa-utils/share/*  -->  /usr/share
# /usr/share/alsa-arm/*                         -->  /usr/share/alsa-arm
```

### Set environment variables

```
# export ALSA_CONFIG_PATH=/usr/share/alsa-arm/alsa.conf
```

## Use tools

### Play audio

To see the list of playback hardware devices, you may run the command: aplay -l

For example, to play a file on soundcard 1, audio device 1, you may run the command:

  aplay -D hw:1,1 play.wav

### Record Audio

To see the list of capture hardware devices, you may run the command: arecord -l

For example, to capture a file on soundcard 0, audio device 1, you may run the command:

  arecord -D hw:0,1 -r 16000 -c 2 -f S16_LE test4.wav