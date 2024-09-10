project(mask_face_recognition)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
add_library(${PROJECT_NAME} OBJECT mask_face_recognition.cpp)
