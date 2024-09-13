project(yolov10)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../)
add_library(${PROJECT_NAME} OBJECT yolov10.cpp)
