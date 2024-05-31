mkdir build
cd build
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-g3 -DCMAKE_CXX_FLAGS_RELEASE=-g3 \
    -DTPU_SDK_PATH=$MLIR_PATH/tpuc \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build .


