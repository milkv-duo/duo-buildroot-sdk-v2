get_filename_component(FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME_WE)

if ("${FILE_NAME}" MATCHES "uclibc")
  set(RTSP_SDK_VER "uclibc")
elseif("${FILE_NAME}" MATCHES "aarch64")
  set(RTSP_SDK_VER "64bit")
elseif("${FILE_NAME}" MATCHES "gnueabihf")
  set(RTSP_SDK_VER "32bit")
elseif("${FILE_NAME}" MATCHES "riscv64-linux")
  set(RTSP_SDK_VER "glibc_riscv64")
elseif("${FILE_NAME}" MATCHES "riscv64-musl")
  set(RTSP_SDK_VER "musl_riscv64")
else()
  set(RTSP_SDK_VER "unknown")
  message(WARNING "Unrecognized toolchain file: ${CMAKE_TOOLCHAIN_FILE}. Defaulting to RTSP_SDK_VER=${RTSP_SDK_VER}")
endif()

if(EXISTS $ENV{TOP_DIR}/cvi_rtsp)
  set(PROJECT_URL "file://$ENV{TOP_DIR}/cvi_rtsp")
else()
  set(PROJECT_URL "https://github.com/sophgo/cvi_rtsp.git")
endif()

set(SOURCE_DIR ${BUILD_DOWNLOAD_DIR}/cvi_rtsp/src/cvi_rtsp)

if(NOT EXISTS "$ENV{TOP_DIR}/cvi_rtsp/src/libcvi_rtsp.so")
  if(NOT EXISTS ${SOURCE_DIR}/src/libcvi_rtsp.so)
    ExternalProject_Add(cvi_rtsp
      GIT_REPOSITORY ${PROJECT_URL}
      PREFIX ${BUILD_DOWNLOAD_DIR}/cvi_rtsp
      BUILD_COMMAND CROSS_COMPILE=${TC_PATH}${CROSS_COMPILE} SDK_VER=${RTSP_SDK_VER} ./build.sh
      CONFIGURE_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_IN_SOURCE true
      BUILD_BYPRODUCTS <SOURCE_DIR>/src/libcvi_rtsp.so
    )
    ExternalProject_Get_property(cvi_rtsp SOURCE_DIR)  
    message("Content downloaded to ${SOURCE_DIR}")
  endif()
else()
  set(SOURCE_DIR $ENV{TOP_DIR}/cvi_rtsp)
endif()

set(CVI_RTSP_LIBPATH ${SOURCE_DIR}/src/libcvi_rtsp.so)
set(CVI_RTSP_INCLUDE ${SOURCE_DIR}/include/cvi_rtsp)

set(RTSP_PATH ${CMAKE_INSTALL_PREFIX}/sample/3rd/rtsp)
install(FILES ${CVI_RTSP_LIBPATH} DESTINATION ${RTSP_PATH}/lib)
install(DIRECTORY ${CVI_RTSP_INCLUDE} DESTINATION ${RTSP_PATH}/include)
