project(draw_rect)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils
                    ${IVE_INCLUDES})
add_library(${PROJECT_NAME} OBJECT draw_rect.cpp)
