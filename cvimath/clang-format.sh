#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLANG_ROOT=$(readlink -f $SCRIPT_DIR)

find $CLANG_ROOT/include -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' -exec clang-format -i {} \;
find $CLANG_ROOT/src -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' -exec clang-format -i {} \;
find $CLANG_ROOT/tests -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' -exec clang-format -i {} \;
find $CLANG_ROOT/sample -regex '.*\.\(cpp\|h\|hpp\|cc\|c\|cxx\|inc\)' -exec clang-format -i {} \;
