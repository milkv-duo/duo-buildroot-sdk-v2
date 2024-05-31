cmake_minimum_required(VERSION 2.8.0)

project(cvi_sample_model_info C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED TPU_SDK_PATH)
  message(FATAL_ERROR "Please set TPU_SDK_PATH to point to the TPU_SDK installation")
endif()
include_directories(${TPU_SDK_PATH}/include)
link_directories(${TPU_SDK_PATH}/lib)

set(CVI_LIBS ${CVI_LIBS} cviruntime cvikernel)
set(EXTRA_LIBS ${EXTRA_LIBS} dl)

add_executable(cvi_sample_model_info
    model_info.c)
target_link_libraries(cvi_sample_model_info
    ${CVI_LIBS}
    ${EXTRA_LIBS})
install(TARGETS cvi_sample_model_info
    cvi_sample_model_info DESTINATION bin)
