project(license_plate_detection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
add_library(${PROJECT_NAME} OBJECT license_plate_detection.cpp
                                   license_plate_detection_utils.cpp)
