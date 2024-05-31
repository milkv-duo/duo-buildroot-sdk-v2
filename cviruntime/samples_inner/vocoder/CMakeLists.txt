cmake_minimum_required(VERSION 2.8.0)

project(cvi_sample_detector C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED TPU_SDK_PATH)
  message(FATAL_ERROR "Please set TPU_SDK_PATH to point to the TPU_SDK installation")
endif()
include_directories(${TPU_SDK_PATH}/include)
link_directories(${TPU_SDK_PATH}/lib)

if(NOT DEFINED OPENCV_PATH)
  message(FATAL_ERROR "Please set OPENCV_PATH to point to the opencvn installation")
endif()
include_directories(${OPENCV_PATH}/include)
link_directories(${OPENCV_PATH}/lib)

set(CVI_LIBS ${CVI_LIBS} cviruntime cvikernel cnpy)
set(OPENCV_LIBS ${OPENCV_LIBS} opencv_core opencv_imgcodecs opencv_imgproc)
set(EXTRA_LIBS ${EXTRA_LIBS} dl stdc++ pthread z)

add_executable(vocoder vocoder.cpp vocoder_model.cpp)
target_link_libraries(vocoder
    ${CVI_LIBS}
    ${OPENCV_LIBS}
    ${EXTRA_LIBS})
install(TARGETS vocoder vocoder DESTINATION bin)
