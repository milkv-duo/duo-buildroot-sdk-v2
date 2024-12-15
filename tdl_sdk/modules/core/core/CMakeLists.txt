project(core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utils
                    ${IVE_INCLUDES})
if("${CVI_PLATFORM}" STREQUAL "CV186X")
add_library(${PROJECT_NAME} OBJECT vpss_engine.cpp core_a2.cpp obj_detection.cpp face_detection.cpp pose_detection.cpp)
else()
add_library(${PROJECT_NAME} OBJECT vpss_engine.cpp core.cpp obj_detection.cpp face_detection.cpp pose_detection.cpp)
endif()
