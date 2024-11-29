#!/bin/sh

rm ./misc/*.o

MW_DIR=../../../../../..
ISP_DIR=../../../..
ISP_DIR_INC=$ISP_DIR/cv181x/include
INC_DIR="-I./ -I../ -I./misc -I$MW_DIR/include -I$ISP_DIR/include/cv181x/ -I$ISP_DIR_INC -I./include_linux"
CFLAGS="-DAAA_LINUX_PLATFORM -DWBIN_SIM"

gcc $INC_DIR $CFLAGS -c ./misc/misc.c -o ./misc/misc.o
gcc $INC_DIR $CFLAGS -c ./misc/simple_jpg.c -o./misc/simple_jpg.o
gcc $INC_DIR $CFLAGS -c ./misc/bmp.c -o ./misc/bmp.o
gcc $INC_DIR $CFLAGS -c ./misc/json.c -o ./misc/json.o
gcc $INC_DIR $CFLAGS -c ./misc/getAttr_json.c -o ./misc/getAttr_json.o

ls ./misc/*.o


