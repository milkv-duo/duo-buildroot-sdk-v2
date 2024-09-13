project(super_resolution)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)

add_library(${PROJECT_NAME} OBJECT super_resolution.cpp)

