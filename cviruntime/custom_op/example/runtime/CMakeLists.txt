cmake_minimum_required(VERSION 2.8.0)
project(custom_cpu_function CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CVIRUNTIME_INCLUDE})

add_library(CustomOpRuntime SHARED
            SoftmaxOpRuntime.cpp
            ROIAlignOpRuntime.cpp
            OpRuntimeRegister.cpp)
if(NOT CMAKE_CROSSCOMPILING)
  set_target_properties(CustomOpRuntime PROPERTIES SUFFIX "_x86.so")
else()
  set_target_properties(CustomOpRuntime PROPERTIES SUFFIX "_arm64.so")
endif()
install(TARGETS CustomOpRuntime DESTINATION lib/custom_op/)
