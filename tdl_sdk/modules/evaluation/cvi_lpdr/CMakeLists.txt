project(cvi_lpdr)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils)
add_library(${PROJECT_NAME} OBJECT cvi_lpdr.cpp)