cmake_minimum_required(VERSION 3.18.4)
project(cvi_tdl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")
endif()

# Find toolchain cmake file and toolchain folder
if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Aborting.")
endif()

set(CMAKE_C_INIT "-fsigned-char -fdiagnostics-color=always -std=gnu11")
set(CMAKE_CXX_INIT "-fsigned-char -fPIC -fdiagnostics-color=always -std=gnu++11 -pthread")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if("${CVI_PLATFORM}" STREQUAL "CV181X")
  add_definitions(-D__CV181X__)
elseif("${CVI_PLATFORM}" STREQUAL "CV180X")
  add_definitions(-D__CV180X__)
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "SDKRelease")
    set(NO_OPENCV ON)
    add_definitions(-DNO_OPENCV)
  endif()
elseif("${CVI_PLATFORM}" STREQUAL "CV182X")
  add_definitions(-D__CV182X__)
  include (cmake/tracer.cmake)
elseif("${CVI_PLATFORM}" STREQUAL "CV183X")
  add_definitions(-D__CV183X__)
  include (cmake/tracer.cmake)
  add_definitions(-DUSE_NEON)
elseif("${CVI_PLATFORM}" STREQUAL "SOPHON")
  add_definitions(-D__CV186X__)
else()
  message(FATAL_ERROR "Unrecognized platform ${CVI_PLATFORM}.")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} ${CMAKE_C_INIT} -s" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_INIT} -s" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "SDKRelease")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} ${CMAKE_C_INIT} -s" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_INIT} -s" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_INIT}" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_INIT}" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Asan")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_INIT} -fsanitize=address" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_INIT} -fsanitize=address" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Tsan")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_INIT} -fsanitize=thread" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_INIT} -fsanitize=thread" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "UBsan")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_INIT} -fsanitize=undefined" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_INIT} -fsanitize=undefined" )
    set( CMAKE_EXE_LINKER_FLAGS "-lubsan")
else()
    message(FATAL_ERROR "No build type!!!")
endif()

if(DEFINED ENV{PERF_EVAL})
  add_definitions(-DPERF_EVAL)
  message("set PERF_EVAL")
  endif()

# Get current commit
execute_process(COMMAND git show --oneline -s WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE CURRENT_COMMIT)
execute_process(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE LATEST_BRANCH)
string(STRIP ${LATEST_BRANCH} LATEST_BRANCH)

if("${LATEST_BRANCH}" STREQUAL "master")
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "nightly-${COMMITID}")
elseif("${LATEST_BRANCH}" STREQUAL "HEAD")
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "${COMMITID}")
else()
  string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)*[^(\\\n).]+" LATEST_TAG ${LATEST_BRANCH})
  if("${LATEST_TAG}" STREQUAL "")
    set(LATEST_TAG "unknown")
  endif()
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "${LATEST_TAG}-${COMMITID}")
endif()

foreach(flags CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    string(REPLACE "-O3" "-Os" ${flags} "${${flags}}")
endforeach()

include(${CMAKE_TOOLCHAIN_FILE})
set(KERNEL_ROOT $ENV{KERNEL_PATH}/build/$ENV{PROJECT_FULLNAME}/${ARCH}/usr/)

message("==================================================")
message("Branch                 ${LATEST_BRANCH}")
message("Latest tag             ${LATEST_TAG}")
message("Commit                 ${CURRENT_COMMIT}")
message("[Summary]")
message("Build type             ${CMAKE_BUILD_TYPE}")
message("C   compiler           ${CMAKE_C_COMPILER}")
message("CXX compiler           ${CMAKE_CXX_COMPILER}")
message("C Flags                ${CMAKE_C_FLAGS}")
message("CXX Flags              ${CMAKE_CXX_FLAGS}")
message("Install dir            ${CMAKE_INSTALL_PREFIX}")
message("CMAKE_TOOLCHAIN_FILE   ${CMAKE_TOOLCHAIN_FILE}")
message("Enable Perfetto        ${ENABLE_PERFETTO}")
message("CVI_PLATFORM           ${CVI_PLATFORM}")
message("USE_TPU_IVE            ${USE_TPU_IVE}")
message("KERNEL_ROOT            ${KERNEL_ROOT}")
message("CONFIG_DUAL_OS         ${CONFIG_DUAL_OS}")
message("==================================================")

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
include(FetchContent)
get_filename_component(_deps "${BUILD_DOWNLOAD_DIR}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
set(FETCHCONTENT_BASE_DIR ${_deps})

# Check if OpenCV exists.
if (NOT DEFINED NO_OPENCV)
include(cmake/opencv.cmake)
endif()

# Check if MLIR SDK exists.
include(cmake/mlir.cmake)

# Check if mpi SDK exists.
include(cmake/middleware.cmake)

# 3rdparty libraries
include(cmake/thirdparty.cmake)

include(cmake/ive.cmake)

# ===========need remove this=============
include(cmake/cvi_rtsp.cmake)
# ===========need remove this=============

# 3rdpart includes
include_directories(${IVE_INCLUDES}
                    ${OPENCV_INCLUDES}
                    ${MLIR_INCLUDES}
                    ${MIDDLEWARE_INCLUDES})

if("${BUILD_OPTION}" STREQUAL "")
  add_subdirectory(modules)
elseif("${BUILD_OPTION}" STREQUAL "sample")
  add_subdirectory(sample)
elseif("${BUILD_OPTION}" STREQUAL "all")
  add_subdirectory(modules)
  add_subdirectory(sample)
else()
  message(FATAL_ERROR "Error: BUILD_OPTION is set to an invalid value: ${BUILD_OPTION}. Valid options are '', 'sample', or 'all'.")
endif()

add_subdirectory(sample_video)
