#! /bin/bash

#genrate result.txt & outputDir to autotest dir
AUTO_TEST_PATH=./autotest
APP=./awbalgo

#please input your test folder and use json
RES_PATH="/media/cvitek/oliver.he/back"
JSON="/media/cvitek/oliver.he/2059_307_20220307-V2.json"

function clean() {
    rm output -rf
    rm result.txt -rf
}

function check() {
    if [ ! -d  $RES_PATH ]
    then
        echo "please check RES_PATH:$RES_PATH is valid"
    exit
    fi
    if [ ! -f  $JSON ]
    then
        echo "please check JSON:$JSON is valid"
    exit
    fi
}

function build() {
    ./m.sh
}

function getAndRun_wbin(){
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            getAndRun_wbin $1"/"$file
        else
            if [[ $file == *.wbin ]]
            then
                $APP $1"/judge.txt" $1"/"$file $JSON
            fi
        fi
    done
}

function checkAndSave_result(){
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            checkAndSave_result $1"/"$file
        else
            if [[ $file == WB.txt ]]
            then
                if [[ `grep "FAIL" $1"/"$file` == "FAIL" ]]
                then
                    mv $1 $1"_FAIL"
                    echo $1 "FAIL" >> result.txt
                else
                    mv $1 $1"_PASS"
                    echo $1 "PASS" >> result.txt
                fi
            fi
        fi
    done
}

cd $AUTO_TEST_PATH
check
clean
build
getAndRun_wbin $RES_PATH
checkAndSave_result "./output"
