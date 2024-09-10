project(tamper_detection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../ive)
add_library(${PROJECT_NAME} OBJECT tamper_detection.cpp)
