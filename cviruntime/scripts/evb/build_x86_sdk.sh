#!/bin/bash
echo "$WORKSPACE"

PROJECT_ROOT=~/workspace/sdk
BUILD_PATH=$PROJECT_ROOT/build_x86_64 
INSTALL_PATH=$PROJECT_ROOT/cvitek_tpu_sdk 
pushd $PROJECT_ROOT
rm -rf $BUILD_PATH 
rm -rf $INSTALL_PATH
mkdir -p $BUILD_PATH
mkdir -p $INSTALL_PATH

BUILD_FLAG="-DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3"

#build flatbuffers
mkdir -p $BUILD_PATH/flatbuffers
pushd $BUILD_PATH/flatbuffers
cmake -G Ninja \
    $PROJECT_ROOT/flatbuffers \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/flatbuffers
cmake --build . --target install
test $? -ne 0 && echo "build flatbuffers failed !!" && popd && exit 1
popd

#build cvikernel
mkdir -p $BUILD_PATH/cvikernel
pushd $BUILD_PATH/cvikernel
cmake -G Ninja -DCHIP=BM1880v2 $BUILD_FLAG \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/tpuc \
    $PROJECT_ROOT/cvikernel
cmake --build . --target install
test $? -ne 0 && echo "build cvikernel failed !!" && popd && exit 1
popd

# cvibuilder
mkdir -p $BUILD_PATH/cvimodel
pushd $BUILD_PATH/cvimodel
cmake -G Ninja -DFLATBUFFERS_PATH=$INSTALL_PATH/flatbuffers \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/tpuc \
    $PROJECT_ROOT/cvibuilder
cmake --build . --target install
test $? -ne 0 && echo "build cvibuilder failed !!" && popd && exit 1
popd

#build cnpy
mkdir -p $BUILD_PATH/cnpy
pushd $BUILD_PATH/cnpy
cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/cnpy \
    $PROJECT_ROOT/cnpy
cmake --build . --target install
test $? -ne 0 && echo "build cnpy failed !!" && popd && exit 1
popd
cp $INSTALL_PATH/cnpy/lib/* $INSTALL_PATH/tpuc/lib/

#build opencv
mkdir -p $BUILD_PATH/opencv
pushd $BUILD_PATH/opencv
cmake -G Ninja \
    $PROJECT_ROOT/oss/opencv \
    -DWITH_CUDA=OFF -DWITH_IPP=OFF -DWITH_LAPACK=OFF \
    -DWITH_DC1394=OFF -DWITH_GPHOTO2=OFF \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DPYTHON_DEFAULT_EXECUTABLE=$(which python3) \
    -DBUILD_opencv_videoio=OFF \
    -DBUILD_opencv_superres=OFF \
    -DBUILD_opencv_videostab=OFF \
    -DBUILD_opencv_stitching=OFF \
    -DBUILD_opencv_objdetect=OFF \
    -DBUILD_opencv_calib3d=OFF \
    -DBUILD_opencv_ml=OFF \
    -DBUILD_opencv_video=OFF \
    -DBUILD_opencv_flann=OFF \
    -DBUILD_opencv_photo=OFF \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/opencv
cmake --build . --target install
test $? -ne 0 && echo "build opencv failed !!" && popd && exit 1
popd


#build cmodel
mkdir -p $BUILD_PATH/cmodel
pushd $BUILD_PATH/cmodel
cmake -G Ninja -DCHIP=BM1880v2 $BUILD_FLAG \
    -DCVIKERNEL_PATH=$INSTALL_PATH/tpuc \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/tpuc \
    $PROJECT_ROOT/cmodel
cmake --build . --target install
test $? -ne 0 && echo "build cmodel failed !!" && popd && exit 1
popd


#build cviruntime
mkdir -p $BUILD_PATH/cviruntime
pushd $BUILD_PATH/cviruntime
cmake -G Ninja -DCHIP=BM1880v2 -DRUNTIME=CMODEL $BUILD_FLAG \
    -DCVIKERNEL_PATH=$INSTALL_PATH/tpuc \
    -DCMODEL_PATH=$INSTALL_PATH/tpuc \
    -DENABLE_PYRUNTIME=ON \
    -DFLATBUFFERS_PATH=$INSTALL_PATH/flatbuffers \
    -DCNPY_PATH=$INSTALL_PATH/cnpy \
    -DCVIBUILDER_PATH=$INSTALL_PATH/tpuc \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/tpuc \
    -DENABLE_TEST=OFF \
    $PROJECT_ROOT/cviruntime
cmake --build . --target install
test $? -ne 0 && echo "build cviruntime failed !!" && popd && exit 1
#ctest --progress || true
rm -f $INSTALL_PATH/tpuc/README.md
# rm -f $INSTALL_PATH/tpuc/envs_tpu_sdk.sh
rm -f $INSTALL_PATH/tpuc/regression_models.sh
rm -f $INSTALL_PATH/tpuc/regression_models_e2e.sh
rm -f $INSTALL_PATH/tpuc/regression_models_fused_preprocess.sh
popd

#adjust the directory
pushd $INSTALL_PATH
mkdir -p ./bin
mv ./cnpy/bin/* ./bin
rm -rf ./cnpy/bin
mv ./tpuc/bin/* ./bin
mv  ./tpuc/include ./
mv  ./tpuc/lib ./
mv ./tpuc/envs_tpu_sdk.sh .
rm -rf ./tpuc
popd

#build samples
mkdir -p $BUILD_PATH/samples
pushd $BUILD_PATH/samples
cmake -G Ninja $BUILD_FLAG \
    -DTPU_SDK_PATH=$INSTALL_PATH \
    -DCNPY_PATH=$INSTALL_PATH/cnpy \
    -DOPENCV_PATH=$INSTALL_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/samples \
    $PROJECT_ROOT/cviruntime/samples
cmake --build . --target install -- -v
test $? -ne 0 && echo "build samples failed !!" && popd && exit 1
popd

#get tpu_sdk_version
pushd $INSTALL_PATH
RELEASE_PATH="/data/dailyrelease/$(date '+%Y-%m-%d')-18.04"
version="$(./bin/model_runner | grep Runtime | cut -d ")" -f2 | cut -d "@" -f1)"
cat>$RELEASE_PATH/tpu_sdk_version.txt<<EOF
$version
EOF
popd

tar -zcf cvitek_tpu_sdk_x86_64.tar.gz cvitek_tpu_sdk
rm -rf $INSTALL_PATH
rm -rf $BUILD_PATH
popd

