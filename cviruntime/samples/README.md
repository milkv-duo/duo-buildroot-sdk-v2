# Samples for CVI TPU SDK

## Catalogue
| name                              | sample simple introduce |  
| --------------------------------- | :------------: |
| classifier                         |      sample without fuse_preprocess and quant to int8       |
| classifier_bf16                    |      sample without fuse_preprocess and quant to bf16       |
| classifier_fused_preprocess        |      sample with fuse_preprocess and quant to int8         |
| classifier_multi_batch             |      sample with multiple batch merged model       | 

## Sample introduction
### classifier_bf16
For model deployment, you need to refer to this sample first and convert it into a bf16 model to evaluate the model accuracy effect in the business scenario. For preprocessing, you can refer to the sample and use OpenCV to implement it.

### classifier
Ensure that the sample of bf16 model can be adjusted normally. Next, we can see the implementation of quantification as int8. Similarly, the pretreatment can be implemented by using OpenCV with reference to sample.

### classifier_fused_preprocess
If the pre-processing of the model takes a long time, you can bring --fuse_preprocess parameter allows TPU to implement partial preprocessing of the model to reduce the time of model preprocessing or memory copying.

### classifier_multi_batch
The combination of two models can share weight and memory, and support the implementation of different batches of the same model. Please refer to this sample.

## How to Compile image input sample in docker

The following documents are required:

* cvitek_tpu_sdk_[cv182x|cv182x_uclibc|cv183x|cv181x_glibc32|cv181x_musl_riscv64].tar.gz
* cvitek_tpu_samples.tar.gz

**64 bit platform**

``` shell
tar zxf cvitek_tpu_sdk_cv183x.tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-aarch64-linux.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

**32 bit platform**

``` shell
tar zxf cvitek_tpu_sdk_[cv182x|cv181x_glibc32].tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-linux-gnueabihf.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

**uclibc platform**

``` shell
tar zxf cvitek_tpu_sdk_cv182x_uclibc.tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-linux-uclibc.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

**cv181x musl riscv64 platform**

``` shell
tar zxf cvitek_tpu_sdk_cv181x_musl_riscv64.tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-riscv64-linux-musl-x86_64.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

Fianlly, copy install_samples folder to development board.

## How to Compile vpss input sample in docker

just add -DMW_PATH in cmake

The following documents are required:

* cvitek_tpu_sdk_[cv182x|cv182x_uclibc|cv183x|cv181x_glibc32|cv181x_musl_riscv64].tar.gz
* cvitek_tpu_samples.tar.gz
* mw.tar.gz


**64 bit platform**

``` shell
mkdir mw_path
tar -zxvf mw.tar.gz -C mw_path
export MW_PATH=$PWD/mw_path

tar zxf cvitek_tpu_sdk_cv183x.tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-aarch64-linux.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DMW_PATH=$MW_PATH \
    -DCHIP=183x \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

Fianlly, copy install_samples folder to development board.

**musl platform**
``` shell
mkdir mw_path
tar -zxvf mw.tar.gz -C mw_path
export MW_PATH=$PWD/mw_path

tar zxf cvitek_tpu_sdk_cv181x_musl_riscv64.tar.gz
export TPU_SDK_PATH=$PWD/cvitek_tpu_sdk
cd cvitek_tpu_sdk && source ./envs_tpu_sdk.sh && cd ..

tar zxf cvitek_tpu_samples.tar.gz
cd cvitek_tpu_samples
mkdir build_soc
cd build_soc
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=RELEASE \
    -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 \
    -DCMAKE_TOOLCHAIN_FILE=$TPU_SDK_PATH/cmake/toolchain-riscv64-linux-musl-x86_64.cmake \
    -DTPU_SDK_PATH=$TPU_SDK_PATH \
    -DSDK_VER=musl_riscv64 \
    -DOPENCV_PATH=$TPU_SDK_PATH/opencv \
    -DMW_PATH=$MW_PATH \
    -DCHIP=mars \
    -DCMAKE_INSTALL_PREFIX=../install_samples \
    ..
cmake --build . --target install
```

Fianlly, copy install_samples folder to development board.