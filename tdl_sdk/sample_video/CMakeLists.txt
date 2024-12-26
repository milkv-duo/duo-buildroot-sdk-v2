# Copyright 2020 cvitek Inc.

cmake_minimum_required(VERSION 3.2.2)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
project(ai_sdk_sample)

function(buildninstall)
  # From https://stackoverflow.com/questions/23327687/how-to-write-a-cmake-function-with-more-than-one-parameter-groups
  cmake_parse_arguments(
    PARSED_ARGS # prefix of output variables
    "" # list of names of the boolean arguments (only defined ones will be true)
    "NAME" # list of names of mono-valued arguments
    "INC;SRCS;DEPS" # list of names of multi-valued arguments (output variables are lists)
    ${ARGN} # arguments of the function to parse, here we take the all original ones
  )
  # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
  if(NOT PARSED_ARGS_NAME)
    message(FATAL_ERROR "You must provide a name")
  endif(NOT PARSED_ARGS_NAME)
  project(${PARSED_ARGS_NAME})
  # For debug purpose.
  # message("Provided additional sources are:")
  # foreach(src ${PARSED_ARGS_SRCS})
  #   message("- ${src}")
  # endforeach(src)
  include_directories(${PARSED_ARGS_INC})
  add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c ${PARSED_ARGS_SRCS})
  target_link_libraries(${PROJECT_NAME} ${PARSED_ARGS_DEPS} atomic)
  install(TARGETS ${PROJECT_NAME} DESTINATION bin)
  string(SUBSTRING "${PROJECT_NAME}" 0 10 substring1)
  if (substring1 STREQUAL "sample_app")
      install(FILES ${PROJECT_NAME}.c DESTINATION sample/cvi_tdl_app/)
  else ()
      install(FILES ${PROJECT_NAME}.c DESTINATION sample/cvi_tdl/)
  endif()
endfunction(buildninstall)

function(buildninstallcpp)
  # From https://stackoverflow.com/questions/23327687/how-to-write-a-cmake-function-with-more-than-one-parameter-groups
  cmake_parse_arguments(
    PARSED_ARGS # prefix of output variables
    "" # list of names of the boolean arguments (only defined ones will be true)
    "NAME" # list of names of mono-valued arguments
    "INC;SRCS;DEPS" # list of names of multi-valued arguments (output variables are lists)
    ${ARGN} # arguments of the function to parse, here we take the all original ones
  )
  # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
  if(NOT PARSED_ARGS_NAME)
    message(FATAL_ERROR "You must provide a name")
  endif(NOT PARSED_ARGS_NAME)
  project(${PARSED_ARGS_NAME})

  include_directories(${PARSED_ARGS_INC})
  add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp ${PARSED_ARGS_SRCS})
  target_link_libraries(${PROJECT_NAME} ${PARSED_ARGS_DEPS} atomic)
  install(TARGETS ${PROJECT_NAME} DESTINATION bin)
  string(SUBSTRING "${PROJECT_NAME}" 0 10 substring1)
  if (substring1 STREQUAL "sample_app")
      install(FILES ${PROJECT_NAME}.cpp DESTINATION sample/cvi_tdl_app/)
  else ()
      install(FILES ${PROJECT_NAME}.cpp DESTINATION sample/cvi_tdl/)
  endif()
