project(cvimath_sample)

# wrapper source

# include header
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${TPU_SDK_ROOT}/include
  ${TPU_SDK_ROOT}/include/cvimath
  )

# add libs
set( TPU_KERNEL_LIB "-L${TPU_SDK_ROOT}/lib -lcvikernel")
set( TEST_LIBS cvimath cviruntime)

file(GLOB CVI1835_SAMPLE ./*.cpp)

foreach(SAMPLE_SRC ${CVI1835_SAMPLE})
    get_filename_component(SAMPLE_NAME ${SAMPLE_SRC} NAME_WE)

    add_executable(${SAMPLE_NAME} ${SAMPLE_UTIL} ${SAMPLE_SRC})
    target_link_libraries(${SAMPLE_NAME} ${TPU_KERNEL_LIB} ${TEST_LIBS})
    set_target_properties(${SAMPLE_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${SAMPLE_NAME} DESTINATION bin)

    add_test(${SAMPLE_NAME} ${SAMPLE_NAME} ctest_test)

endforeach()
