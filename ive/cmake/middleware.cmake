# Copyright 2018 Bitmain Inc.
# License
# Author Yangwen Huang <yangwen.huang@bitmain.com>


if("${MIDDLEWARE_SDK_ROOT}" STREQUAL "")
  message(FATAL_ERROR "You must set MIDDLEWARE_SDK_ROOT before building IVE library.")
elseif(EXISTS "${MIDDLEWARE_SDK_ROOT}")
  message("-- Found MIDDLEWARE_SDK_ROOT (directory: ${MIDDLEWARE_SDK_ROOT})")
else()
  message(FATAL_ERROR "${MIDDLEWARE_SDK_ROOT} is not a valid folder.")
endif()


if ("${CVI_PLATFORM}" STREQUAL "CV180X"  OR "${CVI_PLATFORM}" STREQUAL "CV181X")
  set(MIDDLEWARE_INCLUDES 
    ${KERNEL_ROOT}/include/
    )
else()
  set(MIDDLEWARE_INCLUDES 
    ${MIDDLEWARE_SDK_ROOT}/include/
    )
endif()

message("MIDDLEWARE_INCLUDES " ${MIDDLEWARE_INCLUDES})
include_directories(${MIDDLEWARE_INCLUDES})