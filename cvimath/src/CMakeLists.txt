project(cvimath)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB SRC ./*.c ./*.cpp)

add_library(${PROJECT_NAME} SHARED ${SRC})
target_link_libraries(${PROJECT_NAME} ${TPU_KERNEL_LIB})
install(TARGETS ${PROJECT_NAME} DESTINATION lib)

add_library(${PROJECT_NAME}-static STATIC ${SRC})
target_link_libraries(${PROJECT_NAME}-static ${TPU_KERNEL_LIB})
install(TARGETS ${PROJECT_NAME}-static DESTINATION lib)
