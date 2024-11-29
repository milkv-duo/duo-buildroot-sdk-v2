#!/bin/bash

CHIP_ID=$1
CUR_PATH=$(cd "$(dirname "$0")"; pwd)
ISP_BASE_PATH=${CUR_PATH}/../..
MW_INCLUDE_PATH=${ISP_BASE_PATH}/../../include
ISP_INCLUDE_PATH=${ISP_BASE_PATH}/include/${CHIP_ID}

HEADERLIST="${ISP_INCLUDE_PATH}/cvi_comm_isp.h"
HEADERLIST+=" ${ISP_INCLUDE_PATH}/cvi_comm_3a.h"
HEADERLIST+=" ${MW_INCLUDE_PATH}/cvi_comm_video.h"
HEADERLIST+=" ${MW_INCLUDE_PATH}/cvi_comm_vo.h"
HEADERLIST+=" ${MW_INCLUDE_PATH}/cvi_comm_sns.h"
ISP_JSON_STRUCTFILE="${ISP_BASE_PATH}/${CHIP_ID}/isp_bin/src/isp_json_struct.c"
APP=checkPqBinJsonIntegrity.py
cd ${CUR_PATH}
python ${APP} ${ISP_JSON_STRUCTFILE} ${HEADERLIST}

