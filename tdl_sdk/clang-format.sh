#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLANG_ROOT=$(readlink -f $SCRIPT_DIR)

OS_VERSION=$(cat /etc/os-release | grep VERSION_ID)
if [[ $OS_VERSION == *"20"* ]]; then
  CLANG_FORMAT=clang-format-10
else
  CLANG_FORMAT=clang-format-5.0
fi

find $CLANG_ROOT/include -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' | xargs $CLANG_FORMAT -i || exit 1
find $CLANG_ROOT/modules -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' | xargs $CLANG_FORMAT -i || exit 1
find $CLANG_ROOT/sample -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' | xargs $CLANG_FORMAT -i || exit 1
find $CLANG_ROOT/tool -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' | xargs $CLANG_FORMAT -i || exit 1
find $CLANG_ROOT/regression -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' | xargs $CLANG_FORMAT -i || exit 1
