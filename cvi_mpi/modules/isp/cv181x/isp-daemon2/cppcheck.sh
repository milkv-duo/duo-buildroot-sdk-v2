#!/bin/bash

SOC_CHIP_NAME=`echo ${CHIP_ARCH} | tr A-Z a-z`

BASE_DIR=.
ISP_BASE_DIR=${BASE_DIR}/../..
MW_BASE_DIR=${BASE_DIR}/../../../..

ISP_SOC_HEADER_DIR=${ISP_BASE_DIR}/include/${SOC_CHIP_NAME}
ISP_SOC_BASE_DIR=${BASE_DIR}/..
ISP_COMMON_DIR=${ISP_BASE_DIR}/common

INC_DIRS=""
INC_DIRS+=" -I ${BASE_DIR}/inc -I ${BASE_DIR}/src"
INC_DIRS+=" -I ${ISP_SOC_BASE_DIR}/isp/inc"
INC_DIRS+=" -I ${ISP_SOC_BASE_DIR}/isp_algo/inc"
INC_DIRS+=" -I ${ISP_SOC_BASE_DIR}/algo"
INC_DIRS+=" -I ${ISP_SOC_HEADER_DIR}"
INC_DIRS+=" -I ${MW_BASE_DIR}/include"
INC_DIRS+=" -I ${ISP_COMMON_DIR}/raw_replay -I ${ISP_COMMON_DIR}/raw_dump/inc"

SRC_DIRS="${BASE_DIR}/src"

#TEMPLATE="cppcheck1"
TEMPLATE="{file},{line},{severity},{id}:{message}"
cppcheck --enable=all --quiet --template=${TEMPLATE} ${INC_DIRS} ${SRC_DIRS} 2> cppcheck_output.txt
