cmake_minimum_required(VERSION 3.18.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(cvitdl)

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install_soc")
endif()
# Find toolchain cmake file and toolchain folder
if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Aborting.")
endif()

set(FTP_SERVER_IP "10.80.0.5" CACHE STRING "3rd party resource ftp")
set(DL_SERVER_IP 10.132.3.76)

set(CMAKE_C_INIT "-fsigned-char -fdiagnostics-color=always -std=gnu11")
set(CMAKE_CXX_INIT "-fsigned-char -fPIC -fdiagnostics-color=always -std=gnu++17 -pthread")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if("${CVI_PLATFORM}" STREQUAL "CV181X")
  add_definitions(-DCV181X)
  add_definitions(-D__CV181X__)
elseif("${CVI_PLATFORM}" STREQUAL "CV180X")
  add_definitions(-DCV180X)
  add_definitions(-D__CV180X__)
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "SDKRelease")
    set(NO_OPENCV ON)
    add_definitions(-DNO_OPENCV)
  endif()
elseif("${CVI_PLATFORM}" STREQUAL "CV182X")
  add_definitions(-DCV182X)
  include (cmake/tracer.cmake)
elseif("${CVI_PLATFORM}" STREQUAL "CV183X")
  add_definitions(-DCV183X)
  include (cmake/tracer.cmake)
  add_definitions(-DUSE_NEON)
elseif("${CVI_PLATFORM}" STREQUAL "CV186X")
  add_definitions(-DCV186X)
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

if(DEFINED ENV{WEB_VIEW})
  set(BUILD_WEB_VIEW ON)
  message("set BUILD_WEB_VIEW ON")
endif()

if(DEFINED ENV{PERF_EVAL})
  add_definitions(-DPERF_EVAL)
  message("set PERF_EVAL")
  endif()

# Get current commit
execute_process(COMMAND git show --oneline -s WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE CURRENT_COMMIT)
execute_process(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE LATEST_BRANCH)
string(STRIP ${LATEST_BRANCH} LATEST_BRANCH)

if ("${LATEST_BRANCH}" STREQUAL "master")
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "nightly-${COMMITID}")
elseif("${LATEST_BRANCH}" STREQUAL "HEAD")
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "${COMMITID}")
else()
  string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)*[^(\\\n).]+" LATEST_TAG ${LATEST_BRANCH})
  if ("${LATEST_TAG}" STREQUAL "")
    set(LATEST_TAG "unknown")
  endif()
  string(REPLACE " " ";" WORD_LIST ${CURRENT_COMMIT})
  list(GET WORD_LIST 0 COMMITID)
  set(LATEST_TAG "${LATEST_TAG}-${COMMITID}")
endif()

foreach(flags CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    string(REPLACE "-O3" "-Os" ${flags} "${${flags}}")
endforeach()

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
message("Enable Perfetto        ${ENABLE_PERFETTO}")
message("CVI_PLATFORM           ${CVI_PLATFORM}")
message("CVI_SYSTEM_PROCESSOR   ${CVI_SYSTEM_PROCESSOR}")
message("CMAKE_TOOLCHAIN_FILE   ${CMAKE_TOOLCHAIN_FILE}")
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

# Check if middleware exists.
include (cmake/middleware.cmake)

# 3rdparty libraries
include(cmake/thirdparty.cmake)

include (cmake/ive.cmake)

include (cmake/cvi_rtsp.cmake)

# 3rdpart includes
include_directories(${IVE_INCLUDES}
                    ${OPENCV_INCLUDES}
                    ${MLIR_INCLUDES}
                    ${MIDDLEWARE_INCLUDES})

add_subdirectory(modules)
add_subdirectory(sample)
add_subdirectory(lib)
