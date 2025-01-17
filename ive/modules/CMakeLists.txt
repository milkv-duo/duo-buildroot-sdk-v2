add_subdirectory(motion_detection)
include_directories(${CMAKE_SOURCE_DIR}/include
                    ${MLIR_INCLUDES}
                    ${MIDDLEWARE_INCLUDES})
set(SRCS $<TARGET_OBJECTS:motion_detection>
         modules_core.cpp)
# Modules
project(cvi_ive_module)
add_library(${PROJECT_NAME} SHARED ${SRCS})
target_link_libraries(${PROJECT_NAME} cvi_ive_tpu)
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_module_core.h DESTINATION include/ive)