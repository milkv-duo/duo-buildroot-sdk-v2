

include_directories(${MIDDLEWARE_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/service/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/ive
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/cvi_tdl_app 
                    ${CMAKE_CURRENT_SOURCE_DIR})

set(PROJ_SRCS   $<TARGET_OBJECTS:utils>
                ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/cvi_tdl_utils.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/core/cvi_tdl_types_mem.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/service/draw_rect/draw_rect.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/cvi_draw_rect.cpp )

project(cvi_draw_rect)
add_library(${PROJECT_NAME} SHARED ${PROJ_SRCS})
install(TARGETS ${PROJECT_NAME} DESTINATION lib)

project(cvi_draw_rect-static)
add_library(${PROJECT_NAME} STATIC ${PROJ_SRCS})
SET_TARGET_PROPERTIES(cvi_draw_rect-static PROPERTIES OUTPUT_NAME "cvi_draw_rect")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cvi_draw_rect.h DESTINATION include/cvi_draw_rect)
