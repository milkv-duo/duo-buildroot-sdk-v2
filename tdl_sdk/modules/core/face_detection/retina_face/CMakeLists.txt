project(retina_face)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../)
add_library(${PROJECT_NAME} OBJECT anchor_generator.cpp
                                   retina_face.cpp
                                   scrfd_face.cpp)