#!/bin/bash

# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitpull subtree.xml
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml --normal
# [Usage-Example] : ./cvi_manifest/repo_clone.sh --gitclone subtree.xml --reproduce git_version_2023-08-18.txt

function printusage {
    echo "Usage: $0 arg1 arg2 [arg3] [arg4] [arg5 arg6]"
    echo "arg1: --gitclone or --gitpull(DEFAULT: git clone)"
    echo "arg2: xxx.xml"
    echo "arg3: --normal(DEFAULT: clone single branch)"
    echo "arg4: --reproduce"
    echo "arg5: git_version.txt"
}

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
           dir=$1
           REMOTE_URL=$(grep -o '<remote.*/>' ${dir} | sed 's/.*fetch="\([^"]*\)".*/\1/' | sed "s/ssh:\/\/\(.*\)/ssh:\/\/$USERNAME@\1/")
           shift
           ;;
        --gitpull)
           shift
           dir=$1
           DOWNLOAD=1
           shift
           ;;
        --normal)
           NORMAL=1
           shift
           ;;
        --reproduce)
           shift
           txt=$1
           REPRODUCE=1
           shift
           ;;
        *)
           printusage
           exit 1
           ;;
   esac
done

# 设置全局变量
REMOTE_NAME=$(grep -o '<remote.*/>' ${dir} | sed 's/.*name="\([^"]*\)".*/\1/')
DEFAULT_REVISION=$(grep -o '<default.*/>' ${dir} | sed 's/.*revision="\([^"]*\)".*/\1/')
PARAMETERS="--single-branch --depth 200"

# 获取xml文件中所有的project name
repo_list=$(grep -o '<project name="[^"]*"' ${dir} | sed 's/.*name="\([^"]*\)".*/\1/')

# git clone function
function git_clone {

    revision=$(grep "name=\"$repo\"" ${dir} | sed -n 's/.*revision="\([^"]*\)".*/\1/p')
    path=$(grep "name=\"$repo\"" ${dir} | sed -n 's/.*path="\([^"]*\)".*/\1/p')

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
	echo "git clone $REMOTE_URL$repo.git $PWD/$path -b $revision $PARAMETERS"
        git clone $REMOTE_URL$repo.git $PWD/$path -b $revision $PARAMETERS
    else
        git clone $REMOTE_URL$repo.git $PWD/$path
        pushd $PWD/$path
          git checkout $revision
        popd
    fi
}

# git pull function
function git_pull {

    path=$(grep "name=\"$repo\"" ${dir} | sed -n 's/.*path="\([^"]*\)".*/\1/p')

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
function git_reproduce {

    while IFS= read -r project_line && IFS= read -r commit_line; do
        project=$(echo "$project_line" | cut -d ' ' -f 2)
        commit=$(echo "$commit_line" | cut -d ' ' -f 1)

        # reset
        # 判断 project 是否为合法路径
        if [ -d $project ]; then
            pushd "$project"
            # 判断 project 是否为合法git仓库
            if [ -d ".git" ]; then
                git reset --hard "$commit" ||
                    {
                        echo "Error: Unable to reset to project $project"; 
                        echo "Error: Unable to reset to commit $commit"; 
                        exit 1;
                    }
            else
                echo "Error: '$project' is not a Git repository."
            fi
            popd
        else
            echo "Error: Directory '$project' does not exist."
        fi

        # 读取并丢弃空行
        IFS= read -r empty_line
    done < $txt
}

function main {

    # 遍历每个project，并使用0: git clone 或者1：git pull
    for repo in $repo_list
    do
        if [ $DOWNLOAD == 0 ]; then
            git_clone
        else
            git_pull
        fi
    done

    # 复现git_version.txt中的所有仓库
    if [ $REPRODUCE -eq 1 ]; then
        git_reproduce
    fi
}

main
