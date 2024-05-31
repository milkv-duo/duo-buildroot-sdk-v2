cmake_minimum_required(VERSION 2.8.0)

project(megvii_euc C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED TPU_SDK_PATH)
  message(FATAL_ERROR "Please set TPU_SDK_PATH to point to the TPU_SDK installation")
endif()
include_directories(${TPU_SDK_PATH}/include)
link_directories(${TPU_SDK_PATH}/lib)

set(CVI_LIBS ${CVI_LIBS} cviruntime cvikernel)
if(NOT CMAKE_CROSSCOMPILING)
  set(CVI_LIBS ${CVI_LIBS} cvicmodel)
endif()

set(EXTRA_LIBS ${EXTRA_LIBS} dl stdc++ pthread z)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(megvii_euc
    main.cpp)
    
target_link_libraries(megvii_euc
    ${CVI_LIBS}
    ${EXTRA_LIBS})
