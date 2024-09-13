 #!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLANG_ROOT=$(readlink -f $SCRIPT_DIR)

run-clang-tidy.py $CLANG_ROOT/modules \
                  -p $CLANG_ROOT/build \
                  -header-filter="$CLANG_ROOT/include/.*" \
                  -checks=-*,readability-*,clang-analyzer-*,performance-* \
                  -j$(nproc)