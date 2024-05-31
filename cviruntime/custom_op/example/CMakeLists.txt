cmake_minimum_required(VERSION 2.8.0)
project(custom_cpu_function CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED MLIR_INCLUDE)
  message(FATAL_ERROR "Please set MLIR_INCLUDE to point to the include path of mlir")
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${MLIR_INCLUDE})

add_library(CustomOpPlugin SHARED
            LeakyReluOp.cpp
            ROIAlignOp.cpp
            SoftmaxOp.cpp
            UnPoolingOp.cpp)

install(TARGETS CustomOpPlugin DESTINATION lib/custom_op/)
