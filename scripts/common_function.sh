#!/bin/bash

export _DATE=$(date '+%Y-%m-%d')

function get_gitlog {
    # usage step1: source scripts/common_function.sh
    # usage step2: get_gitlog v4.1.0_sdk sophpi/scripts/git_version_v410
    sdk_path=$1
    output_path=$2
    dir_name=$(basename "$output_path")
    output_file=${output_path}/${dir_name}_${_DATE}.txt

    if [ ! -d ${sdk_path} ]; then
        echo -e "\033[31merror, sdk path not exist!! ${sdk_path}}\033[0m"
        exit 1
    fi

    rm -rf $output_file

    find $1 -type d -name ".git" | while read git_dir; do
        pushd "$(dirname "${git_dir}")" > /dev/null
            echo "project_name: $(git remote -v | grep "fetch" | awk -F'/' '{print $NF}' | awk -F'.git' '{print $1}')"
            echo "commit_id: $(git log --pretty=oneline -n 1)"
            echo ""
        popd > /dev/null
    done > "$output_file"
}