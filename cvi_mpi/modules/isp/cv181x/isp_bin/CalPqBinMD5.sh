#!/bin/sh

ERR_RESULT=-1
FILE_LIST="../../include/cv181x/cvi_comm_3a.h ../../include/cv181x/cvi_comm_isp.h"
OUTFILE="pqbin.$$"

for file in $FILE_LIST
do
    cat $file >> $OUTFILE 2>&1
    if [ $? -ne 0 ];then
        echo $ERR_RESULT
        rm $OUTFILE -f
        exit
    fi
done
result=`md5sum $OUTFILE |cut -d" " -f1`
if [ $? -eq 0 ];then
    echo $result
else
    echo $ERR_RESULT
fi
rm $OUTFILE -f
