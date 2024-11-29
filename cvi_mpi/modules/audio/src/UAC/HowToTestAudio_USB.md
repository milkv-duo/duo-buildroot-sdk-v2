#### [Stable version]:
>init audio ...easy audio api[20201118]
>current version...20210412
> Above message should be printed while you initialize audio.

#### Quick build in middleware
SDK_VER=64bit make clean;
SDK_VER=64bit make
the output execution file:uacaudio

put it in cv183x board and run as:
./uacaudio



#### Ignore below old version message xxxxxxxx
#### [Environment Setup]:
- Make sure the linux kernel driver been updated.
- download the code from gitlab
 (http://gitlab.cvitek.com:8480/sys_app/cvi-uvc)

#### [Audio Build Method(uvcgadget)]:
- 1.Build with video
    ```sh
	vi /cvi-uvc/Makefile,
	(turn the USE_AUDIO = no ,to USE_AUDIO = yes)
	make -j12 SOURCE_DIR=$(your sdk folder) PREBUILT_DIR=./prebuilt
    ```
	->Put uvc-gadget and script folder into board.

- 2.Build audio unit test
    ```sh
	/cvi-uvc/audio/Makefile/ ,
	(Modify [SOURCE_DIR] and [CROSS_COMPILE] variable to
	your own location on PC.)
	make clean,
	make
    ```
	->Create uacaudio  exe file, put into board /mnt/data/
#### [How to test audio](After board environment setup ok)
- ###### 1.Method 1.(audiotest)
	modify cvi_uvc.sh in board, make uvc-gadget execute in background.
	Detach the sensor to test without video.
	##### [In board console]

    ```sh
	cd /script/
	./start_uvc.sh
	(Plugin the usb cable between board and PC)
	./cvi_uvc.sh
	cd /mnt/data/
	./uacaudio
    ```
	You can test two way audio now.

- ###### 2.Method 2 (uvc-gadget)
	Make sure the uvc-gadget was built with video.
	##### [In board console]
    ```sh
	cd /script/
	./start_uvc.sh
	(then plug in the usb cable between board and PC)
	./cvi_uvc.sh
    ```

	You can test two way audio now.
