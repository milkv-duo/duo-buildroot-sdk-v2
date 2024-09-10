project(lfw)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils)
add_library(${PROJECT_NAME} OBJECT lfw.cpp)
