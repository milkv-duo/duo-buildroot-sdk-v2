#!/bin/sh

if [ -f ./awbalgo ]; then
	rm ./awbalgo
	echo rm awbalgo
fi

MW_DIR=../../../../../..
ISP_DIR=../../../..
ISP_DIR_INC=$ISP_DIR/cv181x/include
INC_DIR="-I./ -I../ -I./misc -I$MW_DIR/include -I$ISP_DIR/include/cv181x/ -I$ISP_DIR_INC -I./include_linux"
CFLAGS="-DAAA_LINUX_PLATFORM -DWBIN_SIM"
MISC_OBJ="./misc/misc.o ./misc/simple_jpg.o ./misc/bmp.o ./misc/json.o ./misc/getAttr_json.o"
gcc -oawbalgo  $INC_DIR $CFLAGS awb_test.c awb_platform.c ../awbalgo.c  $MISC_OBJ -lm

if [ -f ./awbalgo ]; then
	echo Build Ok....
fi
