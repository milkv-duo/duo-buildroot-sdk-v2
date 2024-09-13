#!/bin/bash

DEST="$1"
PLATFORM=`echo ${CHIP_ARCH} | tr A-Z a-z`

MODELS="
retinaface_mnet0.25_342_608=2021-0831
mobiledetv2-lite-ls=2021-0831
"
usage() {
    echo "$0 plaform [ cv183x | cv182x ] dest"
    exit 1
}

download_cvi_models() {
    mkdir -p ${DEST}
    touch ${DEST}/installed

    for model in ${1};
    do
        name=`echo ${model} | cut -d "=" -f1`
        version=`echo ${model} | cut -d "=" -f2`
        pkg="${name}_${version}_${PLATFORM}"
        installed_ver=`cat ${DEST}/installed | grep ${name}`

        if [ "${pkg}" = "${installed_ver}" ]; then
            continue
        fi

        curl "ftp://10.58.65.3/prebuilt/easy_build/cvi_models/${pkg}.tar.gz" | tar -xzC "${DEST}"
        if [ $? != 0 ]; then
            echo "fail to install ${pkg}"
            continue
        fi

        if [ "${installed_ver}" = "" ]; then
            echo ${pkg} >> ${DEST}/installed
        else
            sed -i "s/${installed_ver}/${pkg}/g" ${DEST}/installed
        fi
    done
}

download_cvi_models "${MODELS}"
