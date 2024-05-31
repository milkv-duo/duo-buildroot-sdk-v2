cmake_minimum_required(VERSION 2.8.0)

project(insightface_vpss C CXX)

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

if(NOT DEFINED CHIP)
  message(FATAL_ERROR "Please set CHIP to 183x or 182x")
endif()

if(NOT DEFINED MW_PATH)
  message(FATAL_ERROR "Please set MW_PATH to point to the middleware installation")
endif()
include_directories(${MW_PATH}/include)
include_directories(${MW_PATH}/sample/common)
include_directories(${MW_PATH}/include/isp/cv${CHIP})
link_directories(${MW_PATH}/lib)
link_directories(${MW_PATH}/lib/3rd)

if (CHIP STREQUAL "182x")
  add_definitions(-DCHIP_182x)
  set(MW_LIB sample isp cvi_bin cvi_bin_isp isp_algo vpu venc vdec cvi_vcodec sys awb ae af sns_full ini cvitracer)
else()
  set(MW_LIB sample isp cvi_bin cvi_bin_isp vpu venc vdec cvi_vcodec sys awb ae af sns_full ini)
endif()

set(CVI_LIBS ${CVI_LIBS} cviruntime cvikernel)
if(NOT CMAKE_CROSSCOMPILING)
  set(CVI_LIBS ${CVI_LIBS} cvicmodel)
endif()

set(OPENCV_LIBS ${OPENCV_LIBS} opencv_core opencv_imgcodecs opencv_imgproc)
if(NOT CMAKE_CROSSCOMPILING)
  set(OPENCV_LIBS ${OPENCV_LIBS} opencv_highgui)
endif()

set(EXTRA_LIBS ${EXTRA_LIBS} dl stdc++ pthread z)


add_executable(insightface_vpss
    fd_fr_demo_mpi.cpp
    cvi_media_sdk.cpp
    affine_hw.cpp)
    
target_link_libraries(insightface_vpss
    ${CVI_LIBS}
    ${OPENCV_LIBS}
    ${EXTRA_LIBS}
    ${MW_LIB})

install(TARGETS insightface_vpss 
    insightface_vpss DESTINATION samples_extra/bin)
