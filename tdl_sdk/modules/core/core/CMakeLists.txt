project(core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utils
                    ${IVE_INCLUDES})
add_library(${PROJECT_NAME} OBJECT vpss_engine.cpp core.cpp obj_detection.cpp face_detection.cpp pose_detection.cpp)