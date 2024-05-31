include(CMakeForceCompiler)
include($ENV{TOP_DIR}/build/config.cmake)
# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm-linux.cmake ../
# The Generic system name is used for embedded targets (targets without OS) in
# CMake
set( CMAKE_SYSTEM_NAME          Linux )
set( CMAKE_SYSTEM_PROCESSOR     ${CONFIG_ARCH} )

# The toolchain prefix for all toolchain executables
set( CROSS_COMPILE ${CONFIG_CROSS_COMPILE_SDK} )
set( ARCH ${CONFIG_ARCH} )

# specify the cross compiler. We force the compiler so that CMake doesn't
# attempt to build a simple test program as this will fail without us using
# the -nostartfiles option on the command line
set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)

# To build the tests, we need to set where the target environment containing
# the required library is. On Debian-like systems, this is
# /usr/aarch64-linux-gnu.
# SET(CMAKE_FIND_ROOT_PATH $ENV{TOOLCHAIN_TOPDIR})
# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# We must set the OBJCOPY setting into cache so that it's available to the
# whole project. Otherwise, this does not get set into the CACHE and therefore
# the build doesn't know what the OBJCOPY filepath is
set( CMAKE_OBJCOPY      ${CROSS_COMPILE}objcopy
	    CACHE FILEPATH "The toolchain objcopy command " FORCE )

# Set the CMAKE C flags (which should also be used by the assembler!
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -std=gnu11" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-to-int-cast" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers" )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "" )
set( CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "" )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-parentheses" )
if ("${CONFIG_CROSS_COMPILE_SDK}" STREQUAL "arm-linux-gnueabihf-")
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon" )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard -marm" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard -marm" )
  set( CMAKE_SYSROOT $ENV{TOP_DIR}/ramdisk/sysroot/sysroot-glibc-linaro-2.23-2017.05-arm-linux-gnueabihf)
elseif("${CONFIG_CROSS_COMPILE_SDK}" STREQUAL "aarch64-linux-gnu-")
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53" )
  set( CMAKE_SYSROOT $ENV{TOP_DIR}/ramdisk/sysroot/sysroot-glibc-linaro-2.23-2017.05-aarch64-linux-gnu)
elseif("${CONFIG_CROSS_COMPILE_SDK}" STREQUAL "riscv64-unknown-linux-gnu-")
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=c906fdv" )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=c906fdv" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d" )
  set( CMAKE_SYSROOT $ENV{TOP_DIR}/host-tools/gcc/riscv64-linux-x86_64/sysroot)
elseif("${CONFIG_CROSS_COMPILE_SDK}" STREQUAL "riscv64-unknown-linux-musl-")
  if (DEFINED ENV{RISCV_LEGACY})
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=c906" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64gcv_zfh_xthead" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=c906" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64gcv_zfh_xthead" )
  else()
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=c906fdv" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=c906fdv" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d" )
  endif()
  set( CMAKE_SYSROOT $ENV{TOP_DIR}/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot)
else()
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon" )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard -marm" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard -marm" )
  set( CMAKE_SYSROOT $ENV{TOP_DIR}/ramdisk/sysroot/sysroot-uclibc)
endif()
