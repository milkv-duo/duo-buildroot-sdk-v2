#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CVI_TDL_ROOT=$(readlink -f $SCRIPT_DIR/../)
cd $CVI_TDL_ROOT

set -x
git diff --exit-code -- . ':(exclude).gitmodules' || exit 1
./clang-format.sh || exit 1
git diff --exit-code -- . ':(exclude).gitmodules' || exit 1