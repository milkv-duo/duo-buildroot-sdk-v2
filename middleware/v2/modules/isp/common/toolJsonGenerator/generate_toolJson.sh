#!/bin/bash
CHIP_ID=$1
DATE=$(date +%Y%m%d)
#toolJsonGenerator repo
GENERATOR_CHANGE_ID=$(git log -n1 -- $PWD | grep Change-Id: | sed 's/^.*Change-Id: //g' | cut -c1-7)
GENERATOR_COMMIT_ID=$(git log -n1 --pretty=format:'%h' -- $PWD)
GENERATOR_V="(${GENERATOR_CHANGE_ID},${GENERATOR_COMMIT_ID})"
ISP_BRANCH="v4.1.0"
#isp repo
cd $PWD/../../
ISP_CHANGE_ID=$(git log -n1 -- $PWD | grep Change-Id: | sed 's/^.*Change-Id: //g' | cut -c1-7)
ISP_COMMIT_ID=$(git log -n1 --pretty=format:'%h' -- $PWD)
ISP_V="(${ISP_CHANGE_ID},${ISP_COMMIT_ID})"
cd -
CUR_PATH=$(cd "$(dirname "$0")"; pwd)
DEVICE_FILE="${CUR_PATH}/device.json"
OUTPUT="pqtool_definition.json"
OUTPUT_TEMP="pqtool_definition_temp.json"

#config path by chip id
if [ $CHIP_ID == "cv181x" ]
then
    OSDRV_INCLUDE_PATH=${CUR_PATH}/../../../../../../osdrv/interdrv/v2/include/chip/cv181x/uapi/linux
    OSDRV_INCLUDE_LINUX_PATH=$OSDRV_INCLUDE_PATH/../../../../common/uapi/linux
    MW_INCLUDE_PATH=${CUR_PATH}/../../../../../v2/include
    ISP_INCLUDE_PATH=${CUR_PATH}/../../include/$CHIP_ID
    RPCJSON_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2
    OUTPUT_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2/src/
elif [ $CHIP_ID == "cv180x" ]
then
    OSDRV_INCLUDE_PATH=${CUR_PATH}/../../../../../../osdrv/interdrv/v2/include/chip/cv180x/uapi/linux
    OSDRV_INCLUDE_LINUX_PATH=$OSDRV_INCLUDE_PATH/../../../../common/uapi/linux
    MW_INCLUDE_PATH=${CUR_PATH}/../../../../../v2/include
    ISP_INCLUDE_PATH=${CUR_PATH}/../../include/$CHIP_ID
    RPCJSON_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2
    OUTPUT_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2/src/
elif [ $CHIP_ID == "cv182x" ]
then
    MW_INCLUDE_PATH=${CUR_PATH}/../../../../../v1/include
    ISP_INCLUDE_PATH=${CUR_PATH}/../../include/$CHIP_ID
    RPCJSON_PATH=${CUR_PATH}/../../$CHIP_ID/isp-daemon2
    OUTPUT_PATH=${CUR_PATH}/../../$CHIP_ID/isp-tool-daemon/
fi
HEADERLIST="$ISP_INCLUDE_PATH/cvi_comm_isp.h"
HEADERLIST+=" $ISP_INCLUDE_PATH/cvi_comm_3a.h"
HEADERLIST+=" $OSDRV_INCLUDE_LINUX_PATH/cvi_comm_video.h"
HEADERLIST+=" $MW_INCLUDE_PATH/cvi_comm_sns.h"

if [ $CHIP_ID == "cv181x" ] || [ $CHIP_ID == "cv180x" ]
then
    HEADERLIST+=" $OSDRV_INCLUDE_LINUX_PATH/cvi_comm_vi.h"
    HEADERLIST+=" $OSDRV_INCLUDE_LINUX_PATH/cvi_comm_vpss.h"
    HEADERLIST+=" $OSDRV_INCLUDE_PATH/cvi_defines.h"
fi
LEVELJSON=$CHIP_ID/level.json
LAYOUTJSON=$CHIP_ID/layout.json
RPCJSON=$RPCJSON_PATH/rpc.json

#reset&update device.json
git checkout ${DEVICE_FILE}
sed -i 's/"FULL_NAME": ""/"FULL_NAME": "'"${CHIP_ID}"'"/g' ${DEVICE_FILE}
sed -i 's/"CODE_NAME": ""/"CODE_NAME": "'"${CHIP_ID}"'"/g' ${DEVICE_FILE}
sed -i 's/"SDK_VERSION": ""/"SDK_VERSION": "'"${DATE}"'"/g' ${DEVICE_FILE}
sed -i 's/"GENERATOR_VERSION": ""/"GENERATOR_VERSION": "'"${GENERATOR_V}"'"/g' ${DEVICE_FILE}
sed -i 's/"ISP_VERSION": ""/"ISP_VERSION": "'"${ISP_V}"'"/g' ${DEVICE_FILE}
sed -i 's/"ISP_BRANCH": ""/"ISP_BRANCH": "'"${ISP_BRANCH}"'"/g' ${DEVICE_FILE}

#generate pqtool_definition.json
cd ${CUR_PATH}
python hFile2json.py  $LEVELJSON $LAYOUTJSON $RPCJSON $HEADERLIST
git checkout ${DEVICE_FILE}

#check json file validity
cat ${OUTPUT} | python -m json.tool 1>/dev/null 2>json_error
error=$(cat json_error)
if [ ${#error} -eq 0 ]
then
    rm -r json_error
else
    echo  "the json file is a invalid, error is : ${error}!!!"
    exit -1
fi

#cp pqtool_definition.json
if [ -f ${OUTPUT} ]
then
    if [ -x "$(command -v zip)" ]
    then
        zip ${OUTPUT_TEMP} ${OUTPUT}
        mv ${OUTPUT_TEMP} ${OUTPUT}
    fi

    if [ $CHIP_ID == "cv181x" ] || [ $CHIP_ID == "cv180x" ]
    then
        xxd -i ${OUTPUT} > ${OUTPUT_PATH}/cvi_pqtool_json.h
        echo "Success!! generate cvi_pqtool_json.h to src dir of isp-daemon2 done"
    else
        cp ${OUTPUT} ${OUTPUT_PATH}
        echo "Success!! mv pqtool_definition.json to isp-tool-daemon done"
    fi
else
    echo "FAIL!! please check gen pqtool_definition.json flow"
fi

