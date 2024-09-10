project(personvehicle_capture)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../sample)
add_library(${PROJECT_NAME} OBJECT personvehicle_capture.c)
