cmake_minimum_required(VERSION 2.8.0)

project(cvi_sample_classifier_tpu_preprocess C CXX)

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
  message(FATAL_ERROR "Please set CHIP to 183x or 182x or 181x or mars")
endif()

if(NOT DEFINED MW_PATH)
  message(FATAL_ERROR "Please set MW_PATH to point to the middleware installation")
endif()

include_directories(${MW_PATH}/include)
include_directories(${MW_PATH}/sample/common)

if (CHIP STREQUAL "mars")
  include_directories(${MW_PATH}/include/isp/${CHIP})
else()
  include_directories(${MW_PATH}/include/isp/cv${CHIP})
endif()

if("${SDK_VER}" STREQUAL "musl_riscv64")
  link_directories(${MW_PATH}/lib_musl_riscv64)
  link_directories(${MW_PATH}/lib_musl_riscv64/3rd)
elseif("${SDK_VER}" STREQUAL "glibc_riscv64")
  link_directories(${MW_PATH}/lib_glibc_riscv64)
  link_directories(${MW_PATH}/lib_glibc_riscv64/3rd)
else()
  link_directories(${MW_PATH}/lib)
  link_directories(${MW_PATH}/lib/3rd)
endif()

if(CHIP STREQUAL "mars")
  set(MW_LIB sample isp cvi_bin_isp cvi_bin isp_algo vpu venc vdec af ae awb sns_full misc ini sys atomic)
elseif(CHIP STREQUAL "181x")
  set(MW_LIB sample isp cvi_bin_isp cvi_bin isp_algo vpu venc vdec af ae awb sns_full misc ini sys atomic)
elseif(CHIP STREQUAL "182x")
  set(MW_LIB sample isp cvi_bin_isp cvi_bin isp_algo vpu venc vdec cvi_vcodec sys awb ae af sns_full ini cvitracer)
elseif(CHIP STREQUAL "183x")
  set(MW_LIB sample isp cvi_bin_isp cvi_bin vpu venc vdec cvi_vcodec sys awb ae af sns_full ini)
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

add_executable(classifier_vpss_yuv
classifier_vpss_yuv.cpp)
    
target_link_libraries(classifier_vpss_yuv
    ${CVI_LIBS}
    ${OPENCV_LIBS}
    ${EXTRA_LIBS}
    ${MW_LIB})
install(TARGETS classifier_vpss_yuv
classifier_vpss_yuv DESTINATION samples_extra/bin)