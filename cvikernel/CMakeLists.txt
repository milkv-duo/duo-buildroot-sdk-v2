cmake_minimum_required(VERSION 3.1.0)

project(cvikernel C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(SAFETY_FLAGS "-Werror -Wall -Wextra -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SAFETY_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SAFETY_FLAGS}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

install(FILES include/bmkernel/bm_kernel.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/bm_kernel_legacy.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/bm_regcpu.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/reg_tiu.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/reg_tdma.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/reg_bdcast.h
  DESTINATION include/bmkernel)
install(FILES include/bmkernel/bm1880v2/bmkernel_1880v2.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/non_atomic.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/1880v2_fp_convert.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/bm_vlc_compress.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/compression.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/bm1880v2_tiu_reg.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/bm1880v2_tdma_reg.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1880v2/bm1880v2_tpu_cfg.h
  DESTINATION include/bmkernel/bm1880v2)
install(FILES include/bmkernel/bm1822/bmkernel_1822.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/1822_fp_convert.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/bm_vlc_compress.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/compression.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/bm1822_tiu_reg.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/bm1822_tdma_reg.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/bmkernel/bm1822/bm1822_tpu_cfg.h
  DESTINATION include/bmkernel/bm1822)
install(FILES include/cvikernel/cv181x/cv181x_tiu_reg.h
  DESTINATION include/cvikernel/cv181x)
install(FILES include/cvikernel/cv181x/cv181x_tdma_reg.h
  DESTINATION include/cvikernel/cv181x)
install(FILES include/cvikernel/cv181x/cv181x_tpu_cfg.h
  DESTINATION include/cvikernel/cv181x)
install(FILES include/cvikernel/cv180x/cv180x_tiu_reg.h
  DESTINATION include/cvikernel/cv180x)
install(FILES include/cvikernel/cv180x/cv180x_tdma_reg.h
  DESTINATION include/cvikernel/cv180x)
install(FILES include/cvikernel/cv180x/cv180x_tpu_cfg.h
  DESTINATION include/cvikernel/cv180x)
install(FILES include/cvikernel/cvikernel.h
  DESTINATION include/cvikernel)
install(FILES include/cvikernel/cvk_fp_convert.h
  DESTINATION include/cvikernel)
install(FILES include/cvikernel/cvk_vlc_compress.h
  DESTINATION include/cvikernel)
enable_testing()

include_directories(include)
include_directories(src)

file(GLOB COMMON_SOURCES "src/*.c")
file(GLOB_RECURSE BM1822_SOURCES "src/bm1822/*.c")
file(GLOB_RECURSE BM1880v2_SOURCES "src/bm1880v2/*.c")
file(GLOB_RECURSE CV1822_SOURCES "src/cv1822/*.c")
file(GLOB_RECURSE CV1880v2_SOURCES "src/cv1880v2/*.c")
file(GLOB_RECURSE CV181X_SOURCES "src/cv181x/*.c")
file(GLOB_RECURSE CV180X_SOURCES "src/cv180x/*.c")

SET(_SOURCES ${COMMON_SOURCES})
SET(_SOURCES ${_SOURCES} ${BM1822_SOURCES})
SET(_SOURCES ${_SOURCES} ${CV1822_SOURCES})

if (CHIP STREQUAL "cv181x")
SET(_SOURCES ${_SOURCES} ${CV181X_SOURCES})
add_definitions(-DCHIPID=0x3)
elseif (CHIP STREQUAL "cv180x")
SET(_SOURCES ${_SOURCES} ${CV180X_SOURCES})
add_definitions(-DCHIPID=0x4)
elseif(CHIP STREQUAL "cv183x")
add_definitions(-DCHIPID=0x1)
SET(_SOURCES ${_SOURCES} ${BM1880v2_SOURCES})
SET(_SOURCES ${_SOURCES} ${CV1880v2_SOURCES})
elseif(CHIP STREQUAL "cv182x")
add_definitions(-DCHIPID=0x2)
else()
# pc cmodel
add_definitions(-DCHIPID=0x0)
SET(_SOURCES ${_SOURCES} ${CV180X_SOURCES})
SET(_SOURCES ${_SOURCES} ${CV181X_SOURCES})
SET(_SOURCES ${_SOURCES} ${BM1880v2_SOURCES})
SET(_SOURCES ${_SOURCES} ${CV1880v2_SOURCES})
endif()

#
# check for `enum-compare`
# for c compiler not treat enum-compare as [error](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=30357), we could leverage c++ compiler check temporary
# you could refer [here](https://stackoverflow.com/questions/7690800/can-cmake-use-g-to-compile-c-files) for more details
# and [here](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html) for compiler options c/c++
# default check is off
#file(GLOB_RECURSE CHECK_SOURCE "src/bm1880v2/non_atomic/*.c")
#set_source_files_properties(${CHECK_SOURCE} PROPERTIES LANGUAGE CXX )

add_library(cvikernel SHARED ${_SOURCES})
add_library(cvikernel-static STATIC ${_SOURCES})
target_link_libraries(cvikernel m) # m for <math.h>

install(TARGETS cvikernel cvikernel-static DESTINATION lib)

set(CVI_LIBS ${CVI_LIBS} cvikernel)
