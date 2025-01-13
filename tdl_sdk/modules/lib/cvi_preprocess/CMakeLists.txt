

include_directories(
                    ${MIDDLEWARE_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/cvi_tdl_app
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/core
                    )

set(PROJ_SRCS 
              ${CMAKE_CURRENT_SOURCE_DIR}/../../core/core/vpss_engine.cpp
              ${CMAKE_CURRENT_SOURCE_DIR}/cvi_bmcv.cpp
              )
project(cvi_preprocess)

set(PP_CORES_DEPS ${MIDDLEWARE_LIBS})
add_library(${PROJECT_NAME} SHARED ${PROJ_SRCS})
target_link_libraries(${PROJECT_NAME} ${PP_CORES_DEPS})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

project(cvi_preprocess-static)
add_library(${PROJECT_NAME} STATIC ${PROJ_SRCS})
target_link_libraries(${PROJECT_NAME} ${PP_CORES_DEPS})
SET_TARGET_PROPERTIES(cvi_preprocess-static PROPERTIES OUTPUT_NAME "cvi_preprocess")


install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cvi_bmcv.h DESTINATION include/cvi_preprocess)