endfunction(buildninstallcpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/lib/cvi_kit)
set(SAMPLE_INC
    ${MIDDLEWARE_SDK_ROOT}/include
    ${MIDDLEWARE_SDK_ROOT}/sample/common
    ${MIDDLEWARE_SDK_ROOT}/3rdparty/inih
    ${MLIR_SDK_PATH}/opencv/include
    ${MLIR_SDK_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_tdl_app
    ${CVI_RTSP_INCLUDE}
    ${IVE_INCLUDES}
)


if(${MW_VER} STREQUAL "v1")
  set(SAMPLE_INC ${SAMPLE_INC} ${TRACER_PATH})
elseif(${MW_VER} STREQUAL "v3")
  set(SAMPLE_INC ${SAMPLE_INC} ${MIDDLEWARE_SDK_ROOT}/component/panel/${CVI_PLATFORM_LOWER})
else()
  if(${CVI_PLATFORM} STREQUAL "SOPHON")
    set(SAMPLE_INC ${SAMPLE_INC} ${MIDDLEWARE_SDK_ROOT}/component/panel/cv186x)
  else()
    set(SAMPLE_INC ${SAMPLE_INC} ${MIDDLEWARE_SDK_ROOT}/component/panel/${CVI_PLATFORM_LOWER})
  endif()
endif()

set(AUDIO_SAMPLE_LIBS ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_ssp.so
                      ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_audio.so
                      ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_vqe.so
                      ${MIDDLEWARE_SDK_ROOT}/lib/libtinyalsa.so
                      ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_VoiceEngine.so
                      ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_RES1.so
)

if(${CONFIG_DUAL_OS} STREQUAL "ON")
  set(AUDIO_SAMPLE_LIBS
    ${AUDIO_SAMPLE_LIBS}
    ${MIDDLEWARE_SDK_ROOT}/lib/libdnvqe.so
    ${MIDDLEWARE_SDK_ROOT}/lib/3rd/libsbc.so
  )
else()
  set(AUDIO_SAMPLE_LIBS
    ${AUDIO_SAMPLE_LIBS}
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_ssp2.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_dnvqe.so
  )
endif()

set(SAMPLE_LIBS_FILE
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_platform.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_sys.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_vi.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_isp.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_vpss.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_venc.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_vo.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_bin.c
  ${MIDDLEWARE_SDK_ROOT}/sample/common/sample_common_peripheral.c
)

if(${CVI_PLATFORM} STREQUAL "SOPHON")
  set(SAMPLE_LIBS
    ${MIDDLEWARE_SDK_ROOT}/lib/libsys.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvi.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvpss.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvo.so
    ${MIDDLEWARE_SDK_ROOT}/lib/librgn.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libgdc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/3rd/libini.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libsns_full.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libisp.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libisp_algo.so
    ${CVI_RTSP_LIBPATH}
  )
  # set(AUDIO_SAMPLE_LIBS
  #     ${AUDIO_SAMPLE_LIBS}
  #     ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_dnvqe.so
  #     ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_ssp2.so
  # )

  add_definitions(-DSENSOR_GCORE_GC4653)
elseif(${CONFIG_DUAL_OS} STREQUAL "ON")
  set(SAMPLE_LIBS
    ${MIDDLEWARE_SDK_ROOT}/lib/libsys.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvi.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvo.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvpss.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libgdc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/librgn.so
    ${MIDDLEWARE_SDK_ROOT}/lib/3rd/libini.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libmsg.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvilink.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libsys.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin_isp.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libisp.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
    ${MLIR_SDK_ROOT}/lib/libz.so
    ${CVI_RTSP_LIBPATH}
  )
else()
  set(SAMPLE_LIBS
    ${MIDDLEWARE_SDK_ROOT}/lib/libsys.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvi.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvo.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvpss.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libgdc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/librgn.so
    ${MIDDLEWARE_SDK_ROOT}/lib/3rd/libini.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libsns_full.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libsample.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libisp.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin_isp.so
    ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
    ${MLIR_SDK_ROOT}/lib/libz.so
    ${CVI_RTSP_LIBPATH}
  )

  if("${MW_VER}" STREQUAL "v2")
    if(${CVI_PLATFORM} STRLESS "SOPHON")
      set(SAMPLE_LIBS
        ${MIDDLEWARE_SDK_ROOT}/lib/libsys.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvi.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvo.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvpss.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libgdc.so
        ${MIDDLEWARE_SDK_ROOT}/lib/librgn.so
        ${MIDDLEWARE_SDK_ROOT}/lib/3rd/libini.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libsns_full.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libsample.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libisp.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin_isp.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
        ${MLIR_SDK_ROOT}/lib/libz.so
        ${CVI_RTSP_LIBPATH}
      )
    endif()
    set(SAMPLE_LIBS ${SAMPLE_LIBS}
      ${MIDDLEWARE_SDK_ROOT}/lib/libmisc.so
    )
  else()
    set(SAMPLE_LIBS ${SAMPLE_LIBS}
      ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_vcodec.so
    )
  endif()

  if (NOT "${CVI_PLATFORM}" STREQUAL "CV183X")
    set(SAMPLE_LIBS ${SAMPLE_LIBS} ${MIDDLEWARE_SDK_ROOT}/lib/libisp_algo.so)
  endif()
endif()

if((DEFINED BUILD_WEB_VIEW) AND ("${CVI_PLATFORM}" STREQUAL "CV182X"))
  set(SAMPLE_LIBS_WEB ${SAMPLE_LIBS}
    ${sophapp_SOURCE_DIR}/prebuilt/thttpd/lib32bit/libthttpd.so
    ${sophapp_SOURCE_DIR}/prebuilt/openssl/lib32bit/libssl.so
    ${sophapp_SOURCE_DIR}/prebuilt/libwebsockets/lib32bit/libwebsockets.so)

  set(SAMPLE_INC_WEB ${SAMPLE_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket
    ${sophapp_SOURCE_DIR}/prebuilt/libwebsockets/include
    ${sophapp_SOURCE_DIR}/prebuilt/libwebsockets/include/libwebsockets
    ${sophapp_SOURCE_DIR}/prebuilt/libwebsockets/include/libwebsockets/abstract/protocols
    ${sophapp_SOURCE_DIR}/prebuilt/libwebsockets/include/libwebsockets/abstract/transports
    ${sophapp_SOURCE_DIR}/prebuilt/thttpd/include
    ${sophapp_SOURCE_DIR}/prebuilt/openssl/include
  )

  set(SAMPLE_SRC_WEB
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket/cJSON.c
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket/middleware_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket/app_ipcam_net.c
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket/app_ipcam_netctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/web_socket/app_ipcam_websocket.c)
endif()

# buildninstallcpp(NAME sample_read_od INC ${SAMPLE_INC} DEPS cvi_tdl ${SAMPLE_LIBS})
buildninstallcpp(NAME sample_aud_order INC ${SAMPLE_INC} DEPS cvi_tdl pthread ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
# buildninstall(NAME sample_img_md INC  ${SAMPLE_INC} DEPS cvi_tdl pthread ${SAMPLE_LIBS})
# buildninstall(NAME sample_init INC ${SAMPLE_INC} DEPS cvi_tdl ${SAMPLE_LIBS})
# buildninstall(NAME sample_aud_esc INC ${SAMPLE_INC} SRCS sample_utils.c DEPS cvi_tdl pthread ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
# buildninstall(NAME sample_aud_soundcmd INC ${SAMPLE_INC} DEPS cvi_tdl pthread ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
buildninstall(NAME sample_aud_cls_read INC ${SAMPLE_INC} DEPS cvi_tdl pthread ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
# buildninstall(NAME sample_aud_cry INC ${SAMPLE_INC} DEPS cvi_tdl pthread ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
# buildninstall(NAME sample_baby_aud_det INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} ${AUDIO_SAMPLE_LIBS})
# buildninstall(NAME sample_hand_detect INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_hand_keypoint INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_fq INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
buildninstall(NAME sample_vi_od INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_intrusion_det INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_obj_counting INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_fall INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_fall_det INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_dms INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_fdmask INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_lpdr INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_md INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_object_tracking INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_face_tracking INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_meeting INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
# buildninstall(NAME sample_vi_face_recog INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app ${SAMPLE_LIBS})
buildninstall(NAME sample_cap_face INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_cap_face_pet INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_adas INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_vi_fd INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
buildninstall(NAME sample_vi_fdlm INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_face_capture INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_irregular_personvehicle_cross INC ${SAMPLE_INC} SRCS vi_vo_utils ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_person_capture INC ${SAMPLE_INC} SRCS vi_vo_utils ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_face_mask_det INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_face_recognition INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_consumer_counting INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_personvehicle_cross INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS} )
buildninstall(NAME sample_stream_hardhat INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS})
buildninstall(NAME sample_stream_fr INC ${SAMPLE_INC} SRCS vi_vo_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_tdl_app pthread ${SAMPLE_LIBS})
buildninstall(NAME sample_clip INC ${SAMPLE_INC} SRCS middleware_utils.c sample_utils.c ${SAMPLE_LIBS_FILE} DEPS cvi_tdl cvi_kit ${SAMPLE_LIBS} )

if((DEFINED BUILD_WEB_VIEW) AND ("${CVI_PLATFORM}" STREQUAL "CV182X"))
  buildninstall(NAME sample_web_multi_model INC ${SAMPLE_INC_WEB} SRCS middleware_utils.c ${SAMPLE_SRC_WEB} DEPS cvi_tdl ${SAMPLE_LIBS_WEB} )
endif()

install(FILES vi_vo_utils.c sample_utils.c middleware_utils.c DESTINATION sample/utils)
install(FILES vi_vo_utils.h sample_utils.h middleware_utils.h DESTINATION sample/utils)
install(FILES Makefile DESTINATION sample/cvi_tdl)
install(FILES Makefile DESTINATION sample/cvi_tdl_app)
install(FILES Makefile DESTINATION sample/cvi_yolo)
