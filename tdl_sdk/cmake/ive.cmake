if(USE_TPU_IVE)
    if("${TPU_IVE_SDK_ROOT}" STREQUAL "")
        message(FATAL_ERROR "Missing ${TPU_IVE_SDK_ROOT}.")
    elseif(EXISTS "${TPU_IVE_SDK_ROOT}")
        message("-- Found TPU_IVE_SDK_ROOT (directory: ${TPU_IVE_SDK_ROOT})")
    else()
        message(FATAL_ERROR "${TPU_IVE_SDK_ROOT} is not a valid folder.")
    endif()

    set(IVE_INCLUDES ${TPU_IVE_SDK_ROOT}/include/)
    set(IVE_LIBS     ${TPU_IVE_SDK_ROOT}/lib/libcvi_ive_tpu.so)

    add_definitions(-DUSE_TPU_IVE)

    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(IVE_PATH ${CMAKE_INSTALL_PREFIX}/sample/3rd/ive)
    install(DIRECTORY ${TPU_IVE_SDK_ROOT}/include/ DESTINATION ${IVE_PATH}/include)
    install(DIRECTORY ${TPU_IVE_SDK_ROOT}/lib DESTINATION ${IVE_PATH})
    endif()
else()
    # Use standalone IVE hardware
    set(IVE_INCLUDES ${MIDDLEWARE_SDK_ROOT}/include/)
    set(IVE_LIBS     ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_ive.so)
endif()