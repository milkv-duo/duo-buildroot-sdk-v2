add_subdirectory(digital_tracking)
add_subdirectory(draw_rect)
add_subdirectory(tracker)
add_subdirectory(area_detect)
if(NOT ${CVI_PLATFORM} STREQUAL "SOPHON")
add_subdirectory(feature_matching)
endif()

if(NOT DEFINED NO_OPENCV)
add_subdirectory(face_angle)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../service
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/ive)

set(FRS_SRCS cvi_tdl_service.cpp)

project(cvi_tdl_service)
message("add cvi tdl service")
get_directory_property(MYDEFS COMPILE_DEFINITIONS)
MESSAGE( STATUS "Compile defs contain: " ${MYDEFS} )
add_library(${PROJECT_NAME} OBJECT ${FRS_SRCS})