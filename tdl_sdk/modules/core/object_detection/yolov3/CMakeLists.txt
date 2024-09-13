project(yolov3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../)
add_library(${PROJECT_NAME} OBJECT yolov3.cpp
                                   yolov3_utils.cpp)
