# Copyright 2020 cvitek Inc.
cmake_minimum_required(VERSION 3.18.4)

project(sample)

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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_tdl_app 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/lib/cvi_kit)
set(REG_INCLUDES
    ${MIDDLEWARE_SDK_ROOT}/include
    ${MIDDLEWARE_SDK_ROOT}/sample/common
    ${MIDDLEWARE_SDK_ROOT}/component/isp/common
)
# CMAKE_CURRENT_SOURCE_DIR 指 当前cmakelists所在路径
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
    )
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
        ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin_isp.so
        ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
        ${MLIR_SDK_ROOT}/lib/libz.so
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
                ${MIDDLEWARE_SDK_ROOT}/lib/libvenc.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libvdec.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libawb.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libae.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libaf.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin_isp.so
                ${MIDDLEWARE_SDK_ROOT}/lib/libcvi_bin.so
                ${MLIR_SDK_ROOT}/lib/libz.so
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
if (NOT DEFINED NO_OPENCV)
    buildninstall(NAME sample_img_fall_monitor INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_fdfr INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_hrnet INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_simcc_pose INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_lane_det INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_lstr INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_ocr INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_pose INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_sr INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_kit ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_lpd_lpr_v1 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_lpd_lpr_v2 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstall(NAME sample_img_yolov8_seg INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS})
endif()

if("${CVI_PLATFORM}" STREQUAL "CV186X")
    buildninstall(NAME sample_clip_image INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
endif()

buildninstall(NAME sample_face_attribute_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_od INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_fd INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_fdln INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_hand_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_hand_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_pet_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_vehicle_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_handfaceperson_det INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_md INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_img_hand_keypoint INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_face_landmarker_det2 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstall(NAME sample_image_classification INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstall(NAME sample_audio_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_face_cap INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_face_cap_pet INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_capture INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_consumer_counting INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_irregular_people_vehicle INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_people_vehicle INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_dms_landmarks INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstall(NAME sample_clip_text INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_clip_pipeline INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_hardhat_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_hardhat_retinaface INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_fire_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)
buildninstall(NAME sample_img_stereo INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstall(NAME sample_video_adas INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.c)

# yolo external sample
buildninstall(NAME sample_yolo INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov5 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov5_roi INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov10 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov6 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolov7 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_yolox INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstall(NAME sample_ppyoloe INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
