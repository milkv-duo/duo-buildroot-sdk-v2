project(cvimath)

include(CTest)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB _TEST_UTILS "${CMAKE_CURRENT_SOURCE_DIR}/common/*c")

# cvi1835 test
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    )
file(GLOB CVI1835_TESTS cvi1835/*.cpp)

# FIXME: repair test case
list(FILTER CVI1835_TESTS EXCLUDE REGEX ".*atan2.*")
list(FILTER CVI1835_TESTS EXCLUDE REGEX ".*depthwise_reshape_same.*")

foreach(TEST_SRC ${CVI1835_TESTS})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${_TEST_UTILS} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${TPU_KERNEL_LIB} ${TEST_LIBS})
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
endforeach()

#add_library(${PROJECT_NAME} SHARED ${SRC})
#target_link_libraries(${PROJECT_NAME} ${TPU_KERNEL_LIB})
#install(TARGETS ${PROJECT_NAME} DESTINATION tests)

