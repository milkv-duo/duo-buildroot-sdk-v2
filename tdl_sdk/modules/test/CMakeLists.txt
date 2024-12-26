project(test)

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
  target_link_libraries(${PROJECT_NAME} ${PARSED_ARGS_DEPS})
  install(TARGETS ${PROJECT_NAME} DESTINATION _testing)
endfunction(buildninstallcpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/cvi_kit)

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
    buildninstallcpp(NAME test_img_fall_monitor INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_fdfr INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_hrnet INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_simcc_pose INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_lane_det INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_lstr INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_ocr INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_pose INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_sr INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    # buildninstallcpp(NAME test_vpss_fuse_vehicle INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    # buildninstallcpp(NAME test_vpss_mosaic INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    # buildninstallcpp(NAME test_vpss_pd INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_lpd_lpr_v1 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_lpd_lpr_v2 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
    buildninstallcpp(NAME test_img_yolov8_seg INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
endif()

if("${CVI_PLATFORM}" STREQUAL "SOPHON")
    buildninstallcpp(NAME test_clip_image INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
endif()

buildninstallcpp(NAME test_face_attribute_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_od INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_fd INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_fdlm INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_fdln INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_hand_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_hand_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_pet_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_vehicle_detection INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_handfaceperson_det INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_md INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_img_hand_keypoint INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_face_landmarker_det2 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstallcpp(NAME test_image_classification INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstallcpp(NAME test_audio_cls INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_face_cap INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_face_cap_pet INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_capture INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_consumer_counting INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_irregular_people_vehicle INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_people_vehicle INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_dms_landmarks INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS})
buildninstallcpp(NAME test_clip_text INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_clip_pipeline INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app cvi_kit ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_hardhat_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_hardhat_retinaface INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_fire_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_img_stereo INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)
buildninstallcpp(NAME test_video_adas INC ${REG_INCLUDES} DEPS cvi_tdl atomic cvi_tdl_app pthread ${SAMPLE_LIBS} SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/sys_utils.cpp)

# yolo external test
buildninstallcpp(NAME test_yolo INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov5 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov5_roi INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov8 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov10 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov6 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolov7 INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_yolox INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
buildninstallcpp(NAME test_ppyoloe INC ${REG_INCLUDES} DEPS cvi_tdl atomic ${SAMPLE_LIBS})
