# Copyright 2020 Cvitek Inc.
# License
# Author Yangwen Huang <yangwen.huang@cvitek.com>

if("${IVE_SDK_ROOT}" STREQUAL "")
  message(FATAL_ERROR "Missing ${IVE_SDK_ROOT}.")
elseif(EXISTS "${IVE_SDK_ROOT}")
  message("-- Found IVE_SDK_ROOT (directory: ${IVE_SDK_ROOT})")
else()
  message(FATAL_ERROR "${IVE_SDK_ROOT} is not a valid folder.")
endif()

set(IVE_INCLUDES
    ${IVE_SDK_ROOT}/include/
)

set(IVE_LIBS
    ${IVE_SDK_ROOT}/lib/libcvi_ive_tpu.so
)

install(DIRECTORY ${IVE_SDK_ROOT}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/)
install(FILES ${IVE_LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)