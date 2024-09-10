project(face_pet_capture)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../sample
                    ${CMAKE_CURRENT_SOURCE_DIR}/../face_cap_utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/core/utils)
add_library(${PROJECT_NAME} OBJECT face_pet_capture.c)
