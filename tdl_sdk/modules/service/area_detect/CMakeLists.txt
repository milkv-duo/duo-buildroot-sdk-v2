project(area_detect)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils)
add_library(${PROJECT_NAME} OBJECT intrusion_detect.cpp)
target_link_libraries(${PROJECT_NAME} $<TARGET_OBJECTS:tracker>)
