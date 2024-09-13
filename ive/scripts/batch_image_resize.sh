#!/bin/bash
#Filename: batch_image_resize.sh
#Description: A script for image management

# Usage: sh batch_image_resize.sh -source ./images -fixed_wh 1 -dest ./new_folder

SIZES_W=("320" "640" "720" "1024")
SIZES_H=("240" "480" "576" "768")

if [ $# -ne 4 -a $# -ne 6 -a $# -ne 8 ];
then
  echo Incorrect number of arguments
  exit 2
fi

while [ $# -ne 0 ];
do

  case $1 in
  -source) shift; source_dir=$1 ; shift ;;
  -scale) shift; scale=$1 ; shift ;;
  -percent) shift; percent=$1 ; shift ;;
  -fixed_wh) shift; fixed_wh=$1 ; shift ;;
  -dest) shift ; dest_dir=$1 ; shift ;;
  -ext) shift ; ext=$1 ; shift ;;
  *) echo Wrong parameters; exit 2 ;;
  esac;

done
for i in ${!SIZES_W[*]}
do
  SIZEW=${SIZES_W[$i]}
  SIZEH=${SIZES_H[$i]}
  echo $SIZEW $SIZEH
  folderName="$SIZEW"x"$SIZEH"
  subfolder="$dest_dir/$folderName"
  mkdir -p $subfolder


  for img in `echo $source_dir/*` ;
  do
    source_file=$img
    if [[ -n $ext ]];
    then
      dest_file=${img%.*}.$ext
    else
      dest_file=$img
    fi

    if [[ -n $subfolder ]];
    then
      dest_file=${dest_file##*/}
      dest_file="$subfolder/$dest_file"
    fi

    if [[ -n $scale ]];
    then
      PARAM="-resize $scale"
    elif [[ -n $percent ]];   then
      PARAM="-resize $percent%"
    elif [[ -n $fixed_wh ]];   then
      PARAM="-resize $SIZEWx$SIZEH"
    fi

    echo Processing file : $source_file
    convert $source_file $PARAM $dest_file

  done

done
