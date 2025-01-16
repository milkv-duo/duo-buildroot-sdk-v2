

include_directories(${IVE_INCLUDES}
                    ${MIDDLEWARE_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/cvi_tdl_app
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/ive
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/motion_detection
                    ${CMAKE_CURRENT_SOURCE_DIR})

set(PROJ_SRCS $<TARGET_OBJECTS:ivewrapper> 
              $<TARGET_OBJECTS:motion_detection>
              ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils/ccl.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../../core/utils/profiler.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/cvi_md.cpp )
project(cvi_md)

set(MD_CORES_DEPS ${MIDDLEWARE_LIBS} ${IVE_LIBS})
add_library(${PROJECT_NAME} SHARED ${PROJ_SRCS})
target_link_libraries(${PROJECT_NAME} ${MD_CORES_DEPS})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

project(cvi_md-static)
add_library(${PROJECT_NAME} STATIC ${PROJ_SRCS})
target_link_libraries(${PROJECT_NAME} ${MD_CORES_DEPS})

SET_TARGET_PROPERTIES(cvi_md-static PROPERTIES OUTPUT_NAME "cvi_md")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cvi_md.h DESTINATION include/cvi_md)

