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
  set(PROJECT_URL "$ENV{TOP_DIR}/cvi_rtsp")
else()
  set(PROJECT_URL "https://github.com/sophgo/cvi_rtsp.git")
endif()

message(STATUS "PROJECT_URL for cvi_rtsp is: ${PROJECT_URL}")

set(SOURCE_DIR ${BUILD_DOWNLOAD_DIR}/cvi_rtsp)

if(NOT EXISTS "$ENV{TOP_DIR}/cvi_rtsp/src/libcvi_rtsp.so")
  if(NOT EXISTS ${SOURCE_DIR}/src/libcvi_rtsp.so)
    if(EXISTS "${PROJECT_URL}")
      message(STATUS "Copying cvi_rtsp from ${PROJECT_URL} to ${BUILD_DOWNLOAD_DIR}")
      execute_process(
        COMMAND cp -r ${PROJECT_URL} ${BUILD_DOWNLOAD_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE result
      )

      if(result EQUAL 0)
        message(STATUS "cvi_rtsp copied to ${BUILD_DOWNLOAD_DIR}")
        if(EXISTS "${SOURCE_DIR}/build.sh")
          message(STATUS "build.sh exists and is executable")
        endif()
        add_custom_command(
          OUTPUT ${SOURCE_DIR}/src/libcvi_rtsp.so
          COMMAND CROSS_COMPILE=${TC_PATH}${CROSS_COMPILE} SDK_VER=${RTSP_SDK_VER} ./build.sh
          WORKING_DIRECTORY ${BUILD_DOWNLOAD_DIR}/cvi_rtsp
          VERBATIM
          COMMENT "Building libcvi_rtsp from copied files"
        )

        add_custom_target(build_cvi_rtsp ALL DEPENDS ${SOURCE_DIR}/src/libcvi_rtsp.so)
      endif()

    elseif(PROJECT_URL MATCHES "^https://")
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

  endif()
else()
  set(SOURCE_DIR $ENV{TOP_DIR}/cvi_rtsp)
endif()

message(STATUS "cvi_rtsp SOURCE_DIR: ${SOURCE_DIR}")

set(CVI_RTSP_LIBPATH ${SOURCE_DIR}/src/libcvi_rtsp.so)
set(CVI_RTSP_INCLUDE ${SOURCE_DIR}/include/cvi_rtsp)

message(STATUS "CVI_RTSP_LIBPATH: ${CVI_RTSP_LIBPATH}")
message(STATUS "CVI_RTSP_INCLUDE: ${CVI_RTSP_INCLUDE}")

set(RTSP_PATH ${CMAKE_INSTALL_PREFIX}/sample/3rd/rtsp)
install(FILES ${CVI_RTSP_LIBPATH} DESTINATION ${RTSP_PATH}/lib)
install(DIRECTORY ${CVI_RTSP_INCLUDE} DESTINATION ${RTSP_PATH}/include)
