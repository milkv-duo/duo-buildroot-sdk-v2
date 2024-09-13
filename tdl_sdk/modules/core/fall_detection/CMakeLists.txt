project(fall_detection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
add_library(${PROJECT_NAME} OBJECT fall_detection.cpp fall_det.cpp fall_det_monitor.cpp)
