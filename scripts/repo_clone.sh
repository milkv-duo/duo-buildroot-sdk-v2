#!/bin/bash

# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitpull subtree.xml
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml --normal
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml --reproduce git_version_2023-08-18.txt

function print_usage {
    echo "Usage: $0 arg1 arg2 [arg3] [arg4] [arg5 arg6]"
    echo "arg1: --gitclone or --gitpull(DEFAULT: git clone)"
    echo "arg2: xxx.xml"
    echo "arg3: --normal(DEFAULT: clone single branch)"
    echo "arg4: --reproduce"
    echo "arg5: git_version.txt"
}

# 打印等级及颜色设置
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 判断传参数目是否合法
if [[ $# -lt 2 ]]; then
    printusage
    exit 1
fi

# 获取 User Name
function get_user_name {
    USERNAME=$(git config --global user.name)
}
get_user_name

# git clone enable
DOWNLOAD=0

# normal clone enable
NORMAL=0

# reproduce enable
REPRODUCE=0

# 传入参数并赋值
while [[ $# -gt 0 ]]; do
   case $1 in
        --gitclone)
           shift
           xml_file=$1
           REMOTE_URL=$(grep -o '<remote.*/>' ${xml_file} | sed 's/.*fetch="\([^"]*\)".*/\1/' | sed "s/ssh:\/\/\(.*\)/ssh:\/\/$USERNAME@\1/")
           shift
           ;;
        --gitpull)
           shift
           xml_file=$1
           DOWNLOAD=1
           shift
           ;;
        --normal)
           NORMAL=1
           shift
           ;;
        --reproduce)
           shift
           gitver_txt=$1
           REPRODUCE=1
           shift
           ;;
        *)
           print_usage
           exit 1
           ;;
   esac
done

# 设置全局变量
REMOTE_NAME=$(grep -o '<remote.*/>' ${xml_file} | sed 's/.*name="\([^"]*\)".*/\1/')
DEFAULT_REVISION=$(grep -o '<default.*/>' ${xml_file} | sed 's/.*revision="\([^"]*\)".*/\1/')
PARAMETERS="--single-branch --depth 2000"

# 获取xml文件中所有的project name
repo_list=$(grep -o '<project name="[^"]*"' ${xml_file} | sed 's/.*name="\([^"]*\)".*/\1/')

# git clone function
function git_clone {

    revision=$(grep "name=\"$repo\"" ${xml_file} | sed -n 's/.*revision="\([^"]*\)".*/\1/p')
    path=$(grep "name=\"$repo\"" ${xml_file} | sed -n 's/.*path="\([^"]*\)".*/\1/p')
    sync=$(grep "name=\"$repo\"" ${xml_file} | sed -n 's/.*sync-s="\([^"]*\)".*/\1/p')

    # 判断参数是否带有revision
    if [[ ! -n $revision ]]; then
        revision=$DEFAULT_REVISION
    fi

    # 判断参数是否带有path
    if [[ ! -n $path ]]; then
        path=$repo
    fi

    # 判断参数是否带有normal
    if [ $NORMAL == 0 ]; then
        git clone $REMOTE_URL$repo.git $PWD/$path -b $revision $PARAMETERS
    else
        git clone $REMOTE_URL$repo.git $PWD/$path
        pushd $PWD/$path
          git checkout $revision
        popd
    fi

    # 判断参数是否带有sync-s
    if [[ "$sync" == "true" ]]; then
        echo -e "${YELLOW}Warning: Pay attention to managing $repo/, as it requires submodule management${NC}"
        pushd $PWD/$path
            git submodule update --init
        popd
    fi
}

# git pull function
function git_pull {

    path=$(grep "name=\"$repo\"" ${xml_file} | sed -n 's/.*path="\([^"]*\)".*/\1/p')

    # 判断参数是否带有path
    if [[ ! -n $path ]]; then
        path=$repo
    fi

    if [ ! -d $PWD/$path ]; then
        echo -e "\033[31merror, target porject already exist!! $PWD/$path}\033[0m"
        continue
    fi

    pushd $PWD/$path
      git pull
    popd
}

# git reproduce function
function reproduce_repo {
    local _repo=$1
    local _commit_id=$2
    echo "reproduce_repo: ""${_repo}" " reset to " "${_commit_id}"
    path=$(grep "name=\"${_repo}\"" ${xml_file} | sed -n 's/.*path="\([^"]*\)".*/\1/p')

    # 判断参数是否带有path
    if [[ ! -n ${path} ]]; then
        path=${_repo}
    fi

    if [ ! -d ${PWD}/${path} ]; then
        echo -e "\033[31merror, target porject not exist!! ${PWD}/${path}}\033[0m"
        continue
    fi

    pushd $PWD/$path
    git reset --hard "${_commit_id}" ||
    {
        echo "Error: Unable to reset to project ${_repo}";
        echo "Error: Unable to reset to commit ${_commit_id}";
        exit 1;
    }
    popd
}

function get_commit_id {
    local Project_name="$1"
    local File=$2
    local Commit_id=""

    # 使用awk搜索文件，找到匹配的project_name并提取commit_id
    Commit_id=$(awk -v proj="$Project_name" '
        /^project_name: / {
            if ($2 == proj) {
                found = 1;
                next;
            }
            found = 0;
        }
        found && /^commit_id: / {
            print $2;
            exit;  # 找到后退出awk,避免打印多余信息
        }
    ' "$File")

    echo "$Commit_id"
}

function main {
    for repo in $repo_list; do
        if [ $DOWNLOAD == 0 ]; then
            git_clone
        else
            git_pull
        fi
        # 复现git_version.txt中的所有仓库
        if [ $REPRODUCE -eq 1 ]; then
            item=$(basename ${repo})
            commit_id=$(get_commit_id ${item} ${gitver_txt})
            if [ "$commit_id" != "" ]; then
                reproduce_repo ${repo} ${commit_id}
            fi
        fi
    done
}

main
