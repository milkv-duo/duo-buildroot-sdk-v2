#!/bin/bash

# 打印用法信息
function print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --gitclone xxx.xml      Clone the repo in xml file"
    echo "  --gitpull xxx.xml       Pull the repo in xml file"
    echo "  --normal                Clone the repo with all branches"
    echo "  --reproduce xxx.txt     Reproduce the repo as per the commit_id in xxx.txt"
    echo ""
    echo "Example:"
    echo "  $0 --gitclone scripts/subtree.xml"
    echo "  $0 --gitpull scripts/subtree.xml"
    echo "  $0 --gitclone scripts/subtree.xml --normal"
    echo "  $0 --gitclone scripts/subtree.xml --reproduce scripts/git_version_v410/git_version_v410_2024-11-18.txt"
}

# 打印等级及颜色设置
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 判断传参数目是否合法
if [[ $# -lt 2 ]]; then
    print_usage
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
           DOWNLOAD=1
           REMOTE_URL=$(grep -o '<remote.*/>' ${xml_file} | sed 's/.*fetch="\([^"]*\)".*/\1/' | sed "s/ssh:\/\/\(.*\)/ssh:\/\/$USERNAME@\1/")
           shift
           ;;
        --gitpull)
           shift
           xml_file=$1
           DOWNLOAD=0
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

# git clone function
function git_clone {
    # 逐行读取 XML 文件
    while IFS= read -r line
    do
        repo=$(echo "$line" | grep -o '<project name="[^"]*"' | sed 's/.*name="\([^"]*\)".*/\1/')
        revision=$(echo "$line" | grep -o 'revision="[^"]*"' | sed 's/.*revision="\([^"]*\)".*/\1/')
        path=$(echo "$line" | grep -o 'path="[^"]*"' | sed 's/.*path="\([^"]*\)".*/\1/')
        sync=$(echo "$line" | grep -o 'sync-s="[^"]*"' | sed 's/.*sync-s="\([^"]*\)".*/\1/')

        # 判断参数是否匹配到project行
        if [[ -z $repo ]]; then
            continue
        fi

        # 判断参数是否带有revision
        if [[ -z $revision ]]; then
            revision=$DEFAULT_REVISION
        fi

        # 判断参数是否带有path
        if [[ -z $path ]]; then
            path=$repo
        fi

        # 判断是否已经下载了repo
        if [[ -d $PWD/$path ]]; then
            echo -e "${YELLOW}Warning: target porject already exist!! $PWD/$path${NC}"
            continue
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
    done < ${xml_file}
}

# git pull function
function git_pull {
    # 逐行读取 XML 文件
    while IFS= read -r line
    do
        repo=$(echo "$line" | grep -o '<project name="[^"]*"' | sed 's/.*name="\([^"]*\)".*/\1/')
        revision=$(echo "$line" | grep -o 'revision="[^"]*"' | sed 's/.*revision="\([^"]*\)".*/\1/')
        path=$(echo "$line" | grep -o 'path="[^"]*"' | sed 's/.*path="\([^"]*\)".*/\1/')

        # 判断参数是否匹配到project行
        if [[ -z $repo ]]; then
            continue
        fi

        # 判断参数是否带有revision
        if [[ -z $revision ]]; then
            revision=$DEFAULT_REVISION
        fi

        # 判断参数是否带有path
        if [[ -z $path ]]; then
            path=$repo
        fi

        if [[ ! -d $PWD/$path ]]; then
            echo -e "${RED}ERROR: target porject not exist!! $PWD/$path${NC}"
            continue
        fi

        pushd $PWD/$path
            git checkout .
            git clean -f .
            git reset --hard $REMOTE_NAME/$revision
            git pull
        popd
    done < ${xml_file}
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
            split($0, parts, " ");  # 将整行按空格拆分
            print parts[2];         # 打印第二个字段，即提交 ID
            exit;                   # 找到后退出awk,避免打印多余信息
        }
    ' "$File")

    echo "$Commit_id"
}

# git reproduce function
function reproduce_repo {
    # 逐行读取 XML 文件
    while IFS= read -r line
    do
        repo=$(echo "$line" | grep -o '<project name="[^"]*"' | sed 's/.*name="\([^"]*\)".*/\1/')
        path=$(echo "$line" | grep -o 'path="[^"]*"' | sed 's/.*path="\([^"]*\)".*/\1/')

        # 判断参数是否匹配到project行
        if [[ -z $repo ]]; then
            continue
        fi

        commit_id=$(get_commit_id $(basename ${repo}) ${gitver_txt})

        # 判断是否从txt中读取到commit_id
        if [[ -z $commit_id ]]; then
            continue
        fi

        echo -e "\nReproduce: ""${repo}" " reset to " "${commit_id}"

        # 判断参数是否带有path
        if [[ -z $path ]]; then
            path=$repo
        fi

        if [[ ! -d $PWD/$path ]]; then
            echo -e "${RED}ERROR: target porject not exist!! $PWD/$path${NC}"
            continue
        fi

        pushd $PWD/$path
            git reset --hard "${commit_id}" ||
            {
                echo "${RED}ERROR: Unable to reset to project ${repo}${NC}";
                echo "${RED}ERROR: Unable to reset to commit ${commit_id}${NC}";
                exit 1;
            }
        popd
    done < ${xml_file}
}

function main {
    if [ $DOWNLOAD -eq 1 ]; then
        git_clone
    else
        git_pull
    fi

    # 复现git_version.txt中的所有仓库
    if [ $REPRODUCE -eq 1 ]; then
        reproduce_repo
    fi
}

main
