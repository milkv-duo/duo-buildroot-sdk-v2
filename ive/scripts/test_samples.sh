#!/bin/bash
#Filename: test_funcs.sh
#Description: A script for testing functions

# Usage: sh test_funcs.sh -source ./images_folder

NBR_ITER=1
FUNC_NAME=("sample_add"
"sample_and"
"sample_filter"
"sample_hog"
"sample_magandang"
"sample_morph"
"sample_or"
"sample_ordstat"
"sample_sub"
"sample_thresh16"
"sample_xor"
"sample_16bitTo8bit"
"sample_histogram"
"sample_integral_image"
"sample_histEq"
"sample_lbp"
"sample_ordstat"
)
#"sample_normgrad"
#"sample_copy"

FUNC_NAME_TWO_IMGS=("sample_ncc"
"sample_sad"
)

FUNC_NAME_WO_IMGS=("sample_map"
"sample_block"
"sample_island"
)


if [ $# -ne 2 -a $# -ne 4 -a $# -ne 6 -a $# -ne 8 ];
then
  echo Incorrect number of arguments
  exit 2
fi

while [ $# -ne 0 ];
do

  case $1 in
  -source) shift; source_dir=$1 ; shift ;;
  -dest) shift ; dest_dir=$1 ; shift ;;
  *) echo Wrong parameters; exit 2 ;;
  esac;

done

# w/o images
for i in ${!FUNC_NAME_WO_IMGS[*]}
do
  test_function=${FUNC_NAME_WO_IMGS[$i]}
  #echo $test_function

  for img in `echo $source_dir/*` ;
  do
    source_file=$img
    PARAM="$NBR_ITER"
    #echo Processing file : $source_file
    #echo "./$test_function" $source_file $PARAM
    {
      #rtn=$( "./$test_function" $source_file $PARAM )
      {
          rtn=$( "./$test_function" )
      } && {
          echo $test_function TEST-PASS #"$rtn"
      }
    } || {
      echo $test_function ERROR #"$rtn"
    }
  done

done

# one image input
for i in ${!FUNC_NAME[*]}
do
  test_function=${FUNC_NAME[$i]}
  #echo $test_function

  for img in `echo $source_dir/*` ;
  do
    source_file=$img
    PARAM="$NBR_ITER"
    #echo Processing file : $source_file
    #echo "./$test_function" $source_file $PARAM
    {
      #rtn=$( "./$test_function" $source_file $PARAM )
      {
          rtn=$( "./$test_function" $source_file )
      } && {
          echo $test_function TEST-PASS #"$rtn"
      }
    } || {
      echo $test_function ERROR #"$rtn"
    }
  done

done

# two images input
for i in ${!FUNC_NAME_TWO_IMGS[*]}
do
  test_function=${FUNC_NAME_TWO_IMGS[$i]}
  #echo $test_function

  for img in `echo $source_dir/*` ;
  do
    source_file=$img
    PARAM="$NBR_ITER"
    #echo Processing file : $source_file
    #echo "./$test_function" $source_file $PARAM
    {
      #rtn=$( "./$test_function" $source_file $PARAM )
      {
          rtn=$( "./$test_function" $source_file $source_file)
      } && {
          echo $test_function TEST-PASS #"$rtn"
      }
    } || {
      echo $test_function ERROR #"$rtn"
    }
  done

done