project(cvimath)

cmake_minimum_required(VERSION 3.2.2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH "\${ORIGIN}/../lib;\${ORIGIN}/")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
  message("No toolchain file found. Using host compiler.")
  if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")
  endif()
else()
  if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install_soc")
  endif()
endif()

set(CMAKE_C_INIT "-fsigned-char -fPIC -Werror=all -fdiagnostics-color=always")
set(CMAKE_CXX_INIT "-fsigned-char -fPIC -Werror=all -fdiagnostics-color=always -std=gnu++11")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_INIT} -O3" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_INIT} -O3" )
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set( SAFETY_FLAGS "-Werror -Wall -Wextra -ggdb -fno-strict-aliasing")
  set( SAFETY_FLAGS "${SAFETY_FLAGS} -fsanitize=address")
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_INIT} -g -O0 ${SAFETY_FLAGS}")
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_INIT} -g -O0 ${SAFETY_FLAGS}" )
else()
  message(FATAL_ERROR "No build type!!!")
endif()

message("==================================================")
message("[Summary]")
message("C   compiler ${CMAKE_C_COMPILER}")
message("CXX compiler ${CMAKE_CXX_COMPILER}")
message("Build type   ${CMAKE_BUILD_TYPE}")
message("Install dir  ${CMAKE_INSTALL_PREFIX}")
message("==================================================")

# Add externel libs
set( TPU_LD "-L${TPU_SDK_ROOT}/lib")
set( TPU_KERNEL_LIB "${TPU_LD} -lcvikernel")
# wait cvimath/cviruntime so are generated
set( TEST_LIBS cvimath cviruntime)

# Add include path and set tpu libraries.
include_directories(
  ${TPU_SDK_ROOT}/include
  ${CVI_EXTRA}/include
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

# https://stackoverflow.com/questions/30250494/ctest-not-detecting-tests
enable_testing()

# ctest config
if (NOT CMAKE_CROSSCOMPILING)
  if (ENABLE_TEST STREQUAL "ON")
    add_subdirectory(tests)
  endif()
endif()

add_subdirectory(src)
add_subdirectory(sample)

# export header
file(GLOB HEADERS
  include/cvimath.h
  include/cvimath_internal.h
  include/test_cvikernel_util.h
  )

# export sample
#file(GLOB SAMPLES sample/*)

#install(FILES ${SAMPLES} DESTINATION samples/cvimath)
install(FILES ${CMAKE_SOURCE_DIR}/toolchain/toolchain-aarch64-linux.cmake DESTINATION samples/cvimath)
install(FILES ${HEADERS} DESTINATION include/cvimath)
