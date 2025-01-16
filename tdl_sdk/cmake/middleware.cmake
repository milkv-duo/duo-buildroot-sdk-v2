if("${MIDDLEWARE_SDK_ROOT}" STREQUAL "")
  message(FATAL_ERROR "You must set MIDDLEWARE_SDK_ROOT before building IVE library.")
elseif(EXISTS "${MIDDLEWARE_SDK_ROOT}")
  message("-- Found MIDDLEWARE_SDK_ROOT (directory: ${MIDDLEWARE_SDK_ROOT})")
else()
  message(FATAL_ERROR "${MIDDLEWARE_SDK_ROOT} is not a valid folder.")
endif()

if("${MW_VER}" STREQUAL "v1")
  add_definitions(-D_MIDDLEWARE_V1_)
elseif("${MW_VER}" STREQUAL "v2")
  add_definitions(-D_MIDDLEWARE_V2_)
endif()

string(TOLOWER ${CVI_PLATFORM} CVI_PLATFORM_LOWER)
if("${CVI_PLATFORM}" STREQUAL "SOPHON")
  set(ISP_HEADER_PATH ${MIDDLEWARE_SDK_ROOT}/modules/isp/include/cv186x)
else()
  set(ISP_HEADER_PATH ${MIDDLEWARE_SDK_ROOT}/include/isp/${CVI_PLATFORM_LOWER}/
                      ${MIDDLEWARE_SDK_ROOT}/component/isp/common
    )
endif()

set(MIDDLEWARE_INCLUDES ${ISP_HEADER_PATH}
                        ${MIDDLEWARE_SDK_ROOT}/include/
                        ${MIDDLEWARE_SDK_ROOT}/include/linux/
                        ${KERNEL_ROOT}/include/
)

set(MIDDLEWARE_PATH ${CMAKE_INSTALL_PREFIX}/sample/3rd/middleware/${MW_VER})
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  install(DIRECTORY ${MIDDLEWARE_SDK_ROOT}/include/ DESTINATION ${MIDDLEWARE_PATH}/include)
  install(DIRECTORY ${MIDDLEWARE_SDK_ROOT}/sample/common/ DESTINATION ${MIDDLEWARE_PATH}/include)
  install(DIRECTORY ${MIDDLEWARE_SDK_ROOT}/lib/ DESTINATION ${MIDDLEWARE_PATH}/lib)
endif()
