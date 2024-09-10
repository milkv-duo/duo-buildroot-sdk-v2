#!/bin/bash
#Filename: test_funcs.sh
#Description: A script for testing functions

# Usage: sh test_funcs.sh -source ./images_folder

NBR_ITER=50
FUNC_NAME=("test_sobel_grad_c"
"test_filter_c"
"test_add_c"
"test_and_c"
"test_morph_c"
"test_norm_grad_c"
"test_sub_c"
"test_histogram_c"
"test_histEq_c"
"test_16bitTo8bit_c"
"test_integral_image_c"
"test_lbp_c"
"test_island_c"
#"test_ncc_c"
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
for i in ${!FUNC_NAME[*]}
do
  test_function=${FUNC_NAME[$i]}
  echo $test_function

  for img in `echo $source_dir/*` ;
  do
    source_file=$img
    PARAM="$NBR_ITER"
    #echo Processing file : $source_file
    #echo "./$test_function" $source_file $PARAM
    {
      rtn=$( "./$test_function" $source_file $PARAM )
    } || {
      echo ERROR $test_function #"$rtn"
    }
  done

done
